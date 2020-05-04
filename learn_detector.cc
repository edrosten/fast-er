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
@file learn_detector.cc Main file for the \c learn_detector executable.

\code
learn_detector [--var value] ... [--exec config_file] ...
\endcode
\c learn_detector reads configuration data from \c learn_detector.cfg in the
current directory and acceps standard GVars3 command line arguments for setting
variables and running other configuration files.

The tree is serialized by the function  ::tree_element::print().  This tree can
be extracted from the output with the following command:

<code>awk 'a&&!NF{exit}a;/Final tree/{a=1}' </code><i>filename</i>

This file contains a direct implementation of section V of the accompanying
paper, in the function ::learn_detector. For more information, refer to the
section on \link gOptimize optimization\endlink.


\section ldConf Configuration.

The default parameters for \c learn_detector are in \c learn_detector.cfg, which
are the parameters described in to paper.  They are: 

\include learn_detector.cfg


Variables can be overridden using the \c --varname \c value commandline syntax.
For details on how the data loading and so on operated, refer to
::run_learn_detector.


*/

#include <iostream>
#include <fstream>
#include <climits>
#include <float.h>
#include <cstring>
#include <cerrno>
#include <cmath>
#include <vector>
#include <utility>
#include <algorithm>
#include <array>
#include <random>

#include <cvd/image_io.h>
#include <cvd/vector_image_ref.h>

#include <TooN/TooN.h>

#include "gvars_vector.h"
#include "faster_tree.h"
#include "faster_bytecode.h"
#include "offsets.h"
#include "utility.h"
#include "load_data.h"
#include "varprintf/varprintf.h"

///\cond never
using namespace std;
using namespace CVD;
using namespace varPrintf;
using namespace GVars3;
using namespace TooN;
///\endcond
////////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//

///Square a number
///@param d Number to square
///@return  $d^2$
///@ingroup gUtility
double sq(double d)
{
	return d*d;
}


double rand_u(){
	static std::mt19937 eng;
	static std::uniform_real_distribution<> u(0,1);
	return u(eng);
}

///Populate a std::vector with the numbers 0,1,...,num
///@param num Size if the range
///@return the populated vector.
///@ingroup gUtility
vector<int> range(int num)
{
	vector<int> r;

	for(int i=0; i < num; i++)
		r.push_back(i);
	return r;
}



////////////////////////////////////////////////////////////////////////////////
//
//  Functions related to repeatability 
//


///Generate a disc of ImageRefs.
///@param radius Radius of the disc
///@return the disc of ImageRefs
///@ingroup gRepeatability
vector<ImageRef> generate_disc(int radius)
{
	vector<ImageRef> ret;
	ImageRef p;

	for(p.y = -radius; p.y <= radius; p.y++)
		for(p.x = -radius; p.x <= radius; p.x++)
			if((int)p.mag_squared() <= radius)
				ret.push_back(p);
	return ret;
}


///Paint shapes (a vector<ImageRef>) safely in to an image
///This is used to paint discs at corner locations in order to 
///perform rapid proximity checking.
///
/// @param corners  Locations to paint shapes
/// @param circle   Shape to paint
/// @param size     Image size to be painted in to
/// @return			Image with shapes painted in to it.
///@ingroup gRepeatability
Image<bool> paint_circles(const vector<ImageRef>& corners, const vector<ImageRef>& circle, ImageRef size)
{	
	Image<bool> im(size, 0);


	for(unsigned int i=0; i < corners.size(); i++)
		for(unsigned int j=0; j < circle.size(); j++)
			if(im.in_image(corners[i] + circle[j]))
				im[corners[i] + circle[j]] = 1;

	return im;
}

