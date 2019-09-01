/****************************************************************************
 Module
   HSMReloading.c

 Revision
   2.0.1

 Description
   This is a Reloading file for implementing state machines.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/27/17 09:48 jec      another correction to re-assign both CurrentEvent
                         and ReturnEvent to the result of the During function
                         this eliminates the need for the prior fix and allows
                         the during function to-remap an event that will be
                         processed at a higher level.
 02/20/17 10:14 jec      correction to Run function to correctly assign
                         ReturnEvent in the situation where a lower level
                         machine consumed an event.
 02/03/16 12:38 jec      updated comments to reflect changes made in '14 & '15
                         converted unsigned char to bool where appropriate
                         spelling changes on true (was True) to match standard
                         removed local var used for debugger visibility in 'C32
                         commented out references to Start & RunLowerLevelSM so
                         that this can compile.
 02/07/13 21:00 jec      corrections to return variable (should have been
                         ReturnEvent, not CurrentEvent) and several EV_xxx
                         event names that were left over from the old version
 02/08/12 09:56 jec      revisions for the Events and Services Framework Gen2
 02/13/10 14:29 jec      revised Start and run to add new kind of entry function
                         to make implementing history entry cleaner
 02/13/10 12:29 jec      added NewEvent local variable to During function and
                         comments about using either it or Event as the return
 02/11/10 15:54 jec      more revised comments, removing last comment in during
                         function that belongs in the run function
 02/09/10 17:21 jec      updated comments about internal transitions on During funtion
 02/18/09 10:14 jec      removed redundant call to RunLowerlevelSM in EV_Entry
                         processing in During function
 02/20/07 21:37 jec      converted to use enumerated type for events & states
 02/13/05 19:38 jec      added support for self-transitions, reworked
                         to eliminate repeated transition code
 02/11/05 16:54 jec      converted to implment hierarchy explicitly
 02/25/03 10:32 jec      converted to take a passed event parameter
 02/18/99 10:19 jec      built Reloading from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Reloading_SM.h"
#include "PlayService.h"
#include "MotorService.h"
#include "GamePlayHSM.h"
#include "LineFollowing_SM.h"
#include "REFService.h"

#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_nvic.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_gpio.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE ROTATING_TO_BEACON
#define HANDSHAKE_DURATION 2000
#define ROTATING_DURATION 60 //100
#define OVERTIME_FLAG 5

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringRotatingBeacon(ES_Event_t Event);
static ES_Event_t DuringLineFollowingReloader(ES_Event_t Event);
static ES_Event_t DuringReading(ES_Event_t Event);
static ES_Event_t DuringWaitingBall(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static ReloadingState_t CurrentState;
static uint32_t         NumEdges;
static uint32_t         LastPeriod;
static uint32_t         LastCapture;
static int8_t           NumBalls;
static bool             RightSwitchHit;
static bool             LeftSwitchHit;
static bool             Reloading = true;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunReloadingSM

 Parameters
   ES_Event_t: the event to process

 Returns
   ES_Event_t: an event to return

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 2/11/05, 10:45AM
****************************************************************************/
ES_Event_t RunReloadingSM(ES_Event_t CurrentEvent)
{
  bool              MakeTransition = false;/* are we making a state transition? */
  ReloadingState_t  NextState = CurrentState;
  ES_Event_t        EntryEventKind = { ES_ENTRY, 0 }; // default to normal entry to new state
  ES_Event_t        ReturnEvent = CurrentEvent;       // assume we are not consuming event

  switch (CurrentState)
  {
    case ROTATING_TO_BEACON:  // If current state is state one
    {                         // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringRotatingBeacon(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT) //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_RELOADER_DETECTED:  //If event is event one
          {                           // Execute action function for state one : event one

            //start timer to turn more
            ES_Timer_InitTimer(RELOADING_ROTATE_TIMER, ROTATING_DURATION);
            HWREG(WTIMER3_BASE + TIMER_O_IMR) &= ~TIMER_IMR_CBEIM;

            // for internal transitions, skip changing MakeTransition
//          MakeTransition = true; //mark that we are taking a transition
            // if transitioning to a state with history change kind of entry
            //EntryEventKind.EventType = ES_ENTRY_HISTORY;
            // optionally, consume or re-map this event for the upper
            // level state machine
            ReturnEvent.EventType = ES_NO_EVENT;
          }
          break;
          // repeat cases as required for relevant events

          case ES_TIMEOUT:  //If event is event one
          {                 // Execute action function for state one : event one
            if (CurrentEvent.EventParam == RELOADING_ROTATE_TIMER)
            {
              NextState = LINE_FOLLOWING_RELOADING;//Decide what the next state will be

              // for internal transitions, skip changing MakeTransition
              MakeTransition = true; //mark that we are taking a transition
              // if transitioning to a state with history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;
              // optionally, consume or re-map this event for the upper
              // level state machine
              ReturnEvent.EventType = ES_NO_EVENT;
            }
          }
          break;
        }
      }
    }
    break;

    case LINE_FOLLOWING_RELOADING:  // If current state is state one
    {                               // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringLineFollowingReloader(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT) //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_SWITCH_HIT:     //If event is event one
          {                       // Execute action function for state one : event one
            NextState = READING;  //Decide what the next state will be

            // for internal transitions, skip changing MakeTransition
            MakeTransition = true; //mark that we are taking a transition
            // if transitioning to a state with history change kind of entry
            //EntryEventKind.EventType = ES_ENTRY_HISTORY;
            // optionally, consume or re-map this event for the upper
            // level state machine
            ReturnEvent.EventType = ES_NO_EVENT;
          }
          break;
            // repeat cases as required for relevant events
        }
      }
    }
    break;

    // repeat state pattern as required for other states
    case READING: // If current state is state one
    {             // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringReading(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT) //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_STATE_CHANGE:
          {
            if (CurrentEvent.EventParam == (PlayState_t)OFFENSE)
            {
              NextState = WAITING_FOR_BALL;//Decide what the next state will be

              // for internal transitions, skip changing MakeTransition
              MakeTransition = true; //mark that we are taking a transition
              // if transitioning to a state with history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;
              // optionally, consume or re-map this event for the upper
              // level state machine
              ReturnEvent.EventType = ES_NO_EVENT;
            }
          }
          break;
        }
      }
    }
    break;
    // repeat state pattern as required for other states

    case WAITING_FOR_BALL:        // If current state is state one
      // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringWaitingBall(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT) //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case ES_TIMEOUT:
          {
            if (CurrentEvent.EventParam == HANDSHAKE_TIMER)
            {
              ES_Event_t ThisEvent;
              ThisEvent.EventType = EV_STATE_CHANGE;
              ThisEvent.EventParam = (PlayState_t)OFFENSE;
              PostMasterSM(ThisEvent);
            }
          }
          break;
        }
      }
  }
  //   If we are making a state transition
  if (MakeTransition == true)
  {
    //   Execute exit function for current state
    CurrentEvent.EventType = ES_EXIT;
    RunReloadingSM(CurrentEvent);

    CurrentState = NextState; //Modify state variable

    //   Execute entry function for new state
    // this defaults to ES_ENTRY
    RunReloadingSM(EntryEventKind);
  }
  return ReturnEvent;
}

