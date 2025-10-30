/*! *******************************************************************************************************
* Copyright (c) 2021-2025 Hekk_Elek
*
* \file led.c
*
* \brief Soft-PWM LED driver
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
#ifdef LED_TRADITIONAL_DRIVER
#ifdef LED_SWITCHING_DRIVER
  #error "Switching LED driver cannot be used with traditional LED driver!"
#endif


/***************************************< Definitions >**************************************/
#define PWM_LEVELS      (16u)  //!< PWM levels implemented: [0; PWM_LEVELS)


/***************************************< Types >**************************************/
//! \brief GPIO pin reference type
typedef struct
{
  GPIO_TypeDef* psPort;         //!< Port descriptor pointer (e.g. GPIOA)
  U32           u32Pin;         //!< Pin (e.g. LL_GPIO_PIN_0)
} S_PIN;

//! \brief LED descriptor type
typedef struct
{
  S_PIN         sPin;           //!< GPIO pin of the LED
  U8            u8Multiplexer;  //!< Multiplexer of the LED (1 or 2)
} S_LED_DESCRIPTOR;


/***************************************< Constants >**************************************/
//! \brief Multiplexer pins
static const S_PIN gcasMuxPins[ 2u ] =
{
  { GPIOA, LL_GPIO_PIN_6 },  // MPX1 pin (~right side)
  { GPIOA, LL_GPIO_PIN_5 }   // MPX2 pin (~left side)
};

//! \brief LED-pins assignments
//! \note  The index of the record is used for the animations too
static const S_LED_DESCRIPTOR gcasLEDs[ LEDS_NUM ] =
{
#ifdef KARIFA
  { { GPIOB, LL_GPIO_PIN_1 }, 2u },  // D12
  { { GPIOA, LL_GPIO_PIN_4 }, 2u },  // D4
  { { GPIOA, LL_GPIO_PIN_7 }, 2u },  // D6
  { { GPIOB, LL_GPIO_PIN_2 }, 2u },  // D10
  { { GPIOB, LL_GPIO_PIN_0 }, 2u },  // D8
  { { GPIOA, LL_GPIO_PIN_2 }, 2u },  // D2
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D3
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D9
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D7
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D5
  { { GPIOB, LL_GPIO_PIN_1 }, 1u }   // D13
#endif
#ifdef HOEMBER
  { { GPIOB, LL_GPIO_PIN_1 }, 2u },  // D12
  { { GPIOB, LL_GPIO_PIN_2 }, 2u },  // D10
  { { GPIOA, LL_GPIO_PIN_2 }, 2u },  // D2
  { { GPIOA, LL_GPIO_PIN_4 }, 2u },  // D4
  { { GPIOA, LL_GPIO_PIN_7 }, 2u },  // D6
  { { GPIOB, LL_GPIO_PIN_0 }, 2u },  // D8
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D9
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D7
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D5
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D3
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOB, LL_GPIO_PIN_1 }, 1u }   // D13
#endif
#ifdef HOPEHELY
  { { GPIOB, LL_GPIO_PIN_1 }, 2u },  // D12
  { { GPIOB, LL_GPIO_PIN_2 }, 2u },  // D10
  { { GPIOB, LL_GPIO_PIN_0 }, 2u },  // D8
  { { GPIOA, LL_GPIO_PIN_7 }, 2u },  // D6
  { { GPIOA, LL_GPIO_PIN_2 }, 2u },  // D2
  { { GPIOA, LL_GPIO_PIN_4 }, 2u },  // D4
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D5
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D3
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D7
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D9
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOB, LL_GPIO_PIN_1 }, 1u }   // D13
#endif
#ifdef MEZI
  { { GPIOA, LL_GPIO_PIN_4 }, 2u },  // D4
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D5
  { { GPIOB, LL_GPIO_PIN_2 }, 2u },  // D10
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D9
  { { GPIOB, LL_GPIO_PIN_0 }, 2u },  // D8
  { { GPIOB, LL_GPIO_PIN_1 }, 2u },  // D12
  { { GPIOB, LL_GPIO_PIN_1 }, 1u },  // D13
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D3
  { { GPIOA, LL_GPIO_PIN_2 }, 2u },  // D2
  { { GPIOA, LL_GPIO_PIN_7 }, 2u },  // D6
  { { GPIOA, LL_GPIO_PIN_7 }, 1u }   // D7
#endif
#ifdef AJANDEKCSOMAG
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D5
  { { GPIOA, LL_GPIO_PIN_4 }, 2u },  // D4
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D3
  { { GPIOA, LL_GPIO_PIN_2 }, 2u },  // D2
  { { GPIOA, LL_GPIO_PIN_7 }, 2u },  // D6
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D7
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D9
  { { GPIOB, LL_GPIO_PIN_0 }, 2u },  // D8
  { { GPIOB, LL_GPIO_PIN_2 }, 2u },  // D10
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOB, LL_GPIO_PIN_1 }, 2u },  // D12
  { { GPIOB, LL_GPIO_PIN_1 }, 1u }   // D13