///Computes repeatability the quick way, by caching, but has small rounding errors. This 
///function paints a disc of <code>true</code> around each detected corner in to an image. 
///If a corner warps to a pixel which has the value <code>true</code> then it is a repeat.
///
/// @param warps    Every warping where warps[i][j] specifies warp from image i to image j.
/// @param corners  Detected corners
/// @param r		A corner must be as close as this to be considered repeated
/// @param size		Size of the region for cacheing. All images must be this size.
/// @return 		The repeatability.
/// @ingroup gRepeatability
float compute_repeatability(const vector<vector<Image<array<float, 2> > > >& warps, const vector<vector<ImageRef> >& corners, int r, ImageRef size)
{
	unsigned int n = corners.size();

	vector<ImageRef> disc = generate_disc(r);

	vector<Image<bool> >  detected;
	for(unsigned int i=0; i < n; i++)
		detected.push_back(paint_circles(corners[i], disc, size));
	
	int corners_tested = 0;
	int good_corners = 0;

	for(unsigned int i=0; i < n; i++)
		for(unsigned int j=0; j < n; j++)
		{
			if(i==j)
				continue;
			
			for(unsigned int k=0; k < corners[i].size(); k++)
			{	
				ImageRef dest = ir_rounded(warps[i][j][corners[i][k]]);

				if(dest.x != -1)
				{
					corners_tested++;
					if(detected[j][dest])
						good_corners++;
				}
			}
		}
	
	return 1.0 * good_corners / (DBL_EPSILON + corners_tested);
}


/// Generate a random tree, as part of a stochastic optimization scheme.
///
/// @param d Depth of tree to generate
/// @param is_eq_branch Whether eq-branch constraints should be applied. This should
///                     always be true when the function is called.
/// @ingroup gOptimize
tree_element* random_tree(int d, bool is_eq_branch=1)
{
	//Recursively generate a tree of depth d
	//
	//Generated trees respect invariant 1
	if(d== 0)
		if(is_eq_branch)
			return new tree_element(0);
		else
			return new tree_element(rand()%2);
	else
		return new tree_element(random_tree(d-1, 0), random_tree(d-1, 1), random_tree(d-1, 0), rand()%num_offsets );
}



///Compute the current temperature from parameters in the 
///configuration file.
///
///@ingroup gOptimize
///@param i The current iteration.
///@param imax The maximum number of iterations.
///@return The temperature.
double compute_temperature(int i, int imax)
{
	double scale=GV3::get<double>("Temperature.expo.scale");
	double alpha = GV3::get<double>("Temperature.expo.alpha");

	return scale * exp(-alpha * i / imax);
}




