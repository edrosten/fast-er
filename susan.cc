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
#include <cvd/image.h>
#include <cvd/convolution.h>
#include <cvd/vision.h>
#include <cvd/image_convert.h>

#include <vector>
#include <gvars3/instances.h>

#include "susan.h"

using namespace std;
using namespace CVD;
using namespace GVars3;

////////////////////////////////////////////////////////////////////////////////
//
// SUSAN interface
//



extern "C"
{
	void free_haxored_memory();
	int*  susan(unsigned char* in, int x_size, int y_size, float dt, int bt);
}


void SUSAN::operator()(const CVD::Image<CVD::byte>& im, std::vector<CVD::ImageRef>& corners, unsigned int N) const
{
	float dt = GV3::get<float>("susan.dt", 4.0, 1);
	int* c = susan(const_cast<byte*>(im.data()), im.size().x, im.size().y, dt, N);

	int n = c[0];

	for(int i=0; i < n; i++)
		corners.push_back(ImageRef(c[2*i+2], c[2*i+3]));

	free_haxored_memory();
}
