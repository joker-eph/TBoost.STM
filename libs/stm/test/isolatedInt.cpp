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
#include "isolatedInt.h"
#include <boost/stm/transaction.hpp>
#include "main.h"

static boost::stm::native_trans<int> gInt;
static boost::stm::native_trans<int> gInt2;

////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm;


///////////////////////////////////////////////////////////////////////////////
static void* TestIsolatedCount(void *threadId)
{
   transaction::initialize_thread();
   int start = *(int*)threadId;

   int startingValue = start * 100000;
   int endingValue = startingValue + kMaxInserts;
   volatile int oldVal;

   for (int i = startingValue; i < endingValue; ++i)
   {
      for (transaction t; ; t.restart())
      {
         try
         {
            t.make_isolated();
            //SLEEP(1);
            oldVal = gInt2.value();

            ++gInt2.value();
            
            if (oldVal + 1 != gInt2.value())
            {
               cout << "invariant changed" << endl;
            }
            t.end();
            //cout << gInt2.value() << endl;
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
static void* TestCount(void *threadId)
{
   transaction::initialize_thread();
   int start = *(int*)threadId;

   int startingValue = start * 100000;
   int endingValue = startingValue + kMaxInserts;

   for (int i = startingValue; i < endingValue; ++i)
   {
      for (transaction t; ; t.restart())
      {
         try
         {
            t.w(gInt2).value()++;

            if (1)//0 == rand() % 2) 
            {
               t.w(gInt).value()++;
            }
            t.end();
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
void TestIsolatedInt()
{
   transaction::initialize();

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
      pthread_create(&threads[j], NULL, TestCount, (void *)&threadId[j]);
   }

   int mainThreadId = kMaxThreads-1;

   TestIsolatedCount((void*)&mainThreadId);

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }
}

