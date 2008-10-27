/**
\file test_repeatability.cc Main file for the test_repeatability executable.

\section wpUsage Usage

<code> test_repeatability [--var VAL] [--exec FILE] </code>

\section Description

This program \link gDataset loads a dataset\endlink and then 
computes repeatability of the specified detector on the dataset. This
program accepts standard GVars3 commandline arguments and loads 
<code>learn_detector.cfg</code> as a the default configuration:

\include test_repeatability.cfg

The available detectors are selected using the <code>detector</code> variable.
Options are given in ::get_detector.

*/


#include <iostream>
#include <sstream>
#include <cfloat>
#include <map>
#include <utility>

#include <cvd/image_io.h>
#include <cvd/random.h>
#include <cvd/image_interpolate.h>
#include <tag/printf.h>
#include <tag/stdpp.h>


#include "gvars_vector.h"
#include "load_data.h"
#include "detectors.h"
#include "utility.h"

using namespace std;
using namespace CVD;
using namespace tag;
using namespace GVars3;
using namespace TooN;


///Computes repeatability the slow way to avoid rounding errors, by comparing the warped
///corner position to every detected corner. A warp to x=-1, y=? is considered to be outside
///the image, so it is not counted.
///
/// @param warps    Every warping where warps[i][j] specifies warp from image i to image j.
/// @param corners  Detected corners
/// @param r		A corner must be as close as this to be considered repeated
/// @return 		The repeatability. No corners means zero repeatability.
/// @ingroup gRepeatability
double compute_repeatability_exact(const vector<vector<Image<array<float,2> > > >& warps, const vector<vector<ImageRef> >& corners, double r)
{
	unsigned int n = corners.size();

	int repeatable_corners = 0;
	int repeated_corners = 0;

	r *= r;

	for(unsigned int i=0; i < n; i++)
		for(unsigned int j=0; j < n; j++)
		{
			if(i==j)
				continue;

			for(unsigned int k=0; k < corners[i].size(); k++)
			{
				const array<float, 2>& p = warps[i][j][corners[i][k]];

				if(p[0] != -1) //pixel does not warp to inside image j
				{

					repeatable_corners++;

					for(unsigned int l=0; l < corners[j].size(); l++)
					{
						Vector<2> d = Vec(p) - vec(corners[j][l]);

						if(d*d < r)
						{
							repeated_corners++;
							break;
						}
					}
				}
			}
		}
	
	return 1.0 * (repeated_corners) / (repeatable_corners + DBL_EPSILON);
}

///This wrapper function computed the repeatability for a given detector and a given
///container of corner densities. The result is printed to stdout.
///
/// @param images   Images to test repeatability on
/// @param warps    Every warping where warps[i][j] specifies warp from image i to image j.
/// @param detector Pointer to the corner detection function.
/// @param cpf      The number of corners per frame to be tested.
/// @param fuzz		A corner must be as close as this to be considered repeated
/// @ingroup gRepeatability
void compute_repeatability_all(const vector<Image<byte> >& images, const vector<vector<Image<array<float, 2> > > >& warps, const DetectN& detector, const vector<int>& cpf, double fuzz)
{
	
	for(unsigned int i=0; i < cpf.size(); i++)
	{
		//Detect corners in each if the frames
		vector<vector<ImageRef> > corners;
		double  num_corners = 0;

		for(unsigned int j=0; j < images.size(); j++)
		{
			vector<ImageRef> c;
			detector(images[j], c, cpf[i]);
			corners.push_back(c);

			num_corners += c.size();
		}

		//Compute and print the repeatability.
		cout << print << num_corners / images.size() << compute_repeatability_exact(warps, corners, fuzz);
	}
}


///This wrapper function computed the repeatability for a given detector and a given
///container of corner densities for variable levels of noise, from 0 to n in steps of 1
///The result is printed to stdout.
///
/// @param images   Images to test repeatability on
/// @param warps    Every warping where warps[i][j] specifies warp from image i to image j.
/// @param detector Pointer to the corner detection function.
/// @param cpf      The number of corners per frame to be tested.
/// @param n		The initial noise level
/// @param fuzz		A corner must be as close as this to be considered repeated
/// @ingroup gRepeatability
void compute_repeatability_noise(const vector<Image<byte> >& images, const vector<vector<Image<array<float, 2> > > >& warps, const DetectN& detector, int cpf,  float n, double fuzz)
{
		
	for(float s=0; s <= n; s++)
	{


		//Detect corners in each if the frames
		vector<vector<ImageRef> > corners;
		double  num_corners = 0;

		for(unsigned int j=0; j < images.size(); j++)
		{
			Image<byte> ni = images[j].copy_from_me();

			//Add noise to the image
			for(Image<byte>::iterator i=ni.begin(); i != ni.end(); i++)
				*i = max(0, min(255, (int)floor(*i + rand_g() * s + .5)));

			vector<ImageRef> c;
			detector(ni, c, cpf);
			corners.push_back(c);

			num_corners += c.size();
		}

		//Compute and print the repeatability.
		cout << print << s << compute_repeatability_exact(warps, corners, fuzz) << num_corners / images.size();
	}
}


///This is the driver function. It reads the command line arguments and calls
///functions to load the data and compute the repeatability.
///
/// @param argc    Number of command line argumentsd.
/// @param argv    Pointer to command line arguments.
/// @ingroup gRepeatability
void mmain(int argc, char** argv)
{
	GUI.LoadFile("test_repeatability.cfg");
	GUI.parseArguments(argc, argv);

	vector<Image<byte> > images;
	vector<vector<Image<array<float, 2> > > > warps;


	int n = GV3::get<int>("num", 2, 1);
	string dir = GV3::get<string>("dir", "./", 1);
	string format = GV3::get<string>("type", "cambridge", 1);
	double fuzz = GV3::get<double>("r", 5, 1);
	vector<int> cpf = GV3::get<vector<int> >("cpf", "0 10 20 30 40 50 60 70 80 90 100 150 200 250 300 350 400 450 500 550 600 650 700 750 800 850 900 950 1000 1100 1200 1300 1400 1500 1600 1700 1800 1900 2000 2200", 1);
	int ncpf = GV3::get<int>("ncpf", 500, 1);
	float nmax = GV3::get<int>("nmax", 50, 1);
	string test = GV3::get<string>("test", "normal", 1);
	
	auto_ptr<DetectN> detector = get_detector();

	rpair(images, warps) = load_data(dir, n, format);
	
	if(test == "noise")
		compute_repeatability_noise(images, warps, *detector, ncpf, nmax, fuzz);
	else
		compute_repeatability_all(images, warps, *detector, cpf, fuzz);

}

///Driving function which catches exceptions
///@param argc Number of command line arguments
///@param argv Commandline argument list
int main(int argc, char** argv)
{
	try
	{
		mmain(argc, argv);
	}
	catch(Exceptions::All e)
	{
		cerr << "Error: " << e.what << endl;
	}	
}
