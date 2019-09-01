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
#include "MorseElementService.h"
#include "DecodeMorseService.h"
#include "ButtonService.h"

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
#define ALL_BITS (0xff<<2)

#define MORSE_TIMER 0
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static void TestCalibration(void); 
static void CharacterizeSpace(void); 
static void CharacterizePulse(void); 


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint16_t TimeOfLastRise = 0; 
static uint16_t TimeOfLastFall = 0; 
static uint16_t LengthOfDot = 0; 
static uint16_t FirstDelta = 0; 
static uint8_t LastInputState = 0; 

static MorseElementState_t CurrentState = InitMorseElements;

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
bool InitMorseElementService ( uint8_t Priority )
{
  ES_Event ThisEvent;
  
  MyPriority = Priority;
  
  // set up port B by enabling the peripheral clock, waiting for the 
  // peripheral to be ready and setting the direction
  // of PB0 to input
  HWREG(SYSCTL_RCGCGPIO) |= BIT1HI; //enable Port B
  //wait for Port B to be ready
  while ((HWREG(SYSCTL_PRGPIO) & BIT1HI) != BIT1HI) 
  {
  } 
  //Initialize bit 3 on Port B to be a digital bit
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT6HI); 
  //Initialize bit 3 on Port B to be an input
  HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) &= (BIT6LO);
  
  //Sample port line and use it to initialize the LastInputState var
  LastInputState = (HWREG(GPIO_PORTB_BASE+ ALL_BITS) & BIT6HI); 
  FirstDelta = 0; 
  //Put us into the initial pseudo-state to set up for the initial transition
  CurrentState = InitMorseElements;
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService( MyPriority, ThisEvent) == true)
  {  
    return true;
  }else
  {
      return false;
  }
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
bool PostMorseElementService( ES_Event ThisEvent )
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
ES_Event RunMorseElementService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  MorseElementState_t NextState; 
  NextState = CurrentState; 
  switch (CurrentState){
    case InitMorseElements:
      if ( ThisEvent.EventType == ES_INIT ){
        // move to the Initializing state
        NextState = CalWaitForRise; 
      }
      break;
      
    case CalWaitForRise :
      printf("Calibrating... "); 
      if(ThisEvent.EventType == ES_MORSE_RISING_EDGE)
      {
        TimeOfLastRise = ThisEvent.EventParam; 
        NextState = CalWaitForFall; 
      }
      if(ThisEvent.EventType == ES_CALIBRATION_COMPLETED)
      {
        //NextState = InitMorseElements; 
        //printf("Length of dot: %i\n",LengthOfDot);
        printf("  Calibration completed.\n\r"); 
        NextState = EOC_WaitRise; 
      }
      break;
      
    case CalWaitForFall :
      if (ThisEvent.EventType == ES_MORSE_FALLING_EDGE ){
        TimeOfLastFall = ThisEvent.EventParam; 
        NextState = CalWaitForRise; 
        TestCalibration();  
      }
      break;
      
    case EOC_WaitRise :
      if(ThisEvent.EventType == ES_MORSE_RISING_EDGE){
        TimeOfLastRise = ThisEvent.EventParam; 
        NextState = EOC_WaitFall; 
        CharacterizeSpace(); 
      }
      if(ThisEvent.EventType == DB_BUTTON_DOWN)
      {
        NextState = CalWaitForRise; 
        FirstDelta = 0; 
      }
      break;
      
    case EOC_WaitFall: 
      if (ThisEvent.EventType == ES_MORSE_FALLING_EDGE )
      {
        TimeOfLastFall = ThisEvent.EventParam; 
        NextState = EOC_WaitRise; 
      }
      if (ThisEvent.EventType == DB_BUTTON_DOWN)
      {
        NextState = CalWaitForRise; 
        FirstDelta = 0; 
      }
      if (ThisEvent.EventType == ES_EOC_DETECTED)
      {
        NextState = DecodeWaitFall; 
      }
      break;
      
    case DecodeWaitRise: 
      if (ThisEvent.EventType == ES_MORSE_RISING_EDGE)
      {
        TimeOfLastRise = ThisEvent.EventParam; 
        NextState = DecodeWaitFall; 
        CharacterizeSpace(); 
      }
      if (ThisEvent.EventType == DB_BUTTON_DOWN) 
      {
        NextState = CalWaitForRise; 
        FirstDelta = 0; 
      }
      
    case DecodeWaitFall: 
      if(ThisEvent.EventType == ES_MORSE_FALLING_EDGE)
      {
        TimeOfLastFall = ThisEvent.EventParam; 
        NextState = DecodeWaitRise; 
        CharacterizePulse(); 
      }
      if(ThisEvent.EventType == DB_BUTTON_DOWN)
      {
        NextState = CalWaitForRise; 
        FirstDelta = 0; 
      }
  }
  CurrentState = NextState; 
  return ReturnEvent;
}


