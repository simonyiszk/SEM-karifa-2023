/*! *******************************************************************************************************
* Copyright (c) 2025 Hekk_Elek
*
* \file led_switching.c
*
* \brief Switching current source LED driver
*
* \author Hekk_Elek
*
**********************************************************************************************************/

/***************************************< Includes >**************************************/
#include "main.h"
#include "types.h"
#include "config.h"

// Own include
#include "led.h"


/***************************************< Configuration checks >**************************************/
#ifdef LED_SWITCHING_DRIVER  // Everything will be implemented only when the driver is enabled
#ifdef RGB_DRIVER
  #error "Switching LED driver cannot be used with RGB LED driver!"
#endif
#ifdef LED_TRADITIONAL_DRIVER
  #error "Switching LED driver cannot be used with traditional LED driver!"
#endif


/***************************************< Definitions >**************************************/
#define COLOR_LEVELS       (16u)  //!< Number of brightness levels per color
#define PWM_BRIGHT_1       (72u)  //!< PWM duty cycle for bright color -- 1st channel
#define PWM_BRIGHT_2       (72u)  //!< PWM duty cycle for bright color -- 2nd channel
#define PWM_BRIGHT_3      (100u)  //!< PWM duty cycle for bright color -- 3rd channel
#define PWM_DARK            (0u)  //!< PWM duty cycle for darkness


/***************************************< Types >**************************************/
//! \brief GPIO pin reference type
typedef struct
{
  GPIO_TypeDef* const psPort;         //!< Port descriptor pointer (e.g. GPIOA)
  const U32           u32Pin;         //!< Pin (e.g. LL_GPIO_PIN_0)
} S_PIN;

//! \brief LED descriptor type
typedef struct
{
  S_PIN         sPin;           //!< GPIO pin of the LED
  U8            u8Multiplexer;  //!< Multiplexer of the LED (1 or 2)
} S_LED_DESCRIPTOR;


/***************************************< Constants >**************************************/
//! \brief LED-pins assignments
//! \note  The index of the record is used for the animations too
static const S_LED_DESCRIPTOR gcasLEDs[ LEDS_NUM ] =
{
#ifdef HULLOCSILLAG
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOB, LL_GPIO_PIN_1 }, 1u },  // D10
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D9
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D8
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D7
  { { GPIOA, LL_GPIO_PIN_7 }, 0u },  // D1
  { { GPIOA, LL_GPIO_PIN_2 }, 0u },  // D2
  { { GPIOA, LL_GPIO_PIN_4 }, 0u },  // D3
  { { GPIOB, LL_GPIO_PIN_1 }, 0u },  // D4
  { { GPIOB, LL_GPIO_PIN_2 }, 0u },  // D5
  { { GPIOB, LL_GPIO_PIN_0 }, 0u },  // D6
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D12
#endif
#ifdef ANGYAL
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D8
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D9
  { { GPIOB, LL_GPIO_PIN_1 }, 1u },  // D10
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D12
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D7
  { { GPIOB, LL_GPIO_PIN_0 }, 0u },  // D6
  { { GPIOB, LL_GPIO_PIN_2 }, 0u },  // D5
  { { GPIOB, LL_GPIO_PIN_1 }, 0u },  // D4
  { { GPIOA, LL_GPIO_PIN_4 }, 0u },  // D3
  { { GPIOA, LL_GPIO_PIN_2 }, 0u },  // D2
  { { GPIOA, LL_GPIO_PIN_7 }, 0u },  // D1
#endif
#ifdef MACSKAS
  { { GPIOA, LL_GPIO_PIN_4 }, 2u },  // D14
  { { GPIOA, LL_GPIO_PIN_4 }, 0u },  // D2
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D8

  { { GPIOA, LL_GPIO_PIN_2 }, 2u },  // D13
  { { GPIOA, LL_GPIO_PIN_2 }, 0u },  // D1
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D7
  
  { { GPIOB, LL_GPIO_PIN_1 }, 2u },  // D18
  { { GPIOB, LL_GPIO_PIN_1 }, 0u },  // D6
  { { GPIOB, LL_GPIO_PIN_1 }, 1u },  // D12

  { { GPIOB, LL_GPIO_PIN_2 }, 2u },  // D17
  { { GPIOB, LL_GPIO_PIN_2 }, 0u },  // D5
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11

  { { GPIOB, LL_GPIO_PIN_0 }, 2u },  // D16
  { { GPIOB, LL_GPIO_PIN_0 }, 0u },  // D4
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D10
  
  { { GPIOA, LL_GPIO_PIN_7 }, 2u },  // D15
  { { GPIOA, LL_GPIO_PIN_7 }, 0u },  // D3
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D9
#endif
};


