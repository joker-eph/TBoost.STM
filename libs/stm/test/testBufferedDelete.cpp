
//#include <boost/thread.hpp>
//#include <boost/random.hpp>
#include <boost/stm.hpp>
#include <cstdlib>
#include <ctime>
#include "testatom.h"
#include "main.h"

namespace stm = boost::stm;


class Simple : public boost::stm::transaction_object< Simple >
{
public:

   int val_;
};

Simple *instance = NULL;

using namespace stm;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void* BufferedDeleteEntry(void* threadId)
{
	stm::transaction::initialize_thread();
   int start = *(int*)threadId;

	for (int i = 0; i < 10000; ++i) 
   {
		atomic(tx) {
         Simple const *ref = &tx.read(*instance);

         for (int j = 0; j < 50; ++j)
         {
            cout << i << ", " << j << ":\t" << ref->val_ << endl;
         }
		} end_atom
	}

   finishThread(start);

   if (*(int*)threadId != kMainThreadId)
   {
      transaction::terminate_thread();
      pthread_exit(threadId);
   }

   return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int testBufferedDelete()
{
	stm::transaction::initialize();
	stm::transaction::initialize_thread();
	srand((unsigned int)(time(NULL)));

   stm::transaction::do_deferred_updating();

   instance = new Simple;

   instance->val_ = 0;

   pthread_t *threads = new pthread_t[kMaxThreads];
   int *threadId = new int[kMaxThreads];

   //--------------------------------------------------------------------------
   // Reset barrier variables before creating any threads. Otherwise, it is
   // possible for the first thread 
   //--------------------------------------------------------------------------
   threadsFinished.value() = 0;
   threadsStarted.value() = 0;
   startTimer = nMain::kStartingTime;
   endTimer = 0;

   for (int j = 0; j < kMaxThreads - 1; ++j)
   {
      threadId[j] = j;
      pthread_create(&threads[j], NULL, BufferedDeleteEntry, (void *)&threadId[j]);
   }

   int mainThreadId = kMaxThreads-1;
   kMainThreadId = kMaxThreads-1;

   atomic(tx)
   {
      cout << "in delete" << endl;
      tx.delete_memory(*instance);
   } end_atom

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }

   return 0;
}
