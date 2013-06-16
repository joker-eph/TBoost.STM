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

#include "testHashMapAndLinkedListsWithLocks.h"
#include <fstream>
#include <sstream>

static LATM::HashMap<int> *globalHashMap = NULL;
static LATM::LinkedList<int> *globalLinkedList = NULL;

////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm; using namespace LATM::nHashMap;
using namespace nMain;


#if 0
native_trans<int> arr1[99], arr2[99];

void tx1() { /* no conflict */ }
void tx2() { /* no conflict */ }

void tx3() {
   for (transaction t;;t.restart())
      try {
         for (int i = 0; i < 99; ++i)
         {
           ++t.w(arr1[i]).value();
           ++t.w(arr2[i]).value();
         }
         t.end(); break;
      } catch (abort_tx&) {}
}

int lock1() {
   lock(L1); int sum = 0;
   for (int i = 0; i < 99; ++i) sum += arr1[i];
   unlock(L1); return sum;
}

int lock2() {
   lock(L2); int sum = 0;
   for (int i = 0; i < 99; ++i) sum += arr2[i];
   unlock(L2); return sum;
}

int lock3() { /* no conflict */ }

#endif



///////////////////////////////////////////////////////////////////////////////
static void* TestLinkedListInsertsWithLocks(void *threadId)
{
   LATM::list_node<int> node;
   transaction::initialize_thread();

   int start = *(int*)threadId;
   int startingValue = start * kMaxInserts;

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
      globalLinkedList->lock_insert(node);
   }

   if (kDoMove)
   {
      LATM::list_node<int> node1, node2;

      for (int j = startingValue; j < endingValue; ++j)
      {
         node1.value() = j;
         //node2.value() = -j;
         globalLinkedList->move(node1, node2);
      }

   }

   if (kDoLookup)
   {
      bool allFound = true;
      for (i = startingValue; i < endingValue; ++i)
      {
         if (!globalLinkedList->lookup(i))
         {
            allFound = false;
            std::cout << "Element not found: " << i << endl;
         }
      }
   }

   if (kDoRemoval)
   {
      for (i = startingValue; i < endingValue; ++i)
      {
         node.value() = i;
         globalLinkedList->remove(node);
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
   LATM::list_node<int> node;
   transaction::initialize_thread();

   int start = *(int*)threadId;
   int startingValue = start * kMaxInserts;

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
      globalLinkedList->insert(node);
   }

   if (kDoMove)
   {
      LATM::list_node<int> node1, node2;

      for (int j = startingValue; j < endingValue; ++j)
      {
         node1.value() = j;
         //node2.value() = -j;
         globalLinkedList->move(node1, node2);
      }

   }

   if (kDoLookup)
   {
      bool allFound = true;

      for (i = startingValue; i < endingValue; ++i)
      {
#if 0
         if (!globalLinkedList->lookup(i))
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
         globalLinkedList->remove(node);
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
static void* TestHashMapInserts(void *threadId)
{
   LATM::list_node<int> node;
   transaction::initialize_thread();

   int start = *(int*)threadId;
   int startingValue = start * kMaxInserts;

   if (kInsertSameValues) startingValue = 0;

   int endingValue = startingValue + kMaxInserts;

   idleUntilAllThreadsHaveReached(*(int*)threadId);
   //cout << "i: " << startingValue << endl;

   if (kStartingTime == startTimer) startTimer = time(NULL);

   //--------------------------------------------------------------------------
   // do the transactional inserts. this is the main transactional loop.
   //--------------------------------------------------------------------------
   int i;
   for (i = startingValue; i < endingValue; ++i)
   {
      node.value() = i;
      globalHashMap->insert(node);
   }

   if (kDoLookup)
   {
      bool allFound = true;

      for (i = startingValue; i < endingValue; ++i)
      {
         if (!globalHashMap->lookup(i))
         {
            allFound = false;
            std::cout << "Element not found: " << i << endl;
         }
      }
   }

   if (kDoRemoval)
   {
      for (i = startingValue; i < endingValue; ++i)
      {
         node.value() = i;
         globalHashMap->remove(node);
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
static void* TestHashMapInsertsWithLocks(void *threadId)
{
   LATM::list_node<int> node;
   transaction::initialize_thread();

   int start = *(int*)threadId;
   int startingValue = start * kMaxInserts;

   if (kInsertSameValues) startingValue = 0;

   int endingValue = startingValue + kMaxInserts;

   idleUntilAllThreadsHaveReached(*(int*)threadId);
   //cout << "i: " << startingValue << endl;

   if (kStartingTime == startTimer) startTimer = time(NULL);

   //--------------------------------------------------------------------------
   // do the transactional inserts. this is the main transactional loop.
   //--------------------------------------------------------------------------
   int i;
   for (i = startingValue; i < endingValue; ++i)
   {
      node.value() = i;
      globalHashMap->lock_insert(node);
   }

   if (kDoLookup)
   {
      bool allFound = true;

      for (i = startingValue; i < endingValue; ++i)
      {
         if (!globalHashMap->lock_lookup(i))
         {
            allFound = false;
            std::cout << "Element not found: " << i << endl;
         }
      }
   }

   if (kDoRemoval)
   {
      for (i = startingValue; i < endingValue; ++i)
      {
         node.value() = i;
         globalHashMap->remove(node);
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
static void DoHashMapAndLinkedListOutput(int threadsDoingHashMap, int threadsDoingLinkedList)
{
   int totalInserts = globalHashMap->walk_size();
   int runTime = (int)(endTimer - startTimer);

   if (runTime < 1) runTime = 1;

   cout << "LATM: " << transaction::latm_protection_str() << endl << endl;
   cout << "HT: DSTM_" << transaction::update_policy_string() << "   ";
   cout << "THRD: " << kMaxThreads << "   ";
   cout << "SIZE: " << totalInserts << "   ";
   cout << "TIME: " << runTime;
   cout << " TX_SEC: " << transaction::bookkeeping().commits() / runTime << endl;
   cout << transaction::bookkeeping() << endl;

   if ((kInsertSameValues && totalInserts != kMaxInserts) || 
      (!kInsertSameValues && totalInserts != kMaxInserts * threadsDoingHashMap))
   {
      std::cout << std::endl << std::endl << "###########################################################";
      std::cout << std::endl << "LOST ITEMS IN HASH MAP - HALTING: " << totalInserts << std::endl;
      std::cout << "###########################################################" << std::endl << std::endl;

      ofstream out("testOutput.txt");
      globalHashMap->outputList(out);
      out.close();
   }

   totalInserts = globalLinkedList->walk_size();

   cout << "LL: DSTM_" << transaction::update_policy_string() << "   ";
   cout << "THRD: " << kMaxThreads << "   ";
   cout << "SIZE: " << totalInserts << "   ";
   cout << "TIME: " << runTime << "   ";
   cout << " TX_SEC: " << transaction::bookkeeping().commits() / runTime << endl;
   cout << transaction::bookkeeping() << endl;

   if ((kInsertSameValues && totalInserts != kMaxInserts) || 
      (!kInsertSameValues && totalInserts != kMaxInserts * threadsDoingLinkedList))
   {
      std::cout << std::endl << std::endl << "###########################################################";
      std::cout << std::endl << "LOST ITEMS IN LINKED LIST - HALTING: " << totalInserts << std::endl;
      std::cout << "###########################################################" << std::endl << std::endl;

      ofstream out("testOutput.txt");
      globalLinkedList->outputList(out);
      out.close();
   }

}

///////////////////////////////////////////////////////////////////////////////
void DoHashMapAndLinkedListInitialization()
{
   globalHashMap = new LATM::HashMap<int>;
   globalLinkedList = new LATM::LinkedList<int>;
   transaction::initialize();

   if (transaction::doing_tm_lock_protection())
   {
      for (int k = 1; k < kBuckets; ++k)
      {
         transaction::tm_lock_conflict(globalHashMap->get_hash_lock(k));
      }
   }

   threadsFinished.value() = 0;
   threadsStarted.value() = 0;
   startTimer = kStartingTime;
   endTimer = 0;
}

///////////////////////////////////////////////////////////////////////////////
void TestHashMapAndLinkedListWithLocks()
{
   DoHashMapAndLinkedListInitialization();

   pthread_t *threads = new pthread_t[kMaxThreads];
   int *threadId = new int[kMaxThreads];

   int hashingThreads = 0, listThreads = 0;

   for (int j = 0; j < kMaxThreads - 1; ++j)
   {
      threadId[j] = j;
      if (0 == j % 2) pthread_create(&threads[j], NULL, TestHashMapInserts, (void *)&threadId[j]);
      else pthread_create(&threads[j], NULL, TestHashMapInsertsWithLocks, (void *)&threadId[j]);
      //pthread_create(&threads[j], NULL, TestLinkedListInserts, (void *)&threadId[j]);
      ++hashingThreads;
   }

   int mainThreadId = kMaxThreads-1;
   TestLinkedListInsertsWithLocks((void*)&mainThreadId);
   ++listThreads;

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }

   DoHashMapAndLinkedListOutput(hashingThreads, listThreads);

   delete globalHashMap;
   delete threads;
   delete threadId;
}

