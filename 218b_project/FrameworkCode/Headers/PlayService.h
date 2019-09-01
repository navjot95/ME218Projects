#ifndef PlayService_H
#define PlayService_H

// typedefs for the states
// State definitions for use with the query function
typedef enum { WAITING_TO_START, FACE_OFF, OFFENSE, DEFENSE, OVERTIME, GAME_OVER } PlayState_t;

// Public Function Prototypes

ES_Event_t RunPlayService(ES_Event_t CurrentEvent);
void StartPlayService(ES_Event_t CurrentEvent);
PlayState_t QueryPlayService(void);
uint8_t GetTeamColor(void);
bool Check4SharpEvents(void);
void ResetGoalEdges(void);
void ResetReloadEdges(void);
#endif /*PlayService_H */
