/****************************************************************************
 Offense header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef Offense_SM_H
#define Offense_SM_H

// typedefs for the states
// State definitions for use with the query function
typedef enum { RELOADING, /*LINE_FOLLOWING_OFFENSE,*/ ROTATING_TO_SHOOT, MOVING_BACKWARD, FINDING_SHOT, SHOOTING, ROTATING_TO_DEFINITELY_SHOOT } OffenseState_t;

// Public Function Prototypes

ES_Event_t RunOffenseSM(ES_Event_t CurrentEvent);
void StartOffenseSM(ES_Event_t CurrentEvent);
OffenseState_t QueryOffenseSM(void);
void SetFaceoffFalse(void);
void ResetRetroEdges(void);

#endif /*Offense_SM_H */
