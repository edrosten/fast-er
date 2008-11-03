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
#ifndef FASTER_LEARN_H
#define FASTER_LEARN_H

#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <string>
#include <memory>


#include "detectors.h"

struct tree_element;

///FAST-ER detector
///@ingroup gDetect
struct faster_learn: public DetectT
{
	///Detect corners
	///@param i Image in which to detect corners
	///@param c Detected corners are inserted in to this container
	///@param N Threshold used to detect corners
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N)const;

	///Initialize a detector
	///@param fname File to load the detector from. This was created from \link learn_detector.cc learn_detector\endlink.
	faster_learn(const std::string& fname);

	private:
		///Loaded FAST-ER tree
		std::auto_ptr<tree_element> tree;
};

#endif
