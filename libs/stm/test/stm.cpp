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

////////////////////////////////////////////////////////////////////////////
#include <boost/stm/transaction.hpp>
#include <boost/stm/contention_manager.hpp>
#include "main.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <pthread.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include "testLinkedList.h"
#include "testHashMap.h"
#include "testRBTree.h"
#include "usingLockTx.h"
#include "nestedTxs.h"
#include "testLL_latm.h"
#include "testHT_latm.h"
#include "smart.h"
#include "pointer_test.h"
#include "testatom.h"
#include "testEmbedded.h"
#include "testBufferedDelete.h"
#if 0
#include "testLinkedListWithLocks.h"
#include "testHashMapAndLinkedListsWithLocks.h"
#include "testHashMapWithLocks.h"
#include "irrevocableInt.h"
#include "isolatedInt.h"
#include "isolatedIntLockInTx.h"
#include "isolatedComposedIntLockInTx.h"
#include "isolatedComposedIntLockInTx2.h"
#include "txLinearLock.h"
#include "lotExample.h"
#include "litExample.h"
#endif

using namespace boost::stm; using namespace nMain; using namespace std;

boost::stm::native_trans<int> threadsFinished;
boost::stm::native_trans<int> threadsStarted;
time_t startTimer = kStartingTime;
time_t endTimer = 0;
eWorkType gWorkLoadType;

#ifndef BOOST_STM_USE_BOOST_MUTEX
static Mutex finishLock = PTHREAD_MUTEX_INITIALIZER;
#else
static Mutex finishLock;
#endif
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int kMaxArrSize = 10;
int kMaxArrIter = 10;
int kMaxIterations = 1;
int kMaxInserts = 100;
bool kInsertSameValues = false;
bool kDoRemoval = false;
bool kDoLookup = false;
bool kDoMove = false;
bool kMoveSemantics = false;
std::string bench = "rbtree";
std::string updateMethod = "deferred";
std::string insertAmount = "50000";

