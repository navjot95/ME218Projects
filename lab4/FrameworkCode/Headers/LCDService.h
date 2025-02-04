/****************************************************************************
 
  Header file for Test Harness Service0 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef LCDService_H
#define LCDService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { InitPState, Initializing, Waiting2Write, 
               PausingBetweenWrites } LCDState_t ;

// Public Function Prototypes
bool InitLCDService ( uint8_t Priority );
bool PostLCDService( ES_Event ThisEvent );
ES_Event RunLCDService( ES_Event ThisEvent );


#endif /* LCDService_H */

