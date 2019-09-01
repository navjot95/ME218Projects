/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "PlayService.h"
#include "GamePlayHSM.h"
#include "RefService.h"
#include "Offense_SM.h"
#include "Defense_SM.h"
#include "MotorService.h"
#include "Reloading_SM.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_nvic.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE WAITING_TO_START

// used for play LED
#define RedTeam 1
#define BlueTeam 2
#define LED_VALUE 1 << 6
#define LED_DURATION 500

// used for ball detection
#define STARTING_NUM_BALLS 1

// used for flag servo
#define LOAD_VALUE_SERVO 12500
#define SERVO_CMP_LEFT 350
#define SERVO_CMP_CENTER 925  //1.5ms high time
#define SERVO_CMP_RIGHT 1400  //1500 //2.5ms high time

// used for beacon detecting circuits
#define RED_ATTACK_GOAL_PERIOD 32000
#define BLUE_ATTACK_GOAL_PERIOD 28000
#define RED_RELOAD_PERIOD 24000
#define BLUE_RELOAD_PERIOD 20000
#define PERIOD_ERROR 400
#define REMAINING_TIME_BEFORE_OVERTIME_RELOAD_DECISION 32500
#define A_LITTLE_BIT 500
#define TIME_REMAINING_AFTER_ONE_SHOT_TIMEOUT 38000
#define TIME_REMAINING (TIME_REMAINING_AFTER_ONE_SHOT_TIMEOUT - REMAINING_TIME_BEFORE_OVERTIME_RELOAD_DECISION) //time remaining in ms
#define TIME_REMAINING_ONE_TENTH_SECOND TIME_REMAINING / 100
#define REQ_NUM_EDGES 20

#define OVERTIME_FLAG 5

//#define MAKE_DEFENSE_ENTRY_STATE_ROTATING 10

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringWaitingToStart(ES_Event_t Event);
static ES_Event_t DuringFaceOff(ES_Event_t Event);
static ES_Event_t DuringOffense(ES_Event_t Event);
static ES_Event_t DuringDefense(ES_Event_t Event);
static ES_Event_t DuringGameOver(ES_Event_t Event);
static ES_Event_t DuringOvertime(ES_Event_t Event);

