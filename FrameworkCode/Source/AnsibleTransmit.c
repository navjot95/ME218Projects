/****************************************************************************
 Module
 AnsibleTransmit.c

 Revision
   1.0.1

 Description
  TX service for the ANSIBLE  

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
   Sai Koppaka 5/13/18 
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  
#include "driverlib/gpio.h"


#include "inc/hw_pwm.h"
#include "inc/hw_nvic.h"
#include "inc/hw_timer.h"
#include "inc/tm4c123gh6pm.h" // Define PART_TM4C123GH6PM in project
#include "inc/hw_uart.h" 


#include "AnsibleMain.h"
#include "AnsibleTransmit.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void BuildTXPacket();  

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static AnsibleTXState_t CurrentState;
static bool Ready2TX = false; 

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitAnsibleTX

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
      Sai Koppaka 5/13/18
****************************************************************************/
bool InitAnsibleTX(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial state
  CurrentState = InitTX;  
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
     PostAnsibleTX

 Parameters
     EF_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
   Sai Koppaka 5/13/18
****************************************************************************/
bool PostAnsibleTX(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateFSM

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
    s k first pass
****************************************************************************/
ES_Event_t RunAnsibleTXSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  

  switch (CurrentState)
  {
    case InitTX:        // If current state is initial Psedudo State
    {
      if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
      {
        CurrentState = WaitingToTX;
      }
    }
    break;

    case WaitingToTX:        // If current state is state one
    {
      switch (ThisEvent.EventType)
      {
        case ES_BEGIN_TX:  //If event is event one
        {   
          CurrentState = Transmitting;  //Set next state to transmitting
          if((HWREG(UART2_BASE+UART_O_FR+UART_FR_TXFE)))//If TXFE is set (empty) ***check logic
          {
            //Build the packet to send
              BuildTXPacket();  
            
            //Write the new data to the UARTDR (first byte)
              //HWREG(UART2_BASE+UART_O_DR) =   
            
            //Enable TXIM (Note: also enabled in AnsibleMain)
            HWREG(UART2_BASE + UART_O_IM) |= (UART_IM_TXIM); 
            
            //return Success 
            Ready2TX = true;
          }
          else
          {
            //return Error 
            Ready2TX = false; 
          }
        }
        break;
          ;
      }  // end switch on CurrentEvent
    }
    break;

    case Transmitting:        // If current state is state one
    {
      switch (ThisEvent.EventType)
      {
        case ES_TX_COMPLETE:  //If event is event one
        {   
          CurrentState = WaitingToTX; 
        }
        break;
          ;
      }  // end switch on CurrentEvent
    }
    break;
  }                                   // end switch on Current State
  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:21
****************************************************************************/
AnsibleTXState_t QueryTemplateFSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

void AnsibleTXISR (void)
{
  //Read the Masked Interrupt Status (UARTMIS)

  //If TXMIS Is Set 
  if ((HWREG(UART2_BASE + UART_O_MIS + UART_MIS_TXMIS))) //if bit is set, then an interrupt has occured
  {
    //Write the new data to register (UARTDR)
    //HWREG(UART2_BASE+UART_O_DR) =   
    //Set TXIC in UARTICR (clear int)
    HWREG(UART2_BASE + UART_O_ICR) |= UART_ICR_TXIC; 
    //If this was the last byte in message block
        //Disable TXIM 
  }
  else
  {
    Ready2TX = false; 
    //you are done (not an TX interrupt)
  }
  
  //Post the ES_TX_COMPLETE
   ES_Event_t ReturnEvent; 
   ReturnEvent.EventType = ES_TX_COMPLETE; 
   PostAnsibleTX (ReturnEvent); 
  
  
}