/****************************************************************************
 Module
   LineFollowing_SM.c

 Revision
   2.0.1

 Description
   This is a LineFollowing file for implementing state machines.

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
 02/18/99 10:19 jec      built LineFollowing from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "LineFollowing_SM.h"
#include "MotorService.h"
#include "ADMulti.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_nvic.h"
#include "inc/hw_timer.h"
#include "GamePlayHSM.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE DRIVING_FORWARD

#define INDUCTOR_THRESHOLD 3600

//For control law
#define OFFSET_SPEED_FORWARD 40.0 //duty cycle

#define LEFT_INDUCTOR_OFFSET 1170
#define LEFT_INDUCT 0
#define RIGHT_INDUCT 1
#define RIGHT_MOTOR true
#define LEFT_MOTOR false
#define MOTOR_FORWARD true
#define MOTOR_REVERSE false
#define ControlLawTime 1      //2
#define TurningWireTime 1200  //1150 was most recent but we underturned 1200 1000
#define RUNNING_AVG_ARRAY_LEN 8

#define SQUARE_UP_SPEED 80

#define STOP_TIME 5000

#define FORWARD_DUTY 100

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
static ES_Event_t DuringDrivingForward(ES_Event_t Event);
static ES_Event_t DuringPIDControl(ES_Event_t Event);
static ES_Event_t DuringSquareUp(ES_Event_t Event);

static float Clamp(float val, float clampL, float clampH);
static uint32_t getArrayAvg(uint32_t arrayToSum[]);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well
static LineFollowingState_t CurrentState;
static uint32_t             FieldStrengths[3];  //used to not have static declaration
static bool                 firstSwitchType;    //false means left, true means right
static bool                 OnWire;
static const float          pGain_forward = 0.020;// worked with offset speed 35

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunLineFollowingSM

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
ES_Event_t RunLineFollowingSM(ES_Event_t CurrentEvent)
{
  bool                  MakeTransition = false;/* are we making a state transition? */
  LineFollowingState_t  NextState = CurrentState;
  ES_Event_t            EntryEventKind = { ES_ENTRY, 0 }; // default to normal entry to new state
  ES_Event_t            ReturnEvent = CurrentEvent;       // assume we are not consuming event

  switch (CurrentState)
  {
    case DRIVING_FORWARD: // If current state is state one
    {                     // Execute During function for state one. ES_ENTRY & ES_EXIT are
                          // processed here allow the lower level state machines to re-map
                          // or consume the event
      ReturnEvent = CurrentEvent = DuringDrivingForward(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //is it ES_NO_Event or EV_LINE_HIT
      {
        switch (CurrentEvent.EventType)
        {
          case EV_LINE_HIT:           //If event is event one
          {                           // Execute action function for state one : event one
            NextState = PID_CONTROL;  //Decide what the next state will be
            // for internal transitions, skip changing MakeTransition
            MakeTransition = true;       //mark that we are taking a transition to next state
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

    case PID_CONTROL:
    {
      // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringPIDControl(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_SWITCH_HIT:
          {
            firstSwitchType = CurrentEvent.EventParam;       //sets module level var to which switch hit (left or right)
            // Execute action function for state one : event one
            NextState = SQUARE_UP;      //Decide what the next state will be
            // for internal transitions, skip changing MakeTransition
            MakeTransition = true;       //mark that we are taking a transition
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

    case SQUARE_UP: // If current state is state one
    {               // Execute During function for state one. ES_ENTRY & ES_EXIT are
                    // processed here allow the lower level state machines to re-map
                    // or consume the event
      ReturnEvent = CurrentEvent = DuringSquareUp(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_SWITCH_HIT:
          {
            // Execute action function for state one : event one
            NextState = SQUARE_UP;      //Decide what the next state will be
            // for internal transitions, skip changing MakeTransition
            MakeTransition = false;
            // if transitioning to a state with history change kind of entry
            //EntryEventKind.EventType = ES_ENTRY_HISTORY;
            // optionally, consume or re-map this event for the upper
            // level state machine
            if (CurrentEvent.EventParam != firstSwitchType)
            {
              ReturnEvent.EventType = EV_SWITCH_HIT;       //EV_SWITCH_HIT, so Reloading_SM moves to next state
            }
            else
            {
              //if same switch hit again, then ignore
              ReturnEvent.EventType = ES_NO_EVENT;
            }
          }
          break;
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
    RunLineFollowingSM(CurrentEvent);

    CurrentState = NextState;    //Modify state variable

    // Execute entry function for new state
    // this defaults to ES_ENTRY
    RunLineFollowingSM(EntryEventKind);
  }
  return ReturnEvent;
}

/****************************************************************************
 Function
     StartLineFollowingSM

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
void StartLineFollowingSM(ES_Event_t CurrentEvent)
{
  // to implement entry to a history state or directly to a substate
  // you can modify the initialization of the CurrentState variable
  // otherwise just start in the entry state every time the state machine
  // is started
  if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
  {
    if (OnWire)
    {
      CurrentState = PID_CONTROL;
    }
    else
    {
      CurrentState = ENTRY_STATE;
    }
  }
  // call the entry function (if any) for the ENTRY_STATE
  RunLineFollowingSM(CurrentEvent);
}

/****************************************************************************
 Function
     QueryLineFollowingSM

 Parameters
     None

 Returns
     LineFollowingState_t The current state of the LineFollowing state machine

 Description
     returns the current state of the LineFollowing state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
LineFollowingState_t QueryLineFollowingSM(void)
{
  return CurrentState;
}

//event checker for checking for wire
bool Check4Wire(void)
{
  ADC_MultiRead(FieldStrengths);

  static uint32_t InductorValsLeft[RUNNING_AVG_ARRAY_LEN] = { 0 };
  static uint32_t InductorValsRight[RUNNING_AVG_ARRAY_LEN] = { 0 };
  static uint32_t arrayPos = 0;

  InductorValsLeft[arrayPos & 0x07] = FieldStrengths[LEFT_INDUCT] + LEFT_INDUCTOR_OFFSET;
  InductorValsRight[arrayPos & 0x07] = FieldStrengths[RIGHT_INDUCT];
  arrayPos++;
  uint32_t  InductorLeft = getArrayAvg(InductorValsLeft);
  uint32_t  InductorRight = getArrayAvg(InductorValsRight);

  //if we sense the wire and am not on the wire, post event & OnWire = true.
  //If we sense the wire and are on the wire, do nothing. If we don't sense the wire, OnWire = false.
  if ((FieldStrengths[0] > INDUCTOR_THRESHOLD) || (FieldStrengths[1] > INDUCTOR_THRESHOLD))
  {
    if (!OnWire)
    {
      OnWire = true;
      ES_Event_t SwitchEvent;
      SwitchEvent.EventType = EV_LINE_HIT;
      PostMasterSM(SwitchEvent);
    }
  }
  else
  {
    OnWire = false;
  }
  return false;
}

//function for checking if we're on the wire or not
bool IsOnWire(void)
{
  return OnWire;
}

/***************************************************************************
 private functions
 ***************************************************************************/

//Putting PID control here for line following
void ControlLaw(void)
{
  static float    FieldError;                             /* make static for speed */
  static uint32_t InductorValsLeft[RUNNING_AVG_ARRAY_LEN] = { 0 };
  static uint32_t InductorValsRight[RUNNING_AVG_ARRAY_LEN] = { 0 };
  static uint32_t arrayPos = 0;

  //Read magnetic field strengths
  ADC_MultiRead(FieldStrengths);
  InductorValsLeft[arrayPos & 0x07] = FieldStrengths[LEFT_INDUCT] + LEFT_INDUCTOR_OFFSET;
  InductorValsRight[arrayPos & 0x07] = FieldStrengths[RIGHT_INDUCT];
  arrayPos++;
  uint32_t  InductorLeft = getArrayAvg(InductorValsLeft);
  uint32_t  InductorRight = getArrayAvg(InductorValsRight);

  //Calculate PID terms
  FieldError = (float)InductorLeft - (float)InductorRight;

  float RequestedDuty = pGain_forward * (FieldError); //+ dGain * (FieldError-LastError);
  RequestedDuty = Clamp(RequestedDuty, -21.0, 21.0);
  RequestedDuty = (float)OFFSET_SPEED_FORWARD + RequestedDuty;
  SetDuty((uint8_t)RequestedDuty, RIGHT_MOTOR); // output calculated control to right motor
}

// constrain val to be in range clampL to clampH
static float Clamp(float val, float clampL, float clampH)
{
  if (val > clampH) // if too high
  {
    return clampH;
  }
  if (val < clampL) // if too low
  {
    return clampL;
  }
  return val; // if OK as-is
}

static uint32_t getArrayAvg(uint32_t arrayToSum[])
{
  uint32_t sum = 0;
  for (uint8_t i = 0; i < RUNNING_AVG_ARRAY_LEN; i++)
  {
    sum += arrayToSum[i];
  }

  return sum >> 3; //sum/RUNNING_AVG_ARRAY_LEN;
}

static ES_Event_t DuringDrivingForward(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    DriveForward(FORWARD_DUTY);
  }
  else if (Event.EventType == ES_EXIT)
  {
    //make bot turn right to align with wire
    SetDuty(50, LEFT_MOTOR);      //55
    SetDuty(20, RIGHT_MOTOR);     //12, 15, 20

    //start timer to be parallel with line
    ES_Timer_InitTimer(TURNING_TOWARDS_WIRE_TIMER, TurningWireTime);
  }
  else
  // do the 'during' function for this state
  {}

  return ReturnEvent;
}

static ES_Event_t DuringPIDControl(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
  }
  else if (Event.EventType == ES_EXIT)
  {
    StopMotors();
  }
  else
  // do the 'during' function for this state
  {
//      printf("during for pidcontrol\n\r");
    if (Event.EventType == ES_TIMEOUT)
    {
      if (Event.EventParam == TURNING_TOWARDS_WIRE_TIMER)
      {
        //Done turning towards wire, should be somewhat parallel with wire now

        DriveForward(OFFSET_SPEED_FORWARD);
        ControlLaw();
        ES_Timer_InitTimer(CONTROL_LAW_TIMER, ControlLawTime);
      }

      if (Event.EventParam == CONTROL_LAW_TIMER)
      {
        ControlLaw();
      }
      ES_Timer_InitTimer(CONTROL_LAW_TIMER, ControlLawTime);
    }
  }
  return ReturnEvent;
}

static ES_Event_t DuringSquareUp(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {

    if (firstSwitchType)
    {
      //right switch hit, turn towards right
      SetDuty(0,                RIGHT_MOTOR);
      SetDuty(SQUARE_UP_SPEED,  LEFT_MOTOR);
    }
    else
    {
      //left switch hit, turn towards left
      SetDuty(SQUARE_UP_SPEED,  RIGHT_MOTOR);
      SetDuty(0,                LEFT_MOTOR);
    }
  }
  else if (Event.EventType == ES_EXIT)
  {
    StopMotors();
  }
  else
  // do the 'during' function for this state
  {}

  return ReturnEvent;
}
