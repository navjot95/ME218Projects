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
#include "SHIP_PIC_RX.h"
#include "SHIP_PIC_TX.h"
#include "Init_UART.h"
#include "MotorModule.h"
/*----------------------------- Module Defines ----------------------------*/
#define FUEL_EMPTY_MASK 0x08
#define FUEL_LEVEL_MASK 0x07

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t  MyPriority;
static SHIP_PIC_RX_State_t CurrentState;

static uint8_t  FuelStatus;
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
Function
  InitSHIP_PIC_RX

Parameters
  uint8_t : the priorty of this service

Returns
  bool, false if error in initialization, true otherwise

Description
  Saves away the priority, sets up the initial transition and does any
  other required initialization for this state machine
Notes

****************************************************************************/
bool InitSHIP_PIC_RX ( uint8_t Priority )
{
  ES_Event_t ThisEvent;
  
  Init_UART_PIC();

  MyPriority = Priority;
  // First state is waiting for 0x7E
  CurrentState = WaitingForData_PIC;
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
  PostSHIP_PIC_RX

Parameters
  EF_Event ThisEvent , the event to post to the queue

Returns
  boolean False if the Enqueue operation failed, True otherwise

Description
  Posts an event to this state machine's queue

Notes

****************************************************************************/
bool PostSHIP_PIC_RX( ES_Event_t ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
Function
  RunSHIP_RX

Parameters
  ES_Event : the event to process

Returns
  ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

Description
  add your description here

Notes
  uses nested switch/case to implement the machine.

****************************************************************************/
ES_Event_t RunSHIP_PIC_RX( ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch ( CurrentState )
  {
    case WaitingForData_PIC :
      if(ThisEvent.EventType == BYTE_RECEIVED)
      {
        if (ThisEvent.EventParam == 0xAA)
        {
          break;
        }
        else
        {
          FuelStatus = ThisEvent.EventParam;
          CurrentState = WaitingForData_PIC;
          
          // Fuel LED Control
          // Turn on if fueled
          if (FuelStatus & FUEL_EMPTY_MASK)
          {
            powerFuelLEDs(true); 
          }
          // Turn off if NOT Fueled
          else
          {
            powerFuelLEDs(false); 
          }
        }
      }     
      break;
           
  }
  return ReturnEvent;
}


bool QueryFuelEmpty(void)
{
  // True if Fueled (stupid, but it's consistent with the spec sheet)
  if (FuelStatus & FUEL_EMPTY_MASK)
  {
    return true;
  }
  // False if NOT Fueled
  else
  {
    return false;
  }

}

uint8_t QueryFuelStatus(void)
{
  return FuelStatus;
}

