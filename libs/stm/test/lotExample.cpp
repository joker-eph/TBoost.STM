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
#include "lotExample.h"
#include <boost/stm/transaction.hpp>
#include "main.h"

#ifndef BOOST_STM_USE_BOOST_MUTEX

static Mutex L1 = PTHREAD_MUTEX_INITIALIZER;
static Mutex L2 = PTHREAD_MUTEX_INITIALIZER;
static Mutex L3 = PTHREAD_MUTEX_INITIALIZER;
static Mutex L8 = PTHREAD_MUTEX_INITIALIZER;
static Mutex L9 = PTHREAD_MUTEX_INITIALIZER;

#else
static Mutex L1;
static Mutex L2;
static Mutex L3;
static Mutex L8;
static Mutex L9;

#endif
////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm;

static native_trans<int> *arr1, *arr2, *arr3, *arr4, *arr5, *arr6, *arr7, *arr8, *arr9;

static bool work1 = false, work2 = false, work3 = false, work4 = false;
static int txFactor = 1;
static int lockFactor = 1;

////////////////////////////////////////////////////////////////////////////
static int iterations = 0;

static void do_work1() 
{
   if (work1) return;
   work1 = true;

   if (kBalancedWork == gWorkLoadType || kTxIntenseWork == gWorkLoadType)
   {
      for (int iters = 0; iters < txFactor*iterations; ++iters)
      {
         for (transaction t;;t.restart())
           try {
             for (int i = 0; i < kMaxArrSize; ++i)
             {
               ++t.w(arr6[i]).value();
             }
             t.end(); break;
           } catch (aborted_tx&) {}
      }
   }
}

static void do_work2() 
{
   if (work2) return;
   work2 = true;

   if (kBalancedWork == gWorkLoadType || kTxIntenseWork == gWorkLoadType)
   {
      for (int iters = 0; iters < txFactor*iterations; ++iters)
      {
         for (transaction t;;t.restart())
           try {
             for (int i = 0; i < kMaxArrSize; ++i)
             {
               ++t.w(arr7[i]).value();
             }
             t.end(); break;
           } catch (aborted_tx&) {}
      }
   }
}

static void do_work3() 
{
   if (work3) return;
   work3 = true;

   for (int iters = 0; iters < iterations; ++iters)
   {
      transaction::lock_(L8);
       for (int i = 0; i < kMaxArrSize; ++i)
       {
         ++arr8[i].value();
       }
       transaction::unlock_(L8);
   }
}

static void do_work4() 
{
   if (work4) return;
   work4 = true;

   for (int iters = 0; iters < iterations; ++iters)
   {
      transaction::lock_(L9);
       for (int i = 0; i < kMaxArrSize; ++i)
       {
         ++arr9[i].value();
       }
      transaction::unlock_(L9);
   }
}

static void* tx1(void *threadId) 
{
   transaction::initialize_thread();
   int start = *(int*)threadId;

   idleUntilAllThreadsHaveReached(*(int*)threadId);
   startTimer = time(NULL);

   for (int iters = 0; iters < txFactor*iterations; ++iters)
   {
      for (transaction t;;t.restart())
        try {
          for (int i = 0; i < kMaxArrSize; ++i)
          {
            ++t.w(arr4[i]).value();
          }
          t.end(); break;
        } catch (aborted_tx&) {}
   }

   if (!work1) { do_work1(); }
   if (!work2) { do_work2(); }

   endTimer = time(NULL);
   finishThread(start);

   if (*(int*)threadId != kMainThreadId)
   {
      transaction::terminate_thread();
      pthread_exit(threadId);
   }

   return threadId;
}

static void* tx2(void *threadId) 
{
   transaction::initialize_thread();
   int start = *(int*)threadId;

   idleUntilAllThreadsHaveReached(*(int*)threadId);
   startTimer = time(NULL);

   for (int iters = 0; iters < txFactor*iterations; ++iters)
   {
      for (transaction t;;t.restart())
        try {
          for (int i = 0; i < kMaxArrSize; ++i)
          {
            ++t.w(arr5[i]).value();
          }
          t.end(); break;
        } catch (aborted_tx&) {}
   }

   if (!work1) { do_work1(); }
   if (!work2) { do_work2(); }

   endTimer = time(NULL);
   finishThread(start);

   if (*(int*)threadId != kMainThreadId)
   {
      transaction::terminate_thread();
      pthread_exit(threadId);
   }

   return threadId;
}