static void ReadTeamColor(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static PlayState_t  CurrentState;
static uint8_t      TeamColor;
static uint32_t     LastCaptureGoal;
static uint32_t     LastCaptureReload;
static uint32_t     NumGoalEdges;
static uint32_t     NumReloadEdges;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunPlayService

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
ES_Event_t RunPlayService(ES_Event_t CurrentEvent)
{
  bool        MakeTransition = false; /* are we making a state transition? */
  PlayState_t NextState = CurrentState;
  ES_Event_t  EntryEventKind = { ES_ENTRY, 0 }; // default to normal entry to new state
  ES_Event_t  ReturnEvent = CurrentEvent;       // assume we are not consuming event

  switch (CurrentState)
  {
    case WAITING_TO_START:
    {
      // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event

      ReturnEvent = CurrentEvent = DuringWaitingToStart(CurrentEvent);

      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_STATE_CHANGE: //If event is event one
          {                     // Execute action function for state one : event one
            NextState = (PlayState_t)CurrentEvent.EventParam;

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

          case ES_TIMEOUT:  //If event is event one
          {                 // Execute action function for state one : event one
            if (CurrentEvent.EventParam == LED_TIMER)
            {
              ES_Timer_InitTimer(LED_TIMER, LED_DURATION);
              //toggle LED status
              HWREG(GPIO_PORTD_BASE + (GPIO_O_DATA + ALL_BITS)) ^= LED_VALUE;
              // for internal transitions, skip changing MakeTransition
              //MakeTransition = true; //mark that we are taking a transition
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

    case FACE_OFF:  // If current state is state one
    {               // Execute During function for state one. ES_ENTRY & ES_EXIT are
                    // processed here allow the lower level state machines to re-map
                    // or consume the event
      ReturnEvent = CurrentEvent = DuringFaceOff(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_STATE_CHANGE: //If event is event one
          {                     // Execute action function for state one : event one
            NextState = (PlayState_t)CurrentEvent.EventParam;
            printf("Next State after FaceOff is %d\r\n", CurrentEvent.EventParam);       //should be 2

            // for internal transitions, skip changing MakeTransition
            MakeTransition = true;       //mark that we are taking a transition
            // if transitioning to a state with history change kind of entry
            EntryEventKind.EventType = ES_ENTRY;
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
    // repeat state pattern as required for other states

    case OFFENSE:
    {
      // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringOffense(CurrentEvent);

      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_STATE_CHANGE: //If event is event one
          {                     //printf("Next State after Offense is %d\r\n", CurrentEvent.EventParam); //should be 3
            NextState = (PlayState_t)CurrentEvent.EventParam;
            printf("EV state change event received in PlayService\r\n");
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

          case EV_EARLY_DEFENSE:      //If event is event one

          { NextState = DEFENSE;
            printf("EV early defense event received in PlayService\r\n");
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

          case ES_TIMEOUT:      //If event is event one
          { if (CurrentEvent.EventParam == GAME_TIMER)
            {
              //printf("Next State after Offense is %d\r\n", CurrentEvent.EventParam); //should be 3
              NextState = (PlayState_t)CurrentEvent.EventParam;
              printf("Early overtime timer timed out\r\n");
              if (GetShotClock() >= (uint32_t)TIME_REMAINING_ONE_TENTH_SECOND)
              {
                ES_Timer_InitTimer(GAME_TIMER_2, (uint32_t)A_LITTLE_BIT);
              }
              // for internal transitions, skip changing MakeTransition
              // if transitioning to a state with history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;
              // optionally, consume or re-map this event for the upper
              // level state machine
              //ReturnEvent.EventType = ES_NO_EVENT;
            }
            else if (CurrentEvent.EventParam == GAME_TIMER_2)
            {
              //printf("Next State after Offense is %d\r\n", CurrentEvent.EventParam); //should be 3
              NextState = (PlayState_t)CurrentEvent.EventParam;
              printf("Last overtime timer timed out\r\n");
              if (GetREDScore() == GetBLUEScore())
              {
                //checking for shot clock
                ES_Event_t OvertimeEvent;
                OvertimeEvent.EventType = EV_STATE_CHANGE;
                OvertimeEvent.EventParam = (PlayState_t)OVERTIME;       //this line and next weren't in code last night
                PostMasterSM(OvertimeEvent);
              }
              // for internal transitions, skip changing MakeTransition
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

    case DEFENSE:
    {
      // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringDefense(CurrentEvent);

      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_STATE_CHANGE:       //If event is event one
          { printf("Received state change with param %d\n\r", CurrentEvent.EventParam);
            if (CurrentEvent.EventParam != DEFENSE)
            {
              NextState = (PlayState_t)CurrentEvent.EventParam;
              printf("Changing state\n\r");
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

    case GAME_OVER:
    {
      // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringGameOver(CurrentEvent);

      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_STATE_CHANGE:                               //If event is event one
          {                                                   // Execute action function for state one : event one
            NextState = (PlayState_t)CurrentEvent.EventParam; //Decide what the next state will be

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

    case OVERTIME:
    {
      // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringOvertime(CurrentEvent);

      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_STATE_CHANGE:       //If event is event one
            // Execute action function for state one : event one

            if (CurrentEvent.EventType != OVERTIME)
            {
              NextState = (PlayState_t)CurrentEvent.EventParam;

              // for internal transitions, skip changing MakeTransition
              MakeTransition = true;     //mark that we are taking a transition
              // if transitioning to a state with history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;
              // optionally, consume or re-map this event for the upper
              // level state machine
              //ReturnEvent.EventType = ES_NO_EVENT;
              break;
            }
            // repeat cases as required for relevant events
        }
      }
    }
    break;
  }
  //   If we are making a state transition
  if (MakeTransition == true)
  {
    //   Execute exit function for current state
    CurrentEvent.EventType = ES_EXIT;
    ReturnEvent = RunPlayService(CurrentEvent);

    CurrentState = NextState;    //Modify state variable
//       if (Parameter == MAKE_DEFENSE_ENTRY_STATE_ROTATING)
//       {
//          EntryEventKind.EventParam = MAKE_DEFENSE_ENTRY_STATE_ROTATING;
//       }
    //   Execute entry function for new state
    // this defaults to ES_ENTRY
    RunPlayService(EntryEventKind);
  }
  return ReturnEvent;
}

/****************************************************************************
 Function
     StartPlayService

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
void StartPlayService(ES_Event_t CurrentEvent)
{
  // to implement entry to a history state or directly to a substate
  // you can modify the initialization of the CurrentState variable
  // otherwise just start in the entry state every time the state machine
  // is started
  if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
  {
    CurrentState = ENTRY_STATE;
  }
  // call the entry function (if any) for the ENTRY_STATE (NONE)
  //printf("about to call runplayservice\n\r");
  RunPlayService(CurrentEvent);
}

/****************************************************************************
 Function
     QueryPlayService

 Parameters
     None

 Returns
     PlayState_t The current state of the Play state machine

 Description
     returns the current state of the Play state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
PlayState_t QueryPlayService(void)
{
  return CurrentState;
}

uint8_t GetTeamColor(void)
{
  return TeamColor;
}

void ResetGoalEdges(void)
{
  NumGoalEdges = 0;
}

void ResetReloadEdges(void)
{
  NumReloadEdges = 0;
}

/***************************************************************************
 private functions
 ***************************************************************************/

//Reads switch for team and raises correct flag. If high then RED. If low then BLUE.
static void ReadTeamColor(void)
{
  uint8_t Color = HWREG(GPIO_PORTD_BASE + (GPIO_O_DATA + ALL_BITS)) & BIT7HI;
  if (Color)
  {
    TeamColor = RedTeam;
    printf("team color is red\r\n");
    //1.5ms high time
    HWREG(PWM0_BASE + PWM_O_2_CMPA) = SERVO_CMP_RIGHT;
  }
  else
  {
    TeamColor = BlueTeam;
    printf("team color is blue\r\n");
    //2.5ms high time
    HWREG(PWM0_BASE + PWM_O_2_CMPA) = SERVO_CMP_LEFT;
  }
}

static ES_Event_t DuringWaitingToStart(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    printf("CHANGING TO WAITING_TO_START\n\r");
    StopMotors();

    //Read input state for team color
    ReadTeamColor();
//        TeamColor = RedTeam;

    //Start blinking LED
    ES_Timer_InitTimer(LED_TIMER, LED_DURATION);

    //Set starting number of balls loaded
    SetNumBalls(STARTING_NUM_BALLS);

    //Reset reloading flag
    ResetFlag();

    // after that start any lower level machines that run in this state
    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    // repeat for any concurrently running state machines
    // now do any local exit functionality (NO EXIT FUNCTIONS)
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    //ReturnEvent = RunLowerLevelSM(Event);
    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

static ES_Event_t DuringFaceOff(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    printf("CHANGING TO FACE_OFF\n\r");

    //start game timer
    // now kick the timer off by enabling it and enabling the timer to
    // stall while stopped by the debugger
    HWREG(WTIMER1_BASE + TIMER_O_CTL) |= (TIMER_CTL_TAEN | TIMER_CTL_TASTALL);

    //Turn on LED indicating play
    HWREG(GPIO_PORTD_BASE + (GPIO_O_DATA + ALL_BITS)) |= BIT6HI;

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
    // now do any local exit functionality (NO EXIT FUNCTIONS)
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

static ES_Event_t DuringOffense(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    printf("CHANGING TO OFFENSE\n\r");

    //checking score
    ES_Event_t ScoreEvent;
    ScoreEvent.EventType = EV_SCORE_UPDATE;
    PostMasterSM(ScoreEvent);

    // after that start any lower level machines that run in this state
    StartOffenseSM(Event);

    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    ReturnEvent = RunOffenseSM(Event);

    // repeat for any concurrently running state machines
    // now do any local exit functionality (NO EXIT FUNCTIONS)
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    ReturnEvent = RunOffenseSM(Event);

    // repeat for any concurrent lower level machines

    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

static ES_Event_t DuringDefense(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    printf("CHANGING TO DEFENSE\n\r");
    // after that start any lower level machines that run in this state
    StartDefenseSM(Event);

    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    RunDefenseSM(Event);

    // repeat for any concurrently running state machines
    // now do any local exit functionality (NO EXIT FUNCTIONS)
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    ReturnEvent = RunDefenseSM(Event);

    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}

static ES_Event_t DuringGameOver(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    printf("CHANGING TO GAME_OVER\n\r");
    //Turn off LED indicating play
    HWREG(GPIO_PORTD_BASE + (GPIO_O_DATA + ALL_BITS)) &= BIT6LO;

    //Lower flag
    HWREG(PWM0_BASE + PWM_O_2_CMPA) = SERVO_CMP_CENTER;

    // after that start any lower level machines that run in this state
    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    // repeat for any concurrently running state machines
    // now do any local exit functionality (NO EXIT FUNCTIONS)
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

static ES_Event_t DuringOvertime(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    printf("CHANGING TO OVERTIME\n\r");
    Event.EventParam = OVERTIME_FLAG;
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
    // now do any local exit functionality (NO EXIT FUNCTIONS)
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

void Overtime_ISR(void)
{
  // start by clearing the source of the interrupt
  HWREG(WTIMER1_BASE + TIMER_O_ICR) = TIMER_ICR_TATOCINT;

  ES_Timer_InitTimer(GAME_TIMER, (uint32_t)REMAINING_TIME_BEFORE_OVERTIME_RELOAD_DECISION); //32.5 seconds after 100 seconds of wide timer

//  printf("CHECKING FOR OVERTIME\n\r");
}

void Goal_Beacon_ISR(void)
{
  uint32_t ThisCapture;
  // start by clearing the source of the interrupt, the input capture event
  HWREG(WTIMER3_BASE + TIMER_O_ICR) = TIMER_ICR_CAECINT;

  // now grab the captured value and calculate the period
  ThisCapture = HWREG(WTIMER3_BASE + TIMER_O_TAR);
  uint32_t Period = ThisCapture - LastCaptureGoal; //just changed from uint16_t to uint32_t

  if (((TeamColor == RedTeam) && (abs((int)Period - (int)RED_ATTACK_GOAL_PERIOD) < PERIOD_ERROR))
      || ((TeamColor == BlueTeam) && (abs((int)Period - (int)BLUE_ATTACK_GOAL_PERIOD) < PERIOD_ERROR)))
  {
    NumGoalEdges++;
    if (NumGoalEdges > REQ_NUM_EDGES)
    {
      ES_Event_t ThisEvent;
      ThisEvent.EventType = EV_ATTACK_GOAL_DETECTED;
      PostMasterSM(ThisEvent);
//      if (NumGoalEdges == 21) printf("GOAL DETECTED event sent to MasterSM\r\n");
    }
  }

  // update LastCapture to prepare for the next edge
  LastCaptureGoal = ThisCapture;
}

void Reload_Beacon_ISR(void)
{
  uint32_t ThisCapture;

  // start by clearing the source of the interrupt, the input capture event
  HWREG(WTIMER3_BASE + TIMER_O_ICR) = TIMER_ICR_CBECINT;

  // now grab the captured value and calculate the period
  ThisCapture = HWREG(WTIMER3_BASE + TIMER_O_TBR);
  uint16_t Period = ThisCapture - LastCaptureReload;

  //check for our own reloader beacon
  if (((TeamColor == RedTeam) && (abs((int)Period - (int)RED_RELOAD_PERIOD) < PERIOD_ERROR))
      || ((TeamColor == BlueTeam) && (abs((int)Period - (int)BLUE_RELOAD_PERIOD) < PERIOD_ERROR)))
  {
    NumReloadEdges++;
    if (NumReloadEdges > REQ_NUM_EDGES)
    {
      ES_Event_t ThisEvent;
      ThisEvent.EventType = EV_RELOADER_DETECTED;
      PostMasterSM(ThisEvent);
    }
  }

  //check for our own goal beacon in defense
  if (((TeamColor == RedTeam) && (abs((int)Period - (int)BLUE_ATTACK_GOAL_PERIOD) < PERIOD_ERROR))
      || ((TeamColor == BlueTeam) && (abs((int)Period - (int)RED_ATTACK_GOAL_PERIOD) < PERIOD_ERROR)))
  {
    NumReloadEdges++;
    if (NumReloadEdges > REQ_NUM_EDGES)
    {
      ES_Event_t ThisEvent;
      ThisEvent.EventType = EV_DEFEND_GOAL_DETECTED;
      PostMasterSM(ThisEvent);
    }
  }

  // update LastCapture to prepare for the next edge
  LastCaptureReload = ThisCapture;
}
