#include <iostream>
#include <gvars3/instances.h>

using namespace std;
using namespace GVars3;

struct Brighter { inline static bool test(int a) { return a == 'b'; } };
struct Darker   { inline static bool test(int a) { return a == 'd'; } };

template<class Type> inline bool is_corner(const char* str, int num_for_corner)
{
	//templating on num_for_corner makes a small increase in speed.
	int num_consecutive=0;
	int first_cons=0;
	
	//This is an approximation of 
	for(int i=0; i<16; i++)
	{
		if(Type::test(str[i]))
		{
			num_consecutive++;
			
			if(num_consecutive == num_for_corner)
				return 1;
		} 
		else
		{
			if(num_consecutive == i)
				first_cons=i;

			num_consecutive=0;
		}
	}
	
	if(first_cons+num_consecutive >=num_for_corner)
		return 1;
	else 
		return 0;
}

int main(int argc, char ** argv)
{	
	GUI.parseArguments(argc, argv);

	char types[]="bsd.";
	char F[17]="................";

	int a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p;
	cout << 16 << endl;
	cout << "[0 3] [1 3] [2 2] [3 1] [3 0] [3 -1] [2 -2] [1 -3] [0 -3] [-1 -3] [-2 -2] [-3 -1] [-3 0] [-3 1] [-2 2] [-1 3]" << endl;

	int N = GV3::get<int>("N", 9, 1);

	for(a = 0, F[ 0]='b'; a < 3; a++, F[ 0]=types[a])
	 for(b = 0, F[ 1]='b'; b < 3; b++, F[ 1]=types[b])
	  for(c = 0, F[ 2]='b'; c < 3; c++, F[ 2]=types[c])
	   for(d = 0, F[ 3]='b'; d < 3; d++, F[ 3]=types[d])
	    for(e = 0, F[ 4]='b'; e < 3; e++, F[ 4]=types[e])
	     for(f = 0, F[ 5]='b'; f < 3; f++, F[ 5]=types[f])
	      for(g = 0, F[ 6]='b'; g < 3; g++, F[ 6]=types[g])
	       for(h = 0, F[ 7]='b'; h < 3; h++, F[ 7]=types[h])
	        for(i = 0, F[ 8]='b'; i < 3; i++, F[ 8]=types[i])
	         for(j = 0, F[ 9]='b'; j < 3; j++, F[ 9]=types[j])
	          for(k = 0, F[10]='b'; k < 3; k++, F[10]=types[k])
	           for(l = 0, F[11]='b'; l < 3; l++, F[11]=types[l])
	            for(m = 0, F[12]='b'; m < 3; m++, F[12]=types[m])
	             for(n = 0, F[13]='b'; n < 3; n++, F[13]=types[n])
	              for(o = 0, F[14]='b'; o < 3; o++, F[14]=types[o])
	               for(p = 0, F[15]='b'; p < 3; p++, F[15]=types[p])
				    cout << F << " 1 " << (is_corner<Brighter>(F, N) || is_corner<Darker>(F, N)) << endl;
}
