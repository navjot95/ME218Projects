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
#include <string.h>

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"
#include "MorseElementService.h"
#include "DecodeMorseService.h"
#include "LCDService.h"

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

#define MORSE_ARRAY_LEN 8 

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static void ClearMorseString(void); 
static char DecodeMorseString(void); 


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority; 
static uint8_t MorseStringCounter = 0; 
static char MorseString[8]; 
char LegalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890?.,:'-/()\"= !$&+;@_";
char MorseCode[][8] ={ ".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--..",".----","..---","...--","....-",".....","-....","--...","---..","----.","-----","..--..",".-.-.-","--..--","---...",".----.","-....-","-..-.","-.--.-","-.--.-",".-..-.","-...-","-.-.--","...-..-",".-...",".-.-.","-.-.-.",".--.-.","..--.-"};


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
bool InitDecodeMorseService ( uint8_t Priority )
{
  MyPriority = Priority;
  
  ClearMorseString(); 

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
bool PostDecodeMorseService( ES_Event ThisEvent )
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
ES_Event RunDecodeMorseService( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
  //If dot detected, then add dot to internal representation 
  if(ThisEvent.EventType == ES_DOT_DETECTED)
  {
    if(MorseStringCounter < MORSE_ARRAY_LEN) //there's still room in array
    {
      MorseString[MorseStringCounter] = '.'; 
      MorseStringCounter++;       
    }
    else
    {
      ReturnEvent.EventType = ES_ERROR;       
    }
  }

  //If dash detected, then add dash to internal representation
  else if(ThisEvent.EventType == ES_DASH_DETECTED)
  {
    if(MorseStringCounter < MORSE_ARRAY_LEN) //there's still room in array
    {
      MorseString[MorseStringCounter] = '-'; 
      MorseStringCounter++;       
    }
    else
    {
      ReturnEvent.EventType = ES_ERROR;       
    }
  }

  //if end of character detected, then call DecodeMorse to try and decode the word
  else if(ThisEvent.EventType == ES_EOC_DETECTED)
  {
    char decodedChar = DecodeMorseString(); 
    if(decodedChar != '~')
    {
      printf("%c", decodedChar);
      //print to LCD
      ES_Event ThisEvent; 
      ThisEvent.EventType = ES_LCD_PUTCHAR; 
      ThisEvent.EventParam = decodedChar; 
      PostLCDService(ThisEvent); 
    }
    ClearMorseString();     
  }

  //if end of word detected, then call DecodeMorse to try and decode last char and also print a space
  else if(ThisEvent.EventType == ES_EOW_DETECTED)
  {
    char decodedChar = DecodeMorseString(); 
    if(decodedChar != '~')
    {
      printf("%c", decodedChar);
      printf(" "); 
    
      //print to LCD; 
      //print space to LCD;
      ES_Event ThisEvent; 
      ThisEvent.EventType = ES_LCD_PUTCHAR; 
      ThisEvent.EventParam = decodedChar; 
      PostLCDService(ThisEvent);
      ThisEvent.EventParam = ' '; 
      PostLCDService(ThisEvent);  
    }
    
    ClearMorseString(); 
  }

  //if received a bad reading or buttton was pressed, then start over 
  else if(ThisEvent.EventType == ES_BAD_SPACE || ThisEvent.EventType == ES_BAD_PULSE || ThisEvent.EventType == DB_BUTTON_DOWN)
  {
    ClearMorseString(); 
  }
 
  return ReturnEvent;
}


/***************************************************************************
 private functions
 ***************************************************************************/
char DecodeMorseString(void)
{
  uint8_t sizeOfMorseCode = sizeof(MorseCode)/sizeof(MorseCode[0]);

  for(uint8_t i = 0; i < sizeOfMorseCode; i++)
  {
    if(strcmp(MorseString, MorseCode[i]) == 0){
      return LegalChars[i]; 
    }
  }
  return '~'; 
}

void ClearMorseString(void)
{
  for(uint8_t i = 0; i < MORSE_ARRAY_LEN; i++) //WHY DO WE NEED TO DO THIS?
  {
    MorseString[i] = '\0' ;
  }
  MorseStringCounter = 0;
} 

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

