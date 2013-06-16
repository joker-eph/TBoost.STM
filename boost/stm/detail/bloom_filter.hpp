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

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifndef BOOST_STM_BLOOM_FILTER_H
#define BOOST_STM_BLOOM_FILTER_H

#ifdef WIN32
#pragma warning (disable:4786)
#endif

#ifdef BOOST_STM_BLOOM_FILTER_USE_DYNAMIC_BITSET
#include <boost/dynamic_bitset.hpp>
#else
#include <boost/stm/detail/bit_vector.hpp>
#endif
//#include <vector>
#include <boost/stm/detail/jenkins_hash.hpp>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
namespace boost { namespace stm {

#ifdef BOOST_STM_BLOOM_FILTER_USE_DYNAMIC_BITSET
   std::size_t const def_bit_vector_size = 65536;
#endif
   typedef std::size_t (*size_t_fun_ptr)(std::size_t rhs);
   std::size_t const bitwiseAndOp = def_bit_vector_size-1;
   std::size_t const size_of_size_t = sizeof(std::size_t);
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class bloom_filter
{
public:
    bloom_filter() 
#ifdef BOOST_STM_BLOOM_FILTER_USE_DYNAMIC_BITSET
        :   bit_vector1_(def_bit_vector_size)
        ,   bit_vector2_(def_bit_vector_size) 
#else
#endif
    {}
   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   void insert(std::size_t const &rhs)
   {
      h1_ = h2_ = 0;
      hashlittle2((void*)&rhs, size_of_size_t, &h1_, &h2_);
      bit_vector1_.set( h1_ & bitwiseAndOp );
      bit_vector2_.set( h2_ & bitwiseAndOp );
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   bool exists(std::size_t const &rhs)
   {
      h1_ = h2_ = 0;
      hashlittle2((void*)&rhs, size_of_size_t, &h1_, &h2_);
      return bit_vector1_.test( h1_ & bitwiseAndOp ) &&
         bit_vector2_.test( h2_ & bitwiseAndOp );
   }

   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   std::size_t intersection(bloom_filter const &rhs)
   {
       return bit_vector1_.intersects(rhs.bit_vector1_)
         && bit_vector2_.intersects(rhs.bit_vector2_);
   }

   std::size_t h1() const { return h1_; }
   std::size_t h2() const { return h2_; }

   void set_bv1(std::size_t rhs) { bit_vector1_.set( rhs & bitwiseAndOp ); }
   void set_bv2(std::size_t rhs) { bit_vector2_.set( rhs & bitwiseAndOp ); }
   //------------------------------------------------------------------------
   //------------------------------------------------------------------------
   void clear()
   {
      bit_vector1_.clear();
      bit_vector2_.clear();
   }

private:

   std::size_t h1_, h2_;
#ifdef BOOST_STM_BLOOM_FILTER_USE_DYNAMIC_BITSET
   boost::dynamic_bitset<> bit_vector1_;
   boost::dynamic_bitset<> bit_vector2_;
#else
   bit_vector bit_vector1_;
   bit_vector bit_vector2_;         
#endif
};


} // end of core namespace
}
#endif // BLOOM_FILTER_H


