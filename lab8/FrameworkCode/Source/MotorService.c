#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"

#include "MotorService.h"

//for PWM definitions
#include "inc/hw_pwm.h"

//for timer definitions
//#include "inc/hw_timer.h"

//for NVIC definitions
//#include "inc/hw_nvic.h"

#define STOP 0x00
#define CW_90 0x02
#define CW_45 0x03
#define CCW_90 0x04
#define CCW_45 0x05
#define FORWARD_HALF 0x08
#define FORWARD_FULL 0x09
#define REVERSE_HALF 0x10
#define REVERSE_FULL 0x11
#define FIND_BEACON 0x20
#define FIND_TAPE 0x40

#define DURATION_90 1900
#define DURATION_45 1000

#define BITS_PER_NIBBLE 4

#define PWM_PIN_NUMBER_LEFT_MOTOR 6
#define PWM_PIN_NUMBER_RIGHT_MOTOR 7

//Frequency of 500 Hz
#define LOAD_VALUE 1250

#define COMPARE_VALUE LOAD_VALUE >> 1

static uint8_t MyPriority;
static MotorServiceState_t CurrentState;
static uint8_t LastBeaconState;
static uint8_t LastTapeState;
static uint8_t TapeFlag;
static uint16_t CompareValueB;
 
bool InitMotorService(uint8_t Priority)
{
  MyPriority = Priority;
  
  //Enable the clock to Module 0 of PWM
  HWREG(SYSCTL_RCGCPWM) |= SYSCTL_RCGCPWM_R0;

  //Enable the clock to Port B
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;

  //Wait till clock for Port B is ready
  while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1) != SYSCTL_PRGPIO_R1)
  {}

  //Select the PWM clock as System Clock / 32
  HWREG(SYSCTL_RCC) = (HWREG(SYSCTL_RCC) & ~SYSCTL_RCC_PWMDIV_M) |
      (SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_32);

  //Wait until the clock has started
  while ((HWREG(SYSCTL_PRPWM) & SYSCTL_PRPWM_R0) != SYSCTL_PRPWM_R0)
  {}
    
  //Set as digital
  HWREG(GPIO_PORTB_BASE + GPIO_O_DEN) |= (BIT0HI | BIT1HI | BIT2HI | BIT3HI | BIT4HI);

  //Set as outputs
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) |= (BIT0HI | BIT1HI | BIT2HI);
    
  //Set as inputs
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) &= (BIT3LO & BIT4LO);
    
  //Get initial state of pin for beacon
  LastBeaconState = (BIT4HI & HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)));
    
  //Get initial state of pin for tape
  LastTapeState = (BIT3HI & HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)));
    
  TapeFlag = 0;
  
  // Init PB2 to low for debugging IR sensor
  HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT2LO;
    
  CompareValueB = 625;
    
  return true;
}

bool PostMotorService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

ES_Event_t RunMotorService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;
  
  //Disable PWM while initializing
  HWREG(PWM0_BASE + PWM_O_0_CTL) = 0;
  
  
  
  if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == LED_TIMER))
  {
    HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT2LO;
  }
  
  
  
  if ((ThisEvent.EventType == ES_TIMEOUT) && (ThisEvent.EventParam == ROTATION_TIMER))
  {
    
    puts("timeout\r\n");
    //Set GenA and GenB for 0% duty cycle
    HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ZERO;
    HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ZERO;
   
    HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
    HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
  }
  
  else if ((ThisEvent.EventType == COMMAND_RECEIVED) && (ThisEvent.EventParam == FIND_BEACON))
//  else if ((ThisEvent.EventType == ES_NEW_KEY) && (ThisEvent.EventParam == 'b'))
  {
    puts("20 looking for beacon\r\n");
    CurrentState = LookingForBeacon;
    
    //Program generators to go to 1 at rising compare and 0 on falling compare
    uint32_t GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
    HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
    uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
    HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
    
    //Rotate
    HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT1HI;
    HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT0HI;
  }
  else if ((ThisEvent.EventType == COMMAND_RECEIVED) && (ThisEvent.EventParam == FIND_TAPE))
