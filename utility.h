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
