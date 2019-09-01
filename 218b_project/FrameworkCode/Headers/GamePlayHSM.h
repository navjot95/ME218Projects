/****************************************************************************
 Derived from template header file for Hierarchical State Machines AKA StateCharts

 ****************************************************************************/

#ifndef GamePlayHSM_H
#define GamePlayHSM_H

// State definitions for use with the query function
typedef enum { TOP_STATE } MasterState_t;

// Public Function Prototypes

ES_Event_t RunMasterSM(ES_Event_t CurrentEvent);
void StartMasterSM(ES_Event_t CurrentEvent);
bool PostMasterSM(ES_Event_t ThisEvent);
bool InitMasterSM(uint8_t Priority);

#endif /*GamePlayHSM_H */
