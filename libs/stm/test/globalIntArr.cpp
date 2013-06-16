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

//---------------------------------------------------------------------------
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
#include "transferFun.h"
#include "main.h"
#include "testRBTree.h"
#include <math.h>
#include <fstream>
#include <sstream>

//---------------------------------------------------------------------------
using namespace std; using namespace boost::stm;
using namespace nMain;

namespace nGlobalIntArr
{
   int const kMaxGlobalIntSize = 100000;
}

using namespace nGlobalIntArr;
int globalCountExe = 0;
boost::stm::native_trans<int> global_int;
boost::stm::native_trans<int> arr[kMaxGlobalIntSize];

int executionsPending = 0;

int outArr[kMaxGlobalIntSize];

//-------------------------------------------
// thread 2 executes this iteratively
//-------------------------------------------
void sum_arr(int out[])
{
  transaction t; 

  for (;; t.raise_priority())
  {
    try
    {
      for (int i = 0; i < kMaxArrSize; ++i)
      {
        out[i] = t.read(arr[i]).value();
      }

      t.end();
      return;
    }
    catch (aborted_transaction_exception&) 
    { t.restart(); }
  }
}


//-------------------------------------------
// thread 3 executes this iteratively
//-------------------------------------------
int set_int(int v)
{
  transaction t;
  t.set_priority(10 + (rand() % 90));

  for (;; t.raise_priority()) 
  {
    try 
    {
      int ret = t.read(global_int).value();
      t.write(global_int).value() = v;
      SLEEP(rand() % t.priority());
      if (0 == rand() % 1000)
      {
        for (int i = 0; i < kMaxArrSize; ++i)
        {
          outArr[i] = t.read(arr[i]).value();
        }
        // verify out and orig are the same
      }

      t.end();
      return ret;
    }
    catch (...) { t.restart(); }
  }
}

//-------------------------------------------
// thread 1 executes this iteratively
//-------------------------------------------
void set_arr(int val, int loc)
{
  for (transaction t ;; t.raise_priority())
  {
    try
    {
      t.write(arr[loc]).value() = val;
      t.end();
      break;
    }
    catch (aborted_transaction_exception&)
    { t.restart(); }
  }
}
//---------------------------------------------------------------------------
void* SetGlobalInt(void *threadId)
{
   transaction::initialize_thread();
   idleUntilAllThreadsHaveReached(*(int*)threadId);

   for (;globalCountExe < kMaxArrIter;)
   {
      set_arr(100, rand() % kMaxArrSize);
   }

   pthread_exit(threadId);
   return 0;
}

//---------------------------------------------------------------------------
void* SumGlobalArray(void *threadId)
{
   transaction::initialize_thread();
   idleUntilAllThreadsHaveReached(*(int*)threadId);

   SLEEP(10);

   int out[kMaxGlobalIntSize];
   int currentIter = 0;
   int percentDone = 0;

   for (globalCountExe = 0; globalCountExe < kMaxArrIter; ++globalCountExe)
   {
      ++currentIter;

      // output every 10% completed
      if (currentIter >= (kMaxArrIter / 10))
      {
         percentDone += 10;
         if (percentDone == 10) cout << "Percent done: " << percentDone << "%  ";
         else cout << percentDone << "%  ";

         if (percentDone >= 100) cout << endl;

         currentIter = 0;
      }

      sum_arr(out);
      ++executionsPending;
   }

   SLEEP(10);

   return 0;
}

//---------------------------------------------------------------------------
void* SetGlobalIntThread3(void *threadId)
{
   transaction::initialize_thread();
   idleUntilAllThreadsHaveReached(*(int*)threadId);

   for (;globalCountExe < kMaxArrIter;)
   {
      set_int(rand() % kMaxArrSize);
      --executionsPending;
   }

   pthread_exit(threadId);
   return 0;
}

//---------------------------------------------------------------------------
void TestGlobalIntArrayWithMultipleThreads()
{
   static std::vector<int> runVector;
   int i;

   for (i = 0; i < kMaxArrSize; ++i) arr[i].value() = 100;

   transaction::initialize();
   transaction::initialize_thread();

#if LOGGING_COMMITS_AND_ABORTS
   transaction::enableLoggingOfAbortAndCommitSetSize();
#endif

   int const kLocalThreads = 3;

   pthread_t* threads = new pthread_t[kLocalThreads];
   int* threadId = new int[kLocalThreads];

   //--------------------------------------------------------------------------
   // Reset barrier variables before creating any threads. Otherwise, it is
   // possible for the first thread 
   //--------------------------------------------------------------------------
   threadsFinished.value() = 0;
   threadsStarted.value() = 0;
   endTimer = 0;

   pthread_create(&threads[0], NULL, SetGlobalInt, (void *)&threadId[0]);
   pthread_create(&threads[1], NULL, SetGlobalIntThread3, (void *)&threadId[1]);

   int mainThreadId = kLocalThreads-1;

   startTimer = time(NULL);
   SumGlobalArray((void*)&mainThreadId);
   endTimer = time(NULL);

   int averageRunTime = (int)(endTimer - startTimer);
   if (averageRunTime < 1) averageRunTime = 1;
   runVector.push_back(averageRunTime);

   int totalAverageRunTime = 0;
   for (i = 0; i < (int)runVector.size(); ++i) totalAverageRunTime += runVector[i];

   totalAverageRunTime /= (int)runVector.size();

#ifdef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
   cout << "DEL_INVAL_";
#endif
   //cout << transaction::consistency_checking_string() << " ";
   cout << "(array size, copy iters, time, commits, priority aborts, total aborts)\t";
   cout << kMaxArrSize << "\t" << kMaxArrIter << "\t";
   cout << averageRunTime << "\t" << transaction::bookkeeping().commits() << "\t";
   cout << transaction::bookkeeping().abortPermDenied() << "\t";
   cout << transaction::bookkeeping().totalAborts() << endl;

#if LOGGING_COMMITS_AND_ABORTS
   std::ostringstream out;
   out << kMaxThreads << "_" << kMaxInserts;
   std::string typeOfRun = "rbTreeFun_" + out.str();
   logCommitsAndAborts(typeOfRun);
#endif

   //delete threads;
   //delete threadId;
}

