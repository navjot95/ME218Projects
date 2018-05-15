/****************************************************************************
Module
    SHIP_RX.c

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
#include "inc/hw_timer.h"
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

#include "SHIP_RX_test.h"
#include "Init_UART.h"

/*----------------------------- Module Defines ----------------------------*/
#define RX_PERIOD   1000   // 1 ms 

/*----------------------------- Module Variables ----------------------------*/
static  uint8_t MyPriority;
static  SHIP_RX_testState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
Function
  InitSHIP_RX_test

Parameters
  uint8_t : the priorty of this service

Returns
  bool, false if error in initialization, true otherwise

Description
  Saves away the priority, sets up the initial transition and does any
  other required initialization for this state machine
Notes

****************************************************************************/

bool InitSHIP_RX_test ( uint8_t Priority )
{
  ES_Event_t ThisEvent;
  
  Init_UART_PIC();

  MyPriority = Priority;
  // First state is waiting for 0x7E
  CurrentState = WaitingForStart;
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
  PostSHIP_RX_test

Parameters
  EF_Event ThisEvent , the event to post to the queue

Returns
  boolean False if the Enqueue operation failed, True otherwise

Description
  Posts an event to this state machine's queue

Notes

****************************************************************************/
bool PostSHIP_RX_test( ES_Event_t ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
Function
  RunSHIP_RX_test

Parameters
  ES_Event : the event to process

Returns
  ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

Description
  add your description here

Notes
  uses nested switch/case to implement the machine.

****************************************************************************/
ES_Event_t RunSHIP_RX_test( ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch ( CurrentState )
  {
    case WaitingForStart :
      if(ThisEvent.EventType == ES_INIT)
      {
        printf("\r\nWaitingForStart");
        //Start the inter byte timer
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        // Initialize DataLength to 0 at the start
        //Go to waiting for MSB of length
        CurrentState = WaitingForByte;
      }              
      break;
      
    case WaitingForByte :
      if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == BYTE_TIMER)
      {
        CurrentState = WaitingForByte;
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        printf("\r\nTIMEOUT - WaitingForByte");
      }   
      else if (ThisEvent.EventType == BYTE_RECEIVED)
      {
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        printf("\n\r%X",ThisEvent.EventParam);
        printf("\n\r");
        CurrentState = WaitingForByte;
      }
      break;
      
  }
  
  return ReturnEvent;
}  

void SHIP_ISR_test(void)
{
  static ES_Event_t ThisEvent;
  
  if (HWREG(UART1_BASE + UART_O_MIS) & UART_MIS_RXMIS)
  {
    HWREG(UART1_BASE + UART_O_ICR) |= UART_ICR_RXIC;
    
    ThisEvent.EventType = BYTE_RECEIVED;
    ThisEvent.EventParam = HWREG(UART1_BASE + UART_O_DR);
    PostSHIP_RX_test(ThisEvent);
  }
}