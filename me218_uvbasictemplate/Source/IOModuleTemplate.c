/****************************************************************************
 Module
   IOModuleTemplate.c

 Revision
   1.0.1

 Description
   This is the sample file to use when creating a module that will need to
   access the GPIO ports. All that it really provides is a set of the common 
   header files already included.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/15/14 14:30 jec     first pass
 
****************************************************************************/
// the common headers for I/O, C99 types 
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// the headers to access the GPIO subsystem
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

// the header to get the timing functions
#include "ES_Port.h"
