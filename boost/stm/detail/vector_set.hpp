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
#ifndef BOOST_STM_VECTOR_SET_H
#define BOOST_STM_VECTOR_SET_H

#include <vector>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
namespace boost { namespace stm {
size_t const def_set_reserve = 128;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename T>
class vector_set
{
public:

   vector_set(size_t const res = def_set_reserve) { elements_.reserve(res); }

   typedef T first_t;

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   typedef std::vector<T> cont;
   typedef typename std::vector<T>::iterator iterator;
   typedef typename std::vector<T>::const_iterator const_iterator;

   typename vector_set<T>::iterator begin() { return elements_.begin(); }
   typename vector_set<T>::const_iterator begin() const { return elements_.begin(); }

   typename vector_set<T>::iterator end() { return elements_.end(); }
   typename vector_set<T>::const_iterator end() const { return elements_.end(); }

   size_t size() { return elements_.size(); }
   bool empty() { return elements_.empty(); }

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   typename vector_set<T>::iterator find(T const &t)
   {
      for (typename vector_set<T>::iterator i = elements_.begin(); i != elements_.end(); ++i) 
      {
         if (t == i) return i;
      }

      return elements_.end();
   }

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   typename vector_set<T>::iterator insert(first_t const &rhs)
   {
      for (typename vector_set<T>::iterator i = elements_.begin(); i != elements_.end(); ++i) 
      {
         if (rhs == *i) 
         {
            return i;
         }
      }

      elements_.push_back(rhs);
      return (typename vector_set<T>::iterator)&elements_[elements_.size()-1];
   }

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   void erase(typename vector_set<T>::first_t const &rhs)
   {
      for (typename vector_set<T>::iterator i = elements_.begin(); i != elements_.end(); ++i) 
      {
         if (rhs == *i) 
         {
            elements_.erase(i); return;
         }
      }
   }

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   void push_back(typename vector_set<T>::first_t const &rhs)
   {
      elements_.push_back(rhs);
   }

   //-----------------------------------------------------------------------
   //-----------------------------------------------------------------------
   void clear() { elements_.clear(); }

private:
   cont elements_;
};

} // core namespace
}
#endif

