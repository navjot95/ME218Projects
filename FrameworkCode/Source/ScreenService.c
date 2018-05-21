/****************************************************************************
 Module
   LCDService.c

 Revision
   1.0.1

 Description
   This is the first service for the Test Harness under the 
   Gen2 Events and Services Framework.

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
#include "ScreenService.h" 

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"	// Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"

#include "LCD_Write.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 1.000mS/tick timing
#define ONE_SEC 976
#define HALF_SEC (ONE_SEC/2)
#define TWO_SEC (ONE_SEC*2)
#define FIVE_SEC (ONE_SEC*5)
#define LCD_REFRESH_TIME HALF_SEC

//Screen location defines 
#define BEG_SEC_LINE 0xC0  //beginning of second line 
#define BEG_FIR_LINE 0x80  //beginning of first line 


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static char getNextChar(void);

//These three are for DEBUG only, need to be replaced by other services
static bool getCurrConStatus(void);
static bool getCurrFuelStatus(void);
static uint8_t getCurrTeamNum(void);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
//static char *stringToPrint;
static char stringBuffer[70]; 

static LCDState_t CurrentState = InitPState;

// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
//static ES_Event_t DeferralQueue[3+1];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitLCDService

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
bool InitScreenService ( uint8_t Priority )
{
  ES_Event_t ThisEvent;
  
  MyPriority = Priority;
  
//Put us into the initial pseudo-state to set up for the initial transition
	CurrentState = InitPState;
// set up the short timer for inter-command timings
  ES_ShortTimerInit(MyPriority, SHORT_TIMER_UNUSED );
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
bool PostScreenService( ES_Event_t ThisEvent )
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
ES_Event_t RunScreenService( ES_Event_t ThisEvent )
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  uint16_t DelayTime  = 0;
  
  switch (CurrentState){
    case InitPState :
      if ( ThisEvent.EventType == ES_INIT ){
// initialize the LCD hardware
				LCD_HWInit(); 
        // take the first initialization step and get required delay time
				DelayTime = LCD_TakeInitStep();        

        // start the timer for that delay
				ES_ShortTimerStart(TIMER_A, DelayTime); 

        // move to the Initializing state
				CurrentState = Initializing; 
      }
      break;
    case Initializing :
      if( ThisEvent.EventType == ES_SHORT_TIMEOUT ){  
                // take the next LCD initialization step and get the next delay time
				DelayTime = LCD_TakeInitStep(); 

                // if there are more steps to the initialization, then start the timer
                // else move to the Waiting2Write state
				if(DelayTime != 0){ 
                    ES_ShortTimerStart(TIMER_A, DelayTime); 
				}					
				else {
					CurrentState = Waiting2Write;
                    
                    ES_Event_t NewEvent; 
                    NewEvent.EventType = ES_TIMEOUT; 
                    NewEvent.EventParam = SCREEN_UPDATE_TIMER; 
                    ES_PostToService( MyPriority, NewEvent);
				}					
      }
      break;

    case Waiting2Write :
        if(ThisEvent.EventType == ES_TIMEOUT && (ThisEvent.EventParam == SCREEN_UPDATE_TIMER)){           
            char fuel = getCurrFuelStatus()? 'F' : 'E'; 
            char *connection = getCurrConStatus()? "PAIRED  " : "UNPAIRED";  
            LCD_WriteCommand8(BEG_FIR_LINE); //move to start of line 1
            
            sprintf(stringBuffer, "Team #:%02d  Fl:%c                         Status: %s", getCurrTeamNum(), fuel, connection);
            
            ES_Timer_InitTimer(SCREEN_UPDATE_TIMER, LCD_REFRESH_TIME); //restart the refresh timer
            
            ThisEvent.EventType = ES_LCD_PUTCHAR; //So next part of Waiting2Write executes     
        }
      if (ThisEvent.EventType == ES_LCD_PUTCHAR ){
        // write the character to the LCD
        char charToPrint = getNextChar(); 
        
        if(charToPrint){ //if not null char            
          LCD_WriteData8(charToPrint); 
          // start the inter-character timer
          ES_ShortTimerStart(TIMER_A, INTER_CHAR_DELAY);

          // move to the PausingBetweenWrites state
          CurrentState = PausingBetweenWrites; 
        }
     }      
      break;
      
    case PausingBetweenWrites :
		if(ThisEvent.EventType == ES_SHORT_TIMEOUT){
            ES_Event_t NewEvent; 
            NewEvent.EventType = ES_LCD_PUTCHAR; 
			
			CurrentState = Waiting2Write; 
            ES_PostToService( MyPriority, NewEvent); 
		}
        break;
  }
  return ReturnEvent;
}

/*
void printLCD(char *stringGotten){
  printf("Starting print\n\r"); 
  ES_Event_t ThisEvent; 
  ThisEvent.EventType = ES_LCD_PUTCHAR; 
  stringToPrint = stringGotten;
  ES_PostToService( MyPriority, ThisEvent);   
}


void updateAddr(char *addr){
  ES_Event_t ThisEvent; 
  ThisEvent.EventType = ES_LCD_PUTCHAR; 
  LCD_WriteCommand8(0x86); //Move to where is address is written  
  stringToPrint = addr;
  ES_PostToService( MyPriority, ThisEvent);
}

//sending 0 means "unpaired" will be written, otherwise "paired" will be written 
void updateConnection(uint8_t onOffVal){
  ES_Event_t ThisEvent; 
  ThisEvent.EventType = ES_LCD_PUTCHAR; 
  LCD_WriteCommand8(0xC8); //Move to where is address is written  
  
  if(onOffVal)
    stringToPrint = "PAIRED  ";
  else
    stringToPrint = "UNPAIRED"; 
  
  ES_PostToService( MyPriority, ThisEvent);
}

void updateFuel(uint8_t fuelBool){
  ES_Event_t ThisEvent; 
  ThisEvent.EventType = ES_LCD_PUTCHAR; 
  LCD_WriteCommand8(0x80); //Move to where is fuel status is written  
  
  if(fuelBool){
    stringToPrint = "F"; 
    //LCD_WriteData8(0xff); 
    printf("1\n\r"); 
  }
  else {
    stringToPrint = "E"; 
    //LCD_WriteData8(0x00); 
    printf("0\n\r"); 
  }

  ES_PostToService( MyPriority, ThisEvent);  
}
*/

/***************************************************************************
 private functions
 ***************************************************************************/
static char getNextChar(void){
  static uint32_t i = 0; 
  
  char returnChar = stringBuffer[i]; 
    
  if(returnChar == '\0')
    i = 0; 
  else
    i++; 
  
  return returnChar;
}

/*FOR DEBUG ONLYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY*/
static bool getCurrFuelStatus(void){
    return false; 
}


/*FOR DEBUG ONLYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY*/
static bool getCurrConStatus(void){
    return false; 
}


static uint8_t getCurrTeamNum(void){
    return 5; 
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

