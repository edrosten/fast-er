#include <cvd/image.h>
#include <cvd/byte.h>
#include <vector>
#include <cstdlib>
#include <cmath>
using namespace CVD;
using namespace std;

int erabs(int x)
{
	/*if(x < 0)
		return -x;
	else
		return x;
	*/
	return std::abs(x);

	//return x>0?x:-x;
	/*asm volatile
	(
		".intel_syntax noprefix\n\t"
		"mov        ecx, eax\n\t"
		"neg        ecx\n\t"
		"cmovns     eax, ecx\n\t"
		".att_syntax\n\t"
		: "=a" (x) : "a" (x) : "ecx", "flags"
	) ;

	return x;*/

	//return fabs(x);
}

static inline bool is_a_corner(const byte* p, const int pixel[], byte b)
{   
	byte c = *p;

        if(erabs(p[pixel[0]]-c) > b)
         if(erabs(p[pixel[1]]-c) > b)
          if(erabs(p[pixel[2]]-c) > b)
           if(erabs(p[pixel[3]]-c) > b)
            if(erabs(p[pixel[4]]-c) > b)
             if(erabs(p[pixel[5]]-c) > b)
              if(erabs(p[pixel[6]]-c) > b)
               if(erabs(p[pixel[7]]-c) > b)
                if(erabs(p[pixel[8]]-c) > b)
                 return true;
                else
                 if(erabs(p[pixel[15]]-c) > b)
                  return true;
                 else
                  return false;
               else
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  return true;
                 else
                  return false;
                else
                 return false;
              else
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  return true;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  return true;
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      if(erabs(p[pixel[10]]-c) > b)
                       if(erabs(p[pixel[11]]-c) > b)
                        return true;
                       else
                        return false;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             if(erabs(p[pixel[11]]-c) > b)
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  return true;
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      if(erabs(p[pixel[10]]-c) > b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      if(erabs(p[pixel[10]]-c) > b)
                       return true;
                      else
                       return false;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                return false;
              else
               return false;
             else
              return false;
           else
            if(erabs(p[pixel[10]]-c) > b)
             if(erabs(p[pixel[11]]-c) > b)
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  return true;
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      return true;
                     else
                      return false;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               return false;
             else
              return false;
            else
             return false;
          else
           if(erabs(p[pixel[9]]-c) > b)
            if(erabs(p[pixel[10]]-c) > b)
             if(erabs(p[pixel[11]]-c) > b)
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  return true;
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                else
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if(erabs(p[pixel[3]]-c) > b)
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     return true;
                    else
                     return false;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              return false;
            else
             return false;
           else
            return false;
         else
          if(erabs(p[pixel[8]]-c) > b)
           if(erabs(p[pixel[9]]-c) > b)
            if(erabs(p[pixel[10]]-c) > b)
             if(erabs(p[pixel[11]]-c) > b)
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  return true;
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                else
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
               else
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if(erabs(p[pixel[3]]-c) > b)
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if(erabs(p[pixel[2]]-c) > b)
               if(erabs(p[pixel[3]]-c) > b)
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    return true;
                   else
                    return false;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             return false;
           else
            return false;
          else
           return false;
        else
         if(erabs(p[pixel[7]]-c) > b)
          if(erabs(p[pixel[8]]-c) > b)
           if(erabs(p[pixel[9]]-c) > b)
            if(erabs(p[pixel[6]]-c) > b)
             if(erabs(p[pixel[5]]-c) > b)
              if(erabs(p[pixel[4]]-c) > b)
               if(erabs(p[pixel[3]]-c) > b)
                if(erabs(p[pixel[2]]-c) > b)
                 if(erabs(p[pixel[1]]-c) > b)
                  return true;
                 else
                  if(erabs(p[pixel[10]]-c) > b)
                   return true;
                  else
                   return false;
                else
                 if(erabs(p[pixel[10]]-c) > b)
                  if(erabs(p[pixel[11]]-c) > b)
                   return true;
                  else
                   return false;
                 else
                  return false;
               else
                if(erabs(p[pixel[10]]-c) > b)
                 if(erabs(p[pixel[11]]-c) > b)
                  if(erabs(p[pixel[12]]-c) > b)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
              else
               if(erabs(p[pixel[10]]-c) > b)
                if(erabs(p[pixel[11]]-c) > b)
                 if(erabs(p[pixel[12]]-c) > b)
                  if(erabs(p[pixel[13]]-c) > b)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
             else
              if(erabs(p[pixel[10]]-c) > b)
               if(erabs(p[pixel[11]]-c) > b)
                if(erabs(p[pixel[12]]-c) > b)
                 if(erabs(p[pixel[13]]-c) > b)
                  if(erabs(p[pixel[14]]-c) > b)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
            else
             if(erabs(p[pixel[10]]-c) > b)
              if(erabs(p[pixel[11]]-c) > b)
               if(erabs(p[pixel[12]]-c) > b)
                if(erabs(p[pixel[13]]-c) > b)
                 if(erabs(p[pixel[14]]-c) > b)
                  if(erabs(p[pixel[15]]-c) > b)
                   return true;
                  else
                   return false;
                 else
                  return false;
                else
                 return false;
               else
                return false;
              else
               return false;
             else
              return false;
           else
            return false;
          else
           return false;
         else
          return false;
}

static inline int corner_score(const byte* p, const int pixel[], int bstart)
{    
    int bmin = bstart;
    int bmax = 255;
    int b = (bmax + bmin)/2;
    
    //Compute the score using binary search
	for(;;)
    {
		if(is_a_corner(p, pixel, b))
           	bmin = b;
		else
            bmax = b;
        
		if(bmin == bmax - 1 || bmin == bmax)
			return bmin;
		b = (bmin + bmax) / 2;
    }
}

static void make_offsets(int pixel[], int row_stride)
{
        pixel[0] = 0 + row_stride * 3;
        pixel[1] = 1 + row_stride * 3;
        pixel[2] = 2 + row_stride * 2;
        pixel[3] = 3 + row_stride * 1;
        pixel[4] = 3 + row_stride * 0;
        pixel[5] = 3 + row_stride * -1;
        pixel[6] = 2 + row_stride * -2;
        pixel[7] = 1 + row_stride * -3;
        pixel[8] = 0 + row_stride * -3;
        pixel[9] = -1 + row_stride * -3;
        pixel[10] = -2 + row_stride * -2;
        pixel[11] = -3 + row_stride * -1;
        pixel[12] = -3 + row_stride * 0;
        pixel[13] = -3 + row_stride * 1;
        pixel[14] = -2 + row_stride * 2;
        pixel[15] = -1 + row_stride * 3;
}



void twofast_score(const SubImage<byte>& i, const vector<ImageRef>& corners, int b, vector<int>& scores)
{
    scores.resize(corners.size());
	int pixel[16];
	make_offsets(pixel, i.row_stride());

    for(unsigned int n=0; n < corners.size(); n++)
        scores[n] = corner_score(&i[corners[n]], pixel, b);
}


void twofast_detect(const SubImage<byte>& i, vector<ImageRef>& corners, int b)
{
	corners.clear();

	int pixel[16];
	make_offsets(pixel, i.row_stride());

	for(int y=3; y < i.size().y - 3; y++)
		for(int x=3; x < i.size().x - 3; x++)
		{
			const byte* p = i[y] + x;
		
			int c = *p;
        if(erabs(p[pixel[0]]-c) > b)
         if(erabs(p[pixel[1]]-c) > b)
          if(erabs(p[pixel[2]]-c) > b)
           if(erabs(p[pixel[3]]-c) > b)
            if(erabs(p[pixel[4]]-c) > b)
             if(erabs(p[pixel[5]]-c) > b)
              if(erabs(p[pixel[6]]-c) > b)
               if(erabs(p[pixel[7]]-c) > b)
                if(erabs(p[pixel[8]]-c) > b)
                 {}
                else
                 if(erabs(p[pixel[15]]-c) > b)
                  {}
                 else
                  continue;
               else
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  {}
                 else
                  continue;
                else
                 continue;
              else
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  {}
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  {}
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      if(erabs(p[pixel[10]]-c) > b)
                       if(erabs(p[pixel[11]]-c) > b)
                        {}
                       else
                        continue;
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 continue;
               else
                continue;
              else
               continue;
            else
             if(erabs(p[pixel[11]]-c) > b)
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  {}
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      if(erabs(p[pixel[10]]-c) > b)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      if(erabs(p[pixel[10]]-c) > b)
                       {}
                      else
                       continue;
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                continue;
              else
               continue;
             else
              continue;
           else
            if(erabs(p[pixel[10]]-c) > b)
             if(erabs(p[pixel[11]]-c) > b)
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  {}
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     if(erabs(p[pixel[9]]-c) > b)
                      {}
                     else
                      continue;
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               continue;
             else
              continue;
            else
             continue;
          else
           if(erabs(p[pixel[9]]-c) > b)
            if(erabs(p[pixel[10]]-c) > b)
             if(erabs(p[pixel[11]]-c) > b)
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  {}
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                else
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               if(erabs(p[pixel[3]]-c) > b)
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    if(erabs(p[pixel[8]]-c) > b)
                     {}
                    else
                     continue;
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else
              continue;
            else
             continue;
           else
            continue;
         else
          if(erabs(p[pixel[8]]-c) > b)
           if(erabs(p[pixel[9]]-c) > b)
            if(erabs(p[pixel[10]]-c) > b)
             if(erabs(p[pixel[11]]-c) > b)
              if(erabs(p[pixel[12]]-c) > b)
               if(erabs(p[pixel[13]]-c) > b)
                if(erabs(p[pixel[14]]-c) > b)
                 if(erabs(p[pixel[15]]-c) > b)
                  {}
                 else
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    {}
                   else
                    continue;
                  else
                   continue;
                else
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
               else
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               if(erabs(p[pixel[3]]-c) > b)
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else
              if(erabs(p[pixel[2]]-c) > b)
               if(erabs(p[pixel[3]]-c) > b)
                if(erabs(p[pixel[4]]-c) > b)
                 if(erabs(p[pixel[5]]-c) > b)
                  if(erabs(p[pixel[6]]-c) > b)
                   if(erabs(p[pixel[7]]-c) > b)
                    {}
                   else
                    continue;
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
            else
             continue;
           else
            continue;
          else
           continue;
        else
         if(erabs(p[pixel[7]]-c) > b)
          if(erabs(p[pixel[8]]-c) > b)
           if(erabs(p[pixel[9]]-c) > b)
            if(erabs(p[pixel[6]]-c) > b)
             if(erabs(p[pixel[5]]-c) > b)
              if(erabs(p[pixel[4]]-c) > b)
               if(erabs(p[pixel[3]]-c) > b)
                if(erabs(p[pixel[2]]-c) > b)
                 if(erabs(p[pixel[1]]-c) > b)
                  {}
                 else
                  if(erabs(p[pixel[10]]-c) > b)
                   {}
                  else
                   continue;
                else
                 if(erabs(p[pixel[10]]-c) > b)
                  if(erabs(p[pixel[11]]-c) > b)
                   {}
                  else
                   continue;
                 else
                  continue;
               else
                if(erabs(p[pixel[10]]-c) > b)
                 if(erabs(p[pixel[11]]-c) > b)
                  if(erabs(p[pixel[12]]-c) > b)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
              else
               if(erabs(p[pixel[10]]-c) > b)
                if(erabs(p[pixel[11]]-c) > b)
                 if(erabs(p[pixel[12]]-c) > b)
                  if(erabs(p[pixel[13]]-c) > b)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
             else
              if(erabs(p[pixel[10]]-c) > b)
               if(erabs(p[pixel[11]]-c) > b)
                if(erabs(p[pixel[12]]-c) > b)
                 if(erabs(p[pixel[13]]-c) > b)
                  if(erabs(p[pixel[14]]-c) > b)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
            else
             if(erabs(p[pixel[10]]-c) > b)
              if(erabs(p[pixel[11]]-c) > b)
               if(erabs(p[pixel[12]]-c) > b)
                if(erabs(p[pixel[13]]-c) > b)
                 if(erabs(p[pixel[14]]-c) > b)
                  if(erabs(p[pixel[15]]-c) > b)
                   {}
                  else
                   continue;
                 else
                  continue;
                else
                 continue;
               else
                continue;
              else
               continue;
             else
              continue;
           else
            continue;
          else
           continue;
         else
          continue;

			corners.push_back(ImageRef(x, y));
		}

}


