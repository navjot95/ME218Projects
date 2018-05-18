/****************************************************************************
 
  Header file for Test Harness Service0 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef LCDService_H
#define LCDService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h"
#include "LCD_Write.h"

// typedefs for the states
// State definitions for use with the query function
typedef enum { InitPState, Initializing, Waiting2Write, 
               PausingBetweenWrites } LCDState_t ;

// Public Function Prototypes

bool InitScreenService ( uint8_t Priority );
bool PostScreenService( ES_Event_t ThisEvent );
ES_Event_t RunScreenService( ES_Event_t ThisEvent );
void printLCD(char *stringGotten);
void updateConnection(uint8_t onOffVal);
void updateAddr(char *addr);                

#endif /* LCDService_H */

