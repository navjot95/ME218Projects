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

#define LED_PERIOD 3


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
    
  //enable clock to PWM module (PWM0)  
  HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0; 
  // start by enabling the clock to the PWM Module (PWM1)
  HWREG(SYSCTL_RCGCPWM) |=SYSCTL_RCGCPWM_R1;
  
  //enable the clock to port A
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R0;   // for fuel LED  
  
  
  //enable the clock to port B
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1; 
  
  //enable the clock to port C
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R2; 
  
  //select PWM clock as System Clock/32
  HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) | (SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32); 
  
  //make sure PWM module clock has gotten going
  while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0) {
  }
  // make sure that the PWM module clock has gotten going
  while((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R1) != SYSCTL_PRPWM_R1){             
  }
    
  //disable the PWM0 (PB6 and PB7) while initializing
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
  
  
  //Disable PWM0 (PB4 and PB5) while initializing
  HWREG(PWM0_BASE + PWM_O_1_CTL) = 0;
  // program generators to go to 1 at rising compare A/B, 0 on falling compare A/B
  HWREG(PWM0_BASE+PWM_O_1_GENA) = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
  HWREG(PWM0_BASE+PWM_O_1_GENB) = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
  //Set half the period (load = period/4/32 - adjusting for difference in clock to PWM and timers)
  HWREG(PWM0_BASE + PWM_O_1_LOAD) = PWM_5KHZ; // 5kHz at 125
  //Set value at which pin changes state (50% duty cycle)
  HWREG(PWM0_BASE + PWM_O_1_CMPA) = HWREG(PWM0_BASE+PWM_O_0_LOAD) >> 1;
  HWREG(PWM0_BASE + PWM_O_1_CMPB) = HWREG(PWM0_BASE+PWM_O_0_LOAD) >> 1;
  //Enable PWM output 
  HWREG(PWM0_BASE + PWM_O_ENABLE) |= (PWM_ENABLE_PWM2EN);
  
 
   // disable the PWM0 (PC4 and PC5) on gen3 while initializing
   HWREG( PWM0_BASE + PWM_O_3_CTL) =0;
   // program generators to go to 1 at rising compare A, 0 on falling compare A 
   // GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO )
   HWREG( PWM0_BASE+PWM_O_3_GENA) = (PWM_3_GENA_ACTCMPAU_ONE | PWM_3_GENA_ACTCMPAD_ZERO );
   HWREG( PWM0_BASE+PWM_O_3_GENB) = (PWM_3_GENB_ACTCMPBU_ONE | PWM_3_GENB_ACTCMPBD_ZERO );
   // Set the PWM period
   HWREG( PWM0_BASE+PWM_O_3_LOAD) = LED_PERIOD; 
   //Set value at which pin changes state (50% duty cycle)
   HWREG(PWM0_BASE + PWM_O_3_CMPA) = HWREG(PWM0_BASE+PWM_O_3_LOAD) >> 1;
   HWREG(PWM0_BASE + PWM_O_3_CMPB) = HWREG(PWM0_BASE+PWM_O_3_LOAD) >> 1;                 
   // enable the PWM outputs
   HWREG( PWM0_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM6EN);         
       

  
  //select alternatue function on PWM pins for PB6, PB7, PB4, PB5
  HWREG(GPIO_PORTB_BASE + GPIO_O_AFSEL) |= (BIT7HI | BIT6HI | BIT4HI | BIT5HI); //corresponds to PB6 and PB7
  //map PWM to PB6, PB7, PB4, PB5
  HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) = (HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) & 0x00ffffff) + (4<<(7*BitsPerNibble)) + (4<<(6*BitsPerNibble)) + (4<<(4*BitsPerNibble)) + (4<<(5*BitsPerNibble)); 
  //enable Pin 4, 5, 6 and 7 on Port B for digital I/O
  HWREG(GPIO_PORTB_BASE + GPIO_O_DEN) |= (BIT7HI | BIT6HI | BIT4HI | BIT5HI); 
  //make pin 4, 5, 6  and 7 on Port B into output
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) |= (BIT7HI | BIT6HI | BIT4HI | BIT5HI); 
  //Set up/down count mode, enable PWM generator and make both generator updates locally synchronized to zero count
  HWREG(PWM0_BASE + PWM_O_0_CTL) = (PWM_0_CTL_MODE | PWM_0_CTL_ENABLE | PWM_0_CTL_GENAUPD_LS | PWM_0_CTL_GENBUPD_LS); 
  HWREG(PWM0_BASE + PWM_O_1_CTL) = (PWM_1_CTL_MODE | PWM_1_CTL_ENABLE | PWM_1_CTL_GENAUPD_LS | PWM_1_CTL_GENBUPD_LS); 
  
  //select alternate function on PWM pins for PC4, PC5
  HWREG(GPIO_PORTC_BASE+GPIO_O_AFSEL) |= (BIT4HI | BIT5HI);
  // now choose to map PWM to those pins, this is a mux value of 4 that we
  HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) |= (HWREG(GPIO_PORTC_BASE+GPIO_O_PCTL) &0xfff0ffff) + (4<<(4*BitsPerNibble))+ (4<<(5*BitsPerNibble));
  // Enable pins 4 and 5 on Port C for digital I/O
  HWREG(GPIO_PORTC_BASE+GPIO_O_DEN) |= (BIT4HI | BIT5HI);
  // make pins 4 and 5 on Port C into outputs
  HWREG(GPIO_PORTC_BASE+GPIO_O_DIR) |= (BIT4HI | BIT5HI);
  // set the up/down countmode, enable the PWMgenerator and make
  // both generator updates locally synchronized to zero count
  HWREG(PWM0_BASE+ PWM_O_3_CTL) |= (PWM_3_CTL_MODE|PWM_3_CTL_ENABLE| PWM_3_CTL_GENAUPD_LS);



    // disable the PWM (PA6) while initializing
    HWREG( PWM1_BASE+PWM_O_1_CTL) =0;
    // program generators to go to 1 at rising compare A, 0 on falling compare A 
    HWREG( PWM1_BASE+PWM_O_1_GENA) = (PWM_1_GENA_ACTCMPAU_ONE | PWM_1_GENA_ACTCMPAD_ZERO );
    // Set the PWM period
    HWREG( PWM1_BASE+PWM_O_1_LOAD) = LED_PERIOD; 
    // Set the PWM duty
    HWREG(PWM1_BASE+PWM_O_1_CMPA) = HWREG(PWM1_BASE+PWM_O_1_LOAD) >> 1;  
    // enable the PWM outputs
    HWREG( PWM1_BASE+PWM_O_ENABLE) |= (PWM_ENABLE_PWM2EN);
    // now configure the Port B pin to be PWM outputs
    // start by selecting the alternate function for PA6
    HWREG(GPIO_PORTA_BASE+GPIO_O_AFSEL) |= (BIT6HI);
    // now choose to map PWM tothose pins, this is a mux value of 4 that we
    // want to use for specifying the function on bits 6
    HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) =
    (HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) &0xf0ffffff) + (5<<(6*BitsPerNibble));
    // Enable pins 6 on Port A for digital I/O
    HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= (BIT6HI);
    // make pins 6 on Port A into outputs
    HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= (BIT6HI);
    // set the up/down countmode, enable the PWMgenerator and make
    // both generator updates locally synchronized to zero count
    HWREG(PWM1_BASE+ PWM_O_1_CTL) = (PWM_1_CTL_MODE|PWM_1_CTL_ENABLE|
    PWM_1_CTL_GENAUPD_LS);
  
  
  StopFanMotors(); 
  changePumpPower(false);
  setCurrTeamLED(PURPLE); 
  setHomeTeamLED(true); 
  powerFuelLEDs(true);  
  
}

