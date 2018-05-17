#ifndef SENSOR_UPDATE_H
#define SENSOR_UPDATE_H

#include <stdint.h>
#include <stdbool.h>

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h" 
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

#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"

// Framework methods 
bool InitSensorUpdate( uint8_t Priority );
bool PostSensorUpdate( ES_Event_t ThisEvent);
ES_Event_t RunSensorUpdate( ES_Event_t ThisEvent);

// Getter methods 
uint8_t getBoatNumber( void );
uint8_t getThrottle( void );
uint8_t getPumpSpeed( void ); 


void Encoder_IOC_Response( void ); 

#endif
