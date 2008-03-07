#ifndef INC_GVARS_VECTOR_H
#define INC_GVARS_VECTOR_H

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
#endif

