/*****************************************************************************
 Module
   SensorUpdate.c

 Description
   

 Author
	E. Krimsky, ME218C, Team 6 
****************************************************************************/


#define SENSOR_DEBUG     // comment out when not debugging 

#include "SensorUpdate.h"

#define MAX_AD 4095                     // for AD readings 
#define DEBOUNCE_TIME 100                 // ms 

// Port A 
#define ENCODER_A BIT6HI                // encoder channel A
#define ENCODER_B BIT7HI                // encoder channel B 
#define PUMP_MIN 10


static uint8_t MyPriority;
static uint8_t updateInterval = 100;    // milliseconds (10 Hz refresh rate)

static uint8_t boatNumber = 6;          // start at 6 (our team)
static const uint8_t maxBoatNumber = 11; 

static uint8_t throttle = 0;            // 0 to 100 
static uint8_t pumpSpeed = 0;           // 0 to 100 


// ------------- Private Functions ------------
static void InitIOC( void );



/****************************************************************************
 Function
     InitSensorUpdate

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service

****************************************************************************/
bool InitSensorUpdate( uint8_t Priority )
{
	//Assign local priority variable MyPriority
    MyPriority = Priority;
    bool returnValue = false;

    InitIOC();      // initialize IOC
  
	//Initialize one Analog Input (on PE0) with ADC_MultiInit
    // PE0 - throttle input 
    // PE1 - shoot input (force sensor)
    ADC_MultiInit(2);

	if (ES_Timer_InitTimer(SENSOR_UPDATE_TIMER, updateInterval) == ES_Timer_OK)  
    {
    	returnValue = true;
    } 
    return returnValue;   
}

/****************************************************************************
 Function
     PostSensorUpate

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service

****************************************************************************/
bool PostSensorUpdate( ES_Event_t ThisEvent)
{
	return ES_PostToService( MyPriority, ThisEvent);
}

/****************************************************************************
 Function
     RunSensorUpdate

 Parameters
     uint8_t : the priorty of this service

 Returns
     ES_Event

 Description
     Runs the state machine for this module

****************************************************************************/
ES_Event_t RunSensorUpdate( ES_Event_t ThisEvent )
{
	//Create ES_Event ReturnEvent
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT;

	if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == SENSOR_UPDATE_TIMER)
    {
		// Update all the sensor readings 
		ES_Timer_InitTimer(SENSOR_UPDATE_TIMER, updateInterval);

        //read AD value 
        uint32_t analogIn[2]; // to store AD value      
        ADC_MultiRead(analogIn);

        throttle =  ((100 * analogIn[0])/MAX_AD);   
        pumpSpeed = 130 * ((MAX_AD - analogIn[1])/(MAX_AD + 0.0));    

        if (pumpSpeed < PUMP_MIN)
        {
            pumpSpeed = 0;
        } 
        else if (pumpSpeed > 100)
        {
            pumpSpeed = 100;
        }
        
        #ifdef SENSOR_DEBUG
            printf("\r\n Boat Number: %i, Throttle: %i, Pump: %i", boatNumber, throttle, pumpSpeed);
        #endif 


	}
    else if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == DEBOUNCE_TIMER)
    {
        HWREG(GPIO_PORTA_BASE+GPIO_O_ICR) |= ENCODER_A;         // W1C 
        HWREG(GPIO_PORTA_BASE+GPIO_O_IM) |= ENCODER_A;         // unmask
        printf("a");
    }
    else
    {
		ReturnEvent.EventType = ES_ERROR;
    }
	return ReturnEvent;
}

/****************************************************************************
 Function
     getBoatNumber

 Parameters
     none

 Returns
     current boat number selected by dial 

****************************************************************************/
uint8_t getBoatNumber( void )
{
    return boatNumber; 
}

uint8_t getThrottle( void )
{
    return throttle; 
}

uint8_t getPumpSpeed( void )
{
    return pumpSpeed; 
} 


/****************************************************************************
 Function
   

 Parameters
     none

 Returns
     none

 Description
     

****************************************************************************/
void Encoder_IOC_Response( void )
{
      //printf("w");
    // Double check that it is the right int (add later)
    if (HWREG(GPIO_PORTA_BASE+GPIO_O_MIS) & ENCODER_A)
    {
        ES_Timer_InitTimer(DEBOUNCE_TIMER, DEBOUNCE_TIME);
        HWREG(GPIO_PORTA_BASE+GPIO_O_IM) &= ~ENCODER_A;         // mask 

        // clear the source of the interupt, p. 670 
        HWREG(GPIO_PORTA_BASE+GPIO_O_ICR) |= ENCODER_A; // W1C 
        //printf("p");
        // read encoder channel B 
        if (HWREG(GPIO_PORTA_BASE+(GPIO_O_DATA + ALL_BITS)) & ENCODER_B)
        {
            boatNumber--;
        }
        else
        {
            boatNumber++;
        }

        if (boatNumber > maxBoatNumber)
        {
            boatNumber = 1;
        }    
        else if (boatNumber == 0)
        {
            boatNumber = maxBoatNumber; 
        }
        
    }
}

/*------------------------- Private Methods -------------------------------*/

/****************************************************************************
 Function
     InitIOC

 Parameters
     none

 Returns
     none

 Description
     Initialize int on change (IOC) for boat select dial 

****************************************************************************/
static void InitIOC( void )
{
    // p. 656 Note that GPIO can only be accessed through the AHB aperture.

    // enable clock to port A 
    while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R0) != SYSCTL_PRGPIO_R0)
        ;

    //set up encoder inputs as digital I/O
    HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= (ENCODER_A | ENCODER_B);

    //set up encoder inputs as digital inputs 
    HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) &= ~(ENCODER_A | ENCODER_B); 

    // For int pins see page 664 - 666 
    // use pa 7 and pa 6 

    /* From p. 657
    a. Mask the corresponding port by clearing the IME field in 
    the GPIOIM register. See p. 667 
    */
    //HWREG(GPIO_PORTA_BASE+GPIO_O_IM) &= ~(ENCODER_A);    
    HWREG(GPIO_PORTA_BASE+GPIO_O_IM) = 0;  // mask all ints on the port   p. 667 

    /* b. Configure the IS field in the GPIOIS register and the 
    IBE field in the GPIOIBE register. See p. 665 */
    HWREG(GPIO_PORTA_BASE+GPIO_O_IS) &= ~ENCODER_A;     // Edge sensitive 
    HWREG(GPIO_PORTA_BASE+GPIO_O_IBE) &= ~(ENCODER_A);  // DONT int on both edges 
    // now int will be controlled by value in GPIOIEV register p. 666 
    HWREG(GPIO_PORTA_BASE+GPIO_O_IEV) |= ENCODER_A;     // Rising edge int
   //  HWREG(GPIO_PORTA_BASE+GPIO_O_IEV) &= ~ENCODER_A;     // falling edge int

    // c. Clear the GPIORIS register. p. 668
    HWREG(GPIO_PORTA_BASE+GPIO_O_RIS) &= 0;         //~ENCODER_A; 

    // d. Unmask the port by setting the IME field in the GPIOIM register.
    HWREG(GPIO_PORTA_BASE+GPIO_O_IM) |= ENCODER_A;    

    // NOTE what about the intterupt clear register?

    // see interrupt table on p. 104 
    // int number = 0 
    // vector number = 16
    // see nvic table p. 134 
    HWREG(NVIC_EN0) |= BIT0HI; 

    // globally enable interupts 
    __enable_irq();     
}
