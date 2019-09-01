/****************************************************************************

  Header file for SPI service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ServSPI_H
#define ServSPI_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitSPIService(uint8_t Priority);
bool PostSPIService(ES_Event_t ThisEvent);
ES_Event_t RunSPIService(ES_Event_t ThisEvent);

#endif /* ServSPI_H */

