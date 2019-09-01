//#define Lab3Module
/****************************************************************************
 Module
   Lab3.c

 Revision
   1.0.1

 Description
   This is the driver file for the LCD tester module for Lab3 
   

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/13/15 08:05 jec      began conversion from TemplateService
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "Lab3.h"
#include "LCDService.h"

// these times assume a 1.000mS/tick timing
#define ONE_SEC 976
#define HALF_SEC (ONE_SEC/2)
#define TWO_SEC (ONE_SEC*2)
#define FIVE_SEC (ONE_SEC*5)

/*----------------------------- Module Defines ----------------------------*/
// mask for the number of active bits in the array of messages
#define CHOOSE_MSG_MASK 0x01

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static uint8_t GetNextMessageChar(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
static uint8_t WhichMessage;

#ifdef Lab3Module
static char TestingMessage[][80]={
  "this is message 0", 
  "This is message 1"
};
#define WHICH_MSG_INDEX [WhichMessage]
#else //This must be the tester version
static char TestingMessage[]=
  "If you can read this message then you have built your hardware correctly ";
#define WHICH_MSG_INDEX 
#endif
/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     Lab3Init

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 10/13/15, 08:07
****************************************************************************/
bool Lab3Init ( uint8_t Priority )
{
  ES_Event ThisEvent;

  MyPriority = Priority;
  /********************************************
   in here you write your initialization code
   *******************************************/
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
     PostLab3

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/13/15, 08:10
****************************************************************************/
bool PostLab3( ES_Event_t ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    Lab3Run

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   Run function for the LCD character pumping service
 Notes
   
 Author
   J. Edward Carryer, 10/13/15, 08:11
****************************************************************************/
ES_Event_t Lab3Run( ES_Event_t ThisEvent )
{
  static bool IsTimerActive = false;
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

// when we get a keystroke, we read the timer to get a random number
// to use to choose which message to print and then start printing  
  if(ThisEvent.EventType == ES_NEW_KEY){
    if(IsTimerActive == false){
      WhichMessage = (ES_Timer_GetTime() & CHOOSE_MSG_MASK);
      ES_Timer_InitTimer(LCD_PUMP_TIMER, HALF_SEC);
      IsTimerActive = true;
    }
  }
  if(ThisEvent.EventType == ES_TIMEOUT){
    ES_Event ThisEvent;
    ThisEvent.EventType = ES_LCD_PUTCHAR;
    ThisEvent.EventParam = GetNextMessageChar();
    PostLCDService(ThisEvent);
    ES_Timer_InitTimer(LCD_PUMP_TIMER, HALF_SEC);
  }  
  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
/****************************************************************************
 Function
    GetNextMessageChar

 Parameters
   nothing

 Returns
   the next charcter to be printed

 Description
   steps through the message array and wraps around when it gets to the end.
 Notes
   
 Author
   J. Edward Carryer, 10/13/15, 08:16
****************************************************************************/
static uint8_t GetNextMessageChar(void){
  static uint8_t CurrentIndex = 0;
  char CurrentChar = TestingMessage WHICH_MSG_INDEX[CurrentIndex++];
  
  if (TestingMessage WHICH_MSG_INDEX[CurrentIndex] == 0){
    CurrentIndex=0;
  }
  return CurrentChar;
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

