/****************************************************************************

  Header file for ADService
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ADService_H
#define ADService_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Configure.h"

// Public Function Prototypes

bool InitADService(uint8_t Priority);
bool PostADService(ES_Event_t ThisEvent);
ES_Event_t RunADService(ES_Event_t ThisEvent);
uint32_t getStepTime(void); 

#endif /* ServTemplate_H */

