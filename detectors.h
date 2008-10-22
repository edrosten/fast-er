#ifndef DETECTORS_H
#define DETECTORS_H

#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <memory>
#include <string>

///A corner detector object which is passed a target number of corners to detect.
struct DetectN
{
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N)const =0;
	virtual ~DetectN(){}
};

///A corner detector object which is passed a threshold. These can be wrapped with a 
///searching algorithm to turn them in to a ::DetectN
struct DetectT
{
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N)const =0;
	virtual ~DetectT(){}
};

std::auto_ptr<DetectN> get_detector();

#endif
