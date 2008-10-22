#include <fstream>
#include <cmath>
#include <cstring>
#include <cerrno>
#include <vector>
#include <utility>
#include <algorithm>

#include <cvd/image.h>
#include <cvd/byte.h>
#include <cvd/random.h>
#include <cvd/vector_image_ref.h>

#include <tag/stdpp.h>
#include <tag/fn.h>

#include <sys/mman.h>

#include <TooN/TooN.h>
#include <TooN/helpers.h>

#include "offsets.h"
#include "faster_detector.h"
#include "faster_tree.h"
#include "faster_bytecode.h"

#include <gvars3/instances.h>


using namespace std;
using namespace CVD;
using namespace tag;
using namespace GVars3;
using namespace TooN;


//Generate a detector, and compute its repeatability for all the tests.
//
//@param argc Number of command line arguments
//@ingroup gRepeatability
void init()
{
	static bool once=0;

	if(!once)
	{
		create_offsets();
		once = 1;
	}
}


faster_learn::faster_learn(const std::string& fname)
{
	init();
	ifstream i;
	i.open(fname.c_str());

	if(!i.good())
	{
		cerr << "Error: " << fname << ": " << strerror(errno) << endl;
		exit(1);
	}
	
	try{
		tree.reset(load_a_tree(i));
	}
	catch(ParseError p)
	{
		cerr << "Parse error in " << fname << endl;
		exit(1);
	}

	if(GV3::get<bool>("faster_tree.print_tree", 0, 1))
	{
		clog << "Tree:" << endl;
		tree->print(clog);
	}

	if(GV3::get<bool>("faster_tree.print_block", 0, 1))
	{
		block_bytecode f2 = tree->make_fast_detector(100);
		f2.print(clog,  100);
	}
}


void faster_learn::operator()(const Image<byte>& i, vector<ImageRef>& v, unsigned int t) const
{
	Image<int> scratch(i.size(), 0);

	v = tree_detect_corners(i, tree.get(), t, scratch);
}
