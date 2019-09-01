/****************************************************************************

  Header file for Init_UART module
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef SHIP_RX_test_H
#define SHIP_RX_test_H

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
               ReceivingCheckSum} SHIP_RX_testState_t ;

// Public Function Prototypes
bool InitSHIP_RX_test ( uint8_t Priority );
bool PostSHIP_RX_test( ES_Event_t ThisEvent );
ES_Event_t RunSHIP_RX_test( ES_Event_t ThisEvent);
               
void SHIP_ISR_test(void);

   
               
#endif /* SHIP_RX_test_H */
               