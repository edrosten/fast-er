#ifndef INC_OFFSETS_H
#define INC_OFFSETS_H
#include <cvd/image_ref.h>
#include <vector>
#include <utility>

extern int num_offsets;
extern std::pair<CVD::ImageRef, CVD::ImageRef> offsets_bbox;
extern std::vector<std::vector<CVD::ImageRef> > offsets;

void create_offsets();
void draw_offsets();

#endif
