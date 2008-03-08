#ifndef INC_UTILITY_H
#define INC_UTILITY_H

#include <cvd/image_ref.h>
#include <tag/array.h>

/// Convert a float array into an image co-ordinate. Numbers are rounded
/// @param v The array to convert
/// @ingroup gUtility
inline CVD::ImageRef ir_rounded(const tag::array<float, 2>& v)
{
	return CVD::ImageRef(
  static_cast<int>(v[0] > 0.0 ? v[0] + 0.5 : v[0] - 0.5),
  static_cast<int>(v[1] > 0.0 ? v[1] + 0.5 : v[1] - 0.5));
}

#endif
