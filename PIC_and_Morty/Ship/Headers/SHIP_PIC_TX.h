/****************************************************************************

  Header file for SHIP_PIC_TX module
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/
#ifndef SHIP_PIC_TX_H
#define SHIP_PIC_TX_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
// the common headers for C99 types
#include <stdint.h>
#include <stdbool.h>

typedef enum { WaitingToTX_PIC, 
							 SendingTX_PIC} SHIP_PIC_TX_State_t ;

// Public Function Prototypes
bool InitSHIP_PIC_TX ( uint8_t Priority );
bool PostSHIP_PIC_TX ( ES_Event_t ThisEvent );
ES_Event_t RunSHIP_PIC_TX( ES_Event_t ThisEvent);

void SHIP_PIC_ISR(void);

#endif /* SHIP_PIC_TX_H */
