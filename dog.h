#ifndef DOG_H
#define DOG_H

#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <utility>

#include "detectors.h"

struct dog: public DetectN
{
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const;
};

struct harrisdog: public DetectN
{
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const;
};

#endif
