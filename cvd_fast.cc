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
#include <cvd/fast_corner.h>
#include <cvd/nonmax_suppression.h>

#include <vector>

#include "cvd_fast.h"

using namespace std;
using namespace CVD;

////////////////////////////////////////////////////////////////////////////////
//
// FAST interface
//


void fast_9_old::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int n) const 
{
	vector<ImageRef> ct;
	vector<int> sc;
	fast_corner_detect_9(i, ct, static_cast<int>(n));
	fast_nonmax(i, ct, static_cast<int>(n), c);
}

void fast_9::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int n) const 
{
	fast_corner_detect_9_nonmax(i, c, static_cast<int>(n));
}

void fast_12::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int n) const 
{
	vector<ImageRef> cs;
	fast_corner_detect_12(i, cs, n);
	vector<int> sc;
	fast_corner_score_12(i, cs, n, sc);
	nonmax_suppression(cs, sc, c);
}
