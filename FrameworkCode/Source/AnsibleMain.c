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
first pass      s koppaka 
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
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

/*----------------------------- Module Defines ----------------------------*/
#define BitsPerNibble 4
#define UART2_RX_PIN GPIO_PIN_6
#define UART2_TX_PIN GPIO_PIN_7
#define ATTEMPT_TIME 200 //200ms
#define PAIRING_TIME 1000 //1 sec time 

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static AnsibleMainState_t CurrentState;


//Module Level Variables 
bool pair; 

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

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
     first pass sk
     
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
    first pass sk
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
   first pass sk 
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
//         bool pair = false; 
        //Set next state to WaitingforPair
        NextState = WaitingForPair;
      }
    }
    break; //break out of state 

    case WaitingForPair:        // If current state is state one
    {
        if(ThisEvent.EventType == ES_PAIRBUTTONPRESSED)  // only respond if the button is pressed to pair to a specific team 
        {  
        //set ship address
        //send packet to SHIP (0x01)
        //Start attempt time to 200ms to keep trying at a rate of 5 Hz 
          ES_Timer_InitTimer (PAIR_ATTEMPT_TIMER,ATTEMPT_TIME); 
          
        //Start Pairing Timer to 1sec 
          ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); 
          
          NextState = WaitingForPairResp;  //Decide what the next state will be
        }
    }
    break; //break out of state 
    
    case WaitingForPairResp:        // If current state is state one
    {
      switch (ThisEvent.EventType)
      {
        case ES_TIMEOUT:  //if this event is a timeout 
        {  
          if (ThisEvent.EventParam == PAIR_ATTEMPT_TIMER)
          {
             ES_Timer_InitTimer (PAIR_ATTEMPT_TIMER,ATTEMPT_TIME); //reset timer
             //Self transition and send packet (0x01) again to the SHIP
             //NextState = CurrentState; to self transition 
          }
          if (ThisEvent.EventParam == PAIR_TIMEOUT_TIMER)
          {
            ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); 
            //set currentstate to WaitingForPair
            NextState = WaitingForPair; 
          }
        }
        break;
        case ES_CONNECTIONEST:  //if this event is a timeout 
        {  
         //local variable paired = true
          //Start Pairing Timer (1sec)
          ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); //reset timer
          //Start Attempt Timer (200ms)
          ES_Timer_InitTimer (PAIR_ATTEMPT_TIMER,ATTEMPT_TIME); //reset timer 
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
         //send 0x01 packet
         //start 200ms timer
           ES_Timer_InitTimer (PAIR_ATTEMPT_TIMER,ATTEMPT_TIME); //reset timer 
         //start 1s timer
           ES_Timer_InitTimer (PAIR_TIMEOUT_TIMER,PAIRING_TIME); //reset timer
         //Paired = false 
        // now put the machine into the actual initial state
          NextState = WaitingForPairResp;  //Decide what the next state will be
        }
        break; 
        case ES_TIMEOUT:  //if connection established event is posted
        {  
          if (ThisEvent.EventParam == PAIR_ATTEMPT_TIMER)
          {
            //send CNTRL packet
            //reset PAIR_ATTEMPT_TIMER
            NextState = CommunicatingSHIP;  //Decide what the next state will be
          }
           if (ThisEvent.EventParam == PAIR_TIMEOUT_TIMER)
          {
          //local bool paired = false
           NextState = WaitingForPair;  //Decide what the next state will be
          }
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
AnsibleMainState_t QueryTemplateFSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/



/***************************************************************************
 public functions
 ***************************************************************************/
static void UARTHardwareInit(void){
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
    HWREG(UART2_BASE + UART_O_IM) |= (UART_IM_RXIM); 
  
  //Enable UART TX Interrupt
   HWREG(UART2_BASE + UART_O_IM) |= (UART_IM_TXIM); 
  
  //Enable NVIC (p.104) UART2 is Interrtupt Number 33, so it is EN1, BIT1 HI (p.141)
    HWREG(NVIC_EN1) |= BIT1HI;
  
  //Enable Interrupts Globally 
  __enable_irq();

  //
  }
