#include <iostream>
#include <array>
#include <cmath>
#include <cvd/image.h>
#include <cvd/morphology.h>
using namespace std;
using CVD::Image;
using CVD::ImageRef;


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

int main(){

	vector<ImageRef> ring = circle(3.3);
	int N = ring.size();

	
	//A -> brighter
	//B -> darker
	//C -> similar
	std::string pixels_2(2*N, 'b');

	std::string pattern_b(N/2+1, 'b');
	std::string pattern_d(N/2+1, 'd');

	clog << "FAST " << pattern_b.size() << "/" << N << endl;


	cout << N << endl;
	for(const auto& p:ring)
		cout << p << " ";
	cout << endl;
	for(;;){

		bool corner = 0; 

		if(pixels_2.find(pattern_b) != string::npos)
			corner = 1;
		else if(pixels_2.find(pattern_d) != string::npos)
			corner = 1;

		cout << pixels_2.substr(N) << " 1 " << corner << "\n";


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
	}
	
	overflow:;


}
