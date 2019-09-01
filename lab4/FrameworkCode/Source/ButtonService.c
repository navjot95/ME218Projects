/****************************************************************************
 Module
   MorseElementService.c

 Revision
   1.0.1

 Description
  Morse Code Sample Pseudo Code Using the Gen2.x Event Framework
  Rev 14 10/21/15
  Pseudo-code for the Morse Elements module (a service that implements a state machine)
  Data private to the module: MyPriority, CurrentState, TimeOfLastRise, TimeOfLastFall, LengthOfDot, FirstDelta, LastInputState

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/19/16 13:24 jec      added comments about where to add deferal and recall
 01/12/15 21:47 jec      converted to LCD module for lab 3
 11/02/13 17:21 jec      added exercise of the event deferral/recall module
 08/05/13 20:33 jec      converted to test harness service
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for the framework and this service
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"
#include "ButtonService.h"
#include "MorseElementService.h"
#include "DecodeMorseService.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"


/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define ONE_SEC 976
#define HALF_SEC (ONE_SEC/2)
#define TWO_SEC (ONE_SEC*2)
#define FIVE_SEC (ONE_SEC*5)
#define DEBOUNCE_TIME 976
#define DEBOUNCE_TIMER 1

#define ALL_BITS (0xff<<2)

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
static ES_Event DeferralQueue[3+1];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMorseElementService

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
bool InitButtonService ( uint8_t Priority )
{ 
  MyPriority = Priority;
  
  HWREG(SYSCTL_RCGCGPIO) |= BIT1HI; //enable Port B
  //wait for Port B to be ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1) 
  {
  } 
  //Initialize bit 4 on Port B to be a digital bit
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT4HI); 
  //Initialize bit 4 on Port B to be an input
  HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) &= (BIT4LO);
  
  //Sample port line and use it to initialize the LastButtonState var
  LastButtonState = HWREG(GPIO_PORTB_BASE+ ALL_BITS) & BIT4HI; 
  
  CurrentState = Debouncing;

  //start debounce timer, posts to ButtonDebounceSM
  ES_Timer_InitTimer(DEBOUNCE_TIMER, DEBOUNCE_TIME);

  return true; 
}

/****************************************************************************
 Function
     PostLCDService

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
bool PostButtonService( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunLCDService

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
ES_Event RunButtonService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
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

        ES_Event NewEvent; 
        NewEvent.EventType = DB_BUTTON_UP; 
        ES_PostList01(ThisEvent); 
        //PostMorseElementService(NewEvent); 
        //PostDecodeMorseService(NewEvent);  
      }
      if(ThisEvent.EventType == ES_BUTTON_DOWN)
      { 
        ES_Timer_InitTimer(DEBOUNCE_TIMER, DEBOUNCE_TIME); //check this, not sure if right
        CurrentState = Debouncing;

        //DISTRIBUTION LISTS POSTS TO MORESEELEMTNS AND DECODEMORSE QUEUES?
        ES_Event NewEvent; 
        NewEvent.EventType = DB_BUTTON_DOWN;
        //ES_PostList01(ThisEvent);
        PostMorseElementService(NewEvent); 
        PostDecodeMorseService(NewEvent); 
      }
      break;
  }
  return ReturnEvent;
}


bool CheckButtonEvents (void)
{
  bool returnVal = false; 
  uint8_t CurrentButtonState = (HWREG(GPIO_PORTB_BASE+ ALL_BITS) & BIT4HI);

  if(CurrentButtonState != LastButtonState)
  {
    returnVal = true; 
    ES_Event ThisEvent; 
    //assuming 0 means down, and 1 means not pressed 
    if(CurrentButtonState) //if current state is high, post button up even, else post button down event
    {
      ThisEvent.EventType = ES_BUTTON_UP; 
      PostButtonService(ThisEvent); 
    }
    else 
    {
      ThisEvent.EventType = ES_BUTTON_DOWN; 
      PostButtonService(ThisEvent);
      CurrentState = Ready2Sample; 
      //printf("Recalibration button pressed\n\r"); 
    }
  }
  LastButtonState = CurrentButtonState; 
  return returnVal; 
}

/***************************************************************************
 private functions
 ***************************************************************************/


/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

