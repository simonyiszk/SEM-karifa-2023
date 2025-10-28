/*! *******************************************************************************************************
* Copyright (c) 2023-2025 Hekk_Elek
*
* \file config.h
*
* \brief Configuration file for universal karifa firmware
*
* \author Hekk_Elek
*
**********************************************************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

/***************************************< Definitions -- hardware type >**************************************/
// Uncomment only one!
//#define KARIFA
//#define HOEMBER
//#define HOPEHELY
//#define MEZI
//#define AJANDEKCSOMAG
//#define RUDOLF
//#define HULLOCSILLAG
//#define ANGYAL
#define MACSKAS

// Uncomment only for defective units
//#define LEDS_REVERSED   //!< The LEDs are populated in reverse


/***************************************< Definitions -- drivers >**************************************/
#if( defined(HULLOCSILLAG) || defined(ANGYAL) || defined(MACSKAS) )
  #define LED_SWITCHING_DRIVER
#else
  #define LED_TRADITIONAL_DRIVER
#endif

#if( defined(KARIFA) || defined(HOEMBER) || defined(HOPEHELY) || defined(RUDOLF) )
  #define RGB_DRIVER
#endif

#if( defined(MACSKAS) )
  #define LEDS_NUM      (18u)  //!< Number of _normal_ LEDs
  #define PWM_CHANNELS  (3u)   //!< Number of PWM channels/multiplexers used
#else
  #define LEDS_NUM      (12u)  //!< Number of _normal_ LEDs
  #define PWM_CHANNELS  (2u)   //!< Number of PWM channels/multiplexers used
#endif


#endif /* CONFIG_H */

/***************************************< End of file >**************************************/
