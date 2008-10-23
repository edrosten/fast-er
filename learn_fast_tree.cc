/**
	\file learn_fast_tree.cc Main file for the \p learn_fast_tree executable.  

    <tt> learn_fast_tree [--weight.</tt><i>x  weight</i><tt> ] ... \< </tt> \e infile \p \> \e outfile
    
    \p learn_fast_tree used ID3 to learn a ternary decision tree for corner
    detection. The data is read from the standard input, and the tree is written
    to the standard output. This is designed to learn FAST feature detectors,
    and does not allow for the possibility ambbiguity in the input data.

    \section lfInput Input data

    The input data has the following format:

    \verbatim

5
[-1 -1] [1 1] [3 4] [5 6] [-3 4]
bbbbb 1     0
bsdsb 1000  1
.
.
.
\endverbatim
    
    The first row is the number of features. The second row is the the list of
    offsets assosciated with each feature. This list has no effect on the
    learning of the tree, but it is passed through to the outpur for
    convinience.

    The remaining rows contain the data. The first field is the ternary feature
    vector. The three characters "b", "d" and "s" are the correspond to
    brighter, darker and similar respectively, with the first feature being
    stored in the first character and so on.

    The next field is the number of instances of the particular feature.
    The third field is the class, with 1 for corner, and 0 for background.



\subsection f9Generate Generating input data
    
    Ideally, input data will be generated from some sample images. The 
    program FIXME can be used to do this.

    Additionally, a the program fast_N_features can be used to generate all 
    possible feature combinations for FAST-N features. When run without
    arguments, it generates data for FAST-9 features, otherwise the argument can
    be used to specify N.

\section lfGen Output data
    
    The program does not generate source code directly, rather it generates an
    easily parsabel representation of a decision tree which can be turned in to
    source code.

    The structure of the tree is described in detail in ::print_tree.
    
*/

///\cond never
#ifndef DOXYGEN_IGNORE
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
#endif

using namespace std;
using namespace tr1;
using namespace tag;
using namespace CVD;
using namespace GVars3;
///\endcond 

///Representations of ternary digits.
enum Ternary
{
	Brighter='b',
	Darker  ='d',
	Similar ='s'
};

///Print an error message and the exit
///@param E Error code
///@param S Format string
///@ingroup gUtility
#define fatal(E, S, ...) vfatal((E), (S), (tag::Fmt,## __VA_ARGS__))
///Print an error message and the exit, using Tuple stype VARARGS
///@param err Error code
///@param s Format string
///@param list Argument list
///@ingroup gUtility
template<class C> void vfatal(int err, const string& s, const C& list)
{
	vfPrintf(cerr, s + "\n", list);
	exit(err);
}

