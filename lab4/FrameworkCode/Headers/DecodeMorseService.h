/****************************************************************************
 
  Header file for Test Harness Service0 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef DecodeMorseService_H
#define DecodeMorseService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// Public Function Prototypes
bool InitDecodeMorseService ( uint8_t Priority );
bool PostDecodeMorseService( ES_Event ThisEvent );
ES_Event RunDecodeMorseService( ES_Event ThisEvent );
               
// Event checking functions 
bool CheckMorseEvents (void);                


#endif /* MorseElementService_H */

