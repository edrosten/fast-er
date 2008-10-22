#ifndef FASTER_BYTECODE_H
#define FASTER_BYTECODE_H

#include <vector>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <cvd/byte.h>
#include <cvd/image.h>
#include <tag/stdpp.h>

/// This struct contains a byte code compiled version of the detector.
/// 
///
/// @ingroup gFastTree
struct block_bytecode
{

	/// This is a bytecode element for the bytecode-compiled
	/// detector. The bytecode consists of a number of fixed length 
	/// blocks representing a 3 way branch. Special values of
	/// of a block indicate the result that a pixel is a corner or
	/// non-corner.
	///
	/// Specifically, if <code>lt == 0</code>, then this is a leaf and \c gt holds the class.
	/// The root node is always stored as the first bytecode instruction.

	/// @ingroup gFastTree
	struct fast_detector_bit
	{
		int offset; ///< Memory offset from centre pixel to examine. This means that the fast 
		            ///detector must be created for an image of a known width.

		//Root node is 0. If lt == 0, then this is a leaf.
		//gt holds the class.

		int lt; ///<Position in bytecode to branch to if offset pixel is much darker than the centre pixel. If this 
		        ///is zero, then gt stores the result.
		int gt; ///<Position in bytecode to branch to if offset pixel is much brighter than the centre pixel. If lt==0
		        ///is a result block, then this stores the result, 0 for a non corner, 1 for a corner.
		int eq; ///<Position in bytecode to branch to otherwise.
	};

	
	std::vector<fast_detector_bit> d; ///<This contains the compiled bytecode.

	///Detects a corner at a given pointer, without the book keeping required to compute the score.
	///This is quite a lot faster than @ref detect.
	///
	///@param imp  Pointer at which to detect corner
	///@param b	   FAST barrier
	///@return 	   is a corner or not
	inline bool detect_no_score(const CVD::byte* imp, int b) const 
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
	inline int detect(const CVD::byte* imp, int b) const
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
	void print(std::ostream& o, int width) const
	{
		using tag::operator<<;
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
				o << tag::print << "Block" << i << CVD::ImageRef(x , y) << d[i].gt << d[i].eq << d[i].lt;
			}
		}
	}

	void detect(const CVD::Image<CVD::byte>& im, std::vector<int>& corners, int threshold, int xmin, int xmax, int ymin, int ymax);
};

#endif
