#ifndef REPEATABILITY_H
#define REPEATABILITY_H

#include <vector>
#include <string>
#include <cvd/image.h>
#include <cvd/byte.h>
#include <tag/array.h>

std::pair<std::vector<CVD::Image<CVD::byte> >, std::vector<std::vector<CVD::Image<tag::array<float, 2> > > > > load_data(std::string dir, int num, std::string format);


void prune_warps(std::vector<std::vector<CVD::Image<tag::array<float, 2> > > >& warps, CVD::ImageRef size);

#endif
