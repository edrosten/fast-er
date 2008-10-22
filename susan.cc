#include <cvd/image.h>
#include <cvd/convolution.h>
#include <cvd/vision.h>
#include <cvd/image_convert.h>

#include <vector>
#include <gvars3/instances.h>

#include "susan.h"

using namespace std;
using namespace CVD;
using namespace GVars3;

////////////////////////////////////////////////////////////////////////////////
//
// SUSAN interface
//



extern "C"
{
	void free_fux0red_memory();
	int*  susan(unsigned char* in, int x_size, int y_size, float dt, int bt);
}


void SUSAN::operator()(const Image<byte>& im, vector<ImageRef>& corners, unsigned int N) const
{
	float dt = GV3::get<float>("susan.dt", 4.0, 1);
	int* c = susan(const_cast<byte*>(im.data()), im.size().x, im.size().y, dt, N);

	int n = c[0];

	for(int i=0; i < n; i++)
		corners.push_back(ImageRef(c[2*i+2], c[2*i+3]));

	free_fux0red_memory();
}
