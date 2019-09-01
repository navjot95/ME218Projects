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

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_Port.h"
#include "termio.h"
#include "EnablePA25_PB23_PD7_PF0.h"
#include "ADMulti.h"

#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"
#include "inc/hw_pwm.h"
#include "inc/hw_gpio.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"

#define clrScrn() printf("\x1b[2J")
#define goHome() printf("\x1b[1,1H")
#define clrLine() printf("\x1b[K")

#define TestPWM

//Initializing general GPIO and analog pins
#define NumAnalog 4 //2 for inductors and 2 for Sharp sensors
void InitGPIO(void);

//Initializing interrupts
#define TicksPerMS 40000
#define OVERTIME_DURATION 100000
void InitInterrupts(void);

//Initializing SPI
#define BitsPerNibble 4
#define PreScaler 54
#define ClockRate 50
void InitSPI(void);

//Initializing PWM for motors, frequency of 500 Hz, 50Hz for servos
#define LOAD_VALUE 1250
#define LOAD_VALUE_SERVO 12500
#define SERVO_CMP_CENTER 937  //1.5ms high time
#define SERVO_CMP_RIGHT 1562  //2.5ms high time
#define GenA_Normal (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO)
#define GenB_Normal (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO)
#define FLYWHEEL_DUTY_CYCLE 40
#define RETROREFLECTIVE_SEND_PERIOD 300 //2.5 ms; 400 Hz
void InitPWM(void);

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
  puts("\rStarting Test Harness for \r");
  printf( "the 2nd Generation Events & Services Framework V2.4\r\n");
  printf( "%s %s\n", __TIME__, __DATE__);
  printf( "\n\r\n");
  printf( "Press any key to post key-stroke events to Service 0\n\r");
  printf( "Press 'd' to test event deferral \n\r");
  printf( "Press 'r' to test event recall \n\r");

  // reprogram the ports that are set as alternate functions or
  // locked coming out of reset. (PA2-5, PB2-3, PD7, PF0)
  // After this call these ports are set
  // as GPIO inputs and can be freely re-programmed to change config.
  // or assign to alternate any functions available on those pins
  PortFunctionInit();

  // Your hardware initialization function calls go here
  // First disable interrupts
  __disable_irq();

  //Init everything
  InitGPIO();
  InitInterrupts();
  InitPWM();
  InitSPI();

  //Finally enable global interrupts
  __enable_irq();

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

void InitGPIO(void)
{
  //Port A in SPI init

  //Port B
  //Enable the clock to Port B
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;

  //Wait till clock for Port B is ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1)
  {}

  //Set as digital
  HWREG(GPIO_PORTB_BASE + GPIO_O_DEN) |= (BIT0HI | BIT1HI | BIT2HI | BIT3HI);

  //Set as outputs
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) |= (BIT0HI | BIT1HI);

  //Set as inputs
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) &= (BIT2LO & BIT3LO);

  //Port C

  //Enable the clock to Port C
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2;

  //Wait till clock for Port C is ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R2) != SYSCTL_PRGPIO_R2)
  {}

  //Set as digital
  HWREG(GPIO_PORTC_BASE + GPIO_O_DEN) |= (BIT6HI);

  //Set as outputs

  //Set as inputs
  HWREG(GPIO_PORTC_BASE + GPIO_O_DIR) &= (BIT6LO);

  //Port D

  //Enable the clock to Port D
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R3;

  //Wait till clock for Port B is ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R3) != SYSCTL_PRGPIO_R3)
  {}

  //Set as digital
  HWREG(GPIO_PORTD_BASE + GPIO_O_DEN) |= (BIT4HI | BIT6HI | BIT7HI);

  //Set as outputs
  HWREG(GPIO_PORTD_BASE + GPIO_O_DIR) |= (BIT4HI | BIT6HI);

  //Set as inputs
  HWREG(GPIO_PORTD_BASE + GPIO_O_DIR) &= (BIT7LO);

  //Port E
  //Clock enable for Port E included within function
  //Init analog inputs for magnetic line following and Sharp sensor
  ADC_MultiInit(NumAnalog);
}

