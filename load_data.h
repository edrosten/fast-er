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
#ifndef REPEATABILITY_H
#define REPEATABILITY_H

#include <vector>
#include <string>
#include <cvd/image.h>
#include <cvd/byte.h>
#include <tag/array.h>

std::pair<std::vector<CVD::Image<CVD::byte> >, std::vector<std::vector<CVD::Image<tag::array<float, 2> > > > > load_data(std::string dir, int num, std::string format);


void prune_warps(std::vector<std::vector<CVD::Image<tag::array<float, 2> > > >& warps, CVD::ImageRef size);

#endif
