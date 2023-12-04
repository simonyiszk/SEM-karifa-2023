/*! *******************************************************************************************************
* Copyright (c) 2021-2023 Hekk_Elek
*
* \file animation.c
*
* \brief Implementation of LED animations
*
* \author Hekk_Elek
*
**********************************************************************************************************/

/*----------------------------------------------------------------------------------------
How it works
============
Animations are implemented on a virtual machine. This machine has opcodes that operate on
the LED brightness state variables. Each animation program runs in a loop. Instructions have
the following format:
  [ LED brightness array -- signed integer ] [ Opcode ] [ Opcode specific operand ]

----------------------------------------------------------------------------------------*/

/***************************************< Includes >**************************************/
// Standard C libraries
#include <string.h>

// Own includes
#include "types.h"
#include "config.h"
#include "led.h"
#include "rgbled.h"
#include "util.h"
#include "animation.h"
#include "persist.h"


/***************************************< Definitions >**************************************/
#define RIGHT_LEDS_START    (6u)  //!< Index of the first LED on the right side of the board


/***************************************< Types >**************************************/
//! \brief Opcode bits used in animation virtual machine
typedef enum
{
  LOAD      = 0x00u,  //!< Loads the LED brightness array to the PWM driver
  ADD       = 0x01u,  //!< Adds the LED brightness array elements to the current brightness level; if overflows, it sets to zero
  RSHIFT    = 0x02u,  //!< Shifts all the current LED brightness levels clockwise
  LSHIFT    = 0x04u,  //!< Shifts all the current LED brightness levels anticlockwise
//  UMOVE     = 0x04u,  //!< Moves some of the values upwards. Uses saturation logic. Doesn't roll over.
//  DMOVE     = 0x08u,  //!< Moves some of the values downwards. Uses saturation logic. Doesn't roll over.
  DIV       = 0x10u,  //!< Divides the the current LED brightness levels by the given number
  USOURCE   = 0x20u,  //!< Add values to the brightness and if it overflows/underflows then it will be added to the upwards next value. If it overflows/underflows then it will do the same until it reaches the uppper or lower end.
  DSOURCE   = 0x40u,  //!< Add values to the brightness and if it overflows/underflows then it will be added to the downwards next value. If it overflows/underflows then it will do the same until it reaches the uppper or lower end.
  REPEAT    = 0x80u   //!< Do the instruction and repeat by (operand)-times
} E_ANIMATION_OPCODE;

//! \brief Instruction used by the animation state machine -- for normal LEDs
typedef struct
{
  U16 u16TimingMs;                               //!< How long the machine should stay in this state
  I8  ai8LEDBrightness[ LEDS_NUM ];              //!< Brightness of each LED
  U8  u8AnimationOpcode;                         //!< Opcode (E_ANIMATION_OPCODE)
  U8  u8AnimationOperand;                        //!< Opcode-specific operand
} S_ANIMATION_INSTRUCTION_NORMAL;

//! \brief Instruction used by the animation state machine -- for the RGB LED
typedef struct
{
  U16 u16TimingMs;                               //!< How long the machine should stay in this state
  I8  ai8RGBLEDBrightness[ NUM_RGBLED_COLORS ];  //!< Brightness of each color
  U8  u8AnimationOpcode;                         //!< Opcode (E_ANIMATION_OPCODE)
  U8  u8AnimationOperand;                        //!< Opcode-specific operand
} S_ANIMATION_INSTRUCTION_RGB;

//! \brief Animation structure
typedef struct
{
  U8                                         u8AnimationLengthNormal;  //!< How many instructions this animation has for the normal LEDs
  const S_ANIMATION_INSTRUCTION_NORMAL CODE* psInstructionsNormal;     //!< Pointer to the instructions themselves -- normal LEDs
  U8                                         u8AnimationLengthRGB;     //!< How many instructions this animation has for the RGB LED
  const S_ANIMATION_INSTRUCTION_RGB CODE*    psInstructionsRGB;        //!< Pointer to the instructions themselves -- RGB LED
} S_ANIMATION;


/***************************************< Constants >**************************************/
//! \brief Retro animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasRetroVersion[ 8u ] = 
{
  {133u, {15,  0, 15,  0,  0, 15, 15,  0, 15,  0,  0, 15}, LOAD, 0u },
  {133u, { 0, 15,  0, 15, 15,  0,  0, 15,  0, 15, 15,  0}, LOAD, 0u },
  {133u, {15,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0}, LOAD, 0u },
  {133u, { 0, 15,  0, 15, 15,  0,  0, 15,  0, 15, 15,  0}, LOAD, 0u },
  {133u, {15,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0}, LOAD, 0u },
  {133u, { 0,  0,  0, 15,  0,  0,  0,  0,  0, 15,  0,  0}, LOAD, 0u },
  {133u, {15,  0, 15,  0,  0, 15, 15,  0,  0, 15,  0, 15}, LOAD, 0u },
  {133u, { 0,  0,  0, 15,  0,  0,  0,  0,  0, 15,  0,  0}, LOAD, 0u },
};
//! \brief Retro animation (red) -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasRetroVersionRGB[ 4u ] = 
{
  {133u, {15,  0,  0}, LOAD, 0u },
  {665u, { 0,  0,  0}, LOAD, 0u },
  {133u, {15,  0,  0}, LOAD, 0u },
  {133u, { 0,  0,  0}, LOAD, 0u },
};
//! \brief Retro animation (yellow) -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasRetroVersionRGB_yellow[ 4u ] = 
{
  {133u, {15, 15,  0}, LOAD, 0u },
  {665u, { 0,  0,  0}, LOAD, 0u },
  {133u, {15, 15,  0}, LOAD, 0u },
  {133u, { 0,  0,  0}, LOAD, 0u },
};

