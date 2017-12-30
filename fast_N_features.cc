/*

    This file is part of the FAST-ER machine learning system.
    Copyright (C) 2008  Edward Rosten and Los Alamos National Laboratory

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
/**
\file fast_N_features.cc Main file for the fant_N_features executable.

\section wpUsage Usage

<code> ./fast_N_features [--NUM N] | ./learn_fast_tree </code>

\section Description

This program generates a list of all possible FAST-N features in an output
format suitable for consumption by \link learn_fast_tree.cc learn_fast_tree\endlink.
The program accepts standarg GVars3 commandline arguments. The only useful argument is
N which specifies the N for which FAST-N features should be generated.

*/

#include <iostream>
#include <gvars3/instances.h>

///\cond never
using namespace std;
using namespace GVars3;
///\endcond


///Determine if a string has the properties of a FAST-N corner.
///In other words, if it has enough consecutive characters of the correct
///type. This function assumes that the string wraps in a circular manner.
///@param str String to test for cornerness.
///@param num_for_corner Number of consecutive characters required for corner
///@param type Character value which must appear consecutively#
///@return whether the string is a corner.
inline bool is_corner(const char* str, int num_for_corner, char type)
{
	int num_consecutive=0;
	int first_cons=0;
	
	for(int i=0; i<16; i++)
	{
		if(str[i] == type)
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

///This is the main function for this program. It generates all possible FAST pixel rings
///using brute-force and outputs them along with their class and a uniform weighting over all
///features.
///@param argc Number of commandline arguments
///@param argv List of commandline arguments
int main(int argc, char ** argv)
{	
	GUI.parseArguments(argc, argv);
	cout.sync_with_stdio(false);

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
				    cout << F << " 1 " << (is_corner(F, N, 'b') || is_corner(F, N, 'd')) << endl;
}
