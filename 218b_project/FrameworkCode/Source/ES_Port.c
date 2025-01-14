/****************************************************************************
 Module
   ES_Port.c

 Revision
   1.0.1

 Description
   This is the sample file to demonstrate adding the hardware specific
   functions to the Events & Services Framework. This sample is also used
   as the file for the port to the Freescale MC9S12C32 processor.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 08/21/17 13:47 jec     added functions to init 2 lines for debugging the framework
                        and functions to set & clear those lines.
 03/13/14 10:30	joa		  Updated files to use with Cortex M4 processor core.
                        Specifically, this was tested on a TI TM4C123G mcu.
 03/05/14 13:20	joa		  Began port for TM4C123G
 08/13/13 12:42 jec     moved the hardware specific aspects of the timer here
 08/06/13 13:17 jec     Began moving the stuff from the V2 framework files
 ***************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/systick.h"
#include "driverlib/gpio.h"
#include "utils/uartstdio.h"

#include "ES_Port.h"
#include "ES_Types.h"
#include "ES_Timers.h"

#define UART_PORT 0
#define UART_BAUD 115200UL
#define SRC_CLK_FREQ 16000000UL
#define CLK_FREQ 40000000UL

// change the base address for the debug lines here
#define DEBUG_PORT GPIO_PORTF_BASE
// bit to use when enabling the port for debugging,
// must match base address from DEBUG_PORT definition above
// BIT0HI = PortA, BIT1HI = PortB, BIT2HI = PortC, BIT3HI = PortD,
// BIT4HI = PortE, BIT5HI = PortF
#define DEBUG_PORT_ENABLE_BIT BIT5HI

// which bits on the port are to be used for debugging
#define DEBUG_PORT_WHICH_BITS (BIT1HI | BIT2HI)

// define wihch bit corresponds to which debugging line
#define DEBUG_LINE_1 BIT1HI
#define DEBUG_LINE_2 BIT2HI

// Needed for debug port access
#define ALL_BITS (0xff << 2)

// TickCount is used to track the number of timer ints that have occurred
// since the last check. It should really never be more than 1, but just to
// be sure, we increment it in the interrupt response rather than simply
// setting a flag. Using this variable and checking approach we remove the
// need to post events from the interrupt response routine. This is necessary
// for compilers like HTC for the midrange PICs which do not produce re-entrant
// code so cannot post directly to the queues from within the interrupt resp.
static volatile uint8_t TickCount;

// Global tick count to monitor number of SysTick Interrupts
// make uint16_t to maintain backwards compatibility and not overly burden
// 8 and 16 bit processors
static volatile uint16_t SysTickCounter = 0;

// This variable is used to store the state of the interrupt mask when
// doing EnterCritical/ExitCritical pairs
uint32_t _PRIMASK_temp;

/****************************************************************************
 Function
     _HW_Timer_Init
 Parameters
     unsigned char Rate set to one of the TMR_RATE_XX values to set the
     Tick rate
 Returns
     None.
 Description
     Initializes the timer module by setting up the SysTick Interrupt with the requested
    rate
 Notes
     modify as required to port to other timer hardware
 Author
     J. Edward Carryer, 02/24/97 14:23
     John Alabi, 03/05/14 13:22
****************************************************************************/
void _HW_Timer_Init(TimerRate_t Rate)
{
  SysTickPeriodSet(Rate); /* Set the SysTick Interrupt Rate */
  SysTickIntEnable();     /* Enable the SysTick Interrupt */
  SysTickEnable();        /* Enable SysTick */
  IntMasterEnable();      /* Make sure interrupts are enabled */
}

/****************************************************************************
 Function
     SysTickIntHandler
 Parameters
     none
 Returns
     None.
 Description
     interrupt response routine for the tick interrupt that will allow the
     framework timers to run.
 Notes
     As currently (8/13/13) implemented this does not actually post events
     but simply sets a flag to indicate that the interrupt has occurred.
     the framework response is handled below in _HW_Process_Pending_Ints
 Author
    John Alabi, 03/05/14 13:50
****************************************************************************/
void SysTickIntHandler(void)
{
  /* Interrupt automatically cleared by hardware */
  ++TickCount;          /* flag that it occurred and needs a response */
  ++SysTickCounter;     // keep the free running time going
#ifdef LED_DEBUG
  BlinkLED();
#endif
}

/****************************************************************************
 Function
    _HW_GetTickCount()
 Parameters
    none
 Returns
    uint16_t   count of number of system ticks that have occurred.
 Description
    wrapper for access to SysTickCounter, needed to move increment of tick
    counter to this module to keep the timer ticking during blocking code
 Notes

 Author
    Ed Carryer, 10/27/14 13:55
****************************************************************************/
uint16_t _HW_GetTickCount(void)
{
  return SysTickCounter;
}

