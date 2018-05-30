/****************************************************************************
 Module
   Communication state machine - main state machine for the SHIP

 Revision
   1.0.1

 Description
   This is a template file for implementing flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "SHIP_MASTER.h"
#include "MotorModule.h" 
#include "SHIP_RX.h"
#include "SHIP_TX.h"
#include "SHIP_PIC_RX.h"
#include "SHIP_PIC_TX.h"

/*----------------------------- Module Defines ----------------------------*/
//#define DEBUG_PRINTF

#define ONE_SEC 1000
#define BLUE_TEAM 1
#define RED_TEAM 0
#define PAIR_ATTEMPT_TIME 150
#define PAIR_TIMEOUT_TIME 3*ONE_SEC

// Control Commands
#define PUMP_DC        100
#define MAX_MOTOR_DC   90
#define DEADBAND_VALUE 10

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static bool getHomeTeamColor(void);

/*FOR TESTING ONLYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY*/
static void sendPairAck(void); 
static void sendStatusPacket(void); 
static void executeControlPacketCommands(void); 


/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static shipState_t CurrentState;
static uint8_t MyPriority;
static uint32_t lastAnsAddr = 0; 
static bool homeTeamColorisRed; 

// Control Data
static uint8_t Control_FB;
static uint8_t Control_LR; 
//static uint8_t Control_TurretR;
//static uint8_t Control_TurretP;
static uint8_t Control_CTRL; 

