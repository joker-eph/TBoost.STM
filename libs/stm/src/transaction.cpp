//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Justin E. Gottchlich 2009.
// (C) Copyright Vicente J. Botet Escriba 2009.
// Distributed under the Boost
// Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or
// copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/stm for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/stm/transaction.hpp>
#include <boost/stm/contention_manager.hpp>
#include <iostream>

using namespace std;
using namespace boost::stm;

#if CAPTURING_PROFILE_DATA
std::string const kProfilerVersion = "v.2";
#endif

///////////////////////////////////////////////////////////////////////////////
// Static initialization
///////////////////////////////////////////////////////////////////////////////
transaction::InflightTxes transaction::transactionsInFlight_;
transaction::MutexSet transaction::latmLockedLocks_;
transaction::MutexThreadSetMap transaction::latmLockedLocksAndThreadIdsMap_;
transaction::MutexThreadMap transaction::latmLockedLocksOfThreadMap_;
transaction::ThreadSizetMap transaction::threadCommitMap_;
transaction::MutexSet transaction::tmConflictingLocks_;
transaction::DeletionBuffer transaction::deletionBuffer_;
transaction::ThreadTransactionsStack transaction::threadTransactionsStack_;
transaction::MapOfTxObjects transaction::threadBoundObjects_;

#if CAPTURING_PROFILE_DATA
transaction::ThreadOstringStream transaction::threadOstringStream_;
transaction::ThreadMapTxFileAndNumber transaction::threadFileAndNumberMap_;
#endif

size_t transaction::global_clock_ = 0;
size_t transaction::stalls_ = 0;

bool transaction::dynamicPriorityAssignment_ = false;
bool transaction::direct_updating_ = false;
bool transaction::directLateWriteReadConflict_ = false;
bool transaction::usingMoveSemantics_ = false;

pthread_mutexattr_t transaction::transactionMutexAttribute_;

Mutex transaction::transactionsInFlightMutex_;
Mutex transaction::transactionMutex_;
Mutex transaction::deletionBufferMutex_;
Mutex transaction::latmMutex_;

boost::stm::LatmType transaction::eLatmType_ = eFullLatmProtection;
std::ofstream transaction::logFile_;

#if USE_STM_MEMORY_MANAGER
#ifndef BOOST_STM_USE_BOOST_MUTEX
Mutex base_transaction_object::transactionObjectMutex_ = PTHREAD_MUTEX_INITIALIZER;
#else
boost::mutex base_transaction_object::transactionObjectMutex_;
#endif
boost::stm::MemoryPool<base_transaction_object> base_transaction_object::memory_(16384);
#endif

bool transaction::initialized_ = false;
///////////////////////////////////////////////////////////////////////////////
// first param = initialSleepTime (millis)
// second param = sleepIncrease factor (initialSleepTime * factor)
// third param = # of increases before resetting
///////////////////////////////////////////////////////////////////////////////
base_contention_manager *transaction::cm_ =
    new ExceptAndBackOffOnAbortNoticeCM(0, 0, 0);
//    new DefaultContentionManager();
//    new NoExceptionOnAbortNoticeOnReadWritesCM();
//    new DefaultContentionManager();
//    new ExceptAndBackOffOnAbortNoticeCM(5, 2, 10);
transaction_bookkeeping transaction::bookkeeping_;


#ifndef USE_SINGLE_THREAD_CONTEXT_MAP
#if PERFORMING_LATM
#if USING_TRANSACTION_SPECIFIC_LATM
transaction::ThreadMutexSetContainer transaction::threadConflictingMutexes_;
#endif
transaction::ThreadMutexSetContainer transaction::threadObtainedLocks_;
transaction::ThreadMutexSetContainer transaction::threadCurrentlyLockedLocks_;
#endif
transaction::ThreadWriteContainer transaction::threadWriteLists_;
transaction::ThreadReadContainer transaction::threadReadLists_;
//transaction::ThreadEagerReadContainer transaction::threadEagerReadLists_;
transaction::ThreadMemoryContainerList transaction::threadNewMemoryLists_;
transaction::ThreadMemoryContainerList transaction::threadDeletedMemoryLists_;
transaction::ThreadTxTypeContainer transaction::threadTxTypeLists_;
transaction::ThreadBloomFilterList transaction::threadBloomFilterLists_;
transaction::ThreadBloomFilterList transaction::threadWBloomFilterLists_;
transaction::ThreadBoolContainer transaction::threadForcedToAbortLists_;

transaction::ThreadMutexContainer transaction::threadMutexes_;
transaction::ThreadBoolContainer transaction::threadBlockedLists_;
#else

