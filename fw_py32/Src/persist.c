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
#define EEPROM_SIZE           (4096u)  //!< Number of bytes present as EEPROM memory
#define EEPROM_BASEADDRESS  (0x2000u)  //!< Base address of EEPROM in STC8G1K08


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/
DATA S_PERSIST       gsPersistentData;  //!< Globally accessible persistent data structure
IDATA S_PERSIST CODE* gpsNextSaveSlot;  //!< Address of the next persistent save slot


/***************************************< Static function definitions >**************************************/
static BOOL IsSaveBlockEmpty( S_PERSIST* psLocalCopy, S_PERSIST CODE* psSaveBlock );
static BOOL SearchForLatestSave( S_PERSIST CODE** ppsNextEmpty );
static void IAP_Write( U16 u16Address, U8* pu8Data, U8 u8DataLength );
static void IAP_Erase( U16 u16Address );
static void IAP_Read( U16 u16Address, U8* pu8Data, U8 u8DataLength );


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

  IAP_Read( (U16)psSaveBlock, (U8*)psTemp, sizeof( S_PERSIST ) );
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
  BOOL bEmpty;
  U16 u16SaveIndex;
  BOOL bReturn = FALSE;
  S_PERSIST CODE* psSave = (S_PERSIST CODE*)EEPROM_BASEADDRESS;
  S_PERSIST  sLocalCopy;
  
  for( u16SaveIndex = 0u; u16SaveIndex < ( EEPROM_SIZE / sizeof( S_PERSIST ) ); u16SaveIndex++ )
  {
    IAP_Read( (U16)psSave, (U8*)&sLocalCopy, sizeof( S_PERSIST ) );
    if( sLocalCopy.u16CRC == Util_CRC16( (U8*)&sLocalCopy, sizeof( S_PERSIST ) - sizeof( U16 ) ) )
    {
      memcpy( &gsPersistentData, &sLocalCopy, sizeof( S_PERSIST ) );
      bReturn = TRUE;
    }
    else  // bad CRC
    {
      // Check if this block is empty
      bEmpty = IsSaveBlockEmpty( &sLocalCopy, psSave );
      if( ( TRUE == bReturn ) && ( TRUE == bEmpty ) )
      {
        *ppsNextEmpty = psSave;
        break;
      }
    }
    psSave++;
  }
#warning "What happens if it rolls over?"
  
  return bReturn;
}
//----------------------------------------------------------------------------
//! \brief  Write data block to EEPROM from a given address
//! \param  u16Address: Start address to be written. Only lower 12 bits are used.
//! \param  pu8Data: pointer to the data to be written
//! \param  u8DataLength: write length
//! \return -
//! \global -
//! \note   Stalls the CPU.
//-----------------------------------------------------------------------------
static void IAP_Write( U16 u16Address, U8* pu8Data, U8 u8DataLength )
{
  U8 u8Index;
#warning "Port to PY32F002!"
/*
  DISABLE_IT;
  
  IAP_CONTR = 0x80u;  // EEPROM is enabled
  IAP_TPS = SYSTEM_CLOCK_MHZ;
  IAP_CMD = 0x02u;  // Write operation
  for( u8Index = 0u; u8Index < u8DataLength; u8Index++ )
  {
    IAP_ADDRL = u16Address;
    IAP_ADDRH = (u16Address>>8u) & 0x0Fu;  // only lower 12 bits are used
    IAP_DATA = pu8Data[ u8Index ];
    // Initiate command by magic code sequence
    IAP_TRIG = 0x5Au;
    IAP_TRIG = 0xA5u;
    NOP();
    NOP();
    u16Address++;
  }
  IAP_CONTR = 0x00u;  // Clear flags, EEPROM is disabled
  IAP_CMD   = 0u;
  IAP_TRIG  = 0u;

  ENABLE_IT;
*/
}

