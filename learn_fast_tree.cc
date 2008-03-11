/**
	\file learn_fast_tree.cc Build a FAST tree detector.

*/

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <list>
#include <map>
#include <vector>
#include <cassert>
#include <bitset>
#include <algorithm>
#include <string>
#include <tr1/memory>

#include <stdint.h>

#include <tag/tuple.h>
#include <tag/printf.h>
#include <tag/stdpp.h>
#include <tag/fn.h>

#include <cvd/image_ref.h>

#include <gvars3/instances.h>

using namespace std;
using namespace tr1;
using namespace tag;
using namespace CVD;
using namespace GVars3;

///Representations of ternary digits.
enum Ternary
{
	Brighter='b',
	Darker  ='d',
	Similar ='s'
};

////////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//


#define fatal(E, S, ...) vfatal((E), (S), (tag::Fmt,## __VA_ARGS__))
template<class C> void vfatal(int err, const string& s, const C& list)
{
	vfPrintf(cerr, s + "\n", list);
	exit(err);
}

///Simple and inefficient function to replace all instances of
///a pattern in a string with the specified text.
///@param ss String in which to perform search and replace
///@param pattern String to search for
///@param text Replacement text.
///@return The string with the patterns replaced.
///@ingroup gUtility
string replace_all(const string& ss, const string& pattern, const string& text)
{
	string s = ss;
	for(;;)
	{
		string::size_type pos = s.find(pattern);

		if(pos == string::npos)
			return s;
		else
			s = s.substr(0, pos) + text + s.substr(pos + pattern.size());
	}
}

///Simple function to replace the first instance of
///a pattern in a string with the specified text.
///@param s String in which to perform search and replace
///@param pattern String to search for
///@param text Replacement text.
///@return The string with pattern replaced.
///@ingroup gUtility
string replace_1st(const string& s, const string& pattern, const string& text)
{
	string::size_type pos = s.find(pattern);

	if(pos == string::npos)
		return s;

	return s.substr(0, pos) + text + s.substr(pos + pattern.size());
}

///Serializes an object to a string using the default operator<<
///@param c object to serialize
///@return serialized object
///@ingroup gUtility
template<class C> string xtoa(const C& c)
{
	ostringstream o;
	o << c;
	return o.str();
}

/**This structure represents a datapoint. A datapoint is a group of pixels with
ternary values (much brighter than the centre, much darker than the centre or
similar to the centre pixel). In addition to the feature descriptor, the class
and number of instances is also stored.

The maximum feature vector size is determined by the template parameter. This
allows the ternary vector to be stored in a bitset. This keeps the struct a
fixed size and removes the need for dynamic allocation.

@ingroup gFastTree
*/
template<int FEATURE_SIZE> struct datapoint
{
	///Construct a datapoint 
	///@param s The feature vector in string form 
	///@param c The number of instances
	///@param is The class
	datapoint(const string& s, unsigned long c, bool is)
	:count(c),is_a_corner(is)
	{
		pack_trits(s);
	}
	
	///Default constructor allows for storage in a
	///std::vector.
	datapoint()
	{}

	unsigned long count; ///< Number of instances
	bool is_a_corner;   ///< Class

	static const unsigned int max_size = FEATURE_SIZE; ///< Maximum number of features representable.
	

	///Extract a trit (ternary bit) from the feture vector.
	///@param tnum Number of the bit to extract
	///@return  The trit.
	Ternary get_trit(unsigned int tnum) const
	{
		assert(tnum < size);
		if(tests[tnum] == 1)
			return Brighter;
		else if(tests[tnum + max_size] == 1)
			return Darker;
		else
			return Similar;
	}

	private:

		bitset<max_size*2> tests; ///<Used to store the ternary vector
		                          ///Ternary bits are stored using 3 out of the
								  ///4 values storable by two bits.
								  ///Trit \e n is stored using the bits \e n and
								  ///\e n + \e max_size, with bit \e n being the
								  ///most significant bit.
								  ///
								  ///The values are
								  ///- 3 unused
								  ///- 2 Brighter
								  ///- 1 Darker
								  ///- 0 Similar

		///This code reads a stringified representation of the feature vector
		///and converts it in to the internal representation. 
		///The string represents one feature per character, using "b", "d" and
		//"s".
		///@param unpacked String to parse.
		void pack_trits(const string& unpacked)
		{
			tests = 0;
			for(unsigned int i=0;i < unpacked.size(); i++)
			{
				if(unpacked[i] == 'b')
					set_trit(i, Brighter);
				else if(unpacked[i] == 'd')
					set_trit(i, Darker);
				else if(unpacked[i] == 's')
					set_trit(i, Similar);
				else
					fatal(2, "Bad char while packing datapoint: %s", unpacked);
			}
		}
		
		///Set a ternary digit.
		///@param tnum Digit to set
		///@param val Value to set it to.
		void set_trit(unsigned int tnum, Ternary val)
		{
			assert(val == Brighter || val == Darker || val == Similar);
			assert(tnum < max_size);

			if(val == Brighter)
				tests[tnum] = 1;
			else if(val == Darker)
				tests[tnum + max_size] = 1;
		}
};

