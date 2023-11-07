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
#ifndef PERSIST_H
#define PERSIST_H

/***************************************< Includes >**************************************/
#include "util.h"


/***************************************< Definitions >**************************************/


/***************************************< Types >**************************************/
//! \brief Structure for persistent data
typedef PACKED struct
{
  U8  u8AnimationIndex;             //!< Index of the last played animation
  U16 u16CRC;                       //!< CRC for protecting structure against bit errors
} S_PERSIST;


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/
extern DATA S_PERSIST gsPersistentData;


/***************************************< Public functions >**************************************/
void Persist_Init( void );
void Persist_Save( void );


#endif /* PERSIST_H */

/***************************************< End of file >**************************************/
