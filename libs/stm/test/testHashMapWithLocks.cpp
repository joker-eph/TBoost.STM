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

#include "testHashMapWithLocks.h"
#include <fstream>
#include <sstream>

static LATM::HashMap<int> *globalHashMap = NULL;

////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm; using namespace LATM::nHashMap; 
using namespace nMain;

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
static void DoHashMapOutput()
{
   int totalInserts = globalHashMap->walk_size();
   int runTime = (int)(endTimer - startTimer);

   if (runTime < 1) runTime = 1;

   cout << "HT: DSTM_" << transaction::update_policy_string() << "   ";
   cout << "THRD: " << kMaxThreads << "   ";
   cout << "SIZE: " << totalInserts << "   ";
   cout << "TIME: " << runTime;
   cout << " TX_SEC: " << transaction::bookkeeping().commits() / runTime << endl;
   cout << transaction::bookkeeping() << endl;

   if ((kInsertSameValues && totalInserts != kMaxInserts) || 
      (!kInsertSameValues && totalInserts != kMaxInserts * kMaxThreads))
   {
      std::cout << std::endl << std::endl << "###########################################################";
      std::cout << std::endl << "LOST ITEMS IN HASH MAP - HALTING: " << totalInserts << std::endl;
      std::cout << "###########################################################" << std::endl << std::endl;

      ofstream out("testOutput.txt");
      globalHashMap->outputList(out);
      out.close();
   }
}

///////////////////////////////////////////////////////////////////////////////
void DoHashMapInitialization()
{
   globalHashMap = new LATM::HashMap<int>;
   transaction::initialize();

   transaction::do_full_lock_protection();

#if 0
   transaction::do_tm_conflicting_lock_protection();

   for (int k = 1; k < kBuckets; ++k)
   {
      transaction::add_tm_conflicting_lock(globalHashMap->get_hash_lock(k));
   }
#endif

   threadsFinished.value() = 0;
   threadsStarted.value() = 0;
   startTimer = kStartingTime;
   endTimer = 0;
}

///////////////////////////////////////////////////////////////////////////////
void TestHashMapWithLocks()
{
   DoHashMapInitialization();

   pthread_t *threads = new pthread_t[kMaxThreads];
   int *threadId = new int[kMaxThreads];

   for (int j = 0; j < kMaxThreads - 1; ++j)
   {
      threadId[j] = j;
      if (0 == j % 2) pthread_create(&threads[j], NULL, TestHashMapInserts, (void *)&threadId[j]);
      else pthread_create(&threads[j], NULL, TestHashMapInsertsWithLocks, (void *)&threadId[j]);
   }

   int mainThreadId = kMaxThreads-1;
   TestHashMapInsertsWithLocks((void*)&mainThreadId);

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }

   DoHashMapOutput();

   delete globalHashMap;
   delete threads;
   delete threadId;
}

