/*! *******************************************************************************************************
* Copyright (c) 2021-2022 Hekk_Elek
*
* \file types.h
*
* \brief Generally used types
*
* \author Hekk_Elek
*
**********************************************************************************************************/
#ifndef TYPES_H
#define TYPES_H

/***************************************< Includes >**************************************/
#include "platform.h"


/***************************************< Definitions >**************************************/


/***************************************< Types >**************************************/
typedef unsigned char      U8;
typedef unsigned short int U16;
typedef unsigned long int  U32;

typedef signed char        I8;
typedef signed short int   I16;
typedef signed long int    I32;

//! \brief Boolean type
typedef BIT BOOL;
#define TRUE    1
#define FALSE   0


/***************************************< Static assertions >**************************************/
//STATIC_ASSERT(sizeof(BOOL)==1u);


#endif /* TYPES_H */

/***************************************< End of file >**************************************/
