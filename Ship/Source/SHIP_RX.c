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
#include "Init_UART.h"

/*----------------------------- Module Defines ----------------------------*/
#define RX_PERIOD   1   // 1 ms 

#define API_HEADER      0x00 //Frame Data byte number for API header
#define BROADCAST_BYTE  0x04 //Frame Data byte to check broadcast
#define STATUS_BYTE     0x02 //Frame Data byte to check status
#define MESSAGE_HEADER  0x05 //Frame Data byte numebr for Message header
#define TX_STATUS       0x89 //API Header for Status of TX 
#define RX_PACKET       0x81 //API Header for Received Message
#define TX_NO_ACK       0x01 //Status for No ack from othe radio
#define TX_CCA_FAIL     0x02 //Status for failure to get on the air
#define BROADCAST       BIT1HI //Bit indicating broacast message
#define ADDRESS_MSB     0x01 //Byte for the MSB of the source address
#define ADDRESS_LSB     0x02 //Byte for the LSB of the source address 

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void SavePacketData(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file

static SHIP_RX_State_t CurrentState;
static uint16_t DataLength;
static uint8_t  RxFrameDataBuffer[110];
static uint8_t  RxMessageDataBuffer[33];
static uint16_t SourceAddress;

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

  Init_UART_PIC();
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
  ES_Event_t Event2Post;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static uint8_t ByteCounter;
  static uint8_t CheckSum;

  switch ( CurrentState )
  {
    case WaitingForStart :
      if((ThisEvent.EventType == BYTE_RECEIVED) && (ThisEvent.EventParam == 0x7E))
      {
        //Start the inter byte timer
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        // Initialize DataLength to 0 at the start
        DataLength = 0;
        //Go to waiting for MSB of length
        CurrentState = WaitingForMSBLen;
      }     
      break;


    case WaitingForMSBLen :  //If we are waiting for the MSB of length
        //If this is the inter byte timeout
      if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == BYTE_TIMER))
      {
        CurrentState = WaitingForStart; //Go back to waiting for a start byte
      }
      else if (ThisEvent.EventType == BYTE_RECEIVED)
      { //If this is a byte received
        //Restart the inter byte timer
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        //Set the top 8 bit of the length
        DataLength |= (ThisEvent.EventParam << 8);
        //Go to waiting for LSB of length
        CurrentState = WaitingForLSBLen;
      }
      break;  

    case WaitingForLSBLen:
      if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == BYTE_TIMER))
      {
        CurrentState = WaitingForStart; //Go back to waiting for a start byte
      }
      else if (ThisEvent.EventType == BYTE_RECEIVED) 
      {
        // Start timer for RX_PERIOD
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        // Set the LSB byte of DataLength
        DataLength |= ThisEvent.EventParam;
        // Initialize CheckSum to 0xFF
        CheckSum = 0xFF;
        // Initialize ByteCounter to 0
        ByteCounter = 0;
        CurrentState = ReceivingData;
      }
      break;

    case ReceivingData: 
      if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == BYTE_TIMER))
      {
        CurrentState = WaitingForStart; //Go back to waiting for a start byte
      }
      else if (ThisEvent.EventType == BYTE_RECEIVED)
      {
        ES_Timer_InitTimer(BYTE_TIMER, RX_PERIOD);
        RxFrameDataBuffer[ByteCounter++] = ThisEvent.EventParam;
        CheckSum -= ThisEvent.EventParam;
        if (ByteCounter == DataLength)
        {
          CurrentState = ReceivingCheckSum;
        }
      }

    case ReceivingCheckSum:
      if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == BYTE_TIMER))
      {
        CurrentState = WaitingForStart; //Go back to waiting for a start byte
      }
      else if (ThisEvent.EventType == BYTE_RECEIVED)
      {
        if (ThisEvent.EventParam == CheckSum)
        {
          // If message is TX_STATUS
          if (RxFrameDataBuffer[API_HEADER] == TX_STATUS)
          {
            // If message failed
            if ( (RxFrameDataBuffer[STATUS_BYTE] == TX_NO_ACK) || (RxFrameDataBuffer[STATUS_BYTE] == TX_CCA_FAIL))
            {
              // Send failed message event
              Event2Post.EventType = ES_Tx_FAIL;
              PostSHIPComm(Event2Post);
            }
          }
          // If message is data packet
          else if (RxFrameDataBuffer[API_HEADER] == RX_PACKET)
          {
            // If the data was addressed to our SHIP || our SHIP is waiting to pair
            if (!(RxFrameDataBuffer[BROADCAST_BYTE] & BROADCAST) || (QuerySHIPComm() == WaitingToPair))
            {
              // Save source address
              SourceAddress = ((uint16_t) RxFrameDataBuffer[ADDRESS_MSB])<<8;
              SourceAddress |= (uint16_t) RxFrameDataBuffer[ADDRESS_LSB];

              // Save the data into a message data buffer
              SaveMessageData();

              // Send packet received event, param = message header
              Event2Post.EventType = PACKET_RECEIVED;
              Event2Post.EventParam = RxFrameDataBuffer[MESSAGE_HEADER];
              PostSHIPComm(Event2Post);
            }
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
  QuerySHIPRx

Parameters
  None

Returns
  SHIP_RX_State_t The current state of the SHIPRx state machine

Description
  returns the current state of the DogRx state machine

Notes

****************************************************************************/
SHIP_RX_State_t QuerySHIP_RX ( void )
{
   return(CurrentState);
}

/****************************************************************************
Function
  QuerySourceAddress

Parameters
  None

Returns
  uint16_t The source address of the last non-broadcast message

Description
  returns the source address of the last non-broadcast message

Notes

****************************************************************************/
uint16_t QuerySourceAddress ( void )
{
   return(SourceAddress);
}

/****************************************************************************
Function
  QueryRxMessageBuffer

Parameters
  None

Returns
  uint8_t* The pointer to the message buffer

Description
  returns the pointer to the most recently received message

Notes

****************************************************************************/
uint8_t* QueryRxMessageBuffer ( void )
{
   return(RxMessageDataBuffer);
}

/***************************************************************************
  private functions
 ***************************************************************************/
static void SaveMessageData(void)
{
  for(int i = 5; i < DataLength; i++)
  {
    RxMessageDataBuffer[i-5] = RxFrameDataBuffer[i];
  }
}
