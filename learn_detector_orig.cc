/**

@mainpage

\section Introduction

This program created a corner detector by optimizing a decision tree using simulated annealing.
The tree is optimized to maximize the repeatability of the corner detector.

\section Instructions

To make on any unix-like environment, do:

<code>./configure @@ make</code>

There is no install option. To set the parameters, examine <code>learn_detector.cfg</code>.
In order to run this program, you will need a repeatability dataset, such as the one from
http://mi.eng.cam.ac.uk/~er258/work/datasets.html

The running program will generate a very extensive logfile on the standard
output, which will include the learned detector and the results of its
repeatability evaluation. Running <code>get_block_detector</code> on the output
will generate source code for the detector. Note that this source code will not
yet have been optimized for speed, only repeatability.

The complete sequence of operations is as follows
<ol>
	<li> Make the executable:

		<code> ./configure && make </code>

	<li> Set up <code>learn_detector.cfg</code>. The default parameters are good,
		 except you will need to set up <code>datadir</code> to point to the
		 repeatability dataset.

	<li> Run the corner detector learning program

		<code>./learn_detector > logfile</code>

		If you run it more than once, you will probably want to alter the random
		seed in the configuration file.

	<li> Extract a detector from the logfile

	     <code>./get_block_detector logfile &gt; learned-faster-detector.h</code>

	<li> make <code>extract-fast2.cxx</code>
		
		Note that if you have changed the list of available offsets in
		<code>learn_detector.cfg</code> then you will need to change
		<code>offset_list</code> in <code>extract-fast2.cxx</code>. The list of
		offsets will be in the logfile.

	<li> Use <code>extract-fast2.cxx</code> to extract features from a set of
	     training images. Any reasonable images can be used, including the 
		 training images used earlier.  Do:

		 <code>./extract-fast2<code><i>imagefile  imagefile2 ...<i><code>&gt; features</code>
</ol>

*/

/**
@defgroup gRepeatability Measuring the repeatability of a detector

Functions to load a repeatability dataset, and compute the repeatability of
a list of detected points.


\section data The dataset

The dataset consists of a number of registered images. The images are stored in
<code>frames/frame_X.pgm</code> where X is an integer counting from zero.  The
frames must all be the same size. The warps are stored in
<code>waprs/warp_Y_Z.warp</code>. The file <code>warp_Y_Z.warp</code> contains
one line for every pixel in image Y (pixels arranged in raster-scan order). The
line is the position that the pixel warps to in image Z. If location of -1, -1
indicates that this pixel does not appear in image Z.

*/

/**
@defgroup  gTree Tree representation.

*/

/**
@defgroup  gFastTree Compiled tree representations

*/

/**
@defgroup  gUtility Utility functions.

*/

/**
@defgroup  gOptimize Optimization routines

*/
#include <iostream>
#include <iomanip>
#include <fstream>
#include <climits>
#include <float.h>
#include <cstring>
#include <cerrno>
#include <cmath>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>

#include <cvd/image.h>
#include <cvd/image_io.h>
#include <cvd/byte.h>
#include <cvd/random.h>
#include <cvd/fast_corner.h>
#include <cvd/vector_image_ref.h>
#include <cvd/timer.h>
#include <cvd/cpu_hacks.h>

#include <tag/tuple.h>
#include <tag/stdpp.h>
#include <tag/fn.h>
#include <tag/printf.h>

#include <sys/mman.h>

#include <TooN/TooN.h>
#include <TooN/helpers.h>
#include "svector.h"

#include <gvars3/serialize.h>


namespace GVars3{ namespace serialize {
	
	/**GVars serialization for containers. FIXME: should use existing serialization
	   to load types, rather than >>

	   @ingroup gUtility
	*/

	template<class C, template<class> class D >std::string to_string(const D<C>& s)
	{
		std::ostringstream o;
		typename D<C>::const_iterator i;

		for(i=s.begin();i != s.end(); i++)
		{
			if(i != s.begin())
				o <<  " ";
			o << *i;
		}

		return o.str();
	}

	template<class C, template<class> class D> int from_string(const std::string& s, D<C>& o)
	{
		std::istringstream i(s);
		using namespace tag;
		
		while(1)
		{	
			C c;
			i >> c;

			if(i) //No data lost: 
				o.insert(o.end(), c);
			else //Stream finished for some reason (either bad or b0rked)
				return check_stream(i);
		}
	}
}}
#include <gvars3/instances.h>


using namespace std;
using namespace CVD;
using namespace tag;
using namespace GVars3;
using namespace TooN;

///Actual x,y offset of the offset numbers in the different available orientations.
///@ingroup gTree
vector<vector<ImageRef> > offsets;
///The number of possible offsets. Equivalent to <code>offsets[x].size()</code>
///@ingroup gTree
int num_offsets;
///Bounding box for offsets in all orientations. This is therefore a bounding box for the detector.
///@ingroup gTree
pair<ImageRef, ImageRef> offsets_bbox;



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


///Populate a std::vector with the numbers 0,1,...,num
///@param num Size if the range
///@return the populated vector.
///@ingroup Utility
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

