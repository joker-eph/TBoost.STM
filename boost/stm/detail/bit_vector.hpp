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
#include <math.h>

//#define def_bit_vector_size 4194304

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
#define def_bit_vector_alloc_size def_bit_vector_size / 32
#define def_bit_vector_byte_size def_bit_vector_alloc_size * 4

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
namespace boost { namespace stm {
   typedef size_t chunk_type;
   size_t const chunk_size = sizeof(chunk_type);
   size_t const byte_size = 8;
   size_t const chunk_bits = chunk_size * byte_size;
   size_t const chunk_shift = 5;
   size_t const chunk_shift_bits = (int)pow((double)2.0, (double)chunk_shift) - 1;


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class bit_vector
{
public:

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   bit_vector()
   {
      memset(bits_, 0, def_bit_vector_byte_size);
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   size_t operator[](size_t const rhs) const
   {
      //---------------------------------------------------------------------
      // (1) select the correct chunk from the bits_ array.
      // (2) select the right bit in the chunk ( rhs % chunk_size ) and turn
      //     that bit on ( 1 << ( correct bit ) )
      // (3) perform bitwise AND, this'll return 1 if the bits_ array has
      //     a 1 in the specific bit location
      // (4) shift it back to the first bit location so we return a 0 or 1
      //---------------------------------------------------------------------      
      return ( bits_[(rhs >> chunk_shift)] & (1 << rhs & chunk_shift_bits) ) 
         >> rhs % chunk_bits;
      //return ( bits_[rhs / chunk_bits] & (1 << rhs % chunk_bits) ) 
      //   >> rhs % chunk_bits;
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   void clear()
   {
      memset(bits_, 0, def_bit_vector_byte_size);
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   size_t const size() const { return def_bit_vector_size; }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   size_t const alloc_size() const { return def_bit_vector_alloc_size; }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   void set(size_t rhs) 
   {
      bits_[(rhs >> chunk_shift)] |= 1 << (rhs & chunk_shift_bits); 
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   void reset(size_t rhs) 
   { 
      bits_[(rhs >> chunk_shift)] &= 0 << (rhs & chunk_shift_bits); 
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   bool test(size_t rhs)
   {
      return ( bits_[(rhs >> chunk_shift)] & (1 << (rhs & chunk_shift_bits)) ) > 0 ? true : false; 
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   size_t intersects(bit_vector const & rhs) const
   {
      for (register size_t i = 0; i < def_bit_vector_alloc_size; ++i)
      {
         if (bits_[i] & rhs.bits_[i]) return 1;
      }

      return 0;
   }

private:

   chunk_type bits_[def_bit_vector_alloc_size];
};

}
}


#endif // BIT_VECTOR_H

