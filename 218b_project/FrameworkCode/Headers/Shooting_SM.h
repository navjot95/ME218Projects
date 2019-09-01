#ifndef Shooting_SM_H
#define Shooting_SM_H

// typedefs for the states
// State definitions for use with the query function
typedef enum { WAITING_FOR_FLYWHEEL, WAITING_FOR_BALL_WHEEL, WAITING_FOR_SHOT } ShootingState_t;

// Public Function Prototypes

ES_Event_t RunShootingSM(ES_Event_t CurrentEvent);
void StartShootingSM(ES_Event_t CurrentEvent);
ShootingState_t QueryShootingSM(void);

#endif /*Shooting_SM_H */
