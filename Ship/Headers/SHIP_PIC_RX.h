/****************************************************************************

  Header file for SHIP_PIC_RX module
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/
#ifndef SHIP_PIC_RX_H
#define SHIP_PIC_RX_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
// the common headers for C99 types
#include <stdint.h>
#include <stdbool.h>

typedef enum { WaitingForData_PIC
               } SHIP_PIC_RX_State_t ;

// Public Function Prototypes
bool InitSHIP_PIC_RX ( uint8_t Priority );
bool PostSHIP_PIC_RX ( ES_Event_t ThisEvent );
ES_Event_t RunSHIP_PIC_RX( ES_Event_t ThisEvent);

uint8_t QueryFuelEmpty(void);
uint8_t QueryFuelStatus(void);

#endif /* SHIP_PIC_RX_H */
