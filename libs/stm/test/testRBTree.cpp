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

#include "main.h"
#include "testRBTree.h"
#include <fstream>
#include <sstream>

RedBlackTree<int> *rbTree = NULL;

////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm;
using namespace nMain;

///////////////////////////////////////////////////////////////////////////////
void* TestRedBlackTreeInserts(void *threadId)
{
   RedBlackNode<int> node;
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
      rbTree->insert(node);
   }

   if (kDoLookup)
   {
      bool allFound = true;
      int *found = NULL;

      for (i = startingValue; i < endingValue; ++i)
      {
         if (!rbTree->lookup(i, found))
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
         rbTree->remove(node);
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
void TestRedBlackTreeWithMultipleThreads()
{
   static std::vector<int> runVector;

   rbTree = new RedBlackTree<int>;
   transaction::initialize();
   transaction::initialize_thread();

#if LOGGING_COMMITS_AND_ABORTS
   transaction::enableLoggingOfAbortAndCommitSetSize();
#endif

   pthread_t* threads = new pthread_t[kMaxThreads];
   int* threadId = new int[kMaxThreads];

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
      pthread_create(&threads[j], NULL, TestRedBlackTreeInserts, (void *)&threadId[j]);
   }

   int mainThreadId = kMaxThreads-1;

   TestRedBlackTreeInserts((void*)&mainThreadId);

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }

   int totalInserts = rbTree->walk_size();

   int averageRunTime = (int)(endTimer - startTimer);
   if (averageRunTime < 1) averageRunTime = 1;
   runVector.push_back(averageRunTime);

   int totalAverageRunTime = 0;
   for (int i = 0; i < (int)runVector.size(); ++i) totalAverageRunTime += runVector[i];

   totalAverageRunTime /= (int)runVector.size();

#ifdef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
   cout << "DEL_INVAL_";
#endif
   cout << "RB: DSTM_" << transaction::update_policy_string() << "   ";
   cout << "THRD: " << kMaxThreads << "   ";
   cout << "TX: " << transaction::bookkeeping().commits() << "   ";
   cout << "TIME: " << averageRunTime << "   " << "AVE: " << totalAverageRunTime;
   cout << " TX_SEC: " << transaction::bookkeeping().commits() / (totalAverageRunTime * runVector.size()) << endl;
   cout << transaction::bookkeeping() << endl;

   transaction::terminate_thread();

#if 0
   if ((kInsertSameValues && totalInserts != kMaxInserts) || 
      (!kInsertSameValues && totalInserts != kMaxInserts * kMaxThreads))
   {
      std::cout << std::endl << std::endl << "###########################################################";
      std::cout << std::endl << "LOST ITEMS IN RBTREE - HALTING: " << totalInserts << std::endl;
      std::cout << "###########################################################" << std::endl << std::endl;

      ofstream out("testOutput.txt");
      rbTree->print(out);
   }
#endif

#if LOGGING_COMMITS_AND_ABORTS
   std::ostringstream out;
   out << kMaxThreads << "_" << kMaxInserts;
   std::string typeOfRun = "rbTree_" + out.str();
   logCommitsAndAborts(typeOfRun);
#endif

   rbTree->cheap_clear();
   delete rbTree;
   delete threads;
   delete threadId;
}

