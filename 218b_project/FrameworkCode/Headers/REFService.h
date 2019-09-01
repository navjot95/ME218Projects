/****************************************************************************
 Template header file for Hierarchical Sate Machines AKA StateCharts
 02/08/12 adjsutments for use with the Events and Services Framework Gen2
 3/17/09  Fixed prototpyes to use Event_t
 ****************************************************************************/

#ifndef REFService_H
#define REFService_H

// typedefs for the states
// State definitions for use with the query function
typedef enum { WAITING, SENDING, DELAY } REFState_t;

// Public Function Prototypes

ES_Event_t RunREFService(ES_Event_t CurrentEvent);
void StartREFService(ES_Event_t CurrentEvent);
REFState_t QueryREFService(void);
uint16_t GetREDScore(void);
uint16_t GetBLUEScore(void);
uint16_t GetPossession(void);
uint16_t GetGameStatus(void);
uint16_t GetShotClock(void);

#endif /*REFService_H */
