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
#include "config.h"
#include "util.h"
#include "led.h"
#include "rgbled.h"
#include "animation.h"
#include "persist.h"
#include "batterylevel.h"


/***************************************< Definitions >**************************************/
#define BUTTON_PIN            LL_GPIO_IsInputPinSet(GPIOB,LL_GPIO_PIN_3)  //!< Button for selecting animation and turning it off and on
#define BUTTON_DEBOUNCE_MS                                         (10u)  //!< Time for button debouncing in ms
#define BUTTON_LONGPRESS_MS                                      (2000u)  //!< Time for long button press in ms
#define BUTTON_FASTCLICKS_MS                                      (200u)  //!< Time between two button pushes so it will be registered as fast clicks


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

static U16  gu16ButtonPressTimer;          //!< Timer for the button debouncing state machine
static U16  gu16ButtonFastClicksTimer;     //!< Timer for the fast clicks detector
static BOOL gbButtonFastClicksTimerValid;  //!< Timer for the fast clicks detector is running or not


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
  //DISABLE_IT;
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
  LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOB );
  LL_GPIO_SetPinMode( GPIOB, LL_GPIO_PIN_3, LL_GPIO_MODE_INPUT );       // PB3 input
  LL_EXTI_SetEXTISource( LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE3 );  // PB3
  LL_EXTI_EnableFallingTrig( LL_EXTI_LINE_3 );                          // Falling edge
  LL_EXTI_EnableEvent( LL_EXTI_LINE_3 );                                // Wakes CPU
  LL_EXTI_EnableIT( LL_EXTI_LINE_3 );                                   // Generates interrupt
  NVIC_SetPriority( EXTI2_3_IRQn, 1 );
  NVIC_EnableIRQ( EXTI2_3_IRQn );
  
  LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOA );
  //LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOB );
  LL_IOP_GRP1_DisableClock( LL_IOP_GRP1_PERIPH_GPIOF );
  LL_PWR_EnableLowPowerRunMode();
  LL_PWR_SetRegulVoltageScaling( LL_PWR_REGU_VOLTAGE_SCALE2 );
  LL_PWR_SetSramRetentionVolt( LL_PWR_SRAM_RETENTION_VOLT_0p9 );
  LL_PWR_SetWakeUpFlashDelay( LL_PWR_WAKEUP_FLASH_DELAY_0US );
  LL_PWR_SetWakeUpLPToVRReadyTime( LL_PWR_WAKEUP_LP_TO_VR_READY_5US );
  //LL_LPM_DisableEventOnPend();
  LL_LPM_EnableDeepSleep();
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
  BOOL bPressedLong = FALSE;

  // Initialize system clock
  APP_SystemClockConfig();
  
  // Initialize modules
  Util_Init();
  LED_Init();
#ifdef RGB_DRIVER
  RGBLED_Init();
#endif
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
  gu16ButtonFastClicksTimer = 0u;
  gbButtonFastClicksTimerValid = FALSE;
  
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
            gu16ButtonPressTimer = Util_GetTimerMs() + BUTTON_LONGPRESS_MS;  // long press
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
          gu16ButtonPressTimer = Util_GetTimerMs() + BUTTON_DEBOUNCE_MS;  // debounce time
          geButtonState = BUTTON_RELEASING;
          // Check if this is a fast click, or not
          if( ( TRUE == gbButtonFastClicksTimerValid )
           && ( Util_GetTimerMs() - gu16ButtonFastClicksTimer < BUTTON_FASTCLICKS_MS ) )
          {
            // Fast click
            Animation_SetLoop( TRUE );
          }
          else
          {
            // Actions for normal short button press
            Animation_SetLoop( FALSE );
            Animation_NextAnimation();
          }
          // Start/restart fast click timer
          gu16ButtonFastClicksTimer = Util_GetTimerMs();
          gbButtonFastClicksTimerValid = TRUE;
          // Save animation state
          Persist_Save();
        }
        else if( Util_GetTimerMs() == gu16ButtonPressTimer )  // the long press timer has just went off
        {
          geButtonState = BUTTON_LONGPRESS;
          // Actions for long button press
          // Signal that it will be shut down by setting a completely black animation
          Animation_SetDarkness();
          bPressedLong = TRUE;
        }
        break;
      
      case BUTTON_LONGPRESS:  // The button has been pressed for long
        if( 1 == BUTTON_PIN )  // just got released
        {
          gu16ButtonPressTimer = Util_GetTimerMs() + BUTTON_DEBOUNCE_MS;  // debounce time
          geButtonState = BUTTON_RELEASING;
        }
        break;
      
      case BUTTON_RELEASING:  // The button just got released and it's currently bouncing
        if( Util_GetTimerMs() == gu16ButtonPressTimer )  // the debounce timer has just went off
        {
          if( 1 == BUTTON_PIN )  // if the button is released
          {
            gu16ButtonPressTimer = Util_GetTimerMs() + BUTTON_LONGPRESS_MS;  // long press
            geButtonState = BUTTON_UNPRESSED;
            
            if( TRUE == bPressedLong )
            {
              PowerDown();
              bPressedLong = FALSE;
            }
          }
          else  // still pushed
          {
            gu16ButtonPressTimer = Util_GetTimerMs() + BUTTON_DEBOUNCE_MS;  // debounce time
          }
        }
        break;
      
      default:  // BUTTON_UNPRESSED -- The button is not pressed
        if( 0 == BUTTON_PIN )  // if the button has just got pressed
        {
          gu16ButtonPressTimer = Util_GetTimerMs() + BUTTON_DEBOUNCE_MS;  // debounce time
          geButtonState = BUTTON_BOUNCING;
        }
        // Check if the fast click detector is running
        if( TRUE == gbButtonFastClicksTimerValid )
        {          
          // If button was pushed too long ago
          if( Util_GetTimerMs() - gu16ButtonFastClicksTimer > 2000u )
          {
            // Disable timer
            gbButtonFastClicksTimerValid = FALSE;
          }
        }
        break;
    }
    Animation_Cycle();
    // Sleep until next interrupt
    __WFI();  // Wait for interrupt instruction
  }
}


/***************************************< End of file >**************************************/
