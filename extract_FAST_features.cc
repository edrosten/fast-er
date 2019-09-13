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
\file extract_FAST_features.cc Main file for the extract_features executable.

\section wpUsage Usage

<code> extract_features [--VAR VAL] [--exec FILE] IMAGE1 [IMAGE2 ...]</code>

\section Description

This program extracts FAST-N features from video or files.  The output is is suitable for consumption by \link learn_fast_tree.cc learn_fast_tree\endlink.

The program accpets standard GVars3 commandline arguments, and the default
parameters are contained in \p extract_features.cfg :

\include extract_features.cfg

The images from which
features should be extracted are specified on the commandline. 

*/

#include <gvars3/instances.h>
#include <cvd/image_io.h>
#include <cvd/videosource.h>
#include <stdint.h>
#include <map>
#include <iterator>
#include <string>


using namespace CVD;
using namespace std;
using namespace GVars3;

ImageRef dir[17]=
{
	ImageRef(0,3),
	ImageRef(1,3),
	ImageRef(2,2),
	ImageRef(3,1),
	ImageRef(3,0),
	ImageRef(3,-1),
	ImageRef(2,-2),
	ImageRef(1,-3),
	ImageRef(0,-3),
	ImageRef(-1,-3),
	ImageRef(-2,-2),
	ImageRef(-3,-1),
	ImageRef(-3,0),
	ImageRef(-3,1),
	ImageRef(-2,2),
	ImageRef(-1,3),
};

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


string extract(const BasicImage<byte>& imp, ImageRef pos, int threshold)
{
	int cb = imp[pos] + threshold;
	int c_b = imp[pos] + threshold;
	
	string res;
	res.resize(16);
	for(int i=0; i < 16; i++)
		if(imp[pos + dir[i]] > cb)
			res[i] = 'b';
		else if(imp[pos + dir[i]] < c_b)
			res[i] = 'd';
		else
			res[i] = 's';
	return res;
}

///Driving program
///@param argc Number of commandline arguments
///@param argv List of commandline arguments. Contains GVars3 arguments, and images to process.
int main(int argc, char** argv)
{
	int lastarg = GUI.parseArguments(argc, argv);
	
	int t = GV3::get<int>("threshold", 10);
	int N = GV3::get<int>("N", 9);
	int frames = GV3::get<int>("frames", 1000);

	VideoBuffer<byte>* vbuf = open_video_source<byte>(GV3::get<string>("video", "", FATAL_IF_NOT_DEFINED));

	map<string, uint64_t> corners;
	map<string, uint64_t> non_corners;

	for(int nf = 0; nf < frames; nf++)
	{
		VideoFrame<byte>* vf = vbuf->get_frame();

		for(int y=3; y < vf->size().y-3; y++)
			for(int x=3; x < vf->size().x-3; x++)
			{
				string s = extract(*vf, ImageRef(x, y), t);

				if(is_corner(s.c_str(), N, 'b') || is_corner(s.c_str(), N, 'd'))
					corners[s]++;
				else
					non_corners[s]++;
			}

		vbuf->put_frame(vf);
	
		clog << nf << endl;
	}

	cout << 16 << endl;
	for(int i=0; i < 16; i++)
		cout << dir[i] << " ";
	cout << endl;

	for(map<string, uint64_t>::iterator i=corners.begin(); i != corners.end(); i++)
		cout << i->first << " " << i->second << " 1" << endl;
	for(map<string, uint64_t>::iterator i=non_corners.begin(); i != non_corners.end(); i++)
		cout << i->first << " " << i->second << " 0" << endl;

}
