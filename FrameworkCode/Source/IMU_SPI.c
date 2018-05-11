/*****************************************************************************
 Module
   IMU_SPI.c
   
 Description
   Module for running SPI to Communicate with IMU

 Author
	E. Krimsky, ME218C
****************************************************************************/
#define IMU_DEBUG   // Unccoment to stop printing 

#include "IMU_SPI.h"

// Set up SPI for 0.5 MHz
#define PDIV 2      // SSI clock divisor 
#define SCR 40      // SSI Clock Prescaler  
#define TICKS_PER_MS 40000

//  Masks for reading and writing SPI to IMU

const int IMU_UPDATE_TIME = 10;    

static uint16_t byteIn; 		    // 2 byte transfers


// Stative Variables to store IMU Readings 
static uint16_t accel_x;
static uint16_t accel_y;
static uint16_t accel_z;
static uint16_t gyro_x;
static uint16_t gyro_y;
static uint16_t gyro_z;

// For storing current read state 
typedef enum {  ACC_X_HIGH,
                ACC_X_LOW,
                ACC_Y_HIGH,
                ACC_Y_LOW,
                ACC_Z_HIGH,
                ACC_Z_LOW,
                GYR_X_HIGH,
                GYR_X_LOW,
                GYR_Y_HIGH,
                GYR_Y_LOW,
                GYR_Z_HIGH,
                GYR_Z_LOW
                } SPI_Write_State_t ;

const uint8_t num_reads = GYR_Z_LOW - ACC_X_HIGH + 1; 

uint8_t address_array[num_reads]; 

uint8_t MyPriority; 

static SPI_Write_State_t CurrentState; 



// Local copies of IMU data 


// make this a service? 

uint8_t readCount = 0; 



// ------------------------- Private Functions ---------------------------//
static void IMU_SPI_Init( void ); 


/****************************************************************************
 Function
     InitIMU

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine
 Notes

 Author
     J. Edward Carryer, 10/23/11, 18:55
****************************************************************************/
bool InitIMU(uint8_t Priority)
{
    CurrentState = ACC_X_HIGH;          // State 0 
        
    // Fill up adress array with values 
    address_array[0] = MPU9250_ACCEL_XOUT_H;
    address_array[1] = MPU9250_ACCEL_XOUT_L;

    address_array[2] = MPU9250_ACCEL_YOUT_H;
    address_array[3] = MPU9250_ACCEL_YOUT_L;

    address_array[4] = MPU9250_ACCEL_ZOUT_H;
    address_array[5] = MPU9250_ACCEL_ZOUT_L;

    address_array[6] = MPU9250_GYRO_XOUT_H;
    address_array[7] = MPU9250_GYRO_XOUT_L;

    address_array[8] = MPU9250_GYRO_YOUT_H;
    address_array[9] = MPU9250_GYRO_YOUT_L;

    address_array[10] = MPU9250_GYRO_ZOUT_H;
    address_array[11] = MPU9250_GYRO_ZOUT_L;

    IMU_SPI_Init();     // Initialize SPI 
    uint16_t sendByte;

    // Other Configurations with blocking code 
    // To prevent switching into I2C mode when using SPI, the I2C interface 
    // should be disabled by setting the I2C_IF_DIS configuration bit. 
    // Setting this bit should be performed immediately after waiting 
    // for the time specified by the “Start-Up Time 
    // for Register Read/Write” in Section 6.3.
    uint16_t config_bytes = BIT4HI; 
    uint8_t config_reg = MPU9250_USER_CTRL;
    sendByte = BIT15HI;            // MSB high for read
    sendByte |= (config_reg << 8);
    sendByte |= config_bytes; 
    HWREG(SSI0_BASE+SSI_O_IM) |= BIT3HI;    // Enable TXIM
    HWREG(SSI0_BASE+SSI_O_DR) = sendByte;   // write a new byte to the FIFO

    // Do this write then wait -- 

    // NEED TO DELAY 
    // page 53 
    // This register disables I2C bus interface. I2C bus interface 
    // is enabled in default. To disable I2C bus interface,
    // write “00011011” to I2CDIS register. Then I2C bus interface is disabled.

    // Not sure if this part matters 
    
    // do the I2C disable 
    //sendByte = BIT15HI; 
    //uint16_t disable_I2C = 0x1B;  
    //sendByte = BIT15HI;            // MSB high for read
    //sendByte |= (config_reg << 8);
    //sendByte |= config_bytes; 
    //HWREG(SSI0_BASE+SSI_O_IM) |= BIT3HI;    // Enable TXIM
    //HWREG(SSI0_BASE+SSI_O_DR) = sendByte;   // write a new byte to the FIFO

    // SYSCTL_DELAY if need blocking delay for SPI setup 


    MyPriority = Priority;

    // 

    
    bool returnValue = false;    
    if (ES_Timer_InitTimer(IMU_TIMER, IMU_UPDATE_TIME) == ES_Timer_OK)  
    {
        returnValue = true;
    } 
    return returnValue;   
}


