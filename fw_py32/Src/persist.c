/*! *******************************************************************************************************
* Copyright (c) 2021-2022 Hekk_Elek
*
* \file persist.h
*
* \brief Storage for persistent data
*
* \author Hekk_Elek
*
**********************************************************************************************************/

/***************************************< Includes >**************************************/
// Standard C libraries
#include <string.h>

// Own includes
#include "types.h"
#include "util.h"
#include "persist.h"


/***************************************< Definitions >**************************************/
#define SAVE_SIZE               (4096u)  //!< Number of bytes present as save space (== flash sector size)
#define SAVE_BASEADDRESS  (0x08004000u)  //!< Base address of save space (last 4 kBytes sector of the 20 kbytes flash)


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/
S_PERSIST       gsPersistentData;  //!< Globally accessible persistent data structure
S_PERSIST CODE* gpsNextSaveSlot;   //!< Address of the next persistent save slot


/***************************************< Static function definitions >**************************************/
static BOOL IsSaveBlockEmpty( S_PERSIST* psLocalCopy, S_PERSIST CODE* psSaveBlock );
static BOOL SearchForLatestSave( S_PERSIST CODE** ppsNextEmpty );
static void Flash_Write( U32 u32Address, U8* pu8Data, U8 u8DataLength );
static void Flash_EraseSector( U32 u32Address );
static void Flash_Read( U32 u32Address, U8* pu8Data, U8 u8DataLength );


