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

/* The DRACO Research Group (rogue.colorado.edu/draco) */
/*****************************************************************************\
 *
 * Copyright Notices/Identification of Licensor(s) of
 * Original Software in the File
 *
 * Copyright 2000-2006 The Board of Trustees of the University of Colorado
 * Contact: Technology Transfer Office,
 * University of Colorado at Boulder;
 * https://www.cu.edu/techtransfer/
 *
 * All rights reserved by the foregoing, respectively.
 *
 * This is licensed software.  The software license agreement with
 * the University of Colorado specifies the terms and conditions
 * for use and redistribution.
 *
\*****************************************************************************/
#ifndef BOOST_STM_MEMORY_POOL_H
#define BOOST_STM_MEMORY_POOL_H

//#define MAP_MEMORY_POOL 1

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <map>
#include <vector>

#include <boost/stm/detail/vector_map.hpp>

#ifdef WIN32
#pragma warning( disable : 4786 )
#endif

namespace boost { namespace stm {
   int const kDefaultAllocSize = 512;

/////////////////////////////////////////////////////////////////////////////
template <typename T>
class FixedReserve
{
public:

   //////////////////////////////////////////////////////////////////////////
   explicit FixedReserve(size_t const &amount, size_t const &size) :
      allocSize_(amount), currentLoc_(0), currentSize_(0), chunk_(size), data_(0)
   {
      if (allocSize_ < 1) throw "invalid allocation size";
      data_.reserve(allocSize_ * 2);
      allocateBlock(allocSize_);
   }

   //////////////////////////////////////////////////////////////////////////
   ~FixedReserve() { freeAllocatedMem(); }

   //////////////////////////////////////////////////////////////////////////
   void* retrieveFixedChunk()
   {
      if (currentLoc_ >= currentSize_) allocateBlock(allocSize_);
      return data_[currentLoc_++].object_;
   }

   //////////////////////////////////////////////////////////////////////////
   void returnFixedChunk(void *mem) { data_[--currentLoc_].object_ = mem; }

private:

   // undefined intentionally
   FixedReserve(const FixedReserve&);
   FixedReserve& operator=(const FixedReserve&);

   //////////////////////////////////////////////////////////////////////////
   void freeAllocatedMem()
   {
      // wipe out our allocated memory
      for (register size_t i = currentLoc_; i < currentSize_; ++i)
      {
         free(data_[i].object_);
      }
   }

   //////////////////////////////////////////////////////////////////////////
   void allocateBlock(const size_t amount)
   {
      currentSize_ += amount;
      data_.resize(currentSize_);

      for (register size_t i = currentLoc_; i < currentSize_; ++i)
      {
         data_[i].object_ = malloc(chunk_);
      }
   }

   struct data
   {
      void* object_;
   };

   size_t allocSize_;
   size_t currentLoc_;
   size_t currentSize_;
   size_t chunk_;
   std::vector<data> data_;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename T>
class MemoryPool
{
public:

#ifdef MAP_MEMORY_POOL
   typedef std::map<size_t, FixedReserve<T>* > MemoryMap;
#else
   typedef vector_map<size_t, FixedReserve<T>* > MemoryMap;
#endif
   typedef std::pair<size_t, FixedReserve<T>* > MemoryMapPair;

   MemoryPool() : allocSize_(kDefaultAllocSize) {}

   MemoryPool(size_t const &amount) : allocSize_(amount) {}

   void alloc_size(size_t const &amount) { allocSize_ = amount; }

   //////////////////////////////////////////////////////////////////////////
   ~MemoryPool()
   {
      typename MemoryMap::iterator iter;
      // wipe out all the FixedReserves we created
      for (iter = mem.begin(); mem.end() != iter; ++iter)
      {
         delete iter->second;
      }
   }

   //////////////////////////////////////////////////////////////////////////
   void* retrieveChunk(size_t const &size)
   {
      typename MemoryMap::iterator iter = mem.find(size);

      // if no iterator was found, create one
      if (mem.end() == iter)
      {
#ifdef MAP_MEMORY_POOL
         mem.insert(MemoryMapPair(size, new FixedReserve<T>(allocSize_, size)));
         iter = mem.find(size);
#else
         iter = mem.insert(MemoryMapPair(size, new FixedReserve<T>(allocSize_, size)));
#endif
      }

      return iter->second->retrieveFixedChunk();
   }

   //////////////////////////////////////////////////////////////////////////
   void returnChunk(void *m, size_t const &size)
   { (mem.find(size))->second->returnFixedChunk(m); }

private:

   MemoryMap mem;
   size_t allocSize_;
};

} // namespace core
}
#endif // MEMORY_RESERVE_H
