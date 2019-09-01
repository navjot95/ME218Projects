/****************************************************************************
 Module
   GamePlayHSM.c

 Revision
   2.0.1

 Description
   Top-level module in the statechart. Derived from the template

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/20/17 14:30 jec      updated to remove sample of consuming an event. We
                         always want to return ES_NO_EVENT at the top level
                         unless there is a non-recoverable error at the
                         framework level
 02/03/16 15:27 jec      updated comments to reflect small changes made in '14 & '15
                         converted unsigned char to bool where appropriate
                         spelling changes on true (was True) to match standard
                         removed local var used for debugger visibility in 'C32
                         removed Microwave specific code and replaced with generic
 02/08/12 01:39 jec      converted from MW_MasterMachine.c
 02/06/12 22:02 jec      converted to Gen 2 Events and Services Framework
 02/13/10 11:54 jec      converted During functions to return Event_t
                         so that they match the template
 02/21/07 17:04 jec      converted to pass Event_t to Start...()
 02/20/07 21:37 jec      converted to use enumerated type for events
 02/21/05 15:03 jec      Began Coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "GamePlayHSM.h"
#include "REFService.h"
#include "PlayService.h"

#include "ES_Queue.h"
#include "ES_DeferRecall.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
static ES_Event_t DuringTopState(ES_Event_t Event);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, though if the top level state machine
// is just a single state container for orthogonal regions, we could get
// away without it
static MasterState_t  CurrentState;
// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t        MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMasterSM

 Parameters
     uint8_t : the priority of this service

 Returns
     boolean, False if error in initialization, True otherwise

 Description
     Saves away the priority,  and starts
     the top level state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:06
****************************************************************************/
bool InitMasterSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  // Save the priority
  MyPriority = Priority;

  ThisEvent.EventType = ES_ENTRY;

  // Start the Master state machine
  StartMasterSM(ThisEvent);

  return true;
}

/****************************************************************************
 Function
     PostMasterSM

 Parameters
     ES_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the post operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostMasterSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunMasterSM

 Parameters
   ES_Event_t: the event to process

 Returns
   ES_Event_t: an event to return

 Description
   the run function for the top-level state machine
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 02/06/12, 22:09
****************************************************************************/
ES_Event_t RunMasterSM(ES_Event_t CurrentEvent)
{
  /* are we making a state transition? */
  bool          MakeTransition = false;
  MasterState_t NextState = CurrentState;

  // default to normal entry to new state
  ES_Event_t  EntryEventKind = { ES_ENTRY, 0 };
  ES_Event_t  ReturnEvent = { ES_NO_EVENT, 0 }; // assume no error

  switch (CurrentState)
  {
    case TOP_STATE:           // If current state is state one

    {     // Execute During function for state one. ES_ENTRY & ES_EXIT are
          // processed here to allow the lower-level state machines to re-map
          // or consume the event
      CurrentEvent = DuringTopState(CurrentEvent);

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
    RunMasterSM(CurrentEvent);

    CurrentState = NextState;    //Modify state variable

    // Execute entry function for new state
    // this defaults to ES_ENTRY
    RunMasterSM(EntryEventKind);
  }
  // in the absence of an error the top level state machine should
  // always return ES_NO_EVENT, which we initialized at the top of func
  return ReturnEvent;
}

/****************************************************************************
 Function
     StartMasterSM

 Parameters
     ES_Event_t CurrentEvent

 Returns
     nothing

 Description
     Does any required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 02/06/12, 22:15
****************************************************************************/
void StartMasterSM(ES_Event_t CurrentEvent)
{
  //initialize the state variable
  CurrentState = TOP_STATE;

  // now we need to let the Run function init the lower-level state machines
  // use LocalEvent to keep the compiler from complaining about unused var
  RunMasterSM(CurrentEvent);
}

/***************************************************************************
 private functions
 ***************************************************************************/

static ES_Event_t DuringTopState(ES_Event_t Event)
{
  ES_Event_t ReturnEvent = Event;   // assume no re-mapping or consumption

  // process ES_ENTRY, ES_ENTRY_HISTORY & ES_EXIT events
  if ((Event.EventType == ES_ENTRY) ||
      (Event.EventType == ES_ENTRY_HISTORY))
  {
    // implement any entry actions required for this state machine

    // after that start any lower-level machines that run in this state
    StartREFService(Event);
    StartPlayService(Event);
  }
  else if (Event.EventType == ES_EXIT)
  {
    // on exit, give the lower levels a chance to clean up first
    RunREFService(Event);
    RunPlayService(Event);

    // now do any local exit functionality
  }
  else
  // do the 'during' function for this state
  {
    // run any lower-level state machine
    RunREFService(Event);
    RunPlayService(Event);

    // do any activity that is repeated as long as we are in this state
  }

  return ReturnEvent;
}
