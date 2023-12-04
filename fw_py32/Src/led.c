/*! *******************************************************************************************************
* Copyright (c) 2021-2023 Hekk_Elek
*
* \file led.c
*
* \brief Soft-PWM LED driver
*
* \author Hekk_Elek
*
**********************************************************************************************************/

/***************************************< Includes >**************************************/
// Own includes
#include "main.h"
#include "types.h"
#include "config.h"
#include "led.h"


/***************************************< Definitions >**************************************/
#define PWM_LEVELS      (16u)  //!< PWM levels implemented: [0; PWM_LEVELS)

#ifdef KARIFA
  // Pin definitions
  #define MPX1            GPIOA,LL_GPIO_PIN_6  //!< Pin of MPX1 multiplexer pin
  #define MPX2            GPIOA,LL_GPIO_PIN_5  //!< Pin of MPX2 multiplexer pin
  #define LED0            GPIOB,LL_GPIO_PIN_1  //!< Pin of LED0 common pin
  #define LED1            GPIOA,LL_GPIO_PIN_4  //!< Pin of LED1 common pin
  #define LED2            GPIOA,LL_GPIO_PIN_7  //!< Pin of LED2 common pin
  #define LED3            GPIOB,LL_GPIO_PIN_2  //!< Pin of LED3 common pin
  #define LED4            GPIOB,LL_GPIO_PIN_0  //!< Pin of LED4 common pin
  #define LED5            GPIOA,LL_GPIO_PIN_2  //!< Pin of LED5 common pin
#endif
#ifdef HOEMBER
  // Pin definitions
  #define MPX1            GPIOA,LL_GPIO_PIN_6  //!< Pin of MPX1 multiplexer pin
  #define MPX2            GPIOA,LL_GPIO_PIN_5  //!< Pin of MPX2 multiplexer pin
  #define LED0            GPIOB,LL_GPIO_PIN_1  //!< Pin of LED0 common pin
  #define LED1            GPIOB,LL_GPIO_PIN_2  //!< Pin of LED1 common pin
  #define LED2            GPIOA,LL_GPIO_PIN_2  //!< Pin of LED2 common pin
  #define LED3            GPIOA,LL_GPIO_PIN_4  //!< Pin of LED3 common pin
  #define LED4            GPIOA,LL_GPIO_PIN_7  //!< Pin of LED4 common pin
  #define LED5            GPIOB,LL_GPIO_PIN_0  //!< Pin of LED5 common pin
#endif
#ifdef HOPEHELY
  // Pin definitions
  #define MPX1            GPIOA,LL_GPIO_PIN_5  //!< Pin of MPX1 multiplexer pin
  #define MPX2            GPIOA,LL_GPIO_PIN_6  //!< Pin of MPX2 multiplexer pin
  #define LED0            GPIOB,LL_GPIO_PIN_1  //!< Pin of LED0 common pin
  #define LED1            GPIOB,LL_GPIO_PIN_2  //!< Pin of LED1 common pin
  #define LED2            GPIOB,LL_GPIO_PIN_0  //!< Pin of LED2 common pin
  #define LED3            GPIOA,LL_GPIO_PIN_7  //!< Pin of LED3 common pin
  #define LED4            GPIOA,LL_GPIO_PIN_2  //!< Pin of LED4 common pin
  #define LED5            GPIOA,LL_GPIO_PIN_4  //!< Pin of LED5 common pin
#endif
/*
#ifdef MEZI
  // Pin definitions
  #define MPX1            GPIOA,LL_GPIO_PIN_6  //!< Pin of MPX1 multiplexer pin
  #define MPX2            GPIOA,LL_GPIO_PIN_5  //!< Pin of MPX2 multiplexer pin
  #define LED0            GPIOB,LL_GPIO_PIN_1  //!< Pin of LED0 common pin
  #define LED1            GPIOA,LL_GPIO_PIN_4  //!< Pin of LED1 common pin
  #define LED2            GPIOA,LL_GPIO_PIN_7  //!< Pin of LED2 common pin
  #define LED3            GPIOB,LL_GPIO_PIN_2  //!< Pin of LED3 common pin
  #define LED4            GPIOB,LL_GPIO_PIN_0  //!< Pin of LED4 common pin
  #define LED5            GPIOA,LL_GPIO_PIN_2  //!< Pin of LED5 common pin
#endif
*/
#ifndef MPX1
  #error "Unknown hardware selected!"
