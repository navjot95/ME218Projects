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

#define PDIV                4       // SSI clock divisor 
#define SCR                 100     // SSI Clock Prescaler  
#define TICKS_PER_MS        40000
#define IMU_UPDATE_TIME     7      // 7 ms     

// https://github.com/brianc118/MPU9250/blob/master/MPU9250.h
#define BITS_DLPF_CFG_188HZ         0x01
#define BITS_FS_250DPS              0x00
#define BITS_FS_2G                  0x00
#define AK8963_I2C_ADDR             0x0c
#define AK8963_CNTL1                0x0A
#define AK8963_CNTL2                0x0B

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

static const uint8_t num_reads = GYR_Z_LOW - ACC_X_HIGH + 1; 
static uint8_t address_array[num_reads]; 
static uint8_t MyPriority; 
static SPI_Write_State_t CurrentState; 
typedef enum {Initializing, Reading} IMU_State_t; 
static IMU_State_t IMU_State; 
static uint16_t byteIn; 		    // 2 byte transfers
                
 


// ------------------------- Private Functions ---------------------------//
static void IMU_SPI_Init( void ); 
static void IMU_Write(uint8_t address, uint8_t data);

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
****************************************************************************/
bool InitIMU(uint8_t Priority)
{
    IMU_State = Initializing;           // For first initializatoin bits 
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
    SysCtlDelay(1600000u / 3u);  
        
        
      // #1  
      IMU_Write(MPU9250_USER_CTRL, BIT4HI);       
  SysCtlDelay(16000000u / 3u); 
    
    // https://github.com/brianc118/MPU9250/blob/master/MPU9250.cpp
    
    // #2
    IMU_Write(MPU9250_PWR_MGMT_1, PWR_MGMT_1_H_RESET); 
    SysCtlDelay(16000000u / 3u);  
    
    
    // # 3 
    IMU_Write(MPU9250_PWR_MGMT_1, 0x01);  // clock source 
    SysCtlDelay(16000000u / 3u);  
    
    // #4        
    IMU_Write(MPU9250_PWR_MGMT_2, 0x00); // Enable Acc & Gyro
    SysCtlDelay(16000000u / 3u);  
    

        
     //{my_low_pass_filter, MPUREG_CONFIG},     // Use DLPF set Gyroscope bandwidth 184Hz, temperature bandwidth 188Hz
     // #5 
     IMU_Write(MPU9250_CONFIG, BITS_DLPF_CFG_188HZ);
       SysCtlDelay(16000000u / 3u);  
       
       
    //    {BITS_FS_250DPS, MPUREG_GYRO_CONFIG},    // +-250dps
    // # 6 
      IMU_Write(MPU9250_GYRO_CONFIG, BITS_FS_250DPS); 
      SysCtlDelay(16000000u / 3u);  
      
      
    //{BITS_FS_2G, MPUREG_ACCEL_CONFIG},       // +-2G\
    // #7 
       IMU_Write(MPU9250_ACCEL_CONFIG, BITS_FS_2G);  
  SysCtlDelay(16000000u / 3u);  
  
    //{my_low_pass_filter_acc, MPUREG_ACCEL_CONFIG_2}, // Set Acc Data Rates, Enable Acc LPF , Bandwidth 184Hz
    // # 8 
        IMU_Write(MPU9250_ACCEL_CONFIG_2, BITS_DLPF_CFG_188HZ);
  SysCtlDelay(16000000u / 3u);  
  
  /*
    //{0x12, MPUREG_INT_PIN_CFG},      //
    // # 9 
    IMU_Write(MPU9250_INT_PIN_CFG, 0x12);  // OR 0x12? or 0x30 https://github.com/kriswiner/MPU9250/issues/62
  SysCtlDelay(160000000u / 3u); 
//    DONT DO     {0x30, MPUREG_USER_CTRL},        // I2C Master mode and set I2C_IF_DIS to disable slave mode I2C bus
 //   DONT DO     {0x0D, MPUREG_I2C_MST_CTRL},     // I2C configuration multi-master  IIC 400KHz
        
        
        
        // {0x30, MPUREG_USER_CTRL},        // I2C Master mode and set I2C_IF_DIS to disable slave mode I2C bus
        // #10 
  IMU_Write(MPU9250_USER_CTRL, 0x30);       
  SysCtlDelay(160000000u / 3u); 
  
 

        // # 11 
        IMU_Write(MPU9250_I2C_MST_CTRL, 0x0D);
        SysCtlDelay(160000000u / 3u);
        
        

        //{AK8963_I2C_ADDR, MPUREG_I2C_SLV0_ADDR},  // Set the I2C slave addres of AK8963 and set for write.
        // # 12 
        IMU_Write(MPU9250_I2C_SLV0_ADDR, AK8963_I2C_ADDR);
        SysCtlDelay(16000000u / 3u);
                


        //{AK8963_CNTL2, MPUREG_I2C_SLV0_REG}, // I2C slave 0 register address from where to begin data transfer
        // # 13 
            IMU_Write(MPU9250_I2C_SLV0_REG, AK8963_CNTL2); 
                SysCtlDelay(16000000u / 3u);

        //{0x01, MPUREG_I2C_SLV0_DO},   // Reset AK8963
        // #14 
        IMU_Write(MPU9250_I2C_SLV0_DO, 0x01);
            SysCtlDelay(16000000u / 3u);
        
        //{0x81, MPUREG_I2C_SLV0_CTRL}, // Enable I2C and set 1 byte
        // # 15 
        IMU_Write(MPU9250_I2C_SLV0_CTRL, 0x81); 
         SysCtlDelay(16000000u / 3u);


        //{AK8963_CNTL1, MPUREG_I2C_SLV0_REG}, // I2C slave 0 register address from where to begin data transfer
        // # 16 
               IMU_Write(MPU9250_I2C_SLV0_REG, AK8963_CNTL1); 
        SysCtlDelay(16000000u / 3u);
        
        
    // #17
               IMU_Write(MPU9250_I2C_MST_CTRL, 0x0D); 
        SysCtlDelay(16000000u / 3u);
        
        */
    // Other Configurations with blocking code 
    // To prevent switching into I2C mode when using SPI, the I2C interface 
    // should be disabled by setting the I2C_IF_DIS configuration bit. 
    // Setting this bit should be performed immediately after waiting 
    // for the time specified by the “Start-Up Time 
    // for Register Read/Write” in Section 6.3.    
    

        //{0x0D, MPUREG_I2C_MST_CTRL},     // I2C configuration multi-master  IIC 400KHz
        
//        {AK8963_I2C_ADDR, MPUREG_I2C_SLV0_ADDR},  // Set the I2C slave addres of AK8963 and set for write.
        //{0x09, MPUREG_I2C_SLV4_CTRL},
        //{0x81, MPUREG_I2C_MST_DELAY_CTRL}, // Enable I2C delay

  //      {AK8963_CNTL2, MPUREG_I2C_SLV0_REG}, // I2C slave 0 register address from where to begin data transfer
    //    {0x01, MPUREG_I2C_SLV0_DO},   // Reset AK8963
      //  {0x81, MPUREG_I2C_SLV0_CTRL}, // Enable I2C and set 1 byte
    
    //IMU_Write(MPU9250_USER_CTRL, BIT4HI);
    //SysCtlDelay(16000000u / 3u);
    
    // PAGE 44 - who am i
    // This register is used to verify the identity of the device. The contents of WHO_AM_I is an 8-bit device
    // ID. The default value of the register is 0x71.

    // Do this write then wait -- 

    // NEED TO DELAY 
    // page 53 
    // This register disables I2C bus interface. I2C bus interface 
    // is enabled in default. To disable I2C bus interface,
    // write “00011011” to I2CDIS register. Then I2C bus interface is disabled. - this is for magnetometer

    
    



    MyPriority = Priority;

    // 

    
    bool returnValue = false; 
    
    if (ES_Timer_InitTimer(IMU_TIMER, IMU_UPDATE_TIME) == ES_Timer_OK)  
    {
        returnValue = true;
    } 
    return returnValue;   
}