#endif
#ifdef RUDOLF
  { { GPIOB, LL_GPIO_PIN_1 }, 1u },  // D13
  { { GPIOB, LL_GPIO_PIN_2 }, 1u },  // D11
  { { GPIOB, LL_GPIO_PIN_0 }, 1u },  // D9
  { { GPIOA, LL_GPIO_PIN_7 }, 1u },  // D7
  { { GPIOA, LL_GPIO_PIN_4 }, 1u },  // D5
  { { GPIOA, LL_GPIO_PIN_2 }, 1u },  // D3
  { { GPIOA, LL_GPIO_PIN_2 }, 2u },  // D2
  { { GPIOA, LL_GPIO_PIN_4 }, 2u },  // D4
  { { GPIOA, LL_GPIO_PIN_7 }, 2u },  // D6
  { { GPIOB, LL_GPIO_PIN_0 }, 2u },  // D8
  { { GPIOB, LL_GPIO_PIN_2 }, 2u },  // D10
  { { GPIOB, LL_GPIO_PIN_1 }, 2u },  // D12
#endif
};


/***************************************< Global variables >**************************************/
         U8 gau8LEDBrightness[ LEDS_NUM ];  //!< Array for storing individual brightness levels
static   U8 gu8PWMCounter;                  //!< Counter for the base of soft-PWM
static  BIT gbitSide;                       //!< Stores which side of the panel is active


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
  LL_GPIO_InitTypeDef sGPIOInit = {0};
  LL_TIM_InitTypeDef  sTIM1CountInit = {0};
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
  LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_TIM1 );
  
  // Initialize GPIO pins
  /* Default output states */
  LL_GPIO_WriteOutputPort( GPIOA, 0u );
  LL_GPIO_WriteOutputPort( GPIOB, 0u );
  
  /* GPIOA */
  sGPIOInit.Pin        = LL_GPIO_PIN_2 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
  sGPIOInit.Mode       = LL_GPIO_MODE_OUTPUT;
  sGPIOInit.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  sGPIOInit.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  LL_GPIO_Init( GPIOA, &sGPIOInit );

  /* GPIOB */
  sGPIOInit.Pin        = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2;
  sGPIOInit.Mode       = LL_GPIO_MODE_OUTPUT;
  sGPIOInit.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  sGPIOInit.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  LL_GPIO_Init( GPIOB, &sGPIOInit );
  
  // Initialize TIM1 base -- we will use this as the timebase for software timers
  sTIM1CountInit.ClockDivision       = LL_TIM_CLOCKDIVISION_DIV1;
  sTIM1CountInit.CounterMode         = LL_TIM_COUNTERMODE_UP;
  sTIM1CountInit.Prescaler           = 1;
  sTIM1CountInit.Autoreload          = 1200u - 1u;  // Period: 100 usec / 10 kHz @ 24 MHz system clock
  sTIM1CountInit.RepetitionCounter   = 0;
  LL_TIM_Init( TIM1, &sTIM1CountInit );

  // Start counting
  LL_TIM_EnableCounter( TIM1 );  
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
  U8 u8LEDIdx;
  
  // Increment counter and toggle multiplexer lines
  gu8PWMCounter++;
  if( gu8PWMCounter == PWM_LEVELS )
  {
    gu8PWMCounter = 0;
    gbitSide ^= 1;
    // Set multiplexer pins
#ifdef LEDS_REVERSED
    if( !gbitSide )  // if left side
#else  // normally populated LEDs
    if( gbitSide )  // if left side
#endif
    {
      LL_GPIO_SetOutputPin( gcasMuxPins[ 0u ].psPort, gcasMuxPins[ 0u ].u32Pin );    // MPX1
      LL_GPIO_ResetOutputPin( gcasMuxPins[ 1u ].psPort, gcasMuxPins[ 1u ].u32Pin );  // MPX2
    }
    else  // right side
    {
      LL_GPIO_ResetOutputPin( gcasMuxPins[ 0u ].psPort, gcasMuxPins[ 0u ].u32Pin );  // MPX1
      LL_GPIO_SetOutputPin( gcasMuxPins[ 1u ].psPort, gcasMuxPins[ 1u ].u32Pin );    // MPX2
    }
  }
  
  // Iterate through all the LEDs and set them according to the PWM duty cycles
  for( u8LEDIdx = 0u; u8LEDIdx < LEDS_NUM; u8LEDIdx++ )
  {
    // If the LED is on the current side
    if( ( 1u + gbitSide ) == gcasLEDs[ u8LEDIdx ].u8Multiplexer )
    {
#ifdef LEDS_REVERSED
      // Set or reset pin according to PWM duty cycle
      if( gau8LEDBrightness[ u8LEDIdx ] > gu8PWMCounter )
      {
        LL_GPIO_ResetOutputPin( gcasLEDs[ u8LEDIdx ].sPin.psPort, gcasLEDs[ u8LEDIdx ].sPin.u32Pin );
      }
      else
      {
        LL_GPIO_SetOutputPin( gcasLEDs[ u8LEDIdx ].sPin.psPort, gcasLEDs[ u8LEDIdx ].sPin.u32Pin );
      }
#else
      // Set or reset pin according to PWM duty cycle
      if( gau8LEDBrightness[ u8LEDIdx ] > gu8PWMCounter )
      {
        LL_GPIO_SetOutputPin( gcasLEDs[ u8LEDIdx ].sPin.psPort, gcasLEDs[ u8LEDIdx ].sPin.u32Pin );
      }
      else
      {
        LL_GPIO_ResetOutputPin( gcasLEDs[ u8LEDIdx ].sPin.psPort, gcasLEDs[ u8LEDIdx ].sPin.u32Pin );
      }
#endif
    }
  }
}


#endif  // LED_TRADITIONAL_DRIVER
/***************************************< End of file >**************************************/