///Generate an optimized corner detector.
///
///@ingroup gOptimize
///@param images The training images
///@param warps  Warps for evaluating the performance on the training images.
///@return An optimized detector.
tree_element* learn_detector(const vector<Image<CVD::byte> >& images, const vector<vector<Image<array<float,2> > > >& warps)
{
	unsigned int  iterations=GV3::get<unsigned int>("iterations");       // Number of iterations of simulated annealing.
	int threshold = GV3::get<int>("FAST_threshold");                     // Threshold at which to perform detection
	int fuzz_radius=GV3::get<int>("fuzz");                               // A point must be this close to be repeated (\varepsilon)
	double repeatability_scale = GV3::get<double>("repeatability_scale");// w_r 
	double num_cost	=	GV3::get<double>("num_cost");                    // w_n
	int max_nodes = GV3::get<int>("max_nodes");                          // w_s

	bool first_time = 1;
	double old_cost = HUGE_VAL;                                          //This will store the final score on the previous iteration: \hat{k}_{I-1}

	ImageRef image_size = images[0].size();

	set<int> debug_triggers = GV3::get<set<int> >("triggers");           //Allow artitrary GVars code to be executed at a given iteration.
	
	//Preallocated space for nonmax-suppression. See tree_detect_corners()
	Image<int> scratch_scores(image_size, 0);

	//Start with an initial random tree
	tree_element* tree = random_tree(GV3::get<int>("initial_tree_depth"));
	
	for(unsigned int itnum=0; itnum < iterations; itnum++)
	{
		if(debug_triggers.count(itnum))
			GUI.ParseLine(GV3::get<string>(sPrintf("trigger.%i", itnum)));

		/* Trees:

			Invariants:
				1:     eq->{0,0,0,(0,0),0}			//Leafs of an eq pointer must not be corners

			Operations:
				Leaves:
					1: Splat on a random subtree of depth 1 (respect invariant 1)
					2: Flip  class   (respect invariant 1)

				Nodes:
					3: Copy one subtree to another subtree (no invariants need be respected)
					4: Randomize offset (no invariants need be respected)
					5: Splat a subtree in to a single node.


		Cost:
		
		  (1 + (#nodes/max_nodes)^2) * (1 - repeatability)^2 * Sum_{frames} exp(- (fast_9_num-detected_num)^2/2sigma^2)
		 
		*/


		//Deep copy in to new_tree and work with the copy.
		tree_element* new_tree = tree->copy();

		cout << "\n\n-------------------------------------\n";
		cout << "Iteration " << itnum << endl;

		if(GV3::get<bool>("debug.print_old_tree"))
		{
			cout << "Old tree is:" << endl;
			tree->print(cout);
		}
	
		//Skip tree modification first time so that the randomly generated
		//initial tree can be evaluated
		if(!first_time)
		{

			//Create a tree permutation
			tree_element* node;
			bool node_is_eq;
			

			//Select a random node
			int nnum = rand() % new_tree->num_nodes();
			tie(node, node_is_eq) = new_tree->nth_element(nnum);

			cout << "Permuting tree at node " << nnum << endl;
			cout << "Node " << node << " " << node_is_eq << endl;
			

			//See section 4 in the paper.
			if(node->eq == NULL) //A leaf
			{
				if(rand() % 2 || node_is_eq)  //Operation 1, invariant 1
				{
					cout << "Growing a subtree:\n";
					//Grow a subtree
					tree_element* stub = random_tree(1);

					stub->print(cout);

					//Splice it on manually (ick)
					*node = *stub;
					stub->lt = stub->eq = stub->gt = 0;
					delete stub;

				}
				else //Operation 2
				{
					cout << "Flipping the classification\n";
					node->is_corner  = ! node->is_corner;
				}
			}
			else //A node
			{
				double d = rand_u();

				if(d < 1./3.) //Randomize the test
				{
					cout << "Randomizing the test\n";
					node->offset_index = rand() % num_offsets;
				}
				else if(d < 2./3.)
				{
					//Select r, c \in {0, 1, 2} without replacement
					int r = rand() % 3; //Remove
					int c;				//Copy
					while((c = rand()%3) == r){}

					cout << "Copying branches " << c << " to " << r <<endl;

					//Deep copy node c: it's a tree, not a graph.
					tree_element* tmp;

					if(c == 0)
						tmp = node->lt->copy();
					else if(c == 1)
						tmp = node->eq->copy();
					else
						tmp = node->gt->copy();
					
					//Delete r and put the copy of c in its place
					if(r == 0)
					{
						delete node->lt;
						node->lt = tmp;
					}
					else if(r == 1)
					{
						delete node->eq;
						node->eq = tmp;
					}
					else 
					{
						delete node->gt;
						node->gt = tmp;
					}

					//NB BUG!!!
					//At this point the invariant can be broken,
					//since a "corner" leaf could have been copied
					//to an "eq" branch.

					//Oh dear. This bug made it in to the paper.
					//Fortunately, the bytecode compiler ignores the tree
					//when it can decuce its structure from the invariant.

					//The following line should have been present in the paper:
					if(node->eq->is_leaf())
					    node->eq->is_corner = 0;

					//Happily, because the bytecode compiler deduces this
					//it behaves as if this line was present, at evaluation time.
					//Of course, the presense of this line will produce different
					//results later if the node is subsequently copied back in one
					//of these operations.
				}
				else //Splat!!! ie delete a subtree
				{
					cout << "Splat!!!1\n";
					delete node->lt;
					delete node->eq;
					delete node->gt;
					node->lt = node->eq = node->gt = 0;

					if(node_is_eq) //Maintain invariant 1
						node->is_corner = 0;
					else
						node->is_corner = rand()%2;
				}
			}
		}
		first_time=0;

		if(GV3::get<bool>("debug.print_new_tree"))
		{
			cout << "New tree is: "<< endl;
			new_tree->print(cout);
		}
	
		
		//Detect all corners in all images
		vector<vector<ImageRef> > detected_corners;
		for(unsigned int i=0; i < images.size(); i++)
			detected_corners.push_back(tree_detect_corners(images[i], new_tree, threshold, scratch_scores));


		//Compute repeatability and assosciated cost
		double repeatability = compute_repeatability(warps, detected_corners, fuzz_radius, image_size);
		double repeatability_cost = 1 + sq(repeatability_scale/repeatability);

		//Compute cost associated with the total number of detected corners.
		float number_cost=0;
		for(unsigned int i=0; i < detected_corners.size(); i++)
		{
			double cost = sq(detected_corners[i].size() / num_cost);
			cout << "Image " << i << " " << detected_corners[i].size()<< " " << cost << endl;
			number_cost += cost;
		}
		number_cost = 1 + number_cost / detected_corners.size();
		cout << "Number cost " << number_cost << endl;

		//Cost associated with tree size
		double size_cost = 1 + sq(1.0 * new_tree->num_nodes()/max_nodes);
		
		//The overall cost function
		double cost = size_cost * repeatability_cost * number_cost;

		double temperature = compute_temperature(itnum,iterations);
		

		//The Boltzmann acceptance criterion:
		//If cost < old cost, then old_cost - cost > 0
		//so exp(.) > 1
		//so drand48() < exp(.) == 1
		double liklihood=exp((old_cost-cost) / temperature);

		
		cout << "Temperature" << temperature << endl;
		cout << "Number cost" << number_cost << endl;
		cout << "Repeatability" << repeatability << " " << repeatability_cost << endl;
		cout << "Nodes" << new_tree->num_nodes() << " " << size_cost << endl;
		cout << "Cost" << cost << endl;
		cout << "Old cost" << old_cost << endl;
		cout << "Liklihood" << liklihood << endl;
		
		//Make the Boltzmann decision
		if(rand_u() < liklihood)
		{
			cout << "Keeping change" << endl;
			old_cost = cost;
			delete tree;
			tree = new_tree;
		}
		else
		{
			cout << "Rejecting change" << endl;
			delete new_tree;
		}
		
		cout << "Final cost " << old_cost << endl;

	}


	return tree;
}


