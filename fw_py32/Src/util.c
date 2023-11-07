/*! *******************************************************************************************************
* Copyright (c) 2021-2022 Hekk_Elek
*
* \file util.c
*
* \brief Utilities and housekeeping
*
* \author Hekk_Elek
*
**********************************************************************************************************/
/*
TODOs in this module:
-- CRC calculation may need optimization in assembly, as it is a computation-extensive function.
*/


/***************************************< Includes >**************************************/
// Own includes
#include "main.h"
#include "types.h"
#include "util.h"


/***************************************< Definitions >**************************************/
#define CRC16_PRECONDITION      (0xBD26u)  //!< Precondition (i.e. initial value) of CRC calculation


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/
//! \brief Table for calculating CRC-16F/3
CODE const U16 gcau16CRC16F3Table[] =
{
  0x0000u, 0x1B2Bu, 0x3656u, 0x2D7Du, 0x6CACu, 0x7787u, 0x5AFAu, 0x41D1u,
  0xD958u, 0xC273u, 0xEF0Eu, 0xF425u, 0xB5F4u, 0xAEDFu, 0x83A2u, 0x9889u,
  0xA99Bu, 0xB2B0u, 0x9FCDu, 0x84E6u, 0xC537u, 0xDE1Cu, 0xF361u, 0xE84Au,
  0x70C3u, 0x6BE8u, 0x4695u, 0x5DBEu, 0x1C6Fu, 0x0744u, 0x2A39u, 0x3112u,
  0x481Du, 0x5336u, 0x7E4Bu, 0x6560u, 0x24B1u, 0x3F9Au, 0x12E7u, 0x09CCu,
  0x9145u, 0x8A6Eu, 0xA713u, 0xBC38u, 0xFDE9u, 0xE6C2u, 0xCBBFu, 0xD094u,
  0xE186u, 0xFAADu, 0xD7D0u, 0xCCFBu, 0x8D2Au, 0x9601u, 0xBB7Cu, 0xA057u,
  0x38DEu, 0x23F5u, 0x0E88u, 0x15A3u, 0x5472u, 0x4F59u, 0x6224u, 0x790Fu,
  0x903Au, 0x8B11u, 0xA66Cu, 0xBD47u, 0xFC96u, 0xE7BDu, 0xCAC0u, 0xD1EBu,
  0x4962u, 0x5249u, 0x7F34u, 0x641Fu, 0x25CEu, 0x3EE5u, 0x1398u, 0x08B3u,
  0x39A1u, 0x228Au, 0x0FF7u, 0x14DCu, 0x550Du, 0x4E26u, 0x635Bu, 0x7870u,
  0xE0F9u, 0xFBD2u, 0xD6AFu, 0xCD84u, 0x8C55u, 0x977Eu, 0xBA03u, 0xA128u,
  0xD827u, 0xC30Cu, 0xEE71u, 0xF55Au, 0xB48Bu, 0xAFA0u, 0x82DDu, 0x99F6u,
  0x017Fu, 0x1A54u, 0x3729u, 0x2C02u, 0x6DD3u, 0x76F8u, 0x5B85u, 0x40AEu,
  0x71BCu, 0x6A97u, 0x47EAu, 0x5CC1u, 0x1D10u, 0x063Bu, 0x2B46u, 0x306Du,
  0xA8E4u, 0xB3CFu, 0x9EB2u, 0x8599u, 0xC448u, 0xDF63u, 0xF21Eu, 0xE935u,
  0x3B5Fu, 0x2074u, 0x0D09u, 0x1622u, 0x57F3u, 0x4CD8u, 0x61A5u, 0x7A8Eu,
  0xE207u, 0xF92Cu, 0xD451u, 0xCF7Au, 0x8EABu, 0x9580u, 0xB8FDu, 0xA3D6u,
  0x92C4u, 0x89EFu, 0xA492u, 0xBFB9u, 0xFE68u, 0xE543u, 0xC83Eu, 0xD315u,
  0x4B9Cu, 0x50B7u, 0x7DCAu, 0x66E1u, 0x2730u, 0x3C1Bu, 0x1166u, 0x0A4Du,
  0x7342u, 0x6869u, 0x4514u, 0x5E3Fu, 0x1FEEu, 0x04C5u, 0x29B8u, 0x3293u,
  0xAA1Au, 0xB131u, 0x9C4Cu, 0x8767u, 0xC6B6u, 0xDD9Du, 0xF0E0u, 0xEBCBu,
  0xDAD9u, 0xC1F2u, 0xEC8Fu, 0xF7A4u, 0xB675u, 0xAD5Eu, 0x8023u, 0x9B08u,
  0x0381u, 0x18AAu, 0x35D7u, 0x2EFCu, 0x6F2Du, 0x7406u, 0x597Bu, 0x4250u,
  0xAB65u, 0xB04Eu, 0x9D33u, 0x8618u, 0xC7C9u, 0xDCE2u, 0xF19Fu, 0xEAB4u,
  0x723Du, 0x6916u, 0x446Bu, 0x5F40u, 0x1E91u, 0x05BAu, 0x28C7u, 0x33ECu,
  0x02FEu, 0x19D5u, 0x34A8u, 0x2F83u, 0x6E52u, 0x7579u, 0x5804u, 0x432Fu,
  0xDBA6u, 0xC08Du, 0xEDF0u, 0xF6DBu, 0xB70Au, 0xAC21u, 0x815Cu, 0x9A77u,
  0xE378u, 0xF853u, 0xD52Eu, 0xCE05u, 0x8FD4u, 0x94FFu, 0xB982u, 0xA2A9u,
  0x3A20u, 0x210Bu, 0x0C76u, 0x175Du, 0x568Cu, 0x4DA7u, 0x60DAu, 0x7BF1u,
  0x4AE3u, 0x51C8u, 0x7CB5u, 0x679Eu, 0x264Fu, 0x3D64u, 0x1019u, 0x0B32u,
  0x93BBu, 0x8890u, 0xA5EDu, 0xBEC6u, 0xFF17u, 0xE43Cu, 0xC941u, 0xD26Au
};


