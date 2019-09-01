/****************************************************************************
 Module
   SPIService.c

 Revision
   1.0.1

 Description
   This is a file for implementing a simple service under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "SPIService.h"
#include "MotorService.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"
#include "inc/hw_gpio.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "termio.h"

/*----------------------------- Module Defines ----------------------------*/

#define BitsPerNibble 4
#define TicksPerMS 40000
#define QueryTime 100
#define PreScaler 50

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

void InitSPI(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static const uint8_t Query = 0xAA;
static const uint16_t CommandReady = 0xFF;
static bool NewCommand = 0;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitSPIService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitSPIService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
  
  
  TERMIO_Init();
//  InitPeriodicInt();
  InitSPI();
  
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
     PostSPIService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostSPIService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunSPIService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunSPIService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  /********************************************
   in here you write your service code
   *******************************************/
  
  // Query the command generator
  HWREG(SSI0_BASE+SSI_O_DR) = Query;
  // Enable the NVIC interrupt for the SSI when starting to transmit (vector #23, Interrupt #7)
  HWREG(NVIC_EN0) |= BIT7HI;
  
  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/

void CommGenISR(void) {
  // Disable the NVIC interrupt for the SSI when transmit finished (vector #23, Interrupt #7)
  HWREG(NVIC_EN0) &= BIT7LO;
  
  // Read the comma  nd generator
  uint16_t Command = HWREG(SSI0_BASE+SSI_O_DR);
  
  // If we receive 0xFF, set NewCommand to be ready for next command
  if (Command == CommandReady) {
    NewCommand = 1;
  // If not 0xFF and NewCommand ready, post Command to MotorService
  } else if (NewCommand == 1) {
    ES_Event_t CommandEvent;
    CommandEvent.EventType = COMMAND_RECEIVED;
    CommandEvent.EventParam = (uint8_t)Command;
//    printf("New comm %x\n\r", Command);
    PostMotorService(CommandEvent);
    NewCommand = 0;
  }
  ES_Timer_InitTimer(COMM_TIMER, QueryTime);
}



void InitSPI(void) {
  //Enable the clock to the GPIO port 
  HWREG(SYSCTL_RCGCGPIO) |= BIT0HI;
  // Enable the clock to SSI module 
  HWREG(SYSCTL_RCGCSSI) |= BIT0HI;
  // Wait for the GPIO port to be ready 
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R0) != SYSCTL_PRGPIO_R0) {
  }
  // Program the GPIO to use the alternate functions on the SSI pins 
  HWREG(GPIO_PORTA_BASE+GPIO_O_AFSEL) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI);
  // Set mux position in GPIOPCTL to select the SSI use of the pins 
  HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) & 0xff0000ff) \
        + (2 << (5*BitsPerNibble)) + (2 << (4*BitsPerNibble)) + \
        (2 << (3*BitsPerNibble)) + (2 << (2*BitsPerNibble));
  // Program the port lines for digital I/O 
  HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI);
  // Program the required data directions on the port lines 
  HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= ((BIT2HI | BIT3HI| BIT5HI) & BIT4LO);
  // If using SPI mode 3, program the pull-up on the clock line 
  HWREG(GPIO_PORTA_BASE+GPIO_O_PUR) |= BIT2HI;
  // Wait for the SSI0 to be ready (PRETTY SURE THIS IS CORRECT??)
  while ((HWREG(SYSCTL_PRSSI) & SYSCTL_PRSSI_R0) != SYSCTL_PRSSI_R0) {
  }
  // Make sure that the SSI is disabled before programming mode bits 
  HWREG(SSI0_BASE+SSI_O_CR1) &= BIT1LO;
  // Select master mode (MS) & TXRIS indicating End of Transmit (EOT) 
  HWREG(SSI0_BASE+SSI_O_CR1) |= (SSI_CR1_EOT & (~SSI_CR1_MS));
  // Configure the SSI clock source to the system clock 
  HWREG(SSI0_BASE+SSI_O_CC) = SSI_CC_CS_SYSPLL;
  // Configure the clock pre-scaler: max frequency 961kHz
  // SSInClk = SysClk / (CPSDVSR *(1+SCR)), we want CPSDVSR*(1+SCR) > 42
  HWREG(SSI0_BASE+SSI_O_CPSR) = PreScaler;
  // Configure clock rate (SCR), phase & polarity (SPH, SPO), mode (FRF), data size (DSS) 
  HWREG(SSI0_BASE+SSI_O_CR0) |= (SSI_CR0_SPH | SSI_CR0_SPO | SSI_CR0_DSS_8); // +7 for 8-bit data size
  // Locally enable interrupts (TXIM in SSIIM) 
  HWREG(SSI0_BASE+SSI_O_IM) |= SSI_IM_TXIM;
  // Make sure that the SSI is enabled for operation 
  HWREG(SSI0_BASE+SSI_O_CR1) |= SSI_CR1_SSE;
  // Query the command generator
  HWREG(SSI0_BASE+SSI_O_DR) = Query;
  // Enable the NVIC interrupt for the SSI when starting to transmit (vector #23, Interrupt #7)
  HWREG(NVIC_EN0) |= BIT7HI;
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

