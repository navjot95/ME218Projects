/****************************************************************************
 Module
     ES_Port.h
 Description
     header file to collect all of the hardware/platform dependent info for
     a particular port of the ES framework
 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 08/13/13 12:12 jec      moved the timer rate constants from ES_Timers.h here
 08/05/13 14:24 jec      new version replacing port.h and adding define to
                         capture the C99 compliant behavior of the compiler
 03/13/13		joa		 Updated files to use with Cortex M4 processor core.
 	 	 	 	 	 	 Specifically, this was tested on a TI TM4C123G mcu.
*****************************************************************************/
#ifndef ES_PORT_H
#define ES_PORT_H


#include <stdio.h>
#include <stdint.h>
#include "termio.h"
#include "bitdefs.h"       /* generic bit defs (BIT0HI, BIT0LO,...) */
#include "Bin_Const.h"     /* macros to specify binary constants in C */
#include "ES_Types.h"

// macro to control the use of C99 data types (or simulations in case you don't
// have a C99 compiler).

// Codewarrior V5, is not C99 so keep this commented out for C32 & E128
#define COMPILER_IS_C99

// The macro 'ES_FLASH' is needed on some compilers to allocate const data to 
// ROM. The macro must be defined, even if it evaluates to nothing.
// for the 'C32 & E128 this is not needed so it evaluates to nothing
#define ES_FLASH 

// the macro 'ES_READ_FLASH_BYTE' is needed on some Harvard machines to generate
// code to read a byte from the program space (Flash)
// The macro must be defined, even if it evaluates to nothing.
// for the 'C32 & E128 we don't need special instructions so it evaluates to a
// simple reference to the variable
#define ES_READ_FLASH_BYTE(_flash_var_)    (_flash_var_)                  

// these macros provide the wrappers for critical regions, where ints will be off
// but the state of the interrupt enable prior to entry will be restored.
// allocation of temp var for saving interrupt enable status should be defined
// in ES_Port.c
extern unsigned char _CCR_temp;

// HCS12 assembly breakdown.
// The Condition Code Register (CCR) contains 5 status indicators, 2 interrupt
// masking bits, and a stop instruction disable bit.  
// EnterCritical(): 1) Push CCR to stack 2) Disable interrupts
// 3) store (CCR) value which is on the stack into the _CCR_temp variable  
// EnterCritical breakdown: 1) move contents of _CCR_temp to stack 2) restore the contents of 
// the CCR register from the stack
//#define EnterCritical()     { __asm pshc; __asm sei; __asm movb 1,SP+,_CCR_temp; } /* This macro saves CCR register and disables global interrupts. */
//#define ExitCritical()  { __asm movb _CCR_temp, 1,-SP ; __asm pulc; } /* This macro restores CCR register saved EnterCritical(). */

// Cortex M-series processors 
// The Interrupt Program Status Register (IPSR) contains the exception type number
// of the current interrupt service routine (ISR)
// Using TivaWare, CPUcpsid() - IntMasterDisable() calls this. Equivalent to __diable_irq()?
extern unsigned int _PRIMASK_temp;
//#pragma FUNC_ALWAYS_INLINE (CPUgetPRIMASK_cpsid);
//#pragma FUNC_ALWAYS_INLINE (CPUsetPRIMASK);
//extern unsigned int _FAULTMASK_temp;

uint32_t CPUgetFAULTMASK_cpsid(void);
void CPUsetFAULTMASK(uint32_t newFAULTMASK);
uint32_t CPUgetPRIMASK_cpsid(void);
void CPUsetPRIMASK(uint32_t newPRIMASK);
static uint32_t __disable_fault_irq(void);
static uint32_t __enable_fault_irq(void);

uint16_t ES_Timer_GetTime(void);

#define EnterCritical()	{ _PRIMASK_temp = CPUgetPRIMASK_cpsid(); }
#define ExitCritical() { CPUsetPRIMASK(_PRIMASK_temp); }


/* Rate constants for programming the SysTick Period to generate tick interrupts.
   These assume an 40MHz configuration, they are the values to be used to program
   the SysTick Reload Value (STRELOAD) register. STRELOAD is 24-bits wide and so
   the highest value is 0xFFFFFF (16,777,216) which equates to
   16777216*1000/40000000 = 419.4 mS.
 */
typedef enum {	ES_Timer_RATE_OFF  	=   (0),
				ES_Timer_RATE_100uS = 4000,
				ES_Timer_RATE_500uS = 20000,
				ES_Timer_RATE_1mS	= 40000,
				ES_Timer_RATE_2mS	= 80000,
				ES_Timer_RATE_4mS	= 160000,
				ES_Timer_RATE_5mS	= 200000,
				ES_Timer_RATE_8mS	= 320000,
				ES_Timer_Rate_10mS	= 400000,
				ES_Timer_Rate_16mS	= 640000,
				ES_Timer_Rate_32mS	= 1280000
} TimerRate_t;

// map the generic functions for testing the serial port to actual functions 
// for this platform. If the C compiler does not provide functions to test
// and retrieve serial characters, you should write them in ES_Port.c
#define IsNewKeyReady()  ( kbhit() != 0 )
#define GetNewKey()      getchar()

// prototypes for the hardware specific routines
void _HW_Timer_Init(TimerRate_t Rate);
bool _HW_Process_Pending_Ints( void );
void ConsoleInit(void);

#if defined(ccs)
static uint32_t __disable_fault_irq(void)
{
	// Read faultmask and disable fault handlers and interrupts
    __asm("    mrs     r0, faultmask;	Store FAULTMASK in r0\n"
    	    "    cpsid   f			;	Disable interrupts\n"
          "    bx      lr			;	Return from function\n");

    /* Used to satisfy compiler. Actual return in r0 */
	return 0;
}

static uint32_t __enable_fault_irq(void)
{
	// Read faultmask and enable fault handlers and interrupts
    __asm("    mrs     r0, faultmask;	Store FAULTMASK in r0\n"
    	    "    cpsie   f			;	Enable interrupts\n"
          "    bx      lr			;	Return from function\n");

    /* Used to satisfy compiler. Actual return in r0 */
	return 0;

}
#endif

#if 0
//#if defined(rvmdk) || defined(__ARMCC_VERSION)
static uint32_t __enable_fault_irq(void)
{
  uint32_t r0;
  __asm
  {
    mrs     r0, FAULTMASK;	// Store FAULTMASK in r0
    cpsie   f;				      // Disable interrupts
  }
  return r0;
}


static uint32_t __disable_fault_irq(void)
{
  uint32_t r0;
  __asm
  {
    mrs     r0, FAULTMASK;	// Store FAULTMASK in r0
    cpsid   f;				      // Disable interrupts
  }
  return r0;
}
#endif

#endif /* ES_PORT_H */
