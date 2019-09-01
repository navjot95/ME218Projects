/****************************************************************************
 Defense header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef Defense_SM_H
#define Defense_SM_H

// typedefs for the states
// State definitions for use with the query function
typedef enum { ROTATING_CW, DRIVING_STRAIGHT, WAITING_AT_DEFEND_GOAL } DefenseState_t;

// Public Function Prototypes

ES_Event_t RunDefenseSM(ES_Event_t CurrentEvent);
void StartDefenseSM(ES_Event_t CurrentEvent);
DefenseState_t QueryDefenseSM(void);
bool Check4SharpEvents(void);

#endif /*Defense_SM_H */
