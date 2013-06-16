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

///////////////////////////////////////////////////////////////////////////////
#ifndef HASH_MAP_H
#define HASH_MAP_H

///////////////////////////////////////////////////////////////////////////////
#include <boost/stm/transaction.hpp>
#include "testLinkedList.h"

void TestHashMapWithMultipleThreads();

namespace nHashMap
{
   int const kBuckets = 256;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
class HashMap
{
public:

   bool lookup(T &val)
   {
      return buckets_[val % nHashMap::kBuckets].lookup(val);
   }

   bool insert(list_node<T> &element)
   {
      return buckets_[element.value() % nHashMap::kBuckets].insert(element);
   }

   bool remove(list_node<T> &v)
   {
      return buckets_[v.value() % nHashMap::kBuckets].remove(v);
   }

   size_t walk_size()
   {
      size_t count = 0;
      for (int i = 0; i < nHashMap::kBuckets; ++i)
      {
         count += buckets_[i].walk_size();
      }

      return count;
   }

   ////////////////////////////////////////////////////////////////////////////
   void outputList(std::ofstream &o)
   {
      for (int i = 0; i < nHashMap::kBuckets; ++i)
      {
         buckets_[i].outputList(o);
      }
   }

private:
   LinkedList<T> buckets_[nHashMap::kBuckets];
};

#endif // HASH_MAP_H
