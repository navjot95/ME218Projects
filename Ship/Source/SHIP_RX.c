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

#include "SHIP_MASTER.h"
#include "SHIP_RX.h"
#include "SHIP_TX.h"
#include "Init_UART.h"

/*----------------------------- Module Defines ----------------------------*/
// Timer lengths
#define RX_PERIOD   200   // 200 ms (5Hz transmission rate) 

// XBee API Defines
#define START_DELIMITER           0x7E
#define API_IDENTIFIER            0x81
#define TX_STATUS_API_IDENTIFIER  0x89
#define OPTIONS                   0x00

// Class Protocol Defines
#define REQ_2_PAIR_HEADER  0x01
#define BLUE               0x00
#define RED                0x01

#define PAIR_ACK_HEADER    0x02

#define CTRL_HEADER        0x03

#define STATUS_HEADER      0x04

// Frame Data Indices
#define API_IDENTIFIER_IDX      0
#define SOURCE_ADDRESS_MSB_IDX  1
#define SOURCE_ADDRESS_LSB_IDX  2
#define RSSI_IDX                3
#define OPTIONS_IDX             4
#define DATA_HEADER_IDX         5

// TX Status Packet
#define TX_STATUS_IDX           5

#define TX_SUCCESS              0
#define TX_NO_ACK               1
#define TX_CCA_FAIL             2

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file

static SHIP_RX_State_t CurrentState;

static uint16_t DataLength;
static uint8_t  RX_FrameData[100];
static uint8_t  RX_ControlData[10];
static uint16_t SourceAddress;
static uint8_t  ANSIBLEColour;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
Function
  InitSHIP_RX

Parameters
  uint8_t : the priorty of this service

Returns
  bool, false if error in initialization, true otherwise

Description
  Saves away the priority, sets up the initial transition and does any
  other required initialization for this state machine
Notes

