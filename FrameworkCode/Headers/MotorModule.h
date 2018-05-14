// header file for the Motor Module

#ifndef MOTOR_MODULE
#define MOTOR_MODULE

// Hardware
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_pwm.h"
#include "inc/hw_nvic.h"
#include "inc/hw_timer.h"

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_ShortTimer.h"
#include "ES_Events.h"

//General
#include <stdint.h>
#include <stdbool.h>

void InitFanPumpPWM(void); 
void MoveForward(uint32_t ForwardDuty); 
void StopFanMotors(void); 
void MoveFanMotors(uint32_t LeftSpeed, uint32_t RightSpeed); 
void SetPumpSpeed(uint32_t pumpDuty); 


#endif //MOTOR_MODULE


