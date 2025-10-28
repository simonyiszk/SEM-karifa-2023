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
#define PWM_BRIGHT_3       (2*72u)  //!< PWM duty cycle for bright color -- 3rd channel
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
#warning "Recomment for real hardware!"
  /*
  { { GPIOA, LL_GPIO_PIN_7 }, 0u },  // D1
  { { GPIOA, LL_GPIO_PIN_2 }, 0u },  // D2
  { { GPIOA, LL_GPIO_PIN_4 }, 0u },  // D3
  { { GPIOB, LL_GPIO_PIN_1 }, 0u },  // D4
  { { GPIOB, LL_GPIO_PIN_2 }, 0u },  // D5
  { { GPIOB, LL_GPIO_PIN_0 }, 0u },  // D6
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D12
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOB, LL_GPIO_PIN_1 }, 1u },  // D10
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D9
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D8
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D7
*/
  { { GPIOA, LL_GPIO_PIN_7 }, 0u },  // D1
  { { GPIOA, LL_GPIO_PIN_2 }, 0u },  // D2
  { { GPIOA, LL_GPIO_PIN_4 }, 0u },  // D3
  { { GPIOA, LL_GPIO_PIN_5 }, 0u },  // D4
  { { GPIOA, LL_GPIO_PIN_6 }, 0u },  // D5
  { { GPIOA, LL_GPIO_PIN_12}, 0u },  // D6
  { { GPIOA, LL_GPIO_PIN_12}, 1u },  // D12
  { { GPIOA, LL_GPIO_PIN_6 }, 1u },  // D11
  { { GPIOA, LL_GPIO_PIN_5 }, 1u },  // D10
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D9
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D8
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D7
#endif
#ifdef MACSKAS
  { { GPIOA, LL_GPIO_PIN_2 }, 0u },  // D1
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D7
  { { GPIOA, LL_GPIO_PIN_2 }, 2u },  // D13
  { { GPIOA, LL_GPIO_PIN_4 }, 0u },  // D2
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D8
  { { GPIOA, LL_GPIO_PIN_4 }, 2u },  // D14
  { { GPIOA, LL_GPIO_PIN_7 }, 0u },  // D3
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D9
  { { GPIOA, LL_GPIO_PIN_7 }, 2u },  // D15
  { { GPIOB, LL_GPIO_PIN_0 }, 0u },  // D4
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D10
  { { GPIOB, LL_GPIO_PIN_0 }, 2u },  // D16
  { { GPIOB, LL_GPIO_PIN_2 }, 0u },  // D5
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOB, LL_GPIO_PIN_2 }, 2u },  // D17
  { { GPIOB, LL_GPIO_PIN_1 }, 0u },  // D6
  { { GPIOB, LL_GPIO_PIN_1 }, 1u },  // D12
  { { GPIOB, LL_GPIO_PIN_1 }, 2u },  // D18
#endif
};

