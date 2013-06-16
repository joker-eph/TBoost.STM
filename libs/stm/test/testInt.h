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

#ifndef TEST_INT_H
#define TEST_INT_H

#include <boost/stm/transaction.hpp>

////////////////////////////////////////////////////////////////////////////
// Int Transaction class
////////////////////////////////////////////////////////////////////////////
class Integer : public boost::stm::transaction_object<Integer>
{
public:

   Integer() : value_(0) {}

#if BUILD_MOVE_SEMANTICS
   //--------------------------------------------------
   // move semantics
   //--------------------------------------------------
   Integer(Integer &&rhs) { value_ = rhs.value_;}
   Integer& operator=(Integer &&rhs)
   { value_ = rhs.value_; return *this; }
#endif

   int &value() { return value_; }
   int const &value() const { return value_; }

private:

   int value_;
};

void TestInt();

#endif
