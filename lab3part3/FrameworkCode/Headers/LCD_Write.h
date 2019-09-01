#ifndef LCD_Write_H
#define LCD_Write_H

// the common headers for C99 types 
#include <stdint.h>
#include <stdbool.h>

#define INTER_CHAR_DELAY 53

void LCD_HWInit(void);
uint16_t LCD_TakeInitStep(void);
void LCD_WriteCommand4(uint8_t NewData);
void LCD_WriteCommand8(uint8_t NewData);
void LCD_WriteData8(uint8_t NewData);

#endif //LCD_Write_H