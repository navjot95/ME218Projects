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

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void BuildTXPacket(void);  

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

   //Initialize UART HW
    UARTHardwareInit();  
  
  //Disable UART TX Interrupt so that default is RX 
   HWREG(UART2_BASE + UART_O_IM) &= ~(UART_IM_TXIM); 
  
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  
  printf("\n \r finished init"); 
  
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
      printf("\n \r entered transmit"); 
      if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
      {
        //Set the initial state 
        CurrentState = WaitingToTX;
          
        //enable timer 
        ES_Timer_InitTimer (TX_ATTEMPT_TIMER,TX_TIME); 
        printf("\n \r timer enabled"); 
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
          if((HWREG(UART2_BASE+UART_O_FR)) & ((UART_FR_TXFE)))//If TXFE is set (empty)
          {
            printf("\n \r fifo is empty"); 
            
            //reset timer
            ES_Timer_InitTimer (TX_ATTEMPT_TIMER,TX_TIME); 
            printf( "\n \r timer has been reset"); 
            
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
AnsibleTXState_t QueryAnsibleTransmit(void)
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
  if ((HWREG(UART2_BASE + UART_O_MIS)) & (UART_MIS_TXMIS)) //if bit is set, then an interrupt has occured
  {
    printf("\n \r bit set");
    //Write the new data to register (UARTDR)
      HWREG(UART2_BASE+UART_O_DR) = 0xAF; 
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
  
  //HWREG(UART2_BASE + UART_O_IM) &= ~(UART_IM_TXIM);
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
    HWREG(UART2_BASE+UART_O_IBRD) = HWREG(UART2_BASE+UART_O_IBRD) + 0x15;  //writing 21 in hex
  
  //Write the fractional portion of the BRD to the UARTIBRD register
    HWREG(UART2_BASE+UART_O_FBRD) = HWREG(UART2_BASE+UART_O_FBRD) + 0x2D;  //writing 21 in hex; //writing 45 in hex 

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

  
static void BuildTXPacket(void)
{
  //Building a packet to send
    printf("\n \r startting packet build"); 
  //Write the new data to the UARTDR (first byte)
   HWREG(UART2_BASE+UART_O_DR) = 0xAF; 
  
}
