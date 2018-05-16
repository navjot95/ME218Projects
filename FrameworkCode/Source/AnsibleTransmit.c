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

#include "AnsibleTransmit.h"

/*----------------------------- Module Defines ----------------------------*/
#define TX_TIME 500 //sending bits at 500ms time interval 
#define BitsPerNibble 4
#define UART2_RX_PIN GPIO_PIN_6 //Port D6
#define UART2_TX_PIN GPIO_PIN_7 //Port D7


//Defines for XBee
#define Start_Delimiter 0x7E
#define API_Identifier 0x01
#define Options 0x00 //ack enabled 
#define Preamble_Length_TX 8 //API_ID(1 byte), Frame_ID(1byte), dest_address (2 bytes), options(1byte) 

//Defines for Class Packets 
#define REQ_2_PAIR 0x01
#define CTRL 0x03 
#define PAIR_ACK 0x02
#define STATUS 0x04

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void BuildTXPacket(uint8_t PacketType); 
static uint8_t CheckSum(void); 

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static AnsibleTXState_t CurrentState;
static PacketType_t  PacketType; 

static bool Ready2TX = false; 
static uint16_t BytesRemaining = 0; 
static uint16_t index = 0; 
static uint16_t TXPacket_Length; 
//static uint8_t Preamble_Length = 5;  //API_ID(1 byte), Frame_ID(1byte), dest_address (2 bytes), options(1byte) 
static uint8_t Data_Length = 2;  //number of bytes (**arbitrarily set") 
static uint8_t CHK_SUM = 1; //initialize check sum to 0xFF
static uint8_t DestAddressLSB;
static uint8_t DestAddressMSB; 


