/****************************************************************************
 Module
   Init_UART.c

 Description
   Initialize UART for communication with PIC and XBee

****************************************************************************/

/*----------------------------- Include Files -----------------------------*/

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"  // Define PART_TM4C123GH6PM in project
#include "driverlib/gpio.h"
#include "termio.h"
#include "inc/hw_nvic.h"
#include "inc/hw_pwm.h"
#include "inc/hw_timer.h"
#include "inc/hw_ssi.h"
#include "inc/hw_uart.h"

#include "Init_UART.h"

/*---------------------------- Module Functions ---------------------------*/

void Init_UART_PIC(void)
{
  // Enable the UART module
  HWREG(SYSCTL_RCGCUART) |= SYSCTL_RCGCUART_R1;

  // Wait for the UART to be ready
  while((HWREG(SYSCTL_PRUART) & SYSCTL_PRUART_R1)!= SYSCTL_PRUART_R1)
  {
  }
  
  // Enable Port B clock
  HWREG(SYSCTL_RCGCGPIO) |= SYSCTL_RCGCGPIO_R1;
  
  // Wait for the GPIO module to be ready
  while((HWREG(SYSCTL_PRGPIO) & SYSCTL_PRGPIO_R1)!= SYSCTL_PRGPIO_R1)
  {
  }  
  
  // Set PB0 and PB1 to digital 
  HWREG(GPIO_PORTB_BASE + GPIO_O_DEN) |= (BIT0HI | BIT1HI); 
  
  // PB0 input (RX), PB1 output (TX)
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) |= BIT1HI; 
  HWREG(GPIO_PORTB_BASE + GPIO_O_DIR) &= ~BIT0HI; 
  
  // Select the Alternate function for PB0 and PB1
  HWREG(GPIO_PORTB_BASE + GPIO_O_AFSEL) |= (BIT0HI | BIT1HI);
  
  // Configure PMCn fields in the GPIO_PCTL register to assign the UART pins
  // (pg 1351) Write 1 for PB0 and PB1 for U1Rx and U1Tx 
  HWREG(GPIO_PORTB_BASE + GPIO_O_PCTL) |= (BIT0HI | BIT1HI);

  // Disable UART by clearing the UARTEN bit in the UART_CTL register
  HWREG(UART1_BASE + UART_O_CTL) &= ~UART_CTL_UARTEN;
  
  // Set to 9600 Baud (pg 896)
  // BRD = BRDI + BRDF = UARTSysClk/(ClkDiv * BaudRate) = 260 + 0.41667
  // IBRD: 260 = 0x104
  // FBRD: int(BRDF * 64 + 0.5) = 27 = 0x1B
  HWREG(UART1_BASE + UART_O_IBRD) = 0x104;
  HWREG(UART1_BASE + UART_O_FBRD) = 0x1B;
  
  // Write the desired serial parameters to the UART_LCRH register
  // We want 8 bits
  HWREG(UART1_BASE + UART_O_LCRH) |= UART_LCRH_WLEN_8;
  
  // Set Recieve, Transmit, and End of Transmission bits
  // Enable UART
  HWREG(UART1_BASE + UART_O_CTL) |= (UART_CTL_RXE| UART_CTL_TXE | 
        UART_CTL_EOT | UART_CTL_UARTEN);

}
