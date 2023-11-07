/*! *******************************************************************************************************
* Copyright (c) 2022 Hekk_Elek
*
* \file batterylevel.c
*
* \brief Battery level indicator subprogram
*
* \author Hekk_Elek
*
**********************************************************************************************************/

/***************************************< Includes >**************************************/
// Own includes
#include "main.h"
#include "types.h"
#include "led.h"
#include "rgbled.h"
#include "util.h"
#include "batterylevel.h"


/***************************************< Definitions >**************************************/


/***************************************< Types >**************************************/


/***************************************< Constants >**************************************/


/***************************************< Global variables >**************************************/


/***************************************< Static function definitions >**************************************/
void Delay( U16 u16DelayMs );


/***************************************< Private functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Waits for the given number of milliseconds
//! \param  u16DelayMs: wait time
//! \return -
//! \global -
//! \note   Should be called from init block
//-----------------------------------------------------------------------------
void Delay( U16 u16DelayMs )
{
  U16 u16DelayEnd = Util_GetTimerMs() + u16DelayMs;
  while( Util_GetTimerMs() < u16DelayEnd );
}


/***************************************< Public functions >**************************************/
//----------------------------------------------------------------------------
//! \brief  Initializes battery level measurement
//! \param  -
//! \return -
//! \global -
//! \note   Should be called from init block
//-----------------------------------------------------------------------------
void BatteryLevel_Init( void )
{
#warning "Port to PY32F002!"
  /*
  ADCTIM = 0x3Fu;      // 32 clock sampling time, 2 clocks channel selection hold time
  ADCCFG = 0x2Fu;      // Right-aligned results registers, slowest conversion
  ADC_CONTR = 0x80u;   // Enable ADC
  ADC_CONTR |= 0x0Fu;  // Select internal 1.19V reference
*/
}

//----------------------------------------------------------------------------
//! \brief  Shows battery level on LEDs as a gauge
//! \param  -
//! \return -
//! \global -
//! \note   Should be called only once! Blocking function! Deinitializes ADC.
//-----------------------------------------------------------------------------
void BatteryLevel_Show( void )
{
  U16 u16MeasuredLevel = 2048u;
  U8  u8ChargeLevel;
  U8  u8Index;
  
  // Startup animation
  // After it, all LED brightness will be set to maximum, to ensure a significant current draw during measurement
  for( u8Index = 0u; u8Index < LEDS_NUM/2u; u8Index++ )
  {
    gau8LEDBrightness[ u8Index ] = 15u;
    gau8LEDBrightness[ LEDS_NUM - u8Index - 1u ] = 15u;
    Delay( 100u );
  }
  gau8RGBLEDs[ 0u ] = 15u;
  Delay( 100u );
#warning "Port to PY32F002!"  
  /*
  // Measure battery voltage
  ADC_CONTR |= 0x40u;  // Start conversion
  _nop_();
  _nop_();
  while( !( ADC_CONTR & 0x20u ) );  // Wait for completion flag
  u16MeasuredLevel = ADC_RES<<8u | ADC_RESL;
  // Disable ADC to save power
  ADC_CONTR = 0x00u;
  // Calculate battery voltage
  // The voltage can be calculated using this formula: BatteryVoltage = 1.19/( u16MeasuredLevel / ADC_MAX_VALUE )
  // So the floating-point implementation would be: f32BatteryVoltage = 1.19f/( (float)u16MeasuredLevel/1024.0f );
  // But since floating point calculations are expensive in terms of program memory(!), here we use fixed-point arithmetic...
  // Charge level formula:
  // As CR2032 batteries quickly drop to 2.8V under load, we assume that 2.8V means full charge
  // And since at 2.0V our LEDs can be barely seen, at 2.0V we assume that our battery is completely depleted
  // As we have 6 + 1 LED levels, we divide this range to 7 levels
  // A floating-point based implementation would be: u8ChargeLevel = round( 7.0f*( f32BatteryVoltage - 2.0f )/0.8f );
  // After simplification, the formula for charge level would be: u8ChargeLevel = round( ( 10662.4f / u16MeasuredLevel ) - 17.5f )
*/
  if( u16MeasuredLevel >= 610u )  // If the voltage is below 2.0V
  {
    u8ChargeLevel = 0u;
  }
  else
  {
    u8ChargeLevel = ( ( 42650u / u16MeasuredLevel ) - 70u )>>2u;
  }
  // Display the charge level on the LEDs
  for( u8Index = 0u; u8Index < LEDS_NUM/2u; u8Index++ )
  {
    if( u8ChargeLevel >= u8Index )
    {
      gau8LEDBrightness[ u8Index ] = 15u;
      gau8LEDBrightness[ LEDS_NUM - u8Index - 1u ] = 15u;
    }
    else
    {
      gau8LEDBrightness[ u8Index ] = 0u;
      gau8LEDBrightness[ LEDS_NUM - u8Index - 1u ] = 0u;
    }
  }
  if( u8ChargeLevel > LEDS_NUM/2u )
  {
    gau8RGBLEDs[ 0u ] = 15u;  // Light up red LED
  }
  else
  {
    gau8RGBLEDs[ 0u ] = 0u;
  }
  // Wait, so the user can read the battery charge level
  Delay( 2000u );
}

/***************************************< End of file >**************************************/
