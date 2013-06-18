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

//-----------------------------------------------------------------------------
//
// TransactionImpl.h
//
// This file contains method implementations for transaction.hpp. The main
// purpose of this file is to reduce the complexity of the transaction class
// by separating its implementation into a secondary .h file.
//
// Do NOT place these methods in a .cc/.cpp/.cxx file. These methods must be
// inlined to keep DracoSTM performing fast. If these methods are placed in a
// C++ source file they will incur function call overhead - something we need
// to reduce to keep performance high.
//
//-----------------------------------------------------------------------------
#ifndef BOOST_STM_TRANSACTION_IMPL_H
#define BOOST_STM_TRANSACTION_IMPL_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <math.h>

using namespace std;

#if CAPTURING_PROFILE_DATA
inline unsigned int boost::stm::transaction::txTime()
{
   clock_t clk = clock();

   return (unsigned int)clk;
}
#endif

//--------------------------------------------------------------------------
//
// PRE-CONDITION: transactionsInFlightMutex is obtained prior to call
//
//--------------------------------------------------------------------------
inline bool boost::stm::transaction::isolatedTxInFlight()
{
   for (InflightTxes::iterator i = transactionsInFlight_.begin();
      i != transactionsInFlight_.end(); ++i)
   {
      // if this is our threadId, skip it
      if (((transaction*)*i)->threadId_ == this->threadId_) continue;

      if (((transaction*)*i)->isolated()) return true;
   }

   return false;
}

//--------------------------------------------------------------------------
//
// PRE-CONDITION: transactionsInFlightMutex is obtained prior to call
//
//--------------------------------------------------------------------------
inline bool boost::stm::transaction::irrevocableTxInFlight()
{
   for (InflightTxes::iterator i = transactionsInFlight_.begin();
      i != transactionsInFlight_.end(); ++i)
   {
      // if this is our threadId, skip it
      if (((transaction*)*i)->threadId_ == this->threadId_) continue;

      if (((transaction*)*i)->irrevocable()) return true;
   }

   return false;
}

//--------------------------------------------------------------------------
//
// PRE-CONDITION: transactionMutex is obtained prior to call
// PRE-CONDITION: transactionsInFlightMutex is obtained prior to call
//
//--------------------------------------------------------------------------
inline bool boost::stm::transaction::abortAllInFlightTxs()
{
   for (InflightTxes::iterator i = transactionsInFlight_.begin();
      i != transactionsInFlight_.end(); ++i)
   {
      // if this is our threadId, skip it
      if (((transaction*)*i)->threadId_ == this->threadId_) continue;

      ((transaction*)*i)->force_to_abort();
   }

   return true;
}

