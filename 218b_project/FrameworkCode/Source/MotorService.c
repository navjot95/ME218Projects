/****************************************************************************
 Module
   MotorService.c

 Revision
   1.0.1

 Description
   This is a template file for implementing a simple service under the
   Gen2 Events and Services Framework.

 Notes

 History.00
 When           Who     What/Why
 -------------- ---     --------
 01/16/12 09:58 jec      began conversion from PWMFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "MotorService.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_pwm.h"
#include "inc/hw_gpio.h"

/*----------------------------- Module Defines ----------------------------*/

#define GenA_Normal (PWM_0_GENA_ACTCMPAU_ONE | PWM_0_GENA_ACTCMPAD_ZERO)
#define GenB_Normal (PWM_0_GENB_ACTCMPBU_ONE | PWM_0_GENB_ACTCMPBD_ZERO)
#define FULL_SPEED 50
#define NO_SPEED 0
#define NO_SPEED_REV 100

//Difference between driving the 2 motors
#define SPEED_RATIO_RL 1

//Testing defines, set to 1 if using, 0 if not
#define DEBUGGING 0
#define CHECKPOINT 0

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t              MyPriority;
static uint8_t              PWM0_100_A = 0;
static uint8_t              PWM0_100_B = 0;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMotorService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitMotorService(uint8_t Priority)
{
  ES_Event_t ThisEvent;
  MyPriority = Priority;

  //Enable PWM output to motors
  HWREG(PWM0_BASE + PWM_O_ENABLE) |= (PWM_ENABLE_PWM0EN | PWM_ENABLE_PWM1EN);
  StopMotors();

  //All pin inits done in main.c
  if (DEBUGGING)
  {
    //   ES_Timer_InitTimer(MOTOR_TEST_TIMER, 5000);
    printf("Starting motor test\n\r");
  }

  if (CHECKPOINT)
  {
//    ES_Timer_InitTimer(MOTOR_TEST_TIMER, 10000);
    printf("Starting motor checkpoint\n\r");
  }

  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
     PostMotorService

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostMotorService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMotorService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunMotorService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT;

//  if (DEBUGGING) {
//    switch (CurrentState)
//    {
//      case INIT:        // If current state is initial Psedudo State
//      {
//        if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
//        {
//          CurrentState = FORWARD;
//          printf("Driving forward\n\r");
//          DriveForward(35);
//          //ES_Timer_InitTimer(MOTOR_TEST_TIMER, 4000);
//        }
//      }
//      break;

//      case STOP:        // If current state is state one
//      {
//        switch (ThisEvent.EventType)
//        {
//          case ES_TIMEOUT:  //If event is event one
//          {   // Execute action function for state one : event one
//            if (ThisEvent.EventParam == MOTOR_TEST_TIMER) {
//              CurrentState = BACKWARD;  //Decide what the next state will be
//              printf("Driving backward\n\r");
//              DriveBackward(35);
//              ES_Timer_InitTimer(MOTOR_TEST_TIMER, 4000);
//            }
//          }
//          break;
//        }  // end switch on CurrentEvent
//      }
//      break;
//      // repeat state pattern as required for other states
//
//      case FORWARD:        // If current state is state one
//      {
//        switch (ThisEvent.EventType)
//        {
//          case ES_TIMEOUT:  //If event is event one
//          {   // Execute action function for state one : event one
//            if (ThisEvent.EventParam == MOTOR_TEST_TIMER) {
//              CurrentState = STOP;  //Decide what the next state will be
//              printf("Stopping\n\r");
//              StopMotors();
//              ES_Timer_InitTimer(MOTOR_TEST_TIMER, 2000);
//            }
//          }
//          break;
//        }  // end switch on CurrentEvent
//      }
//      break;
//
//      case BACKWARD:        // If current state is state one
//      {
//        switch (ThisEvent.EventType)
//        {
//          case ES_TIMEOUT:  //If event is event one
//          {   // Execute action function for state one : event one
//            if (ThisEvent.EventParam == MOTOR_TEST_TIMER) {
//              CurrentState = RIGHT;  //Decide what the next state will be
//              printf("Turning right\n\r");
//              RotateRight(35);
//              ES_Timer_InitTimer(MOTOR_TEST_TIMER, 4000);
//            }
//          }
//          break;
//        }  // end switch on CurrentEvent
//      }
//      break;
//
//      case RIGHT:        // If current state is state one
//      {
//        switch (ThisEvent.EventType)
//        {
//          case ES_TIMEOUT:  //If event is event one

//          {   // Execute action function for state one : event one
//            if (ThisEvent.EventParam == MOTOR_TEST_TIMER) {
//              CurrentState = LEFT;  //Decide what the next state will be
//              printf("Turning left\n\r");
//              RotateLeft(35);
//              ES_Timer_InitTimer(MOTOR_TEST_TIMER, 4000);
//            }
//          }
//          break;
//        }  // end switch on CurrentEvent
//      }
//      break;
//
//      case LEFT:        // If current state is state one
//      {
//        switch (ThisEvent.EventType)
//        {
//          case ES_TIMEOUT:  //If event is event one

//          {   // Execute action function for state one : event one
//            if (ThisEvent.EventParam == MOTOR_TEST_TIMER) {
//              CurrentState = FORWARD;  //Decide what the next state will be
//              printf("Driving forward\n\r");
//              DriveForward(35);
//              ES_Timer_InitTimer(MOTOR_TEST_TIMER, 4000);
//            }
//          }
//          break;
//        }  // end switch on CurrentEvent
//      }
//      break;
//      default:
//        ;
//    }                                   // end switch on Current State
//  }
//
//
//  if (CHECKPOINT) {
//    switch (CurrentState)
//    {
//      case INIT:        // If current state is initial Psedudo State
//      {
//        if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
//        {
//          CurrentState = FORWARD;
//          printf("Driving forward\n\r");
//          DriveForward(80);
//          ES_Timer_InitTimer(MOTOR_TEST_TIMER, 5000);
//        }
//      }
//      break;

//      case FORWARD:        // If current state is state one
//      {
//        switch (ThisEvent.EventType)
//        {
//          case ES_TIMEOUT:  //If event is event one
//          {   // Execute action function for state one : event one
//            if (ThisEvent.EventParam == MOTOR_TEST_TIMER) {
//              CurrentState = BACKWARD;  //Decide what the next state will be
//              printf("Driving backward\n\r");
//              DriveBackward(80);
//              ES_Timer_InitTimer(MOTOR_TEST_TIMER, 5000);
//            }
//          }
//          break;
//        }  // end switch on CurrentEvent
//      }
//      break;
//      // repeat state pattern as required for other states
//
//      case BACKWARD:        // If current state is state one
//      {
//        switch (ThisEvent.EventType)
//        {
//          case ES_TIMEOUT:  //If event is event one
//          {   // Execute action function for state one : event one
//            if (ThisEvent.EventParam == MOTOR_TEST_TIMER) {
//              CurrentState = STOP;  //Decide what the next state will be
//              printf("Stopping\n\r");
//              StopMotors();
//            }
//          }
//          break;
//        }  // end switch on CurrentEvent
//      }
//      break;
//
//      case STOP:        // If current state is state one
//      break;
//    }                                   // end switch on Current State
//  }
//
  return ReturnEvent;
}