/**
This function loads as many datapoints from the standard input as 
possible. Datapoints consist of a feature vector (a string containing the
characters "b", "d" and "s"), a number of instances and a class.

See datapoint::pack_trits for a more complete description of the feature vector.

The tokens are whitespace separated.

@param nfeats Number of features in a feature vector. Used to spot errors.
@return Loaded datapoints and total number of instances.
@ingroup gFastTree
*/
template<int S> typename V_tuple<shared_ptr<vector<datapoint<S> > >, uint64_t >::type load_features(int nfeats)
{
	shared_ptr<vector<datapoint<S> > > ret(new vector<datapoint<S> >);


	string unpacked_feature;
	
	uint64_t total_num = 0;

	for(;;)
	{
		uint64_t count;
		bool is;

		cin >> unpacked_feature >> count >> is;

		if(!cin)
			break;

		if(unpacked_feature.size() != nfeats)
			fatal(1, "Feature string length is %i, not %i", unpacked_feature.size(), nfeats);

		if(count == 0)
			fatal(4, "Zero count is invalid");

		ret->push_back(datapoint<S>(unpacked_feature, count, is));

		total_num += count;
	}

	cerr << "Num features: " << total_num << endl
	     << "Num distinct: " << ret->size() << endl;

	return make_vtuple(ret, total_num);
}


///Compute the entropy of a set with binary annotations.
///@param n Number of elements in the set
///@param c1 Number of elements in class 1
///@return The set entropy.
///@ingroup gFastTree
double entropy(uint64_t n, uint64_t c1)
{
	assert(c1 <= n);
	//n is total number, c1 in num in class 1
	if(n == 0)
		return 0;
	else if(c1 == 0 || c1 == n)
		return 0;
	else
	{
		double p1 = (double)c1 / n;
		double p2 = 1-p1;

		return -(double)n*(p1*log(p1) + p2*log(p2)) / log(2.f);
	}
}


template<int S> int find_best_split(const vector<datapoint<S> >& fs, const vector<double>& weights, const string& ind, int nfeats)
{
	unsigned long long num_total = 0, num_corners=0;

	for(typename vector<datapoint<S> >::const_iterator i=fs.begin(); i != fs.end(); i++)
	{
		num_total += i->count;
		if(i->is_a_corner)
			num_corners += i->count;
	}

	double total_entropy = entropy(num_total, num_corners);
	
	double biggest_delta = 0;
	int   feature_num = -1;

	for(unsigned int i=0; i < nfeats; i++)
	{
		uint64_t num_bri = 0, num_dar = 0, num_sim = 0;
		uint64_t cor_bri = 0, cor_dar = 0, cor_sim = 0;



		for(typename vector<datapoint<S> >::const_iterator f=fs.begin(); f != fs.end(); f++)
		{
			switch(f->get_trit(i))
			{
				case Brighter:
					num_bri += f->count;
					if(f->is_a_corner)
						cor_bri += f->count;
					break;

				case Darker:
					num_dar += f->count;
					if(f->is_a_corner)
						cor_dar += f->count;
					break;

				case Similar:
					num_sim += f->count;
					if(f->is_a_corner)
						cor_sim += f->count;
					break;
			}
		}

		double delta_e = total_entropy - (entropy(num_bri, cor_bri) + entropy(num_dar, cor_dar) + entropy(num_sim, cor_sim));

		delta_e *= weights[i];

		if(delta_e > biggest_delta)
		{		
			biggest_delta = delta_e;
			feature_num = i;
		}	
	}

	if(feature_num == -1)
		fatal(3, "Couldn't find a split.");

	return feature_num;
}


