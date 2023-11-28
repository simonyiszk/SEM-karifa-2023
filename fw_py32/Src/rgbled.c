/*! *******************************************************************************************************
* Copyright (c) 2022-2023 Hekk_Elek
*
* \file rgbled.c
*
* \brief RGB LED driver using current-mode pulse control
*
* \author Hekk_Elek
*
**********************************************************************************************************/

/***************************************< Includes >**************************************/
#include "main.h"
#include <string.h>

// Own includes
#include "types.h"
#include "rgbled.h"


/***************************************< Definitions >**************************************/
#define COLOR_LEVELS       (16u)  //!< Number of brightness levels per color
#define PWM_BRIGHT         (36u)  //!< PWM duty cycle for bright color -- 3 us pulse
#define PWM_DARK            (0u)  //!< PWM duty cycle for darkness


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/
//! \brief Global array for RGB LED color values
//! \note  Value set is between [0; COLOR_LEVELS)
volatile U8 gau8RGBLEDs[ NUM_RGBLED_COLORS ];


/***************************************< Static function definitions >**************************************/


/***************************************< Private functions >**************************************/


/***************************************< Public functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Initialize hardware and software layer
//! \param  -
//! \return -
//! \global gau8RGBLEDs
//-----------------------------------------------------------------------------
void RGBLED_Init( void )
{
  LL_GPIO_InitTypeDef TIM1CH1MapInit= {0};
  LL_TIM_OC_InitTypeDef TIM_OC_Initstruct ={0};
  LL_TIM_InitTypeDef TIM1CountInit = {0};

  // Initialize global variables
  memset( (U8*)gau8RGBLEDs, 0, NUM_RGBLED_COLORS );
  
  // Enable clocks
  LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_TIM1 );
  LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOA );
  
  // Initialize GPIO pins
  /* Initialize PA0/PA1/PA3 as TIM1_CH3/TIM1_CH4/TIM1_CH1, respectively */
  TIM1CH1MapInit.Pin        = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_3;
  TIM1CH1MapInit.Mode       = LL_GPIO_MODE_ALTERNATE;
  TIM1CH1MapInit.Alternate  = LL_GPIO_AF_13;
  LL_GPIO_Init( GPIOA, &TIM1CH1MapInit );

  // Configure PWM channels
  TIM_OC_Initstruct.OCMode        = LL_TIM_OCMODE_PWM1;
  TIM_OC_Initstruct.OCState       = LL_TIM_OCSTATE_ENABLE;
  TIM_OC_Initstruct.OCPolarity    = LL_TIM_OCPOLARITY_LOW;
  TIM_OC_Initstruct.OCIdleState   = LL_TIM_OCIDLESTATE_HIGH;
  // Set CH1
  TIM_OC_Initstruct.CompareValue  = PWM_DARK;
  LL_TIM_OC_Init( TIM1, LL_TIM_CHANNEL_CH1, &TIM_OC_Initstruct );
  // Set CH3
  TIM_OC_Initstruct.CompareValue  = PWM_DARK;
  LL_TIM_OC_Init( TIM1, LL_TIM_CHANNEL_CH3, &TIM_OC_Initstruct );
  // Set CH4
  TIM_OC_Initstruct.CompareValue  = PWM_DARK;
  LL_TIM_OC_Init( TIM1, LL_TIM_CHANNEL_CH4, &TIM_OC_Initstruct );
  
  // Initialize TIM1 base
  TIM1CountInit.ClockDivision       = LL_TIM_CLOCKDIVISION_DIV1;
  TIM1CountInit.CounterMode         = LL_TIM_COUNTERMODE_UP;
  TIM1CountInit.Prescaler           = 1;
  TIM1CountInit.Autoreload          = 1200u - 1u;  // Period: 100 usec / 10 kHz @ 24 MHz system clock
  TIM1CountInit.RepetitionCounter   = 0;
  LL_TIM_Init( TIM1, &TIM1CountInit );

  // Enable output drive
  LL_TIM_EnableAllOutputs( TIM1 );

  // Start counting
  LL_TIM_EnableCounter( TIM1 );
}

//----------------------------------------------------------------------------
//! \brief  Interrupt routine for timer-controlled RGB LED driver
//! \param  -
//! \return -
//! \global gau8RGBLEDs
//! \note   Should be called from periodic timer interrupt routine.
//-----------------------------------------------------------------------------
void RGBLED_Interrupt( void )
{
  static U8 u8Cnt = 0u;
  
  // Red
  if( gau8RGBLEDs[ 0 ] > u8Cnt )
  {
    // Pulse for 1 usec
    LL_TIM_OC_SetCompareCH3( TIM1, PWM_BRIGHT );
  }
  else
  {
    // No pulse
    LL_TIM_OC_SetCompareCH3( TIM1, PWM_DARK );
  }
  
  // Green
  if( gau8RGBLEDs[ 1 ] > u8Cnt )
  {
    // Pulse for 1 usec
    LL_TIM_OC_SetCompareCH1( TIM1, PWM_BRIGHT );
  }
  else
  {
    // No pulse
    LL_TIM_OC_SetCompareCH1( TIM1, PWM_DARK );
  }
  
  // Blue
  if( gau8RGBLEDs[ 2 ] > u8Cnt )
  {
    // Pulse for 1 usec
    LL_TIM_OC_SetCompareCH4( TIM1, PWM_BRIGHT );
  }
  else
  {
    // No pulse
    LL_TIM_OC_SetCompareCH4( TIM1, PWM_DARK );
  }
  u8Cnt++;
  if( COLOR_LEVELS <= u8Cnt )
  {
    u8Cnt = 0u;
  }
}


/***************************************< End of file >**************************************/
