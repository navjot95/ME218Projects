#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "driverlib/sysctl.h"

#include "ES_Port.h"
#include "termio.h"
#include "PWM16Tiva.h"

#define clrScrn() 	printf("\x1b[2J")
// num channels can be up to 16
#define NUM_CHAN2_TEST 1

void SetNextDuty( uint8_t NextDC, uint8_t NextChan );

static const char OutPutPins[][4]={"PB6",
                                  "PB7",
                                  "PB4",
                                  "PB5",
                                  "PE4",
                                  "PE5",
                                  "PC4",
                                  "PC5",
                                  "PD0",
                                  "PD1",
                                  "PA6",
                                  "PA7",
                                  "PF0",
                                  "PF1",
                                  "PF2",
                                  "PF3"};
                                  
int main(void)
{
    uint8_t i; // itterator for all channel tests
  
		// Set the clock to run at 40MhZ using the PLL and 16MHz external crystal
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
			| SYSCTL_XTAL_16MHZ);
  // initialize the timer sub-system and console I/O
  _HW_Timer_Init(ES_Timer_RATE_1mS);
	TERMIO_Init();
	clrScrn();

	// When doing testing, it is useful to announce just which program
	// is running.
	puts("\rStarting PWM16R2Test \r");
	printf("Testing %u channel(s)\n\r",NUM_CHAN2_TEST);
	printf("%s %s\n",__TIME__, __DATE__);
	printf("\n\r\n");

	// Your hardware initialization function calls go here
  if(PWM_TIVA_Init(NUM_CHAN2_TEST) != true){
    printf("Failed PWM_TIVA_Init(%d)\n\r",NUM_CHAN2_TEST);
  }else{
    printf("Passed PWM_TIVA_Init(%d)\n\r",NUM_CHAN2_TEST);
  }
 ///////////////////////////////////////////////////////////////////// 
  // set freq on Group 0 to 50Hz
  if(PWM_TIVA_SetFreq(50, 0) != true)
    puts("Failed PWM_TIVA_SetFreq(50, 0)\n\r");
  
  //set the pulse width on channel 0  
  if(PWM_TIVA_SetPulseWidth(1250,0) != true)
    puts("Failed PWM_TIVA_SetPulseWidth(1250,0)\n\r");
  puts("Outputting a 1ms pulse at 50Hz on Ch 0 (PB6)\r");
  
//set the duty cycle on channel 1  
  if(NUM_CHAN2_TEST > 1){
    if(PWM_TIVA_SetDuty( 50, 1) != true){
      puts("Failed PWM_TIVA_SetDuty( 50, 1)\n\r");
    }else{
      puts("Outputting a 50% DC at 50Hz on Ch 1 (PB7)\r");
    }      

    puts("Press any key for next test\r");
    getchar();
  }

//////////////////////////////////////////////////////////////////////  
//set freq on Group 1 to 400Hz
  if(NUM_CHAN2_TEST > 2){
    if(PWM_TIVA_SetFreq(400, 1) != true)
      puts("Failed PWM_TIVA_SetFreq(400, 1)\n\r");
  }
  //set the duty cycle on channel 2  
  if(NUM_CHAN2_TEST > 2){
    if(PWM_TIVA_SetDuty( 20, 2) != true)
      puts("Failed PWM_TIVA_SetDuty( 20, 2)\n\r");
  }
//set the duty cycle on channel 3  
  if(NUM_CHAN2_TEST > 3){
    if(PWM_TIVA_SetDuty( 30, 3) != true)
      puts("Failed PWM_TIVA_SetDuty( 30, 3)\n\r");
  }
  puts("Outputting a 20% DC at 400Hz on Ch 2 (PB4)\r");
  puts("Outputting a 30% DC at 400Hz on Ch 3 (PB5)\r");  
  puts("Press any key for next test\r");
  getchar();

/////////////////////////////////////////////////////////////////////// 
//set period on group 2
  if(NUM_CHAN2_TEST > 4){
    if(PWM_TIVA_SetPeriod( 1250, 2) != true)
      puts("Failed PWM_TIVA_SetPeriod( 1250, 2)\n\r");
  }