#if PERFORMING_LATM
#if USING_TRANSACTION_SPECIFIC_LATM
transaction::ThreadMutexSetContainer transaction::threadConflictingMutexes_;
#endif
transaction::ThreadMutexSetContainer transaction::threadObtainedLocks_;
transaction::ThreadMutexSetContainer transaction::threadCurrentlyLockedLocks_;
#endif

transaction::ThreadMutexContainer transaction::threadMutexes_;
transaction::ThreadBoolContainer transaction::threadBlockedLists_;

transaction::tss_context_map_type transaction::tss_context_map_;
#endif


///////////////////////////////////////////////////////////////////////////////
// static initialization method - must be called before the transaction
// class is used because it initializes our transactionMutex_ which is used
// to guarantee a consistent state of the static
// transactionsInFlight_<transaction* > is correct.
///////////////////////////////////////////////////////////////////////////////
void transaction::initialize()
{
   base_transaction_object::alloc_size(16384);

   if (initialized_) return;
   initialized_ = true;

   logFile_.open("DracoSTM_log.txt");

#ifndef BOOST_STM_USE_BOOST_MUTEX
   //pthread_mutexattr_settype(&transactionMutexAttribute_, PTHREAD_MUTEX_NORMAL);

   pthread_mutex_init(&transactionMutex_, 0);
   pthread_mutex_init(&transactionsInFlightMutex_, 0);
   pthread_mutex_init(&deletionBufferMutex_, 0);
   pthread_mutex_init(&latmMutex_, 0);

   //pthread_mutex_init(&transactionMutex_, &transactionMutexAttribute_);
   //pthread_mutex_init(&transactionsInFlightMutex_, &transactionMutexAttribute_);
   //pthread_mutex_init(&latmMutex_, &transactionMutexAttribute_);
#endif
}

