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