/***************************************< Global variables >**************************************/
U8 gau8LEDBrightness[ LEDS_NUM ];  //!< Array for storing individual brightness levels


/***************************************< Static function definitions >**************************************/


/***************************************< Private functions >**************************************/


/***************************************< Public functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Initialize TIM1 and all IO pins associated with LEDs
//! \param  -
//! \return -
//! \global gau8LEDBrightness[], gu8PWMCounter
//! \note   Should be called in the init block
//-----------------------------------------------------------------------------
void LED_Init( void )
{
  LL_TIM_OC_InitTypeDef sTIM_OC_Initstruct ={0};
  LL_TIM_InitTypeDef    sTIM1CountInit = {0};
  LL_GPIO_InitTypeDef   sGPIOInit = {0};
  U8 u8Index;
  
  // Init globals
  for( u8Index = 0; u8Index < LEDS_NUM; u8Index++ )
  {
    gau8LEDBrightness[ u8Index ] = 0;
  }
  
  // Enable clocks
  LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOA );
  LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOB );
  LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_TIM1 );
  
  // Initialize GPIO pins
  /* Default output states */
  LL_GPIO_WriteOutputPort( GPIOA, 0u );
  LL_GPIO_WriteOutputPort( GPIOB, 0u );
  
  /* GPIOA */
  sGPIOInit.Mode       = LL_GPIO_MODE_OUTPUT;
  sGPIOInit.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  sGPIOInit.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  sGPIOInit.Pin        = 0u;
  for( u8Index = 0u; u8Index < LEDS_NUM; u8Index++ )
  {
    if( GPIOA == gcasLEDs[ u8Index ].sPin.psPort )
    {
      sGPIOInit.Pin |= gcasLEDs[ u8Index ].sPin.u32Pin;
    }
  }
  LL_GPIO_Init( GPIOA, &sGPIOInit );
  
  /* GPIOB */
  sGPIOInit.Pin = 0u;
  for( u8Index = 0u; u8Index < LEDS_NUM; u8Index++ )
  {
    if( GPIOB == gcasLEDs[ u8Index ].sPin.psPort )
    {
      sGPIOInit.Pin |= gcasLEDs[ u8Index ].sPin.u32Pin;
    }
  }
  LL_GPIO_Init( GPIOB, &sGPIOInit );

  // Enable clocks
  LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_TIM1 );
  LL_IOP_GRP1_EnableClock( LL_IOP_GRP1_PERIPH_GPIOA );
  
  // Initialize GPIO pins of timer outputs
  /* Initialize PA0/PA1/PA3 as TIM1_CH3/TIM1_CH4/TIM1_CH1, respectively */
  sGPIOInit.Pin        = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_3;
  sGPIOInit.Mode       = LL_GPIO_MODE_ALTERNATE;
  sGPIOInit.Alternate  = LL_GPIO_AF_13;
  LL_GPIO_Init( GPIOA, &sGPIOInit );

  // Configure PWM channels
  sTIM_OC_Initstruct.OCMode        = LL_TIM_OCMODE_PWM1;
  sTIM_OC_Initstruct.OCState       = LL_TIM_OCSTATE_ENABLE;
  sTIM_OC_Initstruct.OCPolarity    = LL_TIM_OCPOLARITY_LOW;
  sTIM_OC_Initstruct.OCIdleState   = LL_TIM_OCIDLESTATE_HIGH;
  // Set CH1
  sTIM_OC_Initstruct.CompareValue  = PWM_DARK;
  LL_TIM_OC_Init( TIM1, LL_TIM_CHANNEL_CH1, &sTIM_OC_Initstruct );
  LL_TIM_OC_EnablePreload( TIM1, LL_TIM_CHANNEL_CH1 );
  // Set CH3
  sTIM_OC_Initstruct.CompareValue  = PWM_DARK;
  LL_TIM_OC_Init( TIM1, LL_TIM_CHANNEL_CH3, &sTIM_OC_Initstruct );
  LL_TIM_OC_EnablePreload( TIM1, LL_TIM_CHANNEL_CH3 );
  // Set CH4
  sTIM_OC_Initstruct.CompareValue  = PWM_DARK;
  LL_TIM_OC_Init( TIM1, LL_TIM_CHANNEL_CH4, &sTIM_OC_Initstruct );
  LL_TIM_OC_EnablePreload( TIM1, LL_TIM_CHANNEL_CH4 );
  
  // Initialize TIM1 base -- we will use this as the timebase for software timers
  sTIM1CountInit.ClockDivision       = LL_TIM_CLOCKDIVISION_DIV1;
  sTIM1CountInit.CounterMode         = LL_TIM_COUNTERMODE_UP;
  sTIM1CountInit.Prescaler           = 0;
  sTIM1CountInit.Autoreload          = 2400u - 1u;  // Period: 100 usec / 10 kHz @ 24 MHz system clock
  sTIM1CountInit.RepetitionCounter   = 0;
  LL_TIM_Init( TIM1, &sTIM1CountInit );

  // Enable output drive
  LL_TIM_EnableAllOutputs( TIM1 );

  // Start counting
  LL_TIM_EnableCounter( TIM1 );
}