////////////////////////////////////////////////////////////////////////////////
//
// Tree buliding
//

struct tree{
	enum IsCorner
	{
		Corner,
		NonCorner,
		NonTerminal
	};

	shared_ptr<tree> brighter, darker, similar;
	IsCorner is_a_corner;
	int feature_to_test;
	uint64_t num_tests;


	string stringify()
	{
		ostringstream o;
		stringify(o);
		return o.str();
	}


	static shared_ptr<tree> CornerLeaf()
	{
		return shared_ptr<tree>(new tree(Corner));
	}

	static shared_ptr<tree> NonCornerLeaf()
	{
		return shared_ptr<tree>(new tree(NonCorner));
	}

	tree(shared_ptr<tree> b, shared_ptr<tree> d, shared_ptr<tree> s, int n, uint64_t num)
	:brighter(b), darker(d), similar(s), is_a_corner(NonTerminal), feature_to_test(n), num_tests(num)
	{}

	private:
	tree(IsCorner c)
	:is_a_corner(c),feature_to_test(-1),num_tests(0)
	{}

	void stringify(ostream& o)
	{
		o << "(";
		if(is_a_corner == NonTerminal)
		{
			o << feature_to_test;
			brighter->stringify(o);
			darker->stringify(o);
			similar->stringify(o);
		}
		else
			o << "(" << (is_a_corner == Corner) << ")";

		o << ")";
	}
};



template<int S> pair<shared_ptr<tree>, uint64_t> build_tree(vector<datapoint<S> >& corners, const vector<double>& weights, int nfeats, string ind="")
{
	ind += " ";

	//Find the split
	int f = find_best_split<S>(corners, weights, ind, nfeats);

	//Perform the split
	vector<datapoint<S> > brighter, darker, similar;
	uint64_t num_bri=0, cor_bri=0, num_dar=0, cor_dar=0, num_sim=0, cor_sim=0;

	while(!corners.empty())
	{
			switch(corners.back().get_trit(f))
			{
				case Brighter:
					brighter.push_back(corners.back());
					num_bri += corners.back().count;
					if(corners.back().is_a_corner)
						cor_bri += corners.back().count;
					break;

				case Darker:
					darker.push_back(corners.back());
					num_dar += corners.back().count;
					if(corners.back().is_a_corner)
						cor_dar += corners.back().count;
					break;

				case Similar:
					similar.push_back(corners.back());
					num_sim += corners.back().count;
					if(corners.back().is_a_corner)
						cor_sim += corners.back().count;
					break;
			}
		
		corners.resize(corners.size() -1);
	}
	
	uint64_t num_tests =  num_bri + num_dar + num_sim;

	
	uint64_t bri_tests = 0, dar_tests = 0, sim_tests = 0;

	//Build the subtrees
	shared_ptr<tree> b_tree, d_tree, s_tree;

	if(cor_bri == 0)
		b_tree = tree::NonCornerLeaf();
	else if(cor_bri == num_bri)
		b_tree = tree::CornerLeaf();
	else
		rpair(b_tree, bri_tests) = build_tree<S>(brighter, weights, nfeats, ind);
	

	if(cor_dar == 0)
		d_tree = tree::NonCornerLeaf();
	else if(cor_dar == num_dar)
		d_tree = tree::CornerLeaf();
	else
		rpair(d_tree, dar_tests) = build_tree<S>(darker, weights, nfeats, ind);


	if(cor_sim == 0)
		s_tree = tree::NonCornerLeaf();
	else if(cor_sim == num_sim)
		s_tree = tree::CornerLeaf();
	else
		rpair(s_tree, sim_tests) = build_tree<S>(similar, weights, nfeats, ind);

	return make_pair(shared_ptr<tree>(new tree(b_tree, d_tree, s_tree, f, num_tests)), bri_tests + dar_tests + sim_tests + num_tests);
}

