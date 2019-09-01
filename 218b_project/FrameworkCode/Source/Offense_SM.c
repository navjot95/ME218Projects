/****************************************************************************
 Module
   Offense_SM.c

 Revision
   2.0.1

 Description
   This is a Offense file for implementing state machines.

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
 02/18/99 10:19 jec      built Offense from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Offense_SM.h"
#include "PlayService.h"
#include "MotorService.h"
#include "Reloading_SM.h"
#include "GamePlayHSM.h"
#include "Shooting_SM.h"
#include "LineFollowing_SM.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"
#include "inc/hw_timer.h"
#include "inc/hw_pwm.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE RELOADING
#define FLYWHEEL_WAIT_DURATION 1000
#define DEFAULT_DUTY_CYCLE 60
#define RETROREFLECTIVE_SEND_PERIOD_HIGH 12000  //37000 //.24 ms nominal
#define RETROREFLECTIVE_SEND_PERIOD_LOW 8000    //33000
#define RETROREFLECTIVE_TIMER_DURATION 200      //500
#define MAKE_DEFENSE_ENTRY_STATE_ROTATING 10
#define DARK_HORSE_DURATION 1000 //was 1050
#define REVERSE_SPEED 80
#define RETROREFLECTIVE_SEND_PERIOD 300 //500 worked?

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, thibgungs like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringReloading(ES_Event_t Event);
static ES_Event_t DuringRotatingShoot(ES_Event_t Event);
static ES_Event_t DuringFindingShot(ES_Event_t Event);
static ES_Event_t DuringShooting(ES_Event_t Event);
static ES_Event_t DuringMovingBackward(ES_Event_t Event);
static ES_Event_t DuringRotatingToDefinitelyShoot(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static OffenseState_t CurrentState;
static uint32_t       LastCaptureRetroreflective;
static uint32_t       NumRetroEdges = 0;
static bool           FaceOff = true;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunOffenseSM

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
ES_Event_t RunOffenseSM(ES_Event_t CurrentEvent)
{
  bool            MakeTransition = false;/* are we making a state transition? */
  OffenseState_t  NextState = CurrentState;
  ES_Event_t      EntryEventKind = { ES_ENTRY, 0 }; // default to normal entry to new state
  ES_Event_t      ReturnEvent = CurrentEvent;       // assume we are not consuming event

  switch (CurrentState)
  {
    case RELOADING: // If current state is state one
    {               // Execute During function for state one. ES_ENTRY & ES_EXIT are
                    // processed here allow the lower level state machines to re-map
                    // or consume the event
      ReturnEvent = CurrentEvent = DuringReloading(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case ES_TIMEOUT:  //If event is event one
          {                 // Execute action function for state one : event one
            if (CurrentEvent.EventParam == HANDSHAKE_TIMER)
            {
              SetNumBalls(GetNumBalls() + 1);
              NextState = MOVING_BACKWARD;      //Decide what the next state will be
              // for internal transitions, skip changing MakeTransition
              MakeTransition = true;       //mark that we are taking a transition
              // if transitioning to a state with history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;
              // optionally, consume or re-map this event for the upper
              // level state machine
              //ReturnEvent.EventType = ES_NO_EVENT;
            }
          }
          break;
            // repeat cases as required for relevant events
        }
      }
    }
    break;
    // repeat state pattern as required for other states

    case ROTATING_TO_SHOOT: // If current state is state one
    {                       // Execute During function for state one. ES_ENTRY & ES_EXIT are
                            // processed here allow the lower level state machines to re-map
                            // or consume the event
      ReturnEvent = CurrentEvent = DuringRotatingShoot(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_ATTACK_GOAL_DETECTED:                                 //If event is event one
          {                                                             // Execute action function for state one : event one

            NextState = FINDING_SHOT;        //Decide what the next state will be

            // for internal transitions, skip changing MakeTransition
            MakeTransition = true;       //mark that we are taking a transition
            // if transitioning to a state with history change kind of entry
            //EntryEventKind.EventType = ES_ENTRY_HISTORY;
            // optionally, consume or re-map this event for the upper
            // level state machine
            //ReturnEvent.EventType = ES_NO_EVENT;
          }
          break;
            // repeat cases as required for relevant events
        }
      }
    }
    break;

    case FINDING_SHOT:  // If current state is state one
    {                   // Execute During function for state one. ES_ENTRY & ES_EXIT are
                        // processed here allow the lower level state machines to re-map
                        // or consume the event
      ReturnEvent = CurrentEvent = DuringFindingShot(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          // Execute action function for state one : event one
          case ES_TIMEOUT:
          {
            if (CurrentEvent.EventParam == RETROREFLECTIVE_TIMER)
            {
              NextState = SHOOTING;
              MakeTransition = true;
            }
          }
          break;

          case EV_OBJECT_DETECTED_RETRO:
          {
            ResetRetroEdges();
            NextState = MOVING_BACKWARD;
            MakeTransition = true;
          }
          break;
          case EV_EARLY_DEFENSE:
          {
            ReturnEvent.EventParam = MAKE_DEFENSE_ENTRY_STATE_ROTATING;
            MakeTransition = true;
          }
          break;
        }
      }
    }
    break;
    // for internal transitions, skip changing MakeTransition
    //mark that we are taking a transition
    // if transitioning to a state with history change kind of entry
    //EntryEventKind.EventType = ES_ENTRY_HISTORY;
    // optionally, consume or re-map this event for the upper
    // level state machine
    //ReturnEvent.EventType = ES_NO_EVENT;
    // repeat cases as required for relevant events

    case MOVING_BACKWARD:
    {
      ReturnEvent = CurrentEvent = DuringMovingBackward(CurrentEvent);
      if (CurrentEvent.EventType != ES_NO_EVENT)        //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case ES_TIMEOUT:       //If event is event one
          { if (CurrentEvent.EventParam == DARK_HORSE_TIMER)
            {
              // Execute action function for state one : event one

              NextState = ROTATING_TO_SHOOT;

              // for internal transitions, skip changing MakeTransition
              MakeTransition = true;       //mark that we are taking a transition
              // if transitioning to a state with history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;
              // optionally, consume or re-map this event for the upper
              // level state machine
              //ReturnEvent.EventType = ES_NO_EVENT;
            }
          }
          break;
        }
      }
    }
    break;
    case SHOOTING:  // If current state is state one
    {               // Execute During function for state one. ES_ENTRY & ES_EXIT are
                    // processed here allow the lower level state machines to re-map
                    // or consume the event
      ReturnEvent = CurrentEvent = DuringShooting(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
      }
    }
    break;
    case ROTATING_TO_DEFINITELY_SHOOT:
      ReturnEvent = CurrentEvent = DuringRotatingToDefinitelyShoot(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_ATTACK_GOAL_DETECTED:
          {
            // Execute action function for state one : event one

            NextState = SHOOTING;

            // for internal transitions, skip changing MakeTransition
            MakeTransition = true;       //mark that we are taking a transition
            // if transitioning to a state with history change kind of entry
            //EntryEventKind.EventType = ES_ENTRY_HISTORY;
            // optionally, consume or re-map this event for the upper
            // level state machine
            //ReturnEvent.EventType = ES_NO_EVENT;
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
    RunOffenseSM(CurrentEvent);

    CurrentState = NextState;    //Modify state variable

    //   Execute entry function for new state
    // this defaults to ES_ENTRY
    RunOffenseSM(EntryEventKind);
  }
  return ReturnEvent;
}

