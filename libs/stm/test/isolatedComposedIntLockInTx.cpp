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

#include <iostream>
#include "isolatedComposedIntLockInTx.h"
#include <boost/stm/transaction.hpp>
#include "main.h"

static boost::stm::native_trans<int> gInt;
#ifndef BOOST_STM_USE_BOOST_MUTEX
static Mutex lock1 = PTHREAD_MUTEX_INITIALIZER;
#else
static Mutex lock1;
#endif
////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm;

static boost::stm::native_trans<int> globalInt;

///////////////////////////////////////////////////////////////////////////////
static void* TestIsolatedComposedLockInTxCount(void *threadId)
{
   transaction::initialize_thread();
   int start = *(int*)threadId;

   int startingValue = 0;
   int endingValue = (startingValue + kMaxInserts);

   for (int i = startingValue; i < endingValue/2; ++i)
   {
      for (transaction t; ; t.restart())
      {
         try
         {
            transaction::lock_(lock1);
            ++gInt.value();
            cout << "\t" << gInt.value() << endl;
            transaction::unlock_(lock1);

            SLEEP(1000); 
            // do nothing on purpose, allowing other threads time to see 
            // intermediate state IF they can get lock1 (they shouldn't)

            transaction::lock_(lock1);
            --gInt.value();
            cout << "\t" << gInt.value() << endl;
            transaction::unlock_(lock1);

            t.end();
            SLEEP(1000);

            break;
         }
         catch (aborted_tx&) {}
      }
   }

   //--------------------------------------------------------------------------
   // last thread out sets the endTimer
   //--------------------------------------------------------------------------
   endTimer = time(NULL);
   finishThread(start);

   if (*(int*)threadId != kMainThreadId)
   {
      transaction::terminate_thread();
      pthread_exit(threadId);
   }

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void* TestComposedCount(void *threadId)
{
   transaction::initialize_thread();
   int start = *(int*)threadId;

   int startingValue = start * 100000;
   int endingValue = startingValue + kMaxInserts;

   for (int i = startingValue; i < 100*endingValue; ++i)
   {
      transaction::lock_(lock1);
      cout << gInt.value() << endl;
      transaction::unlock_(lock1);
      SLEEP(10); // do nothing on purpose
   }

   //--------------------------------------------------------------------------
   // last thread out sets the endTimer
   //--------------------------------------------------------------------------
   endTimer = time(NULL);
   finishThread(start);

   if (*(int*)threadId != kMainThreadId)
   {
      transaction::terminate_thread();
      pthread_exit(threadId);
   }

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
void TestIsolatedComposedIntLockInTx()
{
   transaction::initialize();
   transaction::tm_lock_conflict(&lock1);

   pthread_t *threads = new pthread_t[kMaxThreads];
   int *threadId = new int[kMaxThreads];

   //--------------------------------------------------------------------------
   // Reset barrier variables before creating any threads. Otherwise, it is
   // possible for the first thread 
   //--------------------------------------------------------------------------
   threadsFinished.value() = 0;
   threadsStarted.value() = 0;
   startTimer = 0;
   endTimer = 0;

   for (int j = 0; j < kMaxThreads - 1; ++j)
   {
      threadId[j] = j;
      pthread_create(&threads[j], NULL, TestIsolatedComposedLockInTxCount, (void *)&threadId[j]);
   }

   int mainThreadId = kMaxThreads-1;

   TestComposedCount((void*)&mainThreadId);

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }

   cout << "gInt : " << gInt.value() << endl;
}

