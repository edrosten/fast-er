/**
\file fast_N_features.cc Main file for the fast_N_features executable.

\section wpUsage Usage

<code> fast_N_features [--R r] [--T threshold] IMAGE1 [IMAGE2 ...]</code>

\section Description

Extracts features so that an 
accelerated tree can be learned. The output is is suitable for consumption 
by \link learn_fast_tree.cc learn_fast_tree\endlink.

The program accpets standard GVars3 commandline arguments. 

The images from which
features should be extracted are specified on the commandline. 

This executable extracts pixel rings of radius r, and guarantees that all
feautres are persent at least once. It will go very slow if r is much larger than 3.

*/



#include <iostream>
#include <gvars3/instances.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <cvd/image.h>
#include <cvd/image_io.h>
#include <cvd/morphology.h>
using namespace std;
using CVD::Image;
using CVD::SubImage;
using CVD::ImageRef;
using namespace GVars3;


double angle(const ImageRef i){
	return std::fmod(std::atan2(i.x, i.y) + M_PI*2*10, M_PI*2);
}

vector<ImageRef> circle(double r){
	//Generate a 1 pixel wide, 8-connected circle.
	
	int r_ = int(r+2);
	ImageRef size(2*r_+1, 2*r_+1);
	ImageRef centre = size/2;

	Image<bool> im(size);

	ImageRef p{0,0};
	do
		if((p-centre).mag_squared() <= r*r)
			im[p]=1;
	while(p.next(im.size()));

	Image<bool> circle = CVD::morphology(im, {ImageRef{-1,0}, ImageRef{1,0}, ImageRef{0,-1}, ImageRef{0,1}}, CVD::Morphology::BinaryErode{});	
	vector<ImageRef> pixels;
	do{
		im[p]=im[p] && !circle[p];
		if(im[p])
			pixels.push_back(p-centre);
	}
	while(p.next(im.size()));
	
	sort(pixels.begin(), pixels.end(), [](const auto& a, const auto& b){
		return angle(a) < angle(b);
	});

	Image<string> viz(im.size(), "•");
	for(const auto& p:pixels){
		viz[p+centre] = 'A'+(&p - pixels.data());	
	}


	for(int y=0; y < im.size().y; y++){
		for(int x=0; x < im.size().x; x++)
			clog << viz[y][x];
		clog << "\n";
	}
	
	return pixels;
}

void extract(const SubImage<uint8_t>& im, int threshold, const vector<ImageRef>& ring, std::unordered_map<string, uint64_t>& counts){
	int lo=100000000, hi=-100000000;

	for(const auto& p: ring){
		lo=std::min(lo, p.x);
		lo=std::min(lo, p.y);
		hi=std::max(hi, p.x);
		hi=std::max(hi, p.y);
	}

	std::string thresholded;
	thresholded.resize(ring.size());
	for(int y=-lo; y < im.size().y - hi; y++)
		for(int x=-lo; x < im.size().x - hi; x++){
			for(size_t i=0; i < ring.size(); i++){
				ImageRef c = ImageRef{x,y};

				if(im[ring[i] + c] > im[c] + threshold)
					thresholded[i]='b';
				else if(im[ring[i] + c] < im[c] - threshold)
					thresholded[i]='d';
				else
					thresholded[i]='s';
			}
			counts[thresholded]++;
		}
}


int main(int argc, char ** argv){
	int lastarg = GUI.parseArguments(argc, argv);

	vector<ImageRef> ring = circle(GV3::get<double>("R", 3.3));
	int N = ring.size();

	
	//Twice the length necessary and contains two copies
	//this lest a simple string search wrap across the end
	std::string pixels_2(2*N, 'b');

	std::string pattern_b(N/2+1, 'b');
	std::string pattern_d(N/2+1, 'd');

	clog << "FAST " << pattern_b.size() << "/" << N << endl;
	


	unordered_map<string, uint64_t> features; //list of all features ever
	unordered_map<string, bool> corners; //classification of all features

	clog << "Generating...\n";

	// Pre-generate one of each kind, and classify them
	for(;;){

		bool corner = 0; 

		if(pixels_2.find(pattern_b) != string::npos)
			corner = 1;
		else if(pixels_2.find(pattern_d) != string::npos)
			corner = 1;
		
		//const auto v = string_view(pixels_2).substr(N)

		corners[pixels_2.substr(N)] = corner;
		features[pixels_2.substr(N)]++;

		for(int i=N-1; i >= 0; i--){
			switch(pixels_2[i]){
				case 'b':
					pixels_2[i] = pixels_2[i+N] = 's';
					goto loop;

				case 's':
					pixels_2[i] = pixels_2[i+N] = 'd';
					goto loop;

				case 'd':
					pixels_2[i] = pixels_2[i+N] = 'b';
					break;

			}
		}	
		goto overflow;
		loop:;

		if(corners.size() % 100000 == 0)
			clog << "   " << corners.size() * 100 / std::pow(3, N) << "%\r" << flush;
	}
	
	overflow:;
	clog << endl;

	for(int n=lastarg; n < argc; n++){
		clog << "   " << n*100/argc << "\r" << flush;
		Image<uint8_t> im = CVD::img_load(argv[n]);
		extract(im, GV3::get<int>("T", 30), ring, features);
	}
	clog << endl;
	
	uint64_t total=0;
	for(const auto& f:features)
		total += f.second;
	clog << "Features extracted = " << total << endl;

	
	cout << N << endl;
	for(const auto& p:ring)
		cout << p << " ";
	cout << endl;

	vector<string> all;
	for(const auto& c:corners)
		all.emplace_back(c.first);
	sort(all.begin(), all.end());

	for(const auto& str: all){
		cout << str << " " << corners[str] << " " << features[str] << endl;
	}
}
