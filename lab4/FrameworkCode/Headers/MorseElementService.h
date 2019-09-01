/****************************************************************************
 
  Header file for Test Harness Service0 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef MorseElementService_H
#define MorseElementService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { InitMorseElements, CalWaitForRise, CalWaitForFall,EOC_WaitRise, 
                  EOC_WaitFall, DecodeWaitRise, DecodeWaitFall } MorseElementState_t ;

// Public Function Prototypes
bool InitMorseElementService ( uint8_t Priority );
bool PostMorseElementService( ES_Event ThisEvent );
ES_Event RunMorseElementService( ES_Event ThisEvent );
               
// Event checking functions 
bool CheckMorseEvents (void);                


#endif /* MorseElementService_H */