//set the duty cycle on channel 4  
  if(NUM_CHAN2_TEST > 4){
    if(PWM_TIVA_SetDuty( 40, 4) != true)
      puts("Failed PWM_TIVA_SetDuty( 40, 4)\n\r");
  }
//set the duty cycle on channel 5  
  if(NUM_CHAN2_TEST > 5){
    if(PWM_TIVA_SetDuty( 50, 5) != true)
      puts("Failed PWM_TIVA_SetDuty( 50, 5)\n\r");
  }
  
  puts("Outputting a 40% DC at 1000Hz on Ch 4 (PE4)\r");
  puts("Outputting a 50% DC at 1000Hz on Ch 5 (PE5)\r");  
  puts("Press any key for next test\r");
  getchar();

///////////////////////////////////////////////////////////////////////  
// set period on group 3  
  if(NUM_CHAN2_TEST > 6){
    if(PWM_TIVA_SetPeriod( 2500, 3) != true)
      puts("Failed PWM_TIVA_SetPeriod( 2500, 3)\n\r");
  }

//set the duty cycle on channel 6  
  if(NUM_CHAN2_TEST > 6){
    if(PWM_TIVA_SetDuty( 20, 6) != true)
      puts("Failed PWM_TIVA_SetDuty( 20, 6)\n\r");
  }
//set the duty cycle on channel 7  
  if(NUM_CHAN2_TEST > 7){
    if(PWM_TIVA_SetDuty( 10, 7) != true)
      puts("Failed PWM_TIVA_SetDuty( 10, 7)\n\r");
  }
  
  puts("Outputting a 20% DC at 500Hz on Ch 6 (PC4)\r");
  puts("Outputting a 10% DC at 500Hz on Ch 7 (PC5)\r");  
  puts("Press any key for next test\r");
  getchar();

///////////////////////////////////////////////////////////////////////  
// set period on group 4  
  if(NUM_CHAN2_TEST > 8){
    if(PWM_TIVA_SetPeriod( 2500, 4) != true)
      puts("Failed PWM_TIVA_SetPeriod( 2500, 4)\n\r");
  }
//set the duty cycle on channel 8  
  if(NUM_CHAN2_TEST > 8){
    if(PWM_TIVA_SetDuty( 20, 8) != true)
      puts("Failed PWM_TIVA_SetDuty( 20, 8)\n\r");
  }
//set the duty cycle on channel 9  
  if(NUM_CHAN2_TEST > 9){
    if(PWM_TIVA_SetDuty( 10, 9) != true)
      puts("Failed PWM_TIVA_SetDuty( 10, 9)\n\r");
  }

  puts("Outputting a 20% DC at 500Hz on Ch 8 (PD0)\r");
  puts("Outputting a 10% DC at 500Hz on Ch 9 (PD1)\r");  
  puts("Press any key for next test\r");
  getchar();

///////////////////////////////////////////////////////////////////////  
// set period on group 5  
  if(NUM_CHAN2_TEST > 10){
    if(PWM_TIVA_SetPeriod( 2500, 5) != true)
      puts("Failed PWM_TIVA_SetPeriod( 2500, 5)\n\r");
  }
 //set the duty cycle on channel 10  
  if(NUM_CHAN2_TEST > 10){
    if(PWM_TIVA_SetDuty( 15, 10) != true)
      puts("Failed PWM_TIVA_SetDuty( 15,10)\n\r");
  }
//set the duty cycle on channel 11  
  if(NUM_CHAN2_TEST > 11){
    if(PWM_TIVA_SetDuty( 25, 11) != true)
      puts("Failed PWM_TIVA_SetDuty( 25,11)\n\r");
  }

  puts("Outputting a 15% DC at 500Hz on Ch 10 (PA6)\r");
  puts("Outputting a 25% DC at 500Hz on Ch 11 (PA7)\r");  
  puts("Press any key for next test\r");
  getchar();

