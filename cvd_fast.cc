#include <cvd/image.h>
#include <cvd/fast_corner.h>
#include <cvd/nonmax_suppression.h>

#include <vector>

#include "cvd_fast.h"

using namespace std;
using namespace CVD;

////////////////////////////////////////////////////////////////////////////////
//
// FAST interface
//


void fast_9_old::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int n) const 
{
	vector<ImageRef> ct;
	vector<int> sc;
	fast_corner_detect_9(i, ct, static_cast<int>(n));
	fast_nonmax(i, ct, static_cast<int>(n), c);
}

void fast_9::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int n) const 
{
	fast_corner_detect_9_nonmax(i, c, static_cast<int>(n));
}

void fast_12::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int n) const 
{
	vector<ImageRef> cs;
	fast_corner_detect_12(i, cs, n);
	vector<int> sc;
	fast_corner_score_12(i, cs, n, sc);
	nonmax_suppression(cs, sc, c);
}
