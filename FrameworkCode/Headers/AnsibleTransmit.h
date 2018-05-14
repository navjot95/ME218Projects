/****************************************************************************

  Header file for Test Harness Service0
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef AnsibleTransmit_H
#define AnsibleTrasmit_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"

// Public Function Prototypes

//State definition for use with the query function 
typedef enum{InitTX, WaitingToTX, Transmitting} AnsibleTXState_t; 

bool InitAnsibleTX(uint8_t Priority);
bool PostAnsibleTX(ES_Event_t ThisEvent);
ES_Event_t RunAnsibleTXSM(ES_Event_t ThisEvent);

#endif 

