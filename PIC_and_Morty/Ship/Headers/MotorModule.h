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

#include "PWMLibrary.h"

//For LED lights
#define PURPLE 0
#define RED 1
#define BLUE 2

void InitFanPumpPWM(void); 
void MoveForward(uint32_t ForwardDuty); //give value 0-100 
void MoveBackward(uint32_t BackwardDuty); //give value 0-100
void StopFanMotors(void); 

void MoveFanMotors(uint32_t LeftSpeed, uint32_t RightSpeed, bool dir); 
void changePumpPower(bool turnOn); //set param turnOn to true to turn pump on, false to turn pump off
//void SetPumpSpeed(uint32_t pumpDuty); 
void changeFlow(bool toTank); //set parameter to true to make flow go to tank, false goes to shooter 


//FOR LIGHTS 
void powerFuelLEDs(bool turnOn); 
void setCurrTeamLED(uint8_t color); //purple is 0, red is 1, blue is 2 
void setHomeTeamLED(bool isRed); 

#endif //MOTOR_MODULE


