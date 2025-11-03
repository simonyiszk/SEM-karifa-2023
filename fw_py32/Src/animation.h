/*! *******************************************************************************************************
* Copyright (c) 2021-2025 Hekk_Elek
*
* \file animation.h
*
* \brief Implementation of LED animation engine and the animations themselves
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
  #define NUM_ANIMATIONS        (11u)  //!< Number of animations implemented (including blackness)
#endif

#ifdef AJANDEKCSOMAG
  #define NUM_ANIMATIONS        (15u)  //!< Number of animations implemented (including blackness)
#endif

#ifdef RUDOLF
  #define NUM_ANIMATIONS        (18u)  //!< Number of animations implemented (including blackness)
#endif

#ifdef HULLOCSILLAG
  #define NUM_ANIMATIONS        (12u)  //!< Number of animations implemented (including blackness)
#endif

#ifdef MACSKAS
  #define NUM_ANIMATIONS        (14u)  //!< Number of animations implemented (including blackness)
#endif


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/


/***************************************< Public functions >**************************************/
void Animation_Init( void );
void Animation_Cycle( void );
void Animation_Set( U8 u8AnimationIndex );
void Animation_NextAnimation( void );
void Animation_SetDarkness( void );
void Animation_SetLoop( BOOL bSetLoop );


#endif /* ANIMATION_H */

/***************************************< End of file >**************************************/