//Init input capture interrupt using wide timers 0A, 0B, 3A, 3B
void InitInterrupts(void)
{
  //WIDE TIMER 0A
  // start by enabling the clock to the timer (Wide Timer 0)
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;

  // kill a few cycles to let the clock get going
  while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R0) != SYSCTL_PRWTIMER_R0)
  {}

  // make sure that timer (Timer A) is disabled before configuring
  HWREG(WTIMER0_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TAEN;

  // set it up in 32bit wide (individual, not concatenated) mode
  // the constant name derives from the 16/32 bit timer, but this is a 32/64
  // bit timer so we are setting the 32bit mode
  HWREG(WTIMER0_BASE + TIMER_O_CFG) = TIMER_CFG_16_BIT;

  // we want to use the full 32 bit count, so initialize the Interval Load
  // register to 0xffff.ffff (its default value :-)
  HWREG(WTIMER0_BASE + TIMER_O_TAILR) = 0xffffffff;

  // set up timer A in capture mode (TAMR=3, TAAMS = 0),
  // for edge time (TACMR = 1) and up-counting (TACDIR = 1)
  HWREG(WTIMER0_BASE + TIMER_O_TAMR) =
      (HWREG(WTIMER0_BASE + TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
      (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);

  // To set the event to rising edge, we need to modify the TAEVENT bits
  // in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
  HWREG(WTIMER0_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;

  // Now Set up the port to do the capture (clock was enabled earlier)
  // start by setting the alternate function for Port C bit 4 (WT0CCP0)
  HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= BIT4HI;

  // Then, map bit 4's alternate function to WT0CCP0
  // 7 is the mux value to select WT0CCP0, 16 to shift it over to the
  // right nibble for bit 4 (4 bits/nibble * 4 bits)
  HWREG(GPIO_PORTC_BASE + GPIO_O_PCTL) =
      (HWREG(GPIO_PORTC_BASE + GPIO_O_PCTL) & 0xfff0ffff) + (7 << (4 * BitsPerNibble));

  // Enable pin 4 on Port C for digital I/O
  HWREG(GPIO_PORTC_BASE + GPIO_O_DEN) |= BIT4HI;

  // make pin 4 on Port C into an input
  HWREG(GPIO_PORTC_BASE + GPIO_O_DIR) &= BIT4LO;

  // back to the timer to enable a local capture interrupt
//  HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;

  // enable the Timer A in Wide Timer 0 interrupt in the NVIC
  // it is interrupt number 94 so appears in EN2 at bit 30
  HWREG(NVIC_EN2) |= BIT30HI;

  // now kick the timer off by enabling it and enabling the timer to
  // stall while stopped by the debugger
  HWREG(WTIMER0_BASE + TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);

  //WIDE TIMER 1A AND 1B
  // start by enabling the clock to the timer (Wide Timer 1)
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R1;

  // kill a few cycles to let the clock get going
  while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R1) != SYSCTL_PRWTIMER_R1)
  {}

  // make sure that timer (Timer A) is disabled before configuring
  HWREG(WTIMER1_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TAEN;

  // set it up in 32bit wide (individual, not concatenated) mode
  HWREG(WTIMER1_BASE + TIMER_O_CFG) = TIMER_CFG_16_BIT;

  // set up timer A in periodic mode so that it repeats the time-outs
  HWREG(WTIMER1_BASE + TIMER_O_TAMR) =
      (HWREG(WTIMER1_BASE + TIMER_O_TAMR) & ~TIMER_TAMR_TAMR_M) | TIMER_TAMR_TAMR_1_SHOT;

  // set timeout to 100 seconds
  HWREG(WTIMER1_BASE + TIMER_O_TAILR) = (uint32_t)(((uint32_t)TicksPerMS * (uint32_t)OVERTIME_DURATION));

  // enable a local timeout interrupt
  HWREG(WTIMER1_BASE + TIMER_O_IMR) |= TIMER_IMR_TATOIM;

  // enable the Timer A in Wide Timer 0 interrupt in the NVIC
  // it is interrupt number 96 so appears in EN3 at bit 0
  HWREG(NVIC_EN3) |= BIT0HI;

  // now kick the timer off by enabling it and enabling the timer to
  // stall while stopped by the debugger
  HWREG(WTIMER1_BASE + TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);

  // make sure that timer (Timer B) is disabled before configuring
  HWREG(WTIMER1_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TBEN;

  // we want to use the full 32 bit count, so initialize the Interval Load
  // register to 0xffff.ffff (its default value :-)
  HWREG(WTIMER1_BASE + TIMER_O_TBILR) = 0xffffffff;

  // set up timer B in capture mode (TAMR=3, TAAMS = 0),
  // for edge time (TACMR = 1) and up-counting (TACDIR = 1)
  HWREG(WTIMER1_BASE + TIMER_O_TBMR) =
      (HWREG(WTIMER1_BASE + TIMER_O_TBMR) & ~TIMER_TBMR_TBAMS) |
      (TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP);

  // To set the event to rising edge, we need to modify the TBEVENT bits
  // in GPTMCTL. Rising edge = 00, so we clear the TBEVENT bits
  HWREG(WTIMER1_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TBEVENT_M;

  // Now Set up the port to do the capture (clock was enabled earlier)
  // start by setting the alternate function for Port C bit 7 (WT1CCP1)
  HWREG(GPIO_PORTC_BASE + GPIO_O_AFSEL) |= BIT7HI;

  // Then, map bit 7's alternate function to WT1CCP1
  // 7 is the mux value to select WT1CCP1, 28 to shift it over to the
  // right nibble for bit 7 (4 bits/nibble * 7 bits)
  HWREG(GPIO_PORTC_BASE + GPIO_O_PCTL) =
      (HWREG(GPIO_PORTC_BASE + GPIO_O_PCTL) & 0x0fffffff) + (7 << (7 * BitsPerNibble));

  // Enable pin 7 on Port C for digital I/O
  HWREG(GPIO_PORTC_BASE + GPIO_O_DEN) |= BIT7HI;

  // make pin 7 on Port C into an input
  HWREG(GPIO_PORTC_BASE + GPIO_O_DIR) &= BIT7LO;

  // back to the timer to enable a local capture interrupt
//  HWREG(WTIMER1_BASE+TIMER_O_IMR) |= TIMER_IMR_CBEIM;

  // enable the Timer B in Wide Timer 1 interrupt in the NVIC
  // it is interrupt number 97 so appears in EN3 at bit 1
  HWREG(NVIC_EN3) |= BIT1HI;

  // now kick the timer off by enabling it and enabling the timer to
  // stall while stopped by the debugger
  HWREG(WTIMER1_BASE + TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);

  //WIDE TIMERS 3A AND 3B
  // start by enabling the clock to the timer (Wide Timer 3)
  HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R3;

  // kill a few cycles to let the clock get going
  while ((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R3) != SYSCTL_PRWTIMER_R3)
  {}

  // make sure that timer (Timer A) is disabled before configuring
  HWREG(WTIMER3_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TAEN;

  // make sure that timer (Timer B) is disabled before configuring
  HWREG(WTIMER3_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TBEN;

  // set it up in 32bit wide (individual, not concatenated) mode
  // the constant name derives from the 16/32 bit timer, but this is a 32/64
  // bit timer so we are setting the 32bit mode
  HWREG(WTIMER3_BASE + TIMER_O_CFG) = TIMER_CFG_16_BIT;

  // we want to use the full 32 bit count, so initialize the Interval Load
  // register to 0xffff.ffff (its default value :-)
  HWREG(WTIMER3_BASE + TIMER_O_TAILR) = 0xffffffff;

  // we want to use the full 32 bit count, so initialize the Interval Load
  // register to 0xffff.ffff (its default value :-)
  HWREG(WTIMER3_BASE + TIMER_O_TBILR) = 0xffffffff;

  // set up timer A in capture mode (TAMR=3, TAAMS = 0),
  // for edge time (TACMR = 1) and up-counting (TACDIR = 1)
  HWREG(WTIMER3_BASE + TIMER_O_TAMR) =
      (HWREG(WTIMER3_BASE + TIMER_O_TAMR) & ~TIMER_TAMR_TAAMS) |
      (TIMER_TAMR_TACDIR | TIMER_TAMR_TACMR | TIMER_TAMR_TAMR_CAP);

  // set up timer B in capture mode (TAMR=3, TAAMS = 0),
  // for edge time (TACMR = 1) and up-counting (TACDIR = 1)
  HWREG(WTIMER3_BASE + TIMER_O_TBMR) =
      (HWREG(WTIMER3_BASE + TIMER_O_TBMR) & ~TIMER_TBMR_TBAMS) |
      (TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP);

  // To set the event to rising edge, we need to modify the TAEVENT bits
  // in GPTMCTL. Rising edge = 00, so we clear the TAEVENT bits
  HWREG(WTIMER3_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TAEVENT_M;

  // To set the event to rising edge, we need to modify the TBEVENT bits
  // in GPTMCTL. Rising edge = 00, so we clear the TBEVENT bits
  HWREG(WTIMER3_BASE + TIMER_O_CTL) &= ~TIMER_CTL_TBEVENT_M;

  // Now Set up the port to do the capture (clock was enabled earlier)
  // start by setting the alternate function for Port D bit 2 (WT3CCP0)
  HWREG(GPIO_PORTD_BASE + GPIO_O_AFSEL) |= BIT2HI;

  // Then, map bit 2's alternate function to WT3CCP0
  // 7 is the mux value to select WT3CCP0, 8 to shift it over to the
  // right nibble for bit 2 (4 bits/nibble * 2 bits)
  HWREG(GPIO_PORTD_BASE + GPIO_O_PCTL) =
      (HWREG(GPIO_PORTD_BASE + GPIO_O_PCTL) & 0xfffff0ff) + (7 << (2 * BitsPerNibble));

  // Enable pin 2 on Port D for digital I/O
  HWREG(GPIO_PORTD_BASE + GPIO_O_DEN) |= BIT2HI;

  // make pin 2 on Port D into an input
  HWREG(GPIO_PORTD_BASE + GPIO_O_DIR) &= BIT2LO;

  // Now Set up the port to do the capture (clock was enabled earlier)
  // start by setting the alternate function for Port D bit 3 (WT3CCP1)
  HWREG(GPIO_PORTD_BASE + GPIO_O_AFSEL) |= BIT3HI;

  // Then, map bit 3's alternate function to WT3CCP1
  // 7 is the mux value to select WT3CCP1, 12 to shift it over to the
  // right nibble for bit 3 (4 bits/nibble * 3 bits)
  HWREG(GPIO_PORTD_BASE + GPIO_O_PCTL) =
      (HWREG(GPIO_PORTD_BASE + GPIO_O_PCTL) & 0xffff0fff) + (7 << (3 * BitsPerNibble));
  // Enable pin 3 on Port D for digital I/O
  HWREG(GPIO_PORTD_BASE + GPIO_O_DEN) |= BIT3HI;

  // make pin 3 on Port D into an input
  HWREG(GPIO_PORTD_BASE + GPIO_O_DIR) &= BIT3LO;

  // back to the timer to enable a local capture interrupt
//  HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_CAEIM;

  // back to the timer to enable a local capture interrupt
//  HWREG(WTIMER3_BASE+TIMER_O_IMR) |= TIMER_IMR_CBEIM;

  // enable the nvic interrupts
  HWREG(NVIC_EN3) |= (BIT4HI | BIT5HI);

  // now kick the timer off by enabling it and enabling the timer to
  // stall while stopped by the debugger
  HWREG(WTIMER3_BASE + TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);

  // now kick the timer off by enabling it and enabling the timer to
  // stall while stopped by the debugger
  HWREG(WTIMER3_BASE + TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
}

//Init PWM to the motors (PB6 and PB7), reload emitter (PB4), retroreflective emitter (PB5),
//flywheel (PD0), servo for ball storage/release mechanism (PE5), servo for flags (PE4),
//servo for phototransistor(s) (PD1)
void InitPWM(void)
{
  //Enable the clock to Module 0 of PWM
  HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0;
  //Select the PWM clock as System Clock / 32
  HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
      (SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32);
  //Wait until the clock has started
  while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
  {}

  #ifdef TestPWM

  //Disable PWM0 (PB6 and PB7) while initializing
  HWREG(PWM0_BASE + PWM_O_0_CTL) = 0;
  // program generators to go to 1 at rising compare A/B, 0 on falling compare A/B
  HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
  HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
  //Set period
  HWREG(PWM0_BASE + PWM_O_0_LOAD) = LOAD_VALUE;
  //Set value at which pin changes state
  HWREG(PWM0_BASE + PWM_O_0_CMPA) = LOAD_VALUE >> 1;
  HWREG(PWM0_BASE + PWM_O_0_CMPB) = LOAD_VALUE >> 1;
  //Enable PWM to motors in InitMotorService

  //Disable PWM1 (PB4 and PB5) while initializing
  HWREG(PWM0_BASE + PWM_O_1_CTL) = 0;
  // program generators to go to 1 at rising compare A/B, 0 on falling compare A/B
  HWREG(PWM0_BASE + PWM_O_1_GENA) = GenA_Normal;
  HWREG(PWM0_BASE + PWM_O_1_GENB) = GenB_Normal;
  //Set half the period (load = period/4/32 - adjusting for difference in clock to PWM and timers)
  HWREG(PWM0_BASE + PWM_O_1_LOAD) = RETROREFLECTIVE_SEND_PERIOD >> 1;
  //Set value at which pin changes state (50% duty cycle)
  HWREG(PWM0_BASE + PWM_O_1_CMPA) = RETROREFLECTIVE_SEND_PERIOD >> 2;
  HWREG(PWM0_BASE + PWM_O_1_CMPB) = RETROREFLECTIVE_SEND_PERIOD >> 2;
  //Enable PWM output (DONT ENABLE, WAIT FOR SM TO ENABLE IT)
//  HWREG(PWM0_BASE + PWM_O_ENABLE) |= (PWM_ENABLE_PWM3EN);

  //Disable PWM2 (PE4 and PE5) while initializing
  HWREG(PWM0_BASE + PWM_O_2_CTL) = 0;
  // program generators to go to 1 at rising compare A/B, 0 on falling compare A/B
  HWREG(PWM0_BASE + PWM_O_2_GENA) = GenA_Normal;
  HWREG(PWM0_BASE + PWM_O_2_GENB) = GenB_Normal;
  //Set period
  HWREG(PWM0_BASE + PWM_O_2_LOAD) = LOAD_VALUE_SERVO;
  //Set value at which pin changes state
  HWREG(PWM0_BASE + PWM_O_2_CMPA) = SERVO_CMP_CENTER;
  HWREG(PWM0_BASE + PWM_O_2_CMPB) = SERVO_CMP_CENTER;
  //Enable PWM output for ball storage servo and flag servo
  HWREG(PWM0_BASE + PWM_O_ENABLE) |= (PWM_ENABLE_PWM4EN | PWM_ENABLE_PWM5EN);

  //Disable PWM3 (PD0 and PD1) while initializing NOT USING PD1
  HWREG(PWM0_BASE + PWM_O_3_CTL) = 0;
  // program generators to go to 1 at rising compare A/B, 0 on falling compare A/B
  HWREG(PWM0_BASE + PWM_O_3_GENA) = GenA_Normal;
  HWREG(PWM0_BASE + PWM_O_3_GENB) = GenB_Normal;
  //Set period
  HWREG(PWM0_BASE + PWM_O_3_LOAD) = LOAD_VALUE_SERVO;
  //Set value at which pin changes state
  uint32_t DesiredHighTime = (LOAD_VALUE_SERVO * FLYWHEEL_DUTY_CYCLE) / 100;
  // Set the Duty cycle on A by programming the compare value
  // to the required duty cycle of Period/2 - DesiredHighTime/2
  HWREG(PWM0_BASE + PWM_O_3_CMPA) = (uint16_t)(LOAD_VALUE_SERVO - DesiredHighTime);
  HWREG(PWM0_BASE + PWM_O_3_CMPB) = LOAD_VALUE_SERVO - SERVO_CMP_RIGHT;
  //Don't enable PWM output to flywheel (0V across leads when MOSFET is off)

  //Select alternate functions for PB6 and PB7 and PB4 and PB5
  HWREG(GPIO_PORTB_BASE + GPIO_O_AFSEL) |= (BIT4HI | BIT5HI | BIT6HI | BIT7HI);
  //Map PWM to PB6 and PB7 and PB4 and PB5. 4 comes from Table 23-5 on Page 1351 of TIVA datasheet
  HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) = (HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL)
      & 0x0000ffff) + (4 << (7 * BitsPerNibble)) + (4 << (6 * BitsPerNibble)) + (4 << (5 * BitsPerNibble)) + (4 << (4 * BitsPerNibble));
  //Set PB6 and PB7 and PB4 and PB5 as digital
  HWREG(GPIO_PORTB_BASE + GPIO_O_DEN) |= (BIT4HI | BIT5HI | BIT6HI | BIT7HI);
  //Set PB6 and PB7 and PB4 and PB5 as outputs
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) |= (BIT4HI | BIT5HI | BIT6HI | BIT7HI);

  //Select an alternate function for PD0 and PD1
  HWREG(GPIO_PORTD_BASE + GPIO_O_AFSEL) |= (BIT0HI | BIT1HI);
  //Map PWM to PD0 and PD1. 4 comes from Table 23-5 on Page 1351 of TIVA datasheet
  HWREG(GPIO_PORTD_BASE + GPIO_O_PCTL) = (HWREG(GPIO_PORTD_BASE + GPIO_O_PCTL)
      & 0xffffff00) + (4 << (0 * BitsPerNibble)) + (4 << (1 * BitsPerNibble));
  //Set PD0 and PD1 as digital
  HWREG(GPIO_PORTD_BASE + GPIO_O_DEN) |= (BIT0HI | BIT1HI);
  //Set PD0 and PD1 as output
  HWREG(GPIO_PORTD_BASE + GPIO_O_DIR) |= (BIT0HI | BIT1HI);

  //Select alternate functions for PE4 and PE5
  HWREG(GPIO_PORTE_BASE + GPIO_O_AFSEL) |= (BIT4HI | BIT5HI);
  //Map PWM to PE4 and PE5. 4 comes from Table 23-5 on Page 1351 of TIVA datasheet
  HWREG(GPIO_PORTE_BASE + GPIO_O_PCTL) = (HWREG(GPIO_PORTE_BASE + GPIO_O_PCTL)
      & 0xff00ffff) + (4 << (4 * BitsPerNibble)) + (4 << (5 * BitsPerNibble));
  //Set PE4 and PE5 as digital
  HWREG(GPIO_PORTE_BASE + GPIO_O_DEN) |= (BIT4HI | BIT5HI);
  //Set PE4 and PE5 as outputs
  HWREG(GPIO_PORTE_BASE + GPIO_O_DIR) |= (BIT4HI | BIT5HI);

  //Make all the enables locally synchronized
//  HWREG(PWM0_BASE + PWM_O_ENUPD) = ((PWM_ENUPD_ENUPD2_M & PWM_ENUPD_ENUPD2_LSYNC) |
//      (PWM_ENUPD_ENUPD3_M & PWM_ENUPD_ENUPD3_LSYNC) | (PWM_ENUPD_ENUPD4_M & PWM_ENUPD_ENUPD4_LSYNC) |
//      (PWM_ENUPD_ENUPD5_M & PWM_ENUPD_ENUPD5_LSYNC) | (PWM_ENUPD_ENUPD6_M & PWM_ENUPD_ENUPD6_LSYNC) |
//      (PWM_ENUPD_ENUPD7_M & PWM_ENUPD_ENUPD7_LSYNC));
  //Set up+down count mode, enable PWM generator, and make generate update locally
  //synchronized to zero count
  HWREG(PWM0_BASE + PWM_O_0_CTL) = (PWM_0_CTL_MODE | PWM_0_CTL_ENABLE |
      PWM_0_CTL_GENAUPD_LS | PWM_0_CTL_GENBUPD_LS);
  //Set up+down count mode, enable PWM generator, and make generate update locally
  //synchronized to zero count
  HWREG(PWM0_BASE + PWM_O_1_CTL) = (PWM_1_CTL_MODE | PWM_1_CTL_ENABLE |
      PWM_1_CTL_GENAUPD_LS | PWM_1_CTL_GENBUPD_LS);
  //Set up+down count mode, enable PWM generator, and make generate update locally
  //synchronized to zero count
  HWREG(PWM0_BASE + PWM_O_2_CTL) = (PWM_2_CTL_MODE | PWM_2_CTL_ENABLE |
      PWM_2_CTL_GENAUPD_LS | PWM_2_CTL_GENBUPD_LS);
  //Set up+down count mode, enable PWM generator, and make generate update locally
  //synchronized to zero count
  HWREG(PWM0_BASE + PWM_O_3_CTL) = (PWM_3_CTL_MODE | PWM_3_CTL_ENABLE |
      PWM_3_CTL_GENAUPD_LS | PWM_3_CTL_GENBUPD_LS);

  #endif
}

//Init SPI function using pins PA2-5
void InitSPI(void)
{
  //Enable the clock to the GPIO port A
  HWREG(SYSCTL_RCGCGPIO) |= BIT0HI;
  // Enable the clock to SSI module
  HWREG(SYSCTL_RCGCSSI) |= BIT0HI;
  // Wait for the GPIO port to be ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R0) != SYSCTL_PRGPIO_R0)
  {}
  // Program the GPIO to use the alternate functions on the SSI pins
  HWREG(GPIO_PORTA_BASE + GPIO_O_AFSEL) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI);
  // Set mux position in GPIOPCTL to select the SSI use of the pins
  HWREG(GPIO_PORTA_BASE + GPIO_O_PCTL) = (HWREG(GPIO_PORTA_BASE + GPIO_O_PCTL) & 0xff0000ff) \
      + (2 << (5 * BitsPerNibble)) + (2 << (4 * BitsPerNibble)) + \
      (2 << (3 * BitsPerNibble)) + (2 << (2 * BitsPerNibble));
  // Program the port lines for digital I/O
  HWREG(GPIO_PORTA_BASE + GPIO_O_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI);
  // Program the required data directions on the port lines
  HWREG(GPIO_PORTA_BASE + GPIO_O_DIR) |= ((BIT2HI | BIT3HI | BIT5HI) & BIT4LO);
  // If using SPI mode 3, program the pull-up on the clock line
  HWREG(GPIO_PORTA_BASE + GPIO_O_PUR) |= BIT2HI;
  // Wait for the SSI0 to be ready
  while ((HWREG(SYSCTL_PRSSI) & SYSCTL_PRSSI_R0) != SYSCTL_PRSSI_R0)
  {}
  // Make sure that the SSI is disabled before programming mode bits
  HWREG(SSI0_BASE + SSI_O_CR1) &= BIT1LO;
  // Select master mode (MS) & TXRIS indicating End of Transmit (EOT)
  HWREG(SSI0_BASE + SSI_O_CR1) |= (SSI_CR1_EOT & (~SSI_CR1_MS));
  // Configure the SSI clock source to the system clock
  HWREG(SSI0_BASE + SSI_O_CC) = SSI_CC_CS_SYSPLL;
  // Configure the clock pre-scaler: max frequency 961kHz
  // SSInClk = SysClk / (CPSDVSR *(1+SCR)), we want CPSDVSR*(1+SCR) > 2642 (CPSR = 54, SCR = 50)
  HWREG(SSI0_BASE + SSI_O_CPSR) = PreScaler;
  // Configure clock rate (SCR), phase & polarity (SPH, SPO), mode (FRF), data size (DSS)
  HWREG(SSI0_BASE + SSI_O_CR0) |= (SSI_CR0_SPH | SSI_CR0_SPO | SSI_CR0_DSS_8) | (ClockRate << SSI_CR0_SCR_S); // +7 for 8-bit data size
  // Locally enable interrupts (TXIM in SSIIM)
  HWREG(SSI0_BASE + SSI_O_IM) |= SSI_IM_TXIM;
  // Make sure that the SSI is enabled for operation
  HWREG(SSI0_BASE + SSI_O_CR1) |= SSI_CR1_SSE;
}
