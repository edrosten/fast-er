I'm sure everyone else has already written a C++11 variadic printf function 
but here's my contribution to the pile.

I expect it's not the fastest or the most complete, but it's a single header
file that does a fine job if you want parameterized formats provided by 
a string, or if you just like using printf format strings.

It's super permissive overall, and relies on working operator<<. It will
probably do more or less sensible things for user defined formatting
operators. The format identifier, i.e. the s or i in %s and %i are really 
nothing more than a hint that affects the meaning of te various other bits 
of the format string. You can print integers with %s if you really want to.


```
#include "varprintf.h"

using namespace varPrintf;
int main()
{
	
	Printf("hello, world\n");
	fPrintf(std::cerr, "int i = %10.3x, %e, %10.3f %i %s %i\n", 10, 100, 123.456789, "p");
	std::cout << sPrintf("b0rk %@{}_3z\n");
}
```

prints:
```
hello, world
int i =          a, 100,    123.457 p <missing value> <missing value>
b0rk <Malformed format>%@{}_3z
```

There are 3 functions provided, Printf (prints to stdout), fPrintf (prints to
a stream provided as the first argument) and sPrintf (returns a std::string).
Since it's done with parameter packs, fPrintf is also equivalent to vfprintf so
there's no seperate v forms of the function.

You can dump this header anywhere into your source tree, or if you're feeling very 
fancy, you can splat it in with a git subtree to get the steady stream of updates
and bugfixes that I'm sure will be coming any decade now.

By the way, it's a direct port of the one I wrote for this mishmash of stuff:

https://www.edwardrosten.com/cvd/tag.html
