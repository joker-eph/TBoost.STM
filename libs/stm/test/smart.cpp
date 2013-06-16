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
#include <boost/stm/transaction.hpp>
#include "smart.h"
#include "main.h"

using namespace boost::stm;

static int kMaxOuterLoops = 2*1000;
static int kMaxInnerLoops = 2*5000;

static native_trans<int> txInt = 0, txInt2, txInt3 = 0;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void test_nested()
{
   atomic(t)
   {
      write_ptr<native_trans<int> > tx(t, txInt);
      ++*tx;
   } end_atom
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void test_parent()
{
   atomic(t)
   {
      read_ptr<native_trans<int> > tx(t, txInt);
      if (0 == *tx) test_nested();
      assert(*tx == 1);
      cout << "*tx should be 1, output: " << *tx << endl;
      cout << "*tx should be 1, output: " << *tx << endl;
   } end_atom
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void test_priv_write_ptr()
{
   int x = 0, y = 0, z = 0;

   for (int i = 0; i < kMaxOuterLoops; ++i)
   {
      atomic(t)
      {
         for (int j = 0; j < kMaxInnerLoops; ++j)
         {
            write_ptr< native_trans<int> > tx(t, txInt);

            x = *tx;
            y = *tx;
            ++*tx;
            z = *tx;
            ++*tx;
         }

      } end_atom
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void test_priv_read_ptr()
{
   int x = 0, y = 0, z = 0;

   for (int i = 0; i < kMaxOuterLoops; ++i)
   {
      atomic(t)
      {
         for (int j = 0; j < kMaxInnerLoops; ++j)
         {
            read_ptr< native_trans<int> > tx(t, txInt);

            x = *tx;
            y = *tx;
            ++*(tx.write_ptr());
            z = *tx;
            ++*(tx.write_ptr());
         }

      } end_atom
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void test_no_private()
{
   int x = 0, y = 0, z = 0;
   for (int i = 0; i < kMaxOuterLoops; ++i)
   {
      atomic(t)
      {
         for (int j = 0; j < kMaxInnerLoops; ++j)
         {
            x = t.r(txInt);
            y = t.r(txInt);
            ++t.w(txInt);
            z = t.r(txInt);
            ++t.w(txInt);
         }

      } end_atom
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void test2()
{
   atomic(t)
   {
      native_trans<int> const loc = t.r(txInt);

      ++t.w(txInt);

   } end_atom

   int y = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void test_smart()
{
   boost::stm::transaction::initialize();
   boost::stm::transaction::initialize_thread();

   //boost::stm::transaction::do_direct_updating();
   boost::stm::transaction::do_deferred_updating();

   //test_2();
   //test_parent();

   int const kMaxTestLoops = 1;

   for (int i = 0; i < kMaxTestLoops; ++i)
   {
      txInt = 0;

      cout << "outer loop #: " << kMaxOuterLoops << endl;
      cout << "inner loop #: " << kMaxInnerLoops << endl << endl;

      startTimer = time(NULL);
      test_no_private();
      endTimer = time(NULL);

      cout << "no_privatization time: " << endTimer - startTimer << endl;
      cout << "                txInt: " << txInt << endl << endl;

      txInt = 0;

      startTimer = time(NULL);
      test_priv_write_ptr();
      endTimer = time(NULL);

      cout << "   privatization time w/ write_ptr: " << endTimer - startTimer << endl;
      cout << "                txInt: " << txInt << endl << endl;

      txInt = 0;

      startTimer = time(NULL);
      test_priv_read_ptr();
      endTimer = time(NULL);

      cout << "   privatization time w/ read_ptr: " << endTimer - startTimer << endl;
      cout << "                txInt: " << txInt << endl << endl;

      cout << "---------------------------------" << endl;

      kMaxOuterLoops /= 10;
      kMaxInnerLoops *= 10;
   }

}