/**This structure represents a datapoint. A datapoint is a group of pixels with
ternary values (much brighter than the centre, much darker than the centre or
similar to the centre pixel). In addition to the feature descriptor, the class
and number of instances is also stored.

The maximum feature vector size is determined by the template parameter. This
allows the ternary vector to be stored in a bitset. This keeps the struct a
fixed size and removes the need for dynamic allocation.
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
		///"s".
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

///Find the feature that has the highest weighted entropy change.
///@param fs datapoints to split in to three subsets.
///@param weights weights on features
///@param nfeats Number of features in use.
///@return best feature.
template<int S> int find_best_split(const vector<datapoint<S> >& fs, const vector<double>& weights, int nfeats)
{
    assert(nfeats == weights.size());
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

///This class represents a decision tree.
///Each leaf node contains a class, being Corner or NonCorner.
///Each decision node contains a feature about which to make a ternary decision.
///Additionally, each node records how many datapoints were tested.
///The generated tree structure is not mutable.
struct tree{
	///The class of the leaf, and a sentinal to indacate that the node is
	///not a leaf. Now that I come back to this, it looks suspiciously like
	///an instance of http://thedailywtf.com/Articles/What_Is_Truth_0x3f_.aspx
	///Oh well.
	enum IsCorner
	{
		Corner,
		NonCorner,
		NonTerminal
	};

	const shared_ptr<tree> brighter;                  ///<Subtrees
	const shared_ptr<tree> darker;                    ///<Subtrees
	const shared_ptr<tree> similar;                   ///<Subtrees
	const IsCorner is_a_corner;                       ///<Class of this node (if its a leaf)
	const int feature_to_test;                        ///<Feature (ie pixel) to test if this  is a non-leaf.
	const uint64_t num_datapoints;	   				  ///<Number of datapoints passing through this node.

	///Convert the tree to a simple string representation.
	///This is allows comparison of two trees to see if they are the same.
	///It's probably rather inefficient to hammer the string class compared
	///to using an ostringstream, but this is not the slowest part of the program.
	///@return a stringified tree representation
	string stringify()
	{
		if(is_a_corner == NonTerminal)
			return "(" + brighter->stringify() + darker->stringify() + similar->stringify() + ")";
		else
			return string("(") + (is_a_corner == Corner?"1":"0")  +  ")";
	}

	///Create a leaf node which is a corner
	///This special constructor function makes it impossible to 
	///construct a leaf with the NonTerminal class.
    ///@param n number of datapoints reaching this node.
	static shared_ptr<tree> CornerLeaf(uint64_t n)
	{
		return shared_ptr<tree>(new tree(Corner, n));
	}
	
	///Creat a leaf node which is a non-corner
	///This special constructor function makes it impossible to 
	///construct a leaf with the NonTerminal class.
    ///@param n number of datapoints reaching this node.
	static shared_ptr<tree> NonCornerLeaf(uint64_t n)
	{
		return shared_ptr<tree>(new tree(NonCorner, n));
	}
	
	///Create a non-leaf node
	///@param b The brighter subtree
	///@param d The darker subtree
	///@param s The similar subtree
    ///@param n Feature number to test
    ///@param num Number of datapoints reaching this node.
	tree(shared_ptr<tree> b, shared_ptr<tree> d, shared_ptr<tree> s, int n, uint64_t num)
	:brighter(b), darker(d), similar(s), is_a_corner(NonTerminal), feature_to_test(n), num_datapoints(num)
	{}

	private:
	///The leaf node constructor is private to prevent a tree
	///being constructed with invalid values.
	///see also CornerLeaf and NonCornerLeaf.
	///@param c Class of the node
	///@param n Number of datapoints which this node represents
	tree(IsCorner c, uint64_t n)
	:is_a_corner(c),feature_to_test(-1),num_datapoints(n)
	{}
};


///This function uses ID3 to construct a decision tree. The entropy changes
///are weighted by the list of weights, to allow bias towards certain features.
///This function assumes that the class is an exact function of the data. If 
///there datapoints with different classes share the same feature vector, the program
///will crash with error code 3.
///@param corners Datapoints in this part of the subtree to classify
///@param weights Weights on the features
///@param nfeats Number of features actually used
///@return The tree required to classify corners
template<int S> shared_ptr<tree> build_tree(vector<datapoint<S> >& corners, const vector<double>& weights, int nfeats)
{
	//Find the split
	int f = find_best_split<S>(corners, weights, nfeats);

	//Split corners in to the three chunks, based on the result of find_best_split.
	//Also, count how many of each class ends up in each of the three bins.
	//It may apper to be inefficient to use a vector here instead of a list, in terms
	//of memory, but the per-element storage overhead of the list is such that it uses
	//considerably more memory and is much slower.
	vector<datapoint<S> > brighter, darker, similar;
	uint64_t num_bri=0, cor_bri=0, num_dar=0, cor_dar=0, num_sim=0, cor_sim=0;

	for(size_t i=0; i < corners.size(); i++)
	{
		switch(corners[i].get_trit(f))
		{
			case Brighter:
				brighter.push_back(corners[i]);
				num_bri += corners[i].count;
				if(corners[i].is_a_corner)
					cor_bri += corners[i].count;
				break;

			case Darker:
				darker.push_back(corners[i]);
				num_dar += corners[i].count;
				if(corners[i].is_a_corner)
					cor_dar += corners[i].count;
				break;

			case Similar:
				similar.push_back(corners[i]);
				num_sim += corners[i].count;
				if(corners[i].is_a_corner)
					cor_sim += corners[i].count;
				break;
		}
	}
	
	//Deallocate the memory now it's no longer needed.
	corners.clear();
	
	//This is not the same as corners.size(), since the corners (datapoints)
	//have a count assosciated with them.
	uint64_t num_tests =  num_bri + num_dar + num_sim;

	
	//Build the subtrees
	shared_ptr<tree> b_tree, d_tree, s_tree;
	
	//If the sublist contains a single class, then instantiate a leaf,
	//otherwise recursively build the tree.
	if(cor_bri == 0)
		b_tree = tree::NonCornerLeaf(num_bri);
	else if(cor_bri == num_bri)
		b_tree = tree::CornerLeaf(num_bri);
	else
		b_tree = build_tree<S>(brighter, weights, nfeats);
	

	if(cor_dar == 0)
		d_tree = tree::NonCornerLeaf(num_dar);
	else if(cor_dar == num_dar)
		d_tree = tree::CornerLeaf(num_dar);
	else
		d_tree = build_tree<S>(darker, weights, nfeats);


	if(cor_sim == 0)
		s_tree = tree::NonCornerLeaf(num_sim);
	else if(cor_sim == num_sim)
		s_tree = tree::CornerLeaf(num_sim);
	else
		s_tree = build_tree<S>(similar, weights, nfeats);
	
	return shared_ptr<tree>(new tree(b_tree, d_tree, s_tree, f, num_tests));
}


/**This function traverses the tree and produces a textual representation of it.
Additionally, if any of the subtrees are the same, then a single subtree is produced
and the test is removed.

A subtree has the following format:
\verbatim 
    subtree= lead | node;
    
    leaf = "corner" | "background" ;

    node = node2 | node3;

    node3 = "if_brighter" feature_number n1 n2 n3
                subtree
            "elsf_darker" feature_number
                subtree
            "else"
                subtree
            "end";

     node2= if_statement feature_number n1 n2
                subtree
            "else"
                subtree
            "end";

    if_statement = "if_brighter" | "if_darker" | "if_either";
    feature_number ==integer;
    n1 = integer;
    n2 = integer;
    n3 = integer;
\endverbatim

\e feature_number refers to the index of the feature that the test is performed on.

In \e node3, a 3 way test is performed. \e n1, \e n2 and \e n3 refer to the
number of training examples landing in the \e if block, the \e elfs block and
the \e else block respectivly.

In a \e node2 node, one of the tests has been removed. \e n1 and  \e n2refer to
the number of training examples landing in the \e if block and the \e else
block respectivly.

Although not mentioned in the grammar, the indenting is kept very strict.

This representation has been designed to be parsed very easily with simple
regular expressions, hence the use if "elsf" as opposed to "elif" or "elseif".

@param node (sub)tree to serialize
@param o Stream to serialize to.
@param i Indent to print before each line of the serialized tree.
*/
void print_tree(const tree* node, ostream& o, const string& i="")
{
	if(node->is_a_corner == tree::Corner)
		o << i << "corner" << endl;
	else if(node->is_a_corner == tree::NonCorner)
		o << i << "background" << endl;
	else
	{
		string b = node->brighter->stringify();
		string d = node->darker->stringify();
		string s = node->similar->stringify();

		const tree * bt = node->brighter.get();
		const tree * dt = node->darker.get();
		const tree * st = node->similar.get();
		string ii = i + " ";

		int f = node->feature_to_test;
	
		if(b == d && d == s) //All the same
		{
			//o << i << "if " << f << " is whatever\n";
			print_tree(st, o, i);
		}
		else if(d == s)  //Bright is different
		{
			o << i << "if_brighter " << f << " " << bt->num_datapoints << " " << dt->num_datapoints+st->num_datapoints << endl;
				print_tree(bt, o, ii);
			o << i << "else" << endl;
				print_tree(st, o, ii);
			o << i << "end" << endl;

		}
		else if(b == s)	//Dark is different
		{	
			o << i << "if_darker " << f << " " << dt->num_datapoints << " " << bt->num_datapoints + st->num_datapoints << endl;
				print_tree(dt, o, ii);
			o << i << "else" << endl;
				print_tree(st, o, ii);
			o << i << "end" << endl;
		}
		else if(b == d) //Similar is different
		{
			o << i << "if_either " << f << " " <<  bt->num_datapoints + dt->num_datapoints  << " " << st->num_datapoints << endl;
				print_tree(bt, o, ii);
			o << i << "else" << endl;
				print_tree(st, o, ii);
			o << i << "end" << endl;
		}
		else //All different
		{
			o << i << "if_brighter " << f << " "  <<  bt->num_datapoints << " " << dt->num_datapoints  << " " << st->num_datapoints << endl;
				print_tree(bt, o, ii);
			o << i << "elsf_darker " << f << endl;
				print_tree(dt, o, ii);
			o << i << "else" << endl;
				print_tree(st, o, ii);
			o << i << "end" << endl;
		}
	}
}

