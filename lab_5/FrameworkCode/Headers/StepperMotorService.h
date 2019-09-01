/****************************************************************************

  Header file for StepperMotorService
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef StepperMotorService_H
#define StepperMotorService_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Configure.h"

// Public Function Prototypes

bool InitStepperMotorService(uint8_t Priority);
bool PostStepperMotorService(ES_Event_t ThisEvent);
ES_Event_t RunStepperMotorService(ES_Event_t ThisEvent);

#endif /* ServTemplate_H */

