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
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>

#include <cvd/vector_image_ref.h>
#include <cvd/image.h>

#include <TooN/TooN.h>

#include "tag/fn.h"

#include <gvars3/instances.h>

#include "offsets.h"

///\cond never
using namespace std;
using namespace CVD;
using namespace TooN;
using namespace tag;
using namespace GVars3;
///\endcond




///Actual x,y offset of the offset numbers in the different available orientations.
///@ingroup gTree
vector<vector<ImageRef> > offsets;
///The number of possible offsets. Equivalent to <code>offsets[x].size()</code>
///@ingroup gTree
int num_offsets;
///Bounding box for offsets in all orientations. This is therefore a bounding box for the detector.
///@ingroup gTree
pair<ImageRef, ImageRef> offsets_bbox;




///Rotate a vector<ImageRef> by a given angle, with an optional reflection.
///@param offsets Offsets to rotate.
///@param angle Angle to rotate by.
///@param r Whether to reflect.
///@return The rotated offsets.
///@ingroup gTree
vector<ImageRef> transform_offsets(const vector<ImageRef>& offsets, int angle, bool r)
{
	double a = angle * M_PI / 2;	

	double R_[] = { cos(a), sin(a), -sin(a) , cos(a) };
	double F_[] = { 1, 0, 0, r?-1:1};

	Matrix<2,2,double,Reference::RowMajor> R(R_), F(F_);
	Matrix<2> T = R*F;

	vector<ImageRef> ret;

	for(unsigned int i=0; i < offsets.size(); i++)
	{
		Vector<2> v = vec(offsets[i]);
		ret.push_back(ir_rounded(T * v));
	}
	
	return ret;
}


///Pretty print some offsets to stdout.
///@param offsets List of offsets to pretty-print.
///@ingroup gUtility
void draw_offset_list(const vector<ImageRef>& offsets)
{

	cout << "Allowed offsets: " << offsets.size() << endl;

	ImageRef min, max;
	min.x = *min_element(member_iterator(offsets.begin(), &ImageRef::x), member_iterator(offsets.end(), &ImageRef::x));
	max.x = *max_element(member_iterator(offsets.begin(), &ImageRef::x), member_iterator(offsets.end(), &ImageRef::x));
	min.y = *min_element(member_iterator(offsets.begin(), &ImageRef::y), member_iterator(offsets.end(), &ImageRef::y));
	max.y = *max_element(member_iterator(offsets.begin(), &ImageRef::y), member_iterator(offsets.end(), &ImageRef::y));

	cout << min << " " <<max << endl;

	Image<int> o(max-min+ImageRef(1,1), -1);
	for(unsigned int i=0; i <offsets.size(); i++)
		o[offsets[i] -min] = i;

	for(int y=0; y < o.size().y; y++)
	{
		for(int x=0; x < o.size().x; x++)
			cout << "+------";
		cout << "+"<< endl;

		for(int x=0; x < o.size().x; x++)
			cout << "|      ";
		cout << "|"<< endl;


		for(int x=0; x < o.size().x; x++)
		{
			if(o[y][x] >= 0)
				cout << "|  " << setw(2) << o[y][x] << "  ";
			else if(ImageRef(x, y) == o.size() / 2)
				cout << "|   " << "#" << "  ";
			else 
				cout << "|      ";
		}
		cout <<  "|" << endl;

		for(int x=0; x < o.size().x; x++)
			cout << "|      ";
		cout << "|"<< endl;
	}

	for(int x=0; x < o.size().x; x++)
		cout << "+------";
	cout << "+"<< endl;

	cout << endl;

}

///Create a list of offsets with various transformation to map the offset number
/// (see Figure 7 in the accompanying paper) to a pixel coordinate, inclusing all 
/// combinations of rotation and reflection.
/// 
/// The function populates ::offsets, and must be called before anything uses
/// this variable.
///
/// All possible offsets are selected in an annulus, which uses the following gvars:
/// - \c offsets.min_radius Minimum distance from (0,0) for offset
/// - \c offsets.max_radius Maximum distance from (0,0) for offset
///
///
///@ingroup gTree
void create_offsets()
{
	//Pixel offsets are represented as integer indices in to an array of
	//ImageRefs. That means that by choosing the array, the tree can be
	//rotated and/or reflected. Here, an annulus of possible offsets is 
	//created and rotated by all multiples of 90 degrees, and then reflected.
	//This gives a total of 8.
	offsets.resize(8);
	{	
		double min_r = GV3::get<double>("offsets.min_radius");
		double max_r = GV3::get<double>("offsets.max_radius");

		ImageRef max((int)ceil(max_r+1), (int)ceil(max_r+1));
		ImageRef min = -max, p = min;

		//cout << "Offsets: ";

		do
		{
			double d = vec(p) * vec(p);

			if(d >= min_r*min_r && d <= max_r * max_r)
			{
				offsets[0].push_back(p);
				//cout << offsets[0].back() << " ";
			}
		}
		while(p.next(min, max));

	//	cout << endl;

		offsets_bbox = make_pair(min, max);
	}
	offsets[1] = transform_offsets(offsets[0], 1, 0);
	offsets[2] = transform_offsets(offsets[0], 2, 0);
	offsets[3] = transform_offsets(offsets[0], 3, 0);
	offsets[4] = transform_offsets(offsets[0], 0, 1);
	offsets[5] = transform_offsets(offsets[0], 1, 1);
	offsets[6] = transform_offsets(offsets[0], 2, 1);
	offsets[7] = transform_offsets(offsets[0], 3, 1);
	num_offsets=offsets[0].size();
}

///Prettyprints the contents of ::offsets
///@ingroup gUtility
void draw_offsets()
{
	//Print the offsets out.	
	for(unsigned int i=0; i < 8; i++)
	{
		cout << "Offsets " << i << endl;
		draw_offset_list(offsets[i]);	
		cout << endl;
	}
}
