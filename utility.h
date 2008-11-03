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
#ifndef INC_UTILITY_H
#define INC_UTILITY_H

#include <cvd/image_ref.h>
#include <tag/array.h>
#include <TooN/TooN.h>

/// Convert a float array into an image co-ordinate. Numbers are rounded
/// @param v The array to convert
/// @ingroup gUtility
inline CVD::ImageRef ir_rounded(const tag::array<float, 2>& v)
{
	return CVD::ImageRef(
  static_cast<int>(v[0] > 0.0 ? v[0] + 0.5 : v[0] - 0.5),
  static_cast<int>(v[1] > 0.0 ? v[1] + 0.5 : v[1] - 0.5));
}

///Convert an array<float, 2> in to a Vector<2>
///@param f The input array
///@return The output Vector<2>
inline TooN::Vector<2> Vec(const tag::array<float, 2>& f)
{
        return (TooN::make_Vector, f[0], f[1]);
}

#endif
