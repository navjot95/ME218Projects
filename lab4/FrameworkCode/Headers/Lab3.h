/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef Lab3_H
#define Lab3_H

#include "ES_Types.h"

// Public Function Prototypes

bool Lab3Init ( uint8_t Priority );
bool PostLab3( ES_Event ThisEvent );
ES_Event Lab3Run( ES_Event ThisEvent );


#endif /* Lab3_H */