/****************************************************************************
 Function
     StartReloadingSM

 Parameters
     None

 Returns
     None

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 2/18/99, 10:38AM
****************************************************************************/
void StartReloadingSM(ES_Event_t CurrentEvent)
{
  // to implement entry to a history state or directly to a substate
  // you can modify the initialization of the CurrentState variable
  // otherwise just start in the entry state every time the state machine
  // is started
  if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
  {
    //either rotate or directly pidcontrol depending on where we are

    if (CurrentEvent.EventParam == OVERTIME_FLAG)
    {
      //enable local interrupt for handshake ir
      HWREG(WTIMER0_BASE + TIMER_O_IMR) |= TIMER_IMR_CAEIM;

      //Enable PWM output
      HWREG(PWM0_BASE + PWM_O_ENABLE) |= PWM_ENABLE_PWM2EN;
      NumEdges = 0;
    }
    if (Reloading)
    {
      CurrentState = LINE_FOLLOWING_RELOADING;
      Reloading = false;
    }
    else
    {
      CurrentState = ENTRY_STATE;     //rotating to reload beacon
    }
  }
  // call the entry function (if any) for the ENTRY_STATE
  RunReloadingSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryReloadingSM

 Parameters
     None

 Returns
     ReloadingState_t The current state of the Reloading state machine

 Description
     returns the current state of the Reloading state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
ReloadingState_t QueryReloadingSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

bool Check4LimitSwitches(void)
{
  bool        RightSwitch = HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) & BIT2HI;
  bool        LeftSwitch = HWREG(GPIO_PORTB_BASE + (GPIO_O_DATA + ALL_BITS)) & BIT3HI;
  ES_Event_t  SwitchEvent;
  SwitchEvent.EventType = EV_SWITCH_HIT;

  if (RightSwitch && !RightSwitchHit)
  {
    SwitchEvent.EventParam = 1;
    PostMasterSM(SwitchEvent);
    RightSwitchHit = RightSwitch;
    return true;
  }
  else if (LeftSwitch && !LeftSwitchHit)
  {
    SwitchEvent.EventParam = 0;
    PostMasterSM(SwitchEvent);
    LeftSwitchHit = LeftSwitch;
    return true;
  }
  RightSwitchHit = RightSwitch;
  LeftSwitchHit = LeftSwitch;
  return false;
}

