/*

    This file is part of the FAST-ER machine learning system.
    Copyright (C) 2008  Edward Rosten and Los Alamos National Laboratory

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef INC_GVARS_VECTOR_H
#define INC_GVARS_VECTOR_H
#include <vector>
#include <set>
#include <gvars3/serialize.h>

///\cond never
namespace GVars3{ namespace serialize {
		
	/**GVars serialization for containers. 
	   @ingroup gUtility
	*/

	template<class C>std::string to_string(const std::set<C>& s)
	{
		std::ostringstream o;
		typename std::set<C>::const_iterator i;

		for(i=s.begin();i != s.end(); i++)
		{
			if(i != s.begin())
				o <<  " ";
			o << *i;
		}

		return o.str();
	}

	template<class C> int from_string(const std::string& s, std::set<C>& o)
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

	/**GVars serialization for containers. 
	   @ingroup gUtility
	*/

	template<class C>std::string to_string(const std::vector<C>& s)
	{
		std::ostringstream o;
		typename std::vector<C>::const_iterator i;

		for(i=s.begin();i != s.end(); i++)
		{
			if(i != s.begin())
				o <<  " ";
			o << *i;
		}

		return o.str();
	}

	template<class C> int from_string(const std::string& s, std::vector<C>& o)
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
///\endcond
#include <gvars3/instances.h>
#endif

