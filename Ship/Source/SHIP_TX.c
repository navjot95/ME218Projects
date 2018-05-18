/****************************************************************************
Module
    SHIP_TX.c

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
#include "Init_UART.h"

/*----------------------------- Module Defines ----------------------------*/
// XBee API Defines
#define START_DELIMITER    0x7E
#define API_IDENTIFIER     0x01
#define FRAME_ID           0x01
#define OPTIONS            0x00

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

// Data Length
#define LENGTH_MSB_PAIR_ACK     0x00
#define LENGTH_LSB_PAIR_ACK     0x06
#define LENGTH_MSB_STATUS       0x00
#define LENGTH_LSB_STATUS       0x08

// EventParam 
#define PAIR_ACK_EVENT  0
#define STATUS_EVENT    1

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void BuildPacket(ES_Event_t ThisEvent);
static uint8_t CheckSum(uint8_t CHECKSUM_INDEX);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file

static SHIP_TX_State_t CurrentState;

static uint8_t IDX = 0;
static uint8_t DataLength;
static uint8_t Packet[100];
static uint8_t PacketLength = 0;

static uint16_t SourceAddress;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
Function
  InitSHIP_TX

Parameters
  uint8_t : the priorty of this service

Returns
  bool, false if error in initialization, true otherwise

Description
  Saves away the priority, sets up the initial transition and does any
  other required initialization for this state machine
Notes

****************************************************************************/
bool InitSHIP_TX ( uint8_t Priority )
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // First state is waiting for 0x7E
  CurrentState = WaitingToTX;
  // post the initial transition event
  //ThisEvent.EventType = ES_INIT;
  // any other initializations
  
  //Init_UART_XBee();
  
  // %%%%% TEST %%%%% //
  //ES_Timer_InitTimer(TEST_TIMER, 500);  
  // %%%%% TEST %%%%% //
  
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
	PostSHIP_TX

Parameters
	EF_Event ThisEvent , the event to post to the queue

Returns
	boolean False if the Enqueue operation failed, True otherwise

Description
	Posts an event to this state machine's queue

Notes

****************************************************************************/
bool PostSHIP_TX( ES_Event_t ThisEvent )
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
ES_Event_t RunSHIP_TX( ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors  
  
  switch ( CurrentState )
  {
    case WaitingToTX :
      
      // %%%%% TEST %%%%% //
      //if (ThisEvent.EventType == ES_TIMEOUT) 
      //{        
      // %%%%% TEST %%%%% //
    
      if (ThisEvent.EventType == BEGIN_TX)
      {
        IDX = 0;
        
        // %%%%% TEST %%%%% //
        //ES_Timer_InitTimer(TEST_TIMER, 500);
        // %%%%% TEST %%%%% //
        
        // Construct Data Packet
        BuildPacket(ThisEvent);

        if (HWREG(UART5_BASE + UART_O_FR) & UART_FR_TXFE) // (Room to TX byte)
        {
          // TX Byte
          HWREG(UART5_BASE + UART_O_DR) = Packet[IDX];
          
          // Disable RX and Enable TX Interrupt
          HWREG(UART5_BASE + UART_O_IM) &= ~UART_IM_RXIM;  
          HWREG(UART5_BASE + UART_O_IM) |= UART_IM_TXIM;        
          IDX++;
          
          CurrentState = SendingTX;    
        
        }
      }
      break;
      
    case SendingTX :
      if (ThisEvent.EventType == BYTE_SENT)
      {
        CurrentState = WaitingToTX;
      }
      break;
  }
  return ReturnEvent;
}


/****************************************************************************
Function
  SHIP_XBEE_ISR

Parameters
  void

Returns
  void

Description
  ISR for XBee communication of SHIP


****************************************************************************/

void SHIP_XBEE_ISR(void)
{
  static ES_Event_t ThisEvent;
  
  // XBee TX
  if(HWREG(UART5_BASE + UART_O_MIS) & UART_MIS_TXMIS)
  {
    // Clear interrupt
		HWREG(UART5_BASE + UART_O_ICR) |= UART_ICR_TXIC;
		
		//Write next byte to DR
		HWREG(UART5_BASE + UART_O_DR) = Packet[IDX];
    
		//Increment the index
		IDX++;
		
    if(IDX == DataLength)
    { 
      // End of packet, Disable TX and Enable RX
			HWREG(UART5_BASE + UART_O_IM) &= ~UART_IM_TXIM;
      HWREG(UART5_BASE + UART_O_IM) |= UART_IM_RXIM;
			
			ThisEvent.EventType = BYTE_SENT;
			PostSHIP_TX(ThisEvent);
		}	
	}
  
  // XBee RX
  if (HWREG(UART5_BASE + UART_O_MIS) & UART_MIS_RXMIS)
  {
    HWREG(UART5_BASE + UART_O_ICR) |= UART_ICR_RXIC;
    
    ThisEvent.EventType = BYTE_RECEIVED;
    ThisEvent.EventParam = HWREG(UART5_BASE + UART_O_DR);
    PostSHIP_RX(ThisEvent);
  }
}


 /***************************************************************************
 private functions
 ***************************************************************************/

static void BuildPacket(ES_Event_t ThisEvent)
{
  SourceAddress = QuerySourceAddress();
  
  Packet[0] = START_DELIMITER;  // 0x7E
  Packet[3] = API_IDENTIFIER;   // 0x01
  Packet[4] = FRAME_ID;         // 0x01
  Packet[5] = (uint8_t)(SourceAddress>>8);
  Packet[6] = (uint8_t)SourceAddress;
  Packet[7] = OPTIONS;
  
  // %%%%% TEST %%%%% //
//  Packet[1] = LENGTH_MSB_STATUS;  // 0x00
//  Packet[2] = LENGTH_LSB_STATUS;  // 0x08
//  Packet[5] = 0x21;
//  Packet[6] = 0x86;
//  Packet[8] = 0x04;
//  Packet[9] = 0xAA;
//  Packet[10] = 0x11;
//  Packet[11] = CheckSum(11);
//  DataLength = 12;
  // %%%%% TEST %%%%% //
  
  if (ThisEvent.EventParam == PAIR_ACK_EVENT)
  {
    Packet[1] = LENGTH_MSB_PAIR_ACK; // 0x00
    Packet[2] = LENGTH_LSB_PAIR_ACK; // 0x06
    Packet[8] = 0x02;
    Packet[9] = CheckSum(9);
    DataLength = 10;
  }
  
  if (ThisEvent.EventParam == STATUS_EVENT)
  {
    Packet[1] = LENGTH_MSB_STATUS;  // 0x00
    Packet[2] = LENGTH_LSB_STATUS;  // 0x08
    Packet[8] = 0x04;
    Packet[9] = QueryFuelStatus();
    Packet[10] = Query_CTRL();
    Packet[11] = CheckSum(11);
    DataLength = 12;
  }
}

static uint8_t CheckSum(uint8_t CHECKSUM_INDEX)
{
  static uint8_t CheckSum = 0xFF;
  static uint8_t i;
  
  CheckSum = 0xFF;
  
  for (i=3;i<CHECKSUM_INDEX;i++)
  {
    CheckSum -= Packet[i];
  }
  
  return CheckSum; 
}
