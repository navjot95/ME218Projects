#ifndef SENSOR_UPDATE_H
#define SENSOR_UPDATE_H

#include <stdint.h>
#include <stdbool.h>

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h" // Erez Added 
#include "ES_Framework.h"


#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"


#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "ADMulti.h"

#include "TimeDefs.h"




bool InitMotor( uint8_t Priority );
bool PostMotor( ES_Event_t ThisEvent);
ES_Event_t RunMotor( ES_Event_t ThisEvent);

#endif