#if 0
////////////////////////////////////////////////////////////////////////////////
//
// Tree visiting
//
string load_file(const string& name)
{
	string file;
	ifstream f;
	f.open(name.c_str());
	getline(f, file, '\xff');
	return file;
}


class print_code
{
	public:
		virtual std::string print_if(const std::string& test) const=0;
		virtual std::string print_elseif(const std::string& test) const=0;
		virtual std::string print_else() const=0;
		virtual std::string print_endif() const=0;
		virtual std::string print_non_corner() const=0;
		virtual std::string print_corner() const=0;
		virtual std::string brighter_test(int pixel) const=0;
		virtual std::string darker_test(int pixel) const=0;
		virtual std::string both_tests(int pixel) const=0;
		virtual ~print_code(){}
		
		print_code(const tree* t, const vector<ImageRef>& o)
		:node(t), offsets(o)
		{}


	protected:
		void visit_tree(const tree* node, const std::string& i, ostream& o) const;
		const tree* node;
		vector<ImageRef> offsets;

	private:
		
		void indent(ostream& o, const string&s, const string& i) const
		{
			if(s == "")
				return;

			o << i;

			for(unsigned int n=0; n < s.size(); n++)
			{
				if(s[n] == '\n')
					o << endl << i;
				else
					o << s[n];
			}
			o << endl;
		}
};


