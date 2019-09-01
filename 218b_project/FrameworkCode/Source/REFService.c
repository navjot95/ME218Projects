/****************************************************************************
 Module
   REFService.c

 Revision
   2.0.1

 Description
   This is a REF file for implementing state machines.

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
 02/18/99 10:19 jec      built REF from MasterMachine.c
 02/14/99 10:34 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// Basic includes for a program using the Events and Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"

/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "REFService.h"
#include "GamePlayHSM.h"
#include "PlayService.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"

/*----------------------------- Module Defines ----------------------------*/
// define constants for the states for this machine
// and any other local defines

#define ENTRY_STATE WAITING
#define StateQuery 0x3F
#define ScoreQuery 0xC3
#define REFWaitTime 2
#define REFQueryTime 200

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine, things like during
   functions, entry & exit functions.They should be functions relevant to the
   behavior of this state machine
*/
void Check4StateChange(void);
static ES_Event_t DuringWaiting(ES_Event_t Event);
static ES_Event_t DuringSending(ES_Event_t Event);
static ES_Event_t DuringDelay(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well

static REFState_t     CurrentState;
static PlayState_t    LastState;
static bool           QueryingScore;
static uint32_t       Bytes;
static uint16_t       StateMessage;
static uint16_t       ScoreMessage;
static const uint16_t ShotClockMask = 0xFF00;
static const uint16_t REDScoreMask = 0xFF00;
static const uint16_t BLUEScoreMask = 0x00FF;
static const uint16_t PossessionMask = 0x0030;
static const uint16_t StatusMask = 0x0007;
static const uint8_t  GameOverMask = 0x04;
static const uint32_t ErrorBytes = 0xFFFF0000;
static const uint32_t ErrorCheck = 0x00FF0000;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
    RunREFService

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
ES_Event_t RunREFService(ES_Event_t CurrentEvent)
{
  bool        MakeTransition = false;/* are we making a state transition? */
  REFState_t  NextState = CurrentState;
  ES_Event_t  EntryEventKind = { ES_ENTRY, 0 }; // default to normal entry to new state
  ES_Event_t  ReturnEvent = CurrentEvent;       // assume we are not consuming event

  switch (CurrentState)
  {
    case WAITING:
    {
      // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringWaiting(CurrentEvent);

      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)    //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_SCORE_UPDATE:   //If event is event one
          {                       // Execute action function for state one : event one
            NextState = SENDING;  //Decide what the next state will be

            //Write param to SSIDR
            HWREG(SSI0_BASE + SSI_O_DR) = ScoreQuery;
            HWREG(SSI0_BASE + SSI_O_DR) = 0;
            HWREG(SSI0_BASE + SSI_O_DR) = 0;
            HWREG(SSI0_BASE + SSI_O_DR) = 0;

            // Enable the NVIC interrupt for the SSI when starting to transmit (vector #23, Interrupt #7)
            HWREG(NVIC_EN0) |= BIT7HI;

            //Reset byte storage
            Bytes = 0;
            //Score, not state
            QueryingScore = 1;

            // for internal transitions, skip changing MakeTransition
            MakeTransition = true;     //mark that we are taking a transition
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
            if (CurrentEvent.EventParam == REF_QUERY_TIMER)
            {
              NextState = SENDING;    //Decide what the next state will be

              //Write param to SSIDR
              HWREG(SSI0_BASE + SSI_O_DR) = StateQuery;
              HWREG(SSI0_BASE + SSI_O_DR) = 0;
              HWREG(SSI0_BASE + SSI_O_DR) = 0;
              HWREG(SSI0_BASE + SSI_O_DR) = 0;

              // Enable the NVIC interrupt for the SSI when starting to transmit (vector #23, Interrupt #7)
              HWREG(NVIC_EN0) |= BIT7HI;

              //Reset byte storage
              Bytes = 0;
              //State, not score
              QueryingScore = 0;

              // for internal transitions, skip changing MakeTransition
              MakeTransition = true;     //mark that we are taking a transition
              // if transitioning to a state with history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;
              // optionally, consume or re-map this event for the upper
              // level state machine
              ReturnEvent.EventType = ES_NO_EVENT;
            }
          }
          break;

          case EV_STATE_UPDATE:   //If event is event one
          {                       // Execute action function for state one : event one
            NextState = SENDING;  //Decide what the next state will be

            //Write param to SSIDR
            HWREG(SSI0_BASE + SSI_O_DR) = StateQuery;
            HWREG(SSI0_BASE + SSI_O_DR) = 0;
            HWREG(SSI0_BASE + SSI_O_DR) = 0;
            HWREG(SSI0_BASE + SSI_O_DR) = 0;

            // Enable the NVIC interrupt for the SSI when starting to transmit (vector #23, Interrupt #7)
            HWREG(NVIC_EN0) |= BIT7HI;

            //Reset byte storage
            Bytes = 0;
            //State, not score
            QueryingScore = 0;

            // for internal transitions, skip changing MakeTransition
            MakeTransition = true;     //mark that we are taking a transition
            // if transitioning to a state with history change kind of entry
            //EntryEventKind.EventType = ES_ENTRY_HISTORY;
            // optionally, consume or re-map this event for the upper
            // level state machine
            ReturnEvent.EventType = ES_NO_EVENT;
          }
          break;
        }
      }
    }
    break;

    case SENDING: // If current state is state one
    {             // Execute During function for state one. ES_ENTRY & ES_EXIT are
                  // processed here allow the lower level state machines to re-map
                  // or consume the event
      ReturnEvent = CurrentEvent = DuringSending(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)      //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case EV_EOM:          //If event is event one
          {                     // Execute action function for state one : event one
            NextState = DELAY;  //Decide what the next state will be

            if ((Bytes & ErrorBytes) == ErrorCheck)
            {
              if (QueryingScore)
              {
                ScoreMessage = Bytes;
              }
              else
              {
                StateMessage = Bytes;
                Check4StateChange();
              }
            }
            else
            {
              printf("REF ERROR\n\r");
            }

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

    case DELAY: // If current state is state one
    {           // Execute During function for state one. ES_ENTRY & ES_EXIT are
      // processed here allow the lower level state machines to re-map
      // or consume the event
      ReturnEvent = CurrentEvent = DuringDelay(CurrentEvent);
      //process any events
      if (CurrentEvent.EventType != ES_NO_EVENT)    //If an event is active
      {
        switch (CurrentEvent.EventType)
        {
          case ES_TIMEOUT:  //If event is event one
          {                 // Execute action function for state one : event one
            if (CurrentEvent.EventParam == REF_WAIT_TIMER)
            {
              NextState = WAITING;    //Decide what the next state will be

              //Restart querying timer for game state
              if (!QueryingScore)
              {
                ES_Timer_InitTimer(REF_QUERY_TIMER, REFQueryTime);
              }
              // for internal transitions, skip changing MakeTransition
              MakeTransition = true;     //mark that we are taking a transition
              // if transitioning to a state with history change kind of entry
              //EntryEventKind.EventType = ES_ENTRY_HISTORY;
              // optionally, consume or re-map this event for the upper
              // level state machine
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
    RunREFService(CurrentEvent);

    CurrentState = NextState;    //Modify state variable

    //   Execute entry function for new state
    // this defaults to ES_ENTRY
    RunREFService(EntryEventKind);
  }
  return ReturnEvent;
}

/****************************************************************************
 Function
     StartREFService

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
void StartREFService(ES_Event_t CurrentEvent)
{
  // to implement entry to a history state or directly to a substate
  // you can modify the initialization of the CurrentState variable
  // otherwise just start in the entry state every time the state machine
  // is started
  if (ES_ENTRY_HISTORY != CurrentEvent.EventType)
  {
    CurrentState = ENTRY_STATE;
  }
  LastState = WAITING_TO_START;
  //Start the timer for periodic state queries to the REF
  ES_Timer_InitTimer(REF_QUERY_TIMER, REFQueryTime);
  // call the entry function (if any) for the ENTRY_STATE (NONE)
  //RunREFService(CurrentEvent);
}

/****************************************************************************
 Function
     QueryREFService

 Parameters
     None

 Returns
     REFState_t The current state of the REF state machine

 Description
     returns the current state of the REF state machine
 Notes

 Author
     J. Edward Carryer, 2/11/05, 10:38AM
****************************************************************************/
REFState_t QueryREFService(void)
{
  return CurrentState;
}

uint16_t GetREDScore(void)
{
  return (ScoreMessage & REDScoreMask) >> BITS_PER_BYTE;
}

uint16_t GetBLUEScore(void)
{
  return ScoreMessage & BLUEScoreMask;
}

uint16_t GetPossession(void)
{
  return (StateMessage & PossessionMask) >> 4;
}

uint16_t GetGameStatus(void)
{
  return StateMessage & StatusMask;
}

uint16_t GetShotClock(void)
{
  return (StateMessage & ShotClockMask) >> BITS_PER_BYTE;
}

/***************************************************************************
 private functions
 ***************************************************************************/

void EOT_ISR(void)
{
  // Disable the NVIC interrupt for the SSI when transmit finished (vector #23, Interrupt #7)
  HWREG(NVIC_EN0) &= BIT7LO;

  //Save received byte in specified place in Bytes variable
  for (int i = 0; i < 4; i++)
  {
    uint8_t Received = HWREG(SSI0_BASE + SSI_O_DR);
    Bytes |= Received << ((3 - i) * BITS_PER_BYTE);
  }

  ES_Event_t EOMEvent;
  EOMEvent.EventType = EV_EOM;
  PostMasterSM(EOMEvent);
}

//Checks if the REF state changed, if so, posts an event to the MasterSM
void Check4StateChange(void)
{
  uint8_t     Status = GetGameStatus();
  uint8_t     Possession = GetPossession();
  ES_Event_t  Event;
  Event.EventType = EV_STATE_CHANGE;
  PlayState_t NewState;

  switch (Status)
  {
    case 0:
    {
      NewState = WAITING_TO_START;
    }
    break;
    case 1:
    {
      NewState = FACE_OFF;
    }
    break;
    case 2:
    {
      if (Possession == GetTeamColor())
      {
        NewState = OFFENSE;
      }
      else if (Possession == 0)
      {
        printf("REF ERROR Possession = 0 and in Play state\n\r");
      }
      else
      {
        NewState = DEFENSE;
      }
    }
    break;
    case 3:
    {
      NewState = OVERTIME;
    }
    break;
    default:
    {
      if ((Status & GameOverMask) == 4)
      {
        NewState = GAME_OVER;
      }
      else
      {
        printf("THERE'S BEEN AN ERROR\n\r");
        NewState = WAITING_TO_START;
      }
    }
    break;
  }

  if (LastState != NewState)
  {
    Event.EventParam = NewState;
    PostMasterSM(Event);
    LastState = NewState;
  }
}

static ES_Event_t DuringWaiting(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
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

static ES_Event_t DuringSending(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    // after that start any lower level machines that run in this state
    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    // repeat for any concurrently running state machines
    // now do any local exit functionality
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

static ES_Event_t DuringDelay(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine
    ES_Timer_InitTimer(REF_WAIT_TIMER, REFWaitTime);

    // after that start any lower level machines that run in this state
    //StartSendingSM( Event );
    // repeat the StartxxxSM() functions for concurrent state machines
    // on the lower level
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    //RunSendingSM(Event);
    // repeat for any concurrently running state machines
    // now do any local exit functionality
  }
  else
  // do the 'during' function for this state
  {
    // run any lower level state machine
    //ReturnEvent = RunSendingSM(Event);
    // repeat for any concurrent lower level machines
    // do any activity that is repeated as long as we are in this state
  }
  // return either Event, if you don't want to allow the lower level machine
  // to remap the current event, or ReturnEvent if you do want to allow it.
  return ReturnEvent;
}
