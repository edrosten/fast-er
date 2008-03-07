#ifndef INC_FASTER_TREE_H
#define INC_FASTER_TREE_H

#include <iostream>
#include <string>
#include <utility>

#include <cvd/image.h>
#include <cvd/byte.h>

#include <tag/stdpp.h>

#include "offsets.h"
#include "faster_bytecode.h"

////////////////////////////////////////////////////////////////////////////////
//
// Fast implementations of the detector
//

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
		std::pair<CVD::ImageRef, CVD::ImageRef> bbox() const
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
		std::pair<tree_element*,bool> nth_element(int t) 
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
		block_bytecode make_fast_detector(int xsize) const
		{
			std::vector<block_bytecode::fast_detector_bit> f;
			
			for(int invert=0; invert < 2; invert++)
				for(unsigned int i=0; i <  offsets.size(); i++)
				{	
					//Make a FAST detector at a certain orientation
					std::vector<block_bytecode::fast_detector_bit> tmp(1);
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

			block_bytecode r = {f};
			
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
		void make_fast_detector_o(std::vector<block_bytecode::fast_detector_bit>& v, int n,int xsize, int N, bool invert) const
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
					std::swap(llt, lgt);

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
		std::pair<tree_element*, bool> nth_element(int target, int& n, bool eq_branch)
		{
			using tag::operator<<;
			#ifndef NDEBUG
				if(!( (eq==0 && lt == 0 && gt == 0) || (eq!=0 && lt!=0 &&gt != 0)))
				{
					std::clog << "Error: corrupted tree\n";
					std::clog << tag::print << "lt" << lt;
					std::clog << tag::print << "eq" << eq;
					std::clog << tag::print << "gt" << gt;
					
					abort();
				}
			#endif

			if(target == n)
				return std::make_pair(this, eq_branch);
			else
			{
				n++;

				tree_element * r;
				bool e;

				if(eq == 0)
					return std::make_pair(r=0,eq_branch);
				else
				{
					tag::rpair(r, e) = lt->nth_element(target, n, false);
					if(r != NULL)
						return std::make_pair(r, e);

					tag::rpair(r, e) = eq->nth_element(target, n, true);
					if(r != NULL)
						return std::make_pair(r, e);

					return gt->nth_element(target, n, false);
				}
			}
		}
		

		/// Apply the tree to detect a corner in a single form.
		///
		/// @param im CVD::Image in which to detecto corners
		/// @param pos position at which to perform detection
		/// @param b Threshold
		/// @param n tree orientation to use (index in to offsets)
		/// @param invert Whether to perform an intensity inversion
		/// @return 0 for no corner, otherwise smallet amount by which a test passed.
		int detect_corner_oriented(const CVD::Image<CVD::byte>& im, CVD::ImageRef pos, int b, int n, bool invert) const
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
					std::swap(llt, lgt);


				if(p > c+b)
					return std::min(p-(c+b), lgt->detect_corner_oriented(im, pos, b, n, invert));
				else if(p < c-b)
					return std::min((c-b)-p, llt->detect_corner_oriented(im, pos, b, n, invert));
				else
					return eq->detect_corner_oriented(im, pos, b, n, invert);
			}
		}

		public:
		/// Apply the tree in all forms to detect a corner.
		///
		/// @param im CVD::Image in which to detecto corners
		/// @param pos position at which to perform detection
		/// @param b Threshold
		/// @return 0 for no corner, otherwise smallet amount by which a test passed.
		int detect_corner(const CVD::Image<CVD::byte>& im, CVD::ImageRef pos, int b) const
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
		void print(std::ostream& o, std::string ind="  ") const
		{
			using tag::operator<<;


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


std::vector<CVD::ImageRef> tree_detect_corners(const CVD::Image<CVD::byte>& im, const tree_element* detector, int threshold, CVD::Image<int> scores);

#endif
