/****************************************************************************

  Header file for SHIP_RX module
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/
#ifndef SHIP_RX_H
#define SHIP_RX_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
// the common headers for C99 types
#include <stdint.h>
#include <stdbool.h>

typedef enum { WaitingForStart, 
							 WaitingForMSBLen,
               WaitingForLSBLen,
               ReceivingData,
               ReceivingCheckSum} SHIP_RX_State_t ;

// Public Function Prototypes
bool InitSHIP_RX ( uint8_t Priority );
bool PostSHIP_RX ( ES_Event_t ThisEvent );
ES_Event_t RunSHIP_RX( ES_Event_t ThisEvent);

uint16_t QuerySourceAddress(void);
uint8_t Query_ANSIBLEColour (void);
uint8_t Query_FB (void);
uint8_t Query_LR (void);
uint8_t Query_TurretR (void);
uint8_t Query_TurretP (void);
uint8_t Query_CTRL (void);

#endif /* SHIP_RX_H */
