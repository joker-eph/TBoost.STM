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

#ifndef TRANSFER_FUN_H
#define TRANSFER_FUN_H

#include <ostream>

void TestTransferFunctionMultipleThreads();

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
enum eTransferFunction
{
   eNoTransferFunction = 0,
   eHardLimit,
   eHardLimitSymmetric,
   eLinear,
   ePositiveLinear,
   eSaturatingLinear,
   eSaturatingLinearSymmetric,
   eSigmoid,
   eTanSigmoid,
   eMaxTransferFunction = eTanSigmoid
};

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TransferFunction
{
public:

   TransferFunction(eTransferFunction const &fun, double inputVal) : 
      fun_(fun), input_(inputVal), output_(0) {}

   TransferFunction() : fun_(eNoTransferFunction), input_(0), output_(0) {}

   double const &input() const { return input_; }
   double const &output() const { return output_; }
   eTransferFunction const &fun() const { return fun_; }

   void input(double const &rhs) { input_ = rhs; }
   void output(double const &rhs) { output_ = rhs; }
   void fun(eTransferFunction const &rhs) { fun_ = rhs; }

   void performFunction();

   bool operator==(TransferFunction const & rhs) const
   { return fun_ == rhs.fun_ && input_ == rhs.input_; }

   bool operator!=(TransferFunction const & rhs) const
   { return !this->operator==(rhs); }

   bool operator<(TransferFunction const & rhs) const
   { 
      if (fun_ < rhs.fun_) return true; 
      else if (input_ < rhs.input_) return true;
      else return false;
   }

   bool operator>(TransferFunction const & rhs) const
   { 
      if (fun_ > rhs.fun_) return true; 
      else if (input_ > rhs.input_) return true;
      else return false;
   }

   friend std::ostream& operator<<(std::ostream &out, TransferFunction &rhs)
   {
      out << rhs.fun_ << ":" << rhs.input_;
      return out;
   }

private:

   eTransferFunction fun_;
   double input_;
   double output_;
};

#endif //TRANSFER_FUN_H

