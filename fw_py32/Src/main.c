/*! *******************************************************************************************************
* Copyright (c) 2022-2023 Hekk_Elek
*
* \file main.c
*
* \brief SEM-karifa main program
*
* \author Hekk_Elek
*
**********************************************************************************************************/

/***************************************< Includes >**************************************/
// Standard C libraries
#include <string.h>

// Own includes
#include "main.h"
#include "types.h"
#include "util.h"
#include "led.h"
#include "rgbled.h"
#include "animation.h"
#include "persist.h"
#include "batterylevel.h"


/***************************************< Definitions >**************************************/
#define BUTTON_PIN     LL_GPIO_IsInputPinSet(GPIOB,LL_GPIO_PIN_3)  //!< Button for selecting animation and turning it off and on


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/
//! \brief State machine for button debouncing
static enum
{
  BUTTON_UNPRESSED,  //!< The button is not pressed
  BUTTON_BOUNCING,   //!< The button just got pressed and it's currently bouncing
  BUTTON_PRESSED,    //!< The button got debounced
  BUTTON_LONGPRESS,  //!< The button has been pressed for long
  BUTTON_RELEASING   //!< The button just got released and it's currently bouncing
} geButtonState;

static U16 gu16ButtonPressTimer;  //!< Timer for the button debouncing state machine


/***************************************< Static function definitions >**************************************/
static void APP_SystemClockConfig( void );
static void PowerDown( void );


