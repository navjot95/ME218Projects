/*****************************************************************************
 Module
   IMU_SPI.c
   
 Description
   Module for running SPI to Communicate with IMU

 Author
	E. Krimsky, ME218B
****************************************************************************/



#include "IMU_SPI.h"

#define TEST
#define PDIV 5 // SSI clock divisor 
#define SCR 10 
#define TICKS_PER_MS 40000

# Masks for reading and writing SPI to IMU




#define INTD_SHIFT 29

static uint16_t byteIn; 		// 2 byte transfers
static uint16_t sendByte;  		// 2 byte transfers

// ------- Private Functions --------------------//
static void SPI_InitPeriodic( void );

/****************************************************************************
 Function
     SPI_Init

 Parameters
     none

 Returns
     non
     
 Description
    Initializes hardware for SPI module 0 (on port A) and configures a debug
    line on PA6
****************************************************************************/

void SPI_Init( void )
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

	// DSS - bitfields 3:0, // 0x7 for 8 bit data -- TODO: change this for multi-byte transfers 
	uint8_t DSS = (BIT0HI | BIT1HI | BIT2HI | BIT3HI);
	HWREG(SSI0_BASE+SSI_O_CR0) = (HWREG(SSI0_BASE+SSI_O_CR0) & ~DSS)| (0x7); 

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
    SPI_InitPeriodic();
}


/****************************************************************************
 Function
     IMU_Write

 Parameters
     byteOut - byte to write out 

 Returns
     none
     
 Description
    Writes out bye over SPI
****************************************************************************/
void IMU_Write(uint8_t byteOut)
{
    sendByte = byteOut;
}




/****************************************************************************
 Function
     IMU_Read

 Parameters
     byteOut - byte to write out 

 Returns
     none
     
 Description
    Writes out bye over SPI
****************************************************************************/
void IMU_Read( void )
{
    // set local variables for all the readings 
	
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
    HWREG(GPIO_PORTA_BASE+(GPIO_O_DATA + ALL_BITS)) &= ~BIT6HI;	
    byteIn = HWREG(SSI0_BASE+SSI_O_DR); 
    HWREG(GPIO_PORTA_BASE+(GPIO_O_DATA + ALL_BITS)) |= BIT6HI;	
}


/****************************************************************************
 Function
     SPI_InitPeriodic

 Parameters
     none

 Returns
     none

 Description
     Initializes Periodic timer for updating the SPI read value 

****************************************************************************/
static void SPI_InitPeriodic( void )
{
    // enable clock to wide timer 0 
    HWREG(SYSCTL_RCGCWTIMER) |= SYSCTL_RCGCWTIMER_R0;
    
    // wait for clock to get going 
    while((HWREG(SYSCTL_PRWTIMER) & SYSCTL_PRWTIMER_R0) != SYSCTL_PRWTIMER_R0)
        ;
    // Disbale timer B before configuring
    HWREG(WTIMER0_BASE+TIMER_O_CTL) &= ~TIMER_CTL_TBEN;
    
    // set in individual (not concatenated) -- make sure this doesn't mess 
    // with other timer
    HWREG(WTIMER0_BASE+TIMER_O_CFG) = TIMER_CFG_16_BIT;
    
    // set up timer B in periodic mode 
    HWREG(WTIMER0_BASE+TIMER_O_TBMR) = (HWREG(WTIMER0_BASE+TIMER_O_TBMR) 
            & ~TIMER_TBMR_TBMR_M) | TIMER_TBMR_TBMR_PERIOD;

    // set timeout to 2 ms 
    HWREG(WTIMER0_BASE+TIMER_O_TBILR) = TICKS_PER_MS * 2;
    
    // enable local timeout interrupt
    HWREG(WTIMER0_BASE+TIMER_O_IMR) |= TIMER_IMR_TBTOIM;
    
    // enable in the NVIC, Int number 95 so 95-64 = BIT31HI, NVIC_EN2
    HWREG(NVIC_EN2) |= BIT31HI;
    
    // set as priority 1 ( encoder edge will have default priority 0) 
    // int 95 so PRI23 and using INTD
    // left shift by 29 because intD 
    HWREG(NVIC_PRI23) = (HWREG(NVIC_PRI23) & NVIC_PRI23_INTD_M) | 
                                ( 1 << INTD_SHIFT);
    
    // enable interrupts globally
    __enable_irq();
    
    // start the timer and set to stall with debugger
    HWREG(WTIMER0_BASE+TIMER_O_CTL) |= (TIMER_CTL_TBEN | TIMER_CTL_TBSTALL);
    
}


/****************************************************************************
 Function
     SPI_PeriodicResponse

 Parameters
     none

 Returns
     none

 Description
     Interrupt response routine for input capture interupt

****************************************************************************/
void SPI_PeriodicResponse ( void )
{
    // clear the source of the interupt
    HWREG(WTIMER0_BASE+TIMER_O_ICR) = TIMER_ICR_TBTOCINT;   
    
    HWREG(SSI0_BASE+SSI_O_IM) |= BIT3HI; // Enable TXIM
	HWREG(SSI0_BASE+SSI_O_DR) = sendByte;
}

#ifdef TEST
int main (void)
{
    // Set the clock to run at 40MhZ using the PLL and 16MHz external crystal
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN
                            | SYSCTL_XTAL_16MHZ);
    TERMIO_Init();
    PortFunctionInit();
    printf("\x1b[2J"); // clear screen 

    // When doing testing, it is useful to announce just which program
    // is running.
    puts("\rStarting Test Harness for SPI\r");
    SPI_Init(); 
    while (1)
    {

        
        
    }   
    return 1; 
}
#endif
