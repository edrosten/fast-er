#ifndef HARRISLIKE_H
#define HARRISLIKE_H

#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <utility>

#include "detectors.h"

struct ShiTomasiDetect: public DetectN
{
	void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const;
};

struct HarrisDetect: public DetectN
{
	void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N)const;
};

void HarrisDetector(const CVD::Image<float>& i, std::vector<std::pair<float, CVD::ImageRef> >& c, unsigned int N, float blur, float sigmas);
#endif
