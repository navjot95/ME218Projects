// This module contains the logic for our shooting functionality

/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "GamePlayHSM.h"
#include "Shooting_SM.h"
#include "Reloading_SM.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"
#include "inc/hw_pwm.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE WAITING_FOR_FLYWHEEL
#define BALL_WHEEL_DURATION 250
#define SHOT_DURATION 1500
#define LOAD_VALUE_SERVO 12500
#define SERVO_CMP_CENTER 937  //1.5ms high time
#define SERVO_CMP_RIGHT 1562  //2.5ms high time

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/

static ES_Event_t DuringWaitingForFlywheel(ES_Event_t Event);
static ES_Event_t DuringWaitingForBallWheel(ES_Event_t Event);
static ES_Event_t DuringWaitingForShot(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static ShootingState_t CurrentState;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunShooting_SM

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
ES_Event_t RunShootingSM(ES_Event_t CurrentEvent)
{
  /* are we making a state transition? */
  bool            MakeTransition = false;
  ShootingState_t NextState = CurrentState;

  // default to normal entry to new state
  ES_Event_t EntryEventKind = { ES_ENTRY, 0 };

  // assume we are not consuming event
  ES_Event_t ReturnEvent = CurrentEvent;

  switch (CurrentState)
  {
    // In this state, we wait for the flywheel to get up and running
    case WAITING_FOR_FLYWHEEL:  // If current state is state one
    {                           // Execute During function for state one. ES_ENTRY & ES_EXIT are
                                // processed here allow the lower-level state machines to re-map
                                // or consume the event
      ReturnEvent = CurrentEvent = DuringWaitingForFlywheel(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case ES_TIMEOUT:       //If event is event one
          { if (CurrentEvent.EventParam == SHOOTING_TIMER)
            {
              // Execute action function for state one : event one
              // Decide what the next state will be
              NextState = WAITING_FOR_BALL_WHEEL;

              // Mark that we are taking a transition
              MakeTransition = true;

              // not transitioning to a state with
              // history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;

              // don't consume or re-map this event for the upper
              // level state machine
              //ReturnEvent.EventType = ES_NO_EVENT;
            }
          }
          break;
        }
      }
    }
    break;

    // In this state, we wait for the ball wheel to rotate the correct
    // amount
    case WAITING_FOR_BALL_WHEEL:  // If current state is state one
    {                             // Execute During function for state one. ES_ENTRY & ES_EXIT are
                                  // processed here allow the lower-level state machines to re-map
                                  // or consume the event
      ReturnEvent = CurrentEvent =
              DuringWaitingForBallWheel(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case ES_TIMEOUT:       //If event is event one
          { if (CurrentEvent.EventParam == BALL_WHEEL_TIMER)
            {
              // Execute action function for state one : event one
              // Decide what the next state will be
              NextState = WAITING_FOR_SHOT;

              //mark that we are taking a transition
              MakeTransition = true;

              // not transitioning to a state with
              // history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;

              // consume this event
              ReturnEvent.EventType = ES_NO_EVENT;

              //Assume that ball successfully exited
              SetNumBalls(GetNumBalls() - 1);
            }
          }
          break;
        }
      }
    }
    break;

    // In this state, we wait to receive word on whether we've scored
    case WAITING_FOR_SHOT:  // If current state is state one
    {                       // Execute During function for state one. ES_ENTRY & ES_EXIT are
                            // processed here allow the lower-level state machines to re-map
                            // or consume the event
      ReturnEvent = CurrentEvent = DuringWaitingForShot(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case ES_TIMEOUT:       //If event is event one

          { if (CurrentEvent.EventParam == SHOOTING_TIMER)
            {
              if (GetNumBalls() > 0)
              {
                // Execute action function for state one : event one
                // Decide what the next state will be
                NextState = WAITING_FOR_BALL_WHEEL;

                // mark that we are taking a transition
                MakeTransition = true;

                // for internal transitions,
                // skip changing MakeTransition
                //MakeTransition = true;

                // not transitioning to a state with
                // history change kind of entry
                //EntryEventKind.EventType = ES_ENTRY_HISTORY;

                // consume this event
                ReturnEvent.EventType = ES_NO_EVENT;
              }
              else
              {
                // if no balls remain, go to defense early
                ES_Event_t DefenseEvent;
                DefenseEvent.EventType = EV_EARLY_DEFENSE;
                PostMasterSM(DefenseEvent);
              }
            }
          }
          break;
        }
      }
    }
    break;
  }
  //   If we are making a state transition
  if (MakeTransition == true)
  {
    // Execute exit function for current state
    CurrentEvent.EventType = ES_EXIT;
    RunShootingSM(CurrentEvent);

    CurrentState = NextState;    //Modify state variable

    // Execute entry function for new state
    // this defaults to ES_ENTRY
    RunShootingSM(EntryEventKind);
  }
  return ReturnEvent;
}

/****************************************************************************
 Function
     StartShooting_SM

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
void StartShootingSM(ES_Event_t CurrentEvent)
{
  // to implement entry to a history state or directly to a substate
  // we can modify the initialization of the CurrentState variable
  // otherwise just start in the entry state every time the state machine
  // is started
  if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
  {
    CurrentState = ENTRY_STATE;
  }
  // call the entry function for the ENTRY_STATE
  RunShootingSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryShooting_SM

 Parameters
     None

 Returns
     ShootingState_t The current state of the Shooting state machine

 Description
     returns the current state of the Shooting state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
ShootingState_t QueryShooting_SM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringWaitingForFlywheel(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine

    // after that start any lower-level machines that run in this state
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first

    // now do any local exit functionality
  }
  else
  // do the 'during' function for this state
  {
    // run any lower-level state machine
  }
  return ReturnEvent;
}

static ES_Event_t DuringWaitingForBallWheel(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    ES_Timer_InitTimer(BALL_WHEEL_TIMER, BALL_WHEEL_DURATION);

    // start servo
    HWREG(PWM0_BASE + PWM_O_2_CMPB) = SERVO_CMP_RIGHT;

    // after that start any lower-level machines that run in this state
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first

    // now do any local exit functionality

    //Stop servo
    HWREG(PWM0_BASE + PWM_O_2_CMPB) = SERVO_CMP_CENTER;
  }
  else
  // do the 'during' function for this state
  {
    // run any lower-level state machine

    // do any activity that is repeated as long as we are in this state
  }
  return ReturnEvent;
}

static ES_Event_t DuringWaitingForShot(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    ES_Timer_InitTimer(SHOOTING_TIMER, SHOT_DURATION);

    // after that start any lower-level machines that run in this state
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first

    // now do any local exit functionality
  }
  else
  // do the 'during' function for this state
  {
    // run any lower-level state machine

    // do any activity that is repeated as long as we are in this state
  }
  return ReturnEvent;
}
