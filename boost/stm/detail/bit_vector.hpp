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

#ifndef BOOST_STM_BIT_VECTOR_H
#define BOOST_STM_BIT_VECTOR_H

#include <string.h>
#include <cstdio>
#include <math.h>
#include <iostream>

//---------------------------------------------------------------------------
// smaller allocation size: potentially more conflicts but more chance to
//                          fit into a single cache line
//---------------------------------------------------------------------------
//#define def_bit_vector_size 131072
//#define def_bit_vector_size 2048
//#define def_bit_vector_size 16384
//#define def_bit_vector_size 32768
#define def_bit_vector_size 65536
//#define def_bit_vector_size 131072
//#define def_bit_vector_size 1048576

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
namespace boost { namespace stm {

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class bit_vector
{
public:
   typedef size_t chunk_type; // Could have a SSE/AVX implementation
   static size_t const byte_size = 8;
   static size_t const chunk_bits = sizeof(chunk_type) * byte_size;
   static size_t const chunk_shift_bits = chunk_bits - 1; // Works for power of 2!

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   bit_vector()
   {
      clear(); // zero initialize
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   chunk_type operator[](size_t const idx) const
   {
      //---------------------------------------------------------------------
      // (1) select the correct chunk from the bits_ array.
      // Hopefully the compiler generates a shift because sizeof() is likely
      // to be a power of two
      const size_t bit_idx = idx_to_bit_idx(idx);
      // (2) select the right bit in the chunk ( rhs % chunk_size )
      const chunk_type chunk_bit_pos = idx_to_chunk_idx(idx);
      // (3) perform bitwise AND, this'll leave "chunk_bit_pos" set to 1 if
      //     the bits_ array has a 1 in the specific bit location
      const chunk_type new_chunk = bits_[bit_idx] & ((chunk_type)1 << chunk_bit_pos);
      // (4) shift it back to the first bit location so we return a 0 or 1
      //     it is probably better than casting to bool here, since the
      //     compiler would have to perform a comparison
      //---------------------------------------------------------------------      
      return new_chunk >> chunk_bit_pos;
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   void clear()
   {
      memset(bits_, 0, sizeof(bits_));
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   size_t const size() const { return def_bit_vector_size; }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   size_t const alloc_size() const { return sizeof(bits_); }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   void set(size_t idx)
   {
     //---------------------------------------------------------------------
     // (1) select the correct chunk from the bits_ array.
     // Hopefully the compiler generates a shift because sizeof() is likely
     // to be a power of two
     const size_t bit_idx = idx_to_bit_idx(idx);
     // (2) select the right bit in the chunk ( rhs % chunk_size )
     const chunk_type chunk_bit_pos = idx_to_chunk_idx(idx);
     // (3) perform bitwise AND, this'll leave "chunk_bit_pos" set to 1 if
     //     the bits_ array has a 1 in the specific bit location
      bits_[bit_idx] |=  ((chunk_type)1 << chunk_bit_pos);
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   // Reset a position (set to 0)
   void reset(size_t idx)
   {
      // Compute a mask by first having the target bit set to 1 and reversing
      chunk_type mask = ~((chunk_type)1 << (idx & chunk_shift_bits));
      bits_[idx_to_bit_idx(idx)] &= mask;
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   bool test(size_t idx) const
   {
      return operator[](idx);
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   size_t intersects(bit_vector const & rhs) const
   {
      for (register size_t i = 0; i < sizeof(bits_)/sizeof(bits_[0]); ++i)
      {
         if (bits_[i] & rhs.bits_[i]) return 1;
      }

      return 0;
   }

private:
   // Select the correct chunk from the bits_ array.
   static const size_t idx_to_bit_idx(size_t const idx) {
      // Hopefully the compiler generates a shift because sizeof() is
      // likely to be (always?) a power of two
      return idx/sizeof(chunk_type);
   }

   // Select the right bit in the chunk
   static const chunk_type idx_to_chunk_idx(chunk_type idx) {
      // ( rhs % sizeof(chunk_type) )
      // chunk_size is always a power of 2, then bitwise and is the trick!
      return idx & chunk_shift_bits;
   }

   // The real array containing the data. Size is know at compile time
   // The "+1" is only useful if "def_bit_vector_size" is not a multiple
   // of sizeof(chunk_type)
   chunk_type bits_[def_bit_vector_size/sizeof(chunk_type)+1];
};

}
}


#endif // BIT_VECTOR_H

