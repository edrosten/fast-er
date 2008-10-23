#ifndef DOG_H
#define DOG_H

#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <utility>

#include "detectors.h"

///Class wrapping the Difference of Gaussians detector.
///@ingroup gDetect
struct dog: public DetectN
{
	///Detect corners
	///@param i Image in which to detect corners
	///@param c Detected corners are inserted in to this container
	///@param N Number of corners to detect
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const;
};

///Class wrapping the Harris-Laplace detector.
///@ingroup gDetect
struct harrisdog: public DetectN
{
	///Detecto corners
	///@param i Image in which to detect corners
	///@param c Detected corners are inserted in to this container
	///@param N Number of corners to detect
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const;
};

#endif
