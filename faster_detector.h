#ifndef FASTER_LEARN_H
#define FASTER_LEARN_H

#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <string>
#include <memory>


#include "detectors.h"

struct tree_element;

struct faster_learn: public DetectT
{
	virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N)const;
	faster_learn(const std::string& fname);

	private:
		std::auto_ptr<tree_element> tree;
};

#endif
