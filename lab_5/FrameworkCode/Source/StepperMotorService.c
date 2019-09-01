/****************************************************************************
 Module
   TestHarnessService0.c

 Revision
   1.0.1

 Description
   This is the first service for the Test Harness under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/26/17 18:26 jec     moves definition of ALL_BITS to ES_Port.h
 10/19/17 21:28 jec     meaningless change to test updating
 10/19/17 18:42 jec     removed referennces to driverlib and programmed the
                        ports directly
 08/21/17 21:44 jec     modified LED blink routine to only modify bit 3 so that
                        I can test the new new framework debugging lines on PF1-2
 08/16/17 14:13 jec      corrected ONE_SEC constant to match Tiva tick rate
 11/02/13 17:21 jec      added exercise of the event deferral/recall module
 08/05/13 20:33 jec      converted to test harness service
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "StepperMotorService.h"

// Hardware
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define ONE_SEC 1000
#define HALF_SEC (ONE_SEC / 2)
#define TWO_SEC (ONE_SEC * 2)
#define FIVE_SEC (ONE_SEC * 5)

#define STEPPER_TIMER 1
#define STEP_TIME 1000 //in ms 

// #define ALL_BITS (0xff<<2)   Moved to ES_Port.h
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

static void InitLED(void);
static void BlinkLED(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for ovehead
static ES_Event_t DeferralQueue[3 + 1];
// variable for position for stepper motor 
static uint8_t posCounter = 0; 

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTestHarnessService0

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
bool InitStepperMotorService(uint8_t Priority)
{
  MyPriority = Priority;
  
  HWREG(SYSCTL_RCGCGPIO) |= BIT1HI; //enable Port B
  //wait for Port B to be ready
  /*while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1) 
  {
  } */
  //Initialize bit 4 on Port B to be a digital bit
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT4HI); 
    //Initialize bit 5 on Port B to be a digital bit
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT5HI); 
    //Initialize bit 6 on Port B to be a digital bit
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT6HI); 
    //Initialize bit 7 on Port B to be a digital bit
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT7HI); 
    
    
  //Initialize bit 4 on Port B to be an input
  HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) &= (BIT4LO);

  //start stepper timer, posts to runStepperMotorService
  //ES_Timer_InitTimer(STEPPER_TIMER, STEP_TIME);
  //send event to postStepperMotorService to start the run
  ES_Event_t ThisEvent; 
  ThisEvent.EventType = ES_TIMEOUT; 
  ThisEvent.EventParam = 1; 
  ES_PostToService(MyPriority, ThisEvent);

  return true; 
}

/****************************************************************************
 Function
     PostTestHarnessService0

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
bool PostStepperMotorService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTestHarnessService0

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunStepperMotorService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors  
    
  const uint8_t seqQ1Q4[8] = {1,1,1,0,0,0,0,0}; 
  const uint8_t seqQ2Q3[8] = {0,0,0,0,1,1,1,0};
  const uint8_t seqQ5Q8[8] = {1,0,0,0,0,0,1,1};
  const uint8_t seqQ6Q7[8] = {0,0,1,1,1,0,0,0};

  if(ThisEvent.EventType == ES_TIMEOUT){
    //update state of each coil   
    if(seqQ1Q4[posCounter])  
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT4HI;
    else
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT4LO;
    
    if(seqQ2Q3[posCounter])  
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT5HI;
    else
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT5LO;
        
    if(seqQ5Q8[posCounter])  
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT6HI;
    else
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT6LO;
        
    if(seqQ6Q7[posCounter])  
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT7HI;
    else
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT7LO;
    
    posCounter++; 
    ES_Timer_InitTimer(STEPPER_TIMER, STEP_TIME);
    printf("ES_TIMEOUT received from Timer %d in Service %d\r\n", ThisEvent.EventParam, MyPriority);
  }
  
  
  
  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/



/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

