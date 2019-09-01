/****************************************************************************
 Module
   ShiftRegisterWrite.c

 Revision
   1.0.1

 Description
   This module acts as the low level interface to a write only shift register.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/11/15 19:55 jec     first pass
 
****************************************************************************/
// the common headers for C99 types 
#include <stdint.h>
#include <stdbool.h>

// the headers to access the GPIO subsystem
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

// the headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "BITDEFS.H"

// readability defines
#define DATA GPIO_PIN_0

#define SCLK GPIO_PIN_1
#define SCLK_HI BIT1HI
#define SCLK_LO BIT1LO

#define RCLK GPIO_PIN_2
#define RCLK_LO BIT2LO
#define RCLK_HI BIT2HI

#define GET_MSB_IN_LSB(x) ((x & 0x80)>>7)
#define ALL_BITS (0xff<<2)

// an image of the last 8 bits written to the shift register
static uint8_t LocalRegisterImage=0;

// Create your own function header comment
void SR_Init(void){

  // set up port B by enabling the peripheral clock, waiting for the 
  // peripheral to be ready and setting the direction
  // of PB0, PB1 & PB2 to output
  HWREG(SYSCTL_RCGCGPIO) |= BIT1HI; //enable Port B
	//wait for Port B to be ready
	while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1) 
	{
	} 
	
	//Initialize bit 0,1,and 2 on Port B to be a digital bit
	HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT0HI | BIT1HI | BIT2HI); 
	//Initialize bit 0 on Port B to be an output
	HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT0HI | BIT1HI | BIT2HI);
	
	// start with the data & sclk lines low and the RCLK line high
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT2HI;
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= (BIT0LO & BIT1LO);
	}


uint8_t SR_GetCurrentRegister(void){
  return LocalRegisterImage;
}


void SR_Write(uint8_t NewValue){
  //printf("In SR_write"); 
  uint8_t BitCounter;
  LocalRegisterImage = NewValue; // save a local copy
	 
// lower the register clock
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT2LO; 
	
// shift out the data while pulsing the serial clock  
// Isolate the MSB of NewValue, put it into the LSB position and output to port
// raise SCLK
// lower SCLK
// finish looping through bits in NewValue 
	
	for (int i = 0; i < 8; i++){
		if((NewValue & BIT7HI) != 0)
			HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT0HI; //set serial data high 
		else
			 HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT0LO; //set serial data low
		
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT1HI;
		HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT1LO;

		NewValue = NewValue << 1; 	
	}
	
	// raise the register clock to latch the new data
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT2HI;
	HWREG(GPIO_PORTB_BASE+(GPIO_O_DATA + ALL_BITS)) &= BIT2LO;
}
