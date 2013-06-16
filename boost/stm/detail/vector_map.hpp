//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Justin E. Gottchlich 2009. 
// (C) Copyright Vicente J. Botet Escriba 2009. 
// Distributed under the Boost
// Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or 
// copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/synchro for documentation.
//
//////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifndef BOOST_STM_VECTOR_MAP_H
#define BOOST_STM_VECTOR_MAP_H

#include <vector>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
namespace boost { namespace stm {

   size_t const def_map_reserve = 128;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename T, typename U>
class vector_map
{
public:

   vector_map(size_t const &res = def_map_reserve) { pairs_.reserve(res); }

   typedef T first_t;
   typedef U second_t;

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   typedef std::vector<std::pair<T, U> > cont;
   typedef std::pair<T, U> cont_pair;
   typedef typename std::vector<std::pair<T, U> >::iterator iterator;

   typename vector_map<T,U>::iterator begin() { return pairs_.begin(); }
   typename vector_map<T,U>::iterator end() { return pairs_.end(); }

   size_t size() { return pairs_.size(); }
   bool empty() { return pairs_.empty(); }

   T& first(size_t const val) { return pairs_[val].first; }
   U& second(size_t const val) { return pairs_[val].second; }

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   typename vector_map<T,U>::iterator find(first_t const &t)
   {
      for (typename vector_map<T,U>::iterator i = pairs_.begin(); i != pairs_.end(); ++i) 
      {
         if (t == i->first) return i;
      }

      return pairs_.end();
   }

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   typename vector_map<T,U>::iterator insert(cont_pair const &wp)
   {
      T const &f = wp.first;

      for (typename vector_map<T,U>::iterator i = pairs_.begin(); i != pairs_.end(); ++i) 
      {
         if (f == i->first) 
         {
            i->second = wp.second;
            return i;
         }
      }

      pairs_.push_back(wp);

	  //-----------------------------------------------------------------------
	  // WARNING: inefficient code (fix at some point)
	  //
	  // recursively call the original function with the argument that we just inserted, so it'll
	  // return an iterator to it.
	  //-----------------------------------------------------------------------
	  return insert(wp);

      //return (typename vector_map<T,U>::iterator)&pairs_[pairs_.size()-1];
   }

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   void erase(typename vector_map<T,U>::iterator &iter)
   {
      pairs_.erase(iter);
   }

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   void clear() { pairs_.clear(); }

private:
   cont pairs_;
};

} // core namespace
}
#endif