//! \brief Look-up table for PWM duty cycle as the function of number of active LEDs
//! \note  Inductor current increases quadratically over time, while the number of active LEDs increases current linearly.
static const U16 gcau16PWMDutycycle[ PWM_CHANNELS ][ 1u + LEDS_NUM/PWM_CHANNELS ] =
{
  {  // First channel
    PWM_DARK,                        // 0 LED active
    PWM_BRIGHT_1,                    // 1 LED active
    (U16)(PWM_BRIGHT_1*1.4142+0.5),  // 2 LEDs active
    (U16)(PWM_BRIGHT_1*1.7321+0.5),  // 3 LEDs active
    (U16)(PWM_BRIGHT_1*2.0000+0.5),  // 4 LEDs active
    (U16)(PWM_BRIGHT_1*2.2361+0.5),  // 5 LEDs active
    (U16)(PWM_BRIGHT_1*2.4495+0.5)   // 6 LEDs active
  },
  {  // Second channel
    PWM_DARK,                        // 0 LED active
    PWM_BRIGHT_2,                    // 1 LED active
    (U16)(PWM_BRIGHT_2*1.4142+0.5),  // 2 LEDs active
    (U16)(PWM_BRIGHT_2*1.7321+0.5),  // 3 LEDs active
    (U16)(PWM_BRIGHT_2*2.0000+0.5),  // 4 LEDs active
    (U16)(PWM_BRIGHT_2*2.2361+0.5),  // 5 LEDs active
    (U16)(PWM_BRIGHT_2*2.4495+0.5)   // 6 LEDs active
  },
  {  // Third channel
    PWM_DARK,                        // 0 LED active
    PWM_BRIGHT_3,                    // 1 LED active
    (U16)(PWM_BRIGHT_3*1.4142+0.5),  // 2 LEDs active
    (U16)(PWM_BRIGHT_3*1.7321+0.5),  // 3 LEDs active
    (U16)(PWM_BRIGHT_3*2.0000+0.5),  // 4 LEDs active
    (U16)(PWM_BRIGHT_3*2.2361+0.5),  // 5 LEDs active
    (U16)(PWM_BRIGHT_3*2.4495+0.5)   // 6 LEDs active
  },
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
  LL_TIM_OC_InitTypeDef TIM_OC_Initstruct ={0};
  LL_TIM_InitTypeDef TIM1CountInit = {0};
  LL_GPIO_InitTypeDef TIM1CH1MapInit = {0};
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
  TIM1CH1MapInit.Mode       = LL_GPIO_MODE_OUTPUT;
  TIM1CH1MapInit.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  TIM1CH1MapInit.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  TIM1CH1MapInit.Pin        = 0u;
  for( u8Index = 0u; u8Index < LEDS_NUM; u8Index++ )
  {
    if( GPIOA == gcasLEDs[ u8Index ].sPin.psPort )
    {
      TIM1CH1MapInit.Pin |= gcasLEDs[ u8Index ].sPin.u32Pin;
    }
  }
  LL_GPIO_Init( GPIOA, &TIM1CH1MapInit );
  
  /* GPIOB */
  TIM1CH1MapInit.Pin        = 0u;
  for( u8Index = 0u; u8Index < LEDS_NUM; u8Index++ )
  {
    if( GPIOB == gcasLEDs[ u8Index ].sPin.psPort )
    {
      TIM1CH1MapInit.Pin |= gcasLEDs[ u8Index ].sPin.u32Pin;
    }
  }
  LL_GPIO_Init( GPIOB, &TIM1CH1MapInit );

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
  LL_TIM_OC_EnablePreload( TIM1, LL_TIM_CHANNEL_CH1 );
  // Set CH3
  TIM_OC_Initstruct.CompareValue  = PWM_DARK;
  LL_TIM_OC_Init( TIM1, LL_TIM_CHANNEL_CH3, &TIM_OC_Initstruct );
  LL_TIM_OC_EnablePreload( TIM1, LL_TIM_CHANNEL_CH3 );
  // Set CH4
  TIM_OC_Initstruct.CompareValue  = PWM_DARK;
  LL_TIM_OC_Init( TIM1, LL_TIM_CHANNEL_CH4, &TIM_OC_Initstruct );
  LL_TIM_OC_EnablePreload( TIM1, LL_TIM_CHANNEL_CH4 );
  
  // Initialize TIM1 base
  TIM1CountInit.ClockDivision       = LL_TIM_CLOCKDIVISION_DIV1;
  TIM1CountInit.CounterMode         = LL_TIM_COUNTERMODE_UP;
  TIM1CountInit.Prescaler           = 0;
  TIM1CountInit.Autoreload          = 2400u - 1u;  // Period: 100 usec / 10 kHz @ 24 MHz system clock
  TIM1CountInit.RepetitionCounter   = 0;
  LL_TIM_Init( TIM1, &TIM1CountInit );

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
  static U8 u8MultiplexerIdx = 0u;     //!< Stores which PWM channel is active
  static U8 u8BrightnessCounter = 0u;  //!< Counts between 0 and COLOR_LEVELS
  U8 u8NumLEDsActive = 0u;
  U8 u8LEDIdx;
  
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
  for( u8LEDIdx = 0u; u8LEDIdx < LEDS_NUM; u8LEDIdx++ )
  {
    // If the LED is on the current multiplexer channel
    if( gcasLEDs[ u8LEDIdx ].u8Multiplexer == u8MultiplexerIdx )
    {
      // Set or reset pin according to brightness counter
      if( gau8LEDBrightness[ u8LEDIdx ] > u8BrightnessCounter )
      {
        LL_GPIO_SetOutputPin( gcasLEDs[ u8LEDIdx ].sPin.psPort, gcasLEDs[ u8LEDIdx ].sPin.u32Pin );
        // Increase current
        u8NumLEDsActive++;
      }
      else
      {
        LL_GPIO_ResetOutputPin( gcasLEDs[ u8LEDIdx ].sPin.psPort, gcasLEDs[ u8LEDIdx ].sPin.u32Pin );
      }
    }
  }
  
  // Set PWM duty cycles
  if( 0u == u8MultiplexerIdx )
  {
    LL_TIM_OC_SetCompareCH3( TIM1, gcau16PWMDutycycle[ u8MultiplexerIdx ][ u8NumLEDsActive ] );
  }
  else
  {
    LL_TIM_OC_SetCompareCH3( TIM1, PWM_DARK );
  }
  if( 1u == u8MultiplexerIdx )
  {
    LL_TIM_OC_SetCompareCH4( TIM1, gcau16PWMDutycycle[ u8MultiplexerIdx ][ u8NumLEDsActive ] );
  }
  else
  {
    LL_TIM_OC_SetCompareCH4( TIM1, PWM_DARK );
  }
  if( 2u == u8MultiplexerIdx )
  {
    LL_TIM_OC_SetCompareCH1( TIM1, gcau16PWMDutycycle[ u8MultiplexerIdx ][ u8NumLEDsActive ] );
  }
  else
  {
    LL_TIM_OC_SetCompareCH1( TIM1, PWM_DARK );
  }
}


#endif  // LED_SWITCHING_DRIVER
/***************************************< End of file >**************************************/