//////////////////////////////////////////////////////////////////////  
// set period on group 6  
  if(NUM_CHAN2_TEST > 12){
    if(PWM_TIVA_SetPeriod( 2500, 6) != true)
      puts("Failed PWM_TIVA_SetPeriod( 2500, 6)\n\r");
  }
 //set the duty cycle on channel 12  
  if(NUM_CHAN2_TEST > 12){
    if(PWM_TIVA_SetDuty( 15, 12) != true)
      puts("Failed PWM_TIVA_SetDuty( 15,12)\n\r");
  }
//set the duty cycle on channel 13  
  if(NUM_CHAN2_TEST > 13){
    if(PWM_TIVA_SetDuty( 25, 13) != true)
      puts("Failed PWM_TIVA_SetDuty( 25,13)\n\r");
  }

  puts("Outputting a 15% DC at 500Hz on Ch 12 (PF0)\r");
  puts("Outputting a 25% DC at 500Hz on Ch 13 (PF1)\r");  
  puts("Press any key for next test\r");
  getchar();

/////////////////////////////////////////////////////////////////////  
// set period on group 7  
  if(NUM_CHAN2_TEST > 14){
    if(PWM_TIVA_SetPeriod( 2500, 7) != true)
      puts("Failed PWM_TIVA_SetPeriod( 2500, 7)\n\r");
  }
 
 //set the duty cycle on channel 14  
  if(NUM_CHAN2_TEST > 14){
    if(PWM_TIVA_SetDuty( 15, 14) != true)
      puts("Failed PWM_TIVA_SetDuty( 15,14)\n\r");
  }
//set the duty cycle on channel 15  
  if(NUM_CHAN2_TEST > 15){
    if(PWM_TIVA_SetDuty( 25, 15) != true)
      puts("Failed PWM_TIVA_SetDuty( 25,15)\n\r");
  }
  puts("Outputting a 15% DC at 500Hz on Ch 14 (PF2)\r");
  puts("Outputting a 25% DC at 500Hz on Ch 15 (PF3)\r");  
  puts("Press any key for next test\r");
  getchar();

////////////////////////////////////////////////////////////////
  PWM_TIVA_SetDuty(100,0);
  puts("\nDuty Cycle set to 100%  on  Channel 0  (PB6)\n\r");
  puts("Press any key for next test\r");
  getchar();
  
  PWM_TIVA_SetDuty(0,0);
  puts("\nDuty Cycle set to 0%  on  Channel 0  (PB6)\n\r");
  puts("Press any key for next test\r");
  getchar();
  
  PWM_TIVA_SetDuty(50,0);
  puts("\nDuty Cycle set to 50%  on  Channel 0  (PB6)\n\r");
  puts("Press any key for next test\r");
  getchar();

//////////////////////////////////////////////////////////////////////
  PWM_TIVA_SetPulseWidth(0,0);
  puts("\nPulse Width set to 0  on  Channel 0  (PB6)\n\r");
  puts("Press any key for next test\r");
  getchar();
  
//////////////////////////////////////////////////////////////////////
  if (PWM_TIVA_SetPulseWidth(5000,0) != false){
    puts("\nPulse Width set to 5000  on  Channel 0  (PB6) FAILS, as it should\n\r");
  }else{
    puts("\nPulse Width set to 5000  on  Channel 0  (PB6) PASSES, This is wrong!!\n\r");
  }
  puts("Press any key for next test\r");
  getchar();
  
  for( i = 1; i < NUM_CHAN2_TEST; i++)
  {
    SetNextDuty( 25, i );
    SetNextDuty( 100, i );
    SetNextDuty( 0, i );
    SetNextDuty( 50, i );

  }
  puts("\nTests Complete");

  
	// if you fall off the end of your code, then hang around here
	for(;;)
	  ;

}

void SetNextDuty( uint8_t NextDC, uint8_t NextChan )
{
  PWM_TIVA_SetDuty(NextDC,NextChan);
  printf("\nDuty Cycle set to %d%%  on  Channel %d outputting on %s\n\r", 
                              NextDC, NextChan,OutPutPins[NextChan]);
  puts("Press any key for next test\r");
  getchar();
  
}
