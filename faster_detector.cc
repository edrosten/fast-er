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
#include <fstream>
#include <cmath>
#include <cstring>
#include <cerrno>
#include <vector>
#include <utility>
#include <algorithm>

#include <cvd/image.h>
#include <cvd/byte.h>
#include <cvd/vector_image_ref.h>

#include <sys/mman.h>

#include <TooN/TooN.h>
#include <TooN/helpers.h>

#include "offsets.h"
#include "faster_detector.h"
#include "faster_tree.h"
#include "faster_bytecode.h"

#include <gvars3/instances.h>


using namespace std;
using namespace CVD;
using namespace GVars3;
using namespace TooN;


//Generate a detector, and compute its repeatability for all the tests.
//
//@param argc Number of command line arguments
//@ingroup gRepeatability
void init()
{
	static bool once=0;

	if(!once)
	{
		create_offsets();
		once = 1;
	}
}


faster_learn::faster_learn(const std::string& fname)
{
	init();
	ifstream i;
	i.open(fname.c_str());

	if(!i.good())
	{
		cerr << "Error: " << fname << ": " << strerror(errno) << endl;
		exit(1);
	}
	
	try{
		tree.reset(load_a_tree(i));
	}
	catch(ParseError p)
	{
		cerr << "Parse error in " << fname << endl;
		exit(1);
	}

	if(GV3::get<bool>("faster_tree.print_tree", 0, 1))
	{
		clog << "Tree:" << endl;
		tree->print(clog);
	}

	if(GV3::get<bool>("faster_tree.print_block", 0, 1))
	{
		block_bytecode f2 = tree->make_fast_detector(100);
		f2.print(clog,  100);
	}
}


void faster_learn::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& v, unsigned int t) const
{
	Image<int> scratch(i.size(), 0);

	v = tree_detect_corners(i, tree.get(), t, scratch);
}