void Handshake_ISR(void)
{
  uint32_t ThisCapture;

  // start by clearing the source of the interrupt, the input capture event
  HWREG(WTIMER0_BASE + TIMER_O_ICR) = TIMER_ICR_CAECINT;

  // now grab the captured value and calculate the period
  ThisCapture = HWREG(WTIMER0_BASE + TIMER_O_TAR);
  uint32_t Period = ThisCapture - LastCapture;

  NumEdges++;
  // If 10 cycles have passed and the period is now different, set new PWM output
  if ((NumEdges % 10 == 0) && (abs(Period - LastPeriod) > (Period / 100)))
  {
    //Disable PWM1 while initializing
    HWREG(PWM0_BASE + PWM_O_1_CTL) = 0;
    //Set half the period (load = period/4/32 - adjusting for difference in clock to PWM and timers)
    HWREG(PWM0_BASE + PWM_O_1_LOAD) = Period >> 7;
    //Set value at which pin changes state (50% duty cycle)
    HWREG(PWM0_BASE + PWM_O_1_CMPA) = Period >> 8;
    //Set up+down count mode, enable PWM generator, and make generate update locally
    //synchronized to zero count
    HWREG(PWM0_BASE + PWM_O_1_CTL) = (PWM_1_CTL_MODE | PWM_1_CTL_ENABLE |
        PWM_1_CTL_GENAUPD_LS | PWM_1_CTL_GENBUPD_LS);
    //Update last period for next compare
    LastPeriod = Period;
  }

  // update LastCapture to prepare for the next edge
  LastCapture = ThisCapture;
}

static ES_Event_t DuringRotatingBeacon(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    RotateLeft(65);
    HWREG(WTIMER3_BASE + TIMER_O_IMR) |= TIMER_IMR_CBEIM;
    ResetReloadEdges();

    // after that start any lower level machines that run in this state
    //StartLowerLevelSM( Event );
    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    //RunLowerLevelSM(Event);
    // repeat for any concurrently running state machines

    // now do any local exit functionality
    HWREG(WTIMER3_BASE + TIMER_O_IMR) &= ~TIMER_IMR_CBEIM;
    StopMotors();
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    // ReturnEvent = RunLowerLevelSM(Event);
    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

static ES_Event_t DuringLineFollowingReloader(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    // after that, start lower level machine that runS in this state
    StartLineFollowingSM(Event);
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    RunLineFollowingSM(Event);
    // repeat for any concurrently running state machines
    // now do any local exit functionality
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    ReturnEvent = RunLineFollowingSM(Event);

    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

static ES_Event_t DuringReading(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine

    //enable local interrupt for handshake ir
    HWREG(WTIMER0_BASE + TIMER_O_IMR) |= TIMER_IMR_CAEIM;

    //Enable PWM output
    HWREG(PWM0_BASE + PWM_O_ENABLE) |= PWM_ENABLE_PWM2EN;
    NumEdges = 0;

    ES_Timer_InitTimer(HANDSHAKE_TIMER, HANDSHAKE_DURATION);

    // after that start any lower level machines that run in this state
    //StartLowerLevelSM( Event );
    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    //RunLowerLevelSM(Event);
    // repeat for any concurrently running state machines
    // now do any local exit functionality

    // disable the Timer A in Wide Timer 0 interrupt in the NVIC
    // it is interrupt number 94 so appears in EN2 at bit 30
//        HWREG(NVIC_DIS2) = BIT30HI;
    HWREG(WTIMER0_BASE + TIMER_O_IMR) &= ~TIMER_IMR_CAEIM;
    //Disable PWM output
    HWREG(PWM0_BASE + PWM_O_ENABLE) &= ~PWM_ENABLE_PWM2EN;
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    // ReturnEvent = RunLowerLevelSM(Event);
    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

static ES_Event_t DuringWaitingBall(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    // after that start any lower level machines that run in this state
    //StartLowerLevelSM( Event );
    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    //RunLowerLevelSM(Event);
    // repeat for any concurrently running state machines
    // now do any local exit functionality
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    // ReturnEvent = RunLowerLevelSM(Event);
    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

int8_t GetNumBalls(void)
{
  return NumBalls;
}

void SetNumBalls(int8_t Num)
{
  NumBalls = Num;
}

void ResetFlag(void)
{
  Reloading = true;
}

//Timer info for reload beacon detection
//Timer B in Wide Timer 3 interrupt in the NVIC - interrupt number 101 so appears in EN3 at bit 5
