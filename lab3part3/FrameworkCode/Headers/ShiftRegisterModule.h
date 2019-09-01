#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H
	void SR_Init(void);
	uint8_t SR_GetCurrentRegister(void);
	void SR_Write(uint8_t NewValue); 
#endif