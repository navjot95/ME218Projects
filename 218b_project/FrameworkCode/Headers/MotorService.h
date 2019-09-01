#ifndef MotorService_H
#define MotorService_H

// Event Definitions
//#include "ES_Events.h"
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum { INIT, FORWARD, BACKWARD, STOP, RIGHT, LEFT } MotorServiceState_t;

// Public Function Prototypes

bool InitMotorService(uint8_t Priority);
bool PostMotorService(ES_Event_t ThisEvent);
ES_Event_t RunMotorService(ES_Event_t ThisEvent);
void DriveForward(uint8_t Duty);
void DriveBackward(uint8_t Duty);
void RotateRight(uint8_t Duty);
void RotateLeft(uint8_t Duty);
void StopMotors(void);
void SetDuty(uint8_t Duty, bool RightMotor);
void SetDirection(bool Forward, bool RightMotor);

#endif