static void* tx3(void *threadId) 
{
   transaction::initialize_thread();

   int start = *(int*)threadId;

   idleUntilAllThreadsHaveReached(*(int*)threadId);
   startTimer = time(NULL);

   for (int iters = 0; iters < txFactor*iterations; ++iters)
   {
      for (transaction t;;t.restart())
        try {
       
          t.add_tx_conflicting_lock(L1);
          t.add_tx_conflicting_lock(L2);

          for (int i = 0; i < kMaxArrSize; ++i)
          {
            ++t.w(arr1[i]).value();
            ++t.w(arr2[i]).value();
          }
          t.end(); break;
        } catch (aborted_tx&) {}
   }

   if (!work1) { do_work1(); }
   if (!work2) { do_work2(); }

   endTimer = time(NULL);
   finishThread(start);

   if (*(int*)threadId != kMainThreadId)
   {
      transaction::terminate_thread();
      pthread_exit(threadId);
   }

   return threadId;
}

static void* lock1(void *threadId) 
{
   transaction::initialize_thread();
   idleUntilAllThreadsHaveReached(*(int*)threadId);
   startTimer = time(NULL);

   for (int iters = 0; iters < lockFactor*3000*iterations; ++iters)
   {
      transaction::lock_(L1); int sum = 0;
      for (int i = 0; i < kMaxArrSize; ++i) sum += arr1[i].value();
      transaction::unlock_(L1);
   }

   endTimer = time(NULL);
   finishThread();

   if (*(int*)threadId != kMainThreadId)
   {
      pthread_exit(threadId);
   }

   return threadId;
}

static void* lock2(void *threadId) 
{
   transaction::initialize_thread();
   idleUntilAllThreadsHaveReached(*(int*)threadId);
   startTimer = time(NULL);

   for (int iters = 0; iters < lockFactor*3000*iterations; ++iters)
   {
      transaction::lock_(L2); int sum = 0;
      for (int i = 0; i < kMaxArrSize; ++i) sum += arr2[i].value();
      transaction::unlock_(L2);
   }

   endTimer = time(NULL);
   finishThread();

   if (*(int*)threadId != kMainThreadId)
   {
      pthread_exit(threadId);
   }

   return threadId;
}

static void* lock3(void *threadId) 
{
   transaction::initialize_thread();
   idleUntilAllThreadsHaveReached(*(int*)threadId);
   startTimer = time(NULL);

   for (int iters = 0; iters < 10000*iterations; ++iters)
   {
      transaction::lock_(L3); int sum = 0;
      for (int i = 0; i < kMaxArrSize; ++i) sum += arr3[i].value();
      transaction::unlock_(L3);
   }

   endTimer = time(NULL);
   finishThread();

   if (*(int*)threadId != kMainThreadId)
   {
      pthread_exit(threadId);
   }

   return threadId;
}

///////////////////////////////////////////////////////////////////////////////
void TestLotExample()
{
   iterations = kMaxArrIter;

   pthread_t *threads = new pthread_t[kMaxThreads];
   int *threadId = new int[kMaxThreads];

   arr1 = new native_trans<int>[kMaxArrSize];
   arr2 = new native_trans<int>[kMaxArrSize];
   arr3 = new native_trans<int>[kMaxArrSize];
   arr4 = new native_trans<int>[kMaxArrSize];
   arr5 = new native_trans<int>[kMaxArrSize];
   arr6 = new native_trans<int>[kMaxArrSize];
   arr7 = new native_trans<int>[kMaxArrSize];
   arr8 = new native_trans<int>[kMaxArrSize];

   transaction::initialize();
   transaction::tm_lock_conflict(L1);
   transaction::tm_lock_conflict(L2);

   if (kTxIntenseWork == gWorkLoadType) txFactor = 5;
   if (kLockIntenseWork == gWorkLoadType) lockFactor = 10;

   //--------------------------------------------------------------------------
   // Reset barrier variables before creating any threads. Otherwise, it is
   // possible for the first thread 
   //--------------------------------------------------------------------------
   threadsFinished.value() = 0;
   threadsStarted.value() = 0;
   startTimer = 0;
   endTimer = 0;

   threadId[0] = 0;
   pthread_create(&threads[0], NULL, tx1, (void *)&threadId[0]);

   threadId[1] = 1;
   pthread_create(&threads[1], NULL, tx2, (void *)&threadId[1]);

   threadId[2] = 2;
   pthread_create(&threads[2], NULL, tx3, (void *)&threadId[2]);

   threadId[3] = 3;
   pthread_create(&threads[3], NULL, lock1, (void *)&threadId[3]);

   threadId[4] = 4;
   pthread_create(&threads[4], NULL, lock2, (void *)&threadId[4]);

   int mainThreadId = kMaxThreads-1;
   lock3((void*)&mainThreadId);

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }

   if (transaction::doing_full_lock_protection()) cout << "full_";
   else if (transaction::doing_tm_lock_protection()) cout << "tm_";
   else if (transaction::doing_tx_lock_protection()) cout << "tx_";

   cout << "run_time\t" << endTimer - startTimer << endl;
}



