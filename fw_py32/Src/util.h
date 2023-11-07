/*! *******************************************************************************************************
* Copyright (c) 2021-2022 Hekk_Elek
*
* \file util.h
*
* \brief Utilities and housekeeping
*
* \author Hekk_Elek
*
**********************************************************************************************************/
#ifndef UTIL_H
#define UTIL_H

/***************************************< Includes >**************************************/
#include "main.h"
#include "platform.h"


/***************************************< Definitions >**************************************/
#define UID_LENGTH        (7u)  //!< Length of the unique ID of the MCU
#define SYSTEM_CLOCK_MHZ (24u)  //!< System clock in MHz, rounded to integers


/***************************************< Macros >**************************************/
#define DISABLE_IT     __enable_irq();   //!< Global interrupt disable
#define ENABLE_IT      __disable_irq();  //!< Global interrupt enable


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/
extern DATA U16 gu16TimerMS;


/***************************************< Public functions >**************************************/
char CODE* Util_Get_UID_ptr( void );
void Util_Get_UID( U8* pu8Dest );
void Util_Interrupt( void );
void Util_Init( void );
U16 Util_GetTimerMs( void );
U16 Util_CRC16( U8* pu8Buffer, U8 u8Length ) REENTRANT;


#endif /* UTIL_H */

/***************************************< End of file >**************************************/
