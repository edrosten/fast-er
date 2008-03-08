#include <cvd/image_io.h>
#include <cvd/vector_image_ref.h>
#include <tag/printf.h>
#include <tag/array.h>
#include <TooN/TooN.h>
#include <TooN/LU.h>
#include <TooN/helpers.h>

#include "load_data.h"
#include "warp_to_png.h"
#include "utility.h"

using namespace std;
using namespace CVD;
using namespace tag;
using namespace TooN;

/** Load images from a "Cambridge" style dataset.

@param dir The base directory of the dataset.
@param n   The number of images in the dataset.
@return The loaded images.
@ingroup gDataset
*/
vector<Image<byte> > load_images_cambridge(string dir, int n)
{
	dir += "/frames/frame_%i.pgm";

	vector<Image<byte> > ret;

	for(int i=0;  i < n; i++)
	{
		Image<byte> im;
		im = img_load(sPrintf(dir, i));
		ret.push_back(im);
	}

	return ret;
}

/** Load images from an "Oxford VGG" style dataset.

@param dir The base directory of the dataset.
@param n   The number of images in the dataset.
@return The loaded images.
@ingroup gDataset
*/
vector<Image<byte> > load_images_vgg(string dir, int n)
{
	dir += "/img%i.ppm";

	vector<Image<byte> > ret;

	for(int i=0;  i < n; i++)
		ret.push_back(img_load(sPrintf(dir, i+1)));

	return ret;
}

///Load an array from an istream
///@ingroup gUtility
istream& operator>>(istream& i, array<float, 2>& f)
{
	i >> f[0] >> f[1];
	return i;
}

///Convert a vector in to an array
///@ingroup gUtility
array<float, 2> Arr(const Vector<2>& vec)
{
	return array<float, 2>((TupleHead, vec[0], vec[1]));
}


/**Load warps from a "Cambridge" repeatability dataset, with the warps
stored encoded in PNG files. See load_warps_cambridge


@param dir  The base directory of the dataset.
@param num   The numbers of images in the dataset.
@param size  The size of the corresponding images.
@return  <code>return_value[i][j][y][x]</code> is where pixel x, y in image i warps to in image j.
@ingroup gDataset
*/
vector<vector<Image<array<float,2> > > > load_warps_cambridge_png(string dir, int num, ImageRef size)
{
	dir += "/pngwarps/warp_%i_%i.png";

	vector<vector<Image<array<float, 2> > > > ret(num, vector<Image<array<float, 2> > >(num));

	BasicImage<byte> tester(NULL, size);

	array<float, 2> outside((TupleHead, -1, -1));

	for(int from = 0; from < num; from ++)
		for(int to = 0; to < num; to ++)
			if(from != to)
			{
				string fname = sPrintf(dir, from, to);
				Image<Rgb<unsigned short> > p = img_load(fname);

				if(p.size() != size)
				{
					cerr << "Error: warp file " << fname << " is the wrong size!\n";
					exit(1);
				}

				Image<array<float,2> > w(size, outside);

				for(int y=0; y < size.y; y++)
					for(int x=0; x < size.x; x++)
					{
						w[y][x][0] = p[y][x].red / MULTIPLIER - SHIFT;
						w[y][x][1] = p[y][x].green / MULTIPLIER - SHIFT;
					}


				cerr << "Loaded " << fname << endl;

				ret[from][to] = w;
			}

	return ret;
}

/**Load warps from a "Cambridge" repeatability dataset. 

The dataset contains warps which round to outside the image by one pixel in the max direction.

Note that the line labelled "prune" is diasbled in the evaluation of the FAST-ER system. This
causes the two systems to produce slightly different results. If this line is commented out, then
FAST-ER generated detectors produce exactly the same results when loaded back in to this system.

@param dir  The base directory of the dataset.
@param num   The numbers of images in the dataset.
@param size  The size of the corresponding images.
@return  <code>return_value[i][j][y][x]</code> is where pixel x, y in image i warps to in image j.
@ingroup gDataset
*/
vector<vector<Image<array<float,2> > > > load_warps_cambridge(string dir, int num, ImageRef size)
{
	dir += "/warps/warp_%i_%i.warp";

	vector<vector<Image<array<float, 2> > > > ret(num, vector<Image<array<float, 2> > >(num));

	BasicImage<byte> tester(NULL, size);

	array<float, 2> outside((TupleHead, -1, -1));

	for(int from = 0; from < num; from ++)
		for(int to = 0; to < num; to ++)
			if(from != to)
			{
				Image<array<float,2> > w(size, outside);
				int n = size.x * size.y;
				Image<array<float,2> >::iterator p = w.begin();

				ifstream f;
				string fname = sPrintf(dir, from, to);
				f.open(fname.c_str());

				if(!f.good())
				{
					cerr << "Error: " << fname << ": " << strerror(errno) << endl;
					exit(1);
				}

				array<float, 2> v;

				for(int i=0; i < n; ++i, ++p)
				{
					f >> v;
					//prune
					//if(v[0] >= 0 && v[1] >= 0 && v[0] <= size.x-1 && v[1] <= size.y-1)
						*p = v;
				}
				
				if(!f.good())
				{
					cerr << "Error: " << fname << " went bad" << endl;
					exit(1);
				}

				cerr << "Loaded " << fname << endl;

				ret[from][to] = w;
			}

	return ret;
}