///////////////////////////////////////////////////////////////////////////////
void transaction::initialize_thread()
{
   lock_general_access();

   //--------------------------------------------------------------------------
   // WARNING: before you think lock_all_mutexes() does not make sense, make
   //          sure you read the following example, which will certainly change
   //          your mind about what you think you know ... (bug found by Arthur
   //          Athrun)
   //
   //          end_transaction() must lock all mutexes() in addition to the
   //          important general access mutex, which serializes commits.
   //
   //          In order to make end_transaction as efficient as possible, we
   //          must release general_access() before we release the specific
   //          threaded mutexes. Unfortunately, because of this, a thread can
   //          can enter this function and add a new thread (and mutex) to the
   //          mutex list. Then end_transaction() can finish its execution and
   //          unlock all mutexes. The problem is that between end_transaction
   //          and this function, any number of operations can be performed.
   //          One of those operations may lock the mutex of the new thread,
   //          which may then be unlocked by end_transaction. If that happens,
   //          all kinds of inconsistencies could occur ...
   //
   //          In order to fix this, we could change the unlock order of
   //          end_transaction() so it unlocks all mutexes before releasing the
   //          the general mutex. The problem with that is end_transaction is
   //          a high serialization point and the general mutex is the most
   //          contended upon lock. As such, it is not wise to prolong its
   //          release. Instead, we can change this method, so it locks all the
   //          thread's mutexes. This ensures that this method cannot be entered
   //          until end_transaction() completes, guaranteeing that
   //          end_transaction() cannot unlock mutexes it doesn't own.
   //
   //          Questions? Contact Justin Gottschlich or Vicente Botet.
   //
   //          DO NOT REMOVE LOCK_ALL_MUTEXES / UNLOCK_ALL_MUTEXES!!
   //
   //--------------------------------------------------------------------------
   lock_all_mutexes_but_this(THREAD_ID);

   size_t threadId = THREAD_ID;

#ifndef USE_SINGLE_THREAD_CONTEXT_MAP
/////////////////////////////////
   ThreadWriteContainer::iterator writeIter = threadWriteLists_.find(threadId);
   ThreadReadContainer::iterator readIter = threadReadLists_.find(threadId);
   ThreadBloomFilterList::iterator bloomIter = threadBloomFilterLists_.find(threadId);
   ThreadBloomFilterList::iterator wbloomIter = threadWBloomFilterLists_.find(threadId);

   ThreadMemoryContainerList::iterator newMemIter = threadNewMemoryLists_.find(threadId);
   ThreadMemoryContainerList::iterator deletedMemIter = threadDeletedMemoryLists_.find(threadId);
   ThreadTxTypeContainer::iterator txTypeIter = threadTxTypeLists_.find(threadId);
   ThreadBoolContainer::iterator abortIter = threadForcedToAbortLists_.find(threadId);
   ThreadMutexContainer::iterator mutexIter = threadMutexes_.find(threadId);
   ThreadBoolContainer::iterator blockedIter = threadBlockedLists_.find(threadId);
#if PERFORMING_LATM
#if USING_TRANSACTION_SPECIFIC_LATM
   ThreadMutexSetContainer::iterator conflictingMutexIter = threadConflictingMutexes_.find(threadId);
   if (threadConflictingMutexes_.end() == conflictingMutexIter)
   {
      threadConflictingMutexes_[threadId] = new MutexSet;
   }
#endif

   ThreadMutexSetContainer::iterator obtainedLocksIter = threadObtainedLocks_.find(threadId);
   if (threadObtainedLocks_.end() == obtainedLocksIter)
   {
      threadObtainedLocks_[threadId] = new MutexSet;
   }

   ThreadMutexSetContainer::iterator currentlyLockedLocksIter = threadCurrentlyLockedLocks_.find(threadId);
   if (threadCurrentlyLockedLocks_.end() == currentlyLockedLocksIter)
   {
      threadCurrentlyLockedLocks_[threadId] = new MutexSet;
   }
#endif
   ThreadTransactionsStack::iterator transactionsdIter = threadTransactionsStack_.find(threadId);
   if (threadTransactionsStack_.end() == transactionsdIter)
   {
      threadTransactionsStack_[threadId] = new TransactionsStack;
   }

   if (threadWriteLists_.end() == writeIter)
   {
      threadWriteLists_[threadId] = new WriteContainer();
   }

   if (threadReadLists_.end() == readIter)
   {
      threadReadLists_[threadId] = new ReadContainer();
   }

   if (threadBloomFilterLists_.end() == bloomIter)
   {
      bloom_filter *bf = new boost::stm::bloom_filter();
      threadBloomFilterLists_[threadId] = bf;
   }

   if (threadWBloomFilterLists_.end() == wbloomIter)
   {
      bloom_filter *bf = new boost::stm::bloom_filter();
      threadWBloomFilterLists_[threadId] = bf;
   }


   if (threadNewMemoryLists_.end() == newMemIter)
   {
      threadNewMemoryLists_[threadId] = new MemoryContainerList();
   }

   if (threadDeletedMemoryLists_.end() == deletedMemIter)
   {
      threadDeletedMemoryLists_[threadId] = new MemoryContainerList();
   }

   if (threadTxTypeLists_.end() == txTypeIter)
   {
      threadTxTypeLists_[threadId] = new TxType(eNormalTx);
   }

   if (threadForcedToAbortLists_.end() == abortIter)
   {
      threadForcedToAbortLists_.insert(thread_bool_pair(threadId, new int(0)));
   }

   if (threadMutexes_.end() == mutexIter)
   {
      Mutex *mutex = new Mutex;
#ifndef BOOST_STM_USE_BOOST_MUTEX
#if WIN32
      *mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
      pthread_mutex_init(mutex, 0);
#endif
      threadMutexes_.insert(thread_mutex_pair(threadId, mutex));
      mutexIter = threadMutexes_.find(threadId);
   }

   if (threadBlockedLists_.end() == blockedIter)
   {
      threadBlockedLists_.insert(thread_bool_pair(threadId, new int(0)));
   }

//////////////////////////////////////
#else

   ThreadMutexContainer::iterator mutexIter = threadMutexes_.find(threadId);
   ThreadBoolContainer::iterator blockedIter = threadBlockedLists_.find(threadId);
#if PERFORMING_LATM
#if USING_TRANSACTION_SPECIFIC_LATM
   ThreadMutexSetContainer::iterator conflictingMutexIter = threadConflictingMutexes_.find(threadId);
   if (threadConflictingMutexes_.end() == conflictingMutexIter)
   {
      threadConflictingMutexes_[threadId] = new MutexSet;
   }
#endif

   ThreadMutexSetContainer::iterator obtainedLocksIter = threadObtainedLocks_.find(threadId);
   if (threadObtainedLocks_.end() == obtainedLocksIter)
   {
      threadObtainedLocks_[threadId] = new MutexSet;
   }

   ThreadMutexSetContainer::iterator currentlyLockedLocksIter = threadCurrentlyLockedLocks_.find(threadId);
   if (threadCurrentlyLockedLocks_.end() == currentlyLockedLocksIter)
   {
      threadCurrentlyLockedLocks_[threadId] = new MutexSet;
   }
#endif

   ThreadSizetMap::iterator commitsIter = threadCommitMap_.find(threadId);
   if (threadCommitMap_.end() == commitsIter)
   {
      threadCommitMap_[threadId] = new size_t;
   }

   ThreadTransactionsStack::iterator transactionsdIter = threadTransactionsStack_.find(threadId);
   if (threadTransactionsStack_.end() == transactionsdIter)
   {
      threadTransactionsStack_[threadId] = new TransactionsStack;
   }

   if (threadMutexes_.end() == mutexIter)
   {
      Mutex *mutex = new Mutex;
#ifndef BOOST_STM_USE_BOOST_MUTEX
#if WIN32
      *mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
      pthread_mutex_init(mutex, 0);
#endif
      threadMutexes_.insert(thread_mutex_pair(threadId, mutex));
      mutexIter = threadMutexes_.find(threadId);
   }

   if (threadBlockedLists_.end() == blockedIter)
   {
      threadBlockedLists_.insert(thread_bool_pair(threadId, new int(0)));
   }

   tss_context_map_type::iterator memIter = tss_context_map_.find(threadId);
   if (tss_context_map_.end() == memIter)
   {
      tss_context_map_.insert(std::pair<size_t, tx_context*>(threadId, new tx_context));
      memIter = tss_context_map_.find(threadId);
      memIter->second->txType = eNormalTx;
   }

#if CAPTURING_PROFILE_DATA
   ThreadOstringStream::iterator transactionOstringStreamIter = threadOstringStream_.find(threadId);
   if (threadOstringStream_.end() == transactionOstringStreamIter)
   {
      threadOstringStream_[threadId] = new std::ostringstream;
      *threadOstringStream_[threadId] << "Thread Started: " << threadId << "\t" << kProfilerVersion << endl;
   }

   ThreadMapTxFileAndNumber::iterator txFileAndNumberIter = threadFileAndNumberMap_.find(threadId);
   if (threadFileAndNumberMap_.end() == txFileAndNumberIter)
   {
      threadFileAndNumberMap_[threadId] = new std::map<TxFileAndNumber, size_t>;
   }
#endif

#endif

   //--------------------------------------------------------------------------
   // WARNING: before you think unlock_all_mutexes() does not make sense, make
   //          sure you read the following example, which will certainly change
   //          your mind about what you think you know ... (bug found by Arthur
   //          Athrun)
   //
   //          end_transaction() must lock all mutexes() in addition to the
   //          important general access mutex, which serializes commits.
   //
   //          In order to make end_transaction as efficient as possible, we
   //          must release general_access() before we release the specific
   //          threaded mutexes. Unfortunately, because of this, a thread can
   //          can enter this function and add a new thread (and mutex) to the
   //          mutex list. Then end_transaction() can finish its execution and
   //          unlock all mutexes. The problem is that between end_transaction
   //          and this function, any number of operations can be performed.
   //          One of those operations may lock the mutex of the new thread,
   //          which may then be unlocked by end_transaction. If that happens,
   //          all kinds of inconsistencies could occur ...
   //
   //          In order to fix this, we could change the unlock order of
   //          end_transaction() so it unlocks all mutexes before releasing the
   //          the general mutex. The problem with that is end_transaction is
   //          a high serialization point and the general mutex is the most
   //          contended upon lock. As such, it is not wise to prolong its
   //          release. Instead, we can change this method, so it locks all the
   //          thread's mutexes. This ensures that this method cannot be entered
   //          until end_transaction() completes, guaranteeing that
   //          end_transaction() cannot unlock mutexes it doesn't own.
   //
   //          Questions? Contact Justin Gottschlich or Vicente Botet.
   //
   //          DO NOT REMOVE LOCK_ALL_MUTEXES / UNLOCK_ALL_MUTEXES!!
   //
   //--------------------------------------------------------------------------
   unlock_all_mutexes_but_this(THREAD_ID);
   //--------------------------------------------------------------------------

   unlock_general_access();
}