//Arrays
uint8_t Message_Packet[100]; //***setting initial large value of array*** needs change

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

   //Initialize UART HW
    UARTHardwareInit();  
  
  //Disable UART TX Interrupt so that default is RX 
    HWREG(UART2_BASE + UART_O_IM) &= ~(UART_IM_TXIM); 
  
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
        //Set the initial state 
        CurrentState = WaitingToTX;
        
        //Get SHIPTeamSelect, Initialized to SHIP Ansible 
         DestAddressMSB = 0x86; 
         DestAddressLSB = 0x20; 
        
        //enable timer 
        ES_Timer_InitTimer (TX_ATTEMPT_TIMER,TX_TIME); 
      }
    }
    break;

    case WaitingToTX:        // If current state is state one
    {
      switch (ThisEvent.EventType)
      {
        case ES_TIMEOUT: //ES_BEGIN_TX:  //If event is event one
        {  
          CurrentState = Transmitting;  //Set next state to transmitting
          
           //reset timer
            ES_Timer_InitTimer (TX_ATTEMPT_TIMER,TX_TIME); 
            printf( "\n \r timer has been reset"); 
            
            //Set local variable TXPacket_Length
             TXPacket_Length =  Preamble_Length_TX + Data_Length + CHK_SUM;  
 
           //Initialize BytesRemaining = Length of XBee Packet (Preamble (Start + Length + API_ID) + DATA + CHKSUM) 
              BytesRemaining = TXPacket_Length;  
            //set index = 0 
              index = 0; 
            
            //Build the packet to send
              BuildTXPacket(BUILD_CTRL);  

          if((HWREG(UART2_BASE+UART_O_FR)) & ((UART_FR_TXFE)))//If TXFE is set (empty)
          {
            printf("\n \r fifo is empty"); 

            //Write the new data to the UARTDR (first byte)
              HWREG(UART2_BASE+UART_O_DR) =  Message_Packet[index];  
            //decremet Bytes Remaining 
              BytesRemaining--;
            //Increment index 
               index++; 
             if((HWREG(UART2_BASE+UART_O_FR)) & ((UART_FR_TXFE))) //if the TXFE is set (still empty)
             {
               //Write the second byte to the UART DR 
               HWREG(UART2_BASE+UART_O_DR) = Message_Packet[index];  
               BytesRemaining--; 
               index++; 
             }
            //Enable TXIM (Note: also enabled in UARTInit)
               HWREG(UART2_BASE + UART_O_IM) |= (UART_IM_TXIM); 
             
            //Enable Interrupts globally  (also enabled in UARTinit) 
               __enable_irq();
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
     AnsibleMainSM

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
AnsibleTXState_t QueryAnsibleTransmit(void)
{
  return CurrentState;
}

/***************************************************************************
 public functions
 ***************************************************************************/

void AnsibleTXISR (void)
{
  //Read the Masked Interrupt Status (UARTMIS)
  
  //If TXMIS Is Set 
  if ((HWREG(UART2_BASE + UART_O_MIS)) & (UART_MIS_TXMIS)) //if bit is set, then an interrupt has occured
  {
      printf("\n \r bit set");
    //Write the new data to register (UARTDR)
      HWREG(UART2_BASE+UART_O_DR) = Message_Packet[index]; 
    //decrement BytesRemaining 
      BytesRemaining--; 
    //increment index
      index++;
    if (BytesRemaining == 0)
    {
    
    HWREG(UART2_BASE + UART_O_IM) &= ~(UART_IM_TXIM); //disable interrupt on TX by clearing TXIM 
   //Post the ES_TX_COMPLETE (note: TX complete does not mean that that the Packet has been sent) 
    ES_Event_t ReturnEvent; 
    ReturnEvent.EventType = ES_TX_COMPLETE; 
    PostAnsibleTX (ReturnEvent); 
    }
    //Set TXIC in UARTICR (clear int)
     HWREG(UART2_BASE + UART_O_ICR) |= UART_ICR_TXIC;  //clear TX interrupt
  }
  else
  {
    Ready2TX = false; 
    //you are done (not an TX interrupt)
  }
  
}

///PUBLIC FUNCTIONS//

void UARTHardwareInit(void){
//Setting up the registers for UART-XBee communications
  
  //Enable the clock to the UART module using the RCGCUART (run time gating clock control) register
   HWREG(SYSCTL_RCGCUART) |= SYSCTL_RCGCUART_R2; //UART2 Clock
  
  //Wait for the UART to be ready (PRUART)
   while ((HWREG(SYSCTL_PRUART) & SYSCTL_PRUART_R2) != SYSCTL_PRUART_R2); 
  
  //Enable the clock to the GPIO port D
   HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;
  
  //Wait for the GPIO module to be ready  (PRGPIO)
   while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_RCGCGPIO_R3) != SYSCTL_RCGCGPIO_R3); 
  
  //Configure the GPIO pine for in/out/drive-level/drive-type 
     HWREG(GPIO_PORTD_BASE+GPIO_O_DEN) |= (UART2_TX_PIN|UART2_RX_PIN); //setting pins as digital
     HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) &= ~(UART2_RX_PIN); //setting RX as input 
     HWREG(GPIO_PORTD_BASE+GPIO_O_DIR) |= (UART2_RX_PIN); //setting TX as output
  
  //Select the Alternative functions for the UART pins (AFSEL)(AFSEL Table pg.1351)
   HWREG(GPIO_PORTD_BASE+GPIO_O_AFSEL) |= (UART2_TX_PIN|UART2_RX_PIN); 
    
  //Configure the PMCn fields in the GPPIOPCTL (p.689) register to assign the UART pins
    HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xf00fffff)+(1<<(6*BitsPerNibble))+(1<<(7*BitsPerNibble)); //Write 1 to select U2RX as alternative function and to select U2TX as alt fun 

  //Disable the UART by clearning the UARTEN bits in the UARTCTL register
    HWREG(UART2_BASE+UART_O_CTL) &= ~(UART_CTL_UARTEN); 
  
  //Write the inter portion of the URTIBRD register (setting baud rate) 
    HWREG(UART2_BASE+UART_O_IBRD) = HWREG(UART2_BASE+UART_O_IBRD) + 0x104;  //writing 21 in hex
  
  //Write the fractional portion of the BRD to the UARTIBRD register
    HWREG(UART2_BASE+UART_O_FBRD) = HWREG(UART2_BASE+UART_O_FBRD) + 0x1B;  //writing 21 in hex; //writing 45 in hex 

  //Write the desired serial parameters to the UARTLCRH registers to set word length to 8
     HWREG (UART2_BASE + UART_O_LCRH) = (UART_LCRH_WLEN_8); 

  //Configure the UART operation using the UARTCTL register 
   //UART Data Registers should be cleared by default (RXE and TXE are already enabled) **
     HWREG(UART2_BASE + UART_O_CTL) |= (UART_CTL_TXE); 
     HWREG(UART2_BASE + UART_O_CTL) |= (UART_CTL_RXE); 

  //Enable the UART by setting the UARTEN bit in the UARTCTL register 
    HWREG(UART2_BASE + UART_O_CTL) |= (UART_CTL_UARTEN); 
        
  //Enable UART RX Interrupt (p.924)
  //  HWREG(UART2_BASE + UART_O_IM) |= (UART_IM_RXIM); 
  
  //Enable UART TX Interrupt
   HWREG(UART2_BASE + UART_O_IM) |= (UART_IM_TXIM); 
  
  //Enable NVIC (p.104) UART2 is Interrtupt Number 33, so it is EN1, BIT1 HI (p.141)
    HWREG(NVIC_EN1) |= BIT1HI;
  
  //Enable Interrupts Globally 
  __enable_irq();

  //Enable IN KEIL 
  }

  
 /***************************************************************************
 private functions
 ***************************************************************************/
  
  static void BuildPreamble (void)
  {
      Message_Packet[0] = Start_Delimiter;  //0x7E
      Message_Packet[1] = 0x00; //Data_Length = Preamble_Length + SizeofArray MSB
      Message_Packet[2] = 0x02; //Data_Length = Preamble_Length + SizeofArray LSB 
      Message_Packet[3] = API_Identifier;  //API_ID
      Message_Packet[4] = 0x01;  //Frame_ID (**arbitrary for the moment)
      Message_Packet[5] = DestAddressMSB;   //Destination Address MSB
      Message_Packet[6] = DestAddressLSB;   //Destination Address LSB
      Message_Packet[7] = Options;  //Options Byte 
      printf("\n \r sent the preamble bytes"); 
  }
  
  