#endif


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/
DATA U8 gau8LEDBrightness[ LEDS_NUM ];  //!< Array for storing individual brightness levels
DATA U8 gu8PWMCounter;                  //!< Counter for the base of soft-PWM
DATA BIT gbitSide;                      //!< Stores which side of the panel is active


/***************************************< Static function definitions >**************************************/


/***************************************< Private functions >**************************************/


/***************************************< Public functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Initialize all IO pins associated with LEDs
//! \param  -
//! \return -
//! \global gau8LEDBrightness[], gu8PWMCounter
//! \note   Should be called in the init block
//-----------------------------------------------------------------------------
void LED_Init( void )
{
  LL_GPIO_InitTypeDef TIM1CH1MapInit = {0};
  U8 u8Index;
  
  // Init globals
  gu8PWMCounter = 0;
  for( u8Index = 0; u8Index < LEDS_NUM; u8Index++ )
  {
    gau8LEDBrightness[ u8Index ] = 0;
  }

  gbitSide = 0u;
  
  // Enable clocks
  LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOA );
  LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOB );
  
  // Initialize GPIO pins
  /* Default output states */
  LL_GPIO_WriteOutputPort( GPIOA, 0u );
  LL_GPIO_WriteOutputPort( GPIOB, 0u );
  LL_GPIO_SetOutputPin( MPX1 );  // MPX1 starts as 1
  
  /* GPIOA */
  TIM1CH1MapInit.Pin        = LL_GPIO_PIN_2 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
  TIM1CH1MapInit.Mode       = LL_GPIO_MODE_OUTPUT;
  TIM1CH1MapInit.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  TIM1CH1MapInit.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  LL_GPIO_Init( GPIOA, &TIM1CH1MapInit );

  /* GPIOB */
  TIM1CH1MapInit.Pin        = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2;
  TIM1CH1MapInit.Mode       = LL_GPIO_MODE_OUTPUT;
  TIM1CH1MapInit.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  TIM1CH1MapInit.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  LL_GPIO_Init( GPIOB, &TIM1CH1MapInit );
}

//----------------------------------------------------------------------------
//! \brief  Interrupt routine to implement soft-PWM
//! \param  -
//! \return -
//! \global gau8LEDBrightness[], gu8PWMCounter
//! \note   Should be called from periodic timer interrupt routine.
//-----------------------------------------------------------------------------
void LED_Interrupt( void )
{
  gu8PWMCounter++;
  if( gu8PWMCounter == PWM_LEVELS )
  {
    gu8PWMCounter = 0;
    gbitSide ^= 1;
    // Set multiplexer pins
    LL_GPIO_TogglePin( MPX1 );
    LL_GPIO_TogglePin( MPX2 );
  }
  
  //NOTE: unfortunately SFRs cannot be put in an array, so this cannot be implented as a for cycle
  if( gbitSide )  // left side
  {
    if( gau8LEDBrightness[ 0u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED0 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED0 );
    }
    if( gau8LEDBrightness[ 1u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED1 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED1 );
    }
    if( gau8LEDBrightness[ 2u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED2 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED2 );
    }
    if( gau8LEDBrightness[ 3u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED3 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED3 );
    }
    if( gau8LEDBrightness[ 4u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED4 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED4 );
    }
    if( gau8LEDBrightness[ 5u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED5 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED5 );
    }
  }
  else  // right side
  {
    if( gau8LEDBrightness[ 6u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED5 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED5 );
    }
    if( gau8LEDBrightness[ 7u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED4 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED4 );
    }
    if( gau8LEDBrightness[ 8u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED3 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED3 );
    }
    if( gau8LEDBrightness[ 9u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED2 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED2 );
    }
    if( gau8LEDBrightness[ 10u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED1 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED1 );
    }
    if( gau8LEDBrightness[ 11u ] > gu8PWMCounter )
    {
      LL_GPIO_SetOutputPin( LED0 );
    }
    else
    {
      LL_GPIO_ResetOutputPin( LED0 );
    }
  }
}


/***************************************< End of file >**************************************/
