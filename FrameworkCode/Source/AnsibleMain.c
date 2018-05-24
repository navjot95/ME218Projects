/****************************************************************************
 Module
 AnsibleMain.c

 Revision
   1.0.1

 Description
  Service to interface with the controller 

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
first pass   Sai Koppaka 5/13/18
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/

#include "SensorUpdate.h"


#include "ES_Configure.h"
#include "ES_Framework.h"


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
#define BitsPerNibble 4
#define UART2_RX_PIN GPIO_PIN_6 //Port D6
#define UART2_TX_PIN        GPIO_PIN_7 //Port D7
#define ATTEMPT_TIME              200     //200ms
#define PAIRING_TIME            1000      //1 sec time 

//Defines for Class Packets 
#define REQ_2_PAIR              0x01
#define CTRL                    0x03 
#define PAIR_ACK                0x02
#define STATUS                  0x04

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static AnsibleMainState_t CurrentState;


//Module Level Variables 
static bool pair_var; 
static uint8_t currentBoat = 6; 

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

//public getter function
uint8_t DestAddressMSB(void); 
uint8_t DestAddressLSB(void); 
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitAnsibleMain

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
bool InitAnsibleMain(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  
  UARTHardwareInit(); //Initialize HW for UART 

  // put us into the Initial PseudoState
  CurrentState = InitAnsible;
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
     PostAnsible 

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
bool PostAnsibleMain(ES_Event_t ThisEvent)
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
     Sai Koppaka 5/13/18 
****************************************************************************/
ES_Event_t RunAnsibleMainSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  //Create a NextState Variable
  static AnsibleMainState_t NextState; 
  NextState = CurrentState;

  switch (CurrentState)
  {
    case InitAnsible:        // If current state is WaitingForPairing
    {
      if (ThisEvent.EventType == ES_INIT)    
      {
        //set bool paired to false 
         pair_var = false; 
        //Set next state to WaitingforPair
        NextState = WaitingForPair;  
      }
    }
    break; //break out of InitAnsible

    case WaitingForPair:        // If current state is state one
    {
       
        if(ThisEvent.EventType == ES_PAIRBUTTONPRESSED)  // only respond if the button is pressed to pair to a specific team 
          {  
             currentBoat = getBoatNumber(); 
         //   printf("\n \r in waiting for pair state");
            //set ship address (**getter function that determines ship destination address and sends dest address to ansibletx)  
            //send packet to SHIP (0x01)
             ThisEvent.EventType = ES_BEGIN_TX;
             ThisEvent.EventParam = REQ_2_PAIR; //CTRL;//REQ_2_PAIR; //// REQ_2_PAIR;
             PostAnsibleTX(ThisEvent); 
              
             // currentBoat = getBoatNumber(); 
              
            // printf("\n \r ES_PairButtonPressed");
             //printf("\n \r EventParam = %x", ThisEvent.EventParam);
            
            //Start attempt time to 200ms to keep trying at a rate of 5 Hz 
            ES_Timer_InitTimer (PAIR_ATTEMPT_TIMER,ATTEMPT_TIME); 
            
          //Start Pairing Timer to 1sec 
            ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); 
             
            NextState = WaitingForPairResp;  //Decide what the next state will be
        }
    }
    break; //break out of WaitingForPair 
    
    case WaitingForPairResp:        // If current state is state one
    {
      switch (ThisEvent.EventType)
      {
        case ES_TIMEOUT:  //if this event is a timeout 
        {  
          if (ThisEvent.EventParam == PAIR_ATTEMPT_TIMER)
          { 
             //currentBoat = getBoatNumber(); 
             ES_Timer_InitTimer (PAIR_ATTEMPT_TIMER,ATTEMPT_TIME); //reset timer
             //Self transition and send packet (0x01) again to the SHIP
              NextState = WaitingForPairResp; 
            //send packet to SHIP (0x01)
             ThisEvent.EventType = ES_BEGIN_TX;
             ThisEvent.EventParam = REQ_2_PAIR;
             PostAnsibleTX(ThisEvent); 
          //   printf("\n \r es_begin_tX");
          }
          else if (ThisEvent.EventParam == PAIR_TIMEOUT_TIMER)
          {
            //ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); 
            //set nextstaate to WaitingForPair
         //   printf("\n \r going bACK TO waiting for pair"); 
            NextState = WaitingForPair; 
          }
        }
        break;
        case ES_CONNECTIONEST:  //if this event is a timeout 
        {  
       //   printf("\n \r connection established event"); 
         //local variable paired = true
          pair_var = true; 
          //Start Pairing Timer (1sec)
          ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); //reset timer
          //Start Attempt Timer (200ms)
          ES_Timer_InitTimer (PAIR_ATTEMPT_TIMER,ATTEMPT_TIME); //reset timer 
          
              ThisEvent.EventType = ES_BEGIN_TX;
             ThisEvent.EventParam = CTRL; //add cntrl data
             PostAnsibleTX(ThisEvent); 
          
          NextState = CommunicatingSHIP;  //Decide what the next state will be
        }
        break; 
      }
    }
    break; //break out for state
    
    case CommunicatingSHIP:        // If current state is state one
    {
      switch (ThisEvent.EventType)
      {
       case ES_PAIRBUTTONPRESSED:  //if connection established event is posted
        {  
           //send packet to SHIP (0x01)
             ThisEvent.EventType = ES_BEGIN_TX;
             ThisEvent.EventParam = REQ_2_PAIR;
             PostAnsibleTX(ThisEvent);  
          //start 200ms timer
           ES_Timer_InitTimer (PAIR_ATTEMPT_TIMER,ATTEMPT_TIME); //reset 200 timer 
          //start 1s timer
           ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); //reset timer
          //Paired = false 
          pair_var = false; 
         // now put the machine into the actual initial state
          NextState = WaitingForPairResp;  //Decide what the next state will be
        }
        break;
        case ES_TIMEOUT:  //if connection established event is posted
        {  
          if (ThisEvent.EventParam == PAIR_ATTEMPT_TIMER)
          {
          // printf("\r\n ATTEMPT timeout"); 
             //send CNTRL packet to SHIP 
             ThisEvent.EventType = ES_BEGIN_TX;
             ThisEvent.EventParam = CTRL; //add cntrl data
             PostAnsibleTX(ThisEvent); 
         //    printf("\n \r timed out pair attempt"); 
           
            //reset PAIR_ATTEMPT_TIMER
            ES_Timer_InitTimer (PAIR_ATTEMPT_TIMER,ATTEMPT_TIME); //reset 200 timer
            ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); //reset 200 timer
            NextState = CommunicatingSHIP;  //Decide what the next state will be
          }
           if (ThisEvent.EventParam == PAIR_TIMEOUT_TIMER)
          {
          //local bool paired = false
           pair_var = false; 
           NextState = WaitingForPair;  //NextState
         //   printf("\n \r timed out"); 
          }
        }
        
        break;  
        
        case STATUS_RX:  //if connection established event is posted
        {  
            //reset PAIR_TIMEOUT_TIMER
            ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); //reset timer 
            NextState = CommunicatingSHIP;  //Decide what the next state will be
        }
        break;
      }
    }
      break; //break out of switch 
      
  }                                   // end switch on Current State
  CurrentState = NextState; 
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
AnsibleMainState_t QueryAnsible(void)
{
  return CurrentState;
}


uint8_t getCurrentBoat( void )
{
    return currentBoat; 
}
/***************************************************************************
 private functions
 ***************************************************************************/



/***************************************************************************
 public functions
 ***************************************************************************/
bool getpairStatus( void )
{
  return pair_var;
}
