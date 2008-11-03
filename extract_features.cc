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

static const char BrighterFlag = 'b'; ///< Character code for pixels significantly brighter than the centre
static const char DarkerFlag = 'd';///< Character code for pixels significantly darker than the centre
static const char SimilarFlag = 's';///< Character code for pixels similar to the centre

///Extracts a feature from an image.
///@param s String to extract feature in to
///@param im Image to extract feature from
///@param pos Location to extract feature from
///@param barrier Threshold used to compute feature
///@param o Index in to offsets (i.e.\ feature orientation) to use
///@param invert_sense Whether or not to invert the extracted feature
void extract_feature(string& s, const BasicImage<byte>& im, const ImageRef&  pos, int barrier, int o, bool invert_sense)
{
	int cb = im[pos] + barrier;
	int c_b = im[pos] - barrier;

	for(int i=0; i < num_offsets; i++)
	{
		int pix = im[pos + offsets[o][i]];
	
		if(pix > cb)
			if(invert_sense == false)
				s[i] = BrighterFlag;
			else
				s[i] = DarkerFlag;
		else if(pix < c_b)
			if(invert_sense == false)
				s[i] = DarkerFlag;
			else
				s[i] = BrighterFlag;
		else
			s[i] = SimilarFlag;
	}
}

///Driving program
///@param argc Number of commandline arguments
///@param argv List of commandline arguments. Contains GVars3 arguments, and images to process.
int main(int argc, char** argv)
{
	//The usual initialization.
	GUI.LoadFile("extract_features.cfg");
	int lastarg = GUI.parseArguments(argc, argv);

	create_offsets();
	
	//Store corners and noncorners by the string representing the feature.
	map<string, uint64_t> corners, non_corners;
	
	//Scratch string of the correct length for extracting features in to.
	string scratch(num_offsets, '.');
    
	//Don't bother examining points outside this border.
	int border = max(max(offsets_bbox.first.x, offsets_bbox.first.y), max(offsets_bbox.second.x, offsets_bbox.second.y));

	int threshold = GV3::get<int>("threshold", 30);
	string fname=GV3::get<string>("detector", "best_faster.tree");
	
	//Load a detector from a tree file
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
			for(int r=border; r < im.size().y - border; r++)
				for(int c=border; c < im.size().x - border; c++)
				{
					ImageRef pos(c,r);
					//Test for cornerness
					bool is_corner = faster_detector->detect_corner(im, pos, threshold);

					//Iterate over all feature orientations and inversions,
					//extracting the reatures, and inserting them in to the
					//correct bin
					for(unsigned int k=0; k < offsets.size(); k++)
						for(int l=0; l < 2; l++)
						{
							extract_feature(scratch, im, pos, threshold, k, l);

							if(is_corner)
							{
								corners[scratch]++;

								if(non_corners.count(scratch))
								{
									cerr << "Fatal error! extracted corner has an identical non-corner!\n";
									cerr << "Are your offsets correct?\n";
									exit(1);
								}
							}
							else
							{
								non_corners[scratch]++;
								if(corners.count(scratch))
								{
									cerr << "Fatal error! extracted non-corner has an identical corner!\n";
									cerr << "Are your offsets correct?\n";
									exit(1);
								}
							}
						}
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