/****************************************************************************
 Function
     _HW_Process_Pending_Ints
 Parameters
     none
 Returns
     always true.
 Description
     processes any pending interrupts (actually the hardware interrupt already
     occurred and simply set a flag to tell this routine to execute the non-
     hardware response)
 Notes
     While this routine technically does not need a return value, we always
     return true so that it can be used in the conditional while() loop in
     ES_Run. This way the test for pending interrupts get processed after every
     run function is called and even when there are no queues with events.
     This routine could be expanded to process any other interrupt sources
     that you would like to use to post events to the framework services.
 Author
     J. Edward Carryer, 08/13/13 13:27
****************************************************************************/
bool _HW_Process_Pending_Ints(void)
{
  while (TickCount > 0)
  {
    /* call the framework tick response to actually run the timers */
    ES_Timer_Tick_Resp();
    TickCount--;
  }
  return true;  // always return true to allow loop test in ES_Run to proceed
}

/****************************************************************************
 Function
     ConsoleInit
 Parameters
     none
 Returns
     none.
 Description
  TBD
 Notes
  TBD
 Author
     John Alabi, 03/05/14 15:07
 ****************************************************************************/
void ConsoleInit(void)
{
  // Enable designated port that will be used for the UART
  SysCtlPeripheralEnable( SYSCTL_PERIPH_GPIOA);

  // Enable the designated UART
  SysCtlPeripheralEnable( SYSCTL_PERIPH_UART0);

  // Configure pins for UART including setup for alternate (UART) functionality
  GPIOPinConfigure( GPIO_PA0_U0RX);
  GPIOPinConfigure( GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  // Select the clock source
  UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

  // Initialize the UART for console I/O
  UARTStdioConfig(UART_PORT, UART_BAUD, SRC_CLK_FREQ);
}

#if defined(rvmdk) || defined(__ARMCC_VERSION)
uint32_t CPUgetPRIMASK_cpsid(void)
{
  register uint32_t r0;
  __asm
  {
    mrs   r0, PRIMASK;      // Store PRIMASK in r0
    cpsid i;                // Disable interrupts
  }
  return r0;
}

void CPUsetPRIMASK(uint32_t newPRIMASK)
{
  __asm
  {
    msr PRIMASK, newPRIMASK         // Store newPRIMASK in PRIMASK
  }
}

#endif

/****************************************************************************
 Function
     _HW_DebugLines_Init
 Parameters
     none
 Returns
     None.
 Description
     Initializes the port lines for framework/application debugging
 Notes

 Author
     J. Edward Carryer, 08/21/17 11:51
****************************************************************************/
void _HW_DebugLines_Init(void)
{
  // enable clock to the debug port
  HWREG(SYSCTL_RCGCGPIO) |= DEBUG_PORT_ENABLE_BIT;
  while ((HWREG(SYSCTL_PRGPIO) & DEBUG_PORT_ENABLE_BIT) != DEBUG_PORT_ENABLE_BIT)
  {
    ; // wait for port to be ready
  }
  // set the debug pins as digital
  HWREG(DEBUG_PORT + GPIO_O_DEN) |= DEBUG_PORT_WHICH_BITS;
  // set pins as outputs
  HWREG(DEBUG_PORT + GPIO_O_DIR) |= DEBUG_PORT_WHICH_BITS;
}

/****************************************************************************
 Function
     _HW_DebugSetLine1
 Parameters
     none
 Returns
     None.
 Description
     Sets debug Line1 to be a 1
 Notes

 Author
     J. Edward Carryer, 08/21/17 13:23
****************************************************************************/
void _HW_DebugSetLine1(void)
{
  HWREG(DEBUG_PORT + (GPIO_O_DATA + ALL_BITS)) |= DEBUG_LINE_1;
}

/****************************************************************************
 Function
     _HW_DebugClearLine1
 Parameters
     none
 Returns
     None.
 Description
     Sets debug Line1 to be a 0
 Notes

 Author
     J. Edward Carryer, 08/21/17 13:23
****************************************************************************/
void _HW_DebugClearLine1(void)
{
  HWREG(DEBUG_PORT + (GPIO_O_DATA + ALL_BITS)) &= ~DEBUG_LINE_1;
}

/****************************************************************************
 Function
     _HW_DebugSetLine2
 Parameters
     none
 Returns
     None.
 Description
     Sets debug Line1 to be a 1
 Notes

 Author
     J. Edward Carryer, 08/21/17 13:25
****************************************************************************/
void _HW_DebugSetLine2(void)
{
  HWREG(DEBUG_PORT + (GPIO_O_DATA + ALL_BITS)) |= DEBUG_LINE_2;
}

/****************************************************************************
 Function
     _HW_DebugClearLine2
 Parameters
     none
 Returns
     None.
 Description
     Sets debug Line1 to be a 0
 Notes

 Author
     J. Edward Carryer, 08/21/17 13:26
****************************************************************************/
void _HW_DebugClearLine2(void)
{
  HWREG(DEBUG_PORT + (GPIO_O_DATA + ALL_BITS)) &= ~DEBUG_LINE_2;
}
