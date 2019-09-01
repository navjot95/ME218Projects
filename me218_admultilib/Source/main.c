/****************************************************************************
 Module
   main.c

 Revision
   1.0.1

 Description
   test harness for proving out the New 4-channel A/D converter routines

 Notes
  
  
 History
 When           Who     What/Why
 -------------- ---     --------
 08/22/17 18:03 jec     started the cleanup and prep to make this the
                        standard A/D libary for ME218
****************************************************************************/

/*----------------------------- Include Files -----------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"

#include "ES_Port.h"
#include "termio.h"
#include "ADMulti.h"

/*----------------------------- Module Defines ----------------------------*/
#define clrScrn() 	printf("\x1b[2J")

// change the base address for the debug lines here
#define DEBUG_PORT GPIO_PORTF_BASE
// bit to use when enabling the port for debugging, 
// must match base address from DEBUG_PORT definition above
// BIT0HI = PortA, BIT1HI = PortB, BIT2HI = PortC, BIT3HI = PortD, 
// BIT4HI = PortE, BIT5HI = PortF
#define DEBUG_PORT_ENABLE_BIT BIT5HI

// which bits on the port are to be used for debugging
#define DEBUG_PORT_WHICH_BITS (BIT1HI)

// define wihch bit corresponds to which debugging line
#define DEBUG_LINE_1 BIT1HI

// Needed for debug port access 
#define ALL_BITS (0xff<<2)

/*---------------------------- Module Functions ---------------------------*/
void _HW_DebugLines_Init(void);
void _HW_DebugSetLine1(void);
void _HW_DebugClearLine1(void);


/*---------------------------- Module Variables ---------------------------*/
static uint32_t ADResults[4];

/*------------------------------ Module Code ------------------------------*/
int main(void)
{  
  // initialize the timer sub-system and console I/O
  _HW_Timer_Init(ES_Timer_RATE_1mS);
	TERMIO_Init();
	clrScrn();

	// When doing testing, it is useful to announce just which program
	// is running.
	puts("\rStarting ADMulti Test \r");
	printf("%s %s\n",__TIME__, __DATE__);
	printf("\n\r\n");

	// hardware initialization function calls 
  ADC_MultiInit(1);
  _HW_DebugLines_Init();
  
  
  while (1){
    _HW_DebugSetLine1();                  // raise line for timing
    ADC_MultiRead(ADResults);                 // read results
    _HW_DebugClearLine1();                // lower line for timing
    
    printf(" Ch0 = %u, Ch1 = %u, Ch2 = %u, Ch3 = %u \n\r", ADResults[0], ADResults[1],
          ADResults[2], ADResults[3]);
    puts("Press any key to do the next conversion\r");
    getchar();
  }
  
}

/***************************************************************************
 private functions
 ***************************************************************************/

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
    ; // wait for port to be ready
 
  // set the debug pins as digital
  HWREG(DEBUG_PORT+GPIO_O_DEN) |= DEBUG_PORT_WHICH_BITS;
  // set pins as outputs
  HWREG(DEBUG_PORT+GPIO_O_DIR) |= DEBUG_PORT_WHICH_BITS;
  
 
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

  HWREG(DEBUG_PORT+(GPIO_O_DATA + ALL_BITS)) |= DEBUG_LINE_1;

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

  HWREG(DEBUG_PORT+(GPIO_O_DATA + ALL_BITS)) &= ~DEBUG_LINE_1;

}

