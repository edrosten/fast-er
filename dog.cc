#include <cvd/image.h>
#include <cvd/convolution.h>
#include <cvd/vision.h>
#include <cvd/gl_helpers.h>
#include <cvd/image_convert.h>

#include <vector>
#include <map>
#include <gvars3/instances.h>

#include "dog.h"
#include "harrislike.h"

#include <TooN/TooN.h>

using namespace std;
using namespace CVD;
using namespace GVars3;
using namespace TooN;


////////////////////////////////////////////////////////////////////////////////
//
// Dog detector
//
typedef Image<float>::iterator fi;


//These classes are used in local_maxima to determine how to 
//modify the indexing of the coarser and finer input images.
//It allows the images to either be a factor of 2 larger or smaller.
///\cond never
struct Equal { static int eval(int x){ return x;} };
struct Larger { static int eval(int x){ return 1+2*x;} };
struct Smaller{ static int eval(int x){ return x/2;} };
///\endcond


template<class LEval, class SEval> void local_maxima(const Image<float>& large, const Image<float>& mid, const Image<float>& small, vector<pair<float, ImageRef> >& corners, int m)
{
		for(int y=1; y < mid.size().y-1; y++)
			for(int x=1; x <mid.size().x-1; x++)
			{
				float c = mid[y][x];

				if( (c > mid[y-1][x-1]  &&
				    c > mid[y-1][x+0]  &&
				    c > mid[y-1][x+1]  &&
				    c > mid[y-0][x-1]  &&
				    c > mid[y-0][x+1]  &&
				    c > mid[y+1][x-1]  &&
				    c > mid[y+1][x+0]  &&
				    c > mid[y+1][x+1]  &&
					c > large[LEval::eval(y)][LEval::eval(x)] &&
					c > small[SEval::eval(y)][SEval::eval(x)]) 
					|| 
					(c < mid[y-1][x-1]  &&
					c < mid[y-1][x+0]  &&
					c < mid[y-1][x+1]  &&
					c < mid[y-0][x-1]  &&
					c < mid[y-0][x+1]  &&
					c < mid[y+1][x-1]  &&
					c < mid[y+1][x+0]  &&
					c < mid[y+1][x+1]  &&
					c < large[LEval::eval(y)][LEval::eval(x)] &&
					c < small[SEval::eval(y)][SEval::eval(x)])
					)
				{
					//A local extrema 

					//Compute the Hessian using finite differences
					Matrix<2> H;

					H[0][0] = mid[y][x-1] - 2 * mid[y][x] + mid[y][x+1];
					H[1][1] = mid[y-1][x] - 2 * mid[y][x] + mid[y+1][x];
					H[0][1] = 0.25 * (mid[y+1][x+1] - mid[y+1][x-1] - mid[y-1][x+1] + mid[y-1][x-1]);
					H[1][0] = H[0][1];

				    double tr = H[0][0] + H[1][1];
				    double det = H[0][0]*H[1][1] - H[0][1]*H[1][0];
				    double edginess =  tr*tr/det;
					double r=10;

				   if (edginess < (r+1)*(r+1)/r)
						corners.push_back(make_pair(-abs(c), m * ImageRef(x, y) + ImageRef(m/2, m/2)));

				}
			}
}

void dog::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const
{
	int s = GV3::get<int>("dog.divisions_per_octave", 3,1);	//Divisions per octave
	int octaves=GV3::get<int>("dog.octaves", 4, 1);

	double k = pow(2, 1.0/s);

	double sigma = GV3::get<double>("dog.sigma", 0.8, 1);

	Image<float> im = convert_image(i);

	convolveGaussian_fir(im, im, sigma);
	

	Image<float> d1, d2, d3;
	c.clear();
	vector<pair<float, ImageRef> > corners;
	corners.reserve(50000);

	int scalemul=1;
	int d1m = 1, d2m = 1, d3m = 1;

	for(int o=0; o < octaves; o++)
	{

		for(int j=0; j < s; j++)
		{
			float delta_sigma = sigma * sqrt(k*k-1);
			Image<float> blurred(im.size());
			convolveGaussian_fir(im, blurred, delta_sigma);
			
			for(fi i1=im.begin(), i2 = blurred.begin(); i1!= im.end(); ++i1, ++i2)
				*i1 = (*i2 - *i1);

			//im is now dog
			//blurred 

			d1 = d2;
			d2 = d3;
			d3 = im;
			im = blurred;

			d1m = d2m;
			d2m = d3m;
			d3m = scalemul;

			//Find maxima
			if(d1.size().x != 0)
			{
				if(d1.size() == d2.size())
					if(d2.size() == d3.size())
						local_maxima<Equal, Equal>(d1, d2, d3, corners, d2m);
					else
						local_maxima<Equal, Smaller>(d1, d2, d3, corners, d2m);
				else
					if(d2.size() == d3.size())
						local_maxima<Larger, Equal>(d1, d2, d3, corners, d2m);
					else
						local_maxima<Larger, Smaller>(d1, d2, d3, corners, d2m);
			}
			

			sigma *= k;
		}
		
		if(o != octaves - 1)
		{
			scalemul *=2;
			sigma /=2;
			Image<float> tmp(im.size()/2);
			halfSample(im,tmp);
			im=tmp;
		}
	}
	

	if(corners.size() > N)
	{
		nth_element(corners.begin(), corners.begin() + N, corners.end());
		corners.resize(N);
	}


	for(unsigned int i=0; i < corners.size(); i++)
		c.push_back(corners[i].second);
}