///Invert a matrix
///@ingroup gUtility
Matrix<3> invert(const Matrix<3>& m)
{
	LU<3> i(m);
	return i.get_inverse();
}

/**Load warps from an "Oxford VGG" repeatability dataset.  The warps are stored
as homographies, so warps need to be generated.

@param dir  The base directory of the dataset.
@param num   The numbers of images in the dataset.
@param size  The size of the corresponding images.
@return  <code>return_value[i][j][y][x]</code> is where pixel x, y in image i warps to in image j.
@ingroup gDataset
*/
vector<vector<Image<array<float, 2> > > > load_warps_vgg(string dir, int num, ImageRef size)
{
	dir += "/H1to%ip";
	array<float, 2> outside((TupleHead, -1, -1));

	//Load the homographies
	vector<Matrix<3> > H_1_to_x;
	
	//The first homography is always the identity.
	{
		Matrix<3> i;
		Identity(i);
		H_1_to_x.push_back(i);
	}

	for(int i=2; i <= num; i++)
	{
		ifstream f;
		string fname = sPrintf(dir, i).c_str();
		f.open(fname.c_str());

		Matrix<3> h;
		f >> h;

		if(!f.good())
		{
			cerr << "Error: " << fname << " went bad" << endl;
			exit(1);
		}

		H_1_to_x.push_back(h);
	}

	vector<vector<Image<array<float, 2> > > > ret(num, vector<Image<array<float, 2> > >(num));
	
	//Generate the warps.
	for(int from = 0; from < num; from ++)
		for(int to = 0; to < num; to ++)
			if(from != to)
			{
				Matrix<3> from_to_one = invert(H_1_to_x[from]);
				Matrix<3> one_to_to   = H_1_to_x[to];
				Matrix<3> from_to_to = one_to_to * from_to_one;

				Image<array<float,2> > w(size, outside);

				for(int y=0; y < size.y; y++)
					for(int x=0; x < size.x; x++)
					{
						Vector<2> p = project(from_to_to * Vector<3>((make_Vector, x, y, 1)));

						if(p[0] >= 0 && p[1] >= 0 && p[0] <= size.x-1 && p[1] <= size.y-1)
							w[y][x] = Arr(p);
					}

				ret[from][to] = w;

				cerr << "Created warp " << from << " -> " << to << endl;
			}
	
	return ret;
}


enum DataFormat
{
	Cambridge,
	CambridgePNGWarp,
	VGG
};

/**Load a dataset.
@param dir The base directory of the dataset.
@param num The number of images in the dataset.
@param format The type of the dataset.
@return The images and the warps.
@ingroup gDataset
*/
pair<vector<Image<byte> >, vector<vector<Image<array<float, 2> > > > > load_data(string dir, int num, string format)
{
	vector<Image<byte> > images;
	vector<vector<Image<array<float, 2> > > > warps;

	DataFormat d;

	if(format == "vgg")
		d = VGG;
	else if(format == "cam-png")
		d = CambridgePNGWarp;
	else
		d = Cambridge;

	switch(d)
	{
		case Cambridge:
		case CambridgePNGWarp:
			images = load_images_cambridge(dir, num);
			break;

		case VGG:
			images = load_images_vgg(dir, num);
	};

	//Check for sanity
	if(images.size() == 0)
	{
		cerr << "No images!\n";
		exit(1);
	}

	for(unsigned int i=0; i < images.size(); i++)
		if(images[i].size() != images[0].size())
		{
			cerr << "Images are different sizes!\n";
			exit(1);
		}

	switch(d)
	{
		case CambridgePNGWarp:
			warps = load_warps_cambridge_png(dir, num, images[0].size());
			break;

		case Cambridge:
			warps = load_warps_cambridge(dir, num, images[0].size());
			break;

		case VGG:
			warps = load_warps_vgg(dir, num, images[0].size());
	};


	return make_pair(images, warps);
}


/**
This function prunes a dataset so that no warped point will lie outside an image. This
will save onmn .in_image() tests later.
@param warps The warps to prune.
@param size the image size to prune to.
@ingroup gDataset
*/
void prune_warps(vector<vector<Image<array<float, 2> > > >& warps, ImageRef size)
{
	BasicImage<byte> test(NULL, size);
	array<float, 2> outside = make_tuple(-1, -1);

	for(unsigned int i=0; i < warps.size(); i++)	
		for(unsigned int j=0; j < warps[i].size(); j++)	
		{
			for(Image<array<float, 2> >::iterator  p=warps[i][j].begin(); p != warps[i][j].end(); p++)
				if(!test.in_image(ir_rounded(*p)))
					*p = outside;
		}
}



