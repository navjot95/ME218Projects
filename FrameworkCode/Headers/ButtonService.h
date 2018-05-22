/*****************************************************************************
ME 218A - Team 5 
Stanford University, Department of Mechanical Engineering
Fall 2017
------------------------------------------------------------------------------

Header File: 
	ButtonService.h 

Description:
	Module for simple contact button with debouncing. 

Notes/Warnings:
	None

Revision History:	
	11/10/17 EK: Copied from Lab 4 work
	11/13/17 EK: Fixed to work with other files
******************************************************************************/

#ifndef BUTTON_SERVICE_H
#define BUTTON_SERVICE_H

// Standard Headers
#include <stdint.h>
#include <stdbool.h>

// Tiva files 
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */
#include "ES_Events.h" // Erez Added 
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"

// Public Function Prototypes 
bool InitButton( uint8_t Priority );
bool PostButton( ES_Event_t ThisEvent );
ES_Event_t RunButton( ES_Event_t ThisEvent );
bool CheckButtonEvents(void);

#endif
