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
#ifndef HARRISLIKE_H
#define HARRISLIKE_H

#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <utility>

#include "detectors.h"
///Class wrapping the Harris detector.
///@ingroup gDetect
struct ShiTomasiDetect: public DetectN
{
	///Detect corners
	///@param i Image in which to detect corners
	///@param c Detected corners are inserted in to this container
	///@param N Number of corners to detect

	void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const;
};

///Class wrapping the Shi-Tomasi detector.
///@ingroup gDetect
struct HarrisDetect: public DetectN
{
	///Detect corners
	///@param i Image in which to detect corners
	///@param c Detected corners are inserted in to this container
	///@param N Number of corners to detect

	void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N)const;
};

///Detect Harris corners
///@ingroup gDetect
///@param i Image in which to detect corners
///@param c Detected corners and strengths are inserted in to this container
///@param N Number of corners to detect
///@param blur Standard deviation of blur to use
///@param sigmas Blur using sigmas standard deviations
void HarrisDetector(const CVD::Image<float>& i, std::vector<std::pair<float, CVD::ImageRef> >& c, unsigned int N, float blur, float sigmas);

#endif
