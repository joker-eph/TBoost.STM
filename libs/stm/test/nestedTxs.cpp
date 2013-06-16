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

#include <sstream>
#include "nestedTxs.h"


#ifndef BOOST_STM_USE_BOOST_MUTEX
static Mutex L = PTHREAD_MUTEX_INITIALIZER;
static Mutex L2 = PTHREAD_MUTEX_INITIALIZER;
static Mutex L3 = PTHREAD_MUTEX_INITIALIZER;
static Mutex L4 = PTHREAD_MUTEX_INITIALIZER;
#else
static Mutex L;
static Mutex L2;
static Mutex L3;
static Mutex L4;
#endif

using namespace boost::stm;

static native_trans<int> x = 0;
static native_trans<int> y = 0;

static void tx_bar();
static void lk_bar();

static void* tx_foo(void*)
{
   transaction::initialize_thread();
   size_t tries = 0;

   try_atomic(t)
   {
      t.lock_conflict(L);

      ++t.write(x);
      tx_bar();
   }
   before_retry { ++tries; }

   cout << "tx done" << endl;
   return 0;
}

static void tx_bar()
{
   use_atomic(t)
   {
      t.write(y) = t.read(x) + y;
   }
}



static void*  lk_foo(void*)
{
   use_lock(&L)
   {
      ++x;
      lk_bar();
   }

   return 0;
}

static void lk_bar()
{
   use_lock(L)
   {
      y = x + y;
      cout << "locking done" << endl;
   }
}





////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm;
using namespace nMain;



///////////////////////////////////////////////////////////////////////////////
static void* stall(void *)
{
   transaction::initialize_thread();

   transaction::lock_(&L2);

   SLEEP(10000);

   transaction::unlock_(&L2);

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void nested2()
{
   bool fail = true;

   atomic(t)
   {
      if (fail)
      {
         t.force_to_abort();
         fail = false;
      }
      ++t.w(x);
   }
   before_retry 
   {
      cout << "caught at nested2\n";
   }
}

///////////////////////////////////////////////////////////////////////////////
static void nested1()
{
   atomic(t)
   {
      ++t.w(x);
      nested2();
   }
   before_retry 
   {
      cout << "caught at nested1\n";
   }
}

///////////////////////////////////////////////////////////////////////////////
static void nested0()
{
   atomic(t)
   {
      ++t.w(x);
      nested1();
   } 
   before_retry 
   {
      cout << "caught at nested0" << endl;
   }
}


///////////////////////////////////////////////////////////////////////////////
static void TestTransactionInsideLock()
{
   using namespace boost::stm;

   cout << "X: " << x.value() << endl;

   SLEEP(1000);

   transaction::lock_(&L);
   transaction::lock_(&L3);

   try_atomic(t)
   {
      t.lock_conflict(&L);
      t.lock_conflict(&L2);
      t.lock_conflict(&L3);

      ++t.write(x);

   } before_retry {}

   transaction::unlock_(&L);
   transaction::unlock_(&L3);

   cout << "X: " << x.value() << endl;
}


///////////////////////////////////////////////////////////////////////////////
static void TestEarlyRelease()
{
   using namespace boost::stm;

   cout << "X: " << x.value() << endl;

   SLEEP(1000);

   transaction::lock_(&L);
   transaction::lock_(&L3);

   try_atomic(t)
   {
      t.lock_conflict(&L);
      t.lock_conflict(&L2);
      t.lock_conflict(&L3);

      transaction::unlock_(&L);

      ++t.write(x);

   } before_retry {}

   transaction::unlock_(&L);
   transaction::unlock_(&L3);

   cout << "X: " << x.value() << endl;
}


///////////////////////////////////////////////////////////////////////////////
void NestedTxTest()
{
   static std::vector<int> runVector;
   int iterations = 0;

   transaction::initialize();
   transaction::initialize_thread();

   //transaction::do_tm_lock_protection();
   transaction::do_tx_lock_protection();

   //transaction::tm_lock_conflict(&L);
   //transaction::tm_lock_conflict(&L2);
   //transaction::tm_lock_conflict(&L3);

   nested2();
   nested1();
   nested0();

   exit(0);

   pthread_t *second_thread = new pthread_t;

   pthread_create(second_thread, NULL, stall, (void*)NULL);

   TestTransactionInsideLock();
   TestEarlyRelease();

   exit(0);
}
