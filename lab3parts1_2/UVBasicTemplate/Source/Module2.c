
/* test function 2 for CodeWarrior Tutorial */
#include <stdio.h>
#include "Module1.h"
// a prototype is not really necessary here, but I want to set a good example
void Module2Function1( void);
void Module2Function1( void)
{
 puts("This is Module 2 Function 1\r\n");
}
#ifdef TEST
/* test Harness for testing this module */
#include "termio.h"
int main(void)
{
 TERMIO_Init();
 puts("\n\r In test harness for Module 2\r\n");
 Module1Function1();
 Module2Function1();
 return 0;
}
#endif 