/***************************************< Private functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Check if given block is empty in the EEPROM
//! \param  psTemp: pointer to a temporary storage
//! \param  psSaveBlock: pointer to the block in EEPROM
//! \return TRUE if the block is empty; FALSE if not
//! \global -
//-----------------------------------------------------------------------------
static BOOL IsSaveBlockEmpty( S_PERSIST* psTemp, S_PERSIST CODE* psSaveBlock )
{
  BOOL bEmpty = TRUE;
  U8  u8ByteIndex;

  Flash_Read( (U32)psSaveBlock, (U8*)psTemp, sizeof( S_PERSIST ) );
  for( u8ByteIndex = 0u; u8ByteIndex < sizeof( S_PERSIST ); u8ByteIndex++ )
  {
    if( 0xFFu != ((U8*)psTemp)[ u8ByteIndex ] )
    {
      bEmpty = FALSE;
    }
  }
  return bEmpty;
}

//----------------------------------------------------------------------------
//! \brief  Search for the latest save in EEPROM
//! \param  ppsNextEmpty: address of the next empty block for writing
//! \return TRUE, if it found a correct save; FALSE if not
//! \global gsPersistentData
//! \note   Takes some time, should only be called in init block.
//-----------------------------------------------------------------------------
static BOOL SearchForLatestSave( S_PERSIST CODE** ppsNextEmpty )
{
  BOOL bEmpty = FALSE;
  U16 u16SaveIndex;
  BOOL bReturn = FALSE;
  S_PERSIST CODE* psSave = (S_PERSIST CODE*)SAVE_BASEADDRESS;
  S_PERSIST  sLocalCopy;
  
  for( u16SaveIndex = 0u; u16SaveIndex < ( SAVE_SIZE / sizeof( S_PERSIST ) ); u16SaveIndex++ )
  {
    Flash_Read( (U32)psSave, (U8*)&sLocalCopy, sizeof( S_PERSIST ) );
    if( sLocalCopy.u16CRC == Util_CRC16( (U8*)&sLocalCopy, sizeof( S_PERSIST ) - sizeof( U16 ) ) )
    {
      memcpy( &gsPersistentData, &sLocalCopy, sizeof( S_PERSIST ) );
      bReturn = TRUE;
    }
    else  // bad CRC
    {
      // Check if this block is empty
      bEmpty = IsSaveBlockEmpty( &sLocalCopy, psSave );
      if( TRUE == bEmpty )
      {
        *ppsNextEmpty = psSave;
        break;
      }
    }
    psSave++;
  }
  // If there was no space left
  if( FALSE == bEmpty )
  {
    Flash_EraseSector( SAVE_BASEADDRESS );
    // If the persistent data was correct
    if( TRUE == bReturn )
    {
      gpsNextSaveSlot = (S_PERSIST CODE*)SAVE_BASEADDRESS;
      Persist_Save();
    }
  }
  
  return bReturn;
}
//----------------------------------------------------------------------------
//! \brief  Write data block to flash from a given address
//! \param  u32Address: Start address to be written.
//! \param  pu8Data: pointer to the data to be written
//! \param  u8DataLength: write length
//! \return -
//! \global -
//! \note   Stalls the CPU.
//-----------------------------------------------------------------------------
static void Flash_Write( U32 u32Address, U8* pu8Data, U8 u8DataLength )
{
  U32 u32PageBaseAddress = u32Address & 0xFFFFFF80u;
  U32 u32CurrentAddress;
  
  DISABLE_IT;
  
  // Unlock flash
  while( READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0x00U )
  {
    WRITE_REG(FLASH->KEYR, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR, FLASH_KEY2);
  }
  
  // Start buffering page contents
  SET_BIT( FLASH->CR, FLASH_CR_PG );
  
  // Write page buffer -- NOTE: we're building on the fact that the persistent data is aligned to 4
  for( u32CurrentAddress = u32PageBaseAddress; u32CurrentAddress < u32PageBaseAddress + FLASH_PAGE_SIZE; u32CurrentAddress += sizeof( U32 ) )
  {
    if( ( u32CurrentAddress < u32Address )
     || ( u32CurrentAddress >= u32Address + u8DataLength ) )
    {
      *((U32*)u32CurrentAddress) = 0xFFFFFFFFu;
    }
    else
    {
      *((U32*)u32CurrentAddress) = *(U32*)&pu8Data[ u32CurrentAddress - u32Address ];
    }
    if( u32CurrentAddress + 2u*sizeof( U32 ) == u32PageBaseAddress + FLASH_PAGE_SIZE )
    {
      // Start page write after the next bus access
      SET_BIT( FLASH->CR, FLASH_CR_PGSTRT );
    }
  }
  
  // Wait for operation to finish
  while( FLASH->SR & FLASH_SR_BSY );
  
  // Re-lock flash
  SET_BIT(FLASH->CR, FLASH_CR_LOCK);
  
  ENABLE_IT;
}

//----------------------------------------------------------------------------
//! \brief  Erases one sector in flash
//! \param  u16Address: Start address of the sector
//! \return -
//! \global -
//! \note   Stalls the CPU.
//-----------------------------------------------------------------------------
static void Flash_EraseSector( U32 u32Address )
{
  DISABLE_IT;
  
  // Unlock flash
  while( READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0x00U )
  {
    WRITE_REG(FLASH->KEYR, FLASH_KEY1);
    WRITE_REG(FLASH->KEYR, FLASH_KEY2);
  }
  
  SET_BIT( FLASH->CR, FLASH_CR_SER );
  *(U32*)u32Address = 0xFFu;
  
  // Wait for operation to finish
  while( FLASH->SR & FLASH_SR_BSY );
  
  // Re-lock flash
  SET_BIT(FLASH->CR, FLASH_CR_LOCK);
  
  ENABLE_IT;
}

//----------------------------------------------------------------------------
//! \brief  Reads given number of bytes from the save space
//! \param  u32Address: Start address to be read
//! \param  pu8Data: data read from flash are written here
//! \param  u8DataLength: read length
//! \return -
//! \global -
//! \note   Stalls the CPU.
//-----------------------------------------------------------------------------
static void Flash_Read( U32 u32Address, U8* pu8Data, U8 u8DataLength )
{
  DISABLE_IT;

  (void)memcpy( pu8Data, (U8*)u32Address, u8DataLength );

  ENABLE_IT;
}


/***************************************< Public functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Initializes module and loads previously saved data
//! \param  -
//! \return -
//! \global All globals from this module
//! \note   Should be called from init block
//-----------------------------------------------------------------------------
void Persist_Init( void )
{
  // Find latest save and load it
  if( TRUE == SearchForLatestSave( &gpsNextSaveSlot ) )
  {
    // persistent data are loaded to memory
  }
  else  // Default values
  {
    memset( &gsPersistentData, 0, sizeof( S_PERSIST ) );
    gpsNextSaveSlot = (S_PERSIST CODE*)SAVE_BASEADDRESS;
  }
}

//----------------------------------------------------------------------------
//! \brief  Saves the current persistent data structure
//! \param  -
//! \return -
//! \global -
//! \note   Disables interrupt for a short time. Stalls the CPU during writing.
//-----------------------------------------------------------------------------
void Persist_Save( void )
{
  S_PERSIST sLocalCopy;
  
  // Assuming that the gpsNextSaveSlot pointer is correct...
  DISABLE_IT;
  memcpy( &sLocalCopy, &gsPersistentData, sizeof( S_PERSIST ) );
  ENABLE_IT;
  // Calculate CRC
  sLocalCopy.u16CRC = Util_CRC16( (U8*)&sLocalCopy, sizeof( S_PERSIST ) - sizeof( U16 ) );
  // Check if next block is empty; if not, erase
  if( (U32)gpsNextSaveSlot + sizeof( S_PERSIST ) > SAVE_BASEADDRESS + SAVE_SIZE )
  {
    Flash_EraseSector( SAVE_BASEADDRESS );
    gpsNextSaveSlot = (S_PERSIST*)SAVE_BASEADDRESS;
  }
  // Write flash
  Flash_Write( (U32)gpsNextSaveSlot, (U8*)&sLocalCopy, sizeof( S_PERSIST ) );
  gpsNextSaveSlot++;
}


/***************************************< End of file >**************************************/
