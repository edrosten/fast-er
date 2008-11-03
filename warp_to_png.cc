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
\file warp_to_png.cc Main file for the warp_to_png executable.

\section wpUsage Usage

<code> warp_to_png [--size "x y"] \< </code>\e infile.warp \c \> \e outfile.png

\section Description

Converts a \ref camDataset "text warp file" in to a \ref canPNG "PNG warp file". The size is used to specify the image shape to convert to
and defaults to 768 by 576.

*/


#include <iostream>
#include <iterator>
#include <vector>
#include <cstdlib>

#include <cvd/image_io.h>

#include <gvars3/instances.h>

#include "warp_to_png.h"

///\cond never
using namespace std;
using namespace CVD;
using namespace GVars3;
using namespace TooN;
///\endcond


///Driving function
///@param argc Number of command line arguments
///@param argv Commandline argument list
int main(int argc, char** argv)
{
	try
	{
		GUI.parseArguments(argc, argv);

		ImageRef size = GV3::get<ImageRef>("size", ImageRef(768,576), 1);

		
		Image<Rgb<unsigned short> > si(size);

		for(int y=0; y < size.y; y++)
			for(int x=0; x < size.x; x++)
			{
				float f1, f2;
				cin >> f1 >> f2;

				if(!cin.good())
				{
					cerr << "EOF!\n";
					exit(1);
				}

				if(f1 < -5 || f1 > 1000)
				{
					cerr << "Bad value at " << x << ", " << y << ": " << f1;
					exit(2);
				}

				if(f2 < -5 || f2 > 1000)
				{
					cerr << "Bad value at " << x << ", " << y << ": " << f2;
					exit(2);
				}

				Rgb<unsigned short> o;

				o.red = (unsigned short) ((SHIFT + f1)*MULTIPLIER + .5);
				o.green = (unsigned short) ((SHIFT + f2)*MULTIPLIER + .5);
				o.blue = 0;

				si[y][x] = o;
			}

		img_save(si, cout, ImageType::PNG);
	}
	catch(Exceptions::All e)
	{
		cerr << "Error: " << e.what << endl;
		return 1;
	}	
}