//----------------------------------------------------------------------------
//! \brief  Interrupt routine to update PWM duty cycles and set LED selector pins
//! \param  -
//! \return -
//! \global gau8LEDBrightness[]
//! \note   Should be called from periodic timer interrupt routine.
//-----------------------------------------------------------------------------
void LED_Interrupt( void )
{
  static U8  u8MultiplexerIdx = 0u;     //!< Stores which PWM channel is active
  static U8  u8BrightnessCounter = 0u;  //!< Counts between 0 and COLOR_LEVELS
  static U32 u32SetPortA = 0u;
  static U32 u32SetPortB = 0u;
  static U32 u32ResetPortA = 0u;
  static U32 u32ResetPortB = 0u;
  U8 u8NumLEDsActive = 0u;
  U8 u8LEDIdx;
  
  // Set LED pins during the inductor charging stage
  LL_GPIO_SetOutputPin( GPIOA, u32SetPortA );
  LL_GPIO_SetOutputPin( GPIOB, u32SetPortB );
  LL_GPIO_ResetOutputPin( GPIOA, u32ResetPortA );
  LL_GPIO_ResetOutputPin( GPIOB, u32ResetPortB );
  
  // Increment counter and multiplexer index
  u8BrightnessCounter++;
  if( u8BrightnessCounter == COLOR_LEVELS )
  {
    u8BrightnessCounter = 0u;
    u8MultiplexerIdx++;
    if( u8MultiplexerIdx >= PWM_CHANNELS )
    {
      u8MultiplexerIdx = 0u;
    }
  }
  
  // Iterate through all the LEDs and calculate how many LEDs will be active this cycle
  u32SetPortA = 0u; u32ResetPortA = 0u;
  u32SetPortB = 0u; u32ResetPortB = 0u;
  for( u8LEDIdx = 0u; u8LEDIdx < LEDS_NUM; u8LEDIdx++ )
  {
    // If the LED is on the current multiplexer channel
    if( gcasLEDs[ u8LEDIdx ].u8Multiplexer == u8MultiplexerIdx )
    {
      // Set or reset pin according to brightness counter
      if( gau8LEDBrightness[ u8LEDIdx ] > u8BrightnessCounter )
      {
        if( GPIOA == gcasLEDs[ u8LEDIdx ].sPin.psPort )
        {
          u32SetPortA |= gcasLEDs[ u8LEDIdx ].sPin.u32Pin;
        }
        else
        {
          u32SetPortB |= gcasLEDs[ u8LEDIdx ].sPin.u32Pin;
        }
        // Increase current
        u8NumLEDsActive++;
      }
      else
      {
        if( GPIOA == gcasLEDs[ u8LEDIdx ].sPin.psPort )
        {
          u32ResetPortA |= gcasLEDs[ u8LEDIdx ].sPin.u32Pin;
        }
        else
        {
          u32ResetPortB |= gcasLEDs[ u8LEDIdx ].sPin.u32Pin;
        }
      }
    }
  }
  
  // Set PWM duty cycles
  if( 0u == u8MultiplexerIdx )
  {
    LL_TIM_OC_SetCompareCH3( TIM1, u8NumLEDsActive*PWM_BRIGHT_1 );
  }
  else
  {
    LL_TIM_OC_SetCompareCH3( TIM1, PWM_DARK );
  }
  if( 1u == u8MultiplexerIdx )
  {
    LL_TIM_OC_SetCompareCH4( TIM1, u8NumLEDsActive*PWM_BRIGHT_2 );
  }
  else
  {
    LL_TIM_OC_SetCompareCH4( TIM1, PWM_DARK );
  }
  if( 2u == u8MultiplexerIdx )
  {
    LL_TIM_OC_SetCompareCH1( TIM1, u8NumLEDsActive*PWM_BRIGHT_3 );
  }
  else
  {
    LL_TIM_OC_SetCompareCH1( TIM1, PWM_DARK );
  }
}


#endif  // LED_SWITCHING_DRIVER
/***************************************< End of file >**************************************/
