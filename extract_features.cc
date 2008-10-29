/**
\file extract_features.cc Main file for the extract_features executable.

\section wpUsage Usage

<code> extract_features [--VAR VAL] [--exec FILE] IMAGE1 [IMAGE2 ...]</code>

\section Description

This program loads a learned FAST-ER tree and extracts features so that an 
accelerated tree can be learned. The output is is suitable for consumption 
by \link learn_fast_tree.cc learn_fast_tree\endlink.

The program accpets standard GVars3 commandline arguments, and the default
parameters are contained in \p extract_features.cfg :

\include extract_features.cfg

The images from which
features should be extracted are specified on the commandline. 

*/
#define CVD_IMAGE_DEBUG
#include <gvars3/instances.h>
#include <cvd/image_io.h>
#include <stdint.h>
#include <map>
#include <iterator>
#include <string>
#include "offsets.h"
#include "faster_tree.h"

using namespace std;
using namespace CVD;
using namespace GVars3;

static const char Brighter = 'b'; ///< Character code for pixels significantly brighter than the centre
static const char Darker = 'd';///< Character code for pixels significantly darker than the centre
static const char Similar = 's';///< Character code for pixels similar to the centre

///Extracts a feature from an image.
///@param s String to extract feature in to
///@param im Image to extract feature from
///@param pos Location to extract feature from
///@param t Threshold used to compute feature
///@param o Index in to offsets (i.e.\ feature orientation) to use
void extract_feature(string& s, const BasicImage<byte>& im, const ImageRef&  pos, int t, int o)
{
	int centre = im[pos];

	for(int i=0; i < num_offsets; i++)
	{
		if(im[pos + offsets[o][i]] > centre + t)
			s[i] = Brighter;
		else if(im[pos + offsets[o][i]] < centre - t)
			s[i] = Darker;
		else
			s[i] = Similar;
	}
}

///Driving program
///@param argc Number of commandline arguments
///@param argv List of commandline arguments. Contains GVars3 arguments, and images to process.
int main(int argc, char** argv)
{
	GUI.LoadFile("extract_features.cfg");
	int lastarg = GUI.parseArguments(argc, argv);

	create_offsets();

	map<string, uint64_t> corners;
	map<string, uint64_t> non_corners;

	string scratch(num_offsets, '.');

	int border = max(max(offsets_bbox.first.x, offsets_bbox.first.y), max(offsets_bbox.second.x, offsets_bbox.second.y));

	int threshold = GV3::get<int>("threshold", 30);
	int skip = GV3::get<int>("skip", 4);
	string fname=GV3::get<string>("detector", "best_faster.tree");

	tree_element* faster_detector;

	ifstream i;
	i.open(fname.c_str());

	if(!i.good())
	{
		cerr << "Error: " << fname << ": " << strerror(errno) << endl;
		exit(1);
	}
	
	try{
		faster_detector = load_a_tree(i);
	}
	catch(ParseError p)
	{
		cerr << "Parse error in " << fname << endl;
		exit(1);
	}


	//Iterate over all images, extracting features
	for(int i=lastarg; i < argc; i++)
	{
		try{
			Image<byte> im = img_load(argv[i]);

			//Detect all features
			vector<ImageRef> detected_corners = tree_detect_corners_all(im, faster_detector, threshold);

			//Extract features for all corners
			for(unsigned int j=0; j < detected_corners.size(); j++)
				for(unsigned int k=0; k < 1; k++)
				{
					extract_feature(scratch, im, detected_corners[j], threshold, k);
					corners[scratch]++;

					if(non_corners.count(scratch))
						cerr << "fuck!\n";
				}
			
			//Extract features for some non corners
			Image<bool> mask(im.size(), 0);
			for(unsigned int j=0; j < detected_corners.size(); j++)
				mask[detected_corners[j]] = 1;

			for(int r=border; r < im.size().y - border; r+=skip)
				for(int c=border; c < im.size().x - border; c+=skip)
					if(mask[r][c] == 0)
						for(unsigned int k=0; k < 1; k++)
						{
							extract_feature(scratch, im, ImageRef(c,r), threshold, k);
							non_corners[scratch]++;

							if(corners.count(scratch))
								cerr << "shit!\n";
						}
			
			cerr << "Processed " << argv[i] << endl;
		}
		catch(Exceptions::All e)
		{
			cerr << "Failed to load " << argv[i] << ": " << e.what << endl;
		}
	}

	cout << num_offsets << endl;
	copy(offsets[0].begin(), offsets[0].end(), ostream_iterator<ImageRef>(cout, " "));
	cout << endl;

	for(map<string, uint64_t>::iterator i=corners.begin(); i != corners.end(); i++)
		cout << i->first << " " << i->second << " 1" << endl;
	for(map<string, uint64_t>::iterator i=non_corners.begin(); i != non_corners.end(); i++)
		cout << i->first << " " << i->second << " 0" << endl;
}
