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
#include "testLL_latm.h"

typedef int list_node_type;

int const kMaxThreadsWhenTestingSets = 128;
// Subtract one max linked list per complete set of four threads
int const kMaxLinkedLists = kMaxThreadsWhenTestingSets - ((kMaxThreadsWhenTestingSets + 1) / 4);
static LATM::LinkedList< list_node_type > *llist[kMaxLinkedLists] = { NULL };
bool usingSingleList = false;

////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm;
using namespace nMain;

////////////////////////////////////////////////////////////////////////////
int convertThreadIdToLinkedListIndex(int id)
{
   //------------------------------------------------------------------------------
   // In set testing, threads are created in groups of four, assigned sequentially
   // to sets of three linked lists with the last pair of threads sharing a list:
   //    trans -> list A
   //    lock  -> list B
   //    trans + lock -> list C
   //
   // Thus, for every set of four incremental thread ids, the linked list index 
   // increments only with the first three incremental thread ids.
   //------------------------------------------------------------------------------
   return id - ((id + 1) / 4);
}

///////////////////////////////////////////////////////////////////////////////
static void* TestLinkedListInsertsWithLocks(void *threadId)
{
   LATM::list_node<list_node_type> node;
   transaction::initialize_thread();

   int start = *(int*)threadId;
   int startingValue = start * kMaxInserts;
   int threadInserts = 0;

   if (kInsertSameValues) startingValue = 0;

   int endingValue = startingValue + kMaxInserts;

   int llindex = convertThreadIdToLinkedListIndex(start);
   if (usingSingleList) llindex = 0;

   //cout << "L" << llindex << endl;

   idleUntilAllThreadsHaveReached(*(int*)threadId);

   if (kStartingTime == startTimer) startTimer = time(NULL);

   //--------------------------------------------------------------------------
   // do the transactional inserts. this is the main transactional loop.
   //--------------------------------------------------------------------------
   int i;
   for (i = startingValue; i < endingValue; ++i)
   {
      node.value() = i;
      llist[llindex]->lock_insert(node);
   }


   if (kDoMove)
   {
      LATM::list_node<list_node_type> node1, node2;

      for (int j = startingValue; j < endingValue; ++j)
      {
         node1.value() = j;
         //node2.value() = -j;
         llist[llindex]->move(node1, node2);
      }

   }

   if (kDoLookup)
   {
      bool allFound = true;

      for (i = startingValue; i < endingValue; ++i)
      {
#if 0
         if (!llist[llindex]->lookup(i))
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
         llist[llindex]->remove(node);
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
   LATM::list_node<list_node_type> node;
   transaction::initialize_thread();

   int start = *(int*)threadId;
   int startingValue = start * kMaxInserts;
   int threadInserts = 0;

   if (kInsertSameValues) startingValue = 0;

   int endingValue = startingValue + kMaxInserts;

   int llindex = convertThreadIdToLinkedListIndex(start);
   if (usingSingleList) llindex = 0;

   //cout << "T" << llindex << endl;

   idleUntilAllThreadsHaveReached(*(int*)threadId);

   if (kStartingTime == startTimer) startTimer = time(NULL);

   //--------------------------------------------------------------------------
   // do the transactional inserts. this is the main transactional loop.
   //--------------------------------------------------------------------------
   int i;
   for (i = startingValue; i < endingValue; ++i)
   {
      node.value() = i;
      llist[llindex]->insert(node);
   }

   if (kDoMove)
   {
      LATM::list_node<list_node_type> node1, node2;

      for (int j = startingValue; j < endingValue; ++j)
      {
         node1.value() = j;
         //node2.value() = -j;
         llist[llindex]->move(node1, node2);
      }

   }

   if (kDoLookup)
   {
      bool allFound = true;

      for (i = startingValue; i < endingValue; ++i)
      {
#if 0
         if (!llist[llindex]->lookup(i))
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
         llist[llindex]->remove(node);
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
void TestLinkedListSetsWithLocks()
{
   static std::vector<int> runVector;
   int iterations = 0;

   usingSingleList = false;

   if (kMaxThreads > kMaxThreadsWhenTestingSets)
   {
      kMaxThreads = kMaxThreadsWhenTestingSets;
      std::cout << "Limiting max threads to " << kMaxThreadsWhenTestingSets << "." << endl;
   }
   else if (kMaxThreads % 4 != 0)
   {
      // threads are always in multiples of four: transaction, lock, and transaction + lock
      kMaxThreads += (4 - (kMaxThreads % 4));
      std::cout << "Rounding max threads to the next multiple of 4 (" << kMaxThreads << ")." << endl;
   }

   kMainThreadId = kMaxThreads-1;
   int lists = (kMaxThreads / 4) * 3;

   for (int k = 0; k < kMaxThreads; ++k)
   {
      llist[k] = new LATM::LinkedList<list_node_type>;   

      transaction::initialize();
      transaction::initialize_thread();

      int index = convertThreadIdToLinkedListIndex(k);

      //----------------------------------------------------------------
      // every 4th thread works on the same linked list that
      // the 3rd thread is working on, so only add that conflict
      //----------------------------------------------------------------
      if (transaction::doing_tm_lock_protection() && 0 == (k+1) % 2)
      {
         //transaction::add_tm_conflicting_lock(llist[index]->get_list_lock());
         //cout << "TM-lock conflict added: " << index << endl;
      }
   }


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
      if (0 == j % 2) pthread_create(&threads[j], NULL, TestLinkedListInserts, (void *)&threadId[j]);
      else pthread_create(&threads[j], NULL, TestLinkedListInsertsWithLocks, (void *)&threadId[j]);
   }

   int mainThreadId = kMaxThreads-1;

   TestLinkedListInsertsWithLocks((void*)&mainThreadId);

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }

   int totalInserts = 0;
   for (int m = 0; m < kMaxThreads; ++m)
   {
      totalInserts += llist[m]->walk_size();
   }

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

#if 0
   if ((kInsertSameValues && totalInserts != kMaxInserts) || 
      (!kInsertSameValues && totalInserts != kMaxInserts * kMaxThreads))
   {
      std::cout << std::endl << std::endl << "###########################################################";
      std::cout << std::endl << "LOST ITEMS IN LINKED LIST - HALTING: " << totalInserts << std::endl;
      std::cout << "###########################################################" << std::endl << std::endl;
   }

   ofstream outTest("testOutput.txt");
   for (int p = 0; p < kMaxThreads; ++p)
   {
      llist[p]->outputList(outTest);
   }
   outTest.close();
#endif

#if LOGGING_COMMITS_AND_ABORTS
   std::ostringstream out;
   out << kMaxThreads << "_" << kMaxInserts;
   std::string typeOfRun = "linkedList_" + out.str();
   logCommitsAndAborts(typeOfRun);
#endif

   for (int q = 0; q < kMaxThreads; ++q)
   {
      delete llist[q];
   }
   delete threads;
   delete threadId;
}
