/****************************************************************************
 LineFollowing header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef LineFollowing_SM_H
#define LineFollowing_SM_H

// typedefs for the states
// State definitions for use with the query function
typedef enum { DRIVING_FORWARD, PID_CONTROL, SQUARE_UP } LineFollowingState_t;

// Public Function Prototypes

ES_Event_t RunLineFollowingSM(ES_Event_t CurrentEvent);
void StartLineFollowingSM(ES_Event_t CurrentEvent);
LineFollowingState_t QueryLineFollowingSM(void);
bool Check4Wire(void);
bool IsOnWire(void);

#endif /*LineFollowing_SM_H */
