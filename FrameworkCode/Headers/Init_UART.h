/****************************************************************************

  Header file for Init_UART module
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef Init_UART_H
#define Init_UART_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
// the common headers for C99 types
#include <stdint.h>
#include <stdbool.h>

// Public Function Prototypes
void Init_UART_PIC(void);


#endif /* Init_UART_H */
