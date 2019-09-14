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
/**
\file extract_FAST_features.cc Main file for the extract_features executable.

\section wpUsage Usage

<code> extract_features [--VAR VAL] [--exec FILE] IMAGE1 [IMAGE2 ...]</code>

\section Description

This program extracts FAST-N features from video or files.  The output is is suitable for consumption by \link learn_fast_tree.cc learn_fast_tree\endlink.

The program accpets standard GVars3 commandline arguments, and the default
parameters are contained in \p extract_features.cfg :

\include extract_features.cfg

The images from which
features should be extracted are specified on the commandline. 

*/

#include <gvars3/instances.h>
#include <cvd/image_io.h>
#include <cvd/videosource.h>
#include <stdint.h>
#include <map>
#include <iterator>
#include <string>
#include <string_view>
#include <array>
#include <algorithm>


using namespace CVD;
using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using namespace GVars3;

ImageRef dir[16]=
{
	ImageRef(0,3),
	ImageRef(1,3),
	ImageRef(2,2),
	ImageRef(3,1),
	ImageRef(3,0),
	ImageRef(3,-1),
	ImageRef(2,-2),
	ImageRef(1,-3),
	ImageRef(0,-3),
	ImageRef(-1,-3),
	ImageRef(-2,-2),
	ImageRef(-3,-1),
	ImageRef(-3,0),
	ImageRef(-3,1),
	ImageRef(-2,2),
	ImageRef(-1,3),
};

using Feature = std::array<char,16>;
bool operator<(const Feature& f1, const Feature& f2){
	return std::lexicographical_compare(f1.begin(), f1.end(), f2.begin(), f2.end());
}

namespace std {
	template <> struct hash<Feature> {
		std::size_t operator()(const Feature& t) const {
			return std::hash<std::string_view>()({t.data(), t.size()});
		}
	};
}


Feature extract(const BasicImage<byte>& imp, ImageRef pos, int threshold)
{
	int cb = imp[pos] + threshold;
	int c_b = imp[pos] - threshold;
	
	Feature res;
	for(int i=0; i < 16; i++)
		if(imp[pos + dir[i]] > cb)
			res[i] = 'b';
		else if(imp[pos + dir[i]] < c_b)
			res[i] = 'd';
		else
			res[i] = 's';
	return res;
}

void extract_features(const SubImage<byte>& im, int t, std::unordered_map<Feature, uint64_t>& features){
	for(int y=3; y < im.size().y-3; y++)
		for(int x=3; x < im.size().x-3; x++)
			features[extract(im, ImageRef(x, y), t)]++;
}



///Driving program
///@param argc Number of commandline arguments
///@param argv List of commandline arguments. Contains GVars3 arguments, and images to process.
int main(int argc, char** argv)
{
	int lastarg = GUI.parseArguments(argc, argv);
	
	int t = GV3::get<int>("threshold", 10);
	bool augment = GV3::get<int>("augment", true);
	int frames = GV3::get<int>("frames", 1000);
	
	std::unordered_map<Feature, uint64_t> features;
	
	for(int arg=lastarg; arg < argc; arg++){
		
		cerr << "Processing " << argv[arg] << " ";

		try{
			Image<byte> im = img_load(argv[arg]);			
			extract_features(im, t, features);
			cerr << "image ";
		}
		catch(CVD::Exceptions::All&){
			try{
				VideoBuffer<byte>* vbuf = open_video_source<byte>(argv[arg]);

				cerr << "video ";
				for(int nf = 0; nf < frames; nf++)
				{
					VideoFrame<byte>* vf = vbuf->get_frame();
					extract_features(*vf, t, features);
					vbuf->put_frame(vf);
					cerr << nf << " ";
				}
			}
			catch(CVD::Exceptions::All&){
				cerr << "bad ";	
			}
		}
		cerr << features.size() << endl;
	}
	

	if(augment){
		char types[]="bsd.";
		Feature F;
		int a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p;
		for(a = 0, F[ 0]='b'; a < 3; a++, F[ 0]=types[a])
		 for(b = 0, F[ 1]='b'; b < 3; b++, F[ 1]=types[b])
		  for(c = 0, F[ 2]='b'; c < 3; c++, F[ 2]=types[c])
		   for(d = 0, F[ 3]='b'; d < 3; d++, F[ 3]=types[d])
			for(e = 0, F[ 4]='b'; e < 3; e++, F[ 4]=types[e])
			 for(f = 0, F[ 5]='b'; f < 3; f++, F[ 5]=types[f])
			  for(g = 0, F[ 6]='b'; g < 3; g++, F[ 6]=types[g])
			   for(h = 0, F[ 7]='b'; h < 3; h++, F[ 7]=types[h])
				for(i = 0, F[ 8]='b'; i < 3; i++, F[ 8]=types[i])
				 for(j = 0, F[ 9]='b'; j < 3; j++, F[ 9]=types[j])
				  for(k = 0, F[10]='b'; k < 3; k++, F[10]=types[k])
				   for(l = 0, F[11]='b'; l < 3; l++, F[11]=types[l])
					for(m = 0, F[12]='b'; m < 3; m++, F[12]=types[m])
					 for(n = 0, F[13]='b'; n < 3; n++, F[13]=types[n])
					  for(o = 0, F[14]='b'; o < 3; o++, F[14]=types[o])
					   for(p = 0, F[15]='b'; p < 3; p++, F[15]=types[p])
							features[F]++;
	}

	cout << 16 << endl;
	for(int i=0; i < 16; i++)
		cout << dir[i] << " ";
	cout << endl;

	for(const auto& f:features)
		cout << string(f.first.data(), f.first.size())  << " " << f.second << endl;

}
