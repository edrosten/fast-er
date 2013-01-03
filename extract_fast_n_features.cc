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
\file extract_features.cc Main file for the extract_features executable.

\section wpUsage Usage

<code> extract_fast_n_features [--N n] [--threshold T] IMAGE1 [IMAGE2 ...]</code>

\section Description

Extracts features so that an 
accelerated tree can be learned. The output is is suitable for consumption 
by \link learn_fast_tree.cc learn_fast_tree\endlink.

The program accpets standard GVars3 commandline arguments. 

The images from which
features should be extracted are specified on the commandline. 

*/

#include <gvars3/instances.h>
#include <cvd/image_io.h>
#include <cvd/fast_corner.h>
#include <stdint.h>
#include <map>
#include <iterator>
#include <string>

using namespace std;
using namespace CVD;
using namespace GVars3;

static const char BrighterFlag = 'b'; ///< Character code for pixels significantly brighter than the centre
static const char DarkerFlag = 'd';///< Character code for pixels significantly darker than the centre
static const char SimilarFlag = 's';///< Character code for pixels similar to the centre

///Extracts a feature from an image.
///@param s String to extract feature in to
///@param im Image to extract feature from
///@param pos Location to extract feature from
///@param barrier Threshold used to compute feature
void extract_feature(string& s, const BasicImage<byte>& im, const ImageRef&  pos, int barrier)
{
	int cb = im[pos] + barrier;
	int c_b = im[pos] - barrier;

	for(int i=0; i < 16; i++)
	{
		int pix = im[pos + fast_pixel_ring[i]];
	
		if(pix > cb)
			s[i] = BrighterFlag;
		else if(pix < c_b)
			s[i] = DarkerFlag;
		else
			s[i] = SimilarFlag;
	}
}

///Test if a pixel is brighter than centre+threshold, for is_corner().
///@param val Pixel value
///@param centre Pixel value at the ring centre
///@param barrier Threshold
inline bool positive(int val, int centre, int barrier)
{
	return val > centre + barrier;
}

///Test if a pixel is darker than centre-threshold, for is_corner().
///@param val Pixel value
///@param centre Pixel value at the ring centre
///@param barrier Threshold
inline bool negative(int val, int centre, int barrier)
{
	return val < centre - barrier;
}

///Tests a pixel ring to see if it passes half of the segment test.
///@param im Image in which to perform test
///@param off Position at which to apply test
///@param barrier Threshold
///@param num_for_corner Number of contiguous pixels required
///@param test Which test to perform, bright or dark
template<class Test>
inline int is_corner(const SubImage<byte>& im, const ImageRef off, int barrier, int num_for_corner, const Test& test)
{
	int num_consecutive=0;
	int first_cons=0;
	const int centre = im[off];

	for(int i=0; i<16; i++)
	{
		int val = im[fast_pixel_ring[i] + off];
		if(test(val, centre, barrier))
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
	
	return first_cons+num_consecutive >=num_for_corner;
}


///Driving program
///@param argc Number of commandline arguments
///@param argv List of commandline arguments. Contains GVars3 arguments, and images to process.
int main(int argc, char** argv)
{
	int lastarg = GUI.parseArguments(argc, argv);

	//Store corners and noncorners by the string representing the feature.
	map<string, uint64_t> corners, non_corners;
	
	//Scratch string of the correct length for extracting features in to.
	string scratch(16, '.');
    
	int threshold = GV3::get<int>("threshold", 30);
	int N = GV3::get<int>("N", 9);
	
	//Iterate over all images, extracting features
	for(int i=lastarg; i < argc; i++)
	{
		cerr << argv[i] << endl;
		try{
			Image<byte> im = img_load(argv[i]);
			for(int r=3; r < im.size().y - 3; r++)
				for(int c=3; c < im.size().x - 3; c++)
				{
					ImageRef pos(c,r);
					//Test for cornerness
					bool is = is_corner(im, pos, threshold, N, positive)  || is_corner(im, pos, threshold, N, negative);

					//Extracting the features, and inserting them in to the
					//correct bin.
					extract_feature(scratch, im, pos, threshold);

					if(is)
						corners[scratch]++;
					else
						non_corners[scratch]++;
				}
			cerr << "Processed " << argv[i] << endl;
		}
		catch(Exceptions::All e)
		{
			cerr << "Failed to load " << argv[i] << ": " << e.what << endl;
		}
	}

	cout << 16 << endl;
	copy(fast_pixel_ring, fast_pixel_ring + 16, ostream_iterator<ImageRef>(cout, " "));
	cout << endl;

	for(map<string, uint64_t>::iterator i=corners.begin(); i != corners.end(); i++)
		cout << i->first << " " << i->second << " 1" << endl;
	for(map<string, uint64_t>::iterator i=non_corners.begin(); i != non_corners.end(); i++)
		cout << i->first << " " << i->second << " 0" << endl;
}
