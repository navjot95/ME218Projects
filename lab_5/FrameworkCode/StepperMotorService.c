/****************************************************************************
 Module
   StepperMotorService.c

 Revision
   1.0.1

 Description
   Service that moves a stepper motor according to full step, wave drive, 
   half step, or micro step

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

#include "ADMulti.h"
#include "PWM16Tiva.h" 
#include "ADService.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define ONE_SEC 1000
#define HALF_SEC (ONE_SEC / 2)
#define TWO_SEC (ONE_SEC * 2)
#define FIVE_SEC (ONE_SEC * 5)

#define STEPPER_TIMER 1
#define STEP_TIME 500 //in ms 

#define CHANNEL_Q1Q4 12
#define CHANNEL_Q2Q3 13
#define CHANNEL_Q5Q8 14
#define CHANNEL_Q6Q7 5

#define STEPPER_FQ 500
#define GROUP6 6
#define GROUP7 7
#define GROUP2 2


// #define ALL_BITS (0xff<<2)   Moved to ES_Port.h
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for ovehead
//static ES_Event_t DeferralQueue[3 + 1];
// variable for position for stepper motor 
static uint8_t posCounter = 0; //set this to 1 for Wave Drive, 0 for half step and full drive 

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitStepperMotorService

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
  
  HWREG(SYSCTL_RCGCGPIO) |= BIT5HI; //enable Port F
  //wait for Port F to be ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R5) != SYSCTL_PRGPIO_R5) 
  {
  } 
  //Initialize bit 0 on Port F to be a digital bit
  HWREG(GPIO_PORTF_BASE+GPIO_O_DEN) |= (BIT0HI); 
    //Initialize bit 1 on Port F to be a digital bit
  HWREG(GPIO_PORTF_BASE+GPIO_O_DEN) |= (BIT1HI); 
    //Initialize bit 2 on Port F to be a digital bit
  HWREG(GPIO_PORTF_BASE+GPIO_O_DEN) |= (BIT2HI); 
    //Initialize bit 3 on Port F to be a digital bit
  HWREG(GPIO_PORTF_BASE+GPIO_O_DEN) |= (BIT3HI); 
    
  
  //Initialize bit 0 on Port B to be an output
  HWREG(GPIO_PORTF_BASE+GPIO_O_DIR) |= (BIT0HI); 
  //Initialize bit 1 on Port B to be an output
  HWREG(GPIO_PORTF_BASE+GPIO_O_DIR) |= (BIT1HI); 
  //Initialize bit 2 on Port B to be an output
  HWREG(GPIO_PORTF_BASE+GPIO_O_DIR) |= (BIT2HI); 
  //Initialize bit 3 on Port B to be an output
  HWREG(GPIO_PORTF_BASE+GPIO_O_DIR) |= (BIT3HI);  
  
  
  //PWM_TIVA_Init(15);  
  
  
  printf("Pins initialized \n\r"); 

  return true; 
}

/****************************************************************************
 Function
     PostStepperMotorService

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
    RunStepperMotorService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   Updates the state of each stepper motor coil according to if the mode 
    is Full Drive, Wave Drive, Half Step, or Micro Step 
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunStepperMotorService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors  
  static bool motorDir = true; 
  
  static bool triggered = false; 
  uint32_t stepTime = getStepTime();  
  
  const uint8_t seqQ1Q4[] = {1,1,1,0,0,0,0,0}; 
  const uint8_t seqQ2Q3[] = {0,0,0,0,1,1,1,0};
  const uint8_t seqQ5Q8[] = {1,0,0,0,0,0,1,1};
  const uint8_t seqQ6Q7[] = {0,0,1,1,1,0,0,0};
  
  /*
  //For micro-step
  const uint8_t seqQ1Q4[] = {71, 92, 100, 92, 71, 38, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38}; 
  const uint8_t seqQ2Q3[] = {0, 0, 0, 0, 0, 0, 0, 38, 71, 92, 100, 92, 71, 38, 0, 0};
  const uint8_t seqQ5Q8[] = {71, 38, 0, 0, 0, 0, 0, 0, 0, 0, 0, 38, 71, 92, 100, 92};
  const uint8_t seqQ6Q7[] = {0, 0, 0, 38, 71, 92, 100, 92, 71, 38, 0, 0, 0, 0, 0, 0};
  */
  
  if(ThisEvent.EventType == DB_BUTTON_DOWN){
     motorDir = !motorDir; 
     //printf("Button pressed\n\r"); 
  }
  
  
  if(ThisEvent.EventType == ES_TIMEOUT){
  
    //update state of each coil for Full, Wave, and half step   
    if(seqQ1Q4[posCounter%16])  
        HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT0HI;
    else
        HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
    
    if(seqQ2Q3[posCounter%16])  
        HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT1HI;
    else
        HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
        
    if(seqQ5Q8[posCounter%16])  
        HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT2HI;
    else
        HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT2LO;
        
    if(seqQ6Q7[posCounter%16])  
        HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT3HI;
    else
        HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT3LO;
  
   /*
    This is for micro-step 
    PWM_TIVA_SetFreq(STEPPER_FQ, GROUP6);
    PWM_TIVA_SetFreq(STEPPER_FQ, GROUP7);
    PWM_TIVA_SetFreq(STEPPER_FQ, GROUP2);
    
    PWM_TIVA_SetDuty(seqQ1Q4[posCounter%16],CHANNEL_Q1Q4); 
    PWM_TIVA_SetDuty(seqQ2Q3[posCounter%16],CHANNEL_Q2Q3);
    PWM_TIVA_SetDuty(seqQ5Q8[posCounter%16],CHANNEL_Q5Q8);
    PWM_TIVA_SetDuty(seqQ6Q7[posCounter%16],CHANNEL_Q6Q7);
    */
    
   
    //for Full Step and Wave Drive, would increment by 2
    if(motorDir)
      posCounter += 1; 
    else
      posCounter -= 1;  
    
    
    /*
    //This chuck of code was used to determine the max step rate.
    //It moves the roto in one direction for 200 steps and then back 
    //200 steps and then stops. 
    if((posCounter < 200) && !triggered){
      posCounter += 2;
    }
    else {
      triggered = true; 
    }
    
    if(posCounter <= 1)
        posCounter = 3; //so it stops at home, when rotating back
    
    if(triggered)
      posCounter -= 2;  
    */
      
    ES_Timer_InitTimer(STEPPER_TIMER, stepTime);
  }
  
  
  
  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/



/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
