/***************************************************************************
 Module
   LCDWrite.c

 Revision
   1.0.1

 Description
   This module acts as the low level interface to an LCD display in 4-bit
   mode with the actual data written to a shift register.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/12/15 15:15 jec     first pass
 
****************************************************************************/
//----------------------------- Include Files -----------------------------*/
// the common headers for C99 types 
#include <stdint.h>
#include <stdbool.h>

#include "BITDEFS.H"
#include "ShiftRegisterWrite.h"
#include "LCD_Write.h"

// Private functions
static void LCD_SetData4(uint8_t NewData);
static void LCD_PulseEnable(void);
static void LCD_RegisterSelect(uint8_t WhichReg);
static void LCD_Write4(uint8_t NewData);
static void LCD_Write8(uint8_t NewData);

// module level defines
#define LSB_MASK 0x0f
#define LCD_COMMAND 0
#define LCD_DATA    1
#define NUM_INIT_STEPS 10
#define USE_4_BIT_WRITE 0x8000

// these are the iniitalization values to be written to set up the LCD
static const uint16_t InitValues[NUM_INIT_STEPS] = {
  (0x0X | USE_4_BIT_WRITE), /* multi-step process to get it into 4-bit mode */
  (0x0X | USE_4_BIT_WRITE),  
  (0x0X | USE_4_BIT_WRITE),  
  (0x0X | USE_4_BIT_WRITE),  
    0xXX, /* 4-bit data width, 1 line, 5x7 font */
    0xXX, /* turn off the display */
    0x01, /* clear the display */
    0x07, /* increment and shift with each new character */
    0x0f, /* turn on display, cursor & blink */
    0xXX /* position cursor */};

// these are the delays between the initialization steps.
// the first delay is the power up delay so there is 1 more entry
// in this table than in the InitValues Table    
static const uint16_t InitDelays[NUM_INIT_STEPS+1] = {
    65535, /* use max delay for powerup */
     4100,
      100,
      100,
      100,
       53,
       53,
     3000,
       53,
       53,
       53 };

// place to keep track of which regiter we are writing to
  static uint8_t RegisterSelect=0;


/****************************************************************************
 Function
   LCD_HWInit
 Parameters
   None
 Returns
   Nothing
 Description
   Initializes the port hardware necessary to write to the LCD
 Notes
   This implementation uses the lower level shift register library so
   it simply calls the init for that library
 Author
   J. Edward Carryer, 10/12/15, 15:22
****************************************************************************/
void LCD_HWInit(void){
  // init the shift Register module
}

/****************************************************************************
 Function
   LCD_TakeInitStep
 Parameters
   None
 Returns
   uint16_t the number of uS to delay before the next step
 Description
   steps through the initialization value array with each call and send the
   initialization value to the LCD and returns the time to delay until the 
   next step. Return of 0 indicates the process is complete.
 Notes
   
 Author
   J. Edward Carryer, 10/12/15, 15:22
****************************************************************************/
uint16_t LCD_TakeInitStep(void){
  uint16_t CurrentInitValue;
  uint16_t Delay;
  // for keeping track of the initialization step
  // 0 is a special case of powerup, then it takes on 1 to NUM_INIT_STEPS       
  static uint8_t CurrentStep = 0;

  // if we have completed the initialization steps, 
    //set Delay value to zero
  // else
    // if we are just getting started, this is a special case and we inc
    // the CurrrentStep and set the Delay to the first entry in the 
    //InitDelays table
    //else
        // normal step, so grab the correct init value into CurrentInitValue
        // check to see if this is a 4-bit or 8-bit write and do the right kind
        // grab the correct delay for this step
    // set up CurrentStep for next call
  }    
  return Delay;
}

/****************************************************************************
 Function
   LCD_WriteCommand4
 Parameters
   uint8_t NewData; the 4 LSBs are written
 Returns
   Nothing
 Description
   clears the register select bit to select the command register then
   writes the 4 bits of data, then pulses (raises, then lowers) the enable 
   line on the LCD
 Notes
   This implementation uses the lower level shift register library so
   it calls that library to change the value of the pins connected
   to the LCD
 Author
   J. Edward Carryer, 10/12/15, 16:18
****************************************************************************/
void LCD_WriteCommand4(uint8_t NewData){
  // clear the register select bit
  // write the 4 LSBs to the shift register
}