/***************************************< Global variables >**************************************/
//! \brief Globally accessible timer with millisecond resolution. IDATA for fast access.
DATA U16 gu16TimerMS;
DATA U8  gu8Prescaler;  //!< Prescaler for the global timer. IDATA for fast access.


/***************************************< Static function definitions >**************************************/


/***************************************< Private functions >**************************************/


/***************************************< Public functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Puts the unique ID of the MCU to a specific array
//! \param  *pu8Dest: UID will be written here (7 bytes!)
//! \return -
//! \global -
//! \note   Make sure pu8Dest points to an array of at least 7 bytes!
//-----------------------------------------------------------------------------
void Util_Get_UID( U8* pu8Dest )
{
  char CODE* pu8Src;
  U8  u8Idx;
  
  pu8Src = Util_Get_UID_ptr();
  for( u8Idx = 0u; u8Idx < UID_LENGTH; u8Idx++ )
  {
    pu8Dest[ u8Idx ] = pu8Src[ u8Idx ];
  }
}

//----------------------------------------------------------------------------
//! \brief  Returns the pointer to the UID array
//! \param  -
//! \return char CODE* pointer
//! \global -
//-----------------------------------------------------------------------------
char CODE* Util_Get_UID_ptr( void )
{
  return (char CODE *)0x1FF9;  // STC8G1K08
}

//----------------------------------------------------------------------------
//! \brief  Increase timer value
//! \param  -
//! \return -
//! \global Global timer (ms)
//! \note   Runs in interrupt routine
//-----------------------------------------------------------------------------
void Util_Interrupt( void )
{
  gu8Prescaler++;
  if( gu8Prescaler >= 10u )
  {
    gu16TimerMS++;
    gu8Prescaler = 0u;
  }
}

//----------------------------------------------------------------------------
//! \brief  Initialize global variables
//! \param  -
//! \return -
//! \global Global timer (ms)
//! \note   Called at initialization only!
//-----------------------------------------------------------------------------
void Util_Init( void )
{
  gu8Prescaler = 0u;
  gu16TimerMS = 0u;
}

//----------------------------------------------------------------------------
//! \brief  Get global timer (ms)
//! \param  -
//! \return Timer value
//! \global Global timer (ms)
//! \note   Should be called from main program only!
//-----------------------------------------------------------------------------
U16 Util_GetTimerMs( void )
{
  U16 u16Ret;
  
  DISABLE_IT;
  u16Ret = gu16TimerMS;
  ENABLE_IT;
  
  return u16Ret;
}

//----------------------------------------------------------------------------
//! \brief  Calculates CRC16 of given buffer
//! \param  *pu8Buffer: given buffer
//! \param  u8Length: length of the buffer
//! \return CRC16 value
//! \global -
//-----------------------------------------------------------------------------
U16 Util_CRC16( U8* pu8Buffer, U8 u8Length ) REENTRANT
{
  U16 u16Crc;
  U8  u8Idx;

  u16Crc = CRC16_PRECONDITION;
  for( u8Idx = 0; u8Idx != u8Length; u8Idx++ )
  {
    u16Crc = (U16)( u16Crc << 8u ) ^ gcau16CRC16F3Table[ (U8)( u16Crc >> 8u ) ^ pu8Buffer[ u8Idx ] ];
  }

  return u16Crc;
}


/***************************************< End of file >**************************************/