//--------------------------------------------------------
//! \brief "Sine" wave flasher animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasSoftFlashing[ 4u ] = 
{
  {125u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,          0u },
  {125u, { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1}, ADD | REPEAT, 14u },
  {125u, {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15}, LOAD,          0u }, 
  {125u, {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, ADD | REPEAT, 14u },
};
//! \brief "Sine" wave flasher animation (red) -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasSoftFlashingRGB[ 4u ] = 
{
  {125u, { 0,  0,  0}, LOAD,          0u },
  {125u, { 1,  0,  0}, ADD | REPEAT, 14u },
  {125u, {15,  0,  0}, LOAD,          0u }, 
  {125u, {-1,  0,  0}, ADD | REPEAT, 14u },
};
//! \brief "Sine" wave flasher animation (blue) -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasSoftFlashingRGB_hopehely[ 4u ] = 
{
  {125u, { 0,  0,  0}, LOAD,          0u },
  {125u, { 0,  0,  1}, ADD | REPEAT, 14u },
  {125u, { 0,  0, 15}, LOAD,          0u }, 
  {125u, { 0,  0, -1}, ADD | REPEAT, 14u },
};

//--------------------------------------------------------
//! \brief "Fade ring" animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasFadeRing[ 3u ] =
{
  { 40u, {15,  1, 15,  1, 15,  1,  1, 15,  1, 15,  1, 15}, LOAD,          0u },
  { 40u, {-1,  1, -1,  1, -1,  1,  1, -1,  1, -1,  1, -1}, ADD | REPEAT, 13u },
  { 40u, { 1, -1,  1, -1,  1, -1, -1,  1, -1,  1, -1,  1}, ADD | REPEAT, 13u },
};
//! \brief "Fade ring" animation -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasFadeRingRGB[ 3u ] =
{
  { 40u, {15,  1,  0}, LOAD,          0u },
  { 40u, {-1,  0,  0}, ADD | REPEAT, 13u },
  { 40u, { 1,  0,  0}, ADD | REPEAT, 13u },
};
//! \brief "Fade ring" animation -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasFadeRingRGB_hopehely[ 3u ] =
{
  { 40u, {15,  1, 15}, LOAD,          0u },
  { 40u, {-1,  0, -1}, ADD | REPEAT, 13u },
  { 40u, { 1,  0,  1}, ADD | REPEAT, 13u },
};

//--------------------------------------------------------
//! \brief Shooting star anticlockwise animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasShootingStar[ 7u ] = 
{ 
  {100u, { 5, 10, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 2u },
  {100u, { 0,  0,  0,  0,  5, 10,  0,  0,  0,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  5, 15,  0,  0,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  0, 10, 15,  0,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  0,  5, 10, 15,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 4u },
};
//! \brief Shooting star anticlockwise animation -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasShootingStarRGB[ 4u ] = 
{ 
  {400u, { 0,  0,  0}, LOAD,            0u },
  {100u, {15,  0,  0}, LOAD,            0u },
  {100u, {-5,  0,  0}, ADD | REPEAT,    1u },
  {600u, { 0,  0,  0}, LOAD,            0u },
};

//--------------------------------------------------------
//! \brief Star launch animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasStarLaunch[ 5u ] = 
{
  {400u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,              0u },
  {200u, { 5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,              0u },
  {200u, { 5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5}, USOURCE | REPEAT, 18u },
  {200u, {15, 15, 15, 15, 15, 15, 10, 15, 15, 15, 15, 15}, LOAD,              0u },
  {200u, { 0,  0,  0,  0,  0, -5, -5,  0,  0,  0,  0,  0}, DSOURCE | REPEAT, 16u },
};
//! \brief Star launch animation -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasStarLaunchRGB[ 5u ] = 
{
  {4000u, { 0,  0,  0}, LOAD,         0u},
  { 800u, {15, 15,  0}, LOAD,         0u},
  { 200u, { 0, -1,  0}, ADD | REPEAT, 9u},
  { 200u, {-3, -1,  0}, ADD | REPEAT, 4u},
  { 200u, { 0,  0,  0}, LOAD,         0u},
};

//--------------------------------------------------------
//! \brief Generic flasher animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasGenericFlasher[ 2u ] = 
{
  {500u, {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15}, LOAD, 0u }, 
  {500u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD, 0u },
};
//! \brief Generic flasher animation -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasGenericFlasherRGB[ 2u ] = 
{
  {500u, { 7,  7,  7}, LOAD, 0u }, 
  {500u, { 0,  0,  0}, LOAD, 0u },
};

//--------------------------------------------------------
//! \brief KITT animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasKITT[ 22u ] = 
{
  {200u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, { 5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5}, LOAD,  0u },
  {100u, {10,  5,  0,  0,  0,  0,  0,  0,  0,  0,  5, 10}, LOAD,  0u },
  {100u, {15, 10,  5,  0,  0,  0,  0,  0,  0,  5, 10, 15}, LOAD,  0u },
  {100u, {10, 15, 10,  5,  0,  0,  0,  0,  5, 10, 15, 10}, LOAD,  0u },
  {100u, { 5, 10, 15, 10,  5,  0,  0,  5, 10, 15, 10,  5}, LOAD,  0u },
  {100u, { 0,  5, 10, 15, 10,  5,  5, 10, 15, 10,  5,  0}, LOAD,  0u },
  {100u, { 0,  0,  5, 10, 15, 10, 10, 15, 10,  5,  0,  0}, LOAD,  0u },
  {100u, { 0,  0,  0,  5, 10, 15, 15, 10,  5,  0,  0,  0}, LOAD,  0u },
  {100u, { 0,  0,  0,  0,  5, 10, 10,  5,  0,  0,  0,  0}, LOAD,  0u },
  {100u, { 0,  0,  0,  0,  0,  5,  5,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, { 0,  0,  0,  0,  0,  5,  5,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, { 0,  0,  0,  0,  5, 10, 10,  5,  0,  0,  0,  0}, LOAD,  0u },
  {100u, { 0,  0,  0,  5, 10, 15, 15, 10,  5,  0,  0,  0}, LOAD,  0u },
  {100u, { 0,  0,  5, 10, 15, 10, 10, 15, 10,  5,  0,  0}, LOAD,  0u },
  {100u, { 0,  5, 10, 15, 10,  5,  5, 10, 15, 10,  5,  0}, LOAD,  0u },
  {100u, { 5, 10, 15, 10,  5,  0,  0,  5, 10, 15, 10,  5}, LOAD,  0u },
  {100u, {10, 15, 10,  5,  0,  0,  0,  0,  5, 10, 15, 10}, LOAD,  0u },
  {100u, {15, 10,  5,  0,  0,  0,  0,  0,  0,  5, 10, 15}, LOAD,  0u },
  {100u, {10,  5,  0,  0,  0,  0,  0,  0,  0,  0,  5, 10}, LOAD,  0u },
  {100u, { 5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5}, LOAD,  0u },
};
//! \brief KITT animation -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasKITTRGB[ 4u ] = 
{
  { 800u, { 0,  0,  0}, LOAD,         0u },
  { 100u, { 5,  0,  0}, ADD | REPEAT, 3u },
  { 100u, {-5,  0,  0}, ADD | REPEAT, 3u },
  {1300u, { 0,  0,  0}, LOAD,         0u },
};

//--------------------------------------------------------
//! \brief Disco animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasDisco[ 6u ] = 
{
  {40u, {  0, 15,  0, 15,  0, 15,  0, 15,  0, 15,  0, 15}, LOAD,         0u },
  {40u, {  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2}, DIV | REPEAT, 3u },
  {100u,{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,         0u },
  {40u, { 15,  0, 15,  0, 15,  0, 15,  0, 15,  0, 15,  0}, LOAD,         0u },
  {40u, {  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1}, DIV | REPEAT, 3u },
  {100u,{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,         0u },
};
//! \brief Disco animation -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasDiscoRGB[ 6u ] = 
{
  { 40u, {15,  0, 15}, LOAD,         0u },
  { 40u, { 2,  1,  2}, DIV | REPEAT, 3u },
  {100u, { 0,  0,  0}, LOAD,         0u },
  { 40u, { 0, 15,  0}, LOAD,         0u },
  { 40u, { 2,  1,  2}, DIV | REPEAT, 3u },
  {100u, { 0,  0,  0}, LOAD,         0u },
};

//--------------------------------------------------------
//! \brief Pseudo-random fade animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasPseudoRandomFade[ 15u ] = 
{
  { 66u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  { 66u, { 0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0}, ADD | REPEAT, 14u },
  { 66u, { 0,  0,  1,  0,  0,  0,  0, -1,  0,  0,  0,  0}, ADD | REPEAT, 14u },
  { 66u, { 0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  1,  0}, ADD | REPEAT, 14u },
  { 66u, { 1,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  0}, ADD | REPEAT, 14u },
  { 66u, {-1,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0}, ADD | REPEAT, 14u },
  { 66u, { 0,  0,  0,  0,  0, -1,  0,  1,  0,  0,  0,  0}, ADD | REPEAT, 14u },
  { 66u, { 0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  1}, ADD | REPEAT, 14u },
  { 66u, { 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1}, ADD | REPEAT, 14u },
  { 66u, { 0, -1,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0}, ADD | REPEAT, 14u },
  { 66u, { 0,  0,  0, -1,  0,  0,  1,  0,  0,  0,  0,  0}, ADD | REPEAT, 14u },
  { 66u, { 0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0}, ADD | REPEAT, 14u },  //RGB lights up here
  { 66u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0}, ADD | REPEAT, 14u },
  { 66u, { 0,  0,  0,  0,  1,  0,  0,  0,  0, -1,  0,  0}, ADD | REPEAT, 14u },
  { 66u, { 0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0}, ADD | REPEAT, 14u },
};
//! \brief Pseudo-random fade animation (red) -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasPseudoRandomFadeRGB[ 4u ] = 
{
  { 9966u, { 0,  0,  0}, LOAD,  0u },
  {   66u, { 1,  0,  0}, ADD | REPEAT, 14u },
  {   66u, {-1,  0,  0}, ADD | REPEAT, 14u },
  { 1980u, { 0,  0,  0}, LOAD,  0u },
};
//! \brief Pseudo-random fade animation (yellow) -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasPseudoRandomFadeRGB_hopehely[ 4u ] = 
{
  { 9966u, { 0,  0,  0}, LOAD,  0u },
  {   66u, { 1,  1,  0}, ADD | REPEAT, 14u },
  {   66u, {-1, -1,  0}, ADD | REPEAT, 14u },
  { 1980u, { 0,  0,  0}, LOAD,  0u },
};

//--------------------------------------------------------
//! \brief CrissCross -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasCrissCross[ 12u ] = 
{
        //0    1   2   3   4   5   6   7   8   9  10  11
  {350u, {15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  0, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0,  0, 15,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0, 15,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15}, LOAD,  0u },
  {350u, { 0, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0, 15,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0}, LOAD,  0u },
};
//! \brief CrissCross -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasCrissCrossRGB[ 4u ] = 
{
  { 1050u, { 0, 15, 15}, LOAD,         0u },
  { 1050u, {15,  0,  0}, LOAD,         0u },
  { 1050u, { 2, 10, 10}, LOAD,         0u },
  { 1050u, {15, 15,  0}, LOAD,         0u },
};

//--------------------------------------------------------
//! \brief Fadeout -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasFadeout[ 12u ] = 
{
  {350u, { 0,  0,  0,  0,  4,  0,  9,  0,  0, 15,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0, 15,  0,  0,  4,  0,  0,  9,  0,  0}, LOAD,  0u },
  {350u, {15,  0,  0,  9,  0,  0,  0,  0,  0,  4,  0,  0}, LOAD,  0u },
  {350u, { 9,  0,  0,  4,  0,  0,  0, 15,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 4,  0,  0,  0,  0,  0,  0,  9,  0,  0,  0, 15}, LOAD,  0u },
  {350u, { 0,  0,  0,  0,  0,  0,  0,  4, 15,  0,  0,  9}, LOAD,  0u },
  {350u, { 0,  0, 15,  0,  0,  0,  0,  0,  9,  0,  0,  4}, LOAD,  0u },
  {350u, { 0,  0,  9,  0,  0,  0,  0,  0,  4,  0, 15,  0}, LOAD,  0u },
  {350u, { 0,  0,  4,  0,  0, 15,  0,  0,  0,  0,  9,  0}, LOAD,  0u },
  {350u, { 0, 15,  0,  0,  0,  9,  0,  0,  0,  0,  4,  0}, LOAD,  0u },
  {350u, { 0,  9,  0,  0, 15,  4,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  4,  0,  0,  9,  0, 15,  0,  0,  0,  0,  0}, LOAD,  0u },
};
//! \brief Fadeout -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasFadeoutRGB[ 6u ] = 
{
  { 700u, {15, 10,  0}, LOAD,         0u },
  { 700u, {11,  6,  0}, LOAD,         0u },
  { 700u, { 4,  2,  0}, LOAD,         0u },
  { 700u, { 0,  0,  0}, LOAD,         0u },
  { 700u, { 4,  2,  0}, LOAD,         0u },
  { 700u, {11,  6,  0}, LOAD,         0u },
};

//--------------------------------------------------------
//! \brief Flicker -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasFlicker[ 10u ] = 
{
  {200u, { 0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0}, LOAD,  0u },
  {200u, { 0,  0, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {200u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0}, LOAD,  0u },
  {200u, { 0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {200u, {15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {200u, { 0, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {200u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15}, LOAD,  0u },
  {200u, { 0,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0}, LOAD,  0u },
  {200u, { 0,  0,  0, 15,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {200u, { 0,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  0}, LOAD,  0u },
};
//! \brief Flicker -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasFlickerRGB[ 6u ] = 
{
  { 400u, {15,  0,  0}, LOAD,         0u },
  { 100u, {15, 15,  0}, LOAD,         0u },
  { 800u, {15,  0,  0}, LOAD,         0u },
  { 100u, {15, 15,  0}, LOAD,         0u },
  { 500u, {15,  0,  0}, LOAD,         0u },
  { 100u, {15, 15,  0}, LOAD,         0u },
};

//--------------------------------------------------------
//! \brief Pingpong -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasPingpong[ 12u ] = 
{
  {175u, {15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {175u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT,4u },
  {175u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {175u, { 0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0}, LOAD,  0u },
  {175u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT,4u },
  {175u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {175u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15}, LOAD,  0u },
  {175u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LSHIFT | REPEAT,4u },
  {175u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {175u, { 0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {175u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LSHIFT | REPEAT,4u },
  {175u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
};
//! \brief Pingpong -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasPingpongRGB[ 3u ] = 
{
  {1050u, {15, 15,  0}, LOAD,         0u },
  {2450u, { 0, 15, 15}, LOAD,         0u },
  {1400u, {15, 15,  0}, LOAD,         0u },
};

//--------------------------------------------------------
//! \brief Sparkle -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasSparkle[ 10u ] = 
{
  {200u, { 4,  4,  4,  4, 15,  4,  4,  4,  4,  4,  4,  4}, LOAD,  0u },
  {200u, { 4, 15,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4}, LOAD,  0u },
  {200u, { 4,  4,  4,  4,  4,  4, 15,  4,  4,  4,  4,  4}, LOAD,  0u },
  {200u, { 4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 15,  4}, LOAD,  0u },
  {200u, { 4,  4, 15,  4,  4,  4,  4,  4,  4,  4,  4,  4}, LOAD,  0u },
  {200u, {15,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4}, LOAD,  0u },
  {200u, { 4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4, 15}, LOAD,  0u },
  {200u, { 4,  4,  4, 15,  4,  4,  4,  4,  4,  4,  4,  4}, LOAD,  0u },
  {200u, { 4,  4,  4,  4,  4,  4,  4,  4,  4, 15,  4,  4}, LOAD,  0u },
  {200u, { 4,  4,  4,  4,  4, 15,  4,  4,  4,  4,  4,  4}, LOAD,  0u },
};
//! \brief Sparkle -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasSparkleRGB[ 6u ] = 
{
  { 500u, {15,  0,  0}, LOAD,         0u },
  { 250u, {15,  3,  1}, LOAD,         0u },
  { 250u, {15,  6,  2}, LOAD,         0u },
  { 500u, {15, 10,  3}, LOAD,         0u },
  { 250u, {15,  6,  2}, LOAD,         0u },
  { 250u, {15,  3,  1}, LOAD,         0u },
};
//! \brief Sparkle -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasSparkleRGB_hopehely[ 6u ] = 
{
  { 500u, { 0,  0, 15}, LOAD,         0u },
  { 250u, { 1,  3, 15}, LOAD,         0u },
  { 250u, { 2,  6, 15}, LOAD,         0u },
  { 500u, { 3, 10, 15}, LOAD,         0u },
  { 250u, { 2,  6, 15}, LOAD,         0u },
  { 250u, { 1,  3, 15}, LOAD,         0u },
};

//--------------------------------------------------------
//! \brief Split2 -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasSplit2[ 2u ] = 
{
  {500u, {15,  0, 15,  0, 15,  0, 15,  0, 15,  0, 15,  0}, LOAD,  0u },
  {500u, { 0, 15,  0, 15,  0, 15,  0, 15,  0, 15,  0, 15}, LOAD,  0u },
};
//! \brief Split2 -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasSplit2RGB[ 3u ] = 
{
  { 333u, {15,  0, 15}, LOAD,         0u },
  { 333u, { 0, 15, 15}, LOAD,         0u },
  { 334u, {15, 15,  0}, LOAD,         0u },
};

/*
//--------------------------------------------------------
//! \brief Split3fade -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasSplit3fade[ 6u ] = 
{
  {500u, {15,  4,  0, 15,  4,  0, 15,  4,  0, 15,  4,  0}, LOAD,  0u },
  {500u, { 0, 15,  4,  0, 15,  4,  0, 15,  4,  0, 15,  4}, LOAD,  0u },
  {500u, { 0,  0, 15,  4,  0, 15,  4,  0, 15,  4,  0, 15}, LOAD,  0u },
  {500u, {15,  4,  0, 15,  4,  0, 15,  4,  0, 15,  4,  0}, LOAD,  0u },
  {500u, { 0, 15,  4,  0, 15,  4,  0, 15,  4,  0, 15,  4}, LOAD,  0u },
  {500u, { 0,  0, 15,  4,  0, 15,  4,  0, 15,  4,  0, 15}, LOAD,  0u },
};
//! \brief Split3fade -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasSplit3fadeRGB[ 6u ] = 
{
  { 500u, {15,  0, 15}, LOAD,         0u },
  { 500u, { 7,  7, 15}, LOAD,         0u },
  { 500u, { 0, 15, 15}, LOAD,         0u },
  { 500u, { 7, 15,  7}, LOAD,         0u },
  { 500u, {15, 15,  0}, LOAD,         0u },
  { 500u, {15,  7,  7}, LOAD,         0u },
};
*/

//--------------------------------------------------------
//! \brief Stepping -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasStepping[ 2u ] = 
{
  {350u, {15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {350u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT,10u },
};


//! \brief Stepping -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasSteppingRGB[ 11u ] = 
{
  { 350u, {15,  0,  0}, LOAD,         0u },
  { 350u, {15,  6,  0}, LOAD,         0u },
  { 350u, {15, 10,  0}, LOAD,         0u },
  { 350u, {15, 15,  0}, LOAD,         0u },
  { 350u, { 0, 15,  0}, LOAD,         0u },
  { 350u, { 0, 10,  0}, LOAD,         0u },
  { 350u, { 2, 10, 10}, LOAD,         0u },
  { 350u, { 0, 15, 15}, LOAD,         0u },
  { 350u, { 7,  5, 10}, LOAD,         0u },
  { 350u, {15,  0, 15}, LOAD,         0u },
  { 350u, {15, 12, 12}, LOAD,         0u },
};

//--------------------------------------------------------
//! \brief Race -- A trace is circulating and accelerating
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasRace[ 21u ] = 
{
  {100u, { 5, 10, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 2u },
  {100u, { 0,  0,  0,  0,  5, 10,  0,  0,  0,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  5, 15,  0,  0,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  0, 10, 15,  0,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  0,  5, 10, 15,  0,  0,  0}, LOAD,            0u },
  {100u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 4u },
  {70u, { 5, 10, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,            0u },
  {70u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 2u },
  {70u, { 0,  0,  0,  0,  5, 10,  0,  0,  0,  0,  0,  0}, LOAD,            0u },
  {70u, { 0,  0,  0,  0,  0,  5, 15,  0,  0,  0,  0,  0}, LOAD,            0u },
  {70u, { 0,  0,  0,  0,  0,  0, 10, 15,  0,  0,  0,  0}, LOAD,            0u },
  {70u, { 0,  0,  0,  0,  0,  0,  5, 10, 15,  0,  0,  0}, LOAD,            0u },
  {70u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 4u },
  {40u, { 5, 10, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,            0u },
  {40u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 2u },
  {40u, { 0,  0,  0,  0,  5, 10,  0,  0,  0,  0,  0,  0}, LOAD,            0u },
  {40u, { 0,  0,  0,  0,  0,  5, 15,  0,  0,  0,  0,  0}, LOAD,            0u },
  {40u, { 0,  0,  0,  0,  0,  0, 10, 15,  0,  0,  0,  0}, LOAD,            0u },
  {40u, { 0,  0,  0,  0,  0,  0,  5, 10, 15,  0,  0,  0}, LOAD,            0u },
  {40u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 4u },
};
//! \brief Race -- RGB
CODE const S_ANIMATION_INSTRUCTION_RGB gasRaceRGB[ 12u ] = 
{ 
  {400u, { 0,  0,  0}, LOAD,            0u },
  {100u, {15,  0,  0}, LOAD,            0u },
  {100u, {-5,  0,  0}, ADD | REPEAT,    1u },
  {600u, { 0,  0,  0}, LOAD,            0u },
  
  {280u, { 0,  0,  0}, LOAD,            0u },
  {70u, {15,  0,  0}, LOAD,            0u },
  {70u, {-5,  0,  0}, ADD | REPEAT,    1u },
  {420u, { 0,  0,  0}, LOAD,            0u },

  {160u, { 0,  0,  0}, LOAD,            0u },
  {40u, {15,  0,  0}, LOAD,            0u },
  {40u, {-5,  0,  0}, ADD | REPEAT,    1u },
  {240u, { 0,  0,  0}, LOAD,            0u },
};

//--------------------------------------------------------
//! \brief Race -- A trace is circulating and accelerating
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasRace_hopehely[ 6u ] = 
{
  {100u, { 5, 10, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,             0u },
  {100u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 10u },
  { 70u, { 5, 10, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,             0u },
  { 70u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 10u },
  { 40u, { 5, 10, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,             0u },
  { 40u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 10u },
};
//! \brief Race -- RGB
CODE const S_ANIMATION_INSTRUCTION_RGB gasRaceRGB_hopehely[ 9u ] = 
{ 
  {90u, { 0,  0,  0}, LOAD,            0u },
  {37u, { 1,  0,  0}, ADD | REPEAT,   14u },
  {37u, {-1,  0,  0}, ADD | REPEAT,   14u },

  {30u, { 0,  0,  0}, LOAD,            0u },
  {27u, { 0,  1,  0}, ADD | REPEAT,   14u },
  {27u, { 0, -1,  0}, ADD | REPEAT,   14u },

  {30u, { 0,  0,  0}, LOAD,            0u },
  {15u, { 0,  0,  1}, ADD | REPEAT,   14u },
  {15u, { 0,  0, -1}, ADD | REPEAT,   14u },
};

//--------------------------------------------------------
//! \brief Ying-yang
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasYingYang[ 2u ] = 
{
  {150u, { 0,  5, 10, 15,  0,  0,  0,  5, 10, 15,  0,  0}, LOAD,            0u },
  {150u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 4u },
};
//! \brief Ying Yang RGB
CODE const S_ANIMATION_INSTRUCTION_RGB gasYingYangRGB[ 2u ] = 
{
  { 450u, {2, 6, 15}, LOAD,        0u },
  { 450u, { 15,  8,  1}, LOAD,     0u },
};

//--------------------------------------------------------
//! \brief Ice
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasIce[ 11u ] = 
{
  {300u, { 0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {300u, { 0,  0,  0,  0, 15, 10,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {300u, { 0,  0,  0, 15, 10,  5, 15,  0,  0,  0,  0,  0}, LOAD, 0u },
  {300u, { 0,  0, 15, 10,  5,  0, 10, 15,  0,  0,  0,  0}, LOAD, 0u },
  {300u, { 0, 15, 10,  5,  0,  0,  5, 10, 15,  0,  0,  0}, LOAD, 0u },
  {300u, {15, 10,  5,  0,  0,  0,  0,  5, 10, 15,  0,  0}, LOAD, 0u },
  {300u, {15,  5,  0,  0,  0,  0,  0,  0,  5, 10, 15,  0}, LOAD, 0u },
  {300u, {15,  0,  0,  0,  0,  0,  0,  0,  0,  5, 10, 15}, LOAD, 0u },
  {300u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5, 15}, LOAD, 0u },
  {300u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15}, LOAD, 0u },
  {300u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
};
//! \brief Ice
CODE const S_ANIMATION_INSTRUCTION_RGB gasIceRGB[ 2u ] = 
{
  { 194u, {0, 15, 15}, LOAD,        0u },
  { 194u, { 0,  -1,  0}, ADD | REPEAT,     15u },
//  { 88u, {0, 0, 15}, LOAD,        0u },
//  { 88u, { 0,  1,  0}, ADD | REPEAT,     15u },
};

//--------------------------------------------------------
//! \brief Ice
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasIce_hopehely[ 10u ] = 
{
  {300u, { 0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0}, LOAD, 0u },
  {300u, { 0,  0,  0,  0,  0, 15, 10, 15,  0,  0,  0,  0}, LOAD, 0u },
  {300u, { 0,  0,  0,  0, 15, 10,  5, 10, 15,  0,  0,  0}, LOAD, 0u },
  {300u, { 0,  0,  0, 15, 10,  5,  0,  5, 10, 15,  0,  0}, LOAD, 0u },
  {300u, { 0,  0, 15, 10,  5,  0,  0,  0,  5, 10, 15,  0}, LOAD, 0u },
  {300u, { 0, 15, 10,  5,  0,  0,  0,  0,  0,  5, 10, 15}, LOAD, 0u },
  {300u, {15, 10,  5,  0,  0,  0,  0,  0,  0,  0,  5, 10}, LOAD, 0u },
  {300u, {10,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5}, LOAD, 0u },
  {300u, { 5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD, 0u },
  {300u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD, 0u },
};
//! \brief Ice RGB
CODE const S_ANIMATION_INSTRUCTION_RGB gasIceRGB_hopehely[ 4u ] = 
{
  { 105u, { 0, 15, 15}, LOAD,          0u },
  {  93u, { 0, -1,  0}, ADD | REPEAT, 14u },
  { 105u, { 0,  0, 15}, LOAD,          0u },
  {  93u, { 0,  1,  0}, ADD | REPEAT, 14u },
};

//--------------------------------------------------------
//! \brief All blackness, reached right before going to power down mode -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasBlackness[ 1u ] =
{
  {0xFFFFu, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD, 0u },
};
//! \brief All blackness, reached right before going to power down mode -- RGB LED
CODE const S_ANIMATION_INSTRUCTION_RGB gasBlacknessRGB[ 1u ] =
{
  {0xFFFFu, { 0,  0,  0}, LOAD, 0u },
};

//--------------------------------------------------------
//! \brief Shooting star anticlockwise animation -- normal LEDs
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasShootingStar_hopehely[ 2u ] = 
{ 
  {100u, { 5, 10, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,             0u },
  {100u, { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, RSHIFT | REPEAT, 10u },
};
//! \brief 20 step Rainbow, 1000ms per cycle
CODE const S_ANIMATION_INSTRUCTION_RGB gasShootingStar_hopehely_RGB[ 20u ] = 
{
 { 50u, {15,  0,  0}, LOAD, 0u },
 { 50u, {15,  5,  0}, LOAD, 0u },
 { 50u, {15,  9,  0}, LOAD, 0u },
 { 50u, {15, 13,  0}, LOAD, 0u },
 { 50u, {12, 15,  0}, LOAD, 0u },
 { 50u, { 8, 15,  0}, LOAD, 0u },
 { 50u, { 3, 15,  0}, LOAD, 0u },
 { 50u, { 0, 15,  1}, LOAD, 0u },
 { 50u, { 0, 15,  6}, LOAD, 0u },
 { 50u, { 0, 15, 11}, LOAD, 0u },
 { 50u, { 0, 15, 15}, LOAD, 0u },
 { 50u, { 0, 10, 15}, LOAD, 0u },
 { 50u, { 0,  6, 15}, LOAD, 0u },
 { 50u, { 0,  1, 15}, LOAD, 0u },
 { 50u, { 3,  0, 15}, LOAD, 0u },
 { 50u, { 8,  0, 15}, LOAD, 0u },
 { 50u, {12,  0, 15}, LOAD, 0u },
 { 50u, {15,  0, 14}, LOAD, 0u },
 { 50u, {15,  0,  9}, LOAD, 0u },
 { 50u, {15,  0,  5}, LOAD, 0u }
  // USOURCE | REPEAT could be used here, but it would be complicated and a bit wrong due 12 steps
};

//--------------------------------------------------------
//! \brief One led going around for "hoember", not including eyes at 1001ms per cycle
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasStepping_hoember[ 11u ] = 
{
 //        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11 
  {91u, { 15,  0,  0,  0,  0,  8,  8,  0,  0,  0,  0,  0}, LOAD,  0u },
  {91u, {  0, 15,  0,  0,  0,  8,  8,  0,  0,  0,  0,  0}, LOAD,  0u },
  {91u, {  0,  0, 15,  0,  0,  8,  8,  0,  0,  0,  0,  0}, LOAD,  0u },
  {91u, {  0,  0,  0, 15,  0,  8,  8,  0,  0,  0,  0,  0}, LOAD,  0u },
  {91u, {  0,  0,  0,  0, 15,  8,  8,  0,  0,  0,  0,  0}, LOAD,  0u },
  {91u, {  0,  0,  0,  0,  0,  8,  8,  0,  0,  0,  0,  0}, LOAD,  0u },
  {91u, {  0,  0,  0,  0,  0,  8,  8, 15,  0,  0,  0,  0}, LOAD,  0u },
  {91u, {  0,  0,  0,  0,  0,  8,  8,  0, 15,  0,  0,  0}, LOAD,  0u },
  {91u, {  0,  0,  0,  0,  0,  8,  8,  0,  0, 15,  0,  0}, LOAD,  0u },
  {91u, {  0,  0,  0,  0,  0,  8,  8,  0,  0,  0, 15,  0}, LOAD,  0u },
  {91u, {  0,  0,  0,  0,  0,  8,  8,  0,  0,  0,  0, 15}, LOAD,  0u },
};
//! \brief 20 step Rainbow, 1000ms per cycle
CODE const S_ANIMATION_INSTRUCTION_RGB gasStepping_hoember_RGB[ 20u ] = 
{
 { 455u, {0, 0, 0}, LOAD, 0u },
 { 91u, {8, 8, 0}, LOAD, 0u },
 { 455u, {0, 0, 0}, LOAD, 0u },
  // USOURCE | REPEAT could be used here, but it would be complicated and a bit wrong due 12 steps
};

//--------------------------------------------------------
//! \brief One led going around for "mezi" (eyes included) at 1000ms per cycle, while buttons are dimming in offset
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasStepping_mezi[ 11u ] = 
{
  {100u, { 15,  0, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, { 12,  3,  0, 15,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, {  9,  6,  0,  0, 15,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, {  6,  9,  0,  0, 0,  15,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, {  3, 12,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, {  0, 15,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0}, LOAD,  0u },
  {100u, {  3, 12,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0}, LOAD,  0u },
  {100u, {  6,  9,  0,  0,  0,  0,  0,  0,  0, 15,  0,  0}, LOAD,  0u },
  {100u, {  9,  6,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0}, LOAD,  0u },
  {100u, { 12,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15}, LOAD,  0u },
};
//! \brief no RGB on "mezi"
CODE const S_ANIMATION_INSTRUCTION_RGB gasStepping_mezi_RGB[ 2u ] = 
{
  { 1000u, {0,  0,  0}, LOAD, 0u },
};

//--------------------------------------------------------
//! \brief Two leds going around on two sides in tandem bottom up - then from top to bottom for "Hopehely" at 1200ms per cycle
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasKITT_hopehely[ 12u ] = 
{
  //        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11
  {100u, { 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, {  0, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15}, LOAD,  0u },
  {100u, {  0,  0, 15,  0,  0,  0,  0,  0,  0,  0, 15,  0}, LOAD,  0u },
  {100u, {  0,  0,  0, 15,  0,  0,  0,  0,  0, 15,  0,  0}, LOAD,  0u },
  {100u, {  0,  0,  0,  0, 15,  0,  0,  0, 15,  0,  0,  0}, LOAD,  0u },
  {100u, {  0,  0,  0,  0,  0, 15,  0, 15,  0,  0,  0,  0}, LOAD,  0u },
  {100u, {  0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0}, LOAD,  0u },
  {100u, {  0,  0,  0,  0,  0, 15,  0, 15,  0,  0,  0,  0}, LOAD,  0u },
  {100u, {  0,  0,  0,  0, 15,  0,  0,  0, 15,  0,  0,  0}, LOAD,  0u },
  {100u, {  0,  0,  0, 15,  0,  0,  0,  0,  0, 15,  0,  0}, LOAD,  0u },
  {100u, {  0,  0, 15,  0,  0,  0,  0,  0,  0,  0, 15,  0}, LOAD,  0u },
  {100u, {  0, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0, 15}, LOAD,  0u },
};
//! \brief constant orange
CODE const S_ANIMATION_INSTRUCTION_RGB gasKITT_hopehely_RGB[ 4u ] = 
{
  { 100u, {8, 8, 0}, LOAD, 0u },
  { 500u, {0, 0, 0}, LOAD, 0u },
  { 100u, {8, 8, 0}, LOAD, 0u },
  { 500u, {0, 0, 0}, LOAD, 0u },
};

//--------------------------------------------------------
//! \brief Inner and outer leds are fading in offset on "hopehely" at 2000ms per cycle
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasSplit2_fade_hopehely[ 10u ] = 
{
  //        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11
  {200u, { 15,  0, 15,  0, 15,  0, 15,  0, 15,  0, 15,  0}, LOAD,  0u },
  {200u, { 12,  3, 12,  3, 12,  3, 12,  3, 12,  3, 12,  3}, LOAD,  0u },
  {200u, {  9,  6,  9,  6,  9,  6,  9,  6,  9,  6,  9,  6}, LOAD,  0u },
  {200u, {  6,  9,  6,  9,  6,  9,  6,  9,  6,  9,  6,  9}, LOAD,  0u },
  {200u, {  3, 12,  3, 12,  3, 12,  3, 12,  3, 12,  3, 12}, LOAD,  0u },
  {200u, {  0, 15,  0, 15,  0, 15,  0, 15,  0, 15,  0, 15}, LOAD,  0u },
  {200u, {  3, 12,  3, 12,  3, 12,  3, 12,  3, 12,  3, 12}, LOAD,  0u },
  {200u, {  6,  9,  6,  9,  6,  9,  6,  9,  6,  9,  6,  9}, LOAD,  0u },
  {200u, {  9,  6,  9,  6,  9,  6,  9,  6,  9,  6,  9,  6}, LOAD,  0u },
  {200u, { 12,  3, 12,  3, 12,  3, 12,  3, 12,  3, 12,  3}, LOAD,  0u },
};
//! \brief 10 step red fade, 2000ms per cycle
CODE const S_ANIMATION_INSTRUCTION_RGB gasSplit2_fade_hopehely_RGB[ 10u ] = 
{
 {200u, { 15, 15, 15}, LOAD, 0u },
 {200u, { 12, 12, 15}, LOAD, 0u },
 {200u, {  9,  9, 15}, LOAD, 0u },
 {200u, {  6,  6, 15}, LOAD, 0u },
 {200u, {  3,  3, 15}, LOAD, 0u },
 {200u, {  0,  0, 15}, LOAD, 0u },
 {200u, {  3,  3, 15}, LOAD, 0u },
 {200u, {  6,  6, 15}, LOAD, 0u },
 {200u, {  9,  9, 15}, LOAD, 0u },
 {200u, { 12, 12, 15}, LOAD, 0u },
  // USOURCE | REPEAT could be used here, but it would be complicated and a bit wrong due 12 steps
};

//--------------------------------------------------------
//! \brief Inner and outer leds are fading in offset on "hopehely" at 2000ms per cycle (easing variant)
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasSplit2_fade_ease_hopehely[ 10u ] = 
{
  // Alternative, with ease in-out.
  //        0, 1,  2, 3,  4, 5,  6, 7,  8, 9, 10, 11
  {200u, { 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0}, LOAD, 0u },
  {200u, { 13, 2, 13, 2, 13, 2, 13, 2, 13, 2, 13, 2}, LOAD, 0u },
  {200u, { 10, 5, 10, 5, 10, 5, 10, 5, 10, 5, 10, 5}, LOAD, 0u },
  {200u, { 5, 10, 5, 10, 5, 10, 5, 10, 5, 10, 5, 10}, LOAD, 0u },
  {200u, { 2, 13, 2, 13, 2, 13, 2, 13, 2, 13, 2, 13}, LOAD, 0u },
  {200u, { 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15}, LOAD, 0u },
  {200u, { 2, 13, 2, 13, 2, 13, 2, 13, 2, 13, 2, 13}, LOAD, 0u },
  {200u, { 5, 10, 5, 10, 5, 10, 5, 10, 5, 10, 5, 10}, LOAD, 0u },
  {200u, { 10, 5, 10, 5, 10, 5, 10, 5, 10, 5, 10, 5}, LOAD, 0u },
  {200u, { 13, 2, 13, 2, 13, 2, 13, 2, 13, 2, 13, 2}, LOAD, 0u },
};
//! \brief 10 step red fade (eased), 2000ms per cycle
CODE const S_ANIMATION_INSTRUCTION_RGB gasSplit2_fade_ease_hopehely_RGB[ 10u ] = 
{
 {200u, { 15, 15, 0}, LOAD, 0u },
 {200u, { 13, 13, 0}, LOAD, 0u },
 {200u, { 10, 10, 0}, LOAD, 0u },
 {200u, {  5,  5, 0}, LOAD, 0u },
 {200u, {  2,  2, 0}, LOAD, 0u },
 {200u, {  0,  0, 0}, LOAD, 0u },
 {200u, {  2,  2, 0}, LOAD, 0u },
 {200u, {  5,  5, 0}, LOAD, 0u },
 {200u, { 10, 10, 0}, LOAD, 0u },
 {200u, { 13, 13, 0}, LOAD, 0u },
  // USOURCE | REPEAT could be used here, but it would be complicated and a bit wrong due 12 steps
};

//--------------------------------------------------------
//! \brief Split2 -- normal LEDs for "mezi"
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasSplit2_mezi[ 2u ] = 
{
 //        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11
  {500u, { 8,  0, 15,  0, 15,  0,  8,  8, 15,  0, 15,  0}, LOAD,  0u },
  {500u, { 8, 15,  0, 15,  0, 15,  8,  8,  0, 15,  0, 15}, LOAD,  0u },
};
CODE const S_ANIMATION_INSTRUCTION_RGB gasSplit2_mezi_RGB[ 2u ] = 
{
  { 1000u, {0,  0,  0}, LOAD, 0u },
};

//! \brief Generic flasher animation with dimmed eyes for "mezi"
CODE const S_ANIMATION_INSTRUCTION_NORMAL gasFlasherNoEyes_mezi[ 2u ] = 
{
   //      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11
  {500u, {15, 15, 15, 15, 15, 15,  8,  8, 15, 15, 15, 15}, LOAD, 0u }, 
  {500u, { 0,  0,  0,  0,  0,  0,  8,  8,  0,  0,  0,  0}, LOAD, 0u },
};
CODE const S_ANIMATION_INSTRUCTION_RGB gasFlasherNoEyes_mezi_RGB[ 2u ] = 
{
  { 1000u, {0,  0,  0}, LOAD, 0u },
};

// *******************************************************
#ifdef KARIFA
//! \brief Table of animations
CODE const S_ANIMATION gasAnimations[ NUM_ANIMATIONS ] = 
{
  {sizeof(gasRetroVersion)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasRetroVersion,     sizeof(gasRetroVersionRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasRetroVersionRGB },
  {sizeof(gasSoftFlashing)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasSoftFlashing,     sizeof(gasSoftFlashingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasSoftFlashingRGB },
  //  {sizeof(gasShootingStar)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasShootingStar,     sizeof(gasShootingStarRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasShootingStarRGB },
  {sizeof(gasDisco)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),            gasDisco,            sizeof(gasDiscoRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),            gasDiscoRGB },
  {sizeof(gasStarLaunch)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),       gasStarLaunch,       sizeof(gasStarLaunchRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),       gasStarLaunchRGB },
  {sizeof(gasCrissCross)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),       gasCrissCross,       sizeof(gasCrissCrossRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),       gasCrissCrossRGB },
  {sizeof(gasGenericFlasher)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),   gasGenericFlasher,   sizeof(gasGenericFlasherRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),   gasGenericFlasherRGB },
  {sizeof(gasKITT)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),             gasKITT,             sizeof(gasKITTRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),             gasKITTRGB },
  {sizeof(gasPingpong)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasPingpong,         sizeof(gasPingpongRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasPingpongRGB },
  {sizeof(gasFadeRing)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasFadeRing,         sizeof(gasFadeRingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasFadeRingRGB },
  {sizeof(gasYingYang)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasYingYang,         sizeof(gasYingYangRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasYingYangRGB },
  {sizeof(gasPseudoRandomFade)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL), gasPseudoRandomFade, sizeof(gasPseudoRandomFadeRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasPseudoRandomFadeRGB },
  //  {sizeof(gasFadeout)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasFadeout,          sizeof(gasFadeoutRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasFadeoutRGB },
  {sizeof(gasFlicker)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasFlicker,          sizeof(gasFlickerRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasFlickerRGB },
  {sizeof(gasRace)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),             gasRace,             sizeof(gasRaceRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),             gasRaceRGB },
  {sizeof(gasSparkle)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasSparkle,          sizeof(gasSparkleRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasSparkleRGB },
  {sizeof(gasIce)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),              gasIce,              sizeof(gasIceRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),              gasIceRGB },
  {sizeof(gasSplit2)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),           gasSplit2,           sizeof(gasSplit2RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),           gasSplit2RGB },
  //  {sizeof(gasSplit3fade)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),       gasSplit3fade,       sizeof(gasSplit3fadeRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),       gasSplit3fadeRGB },
  {sizeof(gasStepping)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasStepping,         sizeof(gasSteppingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasSteppingRGB },
  // Last animation, don't change its location
  {sizeof(gasBlackness)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),        gasBlackness,        sizeof(gasBlacknessRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),        gasBlacknessRGB }
};
#endif

#ifdef HOEMBER
//! \brief Table of animations
CODE const S_ANIMATION gasAnimations[ NUM_ANIMATIONS ] = 
{
  {sizeof(gasRetroVersion)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasRetroVersion,     sizeof(gasRetroVersionRGB_yellow)/sizeof(S_ANIMATION_INSTRUCTION_RGB),  gasRetroVersionRGB_yellow },
  {sizeof(gasSoftFlashing)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasSoftFlashing,     sizeof(gasSoftFlashingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasSoftFlashingRGB },
  //  {sizeof(gasShootingStar)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasShootingStar,     sizeof(gasShootingStarRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasShootingStarRGB },
  {sizeof(gasDisco)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),            gasDisco,            sizeof(gasDiscoRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),            gasDiscoRGB },
  {sizeof(gasCrissCross)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),       gasCrissCross,       sizeof(gasCrissCrossRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),       gasCrissCrossRGB },
  {sizeof(gasGenericFlasher)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),   gasGenericFlasher,   sizeof(gasGenericFlasherRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),   gasGenericFlasherRGB },
  {sizeof(gasKITT)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),             gasKITT,             sizeof(gasKITTRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),             gasKITTRGB },
  {sizeof(gasPingpong)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasPingpong,         sizeof(gasPingpongRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasPingpongRGB },
  {sizeof(gasFadeRing)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasFadeRing,         sizeof(gasFadeRingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasFadeRingRGB },
  {sizeof(gasYingYang)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasYingYang,         sizeof(gasYingYangRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasYingYangRGB },
  {sizeof(gasPseudoRandomFade)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL), gasPseudoRandomFade, sizeof(gasPseudoRandomFadeRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasPseudoRandomFadeRGB },
  //  {sizeof(gasFadeout)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasFadeout,          sizeof(gasFadeoutRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasFadeoutRGB },
  {sizeof(gasFlicker)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasFlicker,          sizeof(gasFlickerRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasFlickerRGB },
  {sizeof(gasRace)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),             gasRace,             sizeof(gasRaceRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),             gasRaceRGB },
  {sizeof(gasSparkle)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasSparkle,          sizeof(gasSparkleRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasSparkleRGB },
  {sizeof(gasIce)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),              gasIce,              sizeof(gasIceRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),              gasIceRGB },
  {sizeof(gasSplit2)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),           gasSplit2,           sizeof(gasSplit2RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),           gasSplit2RGB },
  //  {sizeof(gasSplit3fade)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),       gasSplit3fade,       sizeof(gasSplit3fadeRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),       gasSplit3fadeRGB },
  {sizeof(gasStepping)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasStepping,         sizeof(gasSteppingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasSteppingRGB },
/*
  // NEW, TO BE TESTED
  {sizeof(gasStepping_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasStepping_hopehely, sizeof(gasStepping_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasStepping_hopehely_RGB}, 
  {sizeof(gasStepping_hoember)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasStepping_hoember, sizeof(gasStepping_hoember_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasStepping_hoember_RGB}, 
  {sizeof(gasStepping_mezi)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasStepping_mezi, sizeof(gasStepping_mezi_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasStepping_mezi_RGB}, 
  {sizeof(gasSplit2_fade_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasSplit2_fade_hopehely, sizeof(gasSplit2_fade_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasSplit2_fade_hopehely_RGB}, 
  {sizeof(gasSplit2_fade_ease_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasSplit2_fade_ease_hopehely, sizeof(gasSplit2_fade_ease_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasSplit2_fade_ease_hopehely_RGB}, 
  {sizeof(gasSplit2_mezi)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasSplit2_mezi, sizeof(gasSplit2_mezi_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasSplit2_mezi_RGB}, 
  {sizeof(gasFlasherNoEyes_mezi)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasFlasherNoEyes_mezi, sizeof(gasFlasherNoEyes_mezi_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasFlasherNoEyes_mezi_RGB}, 
  */
  // Last animation, don't change its location
  {sizeof(gasBlackness)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),        gasBlackness,        sizeof(gasBlacknessRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),        gasBlacknessRGB }
};
#endif

#ifdef HOPEHELY
//! \brief Table of animations
CODE const S_ANIMATION gasAnimations[ NUM_ANIMATIONS ] = 
{
  {sizeof(gasRetroVersion)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasRetroVersion,     sizeof(gasRetroVersionRGB_yellow)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasRetroVersionRGB_yellow },
  {sizeof(gasSoftFlashing)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasSoftFlashing,     sizeof(gasSoftFlashingRGB_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasSoftFlashingRGB_hopehely },
  {sizeof(gasShootingStar_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),  gasShootingStar_hopehely,     sizeof(gasShootingStar_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasShootingStar_hopehely_RGB },
  {sizeof(gasSplit2_fade_ease_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasSplit2_fade_ease_hopehely, sizeof(gasSplit2_fade_ease_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasSplit2_fade_ease_hopehely_RGB}, 
  {sizeof(gasGenericFlasher)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),   gasGenericFlasher,   sizeof(gasGenericFlasherRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),   gasGenericFlasherRGB },
  {sizeof(gasKITT_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),    gasKITT_hopehely,    sizeof(gasKITT_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),   gasKITT_hopehely_RGB },
  {sizeof(gasDisco)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),            gasDisco,            sizeof(gasDiscoRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),            gasDiscoRGB },
  {sizeof(gasFadeRing)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasFadeRing,         sizeof(gasFadeRingRGB_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_RGB),  gasFadeRingRGB_hopehely },
  {sizeof(gasYingYang)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasYingYang,         sizeof(gasYingYangRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasYingYangRGB },
  {sizeof(gasPseudoRandomFade)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL), gasPseudoRandomFade, sizeof(gasPseudoRandomFadeRGB_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasPseudoRandomFadeRGB_hopehely },
//  {sizeof(gasFadeout)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasFadeout,          sizeof(gasFadeoutRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasFadeoutRGB },
//  {sizeof(gasFlicker)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasFlicker,          sizeof(gasFlickerRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasFlickerRGB },
  {sizeof(gasRace_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),    gasRace_hopehely,             sizeof(gasRaceRGB_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_RGB),  gasRaceRGB_hopehely },
  {sizeof(gasSparkle)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasSparkle,          sizeof(gasSparkleRGB_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_RGB),  gasSparkleRGB_hopehely },
  {sizeof(gasIce_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasIce_hopehely,              sizeof(gasIceRGB_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_RGB),  gasIceRGB_hopehely },
  {sizeof(gasSplit2_fade_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasSplit2_fade_hopehely, sizeof(gasSplit2_fade_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasSplit2_fade_hopehely_RGB}, 
  {sizeof(gasStepping)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasStepping,         sizeof(gasSteppingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasSteppingRGB },
  // Last animation, don't change its location
  {sizeof(gasBlackness)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),        gasBlackness,        sizeof(gasBlacknessRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),        gasBlacknessRGB }
};
#endif

#ifdef MEZI
//! \brief Table of animations
CODE const S_ANIMATION gasAnimations[ NUM_ANIMATIONS ] = 
{
  // ALL OK
  {sizeof(gasRetroVersion)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasRetroVersion,     sizeof(gasRetroVersionRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasRetroVersionRGB },
  
  // ALL OK
  {sizeof(gasSoftFlashing)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasSoftFlashing,     sizeof(gasSoftFlashingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasSoftFlashingRGB },
  
  // ?
  //  {sizeof(gasShootingStar)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),     gasShootingStar,     sizeof(gasShootingStarRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),     gasShootingStarRGB },
  
  // ?
  {sizeof(gasDisco)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),            gasDisco,            sizeof(gasDiscoRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),            gasDiscoRGB },
  
  // ONLY KARIFA
  {sizeof(gasStarLaunch)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),       gasStarLaunch,       sizeof(gasStarLaunchRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),       gasStarLaunchRGB },
  
  // ONLY KARIFA HOEMBER
  {sizeof(gasCrissCross)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),       gasCrissCross,       sizeof(gasCrissCrossRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),       gasCrissCrossRGB },
  
  // ALL OK
  {sizeof(gasGenericFlasher)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),   gasGenericFlasher,   sizeof(gasGenericFlasherRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),   gasGenericFlasherRGB },
  
  // ONLY KARIFA HOEMBER
  {sizeof(gasKITT)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),             gasKITT,             sizeof(gasKITTRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),             gasKITTRGB },
  
  // NEW, TO BE TESTED
  // ONLY HOPEHELY
  {sizeof(gasKITT_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),             gasKITT_hopehely,             sizeof(gasKITT_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),             gasKITT_hopehely_RGB },
  {sizeof(gasKITT2_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),             gasKITT2_hopehely,             sizeof(gasKITT2_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),             gasKITT2_hopehely_RGB },

  // ?
  // ONLY KARIFA HOEMBER
  {sizeof(gasPingpong)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasPingpong,         sizeof(gasPingpongRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasPingpongRGB },
  
  // ALL OK
  {sizeof(gasFadeRing)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasFadeRing,         sizeof(gasFadeRingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasFadeRingRGB },

  // ? mezi ?
  // ALL OK 
  {sizeof(gasYingYang)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasYingYang,         sizeof(gasYingYangRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasYingYangRGB },
  
  // ALL OK
  {sizeof(gasPseudoRandomFade)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL), gasPseudoRandomFade, sizeof(gasPseudoRandomFadeRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasPseudoRandomFadeRGB },

  // ?
  //  {sizeof(gasFadeout)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasFadeout,          sizeof(gasFadeoutRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasFadeoutRGB },
  
  // ????
  {sizeof(gasFlicker)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasFlicker,          sizeof(gasFlickerRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasFlickerRGB },
  {sizeof(gasRace)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),             gasRace,             sizeof(gasRaceRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),             gasRaceRGB },
  {sizeof(gasSparkle)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),          gasSparkle,          sizeof(gasSparkleRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),          gasSparkleRGB },
  {sizeof(gasIce)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),              gasIce,              sizeof(gasIceRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),              gasIceRGB },
  
  // ONLY KARIFA HOEMBER HOPEHELY
  {sizeof(gasSplit2)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),           gasSplit2,           sizeof(gasSplit2RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),           gasSplit2RGB },
  //  {sizeof(gasSplit3fade)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),       gasSplit3fade,       sizeof(gasSplit3fadeRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),       gasSplit3fadeRGB },
  {sizeof(gasStepping)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),         gasStepping,         sizeof(gasSteppingRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),         gasSteppingRGB },
/*
  // NEW, TO BE TESTED
  {sizeof(gasStepping_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasStepping_hopehely, sizeof(gasStepping_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasStepping_hopehely_RGB}, 
  {sizeof(gasStepping_hoember)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasStepping_hoember, sizeof(gasStepping_hoember_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasStepping_hoember_RGB}, 
  {sizeof(gasStepping_mezi)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasStepping_mezi, sizeof(gasStepping_mezi_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasStepping_mezi_RGB}, 
  {sizeof(gasSplit2_fade_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasSplit2_fade_hopehely, sizeof(gasSplit2_fade_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasSplit2_fade_hopehely_RGB}, 
  {sizeof(gasSplit2_fade_ease_hopehely)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasSplit2_fade_ease_hopehely, sizeof(gasSplit2_fade_ease_hopehely_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasSplit2_fade_ease_hopehely_RGB}, 
  {sizeof(gasSplit2_mezi)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasSplit2_mezi, sizeof(gasSplit2_mezi_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasSplit2_mezi_RGB}, 
  {sizeof(gasFlasherNoEyes_mezi)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),gasFlasherNoEyes_mezi, sizeof(gasFlasherNoEyes_mezi_RGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB), gasFlasherNoEyes_mezi_RGB}, 
  */
  // Last animation, don't change its location
  {sizeof(gasBlackness)/sizeof(S_ANIMATION_INSTRUCTION_NORMAL),        gasBlackness,        sizeof(gasBlacknessRGB)/sizeof(S_ANIMATION_INSTRUCTION_RGB),        gasBlacknessRGB }
};
#endif


/***************************************< Global variables >**************************************/
IDATA U16 gu16NormalTimer;                    //!< Ms resolution timer for normal LED animation
IDATA U16 gu16RGBTimer;                       //!< Ms resolution timer for the RGB LED animation
IDATA U16 gu16LastCall;                       //!< The last time the main cycle was called
// Local variables
static IDATA U8 u8LastState = 0xFFu;          //!< Previously executed instruction index for normal LEDs
static IDATA U8 u8RepetitionCounter = 0u;     //!< Instruction repetition counter for normal LEDs
static IDATA U8 u8LastStateRGB = 0xFFu;       //!< Previously executed instruction index for RGB LED
static IDATA U8 u8RepetitionCounterRGB = 0u;  //!< Instruction repetition counter for RGB LED


/***************************************< Static function definitions >**************************************/
static I8 SaturateBrightness( U8* pu8BrightnessVariable );


/***************************************< Private functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Saturates the given brightness variable
//! \param  *pu8BrightnessVariable: pointer to the variable
//! \return How much the variable needed to change
//! \global -
//! \note   Changes the parameter too.
//-----------------------------------------------------------------------------
static I8 SaturateBrightness( U8* pu8BrightnessVariable )
{
  I8 i8Return = 0;
  if( (I8)*pu8BrightnessVariable < 0 )
  {
    i8Return = (I8)*pu8BrightnessVariable;
    *pu8BrightnessVariable = 0u;
  }
  else if( (I8)*pu8BrightnessVariable > 15u )
  {
    i8Return = (I8)*pu8BrightnessVariable - 15;
    *pu8BrightnessVariable = 15u;
  }
  return i8Return;
}


/***************************************< Public functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Initialize layer
//! \param  -
//! \return -
//! \global All globals in this layer.
//! \note   Should be called in the init block of the firmware.
//-----------------------------------------------------------------------------
void Animation_Init( void )
{
  gu16NormalTimer = 0u;
  gu16RGBTimer = 0u;
  gu16LastCall = Util_GetTimerMs();
}

//----------------------------------------------------------------------------
//! \brief  Check timer and update LED brightnesses based on the animation.
//! \param  -
//! \return -
//! \global -
//! \note   Should be called from main cycle.
//-----------------------------------------------------------------------------
void Animation_Cycle( void )
{
  U8  u8AnimationState;
  U16 u16StateTimer = 0u;
  U16 u16TimeNow = Util_GetTimerMs();
  U8  u8Index, u8InnerIndex;
  U8  u8OpCode;
  U8  u8Temp;
  I8  i8Change;
  
  // Check if time has elapsed since last call
  if( u16TimeNow != gu16LastCall )
  {
    // Increase the synchronized timer with the difference
    DISABLE_IT;
    gu16NormalTimer += ( u16TimeNow - gu16LastCall );
    gu16RGBTimer += ( u16TimeNow - gu16LastCall );
    ENABLE_IT;

    // Make sure not to overindex arrays
    if( gsPersistentData.u8AnimationIndex >= NUM_ANIMATIONS )
    {
      gsPersistentData.u8AnimationIndex = 0u;
    }
    
    // --------------------------------------< For the normal LEDs
    // Calculate the state of the animation
    for( u8AnimationState = 0u; u8AnimationState < gasAnimations[ gsPersistentData.u8AnimationIndex ].u8AnimationLengthNormal; u8AnimationState++ )
    {
      u16StateTimer += gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].u16TimingMs;
      if( u16StateTimer > gu16NormalTimer )
      {
        break;
      }
    }
    if( u8AnimationState >= gasAnimations[ gsPersistentData.u8AnimationIndex ].u8AnimationLengthNormal )
    {
      // restart animation
      u8AnimationState = 0u;
      DISABLE_IT;
      gu16NormalTimer = 0u;
      gu16RGBTimer = 0u;
      ENABLE_IT;
    }
    if( u8LastState != u8AnimationState )  // next instruction
    {
      u8OpCode = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].u8AnimationOpcode;
      // Just a load instruction, nothing more
      if( LOAD == u8OpCode )
      {
        memcpy( gau8LEDBrightness, (void*)gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness, LEDS_NUM );
        u8LastState = u8AnimationState;
      }
      else  // Other opcodes -- IMPORTANT: the order of operations are fixed!
      {
        // Add operation
        if( ADD & u8OpCode )
        {
          for( u8Index = 0u; u8Index < LEDS_NUM; u8Index++ )
          {
            gau8LEDBrightness[ u8Index ] += gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ u8Index ];
            if( gau8LEDBrightness[ u8Index ] > 15u )  // overflow/underflow happened
            {
              gau8LEDBrightness[ u8Index ] = 0u;
            }
          }
        }
        // Right shift operation
        if( RSHIFT & u8OpCode )
        {
          u8Temp = gau8LEDBrightness[ LEDS_NUM - 1u ];
          for( u8Index = LEDS_NUM - 1u; u8Index > 0u; u8Index-- )
          {
            gau8LEDBrightness[ u8Index ] = gau8LEDBrightness[ u8Index - 1u ];
          }
          gau8LEDBrightness[ 0u ] = u8Temp;
        }
        // Left shift operation
        if( LSHIFT & u8OpCode )
        {
          u8Temp = gau8LEDBrightness[ 0u ];
          for( u8Index = 0u; u8Index < (LEDS_NUM - 1u); u8Index++ )
          {
            gau8LEDBrightness[ u8Index ] = gau8LEDBrightness[ u8Index + 1u ];
          }
          gau8LEDBrightness[ LEDS_NUM - 1u ] = u8Temp;
        }
/*
        // Upward move operation
        if( UMOVE & u8OpCode )
        {
          // Left side
          for( u8Index = 0u; u8Index < (RIGHT_LEDS_START - 1u); u8Index++ )
          {
            i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].au8LEDBrightness[ u8Index ];
            gau8LEDBrightness[ u8Index ] -= i8Change;
            for( u8InnerIndex = u8Index; u8InnerIndex < (RIGHT_LEDS_START - 1u); u8InnerIndex++ )
            {
              i8Change += SaturateBrightness( &gau8LEDBrightness[ u8InnerIndex ] );
              gau8LEDBrightness[ u8InnerIndex + 1u ] += i8Change;
              i8Change = SaturateBrightness( &gau8LEDBrightness[ u8InnerIndex + 1u ] );
            }
          }
          i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].au8LEDBrightness[ RIGHT_LEDS_START - 1u ];
          gau8LEDBrightness[ RIGHT_LEDS_START - 1u ] -= i8Change;
          SaturateBrightness( &gau8LEDBrightness[ RIGHT_LEDS_START - 1u ] );
          // Right side
          for( u8Index = LEDS_NUM - 1u; u8Index > RIGHT_LEDS_START; u8Index-- )
          {
            i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].au8LEDBrightness[ u8Index ];
            if( (I8)gau8LEDBrightness[ u8Index ] - i8Change < 0u )  // saturation downwards
            {
              gau8LEDBrightness[ u8Index - 1u ] += gau8LEDBrightness[ u8Index ];
            }
            else  // no saturation
            {
              gau8LEDBrightness[ u8Index - 1u ] += i8Change;
            }
            gau8LEDBrightness[ u8Index ] -= i8Change;
            SaturateBrightness( &gau8LEDBrightness[ u8Index ] );
            SaturateBrightness( &gau8LEDBrightness[ u8Index - 1u ] );  // saturate the next LED too
          }
          i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].au8LEDBrightness[ RIGHT_LEDS_START ];
          gau8LEDBrightness[ RIGHT_LEDS_START ] -= i8Change;
          SaturateBrightness( &gau8LEDBrightness[ RIGHT_LEDS_START ] );
        }
        // Downward move operation
        if( DMOVE & u8OpCode )
        {
          //TODO: this works for positive move values only!
          // Left side
          for( u8Index = (RIGHT_LEDS_START - 1u); u8Index > 0u ; u8Index-- )
          {
            i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].au8LEDBrightness[ u8Index ];
            if( (I8)gau8LEDBrightness[ u8Index ] - i8Change < 0u )  // saturation downwards
            {
              gau8LEDBrightness[ u8Index - 1u ] += gau8LEDBrightness[ u8Index ];
            }
            else  // no saturation
            {
              gau8LEDBrightness[ u8Index - 1u ] += i8Change;
            }
            gau8LEDBrightness[ u8Index ] -= i8Change;
            SaturateBrightness( &gau8LEDBrightness[ u8Index ] );
            SaturateBrightness( &gau8LEDBrightness[ u8Index - 1u ] );  // saturate the next LED too
          }
          i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].au8LEDBrightness[ 0u ];
          gau8LEDBrightness[ 0u ] -= i8Change;
          SaturateBrightness( &gau8LEDBrightness[ 0u ] );
          // Right side
          for( u8Index = RIGHT_LEDS_START; u8Index < (LEDS_NUM - 1u); u8Index++ )
          {
            i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].au8LEDBrightness[ u8Index ];
            if( (I8)gau8LEDBrightness[ u8Index ] - i8Change < 0u )  // saturation downwards
            {
              gau8LEDBrightness[ u8Index + 1u ] += gau8LEDBrightness[ u8Index ];
            }
            else  // no saturation
            {
              gau8LEDBrightness[ u8Index + 1u ] += i8Change;
            }
            gau8LEDBrightness[ u8Index ] -= i8Change;
            SaturateBrightness( &gau8LEDBrightness[ u8Index ] );
            SaturateBrightness( &gau8LEDBrightness[ u8Index + 1u ] );  // saturate the next LED too
          }
          i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].au8LEDBrightness[ LEDS_NUM - 1u ];
          gau8LEDBrightness[ LEDS_NUM - 1u ] -= i8Change;
          SaturateBrightness( &gau8LEDBrightness[ LEDS_NUM - 1u ] );
        }
*/
        // Upward source instruction
        if( USOURCE & u8OpCode )
        {
          // Left side
          for( u8Index = 0u; u8Index < (RIGHT_LEDS_START - 1u); u8Index++ )
          {
            i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ u8Index ];
            gau8LEDBrightness[ u8Index ] += i8Change;
            for( u8InnerIndex = u8Index; u8InnerIndex < (RIGHT_LEDS_START - 1u); u8InnerIndex++ )
            {
              gau8LEDBrightness[ u8InnerIndex + 1u ] += SaturateBrightness( &gau8LEDBrightness[ u8InnerIndex ] );
            }
          }
          i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ RIGHT_LEDS_START - 1u ];
          gau8LEDBrightness[ RIGHT_LEDS_START - 1u ] += i8Change;
          SaturateBrightness( &gau8LEDBrightness[ RIGHT_LEDS_START - 1u ] );
          // Right side
          for( u8Index = LEDS_NUM - 1u; u8Index > RIGHT_LEDS_START; u8Index-- )
          {
            i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ u8Index ];
            gau8LEDBrightness[ u8Index ] += i8Change;
            for( u8InnerIndex = LEDS_NUM - 1u; u8InnerIndex > RIGHT_LEDS_START; u8InnerIndex-- )
            {
              gau8LEDBrightness[ u8InnerIndex - 1u ] += SaturateBrightness( &gau8LEDBrightness[ u8InnerIndex ] );
            }
          }
          i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ RIGHT_LEDS_START ];
          gau8LEDBrightness[ RIGHT_LEDS_START ] += i8Change;
          SaturateBrightness( &gau8LEDBrightness[ RIGHT_LEDS_START ] );
        }
        // Downward source instruction
        if( DSOURCE & u8OpCode )
        {
          // Left side
          for( u8Index = (RIGHT_LEDS_START - 1u); u8Index > 0u; u8Index-- )
          {
            i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ u8Index ];
            gau8LEDBrightness[ u8Index ] += i8Change;
            for( u8InnerIndex = u8Index; u8InnerIndex > 0u; u8InnerIndex-- )
            {
              gau8LEDBrightness[ u8InnerIndex - 1u ] += SaturateBrightness( &gau8LEDBrightness[ u8InnerIndex ] );
            }
          }
          i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ 0u ];
          gau8LEDBrightness[ 0u ] += i8Change;
          SaturateBrightness( &gau8LEDBrightness[ 0u ] );
          // Right side
          for( u8Index = RIGHT_LEDS_START; u8Index < (LEDS_NUM - 1u); u8Index++ )
          {
            i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ u8Index ];
            gau8LEDBrightness[ u8Index ] += i8Change;
            for( u8InnerIndex = RIGHT_LEDS_START; u8InnerIndex < (LEDS_NUM - 1u); u8InnerIndex++ )
            {
              gau8LEDBrightness[ u8InnerIndex + 1u ] += SaturateBrightness( &gau8LEDBrightness[ u8InnerIndex ] );
            }
          }
          i8Change = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ LEDS_NUM - 1u ];
          gau8LEDBrightness[ LEDS_NUM - 1u ] += i8Change;
          SaturateBrightness( &gau8LEDBrightness[ LEDS_NUM - 1u ] );
        }
        // Divide instruction
        if( DIV & u8OpCode )
        {
          for( u8Index = 0u; u8Index < LEDS_NUM; u8Index++ )
          {
            u8Temp = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].ai8LEDBrightness[ u8Index ];
            if( u8Temp != 0u )
            {
              gau8LEDBrightness[ u8Index ] /= u8Temp;
            }
          }
        }
        // Repeat instruction
        if( REPEAT & u8OpCode )
        {
          // If we're here the first time
          if( 0u == u8RepetitionCounter )
          {
            u8RepetitionCounter = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].u8AnimationOperand;
            // Step back in time
            gu16NormalTimer -= gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].u16TimingMs;
          }
          else  // We're already repeating...
          {
            u8RepetitionCounter--;
            if( 0u != u8RepetitionCounter )
            {
              // Step back in time
              gu16NormalTimer -= gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsNormal[ u8AnimationState ].u16TimingMs;
            }
            else  // No more repeating
            {
             u8LastState = u8AnimationState;
            }
          }
        }
        else  // if there's no repeat opcode
        {
          u8LastState = u8AnimationState;  // save that this operation is finished
        }
      }
    }
    
    // --------------------------------------< For the RGB LED
    // Calculate the state of the animation
    u16StateTimer = 0u;
    for( u8AnimationState = 0u; u8AnimationState < gasAnimations[ gsPersistentData.u8AnimationIndex ].u8AnimationLengthRGB; u8AnimationState++ )
    {
      u16StateTimer += gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsRGB[ u8AnimationState ].u16TimingMs;
      if( u16StateTimer > gu16RGBTimer )
      {
        break;
      }
    }
/*
    if( u8AnimationState >= gasAnimations[ gsPersistentData.u8AnimationIndex ].u8AnimationLengthRGB )
    {
      // restart animation
      u8AnimationState = 0u;
      DISABLE_IT;
      gu16SynchronizedTimer = 0;
      ENABLE_IT;
    }
*/
    if( u8LastStateRGB != u8AnimationState )  // next instruction
    {
      u8OpCode = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsRGB[ u8AnimationState ].u8AnimationOpcode;
      // Just a load instruction, nothing more
      if( LOAD == u8OpCode )
      {
        memcpy( (U8*)gau8RGBLEDs, (void*)gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsRGB[ u8AnimationState ].ai8RGBLEDBrightness, NUM_RGBLED_COLORS );
        u8LastStateRGB = u8AnimationState;
      }
      else  // Other opcodes -- IMPORTANT: the order of operations are fixed!
      {
        // Add operation
        if( ADD & u8OpCode )
        {
          for( u8Index = 0u; u8Index < NUM_RGBLED_COLORS; u8Index++ )
          {
            gau8RGBLEDs[ u8Index ] += gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsRGB[ u8AnimationState ].ai8RGBLEDBrightness[ u8Index ];
            if( gau8RGBLEDs[ u8Index ] > 15u )  // overflow/underflow happened
            {
              gau8RGBLEDs[ u8Index ] = 0u;
            }
          }
        }
        // Right shift operation
        if( RSHIFT & u8OpCode )
        {
          // Not implemented
        }
        // Left shift operation
        if( LSHIFT & u8OpCode )
        {
          // Not implemented
        }
/*
        // Upward move operation
        if( UMOVE & u8OpCode )
        {
          // Not implemented
        }
        // Downward move operation
        if( DMOVE & u8OpCode )
        {
          // Not implemented
        }
*/
        if( USOURCE & u8OpCode )
        {
          // Not implemented
        }
        if( DSOURCE & u8OpCode )
        {
          // Not implemented
        }
        if( DIV & u8OpCode )
        {
          for( u8Index = 0u; u8Index < NUM_RGBLED_COLORS; u8Index++ )
          {
            u8Temp = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsRGB[ u8AnimationState ].ai8RGBLEDBrightness[ u8Index ];
            if( u8Temp != 0u )
            {
              gau8RGBLEDs[ u8Index ] /= u8Temp;
            }
          }
        }
        // Repeat instruction
        if( REPEAT & u8OpCode )
        {
          // If we're here the first time
          if( 0u == u8RepetitionCounterRGB )
          {
            u8RepetitionCounterRGB = gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsRGB[ u8AnimationState ].u8AnimationOperand;
            // Step back in time
            gu16RGBTimer -= gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsRGB[ u8AnimationState ].u16TimingMs;
          }
          else  // We're already repeating...
          {
            u8RepetitionCounterRGB--;
            if( 0u != u8RepetitionCounterRGB )
            {
              // Step back in time
              gu16RGBTimer -= gasAnimations[ gsPersistentData.u8AnimationIndex ].psInstructionsRGB[ u8AnimationState ].u16TimingMs;
            }
            else  // No more repeating
            {
             u8LastStateRGB = u8AnimationState;
            }
          }
        }
        else  // if there's no repeat opcode
        {
          u8LastStateRGB = u8AnimationState;  // save that this operation is finished
        }
      }
    }    
    // Store the timestamp
    gu16LastCall = u16TimeNow;
  }
}

//----------------------------------------------------------------------------
//! \brief  Set the new animation
//! \param  -
//! \return -
//! \global -
//! \note   Should be called from main cycle only!
//-----------------------------------------------------------------------------
void Animation_Set( U8 u8AnimationIndex )
{
  if( u8AnimationIndex < NUM_ANIMATIONS )
  {
    gsPersistentData.u8AnimationIndex = u8AnimationIndex;
    DISABLE_IT;
    gu16NormalTimer = 0u;
    gu16RGBTimer = 0u;
    ENABLE_IT;
    u8LastState = 0xFFu;
    u8RepetitionCounter = 0u;
    u8LastStateRGB = 0xFFu;
    u8RepetitionCounterRGB = 0u;
  }
}


/***************************************< End of file >**************************************/