///Load configuration and data and learn a detector.
///
///@param argc Number of command line arguments
///@param argv Vector of command line arguments
///@ingroup gOptimize
void run_learn_detector(int argc, char** argv)
{

	//Process configuration information
	GUI.LoadFile("learn_detector.cfg");
	GUI.parseArguments(argc, argv);

	
	//Load a ransom seed.
	if(GV3::get<int>("random_seed") != -1)
		srand(GV3::get<int>("random_seed"));

	//Initialize the global information for the tree	
	create_offsets();
	draw_offsets();

	
	//Load the training set
	string dir=GV3::get<string>("repeatability_dataset.directory");
	string format=GV3::get<string>("repeatability_dataset.format");
	int num=GV3::get<int>("repeatability_dataset.size");

	vector<Image<CVD::byte> > images;
	vector<vector<Image<array<float, 2> > > > warps;
	
	tie(images, warps) = load_data(dir, num, format);

	prune_warps(warps, images[0].size());


	//Learn a detector
	tree_element* tree = learn_detector(images, warps);

	//Print out the results
	cout << "Final tree is:" << endl;
	tree->print(cout);
	cout << endl;

	cout << "Final block detector is:" << endl;
	{
		block_bytecode f = tree->make_fast_detector(9999);
		f.print(cout, 9999);
	}
}

///Driver wrapper.
///
///@param argc Number of command line arguments
///@param argv Vector of command line arguments
///@ingroup gOptimize
int main(int argc, char** argv)
{
	try
	{	
		run_learn_detector(argc, argv);
	}
	catch(const Exceptions::All& w)
	{	
		cerr << "Error: " << w.what() << endl;
	}
}







