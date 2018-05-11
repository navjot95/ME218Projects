#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include <stdbool.h>

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h" // Erez Added 
#include "ES_Framework.h"

// Configuration Headers 
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"
#include "termio.h"
#include "EnablePA25_PB23_PD7_PF0.h"


#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"

bool InitIMU(uint8_t Priority);
bool PostIMU(ES_Event_t ThisEvent);
ES_Event_t RunIMU(ES_Event_t ThisEvent);
uint16_t get_accel_x( void );
uint16_t get_accel_y( void );
uint16_t get_accel_z( void );
uint16_t get_gyro_x( void );
uint16_t get_gyro_y( void );
uint16_t get_gyro_z( void );


#endif