///////////////////////////////////////////////////////////////////////////////
void transaction::terminate_thread()
{
   lock_general_access();
   lock_inflight_access();

   size_t threadId = THREAD_ID;

#ifndef USE_SINGLE_THREAD_CONTEXT_MAP
   ThreadWriteContainer::iterator writeIter = threadWriteLists_.find(threadId);
   ThreadReadContainer::iterator readIter = threadReadLists_.find(threadId);
   ThreadMemoryContainerList::iterator newMemIter = threadNewMemoryLists_.find(threadId);
   ThreadMemoryContainerList::iterator deletedMemIter = threadDeletedMemoryLists_.find(threadId);
   ThreadBloomFilterList::iterator bloomIter = threadBloomFilterLists_.find(threadId);
   ThreadBloomFilterList::iterator wbloomIter = threadWBloomFilterLists_.find(threadId);
   ThreadTxTypeContainer::iterator txTypeIter = threadTxTypeLists_.find(threadId);
   ThreadBoolContainer::iterator abortIter = threadForcedToAbortLists_.find(threadId);
   ThreadTransactionsStack::iterator transactionsdIter = threadTransactionsStack_.find(threadId);
   delete transactionsdIter->second;
   delete writeIter->second;
   delete readIter->second;
   delete bloomIter->second;
   delete wbloomIter->second;

   delete newMemIter->second;
   delete deletedMemIter->second;
   delete txTypeIter->second;
   delete abortIter->second;

   threadWriteLists_.erase(writeIter);
   threadReadLists_.erase(readIter);
   threadBloomFilterLists_.erase(bloomIter);
   threadWBloomFilterLists_.erase(wbloomIter);

   threadNewMemoryLists_.erase(newMemIter);
   threadDeletedMemoryLists_.erase(deletedMemIter);
   threadTxTypeLists_.erase(txTypeIter);
   threadForcedToAbortLists_.erase(abortIter);

   ThreadMutexContainer::iterator mutexIter = threadMutexes_.find(threadId);
#ifndef BOOST_STM_USE_BOOST_MUTEX
   pthread_mutex_destroy(mutexIter->second);
#endif
   delete mutexIter->second;
   threadMutexes_.erase(mutexIter);

#ifndef MAP_THREAD_MUTEX_CONTAINER
   {
   // realign all in-flight transactions so they are accessing the correct mutex
   for (InflightTxes::iterator i = transactionsInFlight_.begin();
      i != transactionsInFlight_.end(); ++i)
   {
      transaction* t = *i;

      t->mutexRef_ = &mutex(t->threadId_);
   }
   }
#endif

   ThreadBoolContainer::iterator blockedIter = threadBlockedLists_.find(threadId);
   delete blockedIter->second;
   threadBlockedLists_.erase(blockedIter);

#if PERFORMING_LATM
#if USING_TRANSACTION_SPECIFIC_LATM
   ThreadMutexSetContainer::iterator conflictingMutexIter = threadConflictingMutexes_.find(threadId);
   delete conflictingMutexIter->second;
   threadConflictingMutexes_.erase(conflictingMutexIter);
#endif

   ThreadMutexSetContainer::iterator obtainedLocksIter = threadObtainedLocks_.find(threadId);
   delete obtainedLocksIter->second;
   threadObtainedLocks_.erase(obtainedLocksIter);

   ThreadMutexSetContainer::iterator currentlyLockedLocksIter = threadCurrentlyLockedLocks_.find(threadId);
   delete currentlyLockedLocksIter->second;
   threadCurrentlyLockedLocks_.erase(currentlyLockedLocksIter);
#endif


#else
   tss_context_map_type::iterator memIter = tss_context_map_.find(threadId);
   delete memIter->second;
   tss_context_map_.erase(memIter);
#endif




#ifndef MAP_THREAD_BOOL_CONTAINER
   {
   // realign all in-flight transactions so they are accessing the correct mutex
   for (InflightTxes::iterator i = transactionsInFlight_.begin();
      i != transactionsInFlight_.end(); ++i)
   {
      transaction* t = *i;

      t->forcedToAbortRef_ = threadForcedToAbortLists_.find(t->threadId_)->second;
      t->blockedRef_ = blocked(t->threadId_);
   }
   }
#endif

   unlock_inflight_access();
   unlock_general_access();

#if CAPTURING_PROFILE_DATA
   ThreadOstringStream::iterator transactionOstringStreamIter = threadOstringStream_.find(threadId);

   static int threadOutput = 1;
   std::ostringstream name;
   name << threadOutput++ << ".txt";

   ofstream out(name.str().c_str());

   out << transactionOstringStreamIter->second->str().c_str() << endl;

   // output the transactions and their numbers in monotonically increasing order
   std::map<TxFileAndNumber, size_t>& txFileAndNumberMap = *threadFileAndNumberMap_.find(threadId)->second;

   out << endl << "Transactions: " << endl;
   for (std::map<TxFileAndNumber, size_t>::iterator it = txFileAndNumberMap.begin(); 
      txFileAndNumberMap.end() != it; ++it)
   {
      out << it->second << "\t" << it->first.fileName_ << "\t" << it->first.lineNumber_ << endl;
   }

   out.close();
#endif
}

