/*! *******************************************************************************************************
* Copyright (c) 2022-2025 Hekk_Elek
*
* \file boost.c
*
* \brief Boost converter using timer and comparator peripherials
*
* \author Hekk_Elek
*
**********************************************************************************************************/

/***************************************< Includes >**************************************/
#include "main.h"

// Own includes
#include "types.h"
#include "boost.h"


/***************************************< Definitions >**************************************/


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/


/***************************************< Static function definitions >**************************************/


/***************************************< Private functions >**************************************/


/***************************************< Public functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Initialize hardware and software layer
//! \param  -
//! \return -
//! \global -
//-----------------------------------------------------------------------------
void Boost_Init( void )
{
  LL_GPIO_InitTypeDef TIM1CH1MapInit= {0};
  LL_TIM_OC_InitTypeDef TIM_OC_Initstruct ={0};
  LL_TIM_InitTypeDef TIM1CountInit = {0};
  LL_TIM_BDTR_InitTypeDef TIM_BDTRInitStruct = {0};
  
  // Enable clocks
  LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_TIM1 );
  LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_SYSCFG );
  LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_COMP1 );
  LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOA );
  
  // Initialize GPIO pins
  /* Initialize PA0 as TIM1_CH1N */
  TIM1CH1MapInit.Pin        = LL_GPIO_PIN_0;
  TIM1CH1MapInit.Mode       = LL_GPIO_MODE_ALTERNATE;
  TIM1CH1MapInit.Alternate  = LL_GPIO_AF_14;
  LL_GPIO_Init( GPIOA, &TIM1CH1MapInit );
  /* Initialize PA3 as TIM1_CH1 */
  TIM1CH1MapInit.Pin        = LL_GPIO_PIN_3;
  TIM1CH1MapInit.Mode       = LL_GPIO_MODE_ALTERNATE;
  TIM1CH1MapInit.Alternate  = LL_GPIO_AF_13;
  LL_GPIO_Init( GPIOA, &TIM1CH1MapInit );

  // Configure PWM channels
  TIM_OC_Initstruct.OCMode        = LL_TIM_OCMODE_PWM1;
  TIM_OC_Initstruct.OCState       = LL_TIM_OCSTATE_ENABLE;
  TIM_OC_Initstruct.OCPolarity    = LL_TIM_OCPOLARITY_HIGH;
  TIM_OC_Initstruct.OCIdleState   = LL_TIM_OCIDLESTATE_LOW;
  TIM_OC_Initstruct.OCNState      = LL_TIM_OCSTATE_ENABLE;
  TIM_OC_Initstruct.OCNPolarity   = LL_TIM_OCPOLARITY_LOW;
  TIM_OC_Initstruct.OCNIdleState  = LL_TIM_OCIDLESTATE_HIGH;
  // Set CH1
  TIM_OC_Initstruct.CompareValue  = (U32)(0.43*120u);
  LL_TIM_OC_Init( TIM1, LL_TIM_CHANNEL_CH1, &TIM_OC_Initstruct );
  
  // Initialize TIM1 base
  TIM1CountInit.ClockDivision       = LL_TIM_CLOCKDIVISION_DIV1;
  TIM1CountInit.CounterMode         = LL_TIM_COUNTERMODE_UP;
  TIM1CountInit.Prescaler           = 1;
  TIM1CountInit.Autoreload          = 120u - 1u;  // Period: 10 usec / 100 kHz @ 24 MHz system clock
  TIM1CountInit.RepetitionCounter   = 0;
  LL_TIM_Init( TIM1, &TIM1CountInit );

  // Configure break and dead time
  LL_TIM_BDTR_StructInit( &TIM_BDTRInitStruct );
  TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_ENABLE;                   // 
  TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_ENABLE;                   //
  TIM_BDTRInitStruct.DeadTime = __LL_TIM_CALC_DEADTIME( RCC_GetSystemClockFreq(), LL_TIM_GetClockDivision( TIM1 ), 500u );  // Dead time: 500 nsec
  TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_ENABLE;                 // enable break function
  TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;       // break is active high
  TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_ENABLE;  // resume automatically after break
  LL_TIM_BDTR_Init( TIM1, &TIM_BDTRInitStruct );
  
  // Configure comparator input pin
  /* Initialize PA1 as COMP1_INP */
  TIM1CH1MapInit.Pin        = LL_GPIO_PIN_1;
  TIM1CH1MapInit.Mode       = LL_GPIO_MODE_ANALOG;
  TIM1CH1MapInit.Pull       = LL_GPIO_PULL_DOWN;
  TIM1CH1MapInit.Alternate  = LL_GPIO_AF_0;
  LL_GPIO_Init( GPIOA, &TIM1CH1MapInit );
  
  // Connect COMP1 output to TIM1 break input
  LL_SYSCFG_EnableTIMBreakInputs( LL_SYSCFG_TIMBREAK_COMP1_TO_TIM1 );
  
  // Configure comparator
  LL_COMP_InitTypeDef COMP_InitStruct;
  LL_COMP_StructInit( &COMP_InitStruct );
  COMP_InitStruct.PowerMode            = LL_COMP_POWERMODE_HIGHSPEED;    // probably overkill, but who knows?
  COMP_InitStruct.InputPlus            = LL_COMP_INPUT_PLUS_IO3;         // PA1
  COMP_InitStruct.InputMinus           = LL_COMP_INPUT_MINUS_VREFINT;    // 1.2 V for feedback
  COMP_InitStruct.InputHysteresis      = LL_COMP_HYSTERESIS_DISABLE;     // no histeresis
  COMP_InitStruct.OutputPolarity       = LL_COMP_OUTPUTPOL_NONINVERTED;  // break if voltage too high
  COMP_InitStruct.DigitalFilter        = 0;                              // digital filter is disabled
  LL_COMP_Init( COMP1, &COMP_InitStruct); 
  LL_COMP_Enable( COMP1 );
  
  // Enable output drive
  LL_TIM_EnableAllOutputs( TIM1 );

  // Start counting
  LL_TIM_EnableCounter( TIM1 );
  
  // Start TIM1 update interrupts
  LL_TIM_EnableIT_UPDATE( TIM1 );
//  NVIC_EnableIRQ( TIM1_BRK_UP_TRG_COM_IRQn );
  
  #warning "TODO: make it more portable"
}

//----------------------------------------------------------------------------
//! \brief  Boost converter interrupt routine
//! \param  -
//! \return -
//! \global -
//! \note   Should be called from periodic timer interrupt routine.
//-----------------------------------------------------------------------------
void Boost_Interrupt( void )
{
  // TODO: add PWM duty cycle control
}


/***************************************< End of file >**************************************/
