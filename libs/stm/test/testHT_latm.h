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
#ifndef LATM_HASH_MAP_WITH_LOCKS_H
#define LATM_HASH_MAP_WITH_LOCKS_H

///////////////////////////////////////////////////////////////////////////////
#include <boost/stm/transaction.hpp>
#include "testLL_latm.h"

void TestHashMapWithLocks();

namespace LATM
{
   namespace nHashMap
   {
      int const kBuckets = 256;
   }

///////////////////////////////////////////////////////////////////////////////
template <typename T>
class LATM_HashMap
{
public:

   bool lock_lookup(T &val)
   {
      return buckets_[val % nHashMap::kBuckets].lock_lookup(val);
   }

   bool lock_insert(list_node<T> &element)
   {
      return buckets_[element.value() % nHashMap::kBuckets].lock_insert(element);
   }

   bool lookup(T &val)
   {
      return buckets_[val % nHashMap::kBuckets].lookup(val);
   }

   bool insert(list_node<T> &element)
   {
      return buckets_[element.value() % nHashMap::kBuckets].insert(element);
   }

   bool move(list_node<T> const &node1, list_node<T> const &node2)
   {
      using namespace boost::stm;
      bool succeeded1 = true, succeeded2 = true;
      transaction_state state = e_no_state;

      do 
      {
         try
         {
            transaction t;
            succeeded1 = buckets_[node1.value() % nHashMap::kBuckets].remove(node1);
            succeeded2 = buckets_[node2.value() % nHashMap::kBuckets].insert(node2);
            t.end();
         }
         catch (aborted_transaction_exception&) {}

         if (!succeeded1 || !succeeded2) 
         {
            return false; // auto abort of t
         }

      } while (e_committed != state);

      return true;
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

   Mutex* get_hash_lock(int val) { return buckets_[val].get_list_lock(); }

private:
   LATM::LinkedList<T> buckets_[nHashMap::kBuckets];
};

} // namespace LATM

void TestHashTableSetsWithLocks();

#endif // HASH_MAP_H