//  else if ((ThisEvent.EventType == ES_NEW_KEY) && (ThisEvent.EventParam == 't'))
  {
    puts("40 looking for tape\r\n");
    CurrentState = LookingForTape;
    
    TapeFlag = 1;
    
    //Program generators to go to 1 at rising compare and 0 on falling compare
    uint32_t GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
    HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
    uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
    HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
    
    //Set polarity
    //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
    //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
    
    //Set motor directions
    HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT0HI;
    HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
    CompareValueB = 687;
  }
  else if ((TapeFlag == 0) && (ThisEvent.EventType != TAPE_DETECTED) && (ThisEvent.EventType != BEACON_DETECTED))
  {
    CurrentState = IgnoringTapeAndBeacon;
    puts("ignoring tape and beacon\r\n");
  }
  
  if (ThisEvent.EventType != ES_TIMEOUT)
  {
    switch(CurrentState)
    {
      case IgnoringTapeAndBeacon:
      {
        if (ThisEvent.EventType == COMMAND_RECEIVED)
//        if (ThisEvent.EventType == ES_NEW_KEY)
        {
          if (ThisEvent.EventParam == STOP)
//          if (ThisEvent.EventParam == 's')
          {
            puts("motor: 0 stop\r\n");
            //Set GenA and GenB for 0% duty cycle
            HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ZERO;
            HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ZERO;
            
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
          }
          else if (ThisEvent.EventParam == CW_90)
//          else if (ThisEvent.EventParam == '2')
          {
            puts("motor: 2 cw_90\r\n");
            //Program generators to go to 1 at rising compare and 0 on falling compare
            uint32_t GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
            uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
            
            //Set polarity
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
            
            //Set motor directions
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT0HI;
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT1HI;
            
            ES_Timer_InitTimer(ROTATION_TIMER, DURATION_90);
          }
          else if (ThisEvent.EventParam == CW_45)
//          else if (ThisEvent.EventParam == '3')
          {
            puts("motor: 3 cw_45\r\n");
            //Program generators to go to 1 at rising compare and 0 on falling compare
            uint32_t GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
            uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
            
            //Set polarity
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
            
            //Set motor directions
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT0HI;
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT1HI;
            
            ES_Timer_InitTimer(ROTATION_TIMER, DURATION_45);
          }
          else if (ThisEvent.EventParam == CCW_90)
//          else if (ThisEvent.EventParam == '4')
          {
            puts("motor: 4 ccw_90\r\n");
            //Program generators to go to 1 at rising compare and 0 on falling compare
            uint32_t GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
            uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
            
            //Set polarity
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
            
            //Set motor directions
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
            
            ES_Timer_InitTimer(ROTATION_TIMER, DURATION_90);
          }
          else if (ThisEvent.EventParam == CCW_45)
//          else if (ThisEvent.EventParam == '5')
          {
            puts("motor: 5 ccw_45\r\n");
            //Program generators to go to 1 at rising compare and 0 on falling compare
            uint32_t GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
            uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
            
            //Set polarity
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
            
            //Set motor directions
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
            
            ES_Timer_InitTimer(ROTATION_TIMER, DURATION_45);
          }
          else if (ThisEvent.EventParam == FORWARD_HALF)
//          else if (ThisEvent.EventParam == '8')
          {
            puts("motor: 8 forward half\r\n");
            //Program generators to go to 1 at rising compare and 0 on falling compare
            uint32_t GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
            uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
            
            //Set polarity
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
            
            //Set motor directions
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT0HI;
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
            CompareValueB = 687;
          }
          else if (ThisEvent.EventParam == FORWARD_FULL)
//          else if (ThisEvent.EventParam == '9')
          {
            puts("motor: 9 forward full\r\n");
            
            HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ZERO;
            //HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ONE;
            uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
            
            //Set polarity
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
            
            //Set motor directions
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT0HI;
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
            CompareValueB = 62;
          }
          else if (ThisEvent.EventParam == REVERSE_HALF)
//          else if (ThisEvent.EventParam == 'v')
          {
            puts("motor: 10 reverse half\r\n");
            //Program generators to go to 1 at rising compare and 0 on falling compare
            uint32_t GenA_Normal = (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
            uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
            
            //Set polarity
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
            
            //Set motor directions
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT1HI;
            CompareValueB = 562;
            
          }
          else if (ThisEvent.EventParam == REVERSE_FULL)
//          else if (ThisEvent.EventParam == 'n')
          {
            puts("motor: 11 reverse full\r\n");
            //Set GenA and GenB for 100% duty cycle
            HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ONE;
            //HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ZERO;
            uint32_t GenB_Normal = (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO);
            HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
            
            //Set polarity
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
            //HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
            
            //Set motor directions
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
            HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT1HI;
            CompareValueB = 1188;
          }
        }
        break;
      }
      case LookingForTape:
      {
        if (ThisEvent.EventType == TAPE_DETECTED)
        {
          puts("tape detected\r\n");
          HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT2HI;
          ES_Timer_InitTimer(LED_TIMER, DURATION_45);
          HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ZERO;
          HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ZERO;
          
          HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
          HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
          
          
          TapeFlag = 0;
        }
        break;
      }
      case LookingForBeacon:
      {
        if (ThisEvent.EventType == BEACON_DETECTED)
        {
          //HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT2HI;
          puts("beacon detected\r\n");
          HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ZERO;
          HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ZERO;
          
          HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
          HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
          
          // PB2 low for debugging
          //HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT2LO;
        }
        break;
      }   
    }
  }
  
  //Set period
  HWREG(PWM0_BASE + PWM_O_0_LOAD) = LOAD_VALUE;

  //Set value at which pin changes state
  HWREG(PWM0_BASE + PWM_O_0_CMPA) = COMPARE_VALUE;
  HWREG(PWM0_BASE + PWM_O_0_CMPB) = CompareValueB;

  //Enable PWM output
  HWREG(PWM0_BASE + PWM_O_ENABLE) |= (PWM_ENABLE_PWM0EN | PWM_ENABLE_PWM1EN);

  //Select an alternate function for PB6 and PB7
  HWREG(GPIO_PORTB_BASE + GPIO_O_AFSEL) |= BIT6HI;
  HWREG(GPIO_PORTB_BASE + GPIO_O_AFSEL) |= BIT7HI;

  //Map PWM to PB6. 4 comes from Table 23-5 on Page 1351 of TIVA datasheet
  HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) = (HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) & 0xF0FFFFFF)
      + (4 << (PWM_PIN_NUMBER_LEFT_MOTOR * BITS_PER_NIBBLE));
  
  //Map PWM to PB7. 4 comes from Table 23-5 on Page 1351 of TIVA datasheet
  HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) = (HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) & 0x0FFFFFFF)
      + (4 << (PWM_PIN_NUMBER_RIGHT_MOTOR*BITS_PER_NIBBLE));

  //Set PB6 and PB7 as digital
  HWREG(GPIO_PORTB_BASE + GPIO_O_DEN) |= (BIT6HI | BIT7HI);

  //Set PB6 and PB7 as outputs
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) |= (BIT6HI | BIT7HI);

  //Set up+down count mode, enable PWM generator, and make generate update locally
  //synchronized to zero count - does this do it for both?
  HWREG(PWM0_BASE + PWM_O_0_CTL) = (PWM_0_CTL_MODE | PWM_0_CTL_ENABLE | PWM_0_CTL_GENAUPD_LS
      | PWM_0_CTL_GENBUPD_LS);

  
  return ReturnEvent;
}

