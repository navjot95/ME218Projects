/****************************************************************************
Module
    SHIP_PIC_TX.c

Revision
    1.0.1

Description
    This is a template file for implementing flat state machines under the
    Gen2 Events and Services Framework.

Notes


****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_uart.h"
#include "inc/hw_nvic.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

// the headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "SHIP_RX.h"
#include "SHIP_TX.h"
#include "SHIP_PIC_TX.h"
#include "SHIP_PIC_RX.h"
#include "Init_UART.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
static SHIP_PIC_TX_State_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
Function
  InitSHIP_PIC_TX

Parameters
  uint8_t : the priorty of this service

Returns
  bool, false if error in initialization, true otherwise

Description
  Saves away the priority, sets up the initial transition and does any
  other required initialization for this state machine
Notes

****************************************************************************/
bool InitSHIP_PIC_TX ( uint8_t Priority )
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // First state is waiting for byte
  CurrentState = WaitingToTX_PIC;
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  // any other initializations

  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {
      return true;
  }else
  {
      return false;
  }
}

/****************************************************************************
Function
	PostSHIP_PIC_TX

Parameters
	EF_Event ThisEvent , the event to post to the queue

Returns
	boolean False if the Enqueue operation failed, True otherwise

Description
	Posts an event to this state machine's queue

Notes

****************************************************************************/
bool PostSHIP_PIC_TX( ES_Event_t ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
Function
  RunSHIP_TX

Parameters
  ES_Event : the event to process

Returns
  ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

Description
  add your description here

Notes
  uses nested switch/case to implement the machine.

****************************************************************************/
ES_Event_t RunSHIP_PIC_TX( ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors  
  
  switch ( CurrentState )
  {
    case WaitingToTX_PIC :
      if (ThisEvent.EventType == BEGIN_TX)
      {
        if (HWREG(UART5_BASE + UART_O_FR) & UART_FR_TXFE) // (Room to TX byte)
        {
          // Send 0xAA to query fuel status
          HWREG(UART5_BASE + UART_O_DR) = 0xAA;
          
          // Disable RX and Enable TX Interrupt
          HWREG(UART5_BASE + UART_O_IM) &= ~UART_IM_RXIM;
          HWREG(UART5_BASE + UART_O_IM) |= UART_IM_TXIM;
          
          CurrentState = SendingTX_PIC;
          
        }
      }
      
    case SendingTX_PIC :
      if (ThisEvent.EventType == BYTE_SENT)
      {
        CurrentState = WaitingToTX_PIC;
      }
      break;
  }
  return ReturnEvent;
}

/****************************************************************************
Function
  SHIP_PIC_ISR

Parameters
  void

Returns
  void

Description
  ISR for PIC communication of SHIP

****************************************************************************/

void SHIP_PIC_ISR(void)
{
  static ES_Event_t ThisEvent;
  
  // PIC TX
  if(HWREG(UART1_BASE + UART_O_MIS) & UART_MIS_TXMIS)
  {
    // Clear interrupt
		HWREG(UART1_BASE + UART_O_ICR) |= UART_ICR_TXIC;
				
    // End of packet, Disable TX and Enable RX
    HWREG(UART1_BASE + UART_O_IM) &= ~UART_IM_TXIM;
    HWREG(UART1_BASE + UART_O_IM) |= UART_IM_RXIM;

    
    ThisEvent.EventType = BYTE_SENT;
    PostSHIP_PIC_TX(ThisEvent);
	}
  
  // PIC RX
  if (HWREG(UART1_BASE + UART_O_MIS) & UART_MIS_RXMIS)
  {
    HWREG(UART1_BASE + UART_O_ICR) |= UART_ICR_RXIC;
    
    ThisEvent.EventType = BYTE_RECEIVED;
    ThisEvent.EventParam = HWREG(UART1_BASE + UART_O_DR);
    PostSHIP_PIC_RX(ThisEvent);
  }
  
}
