#ifndef MotorService_H
#define MotorService_H

// Event Definitions
//#include "ES_Events.h"
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  IgnoringTapeAndBeacon, LookingForTape, LookingForBeacon
}MotorServiceState_t;

// Public Function Prototypes

bool InitMotorService(uint8_t Priority);
bool PostMotorService(ES_Event_t ThisEvent);
ES_Event_t RunMotorService(ES_Event_t ThisEvent);
bool Check4Beacon(void);
bool Check4Tape(void);

#endif
