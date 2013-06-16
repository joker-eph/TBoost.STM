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

#include "testInt.h"
#include <boost/stm/transaction.hpp>

Integer *globalInt = NULL;

////////////////////////////////////////////////////////////////////////////
namespace nIntTest
{
   int const kMaxInserts = 75000;
   int const kMaxThreads = 10;
}

////////////////////////////////////////////////////////////////////////////
using namespace std; using namespace boost::stm; using namespace nIntTest;

#if USING_INTEGER_IMPL
//MemoryPool<IntegerImpl> IntegerImpl::memory_(200000);
#endif

////////////////////////////////////////////////////////////////////////////
void innerFun()
{
   transaction t;
   ++t.write(*globalInt).value();
   t.end();
}

////////////////////////////////////////////////////////////////////////////
bool intThreadsHaveCommitted()
{
   if (globalInt->value() >= kMaxThreads * kMaxInserts) return true;
   else return false;
}


///////////////////////////////////////////////////////////////////////////////
void* TestCount(void *threadId)
{
   transaction::initialize_thread();
   int start = *(int*)threadId - 1;

   int startingValue = start * 100000;
   int endingValue = startingValue + kMaxInserts;

   for (int i = startingValue; i < endingValue; ++i)
   {
      while (true)
      {
         try { innerFun(); break; }
         catch (...) {}
      }
   }

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
void TestInt()
{
   transaction::initialize();
   globalInt = new Integer;

   pthread_t threads[kMaxThreads];
   int rc;

   for (int i = 0; i < kMaxThreads; ++i)
   {
      cout << "Creating thread: " << i << endl;
      rc = pthread_create(&threads[i], NULL, TestCount, (void *)&i);

      if (rc)
      {
         cerr << "ERROR; return code from pthread_create() is: " << rc << endl;
         exit(-1);
      }
   }

   while (!intThreadsHaveCommitted()) { SLEEP(70); }

   cout << "RUN_TIME: " << clock() << "\t";
   cout << "THREADS: " << kMaxThreads << "\t";
   cout << endl << transaction::bookkeeping();

   cout << endl << globalInt->value() << endl;
}