****************************************************************************/
bool InitSHIP_RX ( uint8_t Priority )
{
  ES_Event_t ThisEvent;
  
  Init_UART_XBee();

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
  PostSHIP_RX

Parameters
  EF_Event ThisEvent , the event to post to the queue

Returns
  boolean False if the Enqueue operation failed, True otherwise

Description
  Posts an event to this state machine's queue

Notes

****************************************************************************/
bool PostSHIP_RX( ES_Event_t ThisEvent )
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
ES_Event_t RunSHIP_RX( ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
  static uint8_t IDX;
  static uint8_t CheckSum;

  switch ( CurrentState )
  {
    case WaitingForStart :
      if((ThisEvent.EventType == BYTE_RECEIVED) && (ThisEvent.EventParam == START_DELIMITER))
      {
        printf("\r\n%X",ThisEvent.EventParam);
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        DataLength = 0;
        CurrentState = WaitingForMSBLen;
      }     
      break;
      
    case WaitingForMSBLen :
      if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == BYTE_TIMER))
      {
        // Didn't receive a byte in time
        
        // Go back to WaitingForStart
        CurrentState = WaitingForStart;
      }
      else if (ThisEvent.EventType == BYTE_RECEIVED)
      { 
        // Received byte in time
        printf("\r\n%X",ThisEvent.EventParam);
        // Start BYTE_TIMER again
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        
        // Record MSB of data length
        DataLength |= (ThisEvent.EventParam << 8);

        CurrentState = WaitingForLSBLen;
      }
      break;  
      
    case WaitingForLSBLen:
      if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == BYTE_TIMER))
      {
        // Didn't receive a byte in time
        
        // Go back to WaitingForStart
        CurrentState = WaitingForStart;
      }
      else if (ThisEvent.EventType == BYTE_RECEIVED) 
      {
        printf("\r\n%X",ThisEvent.EventParam);
        
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);

        DataLength |= ThisEvent.EventParam;

        CheckSum = 0xFF;

        IDX = 0;
        
        CurrentState = ReceivingData;
      }
      break;     

    case ReceivingData: 
      if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == BYTE_TIMER))
      {
        CurrentState = WaitingForStart;
      }
      else if (ThisEvent.EventType == BYTE_RECEIVED)
      {
        printf("\r\n%X",ThisEvent.EventParam);
        
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        
        // First will be API Identifier (0x81), then Source Address (0x20 and 0x8_)
        // then RSSI (signal strength), then Options (0x00), then data
        RX_FrameData[IDX] = ThisEvent.EventParam;
        IDX++;
        CheckSum -= ThisEvent.EventParam;
        
        if (IDX == DataLength)
        {
          CurrentState = ReceivingCheckSum;
        }
        
      }
      break;
      
    case ReceivingCheckSum:
      if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == BYTE_TIMER))
      {
        CurrentState = WaitingForStart; //Go back to waiting for a start byte
      }
      else if (ThisEvent.EventType == BYTE_RECEIVED)
      {

        if (ThisEvent.EventParam == CheckSum)
        {
          printf("\r\n%X",ThisEvent.EventParam);
          printf("\r\n ");
          // If message is TX_STATUS (MIGHT BE UNNESSARY)
          if (RX_FrameData[API_IDENTIFIER_IDX] == TX_STATUS_API_IDENTIFIER)
          {
            // If message failed (MIGHT BE UNNESSARY)
            if ( (RX_FrameData[TX_STATUS_IDX] == TX_NO_ACK) || (RX_FrameData[TX_STATUS_IDX] == TX_CCA_FAIL))
            {
              // Send failed message event
              ThisEvent.EventType = ES_TX_FAIL;
              
              // UNCOMMENT
              PostSHIP_MASTER(ThisEvent);
            }
          }
          // If message is data packet
          else if (RX_FrameData[API_IDENTIFIER_IDX] == API_IDENTIFIER)
          {
            if (RX_FrameData[DATA_HEADER_IDX] == CTRL_HEADER)
            {
              // Save source address
              SourceAddress |= ((uint16_t) RX_FrameData[SOURCE_ADDRESS_MSB_IDX])<<8;
              SourceAddress |= (uint16_t) RX_FrameData[SOURCE_ADDRESS_LSB_IDX];
              
              static uint8_t i;
              
              // Build array containing just the control data 
              for (i=0;i<5;i++)
              {
                RX_ControlData[i] = RX_FrameData[i+9];
              }
              
              // Post to MasterSM that control packet was received
              
              // UNCOMMENT
              ThisEvent.EventType = ES_CONTROL_PACKET;
              PostSHIP_MASTER(ThisEvent);
            }

            // if req_2_pair packet, save ansible colour
            if (RX_FrameData[DATA_HEADER_IDX] == REQ_2_PAIR_HEADER)
            {
              ANSIBLEColour = RX_FrameData[9];
              
              // UNCOMMENT
              ThisEvent.EventType = ES_PAIR_REQUEST;
              PostSHIP_MASTER(ThisEvent);
            }
              
            // Send packet received event, param = message header
            ThisEvent.EventType = PACKET_RECEIVED;
            
            // UNCOMMENT
            PostSHIP_MASTER(ThisEvent);
          }
        }
        CurrentState = WaitingForStart;
      }
      break;
    }
  return ReturnEvent;
}  

/****************************************************************************
Function
	QuerySourceAddress

Parameters
	None

Returns
	SourceAddress of the ANSIBLE connected

Description
	returns the Source Address of the SHIP_RX state machine

Notes

****************************************************************************/

uint16_t QuerySourceAddress(void)
{
  return SourceAddress;
}

uint8_t Query_ANSIBLEColour (void)
{
  return ANSIBLEColour;
}

uint8_t Query_FB (void)
{
  return RX_ControlData[0];
}

uint8_t Query_LR (void)
{
  return RX_ControlData[1];
}

uint8_t Query_TurretR (void)
{
  return RX_ControlData[2];
}

uint8_t Query_TurretP (void)
{
  return RX_ControlData[3];
}

uint8_t Query_CTRL (void)
{
  return RX_ControlData[4];
}
