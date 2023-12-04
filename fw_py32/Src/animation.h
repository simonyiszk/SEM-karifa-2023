/*! *******************************************************************************************************
* Copyright (c) 2021-2023 Hekk_Elek
*
* \file animation.h
*
* \brief Implementation of LED animations
*
* \author Hekk_Elek
*
**********************************************************************************************************/
#ifndef ANIMATION_H
#define ANIMATION_H

/***************************************< Includes >**************************************/
#include "config.h"


/***************************************< Definitions >**************************************/
#ifdef KARIFA
  #define NUM_ANIMATIONS        (18u)  //!< Number of animations implemented (including blackness)
#endif

#ifdef HOEMBER
  #define NUM_ANIMATIONS        (18u)  //!< Number of animations implemented (including blackness)
#endif

#ifdef HOPEHELY
  #define NUM_ANIMATIONS        (16u)  //!< Number of animations implemented (including blackness)
#endif

#ifdef MEZI
  #define NUM_ANIMATIONS        (18u)  //!< Number of animations implemented (including blackness)
#endif

/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/


/***************************************< Public functions >**************************************/
void Animation_Init( void );
void Animation_Cycle( void );
void Animation_Set( U8 u8AnimationIndex );


#endif /* ANIMATION_H */

/***************************************< End of file >**************************************/
