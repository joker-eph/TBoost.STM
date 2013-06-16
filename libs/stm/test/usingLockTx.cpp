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
#include "usingLockTx.h"

typedef int list_node_type;

static newSyntaxNS::LinkedList< list_node_type > *llist = NULL;

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
static void* TestLinkedListInsertsWithLocks(void *threadId)
{
   newSyntaxNS::list_node<list_node_type> node;
   transaction::initialize_thread();

   int start = *(int*)threadId;
   int startingValue = start * kMaxInserts;
   int threadInserts = 0;

   if (kInsertSameValues) startingValue = 0;

   int endingValue = startingValue + kMaxInserts;

   idleUntilAllThreadsHaveReached(*(int*)threadId);

   if (kStartingTime == startTimer) startTimer = time(NULL);

   //--------------------------------------------------------------------------
   // do the transactional inserts. this is the main transactional loop.
   //--------------------------------------------------------------------------
   int i;
   for (i = startingValue; i < endingValue; ++i)
   {
      node.value() = i;
      //llist->insert(node);
      llist->lock_insert(node);
   }

   if (kDoMove)
   {
      newSyntaxNS::list_node<list_node_type> node1, node2;

      for (int j = startingValue; j < endingValue; ++j)
      {
         node1.value() = j;
         //node2.value() = -j;
         llist->move(node1, node2);
      }

   }

   if (kDoLookup)
   {
      bool allFound = true;

      for (i = startingValue; i < endingValue; ++i)
      {
#if 0
         if (!llist->lookup(i))
         {
            allFound = false;
            std::cout << "Element not found: " << i << endl;
         }
#endif
      }
   }

   if (kDoRemoval)
   {
      for (i = startingValue; i < endingValue; ++i)
      {
         node.value() = i;
         llist->remove(node);
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
static void* TestLinkedListInserts(void *threadId)
{
   newSyntaxNS::list_node<list_node_type> node;
   transaction::initialize_thread();

   int start = *(int*)threadId;
   int startingValue = start * kMaxInserts;
   int threadInserts = 0;

   if (kInsertSameValues) startingValue = 0;

   int endingValue = startingValue + kMaxInserts;

   idleUntilAllThreadsHaveReached(*(int*)threadId);

   if (kStartingTime == startTimer) startTimer = time(NULL);

   //--------------------------------------------------------------------------
   // do the transactional inserts. this is the main transactional loop.
   //--------------------------------------------------------------------------
   int i;
   for (i = startingValue; i < endingValue; ++i)
   {
      node.value() = i;
      llist->insert(node);
   }

   if (kDoMove)
   {
      newSyntaxNS::list_node<list_node_type> node1, node2;

      for (int j = startingValue; j < endingValue; ++j)
      {
         node1.value() = j;
         //node2.value() = -j;
         llist->move(node1, node2);
      }

   }

   if (kDoLookup)
   {
      bool allFound = true;

      for (i = startingValue; i < endingValue; ++i)
      {
#if 0
         if (!llist->lookup(i))
         {
            allFound = false;
            std::cout << "Element not found: " << i << endl;
         }
#endif
      }
   }

   if (kDoRemoval)
   {
      for (i = startingValue; i < endingValue; ++i)
      {
         node.value() = i;
         llist->remove(node);
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
static void* stall(void *)
{
   transaction::initialize_thread();

   transaction::lock_(&L2);

   SLEEP(10000);

   transaction::unlock_(&L2);

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void TestNested2()
{
   atomic(t)
   {
      //t.force_to_abort();
      --t.w(x);
   }
   before_retry 
   {
      cout << "TestNested2 caught exception" << endl;
   }

}

///////////////////////////////////////////////////////////////////////////////
static void TestNested()
{
   atomic(t)
   {
      ++t.w(x);
      TestNested2();
   } 
   before_retry 
   {
      cout << "TestNested caught exception" << endl;
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
void TestLinkedListWithUsingLocks()
{
   static std::vector<int> runVector;
   int iterations = 0;

   llist = new newSyntaxNS::LinkedList<list_node_type>;

   transaction::initialize();
   transaction::initialize_thread();

   //transaction::do_tm_lock_protection();
   transaction::do_tx_lock_protection();

   //transaction::tm_lock_conflict(&L);
   //transaction::tm_lock_conflict(&L2);
   //transaction::tm_lock_conflict(&L3);

   //TestNested();

   //exit(0);

   pthread_t *second_thread = new pthread_t;

   pthread_create(second_thread, NULL, stall, (void*)NULL);

   TestTransactionInsideLock();
   TestEarlyRelease();

   exit(0);









   transaction::tm_lock_conflict(llist->get_list_lock());

#if LOGGING_COMMITS_AND_ABORTS
   transaction::enableLoggingOfAbortAndCommitSetSize();
#endif

   pthread_t *threads = new pthread_t[kMaxThreads];
   int *threadId = new int[kMaxThreads];

   //--------------------------------------------------------------------------
   // Reset barrier variables before creating any threads. Otherwise, it is
   // possible for the first thread 
   //--------------------------------------------------------------------------
   threadsFinished.value() = 0;
   threadsStarted.value() = 0;
   startTimer = kStartingTime;
   endTimer = 0;

   for (int j = 0; j < kMaxThreads - 1; ++j)
   {
      threadId[j] = j;
      //if (0 == j % 2) pthread_create(&threads[j], NULL, TestLinkedListInserts, (void *)&threadId[j]);
      //else pthread_create(&threads[j], NULL, TestLinkedListInsertsWithLocks, (void *)&threadId[j]);

      pthread_create(&threads[j], NULL, tx_foo, (void*)&threadId[j]);
   }

   int mainThreadId = kMaxThreads-1;

   //TestLinkedListInsertsWithLocks((void*)&mainThreadId);
   lk_foo((void*)mainThreadId);

   return;

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }

   int totalInserts = llist->walk_size();

   int averageRunTime = (int)(endTimer - startTimer);
   if (averageRunTime < 1) averageRunTime = 1;
   runVector.push_back(averageRunTime);

   int totalAverageRunTime = 0;
   for (int i = 0; i < (int)runVector.size(); ++i) totalAverageRunTime += runVector[i];

   totalAverageRunTime /= (int)runVector.size();


#ifdef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
   cout << "DEL_INVAL_";
#endif
   cout << "LL: DSTM_" << transaction::update_policy_string() << "   ";
   cout << "THRD: " << kMaxThreads << "   ";
   cout << "SIZE: " << totalInserts << "   ";
   cout << "TIME: " << averageRunTime << "   " << "AVE: " << totalAverageRunTime;
   cout << " TX_SEC: " << transaction::bookkeeping().commits() / (totalAverageRunTime * runVector.size()) << endl;
   cout << transaction::bookkeeping() << endl;

   if ((kInsertSameValues && totalInserts != kMaxInserts) || 
      (!kInsertSameValues && totalInserts != kMaxInserts * kMaxThreads))
   {
      std::cout << std::endl << std::endl << "###########################################################";
      std::cout << std::endl << "LOST ITEMS IN LINKED LIST - HALTING: " << totalInserts << std::endl;
      std::cout << "###########################################################" << std::endl << std::endl;

      ofstream out("testOutput.txt");
      llist->outputList(out);
      out.close();
   }

   ofstream outTest("testOutput.txt");
   llist->outputList(outTest);
   outTest.close();

#if LOGGING_COMMITS_AND_ABORTS
   std::ostringstream out;
   out << kMaxThreads << "_" << kMaxInserts;
   std::string typeOfRun = "linkedList_" + out.str();
   logCommitsAndAborts(typeOfRun);
#endif

   delete llist;
   delete threads;
   delete threadId;
}
