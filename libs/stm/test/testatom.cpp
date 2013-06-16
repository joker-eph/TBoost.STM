
//#include <boost/thread.hpp>
//#include <boost/random.hpp>
#include <boost/stm.hpp>
#include <cstdlib>
#include <ctime>
#include "testatom.h"
#include "main.h"

namespace stm = boost::stm;

const int MAX_TRANSFER = 500;
const int THREADS = 10;
const int ACCOUNTS = 10;
const int INITIAL = 10000;

class Account
{
	stm::native_trans<int> m_balance;

public:
	void add(int diff);
	void remove(int diff);
	int get() const;
};

void Account::add(int amount)
{
	using namespace stm;
	atomic(tx) {
		tx.w(m_balance) += amount;
	} end_atom
}

void Account::remove(int amount)
{
	add(-amount);
}

int Account::get() const
{
	using namespace stm;
	int v;
	atomic(tx) {
		v = tx.r(m_balance);
	} end_atom;
	return v;
}

Account accounts[ACCOUNTS];

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void* AccountEntry(void* threadId)
{
   static bool first = true;

	stm::transaction::initialize_thread();
   int start = *(int*)threadId;

	using namespace stm;
	for(int i = 0; i < 100; ++i) {
		int a1 = rand() % ACCOUNTS;
		int a2 = rand() % ACCOUNTS;
		int amount = rand() % MAX_TRANSFER;

		atomic(tx) {
			accounts[a1].remove(amount);
			accounts[a2].add(amount);
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
int testAccounts()
{

   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "Beginning account processing: " << ACCOUNTS << " accounts." << std::endl;

   kMaxThreads = THREADS;

	stm::transaction::initialize();
	stm::transaction::initialize_thread();
	srand((unsigned int)(time(NULL)));

   int i = 0;
	for(i = 0; i < ACCOUNTS; ++i) {
		// Everyone starts with some.
		accounts[i].add(INITIAL);
	}

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
      pthread_create(&threads[j], NULL, AccountEntry, (void *)&threadId[j]);
   }

   int mainThreadId = kMaxThreads-1;
   kMainThreadId = kMaxThreads-1;

   AccountEntry((void*)&mainThreadId);

   while (true)
   {
      if (threadsFinished.value() == kMaxThreads) break;
      SLEEP(10);
   }

   std::cout << "Ending account processing." << std::endl;

	int total = 0;
	for(i = 0; i < ACCOUNTS; ++i) {
		total += accounts[i].get();
      std::cout << "account[" << i << "]: " << accounts[i].get() << std::endl;

	}

   std::cout << "\ntotal:  " << total << std::endl;

	if(total != INITIAL * ACCOUNTS) {
		std::cout << "Preservation of money violated! " << total << "\n";
	} else {
		std::cout << "All fine.\n";
	}

   return 0;
}
