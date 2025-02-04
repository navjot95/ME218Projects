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
#include "AnsibleMain.h"
#include "AnsibleReceive.h"
#include "SensorUpdate.h"

/*----------------------------- Module Defines ----------------------------*/
#define TX_TIME 500 //sending bits at 500ms time interval 
#define BitsPerNibble 4
#define UART5_RX_PIN GPIO_PIN_4 //Port E4
#define UART5_TX_PIN GPIO_PIN_5 //Port E5
#define TEAM_PIN      BIT6HI    // Port C 

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
static void UARTHardwareInit(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static AnsibleTXState_t CurrentState;
//static PacketType_t  PacketType; 
//static bool Ready2TX = false; 
static uint8_t BytesRemaining = 0; 
static uint16_t index = 0; 
static uint8_t TXPacket_Length; 
//static uint8_t Preamble_Length = 5;  //API_ID(1 byte), Frame_ID(1byte), dest_address (2 bytes), options(1byte) 
static uint8_t Packet; 

static uint8_t Data_Length = 2;  //number of bytes (**arbitrarily set") 
static uint8_t CHK_SUM = 1; //initialize check sum to 0xFF
static uint8_t DestAddressLSB_val;
static uint8_t DestAddressMSB_val; 
//static bool receiving = false; 
static uint8_t IDX; 
static uint8_t TeamColor; 


//Arrays
static uint16_t DestAddress[11];
static uint8_t Message_Packet[100]; //***setting initial large value of array*** needs change
uint8_t RXMessage_Packet[100]; //not static because I want to access from AnsibleRX 

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
  
      
       // Will use port C, D and E 
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2; // PORT C
    while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R2) != SYSCTL_PRGPIO_R2)
        ;
      
    HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= TEAM_PIN; // Digital Enable
    HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) &= ~TEAM_PIN; // Set input (clear bit)
    
    
    UARTHardwareInit(); //Initialize HW for UART 
    DestAddress[0] = 0x2181; // = DestAddressMSB(); //= 0x20; 
    DestAddress[1] = 0x2182;
    DestAddress[2] = 0x2183;
    DestAddress[3] = 0x2184;
    DestAddress[4] = 0x2185;
    DestAddress[5] = 0x2186;
    DestAddress[6] = 0x2187;
    DestAddress[7] = 0x2188;
    DestAddress[8] = 0x2189;
    DestAddress[9] = 0x218A;
    DestAddress[10] = 0x218B;
    
    ES_Event_t ThisEvent;

    MyPriority = Priority;
    // put us into the Initial state
    CurrentState = InitTX;  

   //Initialize UART HW
   // UARTHardwareInit();  
  
    //Disable UART TX Interrupt so that default is RX 
    HWREG(UART5_BASE + UART_O_IM) &= ~(UART_IM_TXIM); 
        
    
 
       //Set Team Color 
    if(HWREG(GPIO_PORTC_BASE+(GPIO_O_DATA+ALL_BITS)) & TEAM_PIN) 
    {// 0 = blue 
      TeamColor = 0x00;
      printf("\r\n WE ARE BLUE");
    }
    else 
    {
      TeamColor = 0x01; 
      printf("\r\n WE ARE RED");
     // 1 = red 
    }
  
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
   
      }
    }
    break;

    case WaitingToTX:        // If current state is state one
    {
      switch (ThisEvent.EventType)
      {
        case ES_BEGIN_TX:  //If event is event one
        {  
                 
           //reset timer (testing only)
           // ES_Timer_InitTimer (TX_ATTEMPT_TIMER,TX_TIME); 
        
              
        //Get SHIPTeamSelect, Initialized to SHIP Ansible 
      //   DestAddressMSB_val = 0x20; // = DestAddressMSB(); //= 0x20; 
      // DestAddressLSB_val = 0x86;//DestAddressLSB(); // = 0x86; 
          uint8_t boat_number = getCurrentBoat(); 
          DestAddressMSB_val = DestAddress[boat_number - 1] >> 8;
          DestAddressLSB_val = DestAddress[boat_number - 1] & 0x00FF; 
        
            //Initialize IDX;
             IDX=0; 
          
            //Set local variable TXPacket_Length
             TXPacket_Length =  Preamble_Length_TX + Data_Length + CHK_SUM;  
 
           //Initialize BytesRemaining = Length of XBee Packet (Preamble (Start + Length + API_ID) + DATA + CHKSUM) 
              BytesRemaining = TXPacket_Length;  
                   
            //set index = 0 
              index = 0; 
              
            //Build the packet to send
              Packet= ThisEvent.EventParam;
              BuildTXPacket(Packet);  
             // printf("\n \r PacketTX = %X", ThisEvent.EventParam);
            //Transmiting this packet 
           
          if((HWREG(UART5_BASE+UART_O_FR)) & ((UART_FR_TXFE)))//If TXFE is set (empty)
          {
         //   printf("\r\nwriting to DR");
            //Write the new data to the UARTDR (first byte)
             HWREG(UART5_BASE+UART_O_DR) = Message_Packet[(IDX)];  
              //decremet Bytes Remaining 
                BytesRemaining--;
              //Increment index 
                index++; 
                IDX++; 
            
//             if((HWREG(UART2_BASE+UART_O_FR)) & ((UART_FR_TXFE))) //if the TXFE is set (still empty)
//             {
//               //Write the second byte to the UART DR 
//             //  printf("\n \r  Message_Packet= %d",  Message_Packet[BytesRemaining]); 
//             //  printf("\n \r  Bytes_Remaining= %d",  BytesRemaining); 
//               HWREG(UART2_BASE+UART_O_DR) |= Message_Packet[BytesRemaining-1];  
//               BytesRemaining--; 
//               index++; 
//             }
//            //Enable TXIM (Note: also enabled in UARTInit)
               HWREG(UART5_BASE + UART_O_IM) &= ~(UART_IM_RXIM);
               HWREG(UART5_BASE + UART_O_IM) |= (UART_IM_TXIM); 

            //Enable Interrupts globally  (also enabled in UARTinit) 
               __enable_irq();
            //return Success 
              //Ready2TX = true;
              
              CurrentState = Transmitting;  //Set next state to transmitting
          }
          else
          {
            //return Error 
            //Ready2TX = false; 
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
    
        //default:
        //  printf("\r\n state machine bug, ansible tx");
  }                 
  // end switch on Current State
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

void AnsibleTXRXISR (void)
{
  //Read the Masked Interrupt Status (UARTMIS)
  
  //If TXMIS Is Set 
  if ((HWREG(UART5_BASE + UART_O_MIS)) & (UART_MIS_TXMIS)) //if bit is set, then an interrupt has occured
  {
  //  printf("\r\nISRbitch");
     //clear the source of the interrupt
      HWREG(UART5_BASE+UART_O_ICR) |= UART_ICR_TXIC;
    //Write the new data to register (UARTDR)
      HWREG(UART5_BASE+UART_O_DR) = Message_Packet[IDX]; 
     // printf("\n \r tx = %x", Message_Packet[(IDX)]); 
      IDX++; 
      
  //    printf("\n \r MessagePacket = %i",Message_Packet[BytesRemaining-1]);
    if (IDX == (TXPacket_Length))
    {
      //clear the source of the interrupt
        HWREG(UART5_BASE+UART_O_ICR) |= UART_ICR_TXIC;
        HWREG(UART5_BASE + UART_O_IM) &= ~(UART_IM_TXIM); //disable interrupt on TX by clearing TXIM 
        HWREG(UART5_BASE + UART_O_IM) |= (UART_IM_RXIM); //enable rx
      //Post the ES_TX_COMPLETE (note: TX complete does not mean that that the Packet has been sent) 
      ES_Event_t ReturnEvent; 
      ReturnEvent.EventType = ES_TX_COMPLETE; 
      PostAnsibleTX (ReturnEvent); 
    } else {
    //decrement BytesRemaining 
      BytesRemaining--; 
    //increment index
      index++;
    //Set TXIC in UARTICR (clear int)
    // HWREG(UART2_BASE + UART_O_ICR) |= UART_ICR_TXIC;  //clear TX interrupt
    }
  }
  else
  {
    // Ready2TX = false; 
    // TODO -- decide to delete or not
    //you are done (not an TX interrupt)
  }
  
  
  //If RXMIS Is Set 
  if ((HWREG(UART5_BASE + UART_O_MIS)) & (UART_MIS_RXMIS)) //if bit is set, then an interrupt has occured
  {
       //clear the source of the interrupt
        HWREG(UART5_BASE+UART_O_ICR) |= UART_ICR_RXIC;
//        receiving = true; 
      //Read the new data  register (UARTDR)
          ES_Event_t ThisEvent; 
          ThisEvent.EventType = BYTE_RECEIVED;  
          ThisEvent.EventParam = HWREG(UART5_BASE+UART_O_DR); 
          PostAnsibleRX(ThisEvent);
       //  printf("\n \r RX = %X", ThisEvent.EventParam); 
   
  }
  else
  {
   // receiving = false; 
    //you are done (not an RX interrupt)
  }  
  
    
}

///PUBLIC FUNCTIONS//

static void UARTHardwareInit(void){
//Setting up the registers for UART-XBee communications
  
    printf("\r\nUART hardware init"); 
  
  //Enable the clock to the UART module using the RCGCUART (run time gating clock control) register
   HWREG(SYSCTL_RCGCUART) |= SYSCTL_RCGCUART_R5;   //UART5 Clock
  
  //Wait for the UART to be ready (PRUART)
   while((HWREG(SYSCTL_PRUART) & SYSCTL_PRUART_R5)!= SYSCTL_PRUART_R5);


  
  //Configure the GPIO pine for in/out/drive-level/drive-type 
     HWREG(GPIO_PORTE_BASE+GPIO_O_DEN) |= (UART5_TX_PIN|UART5_RX_PIN); //setting pins as digital
     HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) &= ~(UART5_RX_PIN); //setting RX as input 
     HWREG(GPIO_PORTE_BASE+GPIO_O_DIR) |= (UART5_TX_PIN); //setting TX as output
  
  //Select the Alternative functions for the UART pins (AFSEL)(AFSEL Table pg.1351)
   HWREG(GPIO_PORTE_BASE+GPIO_O_AFSEL) |= (UART5_TX_PIN|UART5_RX_PIN); 
    
  //Configure the PMCn fields in the GPPIOPCTL (p.689) register to assign the UART pins
    HWREG(GPIO_PORTE_BASE + GPIO_O_PCTL) |= (HWREG(GPIO_PORTE_BASE +  GPIO_O_PCTL) & 0xff00ffff) | 0x00110000; //Write 1 to select U5RX as alternative function and to select U5TX as alt fun 

  //Disable the UART by clearning the UARTEN bits in the UARTCTL register
    HWREG(UART5_BASE + UART_O_CTL) &= ~UART_CTL_UARTEN;
  
  //Write the inter portion of the URTIBRD register (setting baud rate) 
    HWREG(UART5_BASE+UART_O_IBRD) = 0x104;  
  
  //Write the fractional portion of the BRD to the UARTIBRD register
    HWREG(UART5_BASE+UART_O_FBRD) = 0x1B;  

  //Write the desired serial parameters to the UARTLCRH registers to set word length to 8
     HWREG(UART5_BASE + UART_O_LCRH) |= UART_LCRH_WLEN_8;

  //Configure the UART operation using the UARTCTL register 
  //UART Data Registers should be cleared by default (RXE and TXE are already enabled) 
   //Enable the UART by setting the UARTEN bit in the UARTCTL register 
    HWREG(UART5_BASE + UART_O_CTL) |= ( UART_CTL_RXE |UART_CTL_TXE | UART_CTL_UARTEN);

  //Enable UART RX Interrupt (p.924)
   HWREG(UART5_BASE + UART_O_IM) |= (UART_IM_RXIM | UART_IM_TXIM);
  
  //Enable UART TX Interrupt
   //HWREG(UART2_BASE + UART_O_IM) |= (UART_IM_TXIM); 
  
  //Enable NVIC (p.104) UART2 is Interrtupt Number 33, so it is EN1, BIT1 HI (p.141)
    HWREG(NVIC_EN1) |= BIT29HI;
  
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
     // printf("\n \r %x \n \r", Message_Packet[1]); 
     
    // Message_Packet[2] = 0x07; //Data_Length = Preamble_Length + SizeofArray LSB 
    //  printf("\n \r %x \n \r", Message_Packet[2]); 
     
     Message_Packet[3] = API_Identifier;  //API_ID
     // printf("\n \r %x \n \r", Message_Packet[3]);
    
      Message_Packet[4] = 0x01;  //Frame_ID (**arbitrary for the moment)
   //   printf("\n \r %x \n \r", Message_Packet[4]); 
    
      Message_Packet[5] = DestAddressMSB_val;   //Destination Address MSB
    // printf("\n \r %x \n \r", Message_Packet[5]); 
    
      Message_Packet[6] = DestAddressLSB_val;   //Destination Address LSB
    //  printf("\n \r %x \n \r", Message_Packet[6]); 
    
      Message_Packet[7] = Options;  //Options Byte 
    // printf("\n \r %x \n \r", Message_Packet[7]); 
    
  }
  
  
static void BuildTXPacket(uint8_t Packet)
{
  switch(Packet)
  { 
          
    case REQ_2_PAIR: 
    {
      //build the preamble 
       BuildPreamble(); 
      //set index to length of preamble
      index = Preamble_Length_TX; 
      //Add RF Data packet header corresponding to REQ_2_PAIR
      Message_Packet[index] = REQ_2_PAIR; 
      //increment index
      index++; 
      //decrement BytesRemaining 
       BytesRemaining--;
      //Red or Blue state 
       Message_Packet[index] = TeamColor; 
      //increment index
       index++; 
      //decrement BytesRemaining 
       BytesRemaining--;
      //ModifyDataLength()
        Message_Packet[2] = 0x07; //Data_Length = Preamble_Length + SizeofArray LSB 
        TXPacket_Length = 11; 
      //calculate CheckSum (); 
      //store CheckSum as the next byte of Message_Packet
      Message_Packet[index] = CheckSum(); 

    }
    break;
    
    case CTRL: 
    {
      //index = 0; 
      //Build Preamble 
      BuildPreamble();
      //set index to length of preamble
       index = Preamble_Length_TX; 
      //Add RF Data packet corresponding to REQ_2_PAIR
      Message_Packet[8] = CTRL; 
      //increment index
        index++; 
      //decrement BytesRemaining 
       BytesRemaining--;
      
      //Forward/Back
       Message_Packet[index] =  getThrottle(); 
      //increment index
        index++; 
      //decrement BytesRemaining 
       BytesRemaining--;
      
      //Left/Right

       Message_Packet[index] = getSteering(); 
      //increment index
        index++; 
      //decrement BytesRemaining 
       BytesRemaining--;
      
     
       //Turret Yaw

       Message_Packet[index] = getYaw(); 
       //increment index
        index++; 
      //decrement BytesRemaining 
       BytesRemaining--;
       
       //Turret Yaw

       Message_Packet[index] = getPitch(); 
         //increment index
       index++; 
      //decrement BytesRemaining 
       BytesRemaining--;
       
       Message_Packet[index] = getControl(); 
         //increment index
       index++; 
      //decrement BytesRemaining 
       BytesRemaining--;
      
       //ModifyDataLength()
        Message_Packet[2] = 0x0B; //Data_Length = Preamble_Length + SizeofArray LSB 
        TXPacket_Length = 0x0F; 
      
      //calculate CheckSum (); 
      //store CheckSum as the next byte of Message_Packet
      Message_Packet[index] = CheckSum(); 

      
    }
    break; 

    case STATUS: 
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
    }
    break; 
  
  }
  return;
}
static uint8_t CheckSum(void)
{
  uint8_t summation = 0; 
  uint8_t FrameDataStart = 3; //check sum is the 0xFF - (Sum of FrameData)
  uint8_t computed_chksum = 0; 
  uint8_t i; 
  
  for(i= FrameDataStart; i<(TXPacket_Length-1); i++)
  {
    summation += Message_Packet[i];
  }
  
  computed_chksum =(0xFF-summation); 
  
  return computed_chksum; 
}


uint8_t getTeamColor (void)
{
    return TeamColor; 
}