//----------------------------------------------------------------------------
//! \brief  Erases one page in EEPROM
//! \param  u16Address: Address of page, the lower 9 bits are discarded
//! \return -
//! \global -
//! \note   Stalls the CPU.
//-----------------------------------------------------------------------------
static void IAP_Erase( U16 u16Address )
{
#warning "Port to PY32F002!"
/*  
  DISABLE_IT;
  
  IAP_CONTR = 0x80u;  // EEPROM is enabled
  IAP_TPS = SYSTEM_CLOCK_MHZ;
  IAP_CMD = 0x03u;  // Erase operation
  IAP_ADDRL = u16Address;  // NOTE: lower 9 bits are automatically discarded
  IAP_ADDRH = (u16Address>>8u) & 0x0Fu;  // only lower 12 bits are used
  // Initiate command by magic code sequence
  IAP_TRIG = 0x5Au;
  IAP_TRIG = 0xA5u;
  NOP();
  NOP();
  // Clear flags and registers
  IAP_CONTR = 0x00u;  // Clear flags, EEPROM is disabled
  IAP_CMD   = 0u;
  IAP_TRIG  = 0u;
  
  ENABLE_IT;
*/
}

//----------------------------------------------------------------------------
//! \brief  Reads given number of bytes from EEPROM
//! \param  u16Address: Start address to be read. Only lower 12 bits are used.
//! \param  pu8Data: data read from EEPROM are written here
//! \param  u8DataLength: read length
//! \return -
//! \global -
//! \note   Stalls the CPU.
//-----------------------------------------------------------------------------
static void IAP_Read( U16 u16Address, U8* pu8Data, U8 u8DataLength )
{
#warning "Port to PY32F002!"
/*  
  U8 u8Index;

  DISABLE_IT;
  
  IAP_CONTR = 0x80u;  // EEPROM is enabled
  IAP_TPS = SYSTEM_CLOCK_MHZ;
  IAP_CMD = 0x01u;  // Read operation
  for( u8Index = 0u; u8Index < u8DataLength; u8Index++ )
  {
    IAP_ADDRL = u16Address;
    IAP_ADDRH = (u16Address>>8u) & 0x0Fu;  // only lower 12 bits are used
    // Initiate command by magic code sequence
    IAP_TRIG = 0x5Au;
    IAP_TRIG = 0xA5u;
    NOP();
    NOP();
    // Output
    pu8Data[ u8Index ] = IAP_DATA;
    u16Address++;
  }
  IAP_CONTR = 0x00u;  // Clear flags, EEPROM is disabled
  IAP_CMD   = 0u;
  IAP_TRIG  = 0u;

  ENABLE_IT;
*/
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
#warning "Port to PY32F002!"
/*
  // Enable EEPROM
  IAP_CONTR = 0x80u;
  IAP_TPS = SYSTEM_CLOCK_MHZ;
  IAP_CMD = 0x01u;  // Read operation
*/
  // Find latest save and load it
  if( TRUE == SearchForLatestSave( &gpsNextSaveSlot ) )
  {
    // persistent data are loaded to memory
  }
  else  // Default values
  {
    memset( &gsPersistentData, 0, sizeof( S_PERSIST ) );
    gpsNextSaveSlot = (S_PERSIST CODE*)EEPROM_BASEADDRESS;
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
  U16       u16PrevSaveAddress = (U16)gpsNextSaveSlot;
  
  // Assuming that the gpsNextSaveSlot pointer is correct...
  DISABLE_IT;
  memcpy( &sLocalCopy, &gsPersistentData, sizeof( S_PERSIST ) );
  ENABLE_IT;
  // Calculate CRC
  sLocalCopy.u16CRC = Util_CRC16( (U8*)&sLocalCopy, sizeof( S_PERSIST ) - sizeof( U16 ) );
  // Write EEPROM
  IAP_Write( (U16)gpsNextSaveSlot, (U8*)&sLocalCopy, sizeof( S_PERSIST ) );
  // Check if next block is empty; if not, erase
  gpsNextSaveSlot++;
  if( FALSE == IsSaveBlockEmpty( &sLocalCopy, gpsNextSaveSlot ) )
  {
    // If the page boundary is inside the save slot
    if( ((U16)gpsNextSaveSlot & 0xFE00u) < u16PrevSaveAddress )
    {
      IAP_Erase( ((U16)gpsNextSaveSlot) + sizeof( S_PERSIST ) );
    }
    else
    {
      IAP_Erase( (U16)gpsNextSaveSlot );
    }
  }
}

#warning "Test this module!"
#warning "Port to PY32F002!"

/***************************************< End of file >**************************************/