static void TestCalibration(void) 
{
  uint16_t SecondDelta = 0; 
  if(FirstDelta == 0) 
  {
    FirstDelta = TimeOfLastFall - TimeOfLastRise; //CHANGE THIS TO MOST RECENT PULSE WIDTH
  }
  else 
  {
    SecondDelta = TimeOfLastFall - TimeOfLastRise; //CHANGE THIS TO MOST RECENT PULSE WIDTH 
    if((100.0 * (FirstDelta/SecondDelta)) <= 33.33)
    {
      LengthOfDot = FirstDelta; 
      ES_Event ThisEvent;
      ThisEvent.EventType = ES_CALIBRATION_COMPLETED; 
      PostMorseElementService(ThisEvent);
    }
    else if((100.0 *(FirstDelta/SecondDelta)) > 300.0)
    {
      LengthOfDot = SecondDelta; 
      ES_Event ThisEvent;
      ThisEvent.EventType = ES_CALIBRATION_COMPLETED; 
      PostMorseElementService(ThisEvent);
    }
    else //prepare for next pulse 
    {
      FirstDelta = SecondDelta; 
    }
  }
  return; 
}

static void CharacterizeSpace(void) 
{
  uint16_t LastInterval; 
  ES_Event Event2Post;
  LastInterval = TimeOfLastRise - TimeOfLastFall; 
  //if last interval not ok for dot space
  if(LastInterval > (LengthOfDot+3) || LastInterval < (LengthOfDot - 3))
  {
    //if last interval ok for character space
    if( (LastInterval <= 3*(LengthOfDot+3)) && (LastInterval >= (3*(LengthOfDot - 3))))
    {
      ES_Event ThisEvent;
      ThisEvent.EventType = ES_EOC_DETECTED;
      ES_PostList02(ThisEvent);
      //PostDecodeMorseService(ThisEvent); 
      //PostMorseElementService(ThisEvent);
      //printf("       ");
    }
    else
    {
      //if last interval ok for word space
      if((LastInterval >= 7*(LengthOfDot - 3)) && (LastInterval <=  7*(LengthOfDot+3)))
      {
        ES_Event ThisEvent;
        ThisEvent.EventType = ES_EOW_DETECTED; 
        PostDecodeMorseService(ThisEvent); 
        //printf("\n"); 
      }
      else
      {
        ES_Event ThisEvent;
        ThisEvent.EventType = ES_BAD_SPACE; 
        //PostDecodeMorseService(ThisEvent); 
        printf("bad space\n\r");
      }
    }
  }
  return;
}

static void CharacterizePulse(void) 
{
  uint16_t LastPulseWidth; 
  LastPulseWidth = TimeOfLastFall - TimeOfLastRise; 
  //last pulse width ok for a dot
  if(LastPulseWidth <= (LengthOfDot+3) && LastPulseWidth >= (LengthOfDot - 3))
  {
    ES_Event ThisEvent;
    ThisEvent.EventType = ES_DOT_DETECTED; 
    PostDecodeMorseService(ThisEvent);
    //printf(".");
  }
  else
  {
    //if last pulse width ok for dash
    if(LastPulseWidth <= 3*(LengthOfDot+3) && LastPulseWidth >= 3*(LengthOfDot - 3)) 
    {
      ES_Event ThisEvent;
      ThisEvent.EventType = ES_DASH_DETECTED; 
      PostDecodeMorseService(ThisEvent);
      //printf("-"); 
    }
    else
    {
      ES_Event ThisEvent;
      ThisEvent.EventType = ES_BAD_PULSE; 
      //PostDecodeMorseService(ThisEvent);
      printf("bad pulse\n\r");
    }
  }
  return; 
}



//WRITE FUNCTION HEADER COMMENTS HERE
bool CheckMorseEvents (void) 
{
  uint8_t CurrentInputState;
  bool ReturnVal = false;
  ES_Event ThisEvent;  
  //Get the CurrentInputState from the input line
  CurrentInputState = (HWREG(GPIO_PORTB_BASE+ ALL_BITS) & BIT6HI); //doesn't work without paranthesis
  
  
  // check if Morse pin high AND different from last time
  // do the check for difference first so that you don't bother with a test
  // of a port/variable that is not going to matter, since it hasn't changed
  //printf("%i",CurrentInputState); 
  //printf("Counter: %i\n",counter); 
  if( (CurrentInputState != LastInputState))
  {                     // event detected
    if(CurrentInputState == BIT6HI)
    {
      //went from low to high
      //printf("R"); 
      ThisEvent.EventType = ES_MORSE_RISING_EDGE; 
      ThisEvent.EventParam = ES_Timer_GetTime(); //CHANGE THIS
    }
    else
    {
      //went from high to low
      //printf("F"); 
      ThisEvent.EventType = ES_MORSE_FALLING_EDGE; 
      ThisEvent.EventParam = ES_Timer_GetTime(); //CHANGE THIS
    }
    
    PostMorseElementService(ThisEvent);
    // this could be any of the service post function, ES_PostListx or 
    // ES_PostAll functions
    //ES_PostList01(ThisEvent); 

    ReturnVal = true;
    LastInputState = CurrentInputState; //update the state for next time
  }
  return ReturnVal;
}


/***************************************************************************
 private functions
 ***************************************************************************/


/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

