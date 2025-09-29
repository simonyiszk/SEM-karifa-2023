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
#define HULLOCSILLAG
//#define ANGYAL

// Uncomment only for defective units
//#define LEDS_REVERSED   //!< The LEDs are populated in reverse


/***************************************< Definitions -- drivers >**************************************/
#if( defined(HULLOCSILLAG) || defined(ANGYAL) )
  #define LED_SWITCHING_DRIVER
#else
  #define LED_TRADITIONAL_DRIVER
#endif

#if( defined(KARIFA) || defined(HOEMBER) || defined(HOPEHELY) || defined(RUDOLF) )
  #define RGB_DRIVER
#endif

#endif /* CONFIG_H */

/***************************************< End of file >**************************************/
