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
#include <math.h>
#include "main.h"
#include "testRBTree.h"
#include <fstream>
#include <sstream>

boost::stm::native_trans<int> global_int2;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
namespace nMath
{
   double const constantE = 2.718281828;
}

using namespace nMath;

////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm;
using namespace nMain;

void* TestTreeTransferFunctionInserts(void *threadId)
{
   transaction::initialize_thread();

   int id = *(int*)threadId;
   int start = *(int*)threadId;
   int startingValue = start * kMaxInserts;
   int threadInserts = 0;

   if (kInsertSameValues) startingValue = 0;

   int endingValue = startingValue + kMaxInserts;

   idleUntilAllThreadsHaveReached(*(int*)threadId);

   if (kStartingTime == startTimer) startTimer = time(NULL);

#if 1

   for (;;)
   {
      try
      {
         transaction t;
         t.write(global_int2).value() = t.read(global_int2).value() + 1;
         t.end();
         break;
      }
      catch (aborted_transaction_exception&) {}
   }
#endif

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
void TestTransferFunctionMultipleThreads()
{
   static std::vector<int> runVector;
   int iterations = 0;

   global_int2.value() = 0;

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
      pthread_create(&threads[j], NULL, TestTreeTransferFunctionInserts, (void *)&threadId[j]);
   }

   int mainThreadId = kMaxThreads-1;

   TestTreeTransferFunctionInserts((void*)&mainThreadId);

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
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
   cout << "TF: DSTM_" << transaction::update_policy_string() << "   ";
   cout << "THRD: " << kMaxThreads << "   ";
   cout << "TIME: " << averageRunTime << "   " << "AVE: " << totalAverageRunTime;
   cout << " TX_SEC: " << transaction::bookkeeping().commits() / (totalAverageRunTime * runVector.size()) << endl;
   cout << transaction::bookkeeping() << endl;

   cout << "global int value: " << global_int2.value() << endl;

#if LOGGING_COMMITS_AND_ABORTS
   std::ostringstream out;
   out << kMaxThreads << "_" << kMaxInserts;
   std::string typeOfRun = "rbTreeFun_" + out.str();
   logCommitsAndAborts(typeOfRun);
#endif

   delete threads;
   delete threadId;
}

//---------------------------------------------------------------------------
void TransferFunction::performFunction()
{
   //------------------------------------------------------------------------
   // based on the transfer function of this object, perform the correct
   // operation and store its output in the output field
   //------------------------------------------------------------------------
   switch (fun())
   {
   case eHardLimit:
      output_ = input_ >= 0 ? 1 : 0;
      break;

   case eHardLimitSymmetric:
      output_ = input_ >= 0 ? 1 : -1;
      break;

   case eLinear:
      output_ = input_;
      break;

   case ePositiveLinear:
      output_ = input_ < 0 ? 0 : input_;
      break;

   case eSaturatingLinear:
      if (input_ <= 0) output_ = 0;
      else if (input_ >= 1) output_ = 1;
      else output_ = input_;
      break;

   case eSaturatingLinearSymmetric:
      if (input_ <= -1) output_ = -1;
      else if (input_ >= 1) output_ = 1;
      else output_ = input_;
      break;

   case eSigmoid:
      output_ = 1 / (1 + pow(constantE, -input_)); 
      break;

   case eTanSigmoid:
      output_ = (pow(constantE, input_) - pow(constantE, -input_)) 
         / (pow(constantE, input_) + pow(constantE, -input_)); 
      break;

   default:
      break;
   }

}
