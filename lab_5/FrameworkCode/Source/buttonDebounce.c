/****************************************************************************
 Module
   ButtonDebounce.c

 Revision
   1.0.1

 Description
  Service that responds to the button being pressed
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for the framework and this service
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"

#include "StepperMotorService.h"
#include "buttonDebounce.h" 

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define ONE_SEC 976
#define HALF_SEC (ONE_SEC/2)
#define TWO_SEC (ONE_SEC*2)
#define FIVE_SEC (ONE_SEC*5)
#define DEBOUNCE_TIME 40
#define DEBOUNCE_TIMER 3


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/ 


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint8_t LastButtonState;
static ButtonState_t CurrentState; 


// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
//static ES_Event_t DeferralQueue[3+1];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitButtonDebounce

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and initializes the pin that connects to the
     hardware
 Notes

 Author
     Navjot Singh
****************************************************************************/
bool InitButtonDebounce ( uint8_t Priority )
{ 
  MyPriority = Priority;
  
  HWREG(SYSCTL_RCGCGPIO) |= BIT5HI; //enable Port F
  //wait for Port F to be ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R5) != SYSCTL_PRGPIO_R5) 
  {
  } 
  //Initialize bit 4 on Port F to be a digital bit
  HWREG(GPIO_PORTF_BASE+GPIO_O_DEN) |= (BIT4HI); 
  //Initialize bit 4 on Port F to be an input
  HWREG(GPIO_PORTF_BASE+GPIO_O_DIR) &= (BIT4LO); 
  
  //enable pull up in button on PF4
  HWREG(GPIO_PORTF_BASE+GPIO_O_PUR) |= (BIT4HI);
  
  //Sample port line and use it to initialize the LastButtonState var
  LastButtonState = HWREG(GPIO_PORTF_BASE+ ALL_BITS) & BIT4HI; 
  
  CurrentState = Debouncing;

  //start debounce timer, posts to ButtonDebounceSM
  ES_Timer_InitTimer(DEBOUNCE_TIMER, DEBOUNCE_TIME);

  return true; 
}

/****************************************************************************
 Function
     PostButtonDebounce

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     Navjot Singh
****************************************************************************/
bool PostButtonDebounce( ES_Event_t ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunButtonDebounce

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   Uses a state machine to respond to the button being pressed 
   and sending the appropriate event StepperMotorService

 Notes
   
 Author
   Navjot Singh
****************************************************************************/
ES_Event_t RunButtonDebounce( ES_Event_t ThisEvent )
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
   
  switch (CurrentState){
    case Debouncing: 
      if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == DEBOUNCE_TIMER)){
        // move to the Initializing state
        CurrentState = Ready2Sample; 
      }
      break;
      
    case Ready2Sample :
      if(ThisEvent.EventType == ES_BUTTON_UP)
      {
        //start debouncing timer
        ES_Timer_InitTimer(DEBOUNCE_TIMER, DEBOUNCE_TIME);
        CurrentState = Debouncing; 
        
      }
      if(ThisEvent.EventType == ES_BUTTON_DOWN)
      { 
        ES_Timer_InitTimer(DEBOUNCE_TIMER, DEBOUNCE_TIME);
        CurrentState = Debouncing;

        ES_Event_t NewEvent; 
        printf("\n\rbutton service: posting dbbutton down");
        NewEvent.EventType = DB_BUTTON_DOWN; 
        PostStepperMotorService(NewEvent);  
      }
      break;
  }
  return ReturnEvent;
}





/****************************************************************************
 Function
    CheckButtonEvents

 Parameters
   None

 Returns
   bool

 Description
   Event checker for this service that sends an event when the button has 
   been pressed. 
 Notes
   
 Author
   Navjot Singh
****************************************************************************/
bool CheckButtonEvents (void)
{
  bool returnVal = false; 
  uint8_t CurrentButtonState = (HWREG(GPIO_PORTF_BASE+ ALL_BITS) & BIT4HI);
  //printf("button val: %d\n\r", CurrentButtonState); 

  if(CurrentButtonState != LastButtonState)
  {
    returnVal = true; 
    ES_Event_t ThisEvent; 
    //0 means down, and 1 means not pressed 
    //if current state is high, post button up even, else post button down event
    if(CurrentButtonState)
    {
      ThisEvent.EventType = ES_BUTTON_UP; 
      PostButtonDebounce(ThisEvent); 
    }
    else 
    {
      ThisEvent.EventType = ES_BUTTON_DOWN; 
      PostButtonDebounce(ThisEvent);
    }
  }
  LastButtonState = CurrentButtonState; 
  return returnVal; 
}

/*------------------------------ End of file ------------------------------*/

