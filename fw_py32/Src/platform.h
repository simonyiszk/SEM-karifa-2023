/*! *******************************************************************************************************
* Copyright (c) 2021-2023 Hekk_Elek
*
* \file platform.h
*
* \brief Compiler-specific directives
*
* \author Hekk_Elek
*
**********************************************************************************************************/
#ifndef PLATFORM_H
#define PLATFORM_H

/***************************************< Includes >**************************************/

/***************************************< Definitions >**************************************/
#if defined(__IAR_SYSTEMS_ICC__)
// Include intrinsic functions
#include <intrinsics.h>

// No operation intrinsic macro
#define NOP()    __no_operation()

// Storage classifiers: 
#define DATA
#define IDATA
#define XDATA
#define CODE
#define REENTRANT  

// Bit definition
#define BIT        unsigned char

// Interrupt definition
#define IT_PRE
#define ITVECTOR0  
#define ITVECTOR1  
#define ITVECTOR10  

//NOTE: In IAR 8051 everything is packed by default
#define PACKED 
// Compile-time size assertion
//FIXME: for some reason it doesn't want to throw an error if the expression is false
#define STATIC_ASSERT(expr) typedef char static_assertion[(expr)?1:-1]


/////////////////////////////////////////////////////////////////////////////////////////////
#elif defined (__GNUC__) // ARM GCC

#include <stdint.h>

// No operation intrinsic macro
#define NOP() __asm volatile("nop")

// Storage classifiers
#define DATA
#define IDATA
#define XDATA
#define CODE
#define REENTRANT

// Bit definition
#define BIT uint8_t

// Interrupt definition
#define IT_PRE
#define ITVECTOR0
#define ITVECTOR1
#define ITVECTOR10

// Packed structure
#define PACKED

// Compile-time size assertion
#define STATIC_ASSERT(expr) _Static_assert((expr), #expr)

/////////////////////////////////////////////////////////////////////////////////////////////
#else  // Keil C51
// Include intrinsic functions
#include <intrins.h>

// No operation intrinsic macro
#define NOP()    _nop_()

// Storage classifiers
#define DATA       data
#define IDATA      idata
#define XDATA      xdata
#define CODE       code
#define REENTRANT  reentrant

// Bit definition
#define BIT        bit

// Interrupt definition
#define IT_PRE     
#define ITVECTOR0   interrupt 0
#define ITVECTOR1   interrupt 1
#define ITVECTOR10  interrupt 10

//NOTE: In Keil C51 everything is packed by default
#define PACKED

// Compile-time size assertion
//FIXME: for some reason it doesn't want to throw an error if the expression is false
#define STATIC_ASSERT(expr) typedef char static_assertion[(expr)?1:-1]


#endif
#endif /* PLATFORM_H */

/***************************************< End of file >**************************************/
