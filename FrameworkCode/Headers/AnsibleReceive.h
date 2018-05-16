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
void UARTHardwareInit(void);
void AnsibleTXISR (void);
 

//State definition for use with the query function 
typedef enum{WaitingForStart, WaitingForMSBLen, WaitingForLSBLen,ReceivingData,ReceivingCheckSum} AnsibleRXState_t; 

bool InitAnsibleRX(uint8_t Priority);
bool PostAnsibleRX(ES_Event_t ThisEvent);
ES_Event_t RunAnsibleRXSM(ES_Event_t ThisEvent);

#endif 

