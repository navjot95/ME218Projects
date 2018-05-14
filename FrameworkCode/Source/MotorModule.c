/****************************************************************************
 Module
   TemplateFSM.c

 Revision
   1.0.1

 Description
   This is a template file for implementing flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/

#include "MotorModule.h"


/*----------------------------- Module Defines ----------------------------*/
//PWM Values
#define MAX_PWM_RAW_VALUE HWREG(PWM0_BASE+PWM_O_0_LOAD)
#define AD_MAX_VAL 4095
#define PWM_5KHZ 125
#define MIN_DUTY_VAL 0
#define MAX_DUTY_VAL 100

#define TICKS_PER_MS 40000
#define PeriodInMS 1
#define BitsPerNibble 4
#define PWMTicksPerMS 40000
#define ONE_SHOT_LENGTH PWMTicksPerMS*500 //500 ms 
#define PERIODIC_TIMER_LENGTH  (TICKS_PER_MS*2) //2 ms


/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

/*---------------------------- Module Variables ---------------------------*/





/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMotors

 Parameters
     none

 Returns
     none

 Description
     Initialize hardware
 Notes

 Author
     Navjot
****************************************************************************/
void InitFanPumpPWM()
{
  printf("Setting up PWM hw for fans and pump\n\r"); 
    
  //enable clock to PWM module (PWM0)  
  HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0; 
  
  //enable the clock to port B
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1; 
  
  //select PWM clock as System Clock/32
  HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) | (SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32); 
  
  //make sure PWM module clock has gotten goign
  while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0) {
   
  }
    
  //disable the PWM while initializing
  HWREG(PWM0_BASE+PWM_O_0_CTL) = 0; 
    
  //program generators to go to 1 at rising compare A, 0 on falling compare A
  HWREG(PWM0_BASE+PWM_O_0_GENA) = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
  //program generators to go to 1 at rising compare B, 0 on falling compare B
  HWREG(PWM0_BASE+PWM_O_0_GENB) = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
  //Set PWM period
  HWREG(PWM0_BASE+PWM_O_0_LOAD) = PWM_5KHZ; // 5kHz at 125
  //Set initial Duty cycle to 50% by programming compare value to 1/2 period to count up (or down)
  HWREG(PWM0_BASE+PWM_O_0_CMPA) = HWREG(PWM0_BASE+PWM_O_0_LOAD) >> 1;  
  //Set initial Duty cycle to 25% by programming compare value to 75% period to count up (or down)
  HWREG(PWM0_BASE+PWM_O_0_CMPB) = HWREG(PWM0_BASE+PWM_O_0_LOAD) - (((PeriodInMS * PWMTicksPerMS))>>3); 
  //enable PWM outputs 
  HWREG(PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM0EN | PWM_ENABLE_PWM1EN); 
  
  
  //select alternatue function on PWM pins
  HWREG(GPIO_PORTB_BASE + GPIO_O_AFSEL) |= (BIT7HI | BIT6HI); //corresponds to PB6 and PB7
  //map PWM to PB6 and PB7
  HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) = (HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) & 0x00ffffff) + (4<<(7*BitsPerNibble)) + (4<<(6*BitsPerNibble)); 
  //enable Pin 6 and 7 on Port B for digital I/O
  HWREG(GPIO_PORTB_BASE + GPIO_O_DEN) |= (BIT7HI | BIT6HI); 
  //make pin 6 on Port B into output
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) |= (BIT7HI | BIT6HI); 
  
  //Set up/down count mode, enable PWM generator and make both generator updates locally synchronized to zero count
  HWREG(PWM0_BASE + PWM_O_0_CTL) = (PWM_0_CTL_MODE | PWM_0_CTL_ENABLE | PWM_0_CTL_GENAUPD_LS | PWM_0_CTL_GENBUPD_LS); 
  
}

void MoveForward(uint32_t ForwardSpeed){
  MoveMotors(ForwardSpeed,ForwardSpeed); 
}


void StopMotors(void){
    MoveMotors(0,0); 
}


void MoveFanMotors(uint32_t LeftSpeed, uint32_t RightSpeed){
    //make sure input is between 0 and 100, corresponding to duty
  if(LeftSpeed <= MIN_DUTY_VAL) //negative u means have gone over desired value 
    LeftSpeed = MIN_DUTY_VAL; 
  else if(LeftSpeed > MAX_DUTY_VAL)
    LeftSpeed = MAX_DUTY_VAL; 
      
  if(RightSpeed <= MIN_DUTY_VAL) //negative u means have gone over desired value 
    RightSpeed = MIN_DUTY_VAL; 
  else if(RightSpeed > MAX_DUTY_VAL)
    RightSpeed = MAX_DUTY_VAL;    
   
  printf("max raw val: %d\n\r", HWREG(PWM0_BASE+PWM_O_0_LOAD)); 
  //convert duty cycle (0-100) to pwm comparator value (0-MAX_PWM_RAW_VALUE) 
  uint32_t compValL = (((LeftSpeed) * (MAX_PWM_RAW_VALUE)) / (MAX_DUTY_VAL));    
  uint32_t compValR = (((RightSpeed) * (MAX_PWM_RAW_VALUE)) / (MAX_DUTY_VAL)); 
      
  //comVal inversely proportional to duty cycle, so subtract from max value 
  compValL = MAX_PWM_RAW_VALUE - compValL;
  compValR = MAX_PWM_RAW_VALUE - compValR;         
  
  //update pwm duty cycle for left fan 
  if(compValL >= (MAX_PWM_RAW_VALUE)){ 
     HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ZERO;
  }
  else if(compValL <= 0) {
    HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ONE;
  }
  else {
    //set to normal action first
    HWREG(PWM0_BASE + PWM_O_0_GENA) = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO); 
    HWREG(PWM0_BASE + PWM_O_0_CMPA) = compValL;
  }
  
  //update pwm duty cycle for right fan 
  if(compValR >= (MAX_PWM_RAW_VALUE)){ 
     HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ZERO;
  }
  else if(compValR <= 0) {
    HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ONE;
  }
  else {
    //set to normal action first
    HWREG(PWM0_BASE + PWM_O_0_GENB) = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO); 
    HWREG(PWM0_BASE + PWM_O_0_CMPB) = compValR;
  }

}

void setPumpSpeed(uint32_t pumpDuty){
    
}

/***************************************************************************
 private functions
 ***************************************************************************/