/****************************************************************************
 Function
     StartOffense_SM

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
void StartOffenseSM(ES_Event_t CurrentEvent)
{
  // to implement entry to a history state or directly to a substate
  // you can modify the initialization of the CurrentState variable
  // otherwise just start in the entry state every time the state machine
  // is started

  if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
  {
    if (FaceOff)
    {
      CurrentState = MOVING_BACKWARD;  //LINE_FOLLOWING_OFFENSE;
      FaceOff = false;
    }
    else if (GetNumBalls() > 1)
    {
      CurrentState = ROTATING_TO_SHOOT;
    }
    else
    {
      CurrentState = ENTRY_STATE;
    }
  }
  // call the entry function (if any) for the ENTRY_STATE
  RunOffenseSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryOffense_SM

 Parameters
     None

 Returns
     OffenseState_t The current state of the Offense state machine

 Description
     returns the current state of the Offense state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
OffenseState_t QueryOffenseSM(void)
{
  return CurrentState;
}

void SetFaceoffFalse(void)
{
  FaceOff = false;
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringReloading(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    // after that start any lower level machines that run in this state
    StartReloadingSM(Event);

    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    RunReloadingSM(Event);

    // repeat for any concurrently running state machines
    // now do any local exit functionality
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    ReturnEvent = RunReloadingSM(Event);

    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

//Do we need to start an event checker for a goal??
static ES_Event_t DuringRotatingShoot(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    
    //Looking for goal now, set number edges = 0
    ResetGoalEdges();
    
    // enable the Timer A in Wide Timer 3 interrupt in the NVIC (goal detection)
    // it is interrupt number 100 so appears in EN3 at bit 4
    HWREG(WTIMER3_BASE + TIMER_O_IMR) |= TIMER_IMR_CAEIM;


    // enable the Timer B in Wide Timer 1 interrupt in the NVIC
    // it is interrupt number 97 so appears in EN3 at bit 1
//        HWREG(NVIC_EN3) |= BIT1HI;

    //Begin Rotating
    RotateRight(DEFAULT_DUTY_CYCLE);

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
    StopMotors();

    // disable the Timer B in Wide Timer 3 interrupt in the NVIC
    // it is interrupt number 100 so appears in EN3 at bit 4 (goal detection)
    HWREG(WTIMER3_BASE + TIMER_O_IMR) &= ~TIMER_IMR_CAEIM;
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


static ES_Event_t DuringFindingShot(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    NumRetroEdges = 0;

    //Disable PWM1 (PB4 and PB5) while initializing
    HWREG(PWM0_BASE + PWM_O_1_CTL) = 0;
    //Set half the period (load = period/4/32 - adjusting for difference in clock to PWM and timers)
    HWREG(PWM0_BASE + PWM_O_1_LOAD) = RETROREFLECTIVE_SEND_PERIOD >> 1;
    //Set value at which pin changes state (50% duty cycle)
    HWREG(PWM0_BASE + PWM_O_1_CMPB) = RETROREFLECTIVE_SEND_PERIOD >> 2;
    //Set up+down count mode, enable PWM generator, and make generate update locally
    //synchronized to zero count
    HWREG(PWM0_BASE + PWM_O_1_CTL) = (PWM_1_CTL_MODE | PWM_1_CTL_ENABLE |
        PWM_1_CTL_GENAUPD_LS | PWM_1_CTL_GENBUPD_LS);

    //enable local interrupt for retroref receiver
    HWREG(WTIMER1_BASE + TIMER_O_IMR) |= TIMER_IMR_CBEIM;

    //Turn on retroreflective emitter
    HWREG(PWM0_BASE + PWM_O_ENABLE) |= PWM_ENABLE_PWM3EN;

    ES_Timer_InitTimer(RETROREFLECTIVE_TIMER, RETROREFLECTIVE_TIMER_DURATION);
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    //RunLowerLevelSM(Event);
    //Disable PWM3 (pin PB5)
    HWREG(PWM0_BASE + PWM_O_ENABLE) &= ~PWM_ENABLE_PWM3EN;
    // disable the Timer B in Wide Timer 1 interrupt in the NVIC
    // it is interrupt number 97 so appears in EN3 at bit 1
    HWREG(WTIMER1_BASE + TIMER_O_IMR) &= ~TIMER_IMR_CBEIM;
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

static ES_Event_t DuringShooting(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine

    //Turn on flywheel (enable PWM)
    HWREG(PWM0_BASE + PWM_O_ENABLE) |= PWM_ENABLE_PWM6EN;

    //Start timer for flywheel turning on
    ES_Timer_InitTimer(SHOOTING_TIMER, FLYWHEEL_WAIT_DURATION);

    // after that start any lower level machines that run in this state
    StartShootingSM(Event);

    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    RunShootingSM(Event);

    // repeat for any concurrently running state machines
    // now do any local exit functionality

    //Turn off flywheel (disable PWM to PD0)
    HWREG(PWM0_BASE + PWM_O_ENABLE) &= ~PWM_ENABLE_PWM6EN;

    //Get number of balls correct (if all balls gone, extra ghost ball so that
    //ball wheel turns the right amount next time)
    if (GetNumBalls() <= 0)
    {
      SetNumBalls(1);
    }
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    ReturnEvent = RunShootingSM(Event);

    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

static ES_Event_t DuringMovingBackward(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event; // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine

    //Start timer for flywheel turning on
    ES_Timer_InitTimer(DARK_HORSE_TIMER, DARK_HORSE_DURATION);
    DriveBackward(REVERSE_SPEED);

    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first

    // repeat for any concurrently running state machines
    // now do any local exit functionality

    StopMotors();
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine

    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

//Do we need to start an event checker for a goal??
static ES_Event_t DuringRotatingToDefinitelyShoot(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine

    //Begin Rotating
    RotateRight(DEFAULT_DUTY_CYCLE);

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
    StopMotors();

    // disable the Timer B in Wide Timer 3 interrupt in the NVIC
    // it is interrupt number 100 so appears in EN3 at bit 4 (goal detection)
    HWREG(WTIMER3_BASE + TIMER_O_IMR) &= ~TIMER_IMR_CAEIM;
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

void Retroreflective_ISR(void)
{
  uint32_t ThisCapture;

  // start by clearing the source of the interrupt, the input capture event
  HWREG(WTIMER1_BASE + TIMER_O_ICR) = TIMER_ICR_CBECINT;

  // now grab the captured value and calculate the period
  ThisCapture = HWREG(WTIMER1_BASE + TIMER_O_TBR);
  uint32_t Period = ThisCapture - LastCaptureRetroreflective;

  if ((Period < RETROREFLECTIVE_SEND_PERIOD_HIGH) && (Period > RETROREFLECTIVE_SEND_PERIOD_LOW))
  {
    NumRetroEdges++;
    if (NumRetroEdges > 20)
    {
      ES_Event_t ThisEvent;
      ThisEvent.EventType = EV_OBJECT_DETECTED_RETRO;
      ThisEvent.EventParam = Period;
      PostMasterSM(ThisEvent);
      HWREG(WTIMER1_BASE + TIMER_O_IMR) &= ~TIMER_IMR_CBEIM;
    }
  }
  else
  {
    NumRetroEdges = 0;
  }

  // update LastCapture to prepare for the next edge
  LastCaptureRetroreflective = ThisCapture;
}

void ResetRetroEdges(void)
{
  NumRetroEdges = 0;
}
