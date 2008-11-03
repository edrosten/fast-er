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
#include <cvd/image.h>
#include <cvd/convolution.h>
#include <gvars3/instances.h>
#include <vector>

#include "harrislike.h"

using namespace std;
using namespace CVD;
using namespace GVars3;



////////////////////////////////////////////////////////////////////////////////
//
// Harris-like corners.
// 

///\cond never
template<class C> inline C sq(const C& c)
{
	return c*c;
}


struct HarrisScore
{
	static float Compute(float xx, float xy, float yy)
	{
		return (xx * yy - xy * xy) - 0.04 * sq(xx + yy);
	}
};

struct ShiTomasiScore
{
	static float Compute(float xx, float xy, float yy)
	{
		float l1 = xx + yy + sqrt(sq(xx - yy)+4.0*xy*xy);   
		float l2 = xx + yy - sqrt(sq(xx - yy)+4.0*xy*xy);
		return min(abs(l1), abs(l2));
	}
};

struct PosInserter
{
	static void insert(vector<ImageRef>& i, const pair<float, ImageRef>& p)
	{
		i.push_back(p.second);
	}
};

struct PairInserter
{
	static void insert(vector<pair<float, ImageRef> >& i, const pair<float, ImageRef>& p)
	{
		i.push_back(p);
	}
};
///\endcond

template<class Score, class Inserter, class C, class B> void harris_like(const Image<B>& i, C& c, unsigned int N, float blur, float sigmas)
{

	Image<float> xx(i.size()), xy(i.size()), yy(i.size());

	zeroBorders(xx);
	zeroBorders(xy);
	zeroBorders(yy);

	typedef typename Pixel::traits<B>::wider_type gType;
	
	//Compute gradients
	for(int y=1; y < i.size().y - 1; y++)
		for(int x=1; x < i.size().x - 1; x++)
		{
			gType gx = (gType)i[y][x-1] - i[y][x+1];
			gType gy = (gType)i[y-1][x] - i[y+1][x];
			
			//Compute the gradient moments
			xx[y][x] = gx * gx;
			xy[y][x] = gx * gy;
			yy[y][x] = gy * gy;
		}

	convolveGaussian_fir(xx, xx, blur, sigmas);
	convolveGaussian_fir(xy, xy, blur, sigmas);
	convolveGaussian_fir(yy, yy, blur, sigmas);
	
	//Avoid computing the score along the image borders where the
	//result of the convolution is not valid.
	int	kspread = (int)ceil(sigmas * blur);

	//Compute harris score
	for(int y=kspread; y < i.size().y-kspread; y++)
		for(int x=kspread; x <i.size().x-kspread; x++)
			xx[y][x] = Score::Compute(xx[y][x], xy[y][x], yy[y][x]);

	vector<pair<float, ImageRef> > corners;
	corners.reserve(10000);

	//Find local maxima
	for(int y=kspread; y < i.size().y-kspread; y++)
		for(int x=kspread; x <i.size().x-kspread; x++)
		{
			float c = xx[y][x];

			if( c > xx[y-1][x-1]  &&
				c > xx[y-1][x+0]  &&
				c > xx[y-1][x+1]  &&
				c > xx[y-0][x-1]  &&
				c > xx[y-0][x+1]  &&
				c > xx[y+1][x-1]  &&
				c > xx[y+1][x+0]  &&
				c > xx[y+1][x+1])
			{
				corners.push_back(make_pair(-c, ImageRef(x, y)));
			}
		}

	if(corners.size() > N)
	{
		nth_element(corners.begin(), corners.begin() + N, corners.end());
		corners.resize(N);
	}


	for(unsigned int i=0; i < corners.size(); i++)
		Inserter::insert(c, corners[i]);
}


void HarrisDetector(const Image<float>& i, vector<pair<float, ImageRef> >& c, unsigned int N, float blur, float sigmas)
{
	harris_like<HarrisScore, PairInserter>(i, c, N, blur, sigmas);
}


void HarrisDetect::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const 
{
	float blur = GV3::get<float>("harris.blur", 2.5, 1);
	float sigmas = GV3::get<float>("harris.sigmas", 2.0, 1);
	harris_like<HarrisScore,PosInserter>(i, c, N, blur, sigmas);
}

void ShiTomasiDetect::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const
{
	float blur = GV3::get<float>("shitomasi.blur", 2.5, 1);
	float sigmas = GV3::get<float>("shitomasi.sigmas", 2.0, 1);
	harris_like<ShiTomasiScore, PosInserter>(i, c, N, blur, sigmas);
}