void print_code::visit_tree(const tree* node, const string& i, ostream& o) const
{
	if(node->is_a_corner == tree::Corner)
		indent(o, print_corner(), i);
	else if(node->is_a_corner == tree::NonCorner)
		indent(o, print_non_corner(), i);
	else
	{
		string b = node->brighter->stringify();
		string d = node->darker->stringify();
		string s = node->similar->stringify();

		//b = "A";
		//d = "B";
		//s = "C";

		const tree * bt = node->brighter.get();
		const tree * dt = node->darker.get();
		const tree * st = node->similar.get();
		string ii = i + " ";

		int f = node->feature_to_test;
	
		string ef = print_endif();

		if(b == d && d == s) //All the same
		{
			//o << i << "if " << f << " is whatever\n";
			visit_tree(st, i, o);
		}
		else if(d == s)  //Bright is different
		{
			indent(o, print_if(brighter_test(f)), i);
			visit_tree(bt, ii, o);
			indent(o, print_else(), i);
			visit_tree(st, ii, o);
			indent(o, ef, ii);

		}
		else if(b == s)	//Dark is different
		{	
			indent(o, print_if(darker_test(f)), i);
			visit_tree(dt, ii, o);
			indent(o, print_else(), i);
			visit_tree(st, ii, o);
			indent(o, ef, ii);
		}
		else if(b == d) //Similar is different
		{
			indent(o, print_if(both_tests(f)), i);
			visit_tree(bt, ii, o);
			indent(o, print_else(), i);
			visit_tree(st, ii, o);
			indent(o, ef, ii);
		}
		else //All different
		{
			indent(o, print_if(brighter_test(f)), i);
			visit_tree(bt, ii, o);
			indent(o, print_elseif(darker_test(f)), i);
			visit_tree(dt, ii, o);
			indent(o, print_else(), i);
			visit_tree(st, ii, o);
			indent(o, ef, ii);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

class C_print_with_scores_bsearch: public print_code
{
	public:
		virtual std::string print_if(const std::string& test)const { return "if(" + test + ")";}
		virtual std::string print_elseif(const std::string& test) const { return "else if(" + test + ")";}
		virtual std::string print_else()const {return "else";}
		virtual std::string print_endif()const {return "";}
		virtual std::string print_non_corner() const { return "goto non_corner;";}
		virtual std::string print_corner() const {return "goto corner;";}

		virtual std::string brighter_test(int pixel) const {return "*(cache_0 + offset[" + xtoa(pixel) + "]) >  cb";}
		virtual std::string darker_test(int pixel) const {return "*(cache_0 + offset[" + xtoa(pixel) + "]) <  c_b";}
		virtual std::string both_tests(int pixel) const { return brighter_test(pixel) + "||" + darker_test(pixel);}
		C_print_with_scores_bsearch(const tree* t, const vector<ImageRef>& o, ostream& oo)
		:print_code(t, o)
		{
			visit_tree(node, "		", oo);
		}

};

class C_print_with_scores: public print_code
{
	public:
		virtual std::string print_if(const std::string& test)const { return "if(" + test + ")";}
		virtual std::string print_elseif(const std::string& test) const { return "else if(" + test + ")";}
		virtual std::string print_else()const {return "else";}
		virtual std::string print_endif()const {return "";}
		virtual std::string print_non_corner() const { return "break;";}
		virtual std::string print_corner() const {return "b += min_diff;";}
		//virtual std::string print_non_corner() const { return "return 0;";}
		//virtual std::string print_corner() const {return "return min_diff;";}


		virtual std::string brighter_test(int pixel) const {return "test_gt_set(*(cache_0 + offset[" + xtoa(pixel) + "]), cb, min_diff)";}
		virtual std::string darker_test(int pixel) const {return "test_gt_set(c_b, *(cache_0 + offset[" + xtoa(pixel) + "]), min_diff)";}
		virtual std::string both_tests(int pixel) const { return brighter_test(pixel) + "||" + darker_test(pixel);}
		C_print_with_scores(const tree* t, const vector<ImageRef>& o, ostream& oo)
		:print_code(t, o)
		{
			visit_tree(node, "		", oo);
		}

};

class CXX_print: public print_code
{
	public:
		virtual std::string print_if(const std::string& test)const { return "if(" + test + ")";}
		virtual std::string print_elseif(const std::string& test) const { return "else if(" + test + ")";}
		virtual std::string print_else()const {return "else";}
		virtual std::string print_endif()const {return "";}
		virtual std::string print_non_corner() const { return "continue;";}
		virtual std::string print_corner() const {return "goto success;";}
		virtual std::string brighter_test(int pixel) const {return pixel_access[pixel] + " > cb" ;}
		virtual std::string darker_test(int pixel) const {return pixel_access[pixel] + " < c_b" ;}
		virtual std::string both_tests(int pixel) const { return brighter_test(pixel) + "||" + darker_test(pixel);}

		CXX_print(const tree* t, const vector<ImageRef>& o, string outfilename, string infilename)
		:print_code(t, o)
		{
			num_to_cache = GV3::get<int>("c++.num_to_cache");
			string file;
			{
				ifstream f;
				f.open(infilename.c_str());
				getline(f, file, '\xff');
			}

			ofstream outfile;
			outfile.open(outfilename.c_str());
			

			generate_pixel_accessor();



			//See if we can do the SSE test?
			const tree * bt = node->brighter.get();
			const tree * dt = node->darker.get();
			const tree * st = node->similar.get();
			
			int bto=-1, dto=-2, sto=-3;

			if(bt && bt->is_a_corner == tree::NonTerminal)
				bto = bt->feature_to_test;
				
			if(dt && dt->is_a_corner == tree::NonTerminal)
				dto = dt->feature_to_test;
			
			if(st && st->is_a_corner == tree::NonTerminal)
				sto = st->feature_to_test;

			cerr << print << bto << dto << sto;

			if(!(dto == bto && bto == sto && st->similar->is_a_corner == tree::NonCorner))
				cerr << print << "SSE is b0rked!" << bto << dto << sto;

			
			//Insert the pixel access for the SSE test
			file = replace_1st(file, "SSE_TEST_1", "&" + pixel_access[node->feature_to_test]);
			file = replace_1st(file, "SSE_TEST_2", "&" + pixel_access[bto]);
			

			//Insert the correct (aligned or unaligned) load instruction
			if(offsets[node->feature_to_test].x == 0)
				file = replace_1st(file, "SSE_LOAD_1", "_mm_load_si128");
			else
				file = replace_1st(file, "SSE_LOAD_1", "_mm_loadu_si128");

			if(offsets[bto].x  == 0)
				file = replace_1st(file, "SSE_LOAD_2", "_mm_load_si128");
			else
				file = replace_1st(file, "SSE_LOAD_2", "_mm_load_si128");


			file = replace_all(file, "PIXEL_ARRAY", pixel_array());
			file = replace_all(file, "DECLARE_CACHES", declare_caches());
			file = replace_all(file, "SET_CACHES", set_caches());
			file = replace_all(file, "INCREMENT_CACHES", increment_caches());
			file = replace_all(file, "INCREMENT_16_CACHES", increment_caches_16());
			file = replace_all(file, "NN", GV3::get<string>("N"));

			file = replace_all(file, "BORDER", xtoa(*max_element(member_iterator(o.begin(), &ImageRef::x), member_iterator(o.end(), &ImageRef::x))));

			ostringstream score;
			C_print_with_scores(node, offsets, score);
			file = replace_1st(file, "SCORE_CODE", score.str());
			//C_print_with_scores_bsearch(node, offsets, score);
			//file = replace_1st(file, "SCORE_BSEARCH", score.str());


			ostringstream blob;
			visit_tree(node, "			", blob);
			file = replace_all(file, "CODE", blob.str());
			outfile << file;
		}

	private:
		vector<string> pixel_access;
		vector<int>  caches;
		int num_to_cache;
	
		string declare_caches()
		{
			string r;
			for(unsigned int i=1; i < caches.size(); i++)
				r += string(i==0?"":"\n") + "	const byte* cache_" + xtoa(i) +  ";";
			return r;
		}

		string increment_caches_16()
		{
			string r;
			for(unsigned int i=1; i < caches.size(); i++)
				r += "				cache_" + xtoa(i) + "+=16;";
			return r;
		}

		string increment_caches()
		{
			string r;
			for(unsigned int i=1; i < caches.size(); i++)
				r += ", cache_" + xtoa(i) + "++";
			return r;
		}


		string set_caches()
		{
			string r;
			for(unsigned int i=1; i < caches.size(); i++)
				r += "		cache_" + xtoa(i) + " = cache_0 + pixel[" + xtoa(caches[i]) + "];\n";
			
			return r;
		}
		
		string pixel_array()
		{
			ostringstream s;
			s << "	int pixel[" << offsets.size() << "] = {\n";

			for(unsigned int i=0; i < offsets.size(); i++)
				s << "		" << offsets[i].x << " + i.row_stride() * " << offsets[i].y << ",\n";
			s << "	};";
		
			return s.str();
		}


		void generate_pixel_accessor()
		{

			//First, compute the frequency of access.
			vector<pair<int,int> > counts(offsets.size());
			
			//Number of things to cache
			unsigned int cache_pointers = num_to_cache + 1;
			//The first one is a bodge because we know we're visiting the centre point.
			

			for(unsigned int i=0; i < offsets.size(); i++)
			{
				counts[i].second = i;
				counts[i].first = 0;
			}
			
			//Now compute the frequency of access, by traversing the tree.
			{
				stack<const tree*>  s;
				s.push(node);

				while(!s.empty())
				{
					const tree* n = s.top();
					s.pop();

					if(n->is_a_corner == tree::NonTerminal)
					{
						counts[n->feature_to_test].first += n->num_tests;
						s.push(n->brighter.get());
						s.push(n->darker.get());
						s.push(n->similar.get());
					}
				}
			}

			
			sort(counts.begin(), counts.end());
			reverse(counts.begin(), counts.end());

			map<int, int> row_to_cache;


			pixel_access.resize(offsets.size());

			//Hack to put in cache for the centre point.
			caches.push_back(offsets.size());
			offsets.push_back(ImageRef(0,0));
			row_to_cache[0] = 0;

			
			//Figure out how to access each pixel
			//ie is it a big index, or can it use cached
			//row pointers directly or indirectly
			for(unsigned int i=0; i < counts.size(); i++)
			{
				int offset = counts[i].second;


				if( row_to_cache.count(offsets[offset].y) != 0)
				{
					//There's already a cached pixel on this row, so use it.
					int cache_num = row_to_cache[offsets[offset].y];
					int dx = offsets[offset].x - offsets[caches[cache_num]].x;

					
					pixel_access[offset] = "*(cache_" + xtoa(cache_num) + " + " + xtoa(dx) + ")";// +" /*" + xtoa(offsets[offset]) + "*/";
				}
				else if(caches.size() < cache_pointers)
				{
					//We have space cache slots, so fill one
					pixel_access[offset] = "*cache_" + xtoa(caches.size());// + "/*" + xtoa(offsets[offset]) + "*/";
					row_to_cache[offsets[offset].y] = caches.size();
					caches.push_back(offset);
				}
				else
				{
					pixel_access[offset] = "*(cache_0 + pixel["+xtoa(offset)+"])";//  + "/*" + xtoa(offsets[offset]) + "*/";
				}
			}

			offsets.pop_back();
		}




};
#endif


template<int S> V_tuple<shared_ptr<tree>, uint64_t, uint64_t>::type load_and_build_tree(int num_features, const vector<double>& weights)
{

	shared_ptr<vector<datapoint<S> > > l;
	uint64_t num_datapoints;
	
	//Load the data
	make_rtuple(l, num_datapoints) = load_features<S>(num_features);
	
	cerr << "Loaded.\n";
	
	//Build the tree
	shared_ptr<tree> tree;
	uint64_t num_tests;
	rpair(tree, num_tests) = build_tree<S>(*l, weights, num_features);

	return make_vtuple(tree, num_tests, num_datapoints);
}



////////////////////////////////////////////////////////////////////////////////
//
// Driver program
//


int main(int argc, char** argv)
{
	//Set up default arguments
	GUI.LoadFile("learn_fast_tree.cfg");
	GUI.parseArguments(argc, argv);

	cin.sync_with_stdio(false);
	cout.sync_with_stdio(false);

	
	///////////////////
	//read file
	
	//Read number of features
	int num_features;
	cin >> num_features;
	if(!cin.good() || cin.eof())
		fatal(6, "Error reading number of features.");

	//Read offset list
	vector<ImageRef> offsets(num_features);
	for(unsigned int i=0; i < num_features; i++)
		cin >> offsets[i];
	if(!cin.good() || cin.eof())
		fatal(7, "Error reading offset list.");

	//Read weights for the various offsets
	vector<double> weights(offsets.size());
	for(unsigned int i=0; i < weights.size(); i++)
		weights[i] = GV3::get<double>(sPrintf("weights.%i", i), 1, 1);


	shared_ptr<tree> tree;
	uint64_t num_tests;
	uint64_t num_datapoints;

	if(num_features <= 16)
		make_rtuple(tree, num_tests, num_datapoints) = load_and_build_tree<16>(num_features, weights);
	else if(num_features <= 32)
		make_rtuple(tree, num_tests, num_datapoints) = load_and_build_tree<32>(num_features, weights);
	else if(num_features <= 48)
		make_rtuple(tree, num_tests, num_datapoints) = load_and_build_tree<48>(num_features, weights);
	else if(num_features <= 64)
		make_rtuple(tree, num_tests, num_datapoints) = load_and_build_tree<64>(num_features, weights);
	else
		fatal(8, "Too many feratures (%i). To learn from this, see %s, line %i.", num_features, __FILE__, __LINE__);

	
	cerr << "Num tests: " << num_tests << endl;
	cerr << "Num datapoints: " << num_datapoints<< endl;
	cerr << "Tests per pixel: " << 1.0 * num_tests / num_datapoints<< endl;

	for(unsigned int i=0; i < offsets.size(); i++)
		cerr << offsets[i] << "(" << weights[i] << ") ";
	cerr << endl;

	//CXX_print(tree.get(), offsets, "fast_"+GV3::get<string>("N")+ "_detect.cxx","libcvd_fast_detect.template");
	//CXX_print(tree.get(), offsets, "fast_"+GV3::get<string>("N")+ "_score.cxx","libcvd_fast_score.template");

}