/****************************************************************************
 Function
   LCD_WriteCommand8
 Parameters
   uint8_t NewData; 
 Returns
   Nothing
 Description
   clears the register select bit to select the command register then
   writes all 8 bits of data, using 2 4-bit writes
 Notes
   This implementation uses the lower level shift register library so
   it calls that library to change the value of the pins connected
   to the LCD
 Author
   J. Edward Carryer, 10/12/15, 16:26
****************************************************************************/
void LCD_WriteCommand8(uint8_t NewData){
  // clear the register select bit
  // write all 8 data bits to the shift register
}

/****************************************************************************
 Function
   LCD_WriteData8
 Parameters
   uint8_t NewData; 
 Returns
   Nothing
 Description
   sets the register select bit to select the data register then
   writes all 8 bits of data, using 2 4-bit writes
 Notes
   This implementation uses the lower level shift register library so
   it calls that library to change the value of the pins connected
   to the LCD
 Author
   J. Edward Carryer, 10/12/15, 16:28
****************************************************************************/
void LCD_WriteData8(uint8_t NewData){
  // set the register select bit
  // write all 8 bits to the shift register in 2 4-bit writes
}

//********************************
// thses functions are private to the module
//********************************
/****************************************************************************
 Function
   LCD_Write4
 Parameters
   uint8_t NewData; the 4 LSBs are written
 Returns
   Nothing
 Description
   writes the 4 bits of data, then pulses (raises, then lowers) the enable 
   line on the LCD
 Notes
   This implementation uses the lower level shift register library so
   it calls that library to change the value of the pins connected
   to the LCD
 Author
   J. Edward Carryer, 10/12/15, 15:58
****************************************************************************/
static void LCD_Write4(uint8_t NewData){
  // put the 4 bits of data onto the LCD data lines
  // pulse the enable line to complete the write
}

/****************************************************************************
 Function
   LCD_Write8
 Parameters
   uint8_t NewData; all 8 bits are written
 Returns
   Nothing
 Description
   writes the 8 bits of data, pulssing the Enable line between the 4 bit writes
 Notes
   This implementation uses the lower level shift register library so
   it calls that library to change the value of the pins connected
   to the LCD
 Author
   J. Edward Carryer, 10/12/15, 15:58
****************************************************************************/
static void LCD_Write8(uint8_t NewData){
  // put the 4 MSBs of data onto the LCD data lines
  // pulse the enable line to complete the write
  // put the 4 LSBs of data onto the LCD data lines
  // pulse the enable line to complete the write
}

/****************************************************************************
 Function
   LCD_RegisterSelect
 Parameters
   uint8_t Which; Should be either LCD_COMMAND or LCD_DATA
 Returns
   Nothing
 Description
   sets the port bit that controls the register select to the requested value
 Notes
   This implementation uses the lower level shift register library so
   it calls that library to change the value of the register select port bit
 Author
   J. Edward Carryer, 10/12/15, 15:28
****************************************************************************/
static void LCD_RegisterSelect(uint8_t WhichReg){
//  uint8_t CurrentValue;
    RegisterSelect = WhichReg;
}

/****************************************************************************
 Function
   LCD_SetData4
 Parameters
   uint8_t NewData; only the 4 LSBs are used
 Returns
   Nothing
 Description
   sets the 4 data bits to the LCD to the requested value and places the
   register select value in the correct Bit position (bit 1)
 Notes
   This implementation uses the lower level shift register library so
   it calls that library to change the value of the data pins
 Author
   J. Edward Carryer, 10/12/15, 15:42
****************************************************************************/
static void LCD_SetData4(uint8_t NewData){
  uint8_t CurrentValue;
  
  // get the current value of the port so that we can preserve the other
  // bit states 
  // insert the current state of RegisterSelect into bit 1
  // put the 4 LSBs into the 4 MSB positions to apply the data to the
  // correct LCD inputs while preserving the states of the other bits
  // now write the new value to the shift register
}

/****************************************************************************
 Function
   LCD_PulseEnable
 Parameters
   Nothing
 Returns
   Nothing
 Description
   pulses (raises, then lowers) the enable line on the LCD
 Notes
   This implementation uses the lower level shift register library so
   it calls that library to change the value of the data pin connected
   to the Enable pin on the LCD (bit 0 on the shift register)
 Author
   J. Edward Carryer, 10/12/15, 15:42
****************************************************************************/
static void LCD_PulseEnable(void){
  uint8_t CurrentValue;
  
  // get the current value of the port so that we can preserve the other
  // bit states
  // set the LSB of the byte to be written to the shift register
  // now write the new value to the shift register
  // clear the LSB of the byte to be written to the shift register
  // now write the new value to the shift register
}