///This function loads data and builds a tree. It is templated because datapoint
///is templated, for reasons of memory efficiency.
///@param num_features Number of features used
///@param weights Weights on each feature. 
///@return The learned tree, and number of datapoints.
template<int S> V_tuple<shared_ptr<tree>, uint64_t>::type load_and_build_tree(int num_features, const vector<double>& weights)
{
    assert(weights.size() == num_features);

	shared_ptr<vector<datapoint<S> > > l;
	uint64_t num_datapoints;
	
	//Load the data
	make_rtuple(l, num_datapoints) = load_features<S>(num_features);
	
	cerr << "Loaded.\n";
	
	//Build the tree
	shared_ptr<tree> tree;
	tree  = build_tree<S>(*l, weights, num_features);

	return make_vtuple(tree, num_datapoints);
}



///The main program
///@param argc Number of commandline arguments
///@param argv Commandline arguments
int main(int argc, char** argv)
{
	//Set up default arguments
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
	uint64_t num_datapoints;

    ///Each feature takes up 2 bits. Since GCC doesn't pack any finer
    ///then 32 bits for hetrogenous structs, there is no point in having
    ///granularity finer than 16 features.
	if(num_features <= 16)
		make_rtuple(tree, num_datapoints) = load_and_build_tree<16>(num_features, weights);
	else if(num_features <= 32)
		make_rtuple(tree, num_datapoints) = load_and_build_tree<32>(num_features, weights);
	else if(num_features <= 48)
		make_rtuple(tree, num_datapoints) = load_and_build_tree<48>(num_features, weights);
	else if(num_features <= 64)
		make_rtuple(tree, num_datapoints) = load_and_build_tree<64>(num_features, weights);
	else
		fatal(8, "Too many feratures (%i). To learn from this, see %s, line %i.", num_features, __FILE__, __LINE__);

	
	cout << num_features << endl;
	copy(offsets.begin(), offsets.end(), ostream_iterator<ImageRef>(cout, " "));
	cout << endl;
	print_tree(tree.get(), cout);
}
