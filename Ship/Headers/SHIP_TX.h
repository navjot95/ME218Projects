/****************************************************************************

  Header file for SHIP_TX module
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/
#ifndef SHIP_TX_H
#define SHIP_TX_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
// the common headers for C99 types
#include <stdint.h>
#include <stdbool.h>

typedef enum { WaitingToTX, 
							 SendingTX} SHIP_TX_State_t ;

// Public Function Prototypes
bool InitSHIP_TX ( uint8_t Priority );
bool PostSHIP_TX ( ES_Event_t ThisEvent );
ES_Event_t RunSHIP_TX( ES_Event_t ThisEvent);

void SHIP_ISR(void);

#endif /* SHIP_TX_H */