void MoveForward(uint32_t ForwardSpeed){
  MoveFanMotors(ForwardSpeed,ForwardSpeed, true); 
}


void MoveBackward(uint32_t BackwardSpeed){
    MoveFanMotors(BackwardSpeed, BackwardSpeed, false); 
}

void StopFanMotors(void){
    MoveFanMotors(0,0, true); 
}

//set pwm duty (0-100) for left fan and right fan 
void MoveFanMotors(uint32_t LeftSpeed, uint32_t RightSpeed, bool dir){
    //make sure input is between 0 and 100, corresponding to duty
  if(LeftSpeed <= MIN_DUTY_VAL) //negative u means have gone over desired value 
    LeftSpeed = MIN_DUTY_VAL; 
  else if(LeftSpeed > MAX_DUTY_VAL)
    LeftSpeed = MAX_DUTY_VAL; 
      
  if(RightSpeed <= MIN_DUTY_VAL) //negative u means have gone over desired value 
    RightSpeed = MIN_DUTY_VAL; 
  else if(RightSpeed > MAX_DUTY_VAL)
    RightSpeed = MAX_DUTY_VAL;    
   
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
  
  //NEED TO IMPLEMENT DIRECTION 
  //for now IN2 for both motors set to low in main.c initialization 
  
}

void changePumpPower(bool turnOn){
    if(turnOn){
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT2HI;  
    }
    else {
        HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT2LO; 
    }        
}

