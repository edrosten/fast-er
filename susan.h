#ifndef SUSAN_H
#define SUSAN_H

#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <utility>

#include "detectors.h"

struct SUSAN: public DetectT
{
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N)const;
};

#endif
