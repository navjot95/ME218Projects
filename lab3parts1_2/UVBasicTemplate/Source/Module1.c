
/* test function 1 for CodeWarrior Tutorial */
#include <stdio.h>
// a prototype is not really necessary here,
// but I want to set a good example
//void Module1Function1( void);
void Module1Function1( void)
{
 puts("This is Module 1 Function 1\r\n");
}



#ifdef TEST
/* test Harness for testing this module */
#include "termio.h"
int main(void)
	

{
 TERMIO_Init();
 puts("\r\n In test harness for Module 1\r\n");
 Module1Function1();

 return 0;
}
#endif 