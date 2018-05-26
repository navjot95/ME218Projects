/****************************************************************************
 Module
     main.c
 Description
     starter main() function for Events and Services Framework applications
 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 08/21/17 12:53 jec     added this header as part of coding standard and added
                        code to enable as GPIO the port poins that come out of
                        reset locked or in an alternate function.
*****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Port.h"
#include "termio.h"
#include "EnablePA25_PB23_PD7_PF0.h"

#include "MotorModule.h"

#define clrScrn() printf("\x1b[2J")
#define goHome() printf("\x1b[1,1H")
#define clrLine() printf("\x1b[K")

//Initilize general purpose I/O pins 
void InitGPIO(void); 



int main(void)
{
  ES_Return_t ErrorType;

  // Set the clock to run at 40MhZ using the PLL and 16MHz external crystal
  SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
      | SYSCTL_XTAL_16MHZ);
  TERMIO_Init();
  clrScrn();

  // When doing testing, it is useful to announce just which program
  // is running.
  
  printf( "2nd Generation Events & Services Framework V2.4\r\n");
  printf( "%s %s\n", __TIME__, __DATE__);
  printf( "\n\r\n");
  

  // reprogram the ports that are set as alternate functions or
  // locked coming out of reset. (PA2-5, PB2-3, PD7, PF0)
  // After this call these ports are set
  // as GPIO inputs and can be freely re-programmed to change config.
  // or assign to alternate any functions available on those pins
  PortFunctionInit();

  // Your hardware initialization function calls go here
  InitFanPumpPWM(); 
  InitGPIO(); 

  // now initialize the Events and Services Framework and start it running
  ErrorType = ES_Initialize(ES_Timer_RATE_1mS);
  if (ErrorType == Success)
  {
    ErrorType = ES_Run();
  }
  //if we got to here, there was an error
  switch (ErrorType)
  {
    case FailedPost:
    {
      printf("Failed on attempt to Post\n");
    }
    break;
    case FailedPointer:
    {
      printf("Failed on NULL pointer\n");
    }
    break;
    case FailedInit:
    {
      printf("Failed Initialization\n");
    }
    break;
    default:
    {
      printf("Other Failure\n");
    }
    break;
  }
  for ( ; ;)
  {
    ;
  }
}

/*PRIVATE FUNCTIONS */

void InitGPIO(void){
  HWREG(SYSCTL_RCGCGPIO) |= BIT0HI; //enable Port A
  //wait for Port A to be ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R0) != SYSCTL_PRGPIO_R0) 
  {
  } 
  //Initialize bit 2(team switch), 3(tank valve), 4(shoot valve), 6(LED - home blue), 7(LED - curr blue) on Port A to be a digital bit
  HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT6HI | BIT7HI); 
  
  //Initialize bit 2 (team switch) on Port A to be an input
  HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) &= BIT2LO;
  //Initialize bit 3,4,6,7 to be digital output  
  HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= (BIT3HI | BIT4HI | BIT6HI | BIT7HI);
  
  
  
  HWREG(SYSCTL_RCGCGPIO) |= BIT1HI; //enable Port B
  //wait for Port B to be ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1) 
  {
  } 
  //Initialize pin 2(pump),3(LED fuel),4(left motor IN2),5(right motor IN2) on Port B to be a digital bit
  HWREG(GPIO_PORTB_BASE+GPIO_O_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); 
  //Initialize bit 2,3,4,5 to be digital output  
  HWREG(GPIO_PORTB_BASE+GPIO_O_DIR) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI);
  
  
  
  HWREG(SYSCTL_RCGCGPIO) |= BIT2HI; //enable Port C
  //wait for Port C to be ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R2) != SYSCTL_PRGPIO_R2) 
  {
  } 
  //Initialize pin 4(LED home - red),5 (LED curr - red) on Port C to be a digital bit
  HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= (BIT4HI | BIT5HI); 
  //Initialize bit 4,5 to be digital output  
  HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) |= (BIT4HI | BIT5HI);
  
  HWREG(GPIO_PORTA_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT3HI; //turn tank valve on (closes)
  HWREG(GPIO_PORTA_BASE + (GPIO_O_DATA + ALL_BITS)) &= (BIT4LO & BIT6LO & BIT7LO); //turn shoot valve (open), home and curr team blue off
  HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= (BIT2LO & BIT3LO & BIT4LO & BIT5LO); //pump, IN2 on right motor, IN2 on left motor, fuel LED set to low 
  HWREG(GPIO_PORTC_BASE + (GPIO_O_DATA + ALL_BITS)) &= (BIT4LO & BIT5LO); //turn home and curr team red off
    
}

