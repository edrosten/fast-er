#ifndef REPEATABILITY_H
#define REPEATABILITY_H

#include <vector>
#include <string>
#include <cvd/image.h>
#include <cvd/byte.h>
#include <tag/array.h>
/*
std::vector<CVD::Image<CVD::byte> > load_images_cambridge(std::string dir, int n);
std::vector<std::vector<CVD::Image<tag::array<float, 2> > > > load_warps_cambridge(std::string dir, int num, CVD::ImageRef size);
std::vector<CVD::Image<CVD::byte> > load_images_vgg(std::string dir, int n);
std::vector<std::vector<CVD::Image<tag::array<float, 2> > > > load_warps_vgg(std::string dir, int num, CVD::ImageRef size);
*/

std::pair<std::vector<CVD::Image<CVD::byte> >, std::vector<std::vector<CVD::Image<tag::array<float, 2> > > > > load_data(std::string dir, int num, std::string format);
#endif