// Functions for telling the motors to go forward/backward/left/right.
// To have individual control over the speed of the separate motors,
// use the SetDuty function individually on each motor.

void DriveForward(uint8_t Duty)
{
  SetDirection( true, true);
  SetDirection( true, false);
  SetDuty(Duty,                   true);
  SetDuty(Duty * SPEED_RATIO_RL,  false);
}

void DriveBackward(uint8_t Duty)
{
  SetDirection( false,  true);
  SetDirection( false,  false);
  SetDuty(Duty,                   true);
  SetDuty(Duty * SPEED_RATIO_RL,  false);
}

void RotateRight(uint8_t Duty)
{
  SetDirection( true,   false);
  SetDirection( false,  true);
  SetDuty(Duty,                   true);
  SetDuty(Duty * SPEED_RATIO_RL,  false);
}

void RotateLeft(uint8_t Duty)
{
  SetDirection( false,  false);
  SetDirection( true,   true);
  SetDuty(Duty,                   true);
  SetDuty(Duty * SPEED_RATIO_RL,  false);
}

void StopMotors(void)
{
  SetDirection( true, true);
  SetDirection( true, false);
  SetDuty(NO_SPEED, true);
  SetDuty(NO_SPEED, false);
}

//Sets the direction on a specific motor
void SetDirection(bool Forward, bool RightMotor)
{
  //Forward = !Forward; //needed this to make motor go forward - Navjot
  if (Forward)
  {
    if (RightMotor)
    {
      //Set polarity of left motor
      HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM0INV;
      //Set motor direction
      HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT0HI;
    }
    else
    {
      //Clear polarity of right motor
      HWREG(PWM0_BASE + PWM_O_INVERT) &= ~PWM_INVERT_PWM1INV;
      //Clear motor direction
      HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT1LO;
    }
  }
  else
  {
    if (RightMotor)
    {
      //Clear polarity of left motor
      HWREG(PWM0_BASE + PWM_O_INVERT) &= ~PWM_INVERT_PWM0INV;
      //Clear motor direction
      HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT0LO;
    }
    else
    {
      //Set polarity of right motor
      HWREG(PWM0_BASE + PWM_O_INVERT) |= PWM_INVERT_PWM1INV;
      //Set motor direction
      HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT1HI;
    }
  }
}

//Sets the duty cycle on a specific motor
void SetDuty(uint8_t Duty, bool RightMotor)
{
  uint32_t  Load = HWREG(PWM0_BASE + PWM_O_0_LOAD);
  uint32_t  DesiredHighTime = (Load * Duty) / 100;
  // Set the Duty cycle on A by programming the compare value
  // to the required duty cycle of Period/2 - DesiredHighTime/2
  uint16_t NewCMP = Load - DesiredHighTime;

  if (RightMotor)
  {
    if (Duty == NO_SPEED)
    {
      HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ZERO;
      PWM0_100_A = 1;
    }
    else if (Duty == 100)
    {
      HWREG(PWM0_BASE + PWM_O_0_GENA) = PWM_0_GENA_ACTZERO_ONE;
      PWM0_100_A = 1;
    }
    else if (PWM0_100_A == 1)
    {
      HWREG(PWM0_BASE + PWM_O_0_GENA) = GenA_Normal;
      PWM0_100_A = 0;
    }
    HWREG(PWM0_BASE + PWM_O_0_CMPA) = NewCMP;
  }
  else
  {
    if (Duty == NO_SPEED)
    {
      HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ZERO;
      PWM0_100_B = 1;
    }
    else if (Duty == 100)
    {
      HWREG(PWM0_BASE + PWM_O_0_GENB) = PWM_0_GENB_ACTZERO_ONE;
      PWM0_100_B = 1;
    }
    else if (PWM0_100_B == 1)
    {
      HWREG(PWM0_BASE + PWM_O_0_GENB) = GenB_Normal;
      PWM0_100_B = 0;
    }
    HWREG(PWM0_BASE + PWM_O_0_CMPB) = NewCMP;
  }
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