static bool    CurrentFuel;
static bool    LastFuel;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTemplateFSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitSHIP_MASTER(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = Waiting2Pair;
  
  #ifdef DEBUG_PRINTF
  printf("\r\nINIT STATE: Waiting2Pair");
  #endif
  
  //initialize all hw necessairy for the SHIP 
  homeTeamColorisRed = getHomeTeamColor(); 
  setHomeTeamLED(homeTeamColorisRed); //turn the LED on to the appropriate color 
  //powerFuelLEDs(false);
  
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
     PostTemplateFSM

 Parameters
     EF_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostSHIP_MASTER(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateFSM

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunSHIP_MASTER(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (CurrentState)
  {
    case Waiting2Pair:      
    {
      setCurrTeamLED(PURPLE); 
      LastFuel = QueryFuelEmpty();
      
      if(ThisEvent.EventType == ES_PAIR_REQUEST){  /*received 0x01 packet*/
        //guard: if fueled, then only the home team can connect 
        if(QueryFuelEmpty() && (Query_ANSIBLEColour() == homeTeamColorisRed)){ 
          //start pairing timer (1sec)
          ES_Timer_InitTimer(PAIR_TIMEOUT_SHIP_TIMER, PAIR_TIMEOUT_TIME);
          //start attempt timer (200ms)
          ES_Timer_InitTimer(PAIR_ATTEMPT_SHIP_TIMER, PAIR_ATTEMPT_TIME);          
          //send Pair_Ack Packet (0x02) 
          //sendPairAck(); 
          CurrentState = Trying2Pair; 
          
          #ifdef DEBUG_PRINTF
          printf("\r\nSTATE TRANSITION: Waiting2Pair - Trying2Pair"); 
          #endif
        }
        else if(!QueryFuelEmpty() && (lastAnsAddr != QuerySourceAddress())){
          //start pairing timer (1sec)
          ES_Timer_InitTimer(PAIR_TIMEOUT_SHIP_TIMER, PAIR_TIMEOUT_TIME);
          //start attempt timer (200ms) 
          ES_Timer_InitTimer(PAIR_ATTEMPT_SHIP_TIMER, PAIR_ATTEMPT_TIME);
          //send Pair_Ack Packet (0x02)
          //sendPairAck(); 
          CurrentState = Trying2Pair;
          
          #ifdef DEBUG_PRINTF
          printf("\r\nSTATE TRANSITION: Waiting2Pair - Trying2Pair"); 
          #endif
        }
      }
    }
    break;

    case Trying2Pair:  //make 4 more attempts at sending 0x02 packet in case first one not read 
    {
      if(ThisEvent.EventType == ES_TIMEOUT){
        if(ThisEvent.EventParam == PAIR_ATTEMPT_SHIP_TIMER){
          //200 ms timer has timeout, just resend 0x02 packet and restart 200ms timer 
          ES_Timer_InitTimer(PAIR_ATTEMPT_SHIP_TIMER, PAIR_ATTEMPT_TIME);
          sendPairAck();
        }
        else if(ThisEvent.EventParam == PAIR_TIMEOUT_SHIP_TIMER){
          CurrentState = Waiting2Pair; 
          
          #ifdef DEBUG_PRINTF
          printf("\r\nSTATE TRANSITION: Trying2Pair - Waiting2Pair");
          #endif
        }
        
      }
      else if(ThisEvent.EventType == ES_CONTROL_PACKET){  /*Control packet 0x03 received*/
        sendStatusPacket(); //0x04
        //restart 1 sec pairing timeout timer
        ES_Timer_InitTimer(PAIR_TIMEOUT_SHIP_TIMER, PAIR_TIMEOUT_TIME);
        executeControlPacketCommands(); 
        
        CurrentState = Communicating;
        
        #ifdef DEBUG_PRINTF
        printf("\r\nSTATE TRANSITION: Trying2Pair - Communicating");
        #endif
        
        // Turn on LED ANSIBLE Color
        if(Query_ANSIBLEColour()){
          setCurrTeamLED(RED); 
        }
        else 
          setCurrTeamLED(BLUE); 
        
      }
    }
    break;
    
    case Communicating:  //regular paired state
    {
      CurrentFuel = QueryFuelEmpty();
      lastAnsAddr = QuerySourceAddress(); 
      
      if(ThisEvent.EventType == ES_CONTROL_PACKET){  /*Control packet 0x03 received*/
        if(!CurrentFuel && (LastFuel != CurrentFuel))
        {
          StopFanMotors();
          CurrentState = Waiting2Pair; 
          
          #ifdef DEBUG_PRINTF
          printf("\r\nOut of Fuel");
          printf("\r\nSTATE TRANSITION: Communicating - Waiting2Pair");
          #endif
        }
        else if ((CurrentFuel && (LastFuel != CurrentFuel)) && (Query_ANSIBLEColour() != homeTeamColorisRed))
        {
          StopFanMotors(); 
          CurrentState = Waiting2Pair; 
          
          #ifdef DEBUG_PRINTF
          printf("\r\nOpposite team kicked out"); 
          printf("\r\nIn Waiting2Pair now");
          #endif
        }
        else
        {
          sendStatusPacket(); //0x04
          //restart 1 sec pairing timeout timer
          ES_Timer_InitTimer(PAIR_TIMEOUT_SHIP_TIMER, PAIR_TIMEOUT_TIME);
          executeControlPacketCommands(); 
        }          
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == PAIR_TIMEOUT_SHIP_TIMER){
        //one sec pairing timer has timed out 
        StopFanMotors(); 
        CurrentState = Waiting2Pair;
        //powerFuelLEDs(false);
        setCurrTeamLED(PURPLE); 
        
        #ifdef DEBUG_PRINTF
        printf("\r\nONE_SEC Timer Timeout");
        printf("\r\nSTATE TRANSITION: Communicating - Waiting2Pair");
        #endif
      } 
//      else if(!CurrentFuel && (LastFuel != CurrentFuel)){  /*Out of fuel event*/
//        powerFuelLEDs(false);
//        StopFanMotors();
//        lastAnsAddr = Query_ANSIBLEColour(); 
//        CurrentState = Waiting2Pair; 
//        
//        #ifdef DEBUG_PRINTF
//        printf("\r\nOut of Fuel");
//        printf("\r\nSTATE TRANSITION: Communicating - Waiting2Pair");
//        #endif
//      }
//      else if((CurrentFuel && (LastFuel != CurrentFuel)) && (Query_ANSIBLEColour() != homeTeamColorisRed)){  /*Refueled Event*/
//        StopFanMotors(); 
//        CurrentState = Waiting2Pair; 
//        
//        #ifdef DEBUG_PRINTF
//        printf("\r\nOpposite team kicked out"); 
//        printf("\r\nIn Waiting2Pair now");
//        #endif
//      }
                  
      LastFuel = CurrentFuel;      
    }
    break;
        
  }                                  
  return ReturnEvent;
}




/****************************************************************************
 Function
     QueryTemplateSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:21
****************************************************************************/
shipState_t QueryCommunicationSM(void)
{
  return CurrentState;
}


/***************************************************************************
 private functions
 ***************************************************************************/

bool getHomeTeamColor(void)
{
  uint8_t CurrentButtonState = (HWREG(GPIO_PORTA_BASE+ ALL_BITS) & BIT2HI);
      
  if(CurrentButtonState)
  {
    //printf("\r\nTeam = RED");
    return 0x01; 
  }
  else
  {
    //printf("\r\nTeam = BLUE");
    return 0x00;
  }    
}

static void sendPairAck(void)
{
  ES_Event_t ThisEvent;
  
  //printf("\r\nACK packet sent"); 
  ThisEvent.EventType = BEGIN_TX;
  ThisEvent.EventParam = PAIR_ACK_EVENT;
  PostSHIP_TX(ThisEvent);
}

static void sendStatusPacket(void)
{
  ES_Event_t ThisEvent;
  
  //printf("\r\nStatus packet sent"); 
  ThisEvent.EventType = BEGIN_TX;
  ThisEvent.EventParam = STATUS_EVENT;
  PostSHIP_TX(ThisEvent);
}

static void executeControlPacketCommands(void)
{
  static uint8_t Motor_DC;
  static uint8_t Right_Motor_DC;
  static uint8_t Left_Motor_DC;
  static bool    Direction;
  static uint8_t DC_Scaler;
  //printf("\r\nControl commands executed");
  Control_FB = Query_FB();
  Control_LR = Query_LR();
//  Control_TurretR = Query_TurretR();
//  Control_TurretP = Query_TurretP();
  Control_CTRL = Query_CTRL();
  
  if (QueryFuelEmpty())
  {
    // If Fueled
    DC_Scaler = 1;
  }
  else
  {
    DC_Scaler = 2;
  }                  
  
  //printf("\r\nFB: %u, LR: %u", Control_FB, Control_LR);
  
  // TODO: USE VARIABLES ABOVE TO MOVE BOAT
  
  // BYTE 1: ANALOG FORWARD/BACK && BYTE 2: ANALOG SHIP YAW (LEFT/RIGHT)
  if (Control_LR > 127) // Turn Right
  {
    if (Control_FB > 127)
    {
      // Turning Right and Moving Forward
      Right_Motor_DC = (100 * (Control_FB - 127))/127;
      Left_Motor_DC = (Right_Motor_DC * (255 - Control_LR))/127;
      Right_Motor_DC /= DC_Scaler;
      Left_Motor_DC /= DC_Scaler;
      Direction = true;
      //MoveFanMotors(Left_Motor_DC, Right_Motor_DC, Direction);
      MoveFanMotors(Right_Motor_DC, Left_Motor_DC, Direction);
    }
    else if (Control_FB < 127)
    {
      // Turning Right and Moving Backward
      Right_Motor_DC = (100 * (Control_FB - 127))/127;
      Left_Motor_DC = (Right_Motor_DC * (255 - Control_LR))/127;
      Right_Motor_DC /= DC_Scaler;
      Left_Motor_DC /= DC_Scaler;
      Direction = false;
      //MoveFanMotors(Left_Motor_DC, Right_Motor_DC, Direction);
      MoveFanMotors(Right_Motor_DC, Left_Motor_DC, Direction);
    }
    else
    {
      StopFanMotors();
    }
  }
  else if (Control_LR < 127) // Turn Left
  {
    if (Control_FB > 127 )
    {
      // Turning Left and Moving Forward
      Left_Motor_DC = (100 * (Control_FB - 127))/127;
      Right_Motor_DC = (Left_Motor_DC * Control_LR)/127; 
      Right_Motor_DC /= DC_Scaler;
      Left_Motor_DC /= DC_Scaler;      
      Direction = true;
      //MoveFanMotors(Left_Motor_DC, Right_Motor_DC, Direction);
      MoveFanMotors(Right_Motor_DC, Left_Motor_DC, Direction);
    }
    else if (Control_FB < 127)
    {
      // Turning Left and Moving Backward
      Left_Motor_DC = (100 * (Control_FB - 127))/127;
      Right_Motor_DC = (Left_Motor_DC * Control_LR)/127;    
      Right_Motor_DC /= DC_Scaler;
      Left_Motor_DC /= DC_Scaler;      
      Direction = false;
      //MoveFanMotors(Left_Motor_DC, Right_Motor_DC, Direction);
      MoveFanMotors(Right_Motor_DC, Left_Motor_DC, Direction);
    }
    else
    {
      StopFanMotors();
    }
  }
  else  // Not turning
  {
    if (Control_FB > 127)
    {
      Motor_DC = (100 * (Control_FB - 127))/127;
      Motor_DC /= DC_Scaler;
      MoveForward(Motor_DC);
    }
    else if (Control_FB < 127)
    {
      Motor_DC = (100 * (Control_FB - 127))/127;
      Motor_DC /= DC_Scaler;
      MoveBackward(Motor_DC);
    }
    else
    {
      StopFanMotors();
    }
        
  }
  
  // BYTE 3: N/A (ANALOG TURRET YAW) && BYTE 4: N/A (ANALOG TURRET PITCH)
  
  // BYTE 5: DIGITAL CONTROL
  if (Control_CTRL & 0x01)  // Bit 0: Shoot Water
  {
    changePumpPower(PUMP_DC);
  }
  else
  {
    changePumpPower(0);
  }
  
}

