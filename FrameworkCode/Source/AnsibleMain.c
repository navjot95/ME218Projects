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

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static TemplateState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateFSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitAnsibleMain(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  
  UARTHardwareInit(); //Initialize HW for UART 
  
  // put us into the Initial PseudoState
  CurrentState = InitPState;
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
     PostTemplateFSM

 Parameters
     EF_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostTemplateFSM(ES_Event_t ThisEvent)
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
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunTemplateFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case InitPState:        // If current state is initial Psedudo State
    {
      if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
      {
        // this is where you would put any actions associated with the
        // transition from the initial pseudo-state into the actual
        // initial state

        // now put the machine into the actual initial state
        CurrentState = UnlockWaiting;
      }
    }
    break;

    case UnlockWaiting:        // If current state is state one
    {
      switch (ThisEvent.EventType)
      {
        case ES_LOCK:  //If event is event one

        {   // Execute action function for state one : event one
          CurrentState = Locked;  //Decide what the next state will be
        }
        break;

        // repeat cases as required for relevant events
        default:
          ;
      }  // end switch on CurrentEvent
    }
    break;
    // repeat state pattern as required for other states
    default:
      ;
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
TemplateState_t QueryTemplateFSM(void)
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
    HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xfffffff0)+(1); //Write 1 to select U2RX as alternative function 
    HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTD_BASE+GPIO_O_PCTL) & 0xffffff0f)+(1<<(1* BitsPerNibble)) ; //Write 1 to select U2TX as alt fun

  //Disable the UART by clearning the UARTEN bits in the UARTCTL register
    //UART Data Registers are cleared by default (RXE and TXE are already enabled) **
  
  //Write the inter portion of the URTIBRD register (setting baud rate) 
    HWREG(UART2_BASE+UART_O_IBRD) = HWREG(UART2_BASE+UART_O_IBRD) + 0x15;  //writing 21 in hex
  
  //Write the fractional portion of the BRD to the UARTIBRD register
    HWREG(UART2_BASE+UART_O_FBRD) = HWREG(UART2_BASE+UART_O_FBRD) + 0x2D;  //writing 21 in hex; //writing 45 in hex 

  //Write the desired serial parameters to the UARTLCRH registers to set word length to 8
     HWREG (UART2_BASE + UART_O_LCRH) = HWREG(UART2_BASE + UART_O_LCRH) + (UART_LCRH_WLEN_8); 

  //Configure the UART operation using the UARTCTL register 
     //UART Data Registers are cleared by default (RXE and TXE are already enabled) **
  
  //Enable the UART by setting the UARTEN bit in the UARTCTL register 
    HWREG(UART2_BASE + UART_O_IM) |= (UART_CTL_UARTEN); 
        
  }