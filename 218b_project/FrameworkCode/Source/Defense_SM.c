/****************************************************************************
 Module
   Defense_SM.c

 Revision
   2.0.1

 Description
   This is a Defense file for implementing state machines.

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
 02/18/99 10:19 jec      built Defense from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "Defense_SM.h"
#include "PlayService.h"
#include "REFService.h"
#include "GamePlayHSM.h"
#include "MotorService.h"
#include "Offense_SM.h"
#include "LineFollowing_SM.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"
#include "inc/hw_timer.h"
#include "ADMulti.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE ROTATING_CW
#define DEFAULT_DUTY_CYCLE 80
#define CONTROL_LAW_TIMER_DURATION 100
#define DEFENSE_TURN_DURATION 1000
#define DEFENSE_STRAIGHT_DURATION 800
#define ROTATE_MORE_DURATION 500
#define NUM_ADC_PINS 4
#define SHARP_ADC 2
#define SHARP_THRESHOLD 850 //2170 ~1.75 V; 6 inches
#define TURN_CW_SPEED 30
#define TURN_CCW_SPEED 30

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/

static ES_Event_t DuringRotatingCW(ES_Event_t Event);
static ES_Event_t DuringDrivingStraight(ES_Event_t Event);
static ES_Event_t DuringWaitingAtDefendGoal(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static DefenseState_t CurrentState;
static uint32_t       LastCount;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunDefense_SM

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
ES_Event_t RunDefenseSM(ES_Event_t CurrentEvent)
{
  bool            MakeTransition = false;/* are we making a state transition? */
  DefenseState_t  NextState = CurrentState;
  ES_Event_t      EntryEventKind = { ES_ENTRY, 0 }; // default to normal entry to new state
  ES_Event_t      ReturnEvent = CurrentEvent;       // assume we are not consuming event

  switch (CurrentState)
  {
    case ROTATING_CW: // If current state is state one
    {                 // Execute During function for state one. ES_ENTRY & ES_EXIT are
                      // processed here allow the lower level state machines to re-map
                      // or consume the event
      ReturnEvent = CurrentEvent = DuringRotatingCW(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_DEFEND_GOAL_DETECTED: //If event is event one
          {                             // Execute action function for state one : event one
            //we want to rotate a bit more after we see the goal
            ES_Timer_InitTimer(DEF_TIMER, ROTATE_MORE_DURATION);

            //disable goal interrupts so timer is able to finish
            HWREG(WTIMER3_BASE + TIMER_O_IMR) &= ~TIMER_IMR_CBEIM;

            // for internal transitions, skip changing MakeTransition
//                  MakeTransition = true; //mark that we are taking a transition
            // if transitioning to a state with history change kind of entry
            //EntryEventKind.EventType = ES_ENTRY_HISTORY;
            // optionally, consume or re-map this event for the upper
            // level state machine
            //ReturnEvent.EventType = ES_NO_EVENT;
          }
          break;

          case ES_TIMEOUT:  //If event is event one
          {                 // Execute action function for state one : event one
            if (CurrentEvent.EventParam == DEF_TIMER)
            {
              NextState = DRIVING_STRAIGHT;        //Decide what the next state will be

              // for internal transitions, skip changing MakeTransition
              MakeTransition = true;         //mark that we are taking a transition
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

    case DRIVING_STRAIGHT:  // If current state is state one
    {                       // Execute During function for state one. ES_ENTRY & ES_EXIT are
                            // processed here allow the lower level state machines to re-map
                            // or consume the event
      ReturnEvent = CurrentEvent = DuringDrivingStraight(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_OBJECT_DETECTED_SHARP:  //If event is event one
          {                               // Execute action function for state one : event one
            NextState = WAITING_AT_DEFEND_GOAL;

            // for internal transitions, skip changing MakeTransition
            MakeTransition = true;         //mark that we are taking a transition
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
    break;

    case WAITING_AT_DEFEND_GOAL:  // If current state is state one
    {                             // Execute During function for state one. ES_ENTRY & ES_EXIT are
                                  // processed here allow the lower level state machines to re-map
                                  // or consume the event
      ReturnEvent = CurrentEvent = DuringWaitingAtDefendGoal(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {}
      }
    }
    break;
  }

  //   If we are making a state transition
  if (MakeTransition == true)
  {
    //   Execute exit function for current state
    CurrentEvent.EventType = ES_EXIT;
    RunDefenseSM(CurrentEvent);

    CurrentState = NextState;    //Modify state variable

    //   Execute entry function for new state
    // this defaults to ES_ENTRY
    RunDefenseSM(EntryEventKind);
  }
  return ReturnEvent;
}

/****************************************************************************
 Function
     StartDefense_SM

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
void StartDefenseSM(ES_Event_t CurrentEvent)
{
  // to implement entry to a history state or directly to a substate
  // you can modify the initialization of the CurrentState variable
  // otherwise just start in the entry state every time the state machine
  // is started
  if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
  {
    CurrentState = ENTRY_STATE;
  }

  //No longer in face-off, flag for first reload should be cleared
  SetFaceoffFalse();
  // call the entry function (if any) for the ENTRY_STATE
  RunDefenseSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryDefense_SM

 Parameters
     None

 Returns
     DefenseState_t The current state of the Defense state machine

 Description
     returns the current state of the Defense state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
DefenseState_t QueryDefense_SM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringRotatingCW(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    RotateRight(TURN_CW_SPEED);

    // enable the Timer B in Wide Timer 3 interrupt in the NVIC (reloader ir)
    // it is interrupt number 101 so appears in EN3 at bit 5
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
    StopMotors();

    // disable the Timer B in Wide Timer 3 interrupt in the NVIC
    // it is interrupt number 97 so appears in EN3 at bit 5
    HWREG(WTIMER3_BASE + TIMER_O_IMR) &= ~TIMER_IMR_CBEIM;
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

static ES_Event_t DuringDrivingStraight(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    // drive forward toward goal, veer slightly to the right
    DriveForward(DEFAULT_DUTY_CYCLE);
    SetDuty(DEFAULT_DUTY_CYCLE + 0.1 * DEFAULT_DUTY_CYCLE, LEFT);

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

static ES_Event_t DuringWaitingAtDefendGoal(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    StopMotors();
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

// Checking for walls and opponents so that we don't hit them
bool Check4SharpEvents(void)
{
  bool      ReturnValue = false;
  uint32_t  Counts[NUM_ADC_PINS];
  ADC_MultiRead(Counts);
  uint32_t  Count = Counts[SHARP_ADC];

  if ((Count >= SHARP_THRESHOLD + 5) && (LastCount < SHARP_THRESHOLD - 5))
  {
    ReturnValue = true;
    ES_Event_t ThisEvent;
    ThisEvent.EventType = EV_OBJECT_DETECTED_SHARP;
    PostMasterSM(ThisEvent);
  }

  LastCount = Count;
  return ReturnValue;
}