/****************************************************************************
 Function
     PostTemplateFSM

 Parameters
     EF_Event ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostIMU(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}



/****************************************************************************
 Function
    RunTemplateFSM

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes
   uses nested switch/case to implement the machine.
 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunIMU(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT;        // assume no errors

    // Could do an ES_INIT 

    if (ThisEvent.EventType == ES_TIMEOUT)
    {
        // Increment current state before write out
        // we increment before because state needs to be the same when EOT
        // occurs and we read in the data 
        if (CurrentState != (num_reads - 1))
        {
            CurrentState++;                    // Increment write state
        }
        else
        {
            CurrentState = ACC_X_HIGH;                  // Set back to first state 
            
            #ifdef IMU_DEBUG
            printf("\r\nX_ACC: %d Y_ACC: %d Z_ACC: %d\n", (int) accel_x, (int) accel_y, (int) accel_z); 
            printf("\rX_GYRO: %d Y_GYRO: %d Z_GYRO: %d\n\n", (int) gyro_x, (int) gyro_y, (int) gyro_z); 
            #endif 
            
        }


        // 
        uint16_t sendByte = BIT15HI;            // MSB high for read
        sendByte |= (address_array[CurrentState] << 8);
        HWREG(SSI0_BASE+SSI_O_IM) |= BIT3HI;    // Enable TXIM
        HWREG(SSI0_BASE+SSI_O_DR) = sendByte;   // write a new byte to the FIFO

        
    }
    ES_Timer_InitTimer(IMU_TIMER, IMU_UPDATE_TIME);

    // NOTE could add error handling for if something else posted 
    return ReturnEvent;
}





/****************************************************************************
 Function
     SPI_IntResponse

 Parameters
     none 

 Returns
     none
     
 Description
    Function that runs when EOT timeout is detected. Assigns the new byte to 
    module level variable "byteIn"
****************************************************************************/
void SPI_IntResponse ( void )
{
    // Disable (mask the interrupt) until next write 
    // see p. 973
    HWREG(SSI0_BASE+SSI_O_IM) &= ~BIT3HI; // Disable TXIM
    HWREG(GPIO_PORTA_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~BIT6HI;	 // for debug 
    byteIn = HWREG(SSI0_BASE+SSI_O_DR); 

    // Add a switch here on what to assign 
    switch (CurrentState)
    {
        case ACC_X_HIGH:
            accel_x = (accel_x & 0x00FF) & (byteIn << 8);
            break;
        case ACC_X_LOW:
            accel_x = (accel_x & 0xFF00) & (byteIn & 0x00FF);
            break;
        case ACC_Y_HIGH:
            accel_y = (accel_x & 0x00FF) & (byteIn << 8);
            break;
        case ACC_Y_LOW:
            accel_y = (accel_y & 0xFF00) & (byteIn & 0x00FF);
            break;
        case ACC_Z_HIGH:
            accel_z = (accel_x & 0x00FF) & (byteIn << 8);
            break;
        case ACC_Z_LOW:
            accel_z = (accel_z & 0xFF00) & (byteIn & 0x00FF);
            break;
        case GYR_X_HIGH:
            gyro_x = (gyro_x & 0x00FF) & (byteIn << 8);
            break;
        case GYR_X_LOW:
            gyro_x = (gyro_x & 0xFF00) & (byteIn & 0x00FF);
            break;
        case GYR_Y_HIGH:
            gyro_y = (gyro_y & 0x00FF) & (byteIn << 8);
            break;
        case GYR_Y_LOW:
            gyro_y = (gyro_y & 0xFF00) & (byteIn & 0x00FF);
            break;
        case GYR_Z_HIGH:
            gyro_z = (gyro_z & 0x00FF) & (byteIn << 8);
            break;
        case GYR_Z_LOW:
            gyro_z = (gyro_z & 0xFF00) & (byteIn & 0x00FF);
            break;
    }
    HWREG(GPIO_PORTA_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT6HI;	 // for debug 
}

// ------------------------- Private Methods ------------------------------//


/****************************************************************************
 Function
     IMU_SPI_Init

 Parameters
     none

 Returns
     non
     
 Description
    Initializes hardware for SPI module 0 (on port A) and configures a debug
    line on PA6
****************************************************************************/
static void IMU_SPI_Init( void )
{
    // See page 340, 346, 965, 1351 (table 23.5 for pins)   
    /*
    SSI0 - Port A , SSI1 - Port D or F, SSI2 - Port B, SSI3 - Port D
    For port A, SSI...
        CLK - PA2, Alt. Fun - SSI0Clk
        Fss - PA3, Alt Fun - SSI0Fss
        Rx - PA4, Alt. Fun - SSI0Rx
        Tx - PA5, Alt. Fun - SSI0Tx
    */

    //1. Enable the clock to the GPIO port
    HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R0; 

    // 2. Enable the clock to SSI module - p. 347
    HWREG(SYSCTL_RCGCSSI) |= SYSCTL_RCGCSSI_R0; //BIT0HI for SSI0

    // 3. Wait for the GPIO port A to be ready
    while ((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R0) != SYSCTL_PRGPIO_R0)
        ;
    
    // 4. Program the GPIO to use the alternate functions on the SSI pins 
    // see p.1351 (table 23.5)
    HWREG(GPIO_PORTA_BASE+GPIO_O_AFSEL) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); 


    // 5. Set mux position in GPIOPCTL to select the SSI use of the pins 
    //  p. 650 mux value is two
    uint8_t BitsPerNibble = 4;
    uint8_t mux = 2; 
    
    // 2, 3, 4, and 5 because those are the bit numbers on the port 
    HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) = (HWREG(GPIO_PORTA_BASE+GPIO_O_PCTL) 
                                           & 0xff0000ff) + 
                                         (mux<<(2*BitsPerNibble)) + 
                                         (mux<<(3*BitsPerNibble)) +
                                         (mux<<(4*BitsPerNibble)) +
                                         (mux<<(5*BitsPerNibble));

    // 6. Program the port lines for digital I/O
    HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= (BIT2HI | BIT3HI | BIT4HI | BIT5HI); 
    
    // 7. Program the required data directions on the port lines
    HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= (BIT2HI | BIT3HI | BIT5HI);
    HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) &= ~BIT4HI; // Rx needs to be an input 

    // Set up PA 6 as a debug line, initially low 
    HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= BIT6HI; 
    HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= BIT6HI;
    HWREG(GPIO_PORTA_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~BIT6HI; 
    
    
    // 8. If using SPI mode 3, program the pull-up on the clock line
    /* This pin is configured as a GPIO by default but is locked and 
    can only be reprogrammed by unlocking the pin in the GPIOLOCK
    register and uncommitting it by setting the GPIOCR register. */
    // see p. 1329, p. 650, p. 684 (LOCK) - handled by calling PortFunctions

    HWREG(GPIO_PORTA_BASE+GPIO_O_PUR) |= BIT2HI; // pullup on clock line 

    // 9. Wait for the SSI0 to be ready, see p. 412
    while ((HWREG(SYSCTL_PRSSI) & SYSCTL_PRSSI_R0) != SYSCTL_PRSSI_R0)
        ;  

    
    // 10. Make sure that the SSI is disabled before programming mode bits
    // see p. 965
    HWREG(SSI0_BASE+SSI_O_CR1) &= BIT1LO; //SSE; 

    // 11. Select master mode (MS) & TXRIS indicating End of Transmit (EOT)
    HWREG(SSI0_BASE+SSI_O_CR1) &= BIT2LO; // MS; //clear bit to be master 
    HWREG(SSI0_BASE+SSI_O_CR1) |= BIT4HI; // EOT;

    // 12. Configure the SSI clock source to the system clock
    // CC for 'Clock Source'
    HWREG(SSI0_BASE+SSI_O_CC) &= (0xffffffff << 4); // Clear the 4 LSBs

    // 13. Configure the clock pre-scaler
    // p. 976
    // bits 8:32 are read only so we should be able to write directly the 
    // 8 LSBs
    HWREG(SSI0_BASE+SSI_O_CPSR) = (HWREG(SSI0_BASE+SSI_O_CPSR) & 
                                                        0xffffff00) | PDIV; 


    //14. Configure clock rate (SCR), phase & polarity (SPH, SPO), mode (FRF),
    // data size (DSS)

    // see p. 969 - 970

    // SCR is bitfields 15:8 
    // first clear the SCR bits then sent them
    HWREG(SSI0_BASE+SSI_O_CR0) = (HWREG(SSI0_BASE+SSI_O_CR0) & ~0x0000ff00) |
                                            (SCR << 8); 
    // SPH is bitfield 7 
    HWREG(SSI0_BASE+SSI_O_CR0) |= BIT7HI;   

    // SPO is bitfield 6 
    HWREG(SSI0_BASE+SSI_O_CR0) |= BIT6HI;                                               

    // FRF - bitfields 5:4, write 00 for freescale
    HWREG(SSI0_BASE+SSI_O_CR0) &= (BIT4LO & BIT5LO);


    // DSS - bitfields 3:0, see p. 969, 0xF for 16 bit data 
    uint8_t DSS = (BIT0HI | BIT1HI | BIT2HI | BIT3HI);
    HWREG(SSI0_BASE+SSI_O_CR0) = (HWREG(SSI0_BASE+SSI_O_CR0) & ~DSS)| (0xF); 

    // 15. Locally enable interrupts (TXIM in SSIIM)
    HWREG(SSI0_BASE+SSI_O_IM) &= ~BIT3HI; // Enable TXIM - Check this 

    // 16. Make sure that the SSI is enabled for operation
    HWREG(SSI0_BASE+SSI_O_CR1) |= BIT1HI; //SSE; 

    //17. Enable the NVIC interrupt for the SSI when starting to transmit
    // see p. 104, table 2-9
    // SSI0 is vector number 23, int num 7 
    // p. 140 use EN0 for int 0 - 31 
    HWREG(NVIC_EN0) |= BIT7HI;

    // Erez added: enable interrupts globally
    __enable_irq();

}

uint16_t get_accel_x( void )
{
    return accel_x; 
}

uint16_t get_accel_y( void )
{
    return accel_y;   
}

uint16_t get_accel_z( void )
{
    return accel_z;    
}

uint16_t get_gyro_x( void )
{
    return gyro_x;
}


uint16_t get_gyro_y( void )
{
    return gyro_y; 
}


uint16_t get_gyro_z( void )
{
    return gyro_z;
}
