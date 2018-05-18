/****************************************************************************
 Module
 AnsibleTransmit.c

 Revision
   1.0.1

 Description
  RX service for the ANSIBLE  

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

#include "AnsibleTransmit.h"
#include "AnsibleReceive.h"

/*----------------------------- Module Defines ----------------------------*/
#define RX_TIME 500 //sending bits at 500ms time interval 
#define BitsPerNibble 4
#define UART2_RX_PIN GPIO_PIN_6 //Port D6
#define UART2_TX_PIN GPIO_PIN_7 //Port D7


//Defines for XBee
#define Start_Delimiter 0x7E
#define API_Identifier 0x81
#define Options 0x00 //ack enabled 

//Defines for Class Packets 
#define BLUE 0x00
#define RED 0x01
#define REQ_2_PAIR 0x01
#define CTRL 0x03 
#define PAIR_ACK 0x02
#define STATUS 0x04

//TX Bytes
#define TX_Status_IDX 0 
#define TX_Success  1
#define TX_Bad      2

//Bit Positions
#define API_Identifier_Index 0
#define API_PACKET_ACK 5 //first bit of RF data is packet type

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static AnsibleRXState_t CurrentState;
static PacketType_t  PacketType; 

//Variables for XBee API RX 
static uint16_t index = 0; 

static uint16_t Data_Length;  //number of bytes (**arbitrarily set") 
static uint8_t Computed_CheckSum; //initialize check sum to 0xFF
static uint8_t SourceAddressLSB;
static uint8_t SourceAddressMSB; 

//static bool receiving; 

//Arrays
uint8_t RXData_Packet[100]; //***setting initial large value of array*** needs change

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitAnsibleRX

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
bool InitAnsibleRX(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial state
    CurrentState = WaitingForStart;  

   //Initialize UART HW
  //  UARTHardwareInit();  
  
  //Disable UART TX Interrupt so that default is
  // HWREG(UART2_BASE + UART_O_IM) &= ~(UART_IM_TXIM); 
  //Enable RX   
  // HWREG(UART2_BASE + UART_O_IM) |= (UART_IM_RXIM); 
  
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
bool PostAnsibleRX(ES_Event_t ThisEvent)
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
ES_Event_t RunAnsibleRXSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  

  switch (CurrentState)
  {
    case WaitingForStart:        // If current state is initial Psedudo State
    {
        //Set the initial state 
        CurrentState = WaitingForMSBLen;     
         
        if((ThisEvent.EventType == BYTE_RECEIVED) && (ThisEvent.EventParam == Start_Delimiter))
        {
             //Enable Timer 
              ES_Timer_InitTimer (RX_ATTEMPT_TIMER,RX_TIME); 
          
              //Initialize DataLength to 0
              Data_Length = 0; 
                
            //Set initial variables 
             Computed_CheckSum = 0xFF; 
             index = 0;  

         //Get SHIPTeamSelect, Initialized to SHIP Ansible 
          SourceAddressMSB = 0x86; 
          SourceAddressLSB = 0x21; 
        
        }
    }
    break;

    case WaitingForMSBLen:        // If current state is state one
    {
        if((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == RX_ATTEMPT_TIMER)) //ES_BEGIN_TX:  //If event is event one
        {  
          //Go back to Waiting For Start
          CurrentState = WaitingForStart;
        }
        else if (ThisEvent.EventType == BYTE_RECEIVED)  //0x7E received in time 
        {     
        //Reset timer 
          ES_Timer_InitTimer (RX_ATTEMPT_TIMER,RX_TIME); 
          
        //Record length of MSB (second byte) 
          Data_Length |= (ThisEvent.EventParam <<8); //byte is length of msb
          
        //set next state to WaitingforLSB  
          CurrentState = WaitingForLSBLen; 

            
        } 
     }        
        break;

    case WaitingForLSBLen:        // If current state is state one
    {
        if((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == RX_ATTEMPT_TIMER)) //ES_BEGIN_TX:  //If event is event one
        {  
          //Go back to Waiting For Start
          CurrentState = WaitingForStart;
        }
        else if (ThisEvent.EventType == BYTE_RECEIVED)
        {            
        //Reset timer 
          ES_Timer_InitTimer (RX_ATTEMPT_TIMER,RX_TIME); 
          
        //Record length of LSB 
          Data_Length |= (ThisEvent.EventParam); //byte is the length of the LSB 
          
        //set next state to ReceivingData  
          CurrentState = ReceivingData; 

        } 
     }        
        break;
     
     
    case ReceivingData:        // If current state is state one
    {
        if((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == RX_ATTEMPT_TIMER)) //ES_BEGIN_TX:  //If event is event one
        {  
          //Go back to Waiting For Start
          CurrentState = WaitingForStart;
        }
        else if (ThisEvent.EventType == BYTE_RECEIVED)
        {  
         
        //Reset timer 
          ES_Timer_InitTimer (RX_ATTEMPT_TIMER,RX_TIME); 
         
          //API Identifier (0x81), Source Address (0x20 and 0x8_), RSSI, and Options, data   
          RXData_Packet[index] = ThisEvent.EventParam; //frame data packet
          index ++; //increment the index by the size of the data packet
          //Compute resultant (oxff - sum) 
          Computed_CheckSum -= ThisEvent.EventParam; 
          
        //set next state to WaitingforLSB  
          CurrentState = ReceivingCheckSum; 
        } 
     }        
        break;
     
        case ReceivingCheckSum:        // If current state is state one
    {
      switch (ThisEvent.EventType)
      {
        if((ThisEvent.EventType ==ES_TIMEOUT) && (ThisEvent.EventParam == RX_ATTEMPT_TIMER))
        {
          CurrentState = WaitingForStart; //Go back to waiting   
        }
        else if (ThisEvent.EventType == BYTE_RECEIVED)
        {
            if (ThisEvent.EventParam  == Computed_CheckSum) //good check sum
            {
             //Loook at the aPI_ID to see that it was indeed for transmit
                if ((RXData_Packet[API_Identifier_Index] == API_Identifier))
                {
                 //if there was a ACK (checking the status bit)   
                    if((RXData_Packet[API_PACKET_ACK] != PAIR_ACK))
                    {
                      //Send a fail message
                        //ThisEvent.EventType == ES_TX_FAIL; //the transmit from SHIP-> ANSIBLE failed
                        //PostAnsibleMaster(ThisEvent); 
                        CurrentState = WaitingForStart; //Go back to waiting 
                    }
                    else
                    {
                     //   ThisEvent.EventType = PACKET_RX; 
                     //   PostAnsibleMaster(ThisEvent); 
                    }
                }
            }
            else
            {
               //Send a fail message
               //ThisEvent.EventType == ES_TX_FAIL; //the transmit from SHIP-> ANSIBLE failed
              //PostAnsibleMaster(ThisEvent); 
              CurrentState = WaitingForStart; //Go back to waiting   
            }
            CurrentState = WaitingForStart; //Go back to waiting

        }
        
      }  // end switch on CurrentEvent
    }
    break;    
  }                                   // end switch on Current State
  return ReturnEvent;
}

/****************************************************************************
 Function
     AnsiblrxSM

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
AnsibleTXState_t QueryAnsibleRX(void)
{
  return CurrentState;
}

/***************************************************************************
 public functions
 ***************************************************************************/

///PUBLIC FUNCTIONS//
 /***************************************************************************
 private functions
 ***************************************************************************/
  
