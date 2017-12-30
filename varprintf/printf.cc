#include "varprintf.h"

using namespace varPrintf;
int main()
{
	
	Printf("hello, world\n");
	fPrintf(std::cerr, "int i = %10.3x, %e, %10.3f %i %s %i\n", 10, 100, 123.456789, "p");
	std::cout << sPrintf("b0rk %@{}_3z\n");
}
