/**
  ******************************************************************************
  * @file    py32f0xx_it.c
  * @author  MCU Application Team
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "py32f0xx_it.h"
#include "types.h"
#include "util.h"
#include "led.h"
#include "rgbled.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*           Cortex-M0+ Processor Interruption and Exception Handlers         */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
}

/******************************************************************************/
/* PY32F0xx Peripheral Interrupt Handlers                                     */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_py32f003xx.s).                   */
/******************************************************************************/
//----------------------------------------------------------------------------
//! \brief  Timer 1 interrupt handler (10 kHz)
//! \param  -
//! \return -
//-----------------------------------------------------------------------------
void TIM1_BRK_UP_TRG_COM_IRQHandler( void )
{
  Util_Interrupt();  // Housekeeping, e.g. ms delay timer
  LED_Interrupt();  // Soft-PWM LED driver
  RGBLED_Interrupt();  // RGB LED driver
  // End of interrupt
  LL_TIM_ClearFlag_UPDATE( TIM1 );
}


/************************ (C) COPYRIGHT Puya *****END OF FILE****/