static void BuildTXPacket(uint8_t PacketType)
{
  switch(PacketType)
  { 
          
    case BUILD_REQ_2_PAIR: 
    {
      //build the preamble 
       BuildPreamble(); 
      //set index to length of preamble
      index = Preamble_Length_TX; 
      //Add RF Data packet corresponding to REQ_2_PAIR
      Message_Packet[index] = REQ_2_PAIR; 
      //increment index
      index++; 
      //calculate CheckSum (); 
      //store CheckSum as the next byte of Message_Packet
      Message_Packet[index] = CheckSum(); 
      printf("\n \r CheckSum=%d", CheckSum());
      printf(" \n \r sent pair request packet"); 
    }
    break;
    
    case BUILD_CTRL: 
    {
      //Build Preamble 
      BuildPreamble();
      //set index to length of preamble
       index = Preamble_Length_TX; 
      //Add RF Data packet corresponding to REQ_2_PAIR
      Message_Packet[index] = CTRL; 
      //increment index
      index++; 
      //calculate CheckSum (); 
      //store CheckSum as the next byte of Message_Packet
      Message_Packet[index] = CheckSum(); 
      printf("\n \r CheckSum=%d", CheckSum());
      printf(" \n \r sent control packet"); 
      
      
    }
    break; 

    case BUILD_STATUS: 
    {
      //BuildPreamble
      BuildPreamble(); 
      //set index to length of preamble
      index = Preamble_Length_TX; 
      //Add RF Data packet corresponding to REQ_2_PAIR
      Message_Packet[index] = STATUS; 
      //increment index
      index++; 
      //calculate CheckSum (); 
      //store CheckSum as the next byte of Message_Packet
      Message_Packet[index] = CheckSum(); 
      printf("\n \r CheckSum=%d", CheckSum());
      printf(" \n \r sent build status packet"); 
      
    }
    break; 
  
  }
  return;
}
static uint8_t CheckSum(void)
{
  uint16_t summation = 0; 
  uint8_t FrameDataStart = 3; //check sum is the 0xFF - (Sum of FrameData)
  uint8_t computed_chksum = 0; 
  
  for(index= FrameDataStart; index<(TXPacket_Length-1); index++)
  {
    summation += Message_Packet[index];
  }
  
  computed_chksum =(0xFF-summation); 
  
  return computed_chksum; 
  
}


//uint8_t setpackettype (ES_Event_t ThisEvent)
//{
//    uint8_t packet_type; 

//    if ((ThisEvent.EventType == ES_NEW_KEY) && (ThisEvent.EventParam == 'p'))
//  {
//    PacketType = BUILD_REQ_2_PAIR; 
//    
//  }
//  return packet_type; 

//}
