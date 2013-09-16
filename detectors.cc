/*

    This file is part of the FAST-ER machine learning system.
    Copyright (C) 2008  Edward Rosten and Los Alamos National Laboratory

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "detectors.h"
#include "harrislike.h"
#include "dog.h"
#ifdef USESUSAN
	#include "susan.h"
#endif
#include "cvd_fast.h"
//#include "faster_block.h"
#include "faster_detector.h"

#include <memory>
#include <cstdlib>
#include <cvd/nonmax_suppression.h>
#include <gvars3/instances.h>

using namespace std;
using namespace CVD;
using namespace GVars3;

/** This takes a detector which requires a threshold and uses binary search to get as
close as possible to the requested number of corners.

@param i The image in which to detect corners.
@param c The detected corners to be returned.
@param N The target number of corners.
@param detector The corner detector.
@ingroup gDetect
*/
int binary_search_threshold(const Image<byte>& i, vector<ImageRef>& c, unsigned int N, const DetectT& detector)
{
	//Corners for high, low and midpoint thresholds.
	vector<ImageRef> ch, cl, cm;
	
	//The high and low thresholds.
	unsigned int t_high = 256;
	unsigned int t_low = 0;


	detector(i, ch, t_high);
	detector(i, cl, t_low);

	while(t_high > t_low + 1)
	{
	
		cm.clear();
		unsigned int t = (t_high + t_low	) / 2;
		detector(i, cm, t);

		if(cm.size() == N)
		{
			c = cm;
			return t;
		}
		else if(cm.size() < N) //If we detected too few points, then the t is too high
		{
			t_high = t;
			ch = cm;
		}
		else //We detected too many points to t is too low.
		{
			t_low = t;
			cl = cm;
		}
	}

	//Pick the closest
	//If there is ambiguity, go with the lower threshold (more corners).
	//The only reason for this is that the evaluation code in the FAST-ER
	//system uses this rule.
	if( N - ch.size() >= cl.size() - N)
	{
		c = cl;
		return t_low;
	}
	else
	{ 
		c = ch;
		return t_high;
	}
}

///This class wraps a ::DetectT class with ::binary_search_threshold and presents
///is as a DetectN class.
///@ingroup gDetect
struct SearchThreshold:public DetectN
{
	///@param d Detector to wrap. This will be managed by SearchThreshold
	SearchThreshold(DetectT* d)
	:detector(d)
	{
	}
    
	///Detect corners
	///@param im Image in which to detect corners
	///@param corners Detected corners are inserted in to this array
	///@param N number of corners to detect
	virtual void operator()(const Image<byte>& im, vector<ImageRef>& corners, unsigned int N)const
	{
		int t = binary_search_threshold(im, corners, N, *detector);	
	}
	
	private: 
	///Detector to wrap
	auto_ptr<DetectT> detector; 
};

///@ingroup gDetect
///Detector which randomly scatters corners around an image.
struct Random:public DetectN
{
	///Detect corners by scattering points around at random
	///@param im Image in which to detect corners
	///@param corners Detected corners are inserted in to this array
	///@param N number of corners to detect
	virtual void operator()(const Image<byte>& im, vector<ImageRef>& corners, unsigned int N)const
	{
		for(unsigned int i=0; i < N; i++)
			corners.push_back(ImageRef(rand() % im.size().x, rand() % im.size().y));
	}
};

typedef struct { int x, y; } xy; 
void twofast_detect(const SubImage<byte>& i, vector<ImageRef>& corners, int b);
void twofast_score(const SubImage<byte>& i, const vector<ImageRef>& corners, int b, vector<int>& scores);


struct twofast: public DetectT
{
	 virtual void operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int n) const
	 {
        vector<ImageRef> cs;
        twofast_detect(i, cs, n);
        vector<int> sc;
        twofast_score(i, cs, n, sc);
        nonmax_suppression(cs, sc, c);

	}
};

///Very simple factory function for getting detector objects.
///Paramaters (including the detector type) are drawn from 
///the GVars database. The parameter "detector" determines the
///detector type. Valid options are:
///  - \link ::Random random \endlink Randomly scatter corners around the image.
///  - \link ::dog dog \endlink Difference of Gaussians detector
///  - \link ::harrisdog harrisdog \endlink Harris-Laplace (actually implemented as Harris-DoG) detector
///  - \link ::HarrisDetect harris\endlink Harris detector with Gaussian blur
///  - \link ::ShiTomasiDetect shitomasi\endlink Shi-Tomasi detector
///  - \link ::SUSAN susan\endlink Reference implementation of the SUSAN detector
///  - \link ::fast_9 fast9\endlink libCVD's builtin FAST-9 detector
///  - \link ::fast_9_old fast9old\endlink libCVD's builtin FAST-9 detector with the old
///          scoring algorithm, as seen in [Rosten, Drummond 2006].
///  - \link ::fast_12 fast12\endlink libCVD's builtin FAST-12 detector
///  - \link ::faster_learn faster2\endlink A FAST-ER detector loaded from a file containing the tree
///@ingroup gDetect
auto_ptr<DetectN> get_detector()
{
	
	string d = GV3::get<string>("detector", "fast9", 1);
	
	if(d == "random")
		return auto_ptr<DetectN>(new Random);
	else if(d == "dog")
		return auto_ptr<DetectN>(new dog);
	else if(d == "harrisdog")
		return auto_ptr<DetectN>(new harrisdog);
	else if(d == "shitomasi")
		return auto_ptr<DetectN>(new ShiTomasiDetect);
	else if(d == "harris")
		return auto_ptr<DetectN>(new HarrisDetect);
	#ifdef USESUSAN
		else if(d == "susan")
			return auto_ptr<DetectN>(new SearchThreshold(new SUSAN));
	#endif
	else if(d == "fast9")
		return auto_ptr<DetectN>(new SearchThreshold(new fast_9));
	else if(d == "fast9old")
		return auto_ptr<DetectN>(new SearchThreshold(new fast_9_old));
	else if(d == "fast12")
		return auto_ptr<DetectN>(new SearchThreshold(new fast_12));
	else if(d == "faster2")
		return auto_ptr<DetectN>(new SearchThreshold(new faster_learn(GV3::get<string>("faster2"))));
	else if(d == "twofast")
		return auto_ptr<DetectN>(new SearchThreshold(new twofast));
	else
	{
		cerr << "Unknown detector: " << d << endl;
		exit(1);
	}
}
