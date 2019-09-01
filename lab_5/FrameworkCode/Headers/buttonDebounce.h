/****************************************************************************
 
  Header file for ButtonDebounce.c
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef buttonDebounce_H
#define buttonDebounce_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
//#include "StepperMotorService.h" 

// typedefs for the states
// State definitions for use with the query function
typedef enum { Debouncing, Ready2Sample} ButtonState_t ;

// Public Function Prototypes
bool InitButtonDebounce ( uint8_t Priority );
bool PostButtonDebounce( ES_Event_t ThisEvent );
ES_Event_t RunButtonDebounce( ES_Event_t ThisEvent ); 
               
// Event checking functions 
bool CheckButtonEvents (void);                


#endif /* buttonDebounce_H */