template<class LEval, class SEval> bool is_scale_maximum(const Image<float>& large, const Image<float>& mid, const Image<float>& small, ImageRef c)
{
	if( 
		  (mid[c] > 0 && 
		  mid[c] > small[SEval::eval(c.y)][SEval::eval(c.x)] && 
		  mid[c] > large[LEval::eval(c.y)][LEval::eval(c.x)])
		||
		  (mid[c] < 0 && 
		  mid[c] < small[SEval::eval(c.y)][SEval::eval(c.x)] && 
		  mid[c] < large[LEval::eval(c.y)][LEval::eval(c.x)]))
		return true;
	else
		return false;
}

bool is_scale_maximum(const Image<float>& d1, const Image<float>& d2, const Image<float>& d3, ImageRef c)
{
	if(d1.size() == d2.size())
		if(d2.size() == d3.size())
			return is_scale_maximum<Equal, Equal>(d1, d2, d3, c);
		else
			return is_scale_maximum<Equal, Smaller>(d1, d2, d3, c);
	else
		if(d2.size() == d3.size())
			return is_scale_maximum<Larger, Equal>(d1, d2, d3, c);
		else
			return is_scale_maximum<Larger, Smaller>(d1, d2, d3, c);
}


void harrisdog::operator()(const CVD::Image<CVD::byte>& i, std::vector<CVD::ImageRef>& c, unsigned int N) const
{
	int s = GV3::get<int>("harrislaplace.dog.divisions_per_octave", 11);	//Divisions per octave
	int octaves=GV3::get<int>("harrislaplace.dog.octaves", 4, 1);
	double sigma = GV3::get<double>("harrislaplace.dog.sigma", 0.8);

	float hblur = GV3::get<float>("harrislaplace.harris.blur", 2.5);
	float hsigmas = GV3::get<float>("harrislaplace.harris.sigmas", 2.0, 1);

	double k = pow(2, 1.0/s);

	Image<float> im = convert_image(i);

	//convolveGaussian(im, sigma);

	Image<float> d1, d2, d3;
	Image<float> im1, im2, im3;
	c.clear();
	vector<pair<float, ImageRef> > corners;
	corners.reserve(50000);

	int scalemul=1;
	int d1m = 1, d2m = 1, d3m = 1;

	for(int o=0; o < octaves; o++)
	{

		for(int j=0; j < s; j++)
		{
			float delta_sigma = sigma * sqrt(k*k-1);
			//hblur *= sqrt(k*k-1);

			//Blur im, and put the result in blurred.
			//im is already blurred from the previous layers
			Image<float> blurred(im.size(), 0);
			convolveGaussian_fir(im, blurred, delta_sigma);
			
			//For DoG, at this point, we don't need im anymore, since blurred
			//will be used as "im" for the next layer. However, we do need it for 
			//HarrisDoG, since we need to do a HarrisDetect on it.
			Image<float>  diff(im.size(), 0);
			for(fi i1=im.begin(), i2 = blurred.begin(), d = diff.begin(); i1!= im.end(); ++i1, ++i2, ++d)
				*d = (*i2 - *i1);
			
			//Insert the current image, and the current difference
			//in to the ring buffer
			d1 = d2;
			d2 = d3;
			d3 = diff;

			im1 = im2;
			im2 = im3;
			im3 = im;


			im = blurred;

			d1m = d2m;
			d2m = d3m;
			d3m = scalemul;

			//Find maxima
			if(d1.size().x != 0)
			{
				//First, find Harris maxima
				vector<pair<float, ImageRef> > layer_corners;
				HarrisDetector(im2, layer_corners, N, hblur, hsigmas);
				
				//Keep if they are LoG (or really DoG) maxima across scales.
				//The Harris score olny is used.
				for(unsigned int c=0; c < layer_corners.size(); c++)
					if(is_scale_maximum(d1, d2, d3, layer_corners[c].second))
						corners.push_back(layer_corners[c]);
			}
			
			sigma *= k;
		}
		
		if(o != octaves - 1)
		{
			scalemul *=2;
			sigma /=2;
			Image<float> tmp(im.size()/2);
			halfSample(im,tmp);
			im=tmp;
		}
	}
	

	if(corners.size() > N)
	{
		nth_element(corners.begin(), corners.begin() + N, corners.end());
		corners.resize(N);
	}
	
	c.clear();

	for(unsigned int i=0; i < corners.size(); i++)
		c.push_back(corners[i].second);

}
