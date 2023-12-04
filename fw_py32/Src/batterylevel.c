/*! *******************************************************************************************************
* Copyright (c) 2022-2023 Hekk_Elek
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
#include "config.h"
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
  // Configure ADC
  LL_ADC_Reset(ADC1);
  LL_APB1_GRP2_EnableClock( LL_APB1_GRP2_PERIPH_ADC1 );
  LL_ADC_SetCommonPathInternalCh( __LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_VREFINT );
  LL_ADC_SetClock( ADC1, LL_ADC_CLOCK_SYNC_PCLK_DIV2 );
  LL_ADC_SetResolution( ADC1, LL_ADC_RESOLUTION_12B );
  LL_ADC_SetResolution( ADC1, LL_ADC_DATA_ALIGN_RIGHT );
  LL_ADC_SetLowPowerMode( ADC1, LL_ADC_LP_AUTOWAIT );
  LL_ADC_SetSamplingTimeCommonChannels( ADC1, LL_ADC_SAMPLINGTIME_239CYCLES_5 );
  LL_ADC_REG_SetTriggerSource( ADC1, LL_ADC_REG_TRIG_SOFTWARE );
  //LL_ADC_REG_SetTriggerEdge(ADC1, LL_ADC_REG_TRIG_EXT_RISING);
  LL_ADC_REG_SetContinuousMode( ADC1, LL_ADC_REG_CONV_SINGLE );
  LL_ADC_REG_SetOverrun( ADC1, LL_ADC_REG_OVR_DATA_OVERWRITTEN );
  LL_ADC_REG_SetSequencerDiscont( ADC1, LL_ADC_REG_SEQ_DISCONT_DISABLE );
  LL_ADC_REG_SetSequencerChannels( ADC1, LL_ADC_CHANNEL_VREFINT );
  //LL_ADC_SetAnalogWDMonitChannels(ADC1, LL_ADC_AWD_ALL_CHANNELS_REG);
  //LL_ADC_ConfigAnalogWDThresholds(ADC1, __LL_ADC_DIGITAL_SCALE(LL_ADC_RESOLUTION_12B)/2, 0x000);
  LL_ADC_StartCalibration( ADC1 );
  while( ADC1->CR & ADC_CR_ADCAL );  // Wait for calibration to finish
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
  
  // Enable ADC
  LL_ADC_Enable( ADC1 );

  // Startup animation
  // After it, all LED brightness will be set to maximum, to ensure a significant current draw during measurement
#ifdef KARIFA
  for( u8Index = 0u; u8Index < LEDS_NUM/2u; u8Index++ )
  {
    gau8LEDBrightness[ u8Index ] = 15u;
    gau8LEDBrightness[ LEDS_NUM - u8Index - 1u ] = 15u;
    Delay( 100u );
  }
#endif
#ifdef HOEMBER
  for( u8Index = 0u; u8Index < LEDS_NUM/2u; u8Index++ )
  {
    gau8LEDBrightness[ u8Index ] = 15u;
    gau8LEDBrightness[ LEDS_NUM - u8Index - 1u ] = 15u;
    Delay( 100u );
  }
#endif
#ifdef HOPEHELY
  for( u8Index = 0u; u8Index < LEDS_NUM; u8Index++ )
  {
    gau8LEDBrightness[ u8Index ] = 15u;
    Delay( 50u );
  }
#endif
#ifdef MEZI
  for( u8Index = 0u; u8Index < LEDS_NUM/2u; u8Index++ )
  {
    gau8LEDBrightness[ 2u*u8Index    ] = 15u;
    gau8LEDBrightness[ 2u*u8Index+1u ] = 15u;
    Delay( 100u );
  }
#endif
  gau8RGBLEDs[ 0u ] = 15u;
  gau8RGBLEDs[ 1u ] = 15u;
  gau8RGBLEDs[ 2u ] = 15u;
  Delay( 100u );
  // Measure battery voltage
  LL_ADC_REG_StartConversion( ADC1 );  // Start (regular) conversion
  while( LL_ADC_REG_IsConversionOngoing( ADC1 ) );  // Wait for conversion to completed
  u16MeasuredLevel = LL_ADC_REG_ReadConversionData12( ADC1 );
  // Disable ADC to save power
  LL_ADC_Disable( ADC1 );
  LL_APB1_GRP2_DisableClock( LL_APB1_GRP2_PERIPH_ADC1 );
  
  // How to calculate battery voltage?
  // The MCU has 1.2 V internal voltage reference that we have just measured.
  // The voltage can be calculated using this formula: BatteryVoltage = 1.2/( u16MeasuredLevel / ADC_MAX_VALUE )
  // So a floating-point implementation would be: f32BatteryVoltage = 1.2f/( (float)u16MeasuredLevel/4096.0f );
  // But since floating point calculations are expensive in terms of program memory(!), here we use fixed-point arithmetic...
  //
  // Charge level formula:
  // As CR2032 batteries quickly drop to 2.8V under load, we assume that 2.8V means full charge
  // And since at 2.0V our LEDs can be barely seen, at 2.0V we assume that our battery is completely depleted
#ifdef KARIFA
  // We have 6 + 1 LED levels, so we divide this range to 7 levels
  // A floating-point based implementation would be: u8ChargeLevel = round( 7.0f*( f32BatteryVoltage - 2.0f )/0.8f );
  // After simplification, the formula for charge level would be: u8ChargeLevel = round( ( 42649.6f / u16MeasuredLevel ) - 17.5f )
  if( u16MeasuredLevel >= 2457u )  // If the voltage is below 2.0V
  {
    u8ChargeLevel = 0u;
  }
  else
  {
    // 
    u8ChargeLevel = ( ( 170600u / u16MeasuredLevel ) - 70u )>>2u;
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
    gau8RGBLEDs[ 1u ] = 0u;   // green stays dark
    gau8RGBLEDs[ 2u ] = 0u;   // blue stays dark
  }
  else
  {
    gau8RGBLEDs[ 0u ] = 0u;
    gau8RGBLEDs[ 1u ] = 0u;
    gau8RGBLEDs[ 2u ] = 0u;
  }
#endif

#ifdef HOEMBER
  // We have 6 + 1 LED levels, so we divide this range to 7 levels
  // A floating-point based implementation would be: u8ChargeLevel = round( 7.0f*( f32BatteryVoltage - 2.0f )/0.8f );
  // After simplification, the formula for charge level would be: u8ChargeLevel = round( ( 42649.6f / u16MeasuredLevel ) - 17.5f )
  if( u16MeasuredLevel >= 2457u )  // If the voltage is below 2.0V
  {
    u8ChargeLevel = 0u;
  }
  else
  {
    // 
    u8ChargeLevel = ( ( 170600u / u16MeasuredLevel ) - 70u )>>2u;
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
    gau8RGBLEDs[ 1u ] = 15u;  // Light up green LED too
    gau8RGBLEDs[ 2u ] = 0u;   // blue stays dark
  }
  else
  {
    gau8RGBLEDs[ 0u ] = 0u;
    gau8RGBLEDs[ 1u ] = 0u;
    gau8RGBLEDs[ 2u ] = 0u;
  }
#endif

#ifdef HOPEHELY  // HOPEHELY
  // We have 12 + 1 LED levels, so we divide this range to 13 levels
  // A floating-point based implementation would be: u8ChargeLevel = round( 13.0f*( f32BatteryVoltage - 2.0f )/0.8f );
  // After simplification, the formula for charge level would be: u8ChargeLevel = round( ( 79872.0f / u16MeasuredLevel ) - 32.5f )
  if( u16MeasuredLevel >= 2457u )  // If the voltage is below 2.0V
  {
    u8ChargeLevel = 0u;
  }
  else
  {
    u8ChargeLevel = ( ( 159744u / u16MeasuredLevel ) - 65u )>>1u;
  }
  // Display the charge level on the LEDs
  for( u8Index = 0u; u8Index < LEDS_NUM; u8Index++ )
  {
    if( u8ChargeLevel >= u8Index )
    {
      gau8LEDBrightness[ u8Index ] = 15u;
    }
    else
    {
      gau8LEDBrightness[ u8Index ] = 0u;
    }
  }
  if( u8ChargeLevel > LEDS_NUM )
  {
    gau8RGBLEDs[ 0u ] = 15u;  // Light up white LED
    gau8RGBLEDs[ 1u ] = 15u;
    gau8RGBLEDs[ 2u ] = 15u;
  }
  else
  {
    gau8RGBLEDs[ 0u ] = 0u;
    gau8RGBLEDs[ 1u ] = 0u;
    gau8RGBLEDs[ 2u ] = 0u;
  }
#endif

#ifdef MEZI
  // We have 5 + 1 LED levels, so we divide this range to 6 levels
  // A floating-point based implementation would be: u8ChargeLevel = round( 6.0f*( f32BatteryVoltage - 2.0f )/0.8f );
  // After simplification, the formula for charge level would be: u8ChargeLevel = round( ( 36864.0f / u16MeasuredLevel ) - 15.0f )
  if( u16MeasuredLevel >= 2457u )  // If the voltage is below 2.0V
  {
    u8ChargeLevel = 0u;
  }
  else
  {
    u8ChargeLevel = ( 36864u / u16MeasuredLevel ) - 15u;
  }
  // Display the charge level on the LEDs
  for( u8Index = 1u; u8Index < LEDS_NUM/2u; u8Index++ )
  {
    if( u8ChargeLevel >= u8Index-1u )
    {
      gau8LEDBrightness[ 2u*u8Index+0u ] = 15u;
      gau8LEDBrightness[ 2u*u8Index+1u ] = 15u;
    }
    else
    {
      gau8LEDBrightness[ 2u*u8Index+0u ] = 0u;
      gau8LEDBrightness[ 2u*u8Index+1u ] = 0u;
    }
  }
  if( u8ChargeLevel > LEDS_NUM/2u )
  {
    gau8LEDBrightness[ 0u ] = 15u;
    gau8LEDBrightness[ 1u ] = 15u;
  }
  else
  {
    gau8LEDBrightness[ 0u ] = 0u;
    gau8LEDBrightness[ 1u ] = 0u;
  }
#endif

  // Wait, so the user can read the battery charge level
  Delay( 2000u );
}

/***************************************< End of file >**************************************/