bool Check4Beacon(void)
{
  ES_Event_t ThisEvent;
  ThisEvent.EventType = BEACON_DETECTED;
  uint8_t CurrentBeaconState;
  bool ReturnValue = false;
  
  //Get current state of beacon pin
  CurrentBeaconState = (BIT4HI & HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)));
  
//  if (CurrentBeaconState != LastBeaconState)
//  {
    if (CurrentBeaconState == 0)
    {
      PostMotorService(ThisEvent);
      printf("Posting beacon event");
      
      // Take PB2 hi for debugging
      //HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT2HI;
      
      //ES_Timer_InitTimer(LED_TIMER, DURATION_45);
      
      ReturnValue = true;
    }
//  }
  
  LastBeaconState = CurrentBeaconState;
  return ReturnValue;
}

bool Check4Tape(void)
{
  ES_Event_t ThisEvent;
  ThisEvent.EventType = TAPE_DETECTED;
  uint8_t CurrentTapeState;
  bool ReturnValue = false;
  
  //Get current state of tape pin
  CurrentTapeState = (BIT3HI & HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)));
  
//  if (CurrentTapeState != LastTapeState)
  {
    if (CurrentTapeState == 0)
    {
      PostMotorService(ThisEvent);
      ReturnValue = true;
    }
  }
  
  LastTapeState = CurrentTapeState;
  return ReturnValue;
}