int kMaxThreads = 2;
int kMainThreadId = kMaxThreads-1;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void usage()
{
   cout << "DracoSTM usage:" << endl << endl;
   cout << "  -bench <name> - 'rbtree', 'linkedlist', 'hashmap' (or 'hashtable')" << endl;
   cout << "                  'using_linkedlist'" << endl;
   cout << "                  'nested_tx'" << endl;
   cout << "                  'ht'" << endl;
   cout << "                  'll'" << endl;
   cout << "                  '1WNR'" << endl;
   cout << "                  'smart'" << endl;
   cout << "                  'pointer'" << endl;
   cout << "                  'accounts'" << endl;
   cout << "                  'embedded'" << endl;
   cout << "                  'delete'" << endl;
   cout << "  -def          - do deferred updating transactions" << endl;
   cout << "  -dir          - do direct updating transactions" << endl;
   cout << "  -latm <name>  - 'full', 'tm', 'tx'" << endl;
   cout << "  -h            - shows this help (usage) output" << endl;
   cout << "  -inserts <#>  - sets the # of inserts per container per thread" << endl;
   cout << "  -threads <#>  - sets the # of threads" << endl;
   cout << "  -lookup       - performs individual lookup after inserts" << endl;
   cout << "  -remove       - performs individual remove after inserts/lookup" << endl;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void setupEnvironment(int argc, char **argv)
{
   for (int i = 1; i < argc; ++i)
   {
      std::string first = argv[i];

      if (first == "-def") transaction::do_deferred_updating();
      else if (first == "-dir") transaction::do_direct_updating();
      else if (first == "-lookup") kDoLookup = true;
      else if (first == "-remove") kDoRemoval = true;
      else if (first == "-inserts")
      {
         kMaxInserts = atoi(argv[++i]);
      }
      else if (first == "-threads")
      {
         kMaxThreads = atoi(argv[++i]);
         kMainThreadId = kMaxThreads-1;
      }
      else if (first == "-moveSemantics")
      {
         kMoveSemantics = true;
      }
      else if (first == "-maxArrSize")
      {
         kMaxArrSize = atoi(argv[++i]);
      }
      else if (first == "-maxArrIter")
      {
         kMaxArrIter = atoi(argv[++i]);
      }
      else if (first == "-bench") bench = argv[++i];
      else if (first == "-cm")
      {
         std::string cmType = argv[++i];
         if (cmType == "iAggr") currentCm = iAggr;
         else if (cmType == "iPrio") currentCm = iPrio;
         else if (cmType == "iFair") currentCm = iFair;
         else if (cmType == "threadFair") currentCm = threadFair;
         else if (cmType == "iBalanced") currentCm = iBalanced;
         else 
         {
            cout << "invalid CM, exiting: " << endl;
            cout << first << cmType << endl;
            exit(0);
         }
      }
      else if (first == "-latm")
      {
#if PERFORMING_LATM

         std::string latmType = argv[++i];
         if (latmType == "full") transaction::do_full_lock_protection();
         else if (latmType == "tm") transaction::do_tm_lock_protection();
         else if (latmType == "tx") transaction::do_tx_lock_protection();
         else 
         {
            cout << "invalid LATM protection type, exiting" << endl;
            cout << first << latmType << endl;
            exit(0);
         }
#endif
      }
      else
      {
         usage();
         exit(0);
      }
   }
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
   transaction::enable_dynamic_priority_assignment();

   setupEnvironment(argc, argv);

   cout << "Current CM: " << currentCm << "\t";

   for (int i = 0; i < kMaxIterations; ++i)
   {
      if ("rbtree" == bench) TestRedBlackTreeWithMultipleThreads();
      else if ("linkedlist" == bench) TestLinkedListWithMultipleThreads();
      else if ("hashmap" == bench || "hashtable" == bench) TestHashMapWithMultipleThreads();
      else if ("using_linkedlist" == bench) TestLinkedListWithUsingLocks();
      else if ("nested_tx" == bench) NestedTxTest();
      else if ("ht" == bench) TestHashTableSetsWithLocks();
      else if ("ll" == bench) TestLinkedListSetsWithLocks();
      else if ("1WNR" == bench) Test1writerNreadersWithMultipleThreads();
      else if ("smart" == bench) test_smart();
      else if ("pointer" == bench) pointer_test();
      else if ("accounts" == bench) testAccounts();
      else if ("embedded" == bench) testEmbedded();
      else if ("delete" == bench) testBufferedDelete();
#if 0
      else if ("linkedlist_w_locks" == bench) TestLinkedListWithLocks();
      else if ("hashmap_w_locks" == bench) TestHashMapWithLocks();
      else if ("list_hash_w_locks" == bench) TestHashMapAndLinkedListWithLocks();
      else if ("irrevocable_int" == bench) TestIrrevocableInt();
      else if ("isolated_int" == bench) TestIsolatedInt();
      else if ("isolated_int_lock" == bench) TestIsolatedIntLockInTx();
      else if ("isolated_composed_int_lock" == bench) TestIsolatedComposedIntLockInTx();
      else if ("isolated_composed_int_lock2" == bench) TestIsolatedComposedIntLockInTx2();
      else if ("tx_linear_lock" == bench) TestTxLinearLock();
      else if ("lot_example" == bench) TestLotExample();
      else if ("lit_example" == bench) TestLitExample();
#endif
      else { usage(); return 0; }
   }

   return 0;
}

//-----------------------------------------------------------------------------
void idleUntilAllThreadsHaveReached(int const &threadId)
{
   lock(&finishLock);
   threadsStarted.value()++;
   unlock(&finishLock);

   while (threadsStarted.value() != kMaxThreads) SLEEP(5);
}

//-----------------------------------------------------------------------------
void finishThread()
{
   lock(&finishLock);
   threadsFinished.value()++;
   unlock(&finishLock);
}

//-----------------------------------------------------------------------------
void finishThread(int const &threadId)
{
   lock(&finishLock);
   threadsFinished.value()++;
   unlock(&finishLock);
}

//-----------------------------------------------------------------------------
void logCommitsAndAborts(std::string const &typeOfRun)
{
   ofstream aborts("abortSetSize.txt", std::ios::app);

   transaction_bookkeeping::AbortHistory::const_iterator j = 
      transaction::bookkeeping().getAbortWriteSetList().begin();
   transaction_bookkeeping::AbortHistory::const_iterator k = 
      transaction::bookkeeping().getAbortReadSetList().begin();

   transaction_bookkeeping::CommitHistory::const_iterator l = 
      transaction::bookkeeping().getCommitWriteSetList().begin();
   transaction_bookkeeping::CommitHistory::const_iterator m = 
      transaction::bookkeeping().getCommitReadSetList().begin();

   std::vector<double> percentList;

   for (;k != transaction::bookkeeping().getAbortReadSetList().end(); ++k, ++j, ++l, ++m)
   {
      if (j->second + k->second > m->second + l->second) continue;

#if 0
      char *space = k->first.commitId_ > 10 ? "   " : k->first.commitId_ > 100 ? "  " : " ";

      aborts << k->first.threadId_ << "\t\t" << space
             << k->first.commitId_ << "\t\t" << k->second << "\t\t" << j->second 
             << "\t\t\t\t" << m->second << "\t\t" << l->second << "\t\t";
#endif
      double percent = double(j->second + k->second) / double(m->second + l->second);
#if 0
      aborts << percent << endl;
#endif
      percentList.push_back(percent);
   }

   double percentTotal = 0.0;

   for (int i = 0; i < (int)percentList.size(); ++i) percentTotal += percentList[i];

   percentTotal = percentTotal / double(percentList.size());

   uint32 abortCount = transaction::bookkeeping().getAbortReadSetList().size() 
      + transaction::bookkeeping().getAbortWriteSetList().size();

   aborts << typeOfRun.c_str() << "  total_aborts: " << transaction::bookkeeping().totalAborts() 
          << "\t\taborted_r+w_count: " <<  abortCount
          << "\t\tabort_ops/commit_ops: " << percentTotal << endl;

   aborts.close();
}