///Computes repeatability the slow way to avoid rounding errors, by comparing the warped
///corner position to every detected corner.
///
/// @param warps    Every warping where warps[i][j] specifies warp from image i to image j.
/// @param corners  Detected corners
/// @param r		A corner must be as close as this to be considered repeated
/// @return 		The repeatability 
/// @ingroup gRepeatability
double compute_repeatability_exact(const vector<vector<Image<Vector<2> > > >& warps, const vector<vector<ImageRef> >& corners, double r)
{
	unsigned int n = corners.size();

	int corners_tested = 0;
	int good_corners = 0;

	r *= r;

	for(unsigned int i=0; i < n; i++)
		for(unsigned int j=0; j < n; j++)
		{
			if(i==j)
				continue;

			for(unsigned int k=0; k < corners[i].size(); k++)
			{
				Vector<2> p = warps[i][j][corners[i][k]];

				if(p[0] != -1) //pixel does not warp to inside image j
				{

					corners_tested++;

					for(unsigned int l=0; l < corners[j].size(); l++)
					{
						Vector<2> d = p - vec(corners[j][l]);

						if(d*d < r)
						{
							good_corners++;
							break;
						}
					}
				}
			}
		}

	return 1.0 * good_corners / (corners_tested + DBL_EPSILON);
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
float compute_repeatability(const vector<vector<Image<Vector<2> > > >& warps, const vector<vector<ImageRef> >& corners, int r, ImageRef size)
{
	cvd_timer tmr;
	unsigned int n = corners.size();

	vector<ImageRef> disc = generate_disc(r);

	vector<Image<bool> >  detected;
	for(unsigned int i=0; i < n; i++)
		detected.push_back(paint_circles(corners[i], disc, size));
	
	cout << "time_rep_paint " << tmr.get_time() << endl;
	
	tmr.reset();
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
	
	cout << "time_rep_rep " << tmr.get_time() << endl;
	return 1.0 * good_corners / (DBL_EPSILON + corners_tested);
}



/**Load warps from a repeatability dataset. 


The dataset contains warps which round to outside the image by one pixel in the max direction.
  Fast repeatability testing founds values to integers, which causes errors, so these points need
  to be pruned from the dataset. Slow repeatability testing does not do this, and these values must
  be left so that the same data _exactly_ are used for the testing as int FAST-9 paper.

@param stub  The directory of the whole dataset.
@param num   The image numbers for which warps will be loaded.
@param size  The size of the corresponding images.
@param prune Whether to prune points which warp to outside the images.
@return  <code>return_value[i][j][y][x]</code> is where pixel x, y in image i warps to in image j.
@ingroup gRepeatability
*/
vector<vector<Image<Vector<2> > > > load_warps(string stub, const vector<int>& num, ImageRef size, bool prune=1)
{
	stub += "/warps/";

	vector<vector<Image<Vector<2> > > > ret(num.size(), vector<Image<Vector<2> > >(num.size()));

	BasicImage<byte> tester(NULL, size);

	Vector<2> outside = (make_Vector, -1, -1);

	for(unsigned int from = 0; from < num.size(); from ++)
		for(unsigned int to = 0; to < num.size(); to ++)
			if(from != to)
			{
				Image<Vector<2> > w(size, (make_Vector, -1, -1));
				int n = size.x * size.y;
				Image<Vector<2> >::iterator p = w.begin();

				ifstream f;
				string fname = stub + sPrintf("warp_%i_%i.warp", num[from], num[to]);
				f.open(fname.c_str());

				if(!f.good())
				{
					cerr << "Error: " << fname << ": " << strerror(errno) << endl;
					exit(1);
				}

				for(int i=0; i < n; ++i, ++p)
				{
					f >> *p;
					if(prune && !tester.in_image(ir_rounded(*p)))
						*p = outside;
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

///Load images from a dataset
///@param stub  The directory of the whole dataset.
///@param num   The image numbers for which warps will be loaded.
///@return The loaded images.
///@ingroup gRepeatability
vector<Image<byte> > load_images(string stub, const vector<int>& num)
{
	stub += "/frames/";

	vector<Image<byte> > r;

	for(unsigned int i=0; i < num.size(); i++)
		r.push_back(img_load(stub + sPrintf("frame_%i.pgm", num[i])));
	
	return r;
}


////////////////////////////////////////////////////////////////////////////////
//
// Fast implementations of the detector
//

/// This struct contains a byte code compiled version of the detector.
/// 
///
/// @ingroup gFastTree
struct fast_detector
{
	/// This is a bytecode element for the bytecode-compiled
	/// detector. The bytecode consists of a number of fixed length 
	/// blocks representing a 3 way branch. Special values of
	/// of a block indicate the result that a pixel is a corner or
	/// non-corner.
	struct fast_detector_bit
	{
		int offset; ///< Memory offset from centre pixel to examine.

		//Root node is 0. If lt == 0, then this is a leaf.
		//gt holds the class.

		int lt; ///<Position in bytecode to branch to if offset pixel is much darker than the centre pixel. If this 
		        ///is zero, then gt stores the result.
		int gt; ///<Position in bytecode to branch to if offset pixel is much brighter than the centre pixel. If lt==0
		        ///is a result block, then this stores the result, 0 for a non corner, 1 for a corner.
		int eq; ///<Position in bytecode to branch to otherwise.
	};

	
	vector<fast_detector_bit> d; ///<This contains the compiled bytecode.

	///Detects a corner at a given pointer, without the book keeping required to compute the score.
	///This is quite a lot faster than @ref detect.
	///
	///@param imp  Pointer at which to detect corner
	///@param b	   FAST barrier
	///@return 	   is a corner or not
	inline bool detect_no_score(const byte* imp, int b) const 
	{
		int n=0;	
		int cb = *imp + b;
		int c_b = *imp - b;
		int p;

		while(d[n].lt)
		{
			p = imp[d[n].offset];

			if(p > cb)
				n = d[n].gt;
			else if(p < c_b)
				n = d[n].lt;
			else
				n = d[n].eq;
		}
		
		return d[n].gt;
	}

	///Detects a corner at a given pointer, with book-keeping required for score computation
	///
	///@param imp  Pointer at which to detect corner
	///@param b	   FAST barrier
	///@return 	   0 for non-corner, minimum increment required to make detector go down different branch, if it is a corner.
	inline int detect(const byte* imp, int b) const
	{
		int n=0;	
		int m = INT_MAX;
		int cb = *imp + b;
		int c_b = *imp - b;
		int p;

		while(d[n].lt)
		{
			p = imp[d[n].offset];

			if(p > cb)
			{
				if(p-cb < m)
					m = p-cb;

				n = d[n].gt;
			}
			else if(p < c_b)
			{
				if(c_b - p < m)
					m = c_b - p;
			
				n = d[n].lt;
			}
			else
				n = d[n].eq;
		}
		
		if(d[n].gt)
			return m;
		else
			return 0;
	}
	
	///Serialize the detector to an ostream. The serialized detector a number of lines
	///of the form:
	///@code
	///Block N [X Y] G E L
	///@endcode
	///or:
	///@code
	///Block N corner
	///@endcode
	///or:
	///@code
	///Block N non_corner
	///@endcode
	///The first block type represents the code:
	///@code
	///if Image[current_pixel + (x, y)] > Image[current_pixel] + threshold
	///   goto block G
	///elseif Image[current_pixel + (x, y)] < Image[current_pixel] -threshold
	///   goto block L
	///else
	///   goto block E
	///endif
	///@endcode
	///@param o 		ostream for output
	///@param width 	width the detector was created at, required to back out the offsets correctly.
	void print(ostream& o, int width) const
	{
		for(unsigned int i=0; i < d.size(); i++)
		{
			if(d[i].lt == 0)
				o << tag::print << "Block" << i <<  (d[i].gt?"corner":"non_corner");
			else
			{
				int a = abs(d[i].offset) + width / 2;
				if(d[i].offset < 0)
					a = -a;
				int y = a / width;

				int x = d[i].offset - y * width;
				o << tag::print << "Block" << i << ImageRef(x , y) << d[i].gt << d[i].eq << d[i].lt;
			}
		}
	}
};


///This struct contains a x86 machine-code compiled version of the detector. The detector
///operates on a single row and inserts offset from the beginning of the image in to a 
///std::vector. 
///@ingroup gFastTree
class jit_detector
{
	public:
		

		///Run the compiled detector on a row of an image.
		///@param im The image.
		///@param row The row to detect corners in.
		///@param xmin The starting position.
		///@param  xmax The ending position.
		///@param corners The detected corners as offsets from image.data().
		///@param threshold The corner detector threshold.
		void detect_in_row(const Image<byte>& im, int row, int xmin, int xmax, vector<int>& corners, int threshold)
		{

			const byte* p = im[row] + xmin;
			const int n = xmax - xmin;
			void* cs = &corners;
			const void* im_data = im.data();
			/* r/m usage, at entry to machine code
				In use:
					%ecx				Num remaining
					%edi 				threshold
					%ebp 			    Detect in row machine code procedure address
					%ebx				cb
					%edx				c_b
					%esi				data
					%eax				Scratch

					4%esp 				%esi: produced automatically by call
					8%esp				image.data()
					12%esp				&vector<int>
					16%esp				vector_inserter: simple function for calling member of std::vector


				Input:	
					0 num remaining
					1 data pointer
					2 threshold
					3 proc 
					4 push_back_proc
					5 vector<int>
					6 image.data()
			*/

			__asm__ __volatile__(
				//Save all registers
				"	pusha								\n"
				
				//Load operands in to correct places
				"	pushl			%4					\n"
				"	pushl			%5					\n"
				"	pushl			%6					\n"
				"	movl			%0, %%ecx			\n"	
				"	movl			%1, %%esi			\n"
				"	movl			%2, %%edi			\n"
				" 	movl			%3, %%ebp			\n"   //%? uses ebp, so trash ebp last

				
				//Start the loop
				"	cmp				$0, %%ecx			\n"
				"	je				1					\n"
				"	call			*%%ebp				\n"
				"1:										\n"


				//Unload operands
				"	popl			%%eax				\n"
				"	popl			%%eax				\n"
				"	popl			%%eax				\n"

				//Restore all registers
				"	popa								\n"
				:
				: "m"(n), "m"(p), "m"(threshold), "m"(proc), "i"(&vector_inserter), "m"(cs), "m"(im_data)
			);


		}


		///Create a compiled detector from the bytecode.
		///@param v Bytecode.
		jit_detector(const vector<fast_detector::fast_detector_bit>& v)
		{
			//blocksize
			const int bs=28;

			length = bs * (v.size() + 2); //Add head and tail block

			/* The original assembler code looked like this
			   This is now done in machine code, with the whole tree in
			   place of  line 0x804e0c1.

			 804e0b3:	83 f9 00             	cmp    $0x0,%ecx
			 804e0b6:	74 1b                	je     804e0d3 <finished>

			0804e0b8 <loop>:
			 804e0b8:	0f b6 16             	movzbl (%esi),%edx
			 804e0bb:	89 d3                	mov    %edx,%ebx
			 804e0bd:	29 fa                	sub    %edi,%edx
			 804e0bf:	01 fb                	add    %edi,%ebx
			 804e0c1:	ff d5                	call   *%ebp
			 804e0c3:	a8 ff                	test   $0xff,%al
			 804e0c5:	74 08                	je     804e0cf <nocorner>
			 804e0c7:	56                   	push   %esi
			 804e0c8:	51                   	push   %ecx
			 804e0c9:	ff 54 24 10          	call   *0x10(%esp)
			 804e0cd:	59                   	pop    %ecx
			 804e0ce:	58                   	pop    %eax

			0804e0cf <nocorner>:
			 804e0cf:	46                   	inc    %esi
			 804e0d0:	49                   	dec    %ecx
			 804e0d1:	75 e5                	jne    804e0b8 <loop>			//jne == jnz

		  	Unused spaces are filled in with int $3, (instruction 0xcc), which
			causes a debug trap. Makes catching errors easier.
			
			The consists of fixed sized blocks pasted together. The size is determined by the
			largest block, which is a tree node. This makes jump computation trivial, but 
			it also means that short jumps are never used, and the code is therefore larger
			than necessary.

			The rest have 0xcc filled in in the spare places. 

			The blocks are templates and have the relevant parts filled in prior to 
			copying.

			Each tree node (including leaves are represented by an entire block)

			Detectod corners are inserted in to a vector<int> as the integer offset of the corner
			pixel from the beginning of the image
			*/

			const unsigned char loop_head[bs] = 
			{
				0xEB, 0x11,							//jmp + 17

				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,		//dead space
				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,
				0xcc,0xcc,0xcc,0xcc,0xcc,


				0x0f, 0xb6, 0x16,					//movzbl (%esi),%edx			Load data
				0x89, 0xd3,                			//mov    %edx,%ebx
				0x29, 0xfa,                			//sub    %edi,%edx				Compute c_b
				0x01, 0xfb,                			//add    %edi,%ebx				Compute cb
			};
			const int loop_head_start=19;			//Jump to here to continue loop


			unsigned char loop_tail[bs] = 
			{
				0x56,								//push %esi				Functions seem to trash this otherwise
				0x51,								//push %ecx				Functions seem to trash this otherwise
				0xFF, 0x54, 0x24, 0x14,				//call *16(%esp)		Other arguments on the stack already
				0x59,								//pop %ecx				Clean stack
				0x58,								//pop %eax				...
				
				0x46,								//inc %esi
				0x49,								//dec %ecx
				0x0F, 0x85, 0xcc, 0xcc, 0xcc, 0xcc,	//jnz <back to first block>

				0xc3, 								//ret
				0xcc,0xcc,0xcc,0xcc,  				//dead space 
				0xcc,0xcc,0xcc,0xcc,
				0xcc,0xcc,0xcc,
			};
			const int loop_tail_address_offset = 12;   //fill in the jump <back to first block> address here
			const int loop_tail_jump_delta     = 16;   //Jump block_size*depth + this, to loop.
			const int loop_tail_entry		   = 8;    //jump to here to avoid inserting current point as corner

			unsigned char cont_or_goto[bs] = 
			{	
				0xE9,0xcc, 0xcc, 0xcc, 0xcc,		//Jump to end of loop
				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc, 		//dead space
				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,
				0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,
				0xcc,0xcc,0xcc,0xcc,0xcc
			};
			const int cont_jmp_addr = 1;			//Jump address filled in here
			const int cont_delta = 5;				//This much in addition to block delta
			
			unsigned char branch[bs] = 
			{
				0x0f, 0xB6, 0x86, 0xcc, 0xcc, 0xcc, 0xcc, 	//movzbl   OOOO(%esi),%eax
				0x39, 0xd8,									//cmp      %ebx, %eax   (eax - ebx) = (data[##]-cb
				0x0F, 0x8F, 0xcc, 0xcc, 0xcc, 0xcc,			//jg       XXXX         jmp by XXXX if eax > ebx
				0x39, 0xC2,									//cmp      %eax, %edx   (edx - eax) = c_b - data[##]
				0x0F, 0x8F, 0xcc, 0xcc, 0xcc, 0xcc,			//jg       YYYY         jmp by YYYY if ecx > ebx
				0xE9, 0xcc, 0xcc, 0xcc, 0xcc,				//jmp	   ZZZZ			Unconditional jump to ZZZZ
			};
			const int block_off_off = 3;
			const int block_gt_off = 11;
			const int block_lt_off = 19;
			const int block_eq_off = 24;


			//mmap a writable, executable block of memory for JITted code
			proc = (unsigned char*) mmap(0, length, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
			if(proc == MAP_FAILED)
			{
				cerr << "mmap failed with error: " << strerror(errno) << endl;
				exit(1);
			}
			
			//Copy in the loop head: no parts to be filled in.
			memcpy(proc, loop_head, bs);

			for(int i=0; i < (int)v.size(); i++)
			{
				if(v[i].lt == 0)			// leaf
				{
					if(v[i].gt == 0) //Fill in jump to continue part
					{
						*(int*)(cont_or_goto + cont_jmp_addr) = bs * (v.size()- i) - cont_delta + loop_tail_entry;
					}
					else //fill in jump to insert part
					{
						*(int*)(cont_or_goto + cont_jmp_addr) = bs * (v.size() - i) - cont_delta;
					}
						

					memcpy(proc + (i+1)*bs, cont_or_goto, bs);
				}
				else
				{
					*(int*)(branch+block_off_off)  = v[i].offset;

					//Optimization: leaf nodes have a non-conditional goto in them
					//so goto the right place directly, rather than the leaf node.
					//This has a 5% effect or so, so bigger gains elsewhere.
					//Removed for simplicity.
					
					*(int*)(branch+block_gt_off) = (v[i].gt -i) * bs - (block_gt_off + 4);
					*(int*)(branch+block_lt_off) = (v[i].lt -i) * bs - (block_lt_off + 4);
					*(int*)(branch+block_eq_off) = (v[i].eq -i) * bs - (block_eq_off + 4);

					memcpy(proc + (i+1)*bs, branch, bs);
				}
			}
			
			//Insert the correct backwards jump for looping
			*(int*)(loop_tail+loop_tail_address_offset) = -bs * (1+v.size()) - loop_tail_jump_delta + loop_head_start;
			memcpy(proc + bs * (v.size() + 1), loop_tail, bs);

		}


		~jit_detector()
		{
			munmap(proc, length);
		}

	private:
		//Not copyable
		void operator=(const jit_detector&);
		jit_detector(const jit_detector&);

		unsigned char* proc;			///< The machine code is stored in this mmap() allocated data which allows code execution.
		int			   length;			///< Number of mmap() allocated bytes.

		///Callback function to allow insertion in to std::vector. The execution of this function
		///relies on the stack having the following layout (stack head on the left):
		///@code
		///return_address first_arg second_arg etc...
		///@endcode
		///so that the arguemnts directly reflect the stack layout.
		///For speed, and in order to minimize stack handling, the argument list spans two call instructions worth of stack.
		///
		///@param ecx_dummy Pushed by the machine code, since the ABI allows ecx to be trashed
		///@param p The pointer to the current pixel. Pushed by the machine code.
		///@param esp_return_dummy Location to return to on a return from the machine code. Generated by the assembler call in to the machine code.
		///@param im_data Pointer to the first image pixel. Pushed by the assembler caller.
		///@param i  Pointer to the std::vector<int> which stores the data. Pushed by the assembler caller.
		static void vector_inserter(int ecx_dummy, const byte* p, const void* esp_return_dummy, const byte* im_data, vector<int>* i)
		{
			i->push_back(p-im_data);
		}
};

/// This struct represents a node of the tree, and has pointers to other 
/// structs, thereby representing a branch or the entire tree.
///
/// @ingroup gTree
class tree_element
{
	public:
		tree_element *lt; ///<Branch of the tree to take if the offset pixel is much darker than the centre.
		tree_element *eq; ///<Branch of the tree to take if the offset pixel is much brighter than the centre.
		tree_element *gt; ///<Branch of the tree to take otherwise.
		bool         is_corner; ///<If the node is a leaf, then this is its attribute.
		int			 offset_index; ///<Offset number of the pixel to examine. This indexes offsets[x]
		

		/// This returns the bounding box of the detector
		pair<ImageRef, ImageRef> bbox() const
		{
			return offsets_bbox;
		}
		
		/// This returns the number of nodes in the tree
		int num_nodes() const
		{
			if(eq == NULL)
				return 1;
			else
				return 1 + lt->num_nodes() + eq->num_nodes() + gt->num_nodes();
		}
		

		///Is the node a leaf?
		bool is_leaf() const
		{
			return eq == NULL;
		}
		

		///Return a given numbered element of the tree. Elements are numbered by depth-first traversal.
		///
		///@param t Element number to return
		///@return pointer to the t'th element, and a flag indicating whether it's the direct child of an eq branch.
		pair<tree_element*,bool> nth_element(int t) 
		{
			//The root node can not be a corner.
			//Otherwise the strength would be inf.
			int n=0;
			return nth_element(t, n, true);
		}
	
		///Compile the detector to bytecode. The bytecode is not a tree, but a graph. This is
		///because the detector is applied in all orientations: offsets are integers which are
		///indices in to a list of (x,y) offsets and there are multiple lists of offsets. The
		///tree is also applied with intensity inversion.
		///
		///@param xsize The width of the image.
		///@return The bytecode compiled detector.
		///@ingroup gFastTree
		fast_detector make_fast_detector(int xsize) const
		{
			vector<fast_detector::fast_detector_bit> f;
			
			for(int invert=0; invert < 2; invert++)
				for(unsigned int i=0; i <  offsets.size(); i++)
				{	
					//Make a FAST detector at a certain orientation
					vector<fast_detector::fast_detector_bit> tmp(1);
					make_fast_detector_o(tmp, 0, xsize, i, invert);

					int endpos = f.size() + tmp.size();
					int startpos = f.size();

					//Append tmp on to f, filling in the non-corners (jumps to endpos)
					//and correcting the intermediate jumps destinations
					for(unsigned int i=0 ; i < tmp.size(); i++)
					{
						f.push_back(tmp[i]);

						if(f.back().eq == -1)
							f.back().eq = endpos;
						else if(f.back().eq > 0)
							f.back().eq += startpos;

						if(f.back().gt == -1)
							f.back().gt = endpos;
						else if(f.back().gt > 0)
							f.back().gt += startpos;

						if(f.back().lt == -1)
							f.back().lt = endpos;
						else if(f.back().lt > 0)
							f.back().lt += startpos;

					}
				}

			//We need a final endpoint for non-corners
			f.resize(f.size() + 1);
			f.back().offset = 0;
			f.back().lt = 0;
			f.back().gt = 0;
			f.back().eq = 0;

			//Now we need an extra endpoint for corners
			for(unsigned int i=0; i < f.size(); i++)
			{
				//EQ is always non-corner
				if(f[i].lt == -2)
					f[i].lt = f.size();
				if(f[i].gt == -2)
					f[i].gt = f.size();
			}

			f.resize(f.size() + 1);
			f.back().offset = 0;
			f.back().lt = 0;
			f.back().gt = 1;
			f.back().eq = 0;

			fast_detector r = {f};
			
			return r;
		}





		private:

		///This compiles the tree in a single orientation and form to bytecode.
		///This is called repeatedly by  make_fast_detector. A jump destination
		///of -1 refers to a non corner and a destination of -2 refers to a 
		///corner.
		///
		///@param v Bytecode storage
		///@param n Position in v to compile the bytecode to
		///@param xsize Width of the image
		///@param N orientation of the tree
		///@param invert whether or not to perform and intensity inversion.
		///@ingroup gFastTree
		void make_fast_detector_o(vector<fast_detector::fast_detector_bit>& v, int n,int xsize, int N, bool invert) const
		{
			//-1 for non-corner
			//-2 for corner

			if(eq == NULL)
			{
				//If the tree is a single leaf, then we end up here. In this case, it must be 
				//a non-corner, otherwise the strength would be inf.
				v[n].offset = 0;
				v[n].lt = -1;
				v[n].gt = -1;
				v[n].eq = -1;
			}
			else
			{
				v[n].offset = offsets[N][offset_index].x + offsets[N][offset_index].y * xsize;

				if(eq->is_leaf())
					v[n].eq = -1; //Can only be non-corner!
				else
				{
					v[n].eq = v.size();
					v.resize(v.size() + 1);
					eq->make_fast_detector_o(v, v[n].eq, xsize, N, invert);
				}

				const tree_element* llt = lt;
				const tree_element* lgt = gt;

				if(invert)
					swap(llt, lgt);

				if(llt->is_leaf())
				{
					v[n].lt = -1 - llt->is_corner;
				}
				else
				{
					v[n].lt = v.size();
					v.resize(v.size() + 1);
					llt->make_fast_detector_o(v, v[n].lt, xsize, N, invert);
				}


				if(lgt->is_leaf())
					v[n].gt = -1 - lgt->is_corner;
				else
				{
					v[n].gt = v.size();
					v.resize(v.size() + 1);
					lgt->make_fast_detector_o(v, v[n].gt, xsize, N, invert);
				}
			}
		}

		///Select the n'th elment of the tree.
		pair<tree_element*, bool> nth_element(int target, int& n, bool eq_branch)
		{
			#ifndef NDEBUG
				if(!( (eq==0 && lt == 0 && gt == 0) || (eq!=0 && lt!=0 &&gt != 0)))
				{
					cout << "Error: corrupted tree\n";
					cout << tag::print << "lt" << lt;
					cout << tag::print << "eq" << eq;
					cout << tag::print << "gt" << gt;
					
					abort();
				}
			#endif

			if(target == n)
				return make_pair(this, eq_branch);
			else
			{
				n++;

				tree_element * r;
				bool e;

				if(eq == 0)
					return make_pair(r=0,eq_branch);
				else
				{
					rpair(r, e) = lt->nth_element(target, n, false);
					if(r != NULL)
						return make_pair(r, e);

					rpair(r, e) = eq->nth_element(target, n, true);
					if(r != NULL)
						return make_pair(r, e);

					return gt->nth_element(target, n, false);
				}
			}
		}
		

		/// Apply the tree to detect a corner in a single form.
		///
		/// @param im Image in which to detecto corners
		/// @param pos position at which to perform detection
		/// @param b Threshold
		/// @param n tree orientation to use (index in to offsets)
		/// @param invert Whether to perform an intensity inversion
		/// @return 0 for no corner, otherwise smallet amount by which a test passed.
		int detect_corner_oriented(const Image<byte>& im, ImageRef pos, int b, int n, bool invert) const
		{
			//Return number that threshold would have to be increased to in
			//order to change the outcome


			if(eq== NULL)
				return is_corner * INT_MAX;
			else
			{	
				int c = im[pos];
				int p = im[pos + offsets[n][offset_index]];

				const tree_element* llt = lt;
				const tree_element* lgt = gt;

				if(invert)
					swap(llt, lgt);


				if(p > c+b)
					return min(p-(c+b), lgt->detect_corner_oriented(im, pos, b, n, invert));
				else if(p < c-b)
					return min((c-b)-p, llt->detect_corner_oriented(im, pos, b, n, invert));
				else
					return eq->detect_corner_oriented(im, pos, b, n, invert);
			}
		}

		public:
		/// Apply the tree in all forms to detect a corner.
		///
		/// @param im Image in which to detecto corners
		/// @param pos position at which to perform detection
		/// @param b Threshold
		/// @return 0 for no corner, otherwise smallet amount by which a test passed.
		int detect_corner(const Image<byte>& im, ImageRef pos, int b) const
		{
			for(int invert=0; invert <2; invert++)
				for(unsigned int i=0; i < offsets.size(); i++)
				{
					int n = detect_corner_oriented(im, pos, b, i, invert);
					if(n)
						return n;
				}
			return 0;
		}

		/// Deep copy the tree.
		tree_element* copy()
		{
			tree_element* t = new tree_element(*this);
			if(eq != NULL)
			{
				t->lt = lt->copy();
				t->gt = gt->copy();
				t->eq = eq->copy();
			}

			return t;
		}

		///Serialize the tree
		///
		///@param o Stream to serialize to.
		///@param ind The indent level to use for the current branch.
		void print(ostream& o, string ind="  ") const
		{


			if(eq == NULL)
				o << ind << tag::print << "Is corner: " << is_corner << this << lt << eq << gt;
			else
			{
				o << ind << tag::print << offset_index << this << lt << eq << gt;
				lt->print(o, ind + "  ");
				eq->print(o, ind + "  ");
				gt->print(o, ind + "  ");
			}
		}
		
		
		~tree_element()
		{	
			delete lt;
			delete eq;
			delete gt;
		}
		
		tree_element(bool b)
		:lt(0),eq(0),gt(0),is_corner(b),offset_index(0)
		{}

		tree_element(tree_element*a, tree_element* b, tree_element* c, int i)
		:lt(a),eq(b),gt(c),is_corner(0),offset_index(i)
		{}
};



/// Generate a random tree.
///
/// @param s Does nothing
/// @param d Depth of tree to generate
/// @param is_eq_branch Whether eq-branch constraints should be applied. This should
///                     always be true when the function is called.
/// @ingroup gTree
tree_element* random_tree(double s, int d, bool is_eq_branch=1)
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
		return new tree_element(random_tree(s, d-1, 0), random_tree(s, d-1, 1), random_tree(s, d-1, 0), rand()%num_offsets );
}




///Detect corners with nonmaximal suppression in an image. This contains a large amount of
///configurable debugging code to verify the correctness of the detector by comparing different
///implementations.
///
///@param im The image to detect corners in.
///@param detector The corner detector.
///@param threshold The detector threshold.
///@param scores This image will be used to store the corner scores for nonmaximal suppression and is
///              the same size as im. It is passed as a parameter since allocation of an image of this
///              size is a significant expense.
vector<ImageRef> detect_corners(const Image<byte>& im, const tree_element* detector, int threshold, Image<int> scores)
{
	cvd_timer tt;
	cvd_timer tmr;
	
	ImageRef tl, br, s;
	rpair(tl,br) = detector->bbox();
	s = im.size();

	int ymin = 1 - tl.y, ymax = s.y - 1 - br.y;
	int xmin = 1 - tl.x, xmax = s.x - 1 - br.x;

	ImageRef pos;
	scores.zero();
	
	vector<int> corners;
	
	tmr.reset();
	fast_detector f2 = detector->make_fast_detector(im.size().x);
	jit_detector jit(f2.d);


	cout << "time_in_det_make_fast_2 " << tmr.get_time() << endl;
	
	tmr.reset();
	for(int y = ymin; y < ymax; y++)
	{
		#ifdef NOJIT
			for(int x=xmin; x < xmax; x++)
				if(f2.detect(&im[y][x], threshold))
					corners.push_back(&im[y][x] - im.data());
		#else
			jit.detect_in_row(im, y, xmin, xmax, corners, threshold);
		#endif
	}

	cout << print << "time_in_det_detect" << tmr.get_time();
	

	if(GV3::get<bool>("debug.verify_detections"))
	{
		//Detect corners using slowest, but most obvious detector, since it's most likely to 
		//be correct.
		vector<ImageRef> t;
		for(pos.y = ymin; pos.y < ymax; pos.y++)
		{
			for(pos.x = xmin; pos.x < xmax; pos.x++)
				if(detector->detect_corner(im, pos, threshold))
					t.push_back(pos);
		}

		//Verify detected corners against this result
		if(t.size() == corners.size())
		{
			for(unsigned int i=0; i < corners.size(); i++)
				if(im.data() + corners[i] != & im[t[i]])
				{
					cerr << "Fatal error: standard and fast detectors do not match!\n";
					cerr << "Same number of corners, but different positions.\n";
					exit(1);
				}
		}
		else
		{
			cerr << "Fatal error: standard and fast detectors do not match!\n";
			cerr << "Different number of corners detected.\n";
			cerr << corners.size() << " " << t.size() << endl;
			exit(1);
		}
	}



	tmr.reset();
	//Compute scores
	for(unsigned int j=0; j < corners.size(); j++)
	{
		int i=threshold + 1;
		while(1)
		{
			int n = f2.detect(im.data() + corners[j], i);
			if(n != 0)
				i += n;
			else
				break;
		}
		scores.data()[corners[j]] = i-1;
	}
	cout << print << "time_in_det_scores" << tmr.get_time();

	if(GV3::get<bool>("debug.verify_scores"))
	{
		//Compute scores using the obvious, but slow recursive implementation.
		//This can be used to test the no obvious FAST implementation and the
		//non obviouser JIT implementation, if it ever exists.
		for(unsigned int j=0; j < corners.size(); j++)
		{
			int i=threshold + 1;
			ImageRef pos =  im.pos(im.data() + corners[j]);
			while(1)
			{
				int n = detector->detect_corner(im, pos, i);
				if(n != 0)
					i += n;
				else
					break;
			}

			if(scores.data()[corners[j]] != i-1)
			{
				cerr << "Fatal error: standard and fast scores do not match!\n";
				cerr << "Different score detected at  " << pos << endl;
				exit(1);
			}
		}
	}

	tmr.reset();

	//Perform non-max suppression the simple way
	vector<ImageRef> nonmax;
	int d = im.size().x;
	for(unsigned int i=0; i < corners.size(); i++)
	{
		int o = corners[i];
		int v = scores.data()[o];

		if( v > *(scores.data() + o + 1    )  &&
		    v > *(scores.data() + o - 1    )  &&
		    v > *(scores.data() + o +d + 1 )  &&
		    v > *(scores.data() + o +d     )  &&
		    v > *(scores.data() + o +d - 1 )  &&
		    v > *(scores.data() + o -d + 1 )  &&
		    v > *(scores.data() + o -d     )  &&
		    v > *(scores.data() + o -d - 1))
		{
			nonmax.push_back(ImageRef(o %d, o/d));
		}
	}
	cout << print << "time_in_det_nonmax" << tmr.get_time();
	cout << print << "time_in_det_total" << tt.get_time();

	return nonmax;
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
	string name = GV3::get<string>("Temperature.schedule");

	if(name == "expo")
	{
		double scale=GV3::get<double>("Temperature.expo.scale");
		double alpha = GV3::get<double>("Temperature.expo.alpha");

		return scale * exp(-alpha * i / imax);
	}
	else
	{
		cerr << "Error, annealing schedule: " << name << " is not defined.\n";
		exit(1);
	}
}




///Generate an optimized corner detector.
///
///@ingroup gOptimize
///@param images The training images
///@param warps  Warps for evaluating the performance on the training images.
///@return An optimized detector.
tree_element* learn_detector(const vector<Image<byte> >& images, const vector<vector<Image<Vector<2> > > >& warps)
{
	unsigned int  iterations=GV3::get<unsigned int>("iterations");
	int threshold = GV3::get<int>("FAST_threshold");
	int fuzz_radius=GV3::get<int>("fuzz");
	double repeatability_scale = GV3::get<double>("repeatability_scale");
	double num_cost	=	GV3::get<double>("num_cost");
	int max_nodes = GV3::get<int>("max_nodes");

	double offset_sigma = GV3::get<double>("offset_sigma");
	


	bool first_time = 1;
	double old_cost = HUGE_VAL;

	ImageRef image_size = images[0].size();

	set<int> debug_triggers = GV3::get<set<int> >("triggers");
	
	//Preallocated space for nonmax-suppression. See detect_corners()
	Image<int> scratch_scores(image_size, 0);

	//Start with an initial random tree
	tree_element* tree = random_tree(offset_sigma, GV3::get<int>("initial_tree_depth"));
	
	for(unsigned int itnum=0; itnum < iterations; itnum++)
	{
		cvd_timer ittmr;
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

			Costs:
				Tree size: should respect operation 4 and reduce node count accordingly
		
		*/

		//Cost:
		//
		//  (1 + (#nodes/max_nodes)^2) * (1 - repeatability)^2 * Sum_{frames} exp(- (fast_9_num-detected_num)^2/2sigma^2)
		// 


		//Copy the new tree and work with the copy.
		tree_element* new_tree = tree->copy();

		cout << "\n\n-------------------------------------\n";
		cout << print << "Iteration" << itnum;

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

			int nnum = rand() % new_tree->num_nodes();

			cout << "Permuting tree at node " << nnum << endl;


			rpair(node, node_is_eq) = new_tree->nth_element(nnum);

			cout << print << "Node" << node << node_is_eq;

			if(node->eq == NULL) //A leaf
			{
				if(rand() % 2 || node_is_eq)  //Operation 1, invariant 1
				{
					cout << "Growing a subtree:\n";
					//Grow a subtree
					tree_element* stub = random_tree(offset_sigma, 1);

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
					int r = rand() % 3; //Remove
					int c;				//Copy
					while((c = rand()%3) == r);

					cout << "Copying branches " << c << " to " << r <<endl;


					tree_element* tmp;

					if(c == 0)
						tmp = node->lt->copy();
					else if(c == 1)
						tmp = node->eq->copy();
					else
						tmp = node->gt->copy();
					
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
	
		
		cvd_timer tmr;
		//Detect all corners
		vector<vector<ImageRef> > detected_corners;
		for(unsigned int i=0; i < images.size(); i++)
			detected_corners.push_back(detect_corners(images[i], new_tree, threshold, scratch_scores));
		cout << print << "time_detecting_corners" << tmr.get_time();


		tmr.reset();
		//Compute repeatability
		double repeatability = compute_repeatability(warps, detected_corners, fuzz_radius, image_size);
		double repeatability_cost = 1 + sq(repeatability_scale/repeatability);
		cout << print << "time_compute_repeatability" << tmr.get_time();

		//Compute cost associated with the total number of detected corners.
		float number_cost=0;
		for(unsigned int i=0; i < detected_corners.size(); i++)
		{
			double cost = sq(detected_corners[i].size() / num_cost);
			cout << print << "Image" << i << detected_corners[i].size()<< cost;
			number_cost += cost;
		}
		number_cost = 1 + number_cost / detected_corners.size();
		cout << print << "Number cost" << number_cost;

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

		
		cout << print << "Temperature" << temperature;
		cout << print << "Number cost" << number_cost;
		cout << print << "Repeatability" << repeatability << repeatability_cost;
		cout << print << "Nodes" << new_tree->num_nodes() << size_cost;
		cout << print << "Cost" << cost;
		cout << print << "Old cost" << old_cost;
		cout << print << "Liklihood" << liklihood;
		

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
		
		cout << print << "Final cost" << old_cost;

		cout << print << "time_iteration" << ittmr.get_time();
	}


	return tree;
}


///Rotate a vector<ImageRef> by a given angle, with an optional reflection.
///@param offsets Offsets to rotate.
///@param angle Angle to rotate by.
///@param r Whether to reflect.
///@return The rotated offsets.
///@ingroup gTree
vector<ImageRef> transform_offsets(const vector<ImageRef>& offsets, int angle, bool r)
{
	double a = angle * M_PI / 2;	

	double R_[] = { cos(a), sin(a), -sin(a) , cos(a) };
	double F_[] = { 1, 0, 0, r?-1:1};

	Matrix<2> R(R_), F(F_);
	Matrix<2> T = R*F;

	vector<ImageRef> ret;

	for(unsigned int i=0; i < offsets.size(); i++)
	{
		Vector<2> v = vec(offsets[i]);
		ret.push_back(ir_rounded(T * v));
	}
	
	return ret;
}


///Pretty print some offsets to stdout.
///@param offsets List of offsets to pretty-print.
///@ingroup gUtility
void draw_offset_list(const vector<ImageRef>& offsets)
{

	cout << "Allowed offsets: " << offsets.size() << endl;

	ImageRef min, max;
	min.x = *min_element(member_iterator(offsets.begin(), &ImageRef::x), member_iterator(offsets.end(), &ImageRef::x));
	max.x = *max_element(member_iterator(offsets.begin(), &ImageRef::x), member_iterator(offsets.end(), &ImageRef::x));
	min.y = *min_element(member_iterator(offsets.begin(), &ImageRef::y), member_iterator(offsets.end(), &ImageRef::y));
	max.y = *max_element(member_iterator(offsets.begin(), &ImageRef::y), member_iterator(offsets.end(), &ImageRef::y));

	cout << print << min <<max << endl;

	Image<int> o(max-min+ImageRef(1,1), -1);
	for(unsigned int i=0; i <offsets.size(); i++)
		o[offsets[i] -min] = i;

	for(int y=0; y < o.size().y; y++)
	{
		for(int x=0; x < o.size().x; x++)
			cout << "+------";
		cout << "+"<< endl;

		for(int x=0; x < o.size().x; x++)
			cout << "|      ";
		cout << "|"<< endl;


		for(int x=0; x < o.size().x; x++)
		{
			if(o[y][x] >= 0)
				cout << "|  " << setw(2) << o[y][x] << "  ";
			else if(ImageRef(x, y) == o.size() / 2)
				cout << "|   " << "#" << "  ";
			else 
				cout << "|      ";
		}
		cout <<  "|" << endl;

		for(int x=0; x < o.size().x; x++)
			cout << "|      ";
		cout << "|"<< endl;
	}

	for(int x=0; x < o.size().x; x++)
		cout << "+------";
	cout << "+"<< endl;

	cout << endl;

}


//Generate a detector, and compute its repeatability for all the tests.
//
//@param argc Number of command line arguments
//@ingroup gRepeatability
void mmain(int argc, char** argv)
{
	GUI.LoadFile("learn_detector.cfg");
	GUI.parseArguments(argc, argv);
	
	//This will abort if the annealing schedule is misconfigured.
	//What an ugly way to do this.
	compute_temperature(1,1);

	string dir=GV3::get<string>("repeatability_dataset.directory");
	vector<int> nums=GV3::get<vector<int> >("repeatability_dataset.examples");

	if(GV3::get<int>("random_seed") != -1)
		srand(GV3::get<int>("random_seed"));


	//Pixel offsets are represented as integer indices in to an array of
	//ImageRefs. That means that by choosing the array, the tree can be
	//rotated and/or reflected. Here, an annulus of possible offsets is 
	//created and rotated by all multiples of 90 degrees, and then reflected.
	//This gives a total of 8.
	offsets.resize(8);
	{	
		double min_r = GV3::get<double>("offsets.min_radius");
		double max_r = GV3::get<double>("offsets.max_radius");

		ImageRef max((int)ceil(max_r+1), (int)ceil(max_r+1));
		ImageRef min = -max, p = min;

		cout << "Offsets: ";

		do
		{
			double d = vec(p) * vec(p);

			if(d >= min_r*min_r && d <= max_r * max_r)
			{
				offsets[0].push_back(p);
				cout << offsets[0].back() << " ";
			}
		}
		while(p.next(min, max));

		cout << endl;

		offsets_bbox = make_pair(min, max);
	}
	offsets[1] = transform_offsets(offsets[0], 1, 0);
	offsets[2] = transform_offsets(offsets[0], 2, 0);
	offsets[3] = transform_offsets(offsets[0], 3, 0);
	offsets[4] = transform_offsets(offsets[0], 0, 1);
	offsets[5] = transform_offsets(offsets[0], 1, 1);
	offsets[6] = transform_offsets(offsets[0], 2, 1);
	offsets[7] = transform_offsets(offsets[0], 3, 1);
	num_offsets=offsets[0].size();


	//Print the offsets out.	
	for(unsigned int i=0; i < 8; i++)
	{
		cout << "Offsets " << i << endl;
		draw_offset_list(offsets[i]);	
		cout << endl;
	}

	vector<Image<byte> > images = load_images(dir, nums);
	vector<vector<Image<Vector<2> > > > warps = load_warps(dir, nums, images.at(0).size());
	tree_element* tree = learn_detector(images, warps);

	cout << "Final tree is:" << endl;
	tree->print(cout);
	cout << endl;

	cout << "Final block detector is:" << endl;
	{
		fast_detector f = tree->make_fast_detector(9999);
		f.print(cout, 9999);
	}

	images.clear();
	warps.clear();

	vector<unsigned int> target_corners = GV3::get<vector<unsigned int> >("eval.corners_per_frame");
	int fuzz_radius=GV3::get<int>("fuzz");
	
	//Use this to make sure that the repeatability evaluation is 
	//the same as the original system.
	bool use_fast_9=GV3::get<bool>("eval.debug.use_fast_9");
	
	//Now output the repeatability graphs
	for(int i=0; i < GV3::get<int>("eval.datasets"); i++)
	{
		string dir = GV3::get<string>(sPrintf("eval.%i.dataset", i));
		int num = GV3::get<int>(sPrintf("eval.%i.examples", i));
		string out_tag = GV3::get<string>(sPrintf("eval.%i.tag", i));

		vector<Image<byte> > images = load_images(dir, range(num));
		vector<vector<Image<Vector<2> > > > warps = load_warps(dir, range(num), images.at(0).size(), 0);
		
		Image<int> scratch_scores(images.at(0).size(), 0);
		

		//The thresholds to get as close to the correct number of corners as possible
		//for each image. The corners per frame must monotonically increase, so the
		//thresholds need only monotonically decrease
		vector<int> thresholds(num, 255);
		for(unsigned int i=0; i < target_corners.size(); i++)
		{
			vector<vector<ImageRef> > corners;
			int num_corners=0;

			
			unsigned int target = target_corners[i];

			//Detect corners
			for(unsigned int j=0; j < images.size(); j++)
			{	
				vector<ImageRef> c;
				vector<ImageRef> prev_c;

				//Try to get as close to target as possible
				for(;;)
				{
					if(use_fast_9)
					{
						vector<ImageRef> c1;
						fast_corner_detect_9(images[j], c1, thresholds[j]);
						fast_nonmax(images[j], c1, thresholds[j], c);
					}
					else
						c = detect_corners(images[j], tree, thresholds[j], scratch_scores);

					if(c.size() ==  target)
					{
						cout << add_fill << "Image" << j << "target" << target << c.size();
						break;
					}

					if(c.size() < target)
						thresholds[j]--;
					else if(c.size() > target)
					{
						//We've overstepped and detected too many corners.
						//We might get here first time. That means that this threshold gave
						//closer to the previous number than a higher threshold. Since the numbers
						//increase, that means that a smaller threshold could not be better.

						cout << add_fill << "Image" << j << "target" << target << c.size() << prev_c.size();
						
						//Who is closer?
						if(c.size() - target > target - prev_c.size())
						{
							c = prev_c;
							thresholds[j]++;
						}

						cout << " " << c.size() << " ";

						break;

					}
					prev_c = c;
				}

				cout << c.size() << endl;

				corners.push_back(c);
				num_corners+=corners.back().size();
			}

			double r = compute_repeatability_exact(warps, corners,fuzz_radius);

			cout << print << out_tag << num_corners * 1.0 / images.size() << r;
		}
	}
	

	//Noise performance
	{
		int i = GV3::get<int>("noise.eval_set");	
		string out_tag = GV3::get<string>(sPrintf("noise.tag"));

		string dir = GV3::get<string>(sPrintf("eval.%i.dataset", i));
		int num = GV3::get<int>(sPrintf("eval.%i.examples", i));


		vector<Image<byte> > images = load_images(dir, range(num));
		vector<vector<Image<Vector<2> > > > warps = load_warps(dir, range(num), images.at(0).size(), 0);

		vector<double> sigmas;
		for(double d=0; d < GV3::get<double>("noise.max_sig"); d+=GV3::get<double>("noise.sig_step"))
			sigmas.push_back(d);

		unsigned int corners_per_frame = GV3::get<unsigned int>("noise.corners");

		Image<int> scratch_scores(images.at(0).size(), 0);

		for(unsigned int i=0; i < sigmas.size(); i++)
		{
			vector<vector<ImageRef> > corners;
			double s = sigmas[i];

			for(unsigned int j = 0; j < images.size(); j++)
			{
				Image<byte> noisy_image = images[j].copy_from_me();

				//Add noise to images
				for(Image<byte>::iterator k = noisy_image.begin(); k != noisy_image.end(); k++)
				{
					double v = floor(*k + rand_g()*s + 0.5);
					
					if(v > 255)
						*k = 255;
					else if(v < 0)
						*k = 0;
					else
						*k = static_cast<byte>(v);
				}


				int t_l = 0, t_h = 255, t = 128;
				vector<ImageRef> c;


				cout << "Starting search " << i << ": " << flush;
				//Find threshold by binary search.
				do
				{
					t = (t_h + t_l) / 2;

					//Detect corners
					if(use_fast_9)
					{
						vector<ImageRef> c1;
						fast_corner_detect_9(noisy_image, c1, t);
						fast_nonmax(noisy_image, c1, t, c);
					}
					else
						c = detect_corners(noisy_image, tree, t, scratch_scores);

					
					if(c.size() > corners_per_frame)
						t_l = t;
					else if(c.size() < corners_per_frame)
						t_h = t;
					
					cout << " [" << t_l << ", " << t_h << ", " << c.size() << "]" << flush;

				}while(c.size() != corners_per_frame && t_l < t_h-1);

				corners.push_back(c);

				cout << endl;


			}
			double r = compute_repeatability_exact(warps, corners,fuzz_radius);
			cout << print << out_tag << s << r;
		}
	}

	GUI.ParseLine("gvarlist");

}


int main(int argc, char** argv)
{
	try
	{	
		mmain(argc, argv);
	}
	catch(Exceptions::All w)
	{	
		cerr << "Error: " << w.what << endl;
	}
}







