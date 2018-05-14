/****************************************************************************

  Header file for Test Harness Service0
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef AnsibleMain_H
#define AnsibleMain_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"

//State definition for use with the query function 
typedef enum{InitAnsible, WaitingForPair, WaitingForPairResp, CommunicatingSHIP} AnsibleMainState_t; 

// Public Function Prototypes
void UARTHardwareInit(void); //initialize the UART hardware

bool InitAnsibleMain(uint8_t Priority);
bool PostAnsibleMain(ES_Event_t ThisEvent);
ES_Event_t RunAnsibleMainSM(ES_Event_t ThisEvent);

#endif 

