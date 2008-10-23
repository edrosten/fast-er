#ifndef FASTER_LEARN_H
#define FASTER_LEARN_H

#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <string>
#include <memory>


#include "detectors.h"

struct tree_element;

///FAST-ER detector
///@ingroup gDetect
struct faster_learn: public DetectT
{
	///Detect corners
	///@param i Image in which to detect corners
	///@param c Detected corners are inserted in to this container
	///@param N Threshold used to detect corners
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N)const;

	///Initialize a detector
	///@param fname File to load the detector from. This was created from \link learn_detector.cc learn_detector\endlink.
	faster_learn(const std::string& fname);

	private:
		///Loaded FAST-ER tree
		std::auto_ptr<tree_element> tree;
};

#endif