//--------------------------------------------------------------------------
//
// PRE-CONDITION: transactionsInFlightMutex is obtained prior to call
//
//--------------------------------------------------------------------------
inline bool boost::stm::transaction::canAbortAllInFlightTxs()
{
   for (InflightTxes::iterator i = transactionsInFlight_.begin();
      i != transactionsInFlight_.end(); ++i)
   {
      // if this is our threadId, skip it
      if (((transaction*)*i)->threadId_ == this->threadId_) continue;

      if (!cm_->permission_to_abort(*((transaction*)*i), *this)) return false;
   }

   return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::make_irrevocable()
{
   if (irrevocable()) return;
   //-----------------------------------------------------------------------
   // in order to make a tx irrevocable, no other irrevocable txs can be
   // running. if there are, we must stall until they commit.
   //-----------------------------------------------------------------------
   while (true)
   {
      lock_inflight_access();

      if (!irrevocableTxInFlight())
      {
         tx_type(eIrrevocableTx);
         unlock_inflight_access();
         return;
      }

      unlock_inflight_access();
      SLEEP(10);
      cm_->perform_irrevocable_tx_wait_priority_promotion(*this);
   }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::make_isolated()
{
   if (isolated()) return;

   using namespace std;
   //-----------------------------------------------------------------------
   // in order to make a tx irrevocable, no other irrevocable txs can be
   // running. if there are, we must stall until they commit.
   //-----------------------------------------------------------------------
   while (true)
   {
      if (forced_to_abort())
      {
         lock_and_abort();
         throw aborted_transaction_exception
         ("aborting tx in make_isolated");
      }

      lock_general_access();
      lock_inflight_access();

      if (!irrevocableTxInFlight() && canAbortAllInFlightTxs())
      {
         tx_type(eIrrevocableAndIsolatedTx);
         abortAllInFlightTxs();
         unlock_general_access();
         unlock_inflight_access();
         return;
      }

      unlock_general_access();
      unlock_inflight_access();
      //SLEEP(10);
      cm_->perform_isolated_tx_wait_priority_promotion(*this);
   }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool boost::stm::transaction::irrevocable() const
{
   switch (tx_type())
   {
   case eNormalTx: return false;
   case eIrrevocableTx: return true;
   case eIrrevocableAndIsolatedTx: return true;
   default:
      throw "tx type not found";
   }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool boost::stm::transaction::isolated() const
{
   switch (tx_type())
   {
   case eNormalTx: return false;
   case eIrrevocableTx: return false;
   case eIrrevocableAndIsolatedTx: return true;
   default:
      throw "tx type not found";
   }
}

//-----------------------------------------------------------------------------
//
// ASSUMPTIONS: this tx MUST be isolated and using DEFERRED update
//
// This method commits the tx's writes and clears the tx's news/deletes
//
//-----------------------------------------------------------------------------
inline void boost::stm::transaction::commit_deferred_update_tx()
{
   // ensure this method is isolated
   if (!this->irrevocable()) throw "cannot commit deferred tx: not isolated";

   //--------------------------------------------------------------------------
   // otherwise, force the tx to commit its writes/reads
   //--------------------------------------------------------------------------
   while (0 != trylock(&transactionMutex_))
   {
      bookkeeping_.inc_lock_convoy_ms(1);
      SLEEP(1);
   }

   lock_tx();

   //--------------------------------------------------------------------------
   // this is a very important and subtle optimization. if the transaction is
   // only reading memory, it does not need to lock the system. it only needs
   // to lock itself and the tx in-flight mutex and remove itself from the
   // tx in flight list
   //--------------------------------------------------------------------------
   if (is_only_reading())
   {
      unlock_general_access();
      unlock_tx();
   }
   else
   {
      lock_inflight_access();

      //-----------------------------------------------------------------------
      // commit writes, clear new and deletes
      //-----------------------------------------------------------------------
      deferredCommitWriteState();
#ifndef DISABLE_READ_SETS
      readList().clear();
#endif
      deferredCommitTransactionNewMemory();
      deferredCommitTransactionDeletedMemory();

      unlock_tx();
      unlock_general_access();
      unlock_inflight_access();
   }

   //--------------------------------------------------------------------------
   // the tx remains in-flight
   //--------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void boost::stm::transaction::lock_tx()
{
   while (0 != trylock(mutex()))
   {
      SLEEP(1);
   }

   hasMutex_ = 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void boost::stm::transaction::unlock_tx()
{
   unlock(mutex());
   hasMutex_ = 0;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::lock_latm_access()
{
   lock(&latmMutex_);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::unlock_latm_access()
{
   unlock(&latmMutex_);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::lock_inflight_access()
{
   lock(&transactionsInFlightMutex_);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::unlock_inflight_access()
{
   unlock(&transactionsInFlightMutex_);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::lock_general_access()
{
   lock(&transactionMutex_);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::unlock_general_access()
{
   unlock(&transactionMutex_);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::lockThreadMutex(size_t threadId)
{
   lock(mutex(threadId));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::unlockThreadMutex(size_t threadId)
{
   unlock(mutex(threadId));
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::lock_all_mutexes_but_this(size_t threadId)
{
    for (ThreadMutexContainer::iterator i = threadMutexes_.begin();
      i != threadMutexes_.end(); ++i)
   {
      if (i->first == threadId) continue;
      lock(i->second);
   }
}

//--------------------------------------------------------------------------
inline void boost::stm::transaction::unlock_all_mutexes_but_this(size_t threadId)
{
   for (ThreadMutexContainer::iterator i = threadMutexes_.begin();
      i != threadMutexes_.end(); ++i)
   {
      if (i->first == threadId) continue;
      unlock(i->second);
   }
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::lock_all_mutexes()
{
   for (ThreadMutexContainer::iterator i = threadMutexes_.begin();
      i != threadMutexes_.end(); ++i)
   {
      lock(i->second);
   }
   hasMutex_ = 1;
}

//-----------------------------------------------------------------------------
// side-effect: this unlocks all mutexes including its own. this is a slight
// optimization over unlock_all_mutexes_but_this() as it doesn't have an
// additional "if" to slow down performance. however, as it will be
// releasing its own mutex, it must reset hasMutex_
//-----------------------------------------------------------------------------
inline void boost::stm::transaction::unlock_all_mutexes()
{
   for (ThreadMutexContainer::iterator i = threadMutexes_.begin();
      i != threadMutexes_.end(); ++i)
   {
      unlock(i->second);
   }
   hasMutex_ = 0;
}

//--------------------------------------------------------------------------
//
//
//             THREE ENTRY POINTS TO STARTING A TRANSACTION
//
//             constructor
//             begin()
//             restart()
//
//
//--------------------------------------------------------------------------
inline boost::stm::transaction::transaction() :

   //-----------------------------------------------------------------------
   // These two lines are vitally important ... make sure they are always
   // the first two members of the class so the member initialization order
   // always initializes them first.
   //-----------------------------------------------------------------------

   threadId_(THREAD_ID),
   //transactionMutexLocker_(),
   auto_general_lock_(general_lock()),

#if USE_SINGLE_THREAD_CONTEXT_MAP
////////////////////////////////////////
   context_(*tss_context_map_.find(threadId_)->second),

#ifdef BOOST_STM_TX_CONTAINS_REFERENCES_TO_TSS_FIELDS
   write_list_ref_(&context_.writeMem),
   bloomRef_(&context_.bloom),
   wbloomRef_(&context_.wbloom),
   newMemoryListRef_(&context_.newMem),
   deletedMemoryListRef_(&context_.delMem),
   txTypeRef_(&context_.txType),
#ifdef USING_SHARED_FORCED_TO_ABORT
   forcedToAbortRef_(&context_.abort),
#else
   forcedToAbortRef_(false),
#endif
#endif
   mutexRef_(threadMutexes_.find(threadId_)->second),

#if PERFORMING_LATM
   blockedRef_(blocked(threadId_)),
#endif

#if PERFORMING_LATM
#if USING_TRANSACTION_SPECIFIC_LATM
   conflictingMutexRef_(*threadConflictingMutexes_.find(threadId_)->second),
#endif
   obtainedLocksRef_(*threadObtainedLocks_.find(threadId_)->second),
   currentlyLockedLocksRef_(*threadCurrentlyLockedLocks_.find(threadId_)->second),
#endif

   commits_ref_(threadCommitMap_.find(threadId_)->second),
   transactionsRef_(transactions(threadId_, true)),

////////////////////////////////////////
#else
////////////////////////////////////////
#ifndef DISABLE_READ_SETS
   readListRef_(*threadReadLists_.find(threadId_)->second),
#endif
   write_list_ref_(threadWriteLists_.find(threadId_)->second),
   bloomRef_(threadBloomFilterLists_.find(threadId_)->second),
#if PERFORMING_WRITE_BLOOM
   wbloomRef_(threadWBloomFilterLists_.find(threadId_)->second),
   //sm_wbv_(*threadSmallWBloomFilterLists_.find(threadId_)->second),
#endif
   newMemoryListRef_(threadNewMemoryLists_.find(threadId_)->second),
   deletedMemoryListRef_(threadDeletedMemoryLists_.find(threadId_)->second),
   txTypeRef_(threadTxTypeLists_.find(threadId_)->second),
#ifdef USING_SHARED_FORCED_TO_ABORT
   forcedToAbortRef_(threadForcedToAbortLists_.find(threadId_)->second),
#else
   forcedToAbortRef_(false),
#endif

   mutexRef_(threadMutexes_.find(threadId_)->second),

#if PERFORMING_LATM
   blockedRef_(blocked(threadId_)),
#endif

#if PERFORMING_LATM
#if USING_TRANSACTION_SPECIFIC_LATM
   conflictingMutexRef_(*threadConflictingMutexes_.find(threadId_)->second),
#endif
   obtainedLocksRef_(*threadObtainedLocks_.find(threadId_)->second),
   currentlyLockedLocksRef_(*threadCurrentlyLockedLocks_.find(threadId_)->second),
#endif

////////////////////////////////////////
   transactionsRef_(transactions(threadId_, true)),
#endif

   hasMutex_(0), priority_(0),
   state_(e_no_state),
   reads_(0),
#if CAPTURING_PROFILE_DATA
   ostrRef_(*threadOstringStream_.find(threadId_)->second),
   txFileAndNumberMap_(*threadFileAndNumberMap_.find(threadId_)->second),
#endif
   startTime_((size_t)time(0))
{
   auto_general_lock_.release_lock();
    //transactionMutexLocker_.unlock();

   doIntervalDeletions();
#if PERFORMING_LATM
   while (blocked()) { SLEEP(10) ; }
#endif

   put_tx_inflight();

#if CAPTURING_PROFILE_DATA
   ostrRef_ << "TxBegin:            " << txTime() << endl;
#endif
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::begin()
{
   if (e_in_flight == state_) return;

#if PERFORMING_LATM
   while (blocked()) { SLEEP(10) ; }
#endif
   put_tx_inflight();
}

#ifdef LOGGING_BLOCKS
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline std::string boost::stm::transaction::outputBlockedThreadsAndLockedLocks()
{
   using namespace std;

   ostringstream o;

   o << "Threads and their conflicting mutexes:" << endl << endl;

   for (ThreadMutexSetContainer::iterator iter = threadConflictingMutexes_.begin();
   threadConflictingMutexes_.end() != iter; ++iter)
   {
      // if this mutex is found in the transaction's conflicting mutexes
      // list, then allow the thread to make forward progress again
      // by turning its "blocked" but only if it does not appear in the
      // locked_locks_thread_id_map
      o << iter->first << " blocked: " << blocked(iter->first) << endl;
      o << "\t";

      for (MutexSet::iterator inner = iter->second->begin(); inner != iter->second->end(); ++inner)
      {
         o << *inner << " ";
      }
      o << endl;
   }

   o << "Currently locked locks:" << endl << endl;

   for (MutexThreadSetMap::iterator i = latmLockedLocksAndThreadIdsMap_.begin();
   i != latmLockedLocksAndThreadIdsMap_.end(); ++i)
   {
      o << i->first << endl << "\t";

      for (ThreadIdSet::iterator j = i->second.begin(); j != i->second.end(); ++j)
      {
         o << *j << " ";
      }
      o << endl;
   }

   return o.str();
}
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool boost::stm::transaction::restart()
{
   if (e_in_flight == state_) lock_and_abort();

#if PERFORMING_LATM
#ifdef LOGGING_BLOCKS
   int iterations = 0;
#endif
   while (blocked())
   {
#ifdef LOGGING_BLOCKS
      if (++iterations > 100)
      {
         var_auto_lock<PLOCK> autolock(latm_lock(), general_lock(), 0);
         //unblock_threads_if_locks_are_empty();
         logFile_ << outputBlockedThreadsAndLockedLocks().c_str();
         SLEEP(10000);
      }
#endif

      SLEEP(10);
   }
#endif
   //-----------------------------------------------------------------------
   // this is a vital check for composed transactions that abort, but the
   // outer instance tx is never destructed, but instead restarted via
   // restart() interface
   //-----------------------------------------------------------------------
#if PERFORMING_COMPOSITION
#ifdef USING_SHARED_FORCED_TO_ABORT
   lock_inflight_access();
   if (!otherInFlightTransactionsOfSameThreadNotIncludingThis(this))
   {
      unforce_to_abort();
   }
   unlock_inflight_access();
#else
   unforce_to_abort();
#endif
#endif

   put_tx_inflight();

#if CAPTURING_PROFILE_DATA
   ostrRef_ << "TxBegin:            " << txTime() << "\t" << txId_ << endl;
#endif

#if 0
   if (doing_dynamic_priority_assignment())
   {
      priority_ += 1 + (reads_ / 100);
   }

   reads_ = 0;
#endif

   return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool boost::stm::transaction::can_go_inflight()
{
   // if we're doing full lock protection, allow transactions
   // to start only if no locks are obtained or the only lock that
   // is obtained is on THREAD_ID
   if (transaction::doing_full_lock_protection())
   {
      for (MutexThreadMap::iterator j = latmLockedLocksOfThreadMap_.begin();
      j != latmLockedLocksOfThreadMap_.end(); ++j)
      {
         if (THREAD_ID != j->second)
         {
            return false;
         }
      }
   }

   // if we're doing tm lock protection, allow transactions
   // to start only if
   else if (transaction::doing_tm_lock_protection())
   {
      for (MutexSet::iterator i = tmConflictingLocks_.begin(); i != tmConflictingLocks_.end(); ++i)
      {
         // if one of your conflicting locks is currently locked ...
         if (latmLockedLocks_.end() != latmLockedLocks_.find(*i))
         {
            // if it is locked by our thread, it is ok ... otherwise it is not
            MutexThreadMap::iterator j = latmLockedLocksOfThreadMap_.find(*i);

            if (j != latmLockedLocksOfThreadMap_.end() &&
               THREAD_ID != j->second)
            {
               return false;
            }
         }
      }
   }

   return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::put_tx_inflight()
{
#if PERFORMING_LATM
   while (true)
   {
      lock_inflight_access();

      if (can_go_inflight() && !isolatedTxInFlight())
      {
         transactionsInFlight_.insert(this);
         state_ = e_in_flight;
         unlock_inflight_access();
         break;
      }

      unlock_inflight_access();
      SLEEP(10);
   }
#else
   lock_inflight_access();
   transactionsInFlight_.insert(this);
   unlock_inflight_access();
   state_ = e_in_flight;
#endif
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline boost::stm::transaction::~transaction()
{
   // if we're not an inflight transaction - bail
   if (state_ != e_in_flight)
   {
      if (hasLock()) unlock_tx();
      return;
   }

   if (!hasLock()) lock_tx();
   abort();
   unlock_tx();
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::no_throw_end()
{
   try { end(); }
   catch (...) {}
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline void boost::stm::transaction::end()
{
   // in case this is called multiple times
   if (!in_flight()) return;

   if (direct_updating())
   {
#if PERFORMING_VALIDATION
      validating_direct_end_transaction();
#else
      invalidating_direct_end_transaction();
#endif
   }
   else
   {
#if PERFORMING_VALIDATION
      validating_deferred_end_transaction();
#else
      invalidating_deferred_end_transaction();
#endif
   }
}

//-----------------------------------------------------------------------------
// lock_and_abort()
//
// used for contention managers when direct updating is happening and the tx
// has already performed some writes. a direct_abort expects the
// transactionMutex_ to be obtained so it can restore the global memory to its
// original state.
//-----------------------------------------------------------------------------
inline void boost::stm::transaction::lock_and_abort()
{
   if (direct_updating())
   {
      bool wasWriting = isWriting() ? true : false;

      if (wasWriting) lock_general_access();
      lock_tx();
      direct_abort();
      unlock_tx();
      if (wasWriting) unlock_general_access();
   }
   else
   {
      lock_tx();
      deferred_abort();
      unlock_tx();
   }
}

//-----------------------------------------------------------------------------
// invalidating_direct_end_transaction()
//-----------------------------------------------------------------------------
inline void boost::stm::transaction::invalidating_direct_end_transaction()
{
   //--------------------------------------------------------------------------
   // this is an optimization to check forced to abort before obtaining the
   // transaction mutex, so if we do need to abort we do it now
   //--------------------------------------------------------------------------
   if (forced_to_abort())
   {
      lock_and_abort();
      throw aborted_transaction_exception
      ("aborting committing transaction due to contention manager priority inversion");
   }

   lock_general_access();
   lock_tx();

   //--------------------------------------------------------------------------
   // erase this from the inflight transactions so processing through the
   // transactionsInFlight is faster. we have removed the check for "this"
   // when iterating through the transactionsInFlight list. additionally,
   // remember that transactionsInFlight has been removed for commit()
   // so commit doesn't need to remove this from the transactionsInFlight
   //--------------------------------------------------------------------------
   if (forced_to_abort())
   {
      //-----------------------------------------------------------------------
      // if it wasn't writing, it doesn't need the transactionMutex any more,
      // so unlock it so we can reduce contention
      //-----------------------------------------------------------------------
      bool wasWriting = isWriting() ? true : false;
      if (!wasWriting) unlock_general_access();
      direct_abort();
      unlock_tx();
      //-----------------------------------------------------------------------
      // if this tx was writing, unlock the transaction mutex now
      //-----------------------------------------------------------------------
      if (wasWriting) unlock_general_access();
      throw aborted_transaction_exception
      ("aborting committing transaction due to contention manager priority inversion");
   }

   lock_all_mutexes_but_this(threadId_);

   lock_inflight_access();
   transactionsInFlight_.erase(this);

   if (other_in_flight_same_thread_transactions())
   {
      state_ = e_hand_off;
      unlock_all_mutexes();
      unlock_general_access();
      unlock_inflight_access();
      bookkeeping_.inc_handoffs();
   }
   else
   {
      // commit releases the inflight mutex as soon as its done with it
      invalidating_direct_commit();

      if (e_committed == state_)
      {
         unlock_all_mutexes();
         unlock_general_access();
         unlock_inflight_access();
      }
   }
}

//-----------------------------------------------------------------------------
// invalidating_deferred_end_transaction()
//-----------------------------------------------------------------------------
inline void boost::stm::transaction::invalidating_deferred_end_transaction()
{
   //--------------------------------------------------------------------------
   // this is an optimization to check forced to abort before obtaining the
   // transaction mutex, so if we do need to abort we do it now
   //--------------------------------------------------------------------------
#ifndef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
   if (forced_to_abort())
   {
      deferred_abort(true);
      throw aborted_transaction_exception
      ("aborting committing transaction due to contention manager priority inversion");
   }
#endif

   //--------------------------------------------------------------------------
   // this is a very important and subtle optimization. if the transaction is
   // only reading memory, it does not need to lock the system. it only needs
   // to lock tx in-flight mutex and remove itself from the tx in flight list
   //--------------------------------------------------------------------------
   if (is_only_reading())
   {
      lock_inflight_access();
      transactionsInFlight_.erase(this);

#if PERFORMING_COMPOSITION
      if (other_in_flight_same_thread_transactions())
      {
         unlock_inflight_access();
         state_ = e_hand_off;
         bookkeeping_.inc_handoffs();
      }
      else
#endif
      {
         unlock_inflight_access();
         tx_type(eNormalTx);
#if PERFORMING_LATM
         get_tx_conflicting_locks().clear();
         clear_latm_obtained_locks();
#endif
         state_ = e_committed;
      }

      ++(*commits_ref_);

#if CAPTURING_PROFILE_DATA
      ostrRef_ << "TxCommit:           " << txTime() << " R" << endl;
#endif

      ++global_clock();

      return;
   }

   while (0 != trylock(&transactionMutex_)) { }

   //--------------------------------------------------------------------------
   // as much as I'd like to transactionsInFlight_.erase() here, we have
   // to do it inside of abort() because the contention managers must abort
   // the txs and therefore must do a transactionsInFlight_.erase(this)
   // anyway. so we actually lose performance by doing it here and then
   // doing it again inside abort()
   //--------------------------------------------------------------------------
   if (forced_to_abort())
   {
      unlock_general_access();
      deferred_abort(true);
      throw aborted_transaction_exception
      ("aborting committing transaction due to contention manager priority inversion");
   }
   else
   {
      //-----------------------------------------------------------------------
      // erase this from the inflight transactions so processing through the
      // transactionsInFlight is faster. we have removed the check for "this"
      // when iterating through the transactionsInFlight list. additionally,
      // remember that transactionsInFlight has been removed for commit()
      // so commit doesn't need to remove this from the transactionsInFlight
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      // before we get the transactions in flight mutex - get all the locks for
      // al threads, because aborted threads will try to obtain the
      // transactionsInFlightMutex
      //-----------------------------------------------------------------------
      lock_all_mutexes();
      lock_inflight_access();

#if PERFORMING_COMPOSITION
      if (other_in_flight_same_thread_transactions())
      {
         transactionsInFlight_.erase(this);
         state_ = e_hand_off;
         unlock_all_mutexes();
         unlock_general_access();
         unlock_inflight_access();
         bookkeeping_.inc_handoffs();
      }
      else
#endif
      {
         // commit releases the inflight mutex as soon as its done with it
         invalidating_deferred_commit();
      }

      ++global_clock();
   }
}


//-----------------------------------------------------------------------------
// validating_direct_end_transaction()
//-----------------------------------------------------------------------------
inline void boost::stm::transaction::validating_direct_end_transaction()
{
   lock_general_access();
   lock_tx();

   //--------------------------------------------------------------------------
   // can only be called after above transactionMutex_ is called
   //--------------------------------------------------------------------------
   if (cm_->abort_before_commit(*this))
   {
      abort();
      //bookkeeping_.inc_abort_perm_denied(threadId_);
      unlock_tx();
      unlock_general_access();
      throw aborted_transaction_exception
      ("aborting commit due to CM priority");
   }

   //--------------------------------------------------------------------------
   // erase this from the inflight transactions so processing through the
   // transactionsInFlight is faster. we have removed the check for "this"
   // when iterating through the transactionsInFlight list. additionally,
   // remember that transactionsInFlight has been removed for commit()
   // so commit doesn't need to remove this from the transactionsInFlight
   //--------------------------------------------------------------------------
   if (forced_to_abort())
   {
      //-----------------------------------------------------------------------
      // if it wasn't writing, it doesn't need the transactionMutex any more,
      // so unlock it so we can reduce contention
      //-----------------------------------------------------------------------
      bool wasWriting = isWriting() ? true : false;
      if (!wasWriting) unlock_general_access();
      direct_abort();
      unlock_tx();
      //-----------------------------------------------------------------------
      // if this tx was writing, unlock the transaction mutex now
      //-----------------------------------------------------------------------
      if (wasWriting) unlock_general_access();
      throw aborted_transaction_exception
      ("aborting committing transaction due to contention manager priority inversion");
   }

   lock_all_mutexes_but_this(threadId_);

   lock_inflight_access();
   transactionsInFlight_.erase(this);

   if (other_in_flight_same_thread_transactions())
   {
      state_ = e_hand_off;
      unlock_all_mutexes();
      unlock_general_access();
      unlock_inflight_access();
      bookkeeping_.inc_handoffs();
   }
   else
   {
      // commit releases the inflight mutex as soon as its done with it
      validating_direct_commit();

      if (e_committed == state_)
      {
         unlock_all_mutexes();
         unlock_general_access();
         unlock_inflight_access();
      }
   }
}

//-----------------------------------------------------------------------------
// validating_deferred_end_transaction()
//-----------------------------------------------------------------------------
inline void boost::stm::transaction::validating_deferred_end_transaction()
{
   lock_general_access();
   lock_inflight_access();
   lock_tx();

   //--------------------------------------------------------------------------
   // can only be called after above transactionMutex_ is called
   //--------------------------------------------------------------------------
   if (cm_->abort_before_commit(*this))
   {
      //bookkeeping_.inc_abort_perm_denied(threadId_);
      unlock_inflight_access();
      unlock_general_access();
      deferred_abort();
      unlock_tx();
      throw aborted_transaction_exception
      ("aborting commit due to CM priority");
   }

   // unlock this - we only needed it to check abort_before_commit()
   unlock_inflight_access();

   uint32 ms = clock();

   //--------------------------------------------------------------------------
   // as much as I'd like to transactionsInFlight_.erase() here, we have
   // to do it inside of abort() because the contention managers must abort
   // the txs and therefore must do a transactionsInFlight_.erase(this)
   // anyway. so we actually lose performance by doing it here and then
   // doing it again inside abort()
   //--------------------------------------------------------------------------
   if (forced_to_abort())
   {
      unlock_general_access();
      deferred_abort();
      unlock_tx();
      throw aborted_transaction_exception
      ("aborting committing transaction due to contention manager priority inversion");
   }
   else
   {
      //--------------------------------------------------------------------------
      // this is a very important and subtle optimization. if the transaction is
      // only reading memory, it does not need to lock the system. it only needs
      // to lock itself and the tx in-flight mutex and remove itself from the
      // tx in flight list
      //--------------------------------------------------------------------------
      if (is_only_reading())
      {
         lock_inflight_access();
         transactionsInFlight_.erase(this);

         if (other_in_flight_same_thread_transactions())
         {
            state_ = e_hand_off;
            bookkeeping_.inc_handoffs();
         }
         else
         {
            tx_type(eNormalTx);
#if PERFORMING_LATM
            get_tx_conflicting_locks().clear();
            clear_latm_obtained_locks();
#endif
            state_ = e_committed;
         }

         ++(*commits_ref_);

#if CAPTURING_PROFILE_DATA
      ostrRef_ << "TxCommit:           " << txTime() << " R" << endl;
#endif

         unlock_general_access();
         unlock_inflight_access();

         bookkeeping_.inc_commits();
#ifndef DISABLE_READ_SETS
         bookkeeping_.pushBackSizeOfReadSetWhenCommitting(readList().size());
#endif
         bookkeeping_.inc_commit_time_ms(clock() - ms);
         unlock_tx();
      }

      //-----------------------------------------------------------------------
      // erase this from the inflight transactions so processing through the
      // transactionsInFlight is faster. we have removed the check for "this"
      // when iterating through the transactionsInFlight list. additionally,
      // remember that transactionsInFlight has been removed for commit()
      // so commit doesn't need to remove this from the transactionsInFlight
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      // before we get the transactions in flight mutex - get all the locks for
      // al threads, because aborted threads will try to obtain the
      // transactionsInFlightMutex
      //-----------------------------------------------------------------------
      lock_all_mutexes_but_this(threadId_);

      lock_inflight_access();
      transactionsInFlight_.erase(this);

      if (other_in_flight_same_thread_transactions())
      {
         state_ = e_hand_off;
         unlock_all_mutexes();
         unlock_general_access();
         unlock_inflight_access();
         bookkeeping_.inc_handoffs();
      }
      else
      {
         // commit releases the inflight mutex as soon as its done with it
         validating_deferred_commit();

         // if the commit actually worked, then we can release these locks
         // an exception happened which caused the tx to abort and thus
         // unlocked these already
         if (e_committed == state_)
         {
            unlock_tx();
            unlock_general_access();
            unlock_inflight_access();
         }
      }
   }

   bookkeeping_.inc_commit_time_ms(clock() - ms);
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::forceOtherInFlightTransactionsWritingThisWriteMemoryToAbort()
{
#ifndef ALWAYS_ALLOW_ABORT
   std::list<transaction*> aborted;
#endif
   InflightTxes::iterator next;

   // iterate through all our written memory
   for (WriteContainer::iterator i = writeList().begin(); writeList().end() != i; ++i)
   {
      // iterate through all the in flight transactions
      for (InflightTxes::iterator j = transactionsInFlight_.begin();
      j != transactionsInFlight_.end();)
      {
         transaction *t = (transaction*)*j;

         // if we're already aborting for this transaction, skip it
         if (t->forced_to_abort()) {++j; continue;}

         ///////////////////////////////////////////////////////////////////
         // as only one thread can commit at a time, and we know that
         // composed txs, handoff, rather than commit, we should never be
         // in this loop when another inflight tx has our threadid.
         ///////////////////////////////////////////////////////////////////
         assert(t->threadId_ != this->threadId_);

         ///////////////////////////////////////////////////////////////////
         // if t's modifiedList is modifying memory we are also modifying, make t bail
         ///////////////////////////////////////////////////////////////////
#ifdef USE_BLOOM_FILTER
         if (t->bloom().exists(i->first))
#else
         if (t->writeList().end() != t->writeList().find(i->first))
#endif
         {
#if ALWAYS_ALLOW_ABORT
            t->force_to_abort();

            next = j;
            ++next;
            transactionsInFlight_.erase(j);
            j = next;

#else
            if (this->irrevocable())
            {
               aborted.push_front(t);
            }
            else if (!t->irrevocable() && cm_->permission_to_abort(*this, *t))
            {
               aborted.push_front(t);
            }
            else
            {
               force_to_abort();
               //bookkeeping_.inc_abort_perm_denied(threadId_);
               throw aborted_transaction_exception
               ("aborting committing transaction due to contention manager priority inversion");
            }
#endif
         }

         ++j;
      }
   }

#ifndef ALWAYS_ALLOW_ABORT
   // ok, forced to aborts are allowed, do them
   for (std::list<transaction*>::iterator k = aborted.begin(); k != aborted.end();)
   {
      (*k)->force_to_abort();
      transactionsInFlight_.erase(*k);
   }
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
// Two ways for transactions to end:
//
//    abort() - failed transaction
//    commit() - successful transaction
//
///////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::direct_abort
   (bool const &alreadyRemovedFromInFlight) throw()
{

#if LOGGING_COMMITS_AND_ABORTS
#ifndef DISABLE_READ_SETS
   bookkeeping_.pushBackSizeOfReadSetWhenAborting(readList().size());
#endif
   bookkeeping_.pushBackSizeOfWriteSetWhenAborting(writeList().size());
#endif

   try
   {
      state_ = e_aborted;

      directAbortWriteList();
#ifndef DISABLE_READ_SETS
      directAbortReadList();
#endif
      directAbortTransactionDeletedMemory();
      directAbortTransactionNewMemory();

      bloom().clear();
#if PERFORMING_WRITE_BLOOM
      wbloom().clear();
#endif
      if (!alreadyRemovedFromInFlight)
      {
         lock_inflight_access();
         // if I'm the last transaction of this thread, reset abort to false
         transactionsInFlight_.erase(this);
      }

#ifdef USING_SHARED_FORCED_TO_ABORT
      if (!other_in_flight_same_thread_transactions())
      {
         unforce_to_abort();
      }
#else
      unforce_to_abort();
#endif
      if (!alreadyRemovedFromInFlight)
      {
         unlock_inflight_access();
      }
   }
   catch (...)
   {
      std::cout << "Exception caught in abort - bad" << std::endl;
   }
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::deferred_abort
   (bool const &alreadyRemovedFromInFlight) throw()
{
#if LOGGING_COMMITS_AND_ABORTS
#ifndef DISABLE_READ_SETS
   bookkeeping_.pushBackSizeOfReadSetWhenAborting(readList().size());
#endif
   bookkeeping_.pushBackSizeOfWriteSetWhenAborting(writeList().size());
#endif

   state_ = e_aborted;

#if CAPTURING_PROFILE_DATA
   ostrRef_ << "TxAbort:            " << txTime()  << " " << abortedBy_ << " ";
   if (this->is_only_reading()) ostrRef_ << "R";
   else if (this->is_only_writing()) ostrRef_ << "W";
   else if (this->is_reading_and_writing()) ostrRef_ << "RW";
   else 
   {
      ostrRef_ << "U";
   }
   ostrRef_ << " " << endl;
#endif

   deferredAbortWriteList();
#ifndef DISABLE_READ_SETS
   deferredAbortReadList();
#endif

   deferredAbortTransactionDeletedMemory();
   deferredAbortTransactionNewMemory();

   bloom().clear();
#if PERFORMING_WRITE_BLOOM
   wbloom().clear();
#endif

   // (some error exists with this optimization) if (!alreadyRemovedFromInFlight)
   {
      lock_inflight_access();
      // if I'm the last transaction of this thread, reset abort to false
      transactionsInFlight_.erase(this);

#ifdef USING_SHARED_FORCED_TO_ABORT
      if (!other_in_flight_same_thread_transactions())
      {
         unforce_to_abort();
      }
#else
      unforce_to_abort();
#endif

      unlock_inflight_access();
   }
   //else unforce_to_abort();
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::invalidating_direct_commit()
{
   //--------------------------------------------------------------------------
   // transactionMutex must be obtained before calling commit
   //--------------------------------------------------------------------------
   try
   {
#if LOGGING_COMMITS_AND_ABORTS
#ifndef DISABLE_READ_SETS
      bookkeeping_.pushBackSizeOfReadSetWhenCommitting(readList().size());
#endif
      bookkeeping_.pushBackSizeOfWriteSetWhenCommitting(writeList().size());
#endif

      bookkeeping_.inc_commits();

      if (!transactionsInFlight_.empty())
      {
         forceOtherInFlightTransactionsReadingThisWriteMemoryToAbort();
      }

      directCommitWriteState();
#ifndef DISABLE_READ_SETS
      directCommitReadState();
#endif
      bookkeeping_.inc_del_mem_commits_by(deletedMemoryList().size());
      directCommitTransactionDeletedMemory();
      bookkeeping_.inc_new_mem_commits_by(newMemoryList().size());
      directCommitTransactionNewMemory();

      tx_type(eNormalTx);
#if PERFORMING_LATM
      get_tx_conflicting_locks().clear();
      clear_latm_obtained_locks();
#endif
      state_ = e_committed;
   }
   //-----------------------------------------------------------------------
   // aborted exceptions can be thrown from the forceOtherInFlight ...
   // if one is thrown it means this transaction was preempted by cm
   //-----------------------------------------------------------------------
   catch (aborted_transaction_exception&)
   {
      unlock_all_mutexes_but_this(threadId_);
      unlock_general_access();
      unlock_inflight_access();

      direct_abort();
      unlock_tx();

      throw;
   }
   //-----------------------------------------------------------------------
   // still - what do we do in the event the exception was the commitState()?
   //-----------------------------------------------------------------------
   catch (...)
   {
      unlock_all_mutexes_but_this(threadId_);
      unlock_general_access();
      unlock_inflight_access();

      direct_abort();
      unlock_tx();

      throw;
   }
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::invalidating_deferred_commit()
{
   //--------------------------------------------------------------------------
   // transactionMutex must be obtained before calling commit
   //--------------------------------------------------------------------------

   try
   {
#if LOGGING_COMMITS_AND_ABORTS
#ifndef DISABLE_READ_SETS
      bookkeeping_.pushBackSizeOfReadSetWhenCommitting(readList().size());
#endif
      bookkeeping_.pushBackSizeOfWriteSetWhenCommitting(writeList().size());
#endif

      //-----------------------------------------------------------------------
      // optimization:
      //
      // whenever multiple transactions are running, if one goes to commit, it
      // never commits, but instead hands-off and exits. thus, from this we
      // know that when a tx is committing it means only one tx for that thread
      // is running.
      //
      // therefore, we can remove our tx from the inflight list at commit time
      // and know that if the in flight tx list is > 1, other threads are
      // currently processing and we must force their memory to abort.
      //
      // this opti gains significant performance improvement since many times
      // only a single tx is processing - so save many cycles from having to
      // walk its read and write list
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      // remember commit can only be called from end_transaction() and
      // end_transaction() has already removed "this" from the
      // transactionsInFlight_ set
      //-----------------------------------------------------------------------
      // commit() already has the transactionsInFlightMutex

      if (transactionsInFlight_.size() > 1)
      {
         static int stalling_ = 0;

         bool wait = stalling_ * stalls_ < global_clock();
         transaction *stallingOn = 0;
         //int iter = 1;

#if USE_BLOOM_FILTER
         while (!forceOtherInFlightTransactionsAccessingThisWriteMemoryToAbort(wait, stallingOn))
         {
            ++stalling_;
            size_t local_clock = global_clock();

            unlock_inflight_access();
            unlock_general_access();
            unlock_all_mutexes();

            for (;;)
            {
               while (local_clock == global_clock())// && sleepTime < maxSleep)
               {
                  SLEEP(1);
               }

               if (forced_to_abort())
               {
                  --stalling_;
                  deferred_abort();
                  throw aborted_transaction_exception_no_unlocks();
               }

               lock_general_access();
               lock_inflight_access();

               // if our stalling on tx is gone, continue
               if (transactionsInFlight_.end() == transactionsInFlight_.find(stallingOn))
               {
                  --stalling_;
                  wait = stalling_ * stalls_ < global_clock();
                  //std::cout << "stalling : stalls : commits: " << stalling_ << " : " << stalls_ << " : " << global_clock() << std::endl;
                  //set_priority(priority() + reads());
                  break;
               }

               unlock_general_access();
               unlock_inflight_access();
            }

            lock_all_mutexes();

            if (forced_to_abort())
            {
               deferred_abort();
               unlock_all_mutexes(); // i added this after the fact, is this right?
               throw aborted_transaction_exception
               ("aborting committing transaction due to contention manager priority inversion");
            }
         }
#else
         forceOtherInFlightTransactionsWritingThisWriteMemoryToAbort();
         forceOtherInFlightTransactionsReadingThisWriteMemoryToAbort();
#endif
      }

      transactionsInFlight_.erase(this);

      ++(*commits_ref_);

      unlock_inflight_access();
      unlock_general_access();

#if CAPTURING_PROFILE_DATA
      ostrRef_ << "TxCommit:           " << txTime() << " ";
      if (this->is_only_reading()) ostrRef_ << "R";
      else if (this->is_only_writing()) ostrRef_ << "W";
      else if (this->is_reading_and_writing()) ostrRef_ << "RW";
      else 
      {
         ostrRef_ << "U";
      }
      ostrRef_ << " " << endl;
#endif

      deferredCommitWriteState();

      if (!newMemoryList().empty())
      {
         bookkeeping_.inc_new_mem_commits_by(newMemoryList().size());
         deferredCommitTransactionNewMemory();
      }

      //-----------------------------------------------------------------------
      // if the commit actually worked, then we can release these locks
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      // once our write state is committed and our new memory has been cleared,
      // we can allow the other threads to make forward progress ... so unlock
      // them all now
      //-----------------------------------------------------------------------
      unlock_all_mutexes();

      if (!deletedMemoryList().empty())
      {
         bookkeeping_.inc_del_mem_commits_by(deletedMemoryList().size());
         deferredCommitTransactionDeletedMemory();
      }

      // finally set the state to committed
      bookkeeping_.inc_commits();

      tx_type(eNormalTx);
#if PERFORMING_LATM
      get_tx_conflicting_locks().clear();
      clear_latm_obtained_locks();
#endif
      state_ = e_committed;
   }
   //-----------------------------------------------------------------------
   // aborted exceptions can be thrown from the forceOtherInFlight ...
   // if one is thrown it means this transaction was preempted by cm
   //-----------------------------------------------------------------------
   catch (aborted_transaction_exception_no_unlocks&)
   {
      throw aborted_transaction_exception("whatever");
   }
   catch (aborted_transaction_exception&)
   {
      unlock_all_mutexes_but_this(threadId_);
      unlock_general_access();
      unlock_inflight_access();
      deferred_abort();
      unlock_tx();

      SLEEP(1);

      throw;
   }
   //-----------------------------------------------------------------------
   // copy constructor failures can cause ..., catch, unlock and re-throw
   //-----------------------------------------------------------------------
   catch (...)
   {
      unlock_all_mutexes_but_this(threadId_);
      unlock_general_access();
      unlock_inflight_access();
      deferred_abort();
      unlock_tx();

      throw;
   }
}


#if PERFORMING_VALIDATION
////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::validating_direct_commit()
{
   throw "not implemented yet";






   //--------------------------------------------------------------------------
   // transactionMutex must be obtained before calling commit
   //--------------------------------------------------------------------------
   try
   {
#if LOGGING_COMMITS_AND_ABORTS
      bookkeeping_.pushBackSizeOfReadSetWhenCommitting(readList().size());
      bookkeeping_.pushBackSizeOfWriteSetWhenCommitting(writeList().size());
#endif

      bookkeeping_.inc_commits();

      if (!transactionsInFlight_.empty())
      {
         forceOtherInFlightTransactionsReadingThisWriteMemoryToAbort();
      }

      directCommitWriteState();
      directCommitReadState();
      bookkeeping_.inc_del_mem_commits_by(deletedMemoryList().size());
      directCommitTransactionDeletedMemory();
      bookkeeping_.inc_new_mem_commits_by(newMemoryList().size());
      directCommitTransactionNewMemory();

      tx_type_ref() = eNormalTx;
#if PERFORMING_LATM
      get_tx_conflicting_locks().clear();
      clear_latm_obtained_locks();
#endif
      state_ = e_committed;
   }
   //-----------------------------------------------------------------------
   // aborted exceptions can be thrown from the forceOtherInFlight ...
   // if one is thrown it means this transaction was preempted by cm
   //-----------------------------------------------------------------------
   catch (aborted_transaction_exception&)
   {
      unlock_all_mutexes_but_this(threadId_);
      unlock_general_access();
      unlock_inflight_access();

      direct_abort();
      unlock_tx();

      throw;
   }
   //-----------------------------------------------------------------------
   // still - what do we do in the event the exception was the commitState()?
   //-----------------------------------------------------------------------
   catch (...)
   {
      unlock_all_mutexes_but_this(threadId_);
      unlock_general_access();
      unlock_inflight_access();

      direct_abort();
      unlock_tx();

      throw;
   }
}
#endif

#if PERFORMING_VALIDATION
////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::validating_deferred_commit()
{
   //--------------------------------------------------------------------------
   // transactionMutex must be obtained before calling commit
   //--------------------------------------------------------------------------

   try
   {
#if LOGGING_COMMITS_AND_ABORTS
      bookkeeping_.pushBackSizeOfReadSetWhenCommitting(readList().size());
      bookkeeping_.pushBackSizeOfWriteSetWhenCommitting(writeList().size());
#endif

      //-----------------------------------------------------------------------
      // optimization:
      //
      // whenever multiple transactions are running, if one goes to commit, it
      // never commits, but instead hands-off and exits. thus, from this we
      // know that when a tx is committing it means only one tx for that thread
      // is running.
      //
      // therefore, we can remove our tx from the inflight list at commit time
      // and know that if the in flight tx list is > 1, other threads are
      // currently processing and we must force their memory to abort.
      //
      // this opti gains significant performance improvement since many times
      // only a single tx is processing - so save many cycles from having to
      // walk its read and write list
      //-----------------------------------------------------------------------

      //-----------------------------------------------------------------------
      // remember commit can only be called from end_transaction() and
      // end_transaction() has already removed "this" from the
      // transactionsInFlight_ set
      //-----------------------------------------------------------------------
      // commit() already has the transactionsInFlightMutex

      verifyReadMemoryIsValidWithGlobalMemory();
      verifyWrittenMemoryIsValidWithGlobalMemory();

      if (this->isWriting())
      {
         deferredCommitWriteState();
      }

      if (!newMemoryList().empty())
      {
         bookkeeping_.inc_new_mem_commits_by(newMemoryList().size());
         deferredCommitTransactionNewMemory();
      }

      //-----------------------------------------------------------------------
      // once our write state is committed and our new memory has been cleared,
      // we can allow the other threads to make forward progress ... so unlock
      // them all now
      //-----------------------------------------------------------------------
      unlock_all_mutexes_but_this(threadId_);

      if (!deletedMemoryList().empty())
      {
         bookkeeping_.inc_del_mem_commits_by(deletedMemoryList().size());
         deferredCommitTransactionDeletedMemory();
      }

      // finally set the state to committed
      bookkeeping_.inc_commits();
      tx_type_ref() = eNormalTx;
#if PERFORMING_LATM
      get_tx_conflicting_locks().clear();
      clear_latm_obtained_locks();
#endif
      state_ = e_committed;
   }

   //-----------------------------------------------------------------------
   // aborted exceptions can be thrown from the forceOtherInFlight ...
   // if one is thrown it means this transaction was preempted by cm
   //-----------------------------------------------------------------------
   catch (aborted_transaction_exception&)
   {
      unlock_all_mutexes_but_this(threadId_);
      unlock_general_access();
      unlock_inflight_access();
      deferred_abort();
      unlock_tx();

      throw;
   }
   //-----------------------------------------------------------------------
   // copy constructor failures can cause ..., catch, unlock and re-throw
   //-----------------------------------------------------------------------
   catch (...)
   {
      unlock_all_mutexes_but_this(threadId_);
      unlock_general_access();
      unlock_inflight_access();
      deferred_abort();
      unlock_tx();

      throw;
   }
}
#endif

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::unlockAllLockedThreads(LockedTransactionContainer &l)
{
   for (LockedTransactionContainer::iterator i = l.begin(); i != l.end(); ++i) (*i)->unlock_tx();
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::directAbortWriteList()
{
   //--------------------------------------------------------------------------
   // copy the newObject into the oldObject, updating the real data back to
   // what it was before we changed it
   //--------------------------------------------------------------------------
   for (WriteContainer::iterator i = writeList().begin(); writeList().end() != i; ++i)
   {
      //-----------------------------------------------------------------------
      // i->second == 0 will happen when a transaction has added a piece of
      // memory to its writeList_ that it is deleting (not writing to).
      // Thus, when seeing 0 as the second value, it signifies that this
      // memory is being destroyed, not updated. Do not perform copy_state()
      // on it.
      //
      // However, deleted memory MUST reset its kInvalidThread
      // transaction_thread (which is performed in    void directAbortTransactionDeletedMemory() throw();

      //-----------------------------------------------------------------------
      if (0 == i->second) continue;

      if (using_move_semantics()) i->first->move_state(i->second);
      else i->first->copy_state(i->second);
      i->first->transaction_thread(boost::stm::kInvalidThread);

      delete i->second;
   }

   writeList().clear();
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::directAbortTransactionDeletedMemory() throw()
{
   for (MemoryContainerList::iterator j = deletedMemoryList().begin();
   j != deletedMemoryList().end(); ++j)
   {
      (*j)->transaction_thread(boost::stm::kInvalidThread);
   }

   deletedMemoryList().clear();
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::deferredAbortWriteList() throw()
{
   for (WriteContainer::iterator i = writeList().begin(); writeList().end() != i; ++i)
   {
      delete i->second; // delete all the temporary memory
   }

   writeList().clear();
}

//----------------------------------------------------------------------------
inline size_t boost::stm::transaction::earliest_start_time_of_inflight_txes()
{
   var_auto_lock<PLOCK> a(inflight_lock(), 0);

   size_t secs = 0xffffffff;

   for (InflightTxes::iterator j = transactionsInFlight_.begin();
   j != transactionsInFlight_.end(); ++j)
   {
      transaction *t = (transaction*)*j;
      //-----------------------------------------------------------------------
      // since this is called while direct_writes are occurring, the transaction
      // calling it will be inflight, so we have to check for it and skip it
      //-----------------------------------------------------------------------
      if (t->startTime_ < secs) secs = t->startTime_;
   }

   return secs;
}

//----------------------------------------------------------------------------
inline void boost::stm::transaction::doIntervalDeletions()
{
   using namespace boost::stm;

   size_t earliestInFlightTx = earliest_start_time_of_inflight_txes();

   var_auto_lock<PLOCK> a(&deletionBufferMutex_, 0);

   for (DeletionBuffer::iterator i = deletionBuffer_.begin(); i != deletionBuffer_.end();)
   {
      if (earliestInFlightTx > i->first)
      {
         for (MemoryContainerList::iterator j = i->second.begin(); j != i->second.end(); ++j)
         {
            delete *j;
         }
         deletionBuffer_.erase(i);
         i = deletionBuffer_.begin();
      }
      // getting to the else means there is an inflight
      // tx that is older than the first entry to delete,
      // so exit, since no other deletions will succeed
      else break;
   }

//----------------------------------------------------------------------------
// TODO: Vicente, do you know what the below code is used for?
//----------------------------------------------------------------------------
#if 0

   for (UnsafeDeletionBuffer::iterator i = unsafeDeletionBuffer_.begin(); 
        i != unsafeDeletionBuffer_.end();)
   {
      if (earliestInFlightTx > i->first)
      {
         for (UnsafeMemoryContainerList::iterator j = i->second.begin(); j != i->second.end(); ++j)
         {
            free(j);
         }
         unsafeDeletionBuffer_.erase(i);
         i = unsafeDeletionBuffer_.begin();
      }
      // getting to the else means there is an inflight
      // tx that is older than the first entry to delete,
      // so exit, since no other deletions will succeed
      else break;
   }
#endif

}

//----------------------------------------------------------------------------
inline void boost::stm::transaction::directCommitTransactionDeletedMemory() throw()
{
   using namespace boost::stm;

   if (!deletedMemoryList().empty())
   {
      var_auto_lock<PLOCK> a(&deletionBufferMutex_, 0);
      deletionBuffer_.insert( std::pair<size_t, MemoryContainerList>
         ((size_t)time(0), deletedMemoryList()) );
      deletedMemoryList().clear();
   }
}

//----------------------------------------------------------------------------
inline void boost::stm::transaction::deferredCommitTransactionDeletedMemory() throw()
{
   using namespace boost::stm;

   if (!deletedMemoryList().empty())
   {
      var_auto_lock<PLOCK> a(&deletionBufferMutex_, NULL);
      deletionBuffer_.insert( std::pair<size_t, MemoryContainerList>
         ((size_t)time(NULL), deletedMemoryList()) );
      deletedMemoryList().clear();
   }
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::deferredAbortTransactionNewMemory() throw()
{
   for (MemoryContainerList::iterator i = newMemoryList().begin(); i != newMemoryList().end(); ++i)
   {
      delete *i;
   }

   newMemoryList().clear();
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::deferredCommitTransactionNewMemory()
{
   for (MemoryContainerList::iterator i = newMemoryList().begin(); i != newMemoryList().end(); ++i)
   {
      (*i)->transaction_thread(boost::stm::kInvalidThread);
      (*i)->new_memory(0);
   }

   newMemoryList().clear();
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::directCommitWriteState()
{
   // direct commit for writes just deletes the backup and changes global memory
   // so its no longer flagged as being transactional
   for (WriteContainer::iterator i = writeList().begin(); writeList().end() != i; ++i)
   {
      //-----------------------------------------------------------------------
      // i->second == 0 will happen when a transaction has added a piece of
      // memory to its writeList_ that it is deleting (not writing to).
      // Thus, when seeing 0 as the second value, it signifies that this
      // memory is being destroyed, not updated. Do not perform copyState()
      // on it.
      //-----------------------------------------------------------------------
      i->first->transaction_thread(boost::stm::kInvalidThread);
      i->first->new_memory(0);

      //-----------------------------------------------------------------------
      // it is true that i->second can be null, but C++ allows delete on null
      // faster to just always delete
      //-----------------------------------------------------------------------
      delete i->second;
   }

   writeList().clear();
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::deferredCommitWriteState()
{
   // copy the newObject into the oldObject, updating the real data
   for (WriteContainer::iterator i = writeList().begin(); writeList().end() != i; ++i)
   {
      //-----------------------------------------------------------------------
      // i->second == 0 will happen when a transaction has added a piece of
      // memory to its writeList_ that it is deleting (not writing to).
      // Thus, when seeing 0 as the second value, it signifies that this
      // memory is being destroyed, not updated. Do not perform copyState()
      // on it.
      //-----------------------------------------------------------------------
      if (0 == i->second)
      {
         continue;
      }

      if (using_move_semantics()) i->first->move_state(i->second);
      else i->first->copy_state(i->second);

      i->first->transaction_thread(boost::stm::kInvalidThread);
      i->first->new_memory(0);

#if PERFORMING_VALIDATION
      i->first->version_++;
#endif

      delete i->second;
   }

   writeList().clear();
}

#if PERFORMING_VALIDATION
//----------------------------------------------------------------------------
inline void boost::stm::transaction::verifyReadMemoryIsValidWithGlobalMemory()
{
   // copy the newObject into the oldObject, updating the real data
   for (ReadContainer::iterator i = readList().begin(); readList().end() != i; ++i)
   {
      if (i->first->version_ != i->second)
      {
         bookkeeping_.inc_read_aborts();
         throw aborted_transaction_exception
         ("aborting committing transaction due to invalid read");
      }
   }
}

//----------------------------------------------------------------------------
inline void boost::stm::transaction::verifyWrittenMemoryIsValidWithGlobalMemory()
{
   // copy the newObject into the oldObject, updating the real data
   for (WriteContainer::iterator i = writeList().begin(); writeList().end() != i; ++i)
   {
      if (0 == i->second) continue;
      if (i->first->version_ != i->second->version_)
      {
         bookkeeping_.inc_write_aborts();
         throw aborted_transaction_exception
         ("aborting committing transaction due to invalid write");
      }
   }
}
#endif

////////////////////////////////////////////////////////////////////////////
inline bool boost::stm::transaction::otherInFlightTransactionsWritingThisMemory(base_transaction_object *obj)
{
   // iterate through all the in flight transactions
   for (InflightTxes::iterator j = transactionsInFlight_.begin();
   j != transactionsInFlight_.end(); ++j)
   {
      transaction *t = (transaction*)*j;
      //-----------------------------------------------------------------------
      // since this is called while direct_writes are occurring, the transaction
      // calling it will be inflight, so we have to check for it and skip it
      //-----------------------------------------------------------------------
      if (t->threadId_ == this->threadId_) continue;

      // if we're already aborting for this transaction, skip it
      if (!t->isWriting()) continue;
      if (t->forced_to_abort()) continue;

      // if t's writing to this memory return true
      if (t->writeList().end() != t->writeList().find(obj)) return true;
   }

   return false;
}

////////////////////////////////////////////////////////////////////////////
inline bool boost::stm::transaction::forceOtherInFlightTransactionsAccessingThisWriteMemoryToAbort
      (bool allow_stall, transaction* &stallingOn)
{
   std::list<transaction*> aborted;
   //static bool initialized = false;

   if (writes() > 3) allow_stall = false;

   //--------------------------------------------------------------------------
   // FOR THE TIME BEING, DO NOT ALLOW STALLS AT ALL! Stalls somehow cause
   // Sebastian's account code to break ... we need to investigate why and fix it.
   //
   // Until such time, stalling txes for increased concurrency MUST be disabled.
   //--------------------------------------------------------------------------

   allow_stall = false;

   //--------------------------------------------------------------------------
   // FOR THE TIME BEING, DO NOT ALLOW STALLS AT ALL! Stalls somehow cause
   // Sebastian's account code to break ... we need to investigate why and fix it.
   //
   // Until such time, stalling txes for increased concurrency MUST be disabled.
   //--------------------------------------------------------------------------


   // warm up the cache with this transaction's bloom filter
#if PERFORMING_WRITE_BLOOM
   bloom_filter &wbloom = this->wbloom();
   bool singleWriteComp = writeList().size() < 20;
#else
   bool const singleWriteComp = true;
#endif

   // iterate through all the in flight transactions
   for (InflightTxes::iterator j = transactionsInFlight_.begin();
   j != transactionsInFlight_.end(); ++j)
   {
      transaction *t = (transaction*)*j;
      // if we're already aborting for this transaction, skip it
      if (t->forced_to_abort()) continue;

      //////////////////////////////////////////////////////////////////////
      // as only one thread can commit at a time, and we know that
      // composed txs, handoff, rather than commit, we should never be
      // in this loop when another inflight tx has our threadid.
      //////////////////////////////////////////////////////////////////////
      if (t->threadId_ == this->threadId_) continue;

      if (singleWriteComp)
      {
         // iterate through all our written memory
         for (WriteContainer::iterator i = writeList().begin(); writeList().end() != i; ++i)
         {
            //////////////////////////////////////////////////////////////////////
            // if t's readList is reading memory we are modifying, make t bail
            //////////////////////////////////////////////////////////////////////
            if (t->bloom().exists(i->first))
            {
               if (allow_stall && t->is_only_reading())// && t->reads() > work)
               {
                  ++stalls_;
                  stallingOn = t;
                  return false;
               }
               // if the conflict is not a write-write conflict, stall
#if 0
               if (allow_stall && !t->wbloom().exists((size_t)i->first))
               {
                  ++stalls_;
                  stallingOn = t;
                  return false;
               }
#endif
#if PERFORMING_LATM
               if (this->irrevocable())
               {
                  aborted.push_front(t);
               }
               else if (!t->irrevocable() && cm_->permission_to_abort(*this, *t))
               {
                  aborted.push_front(t);
               }
               else
               {
                  force_to_abort();
                  throw aborted_transaction_exception
                  ("aborting committing transaction due to contention manager priority inversion");
               }
#else
               aborted.push_front(t);
#endif
            }
         }
      }
#if PERFORMING_WRITE_BLOOM
      else if (wbloom.intersection(t->bloom()))
      {
         if (allow_stall && t->is_only_reading())// && t->reads() > work)
         {
            ++stalls_;
            stallingOn = t;
            return false;
         }
#if PERFORMING_LATM
         if (this->irrevocable())
         {
            aborted.push_front(t);
         }
         else if (!t->irrevocable() && cm_->permission_to_abort(*this, *t))
         {
            aborted.push_front(t);
         }
         else
         {
            force_to_abort();
            throw aborted_transaction_exception
            ("aborting committing transaction due to contention manager priority inversion");
         }
#else
         aborted.push_front(t);
#endif
      }
#endif
   }

   if (!aborted.empty())
   {
      if (cm_->permission_to_abort(*this, aborted))
      {
         // ok, forced to aborts are allowed, do them
         for (std::list<transaction*>::iterator k = aborted.begin();
              k != aborted.end(); ++k)
         {
            (*k)->force_to_abort();
         }

         aborted.clear();
      }
      else
      {
         force_to_abort();
         throw aborted_transaction_exception
         ("aborting committing transaction by contention manager");
      }
   }

   return true;
}

////////////////////////////////////////////////////////////////////////////
inline void boost::stm::transaction::forceOtherInFlightTransactionsReadingThisWriteMemoryToAbort()
{
   std::list<transaction*> aborted;

   // iterate through all the in flight transactions
   for (InflightTxes::iterator j = transactionsInFlight_.begin();
   j != transactionsInFlight_.end(); ++j)
   {
      transaction *t = (transaction*)*j;
      // if we're already aborting for this transaction, skip it
#ifndef DISABLE_READ_SETS
      if (!t->isReading()) continue;
#endif
      if (t->forced_to_abort()) continue;

      //////////////////////////////////////////////////////////////////////
      // as only one thread can commit at a time, and we know that
      // composed txs, handoff, rather than commit, we should never be
      // in this loop when another inflight tx has our threadid.
      //////////////////////////////////////////////////////////////////////
      assert(t->threadId_ != this->threadId_);

      // iterate through all our written memory
      for (WriteContainer::iterator i = writeList().begin(); writeList().end() != i; ++i)
      {

         //////////////////////////////////////////////////////////////////////
         // if t's readList is reading memory we are modifying, make t bail
         //////////////////////////////////////////////////////////////////////
#ifdef USE_BLOOM_FILTER
         if (t->bloom().exists(i->first))
#else
         if (t->readList().end() != t->readList().find(i->first))
#endif
         {
            if (this->irrevocable())
            {
               aborted.push_front(t);
            }
            else if (!t->irrevocable() && cm_->permission_to_abort(*this, *t))
            {
               aborted.push_front(t);
            }
            else
            {
               force_to_abort();
               //bookkeeping_.inc_abort_perm_denied(threadId_);
               throw aborted_transaction_exception
               ("aborting committing transaction due to contention manager priority inversion");
            }
         }
      }
   }

   // ok, forced to aborts are allowed, do them
   for (std::list<transaction*>::iterator k = aborted.begin(); k != aborted.end(); ++k)
   {
      (*k)->force_to_abort();
      //bookkeeping_.inc_abort_perm_denied((*k)->threadId_);
   }
}

//-----------------------------------------------------------------------------
// IMPORTANT ASSUMPTION:
//
// "this" must not be on the list. Thus, otherInFlightTransactionsOfSameThread
// must be checked BEFORE "this" is inserted into the transactionsInFlight_ if
// checked at ctor or begin_transaction().
//
// Or it must be checked AFTER "this" is removed from the transactionsInFlight_
//
//-----------------------------------------------------------------------------
inline bool boost::stm::transaction::other_in_flight_same_thread_transactions() const throw()
{
   for (InflightTxes::iterator i = transactionsInFlight_.begin();
      i != transactionsInFlight_.end(); ++i)
   {
      if (((transaction*)*i) == this) continue;
      // if this is not our threadId or this thread is not composable, skip it
      if (((transaction*)*i)->threadId_ != this->threadId_) continue;
      return true;
   }

   return false;
}

inline bool boost::stm::transaction::
otherInFlightTransactionsOfSameThreadNotIncludingThis(transaction const * const rhs)
{
   //////////////////////////////////////////////////////////////////////
   for (InflightTxes::iterator i = transactionsInFlight_.begin(); i != transactionsInFlight_.end(); ++i)
   {
      if (*i == rhs) continue;
      // if this is not our threadId or this thread is not composable, skip it
      if (((transaction*)*i)->threadId_ != this->threadId_) continue;
      return true;
   }

   return false;
}


#endif // TRANSACTION_IMPL_H