/***************************************< Private functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Initialize 24 MHz HSI system clock
//! \param  -
//! \return -
//-----------------------------------------------------------------------------
static void APP_SystemClockConfig( void )
{
  // Initialize HSI clock
  LL_RCC_HSI_Enable();
  LL_RCC_HSI_SetCalibFreq( LL_RCC_HSICALIBRATION_24MHz );
  while( !LL_RCC_HSI_IsReady() );

  // Set AHB prescaler
  LL_RCC_SetAHBPrescaler( LL_RCC_SYSCLK_DIV_1 );

  // Set system clock source
  LL_RCC_SetSysClkSource( LL_RCC_SYS_CLKSOURCE_HSISYS );
  while( LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS );

  // Set APB1 prescaler
  LL_RCC_SetAPB1Prescaler( LL_RCC_APB1_DIV_1 );
  
  // Store current system clock
  LL_SetSystemCoreClock( 24000000u );
}

//----------------------------------------------------------------------------
//! \brief  Enter deep sleep mode with peripherials set to low-current mode
//! \param  -
//! \return -
//-----------------------------------------------------------------------------
static void PowerDown( void )
{
  // Gradually disable stuff and enter deep sleep
  LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_PWR );
  DISABLE_IT;
  NVIC_DisableIRQ( TIM1_BRK_UP_TRG_COM_IRQn );
  LL_TIM_DisableCounter( TIM1 );
  LL_TIM_DisableAllOutputs( TIM1 );
  LL_APB1_GRP2_DisableClock( LL_APB1_GRP2_PERIPH_TIM1 );
  LL_GPIO_DeInit( GPIOA );
  LL_GPIO_DeInit( GPIOB );
  LL_GPIO_DeInit( GPIOF );
//  LL_GPIO_SetPinPull( GPIOA, LL_GPIO_PIN_ALL, LL_GPIO_PULL_UP );
//  LL_GPIO_SetPinPull( GPIOB, LL_GPIO_PIN_ALL, LL_GPIO_PULL_UP );
//  LL_GPIO_SetPinPull( GPIOF, LL_GPIO_PIN_ALL, LL_GPIO_PULL_UP );
  LL_EXTI_SetEXTISource( LL_EXTI_CONFIG_PORTB, LL_EXTI_LINE_3 );  // PB3
  LL_EXTI_EnableFallingTrig( LL_EXTI_LINE_3 );                    // Falling edge
  LL_EXTI_EnableEvent( LL_EXTI_LINE_3 );                          // Wakes CPU
  LL_EXTI_EnableIT( LL_EXTI_LINE_3 );                             // Generates interrupt
  
  LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOA );
  LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOB );
  LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOF );
  LL_PWR_EnableLowPowerRunMode();
  LL_PWR_SetRegulVoltageScaling( LL_PWR_REGU_VOLTAGE_SCALE2 );
  LL_PWR_SetSramRetentionVolt( LL_PWR_SRAM_RETENTION_VOLT_0p9 );
  LL_PWR_SetWakeUpFlashDelay( LL_PWR_WAKEUP_FLASH_DELAY_0US );
  LL_PWR_SetWakeUpLPToVRReadyTime( LL_PWR_WAKEUP_LP_TO_VR_READY_5US );
  LL_LPM_DisableEventOnPend();
  LL_LPM_EnableDeepSleep();
  SET_BIT( SCB->SCR, SCB_SCR_SLEEPDEEP_Msk );
  __WFI();
  NVIC_SystemReset();  // This should not be reached...
}


/***************************************< Public functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Main program entry point
//! \param  -
//! \return -
//-----------------------------------------------------------------------------
void main( void )
{
  U32  u32UptimeCounter = 0u;
  U16  u16LastCall = 0u;
  U8   u8CurrentAnimation = 0u;
  BOOL bPressedLong = FALSE;

  // Initialize system clock
  APP_SystemClockConfig();
  
  // Initialize modules
  Util_Init();
  LED_Init();
  RGBLED_Init();
  Animation_Init();
  Persist_Init();
  BatteryLevel_Init();

  // Pushbutton @ PB3 --> input with pullup
  LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOB );
  LL_GPIO_SetPinMode( GPIOB, LL_GPIO_PIN_3, LL_GPIO_MODE_INPUT );
  LL_GPIO_SetPinPull( GPIOB, LL_GPIO_PIN_3, LL_GPIO_PULL_UP );
  
  // Init global variables in this module
  geButtonState = BUTTON_UNPRESSED;
  gu16ButtonPressTimer = 0u;
  u8CurrentAnimation = gsPersistentData.u8AnimationIndex;
  
  // Start TIM1 update interrupts
  LL_TIM_EnableIT_UPDATE( TIM1 );
  NVIC_EnableIRQ( TIM1_BRK_UP_TRG_COM_IRQn );

  // Wait if the button is pressed on power up
  // This is necessary, to avoid changing animation on power on
  while( 0 == BUTTON_PIN )
  {
    gu16ButtonPressTimer = Util_GetTimerMs() + 100u;  // 100 ms wait
    while( gu16ButtonPressTimer > Util_GetTimerMs() );
  }

  // Measure and show battery level
  BatteryLevel_Show();
    
  // Main loop
  while( TRUE )
  {
    // Increment uptime counter
    if( Util_GetTimerMs() < u16LastCall )
    {
      u32UptimeCounter += (U32)( 65535u - u16LastCall + Util_GetTimerMs() + 1u );
    }
    else
    {
      u32UptimeCounter += (U32)( Util_GetTimerMs() - u16LastCall );
    }
    u16LastCall = Util_GetTimerMs();
    if( u32UptimeCounter >= 18000000u )  // turn off after 5 hours = 5*60*60*1000 msec
    {
      // Go to power-down sleep
      PowerDown();
    }
    
    // Debounce button in a nonblocking way
    switch( geButtonState )
    {
      case BUTTON_BOUNCING:   // The button just got pressed and it's currently bouncing
        if( Util_GetTimerMs() == gu16ButtonPressTimer )  // the debounce timer has just went off
        {
          if( 0 == BUTTON_PIN )  // if the button is still pressed
          {
            gu16ButtonPressTimer = Util_GetTimerMs() + 2000u;  // 2 sec long press
            geButtonState = BUTTON_PRESSED;
          }
          else  // not pressed anymore
          {
            geButtonState = BUTTON_UNPRESSED;
          }
        }
        break;
      
      case BUTTON_PRESSED:    // The button got debounced
        if( 1 == BUTTON_PIN )  // just got released
        {
          gu16ButtonPressTimer = Util_GetTimerMs() + 50u;  // 50 ms debounce time
          geButtonState = BUTTON_RELEASING;
          // Actions for short button press
          u8CurrentAnimation++;
          if( u8CurrentAnimation >= NUM_ANIMATIONS-1u )
          {
            u8CurrentAnimation = 0u;
          }
          Animation_Set( u8CurrentAnimation );
          // Save it
          Persist_Save();
        }
        else if( Util_GetTimerMs() == gu16ButtonPressTimer )  // the long press timer has just went off
        {
          geButtonState = BUTTON_LONGPRESS;
          // Actions for long button press
          // Signal that it will be shut down by setting a completely black animation
          u8CurrentAnimation = NUM_ANIMATIONS-1u;
          Animation_Set( u8CurrentAnimation );
          bPressedLong = TRUE;
        }
        break;
      
      case BUTTON_LONGPRESS:  // The button has been pressed for long
        if( 1 == BUTTON_PIN )  // just got released
        {
          gu16ButtonPressTimer = Util_GetTimerMs() + 50u;  // 50 ms debounce time
          geButtonState = BUTTON_RELEASING;
        }
        break;
      
      case BUTTON_RELEASING:  // The button just got released and it's currently bouncing
        if( Util_GetTimerMs() == gu16ButtonPressTimer )  // the debounce timer has just went off
        {
          if( 1 == BUTTON_PIN )  // if the button is released
          {
            gu16ButtonPressTimer = Util_GetTimerMs() + 2000u;  // 2 sec long press
            geButtonState = BUTTON_UNPRESSED;
            
            if( TRUE == bPressedLong )
            {
              PowerDown();
              bPressedLong = FALSE;
            }
          }
          else  // still pushed
          {
            gu16ButtonPressTimer = Util_GetTimerMs() + 50u;  // 50 ms debounce time
          }
        }
        break;
      
      default:  // BUTTON_UNPRESSED -- The button is not pressed
        if( 0 == BUTTON_PIN )  // if the button has just got pressed
        {
          gu16ButtonPressTimer = Util_GetTimerMs() + 50u;  // 50 ms debounce time
          geButtonState = BUTTON_BOUNCING;
        }
        break;
    }
    Animation_Cycle();
    // Sleep until next interrupt
    __WFI();  // Wait for interrupt instruction
  }
}


/***************************************< End of file >**************************************/
