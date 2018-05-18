/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef CommSM_H
#define CommSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
typedef enum
{
  Waiting2Pair, 
  Trying2Pair,
  Communicating
}shipState_t;



// Public Function Prototypes
bool InitCommunicationSM(uint8_t Priority);
bool PostCommunicationSM(ES_Event_t ThisEvent);
ES_Event_t RunCommunicationSM(ES_Event_t ThisEvent);
shipState_t QueryCommunicationSM(void);

#endif /* CommSM_H */

