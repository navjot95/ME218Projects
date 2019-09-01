#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "ES_Port.h"
#include "termio.h"

#define clrScrn() 	printf("\x1b[2J")


int main(void)
{  
  // initialize the timer sub-system and console I/O
  _HW_Timer_Init(ES_Timer_RATE_1mS);
	TERMIO_Init();
	clrScrn();

	// When doing testing, it is useful to announce just which program
	// is running.
	puts("\rStarting Basic Template \r");
	printf("%s %s\n",__TIME__, __DATE__);
	printf("\n\r\n");

	// Your hardware initialization function calls go here

  // your program goes here
  putchar('A');
  getchar();
  putchar('l');
  getchar();
  putchar('l');
  getchar();
  putchar(' ');
  getchar();
  putchar('i');
  getchar();
  putchar('s');
  getchar();
  putchar(' ');
  getchar();
  putchar('g');
  getchar();
  putchar('o');
  getchar();
  putchar('o');
  getchar();
  putchar('d');
  getchar();

  
	// if you fall off the end of your code, then hang around here
	for(;;)
	  ;

}