static void IMU_Write(uint8_t address, uint8_t data)
{
    uint16_t out_byte = 0; // MSB clear for write 
    out_byte |= (address << 8);
    out_byte |= data;
    HWREG(SSI0_BASE+SSI_O_IM) |= BIT3HI;    // Enable TXIM
    HWREG(SSI0_BASE+SSI_O_DR) = out_byte;   // write a new byte to the FIFO
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
            printf("\r\nX_ACC: %d Y_ACC: %d Z_ACC: %i\n", (int) accel_x, (int) accel_y, (int) accel_z); 
            printf("\rX_GYRO: %d Y_GYRO: %d Z_GYRO: %x\n\n", (int) gyro_x, (int) gyro_y,  gyro_z); 
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
            accel_x = (accel_x & 0x00FF) | (byteIn << 8);
            break;
        case ACC_X_LOW:
            accel_x = (accel_x & 0xFF00) | (byteIn & 0x00FF);
            break;
        case ACC_Y_HIGH:
            accel_y = (accel_x & 0x00FF) | (byteIn << 8);
            break;
        case ACC_Y_LOW:
            accel_y = (accel_y & 0xFF00) | (byteIn & 0x00FF);
            break;
        case ACC_Z_HIGH:
            accel_z = (accel_x & 0x00FF) | (byteIn << 8);
            break;
        case ACC_Z_LOW:
            accel_z = (accel_z & 0xFF00) | (byteIn & 0x00FF);
            break;
        case GYR_X_HIGH:
            gyro_x = (gyro_x & 0x00FF) | (byteIn << 8);
            break;
        case GYR_X_LOW:
            gyro_x = (gyro_x & 0xFF00) | (byteIn & 0x00FF);
            break;
        case GYR_Y_HIGH:
            gyro_y = (gyro_y & 0x00FF) | (byteIn << 8);
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
    //HWREG(GPIO_PORTA_BASE+GPIO_O_DEN) |= BIT6HI; 
    //HWREG(GPIO_PORTA_BASE+GPIO_O_DIR) |= BIT6HI;
    //HWREG(GPIO_PORTA_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~BIT6HI; 
    
    
    // 8. If using SPI mode 3, program the pull-up on the clock line
    /* This pin is configured as a GPIO by default but is locked and 
    can only be reprogrammed by unlocking the pin in the GPIOLOCK
    register and uncommitting it by setting the GPIOCR register. */
    // see p. 1329, p. 650, p. 684 (LOCK) - handled by calling PortFunctions

    HWREG(GPIO_PORTA_BASE+GPIO_O_PUR) |= BIT2HI; // pullup on clock line 
     //HWREG(GPIO_PORTA_BASE+GPIO_O_PUR) |= BIT2HI; // pullup on clock line     -- = 1? NOTE TRY THID 


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
