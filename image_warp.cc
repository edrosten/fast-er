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
\file image_warp.cc Main file for the image_warp executable.

\section wpUsage Usage

<code> image_warp [--num NUM_IMAGES] [--dir DIR] [--type TYPE] [--out OUT_DIR] [--stub OUT_STUB]</code>

\section Description

Loads a dataset (NUM, DIR and TYPE specify the dataset according to  ::load_data)
and warp every image to look like every other image. The output is placed in
the image <code>./dir/warp_TO_FROM.jpg</code>. You need to create the output 
directory yourself.

By flipping through the images with the same value of TO, you can see the
quality of alignment within a dataset.

*/


#include <iostream>
#include <cvd/image_io.h>
#include <cvd/image_interpolate.h>
#include <gvars3/instances.h>
#include "varprintf/varprintf.h"

#include "load_data.h"
#include "utility.h"

using namespace std;
using namespace CVD;
using namespace varPrintf;
using namespace GVars3;
using namespace TooN;

///Warp one image to look like another, using bilinear interpolation
///@param in The image to warp
///@param warp  The warp to use to warp the image
///@return The warped image
Image<CVD::byte> warp_image(const Image<CVD::byte>& in, const Image<array<float, 2> >& warp)
{
	Image<CVD::byte> ret(in.size(), 0);

	image_interpolate<Interpolate::Bilinear, CVD::byte> interp(in);

	for(int y=0; y < ret.size().y; y++)
		for(int x=0; x < ret.size().x; x++)
		{
			if(warp[y][x][0] != -1 && interp.in_image(Vec(warp[y][x])))
				ret[y][x] = interp[Vec(warp[y][x])];
		}

	return ret;
}

///Driving function
///@param argc Number of command line arguments
///@param argv Commandline argument list
int main(int argc, char** argv)
{
	try
	{
		//Load command line arguments
		GUI.parseArguments(argc, argv);

		vector<Image<CVD::byte> > images;
		vector<vector<Image<array<float, 2> > > > warps;

		//Extract arguments relavent to loading a dataset
		int n = GV3::get<int>("num", 2, 1);
		string dir = GV3::get<string>("dir", "./", 1);
		string format = GV3::get<string>("type", "cambridge", 1);

		//Load the dataset
		tie(images, warps) = load_data(dir, n, format);

		//Generate the output printf string	
		string out = GV3::get<string>("out", "./out/", 1) + "/" + GV3::get<string>("stub", "warped_%i_%i.jpg", 1);

		//Warp every image to look like every other image
		//where this makes sense.
		for(int to = 0; to < n; to++)
			for(int from=0; from < n; from ++)
				if(from != to)
				{
					Image<CVD::byte> w = warp_image(images[from], warps[to][from]);
					img_save(w, sPrintf(out, to, from));

					cout << "Done " << from << " -> " << to << endl;
				}
				else
				{
					img_save(images[from], sPrintf(out, to, from));
				}
	}
	catch(const Exceptions::All& e)
	{
		cerr << "Error: " << e.what() << endl;
	}	
}
