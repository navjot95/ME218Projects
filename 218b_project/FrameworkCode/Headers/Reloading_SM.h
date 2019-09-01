/****************************************************************************
 Reloading header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef Reloading_SM_H
#define Reloading_SM_H

// typedefs for the states
// State definitions for use with the query function
typedef enum { ROTATING_TO_BEACON, LINE_FOLLOWING_RELOADING, READING, WAITING_FOR_BALL } ReloadingState_t;

// Public Function Prototypes

ES_Event_t RunReloadingSM(ES_Event_t CurrentEvent);
void StartReloadingSM(ES_Event_t CurrentEvent);
ReloadingState_t QueryReloadingSM(void);
int8_t GetNumBalls(void);
void SetNumBalls(int8_t);
bool Check4LimitSwitches(void);
void ResetFlag(void);

#endif /*Reloading_SM_H */