/* 
//set the pwm duty (0-100) for the pump motor 
void SetPumpSpeed(uint32_t pumpDuty){
     //make sure input is between 0 and 100, corresponding to duty
  if(pumpDuty <= MIN_DUTY_VAL) //negative u means have gone over desired value 
    pumpDuty = MIN_DUTY_VAL; 
  else if(pumpDuty > MAX_DUTY_VAL)
    pumpDuty = MAX_DUTY_VAL; 
      
      
  //convert duty cycle (0-100) to pwm comparator value (0-MAX_PWM_RAW_VALUE) 
  uint32_t compVal = (((pumpDuty) * (MAX_PWM_RAW_VALUE)) / (MAX_DUTY_VAL));    
      
  //comVal inversely proportional to duty cycle, so subtract from max value 
  compVal = MAX_PWM_RAW_VALUE - compVal;       
  
  //update pwm duty cycle for left fan 
  if(compVal >= (MAX_PWM_RAW_VALUE)){ 
     HWREG(PWM0_BASE + PWM_O_1_GENA) = PWM_1_GENA_ACTZERO_ZERO;
  }
  else if(compVal <= 0) {
    HWREG(PWM0_BASE + PWM_O_1_GENA) = PWM_1_GENA_ACTZERO_ONE;
  }
  else {
    //set to normal action first
    HWREG(PWM0_BASE + PWM_O_1_GENA) = (PWM_1_GENA_ACTCMPAU_ONE | PWM_1_GENA_ACTCMPAD_ZERO); 
    HWREG(PWM0_BASE + PWM_O_1_CMPA) = compVal;
  } 
} */

//powering valves closes them, (valves are normally open)
//if closing a valve, have to make sure the other is open so flow isn't clogged 
void changeFlow(bool toTank){
    if(toTank){
        HWREG(GPIO_PORTA_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT3LO; //open tank valve
        HWREG(GPIO_PORTA_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT4HI; //close shoot valve
    }
    else {
        HWREG(GPIO_PORTA_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT3HI; //close tank valve 
        HWREG(GPIO_PORTA_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT4LO; //open shoot valve
    }        
}



//FUNCTIONS FOR LIGHTS  
void powerFuelLEDs(bool turnOn){
    if(turnOn){
        HWREG(PWM1_BASE+PWM_O_1_CMPA) = HWREG(PWM1_BASE+PWM_O_1_LOAD) >> 1;       
    }
    else {
        HWREG(PWM1_BASE+PWM_O_1_CMPA) = PWM_0_GENA_ACTZERO_ONE;        
    }
}

void setHomeTeamLED(bool isRed){
    if(isRed){
        HWREG(PWM0_BASE + PWM_O_3_CMPA) = HWREG(PWM0_BASE+PWM_O_3_LOAD) >> 1; //turn red on at 50% duty (PC4)
        HWREG(PWM0_BASE + PWM_O_1_CMPA) = PWM_0_GENA_ACTZERO_ONE; //turn home blue off (PB4)
     
    }
    else {
        HWREG(PWM0_BASE + PWM_O_3_CMPA) = PWM_0_GENA_ACTZERO_ONE; //turn red off (PC4)
        HWREG(PWM0_BASE + PWM_O_1_CMPA) = HWREG(PWM0_BASE+PWM_O_0_LOAD) >> 1; //turn home blue on (PB4)
    }
}

//purple is 0, red is 1, blue is 2 
void setCurrTeamLED(uint8_t color){
    
  switch (color) 
  {
    case PURPLE: 
    {
      HWREG(PWM0_BASE + PWM_O_3_CMPB) = HWREG(PWM0_BASE+PWM_O_3_LOAD) >> 1; //turn red on at 50% duty
      HWREG(PWM0_BASE + PWM_O_1_CMPB) = HWREG(PWM0_BASE+PWM_O_0_LOAD) >> 1; //turn blue on at 50% duty  
      break;
    }
    
    case RED: 
    {
      HWREG(PWM0_BASE + PWM_O_3_CMPB) = HWREG(PWM0_BASE+PWM_O_3_LOAD) >> 1; //turn red on at 50% duty
      HWREG(PWM0_BASE + PWM_O_1_CMPB) = PWM_0_GENB_ACTZERO_ONE; //turn blue off
      break;
    }
    
    case BLUE: 
    {
      HWREG(PWM0_BASE + PWM_O_3_CMPB) = PWM_0_GENB_ACTZERO_ONE; //turn red off 
      HWREG(PWM0_BASE + PWM_O_1_CMPB) = HWREG(PWM0_BASE+PWM_O_0_LOAD) >> 1; //turn blue on at 50% duty
      break;
    }
  }
}



/***************************************************************************
 private functions
 ***************************************************************************/

