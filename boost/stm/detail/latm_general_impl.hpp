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

//-----------------------------------------------------------------------------
//
// TransactionLockAwareImpl.h
//
// This file contains method implementations for transaction.hpp (specifically for
// enabling lock aware transactions). The main purpose of this file is to reduce 
// the complexity of the transaction class by separating its implementation into 
// a secondary .h file.
//
// Do NOT place these methods in a .cc/.cpp/.cxx file. These methods must be
// inlined to keep DracoSTM performing fast. If these methods are placed in a
// C++ source file they will incur function call overhead - something we need
// to reduce to keep performance high.
//
//-----------------------------------------------------------------------------
#ifndef BOOST_STM_TRANSACTION_LOCK_AWARE_GENERAL_IMPL_H
#define BOOST_STM_TRANSACTION_LOCK_AWARE_GENERAL_IMPL_H

#if PERFORMING_LATM

//-----------------------------------------------------------------------------
//
//
//
//                            GENERAL LATM INTERFACES
//
//
//
//-----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::wait_until_all_locks_are_released(bool keepLatmLocked)
{
   while (true) 
   {
      lock_latm_access();
      if (latmLockedLocks_.empty()) break;
      unlock_latm_access();
      SLEEP(10);
   }

   if (!keepLatmLocked) unlock_latm_access();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::add_to_obtained_locks(Mutex* m)
{
   obtainedLocksRef().insert(m);

#if LOGGING_BLOCKS
   logFile_ << "----------------------\ntx has obtained mutex: " << m << endl << endl;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline bool boost::stm::transaction::is_on_obtained_locks_list(Mutex *m)
{
   return obtainedLocksRef().find(m) != obtainedLocksRef().end();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline bool boost::stm::transaction::is_currently_locked_lock(Mutex *m)
{
   return currentlyLockedLocksRef().find(m) != currentlyLockedLocksRef().end();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::add_to_currently_locked_locks(Mutex* m)
{
   currentlyLockedLocksRef().insert(m);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::remove_from_currently_locked_locks(Mutex *m)
{
   currentlyLockedLocksRef().erase(m);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::clear_latm_obtained_locks()
{
   for (MutexSet::iterator i = obtainedLocksRef().begin(); i != obtainedLocksRef().end();)
   {
      Mutex* m = *i;
      obtainedLocksRef().erase(i);
      i = obtainedLocksRef().begin();

#if LOGGING_BLOCKS
      logFile_ << "----------------------\nbefore tx release unlocked mutex: " << m << endl << endl;
      logFile_ << outputBlockedThreadsAndLockedLocks() << endl;
#endif

      unblock_conflicting_threads(m);

#if LOGGING_BLOCKS
      logFile_ << "----------------------\nafter tx release unlocked mutex: " << m << endl << endl;
      logFile_ << outputBlockedThreadsAndLockedLocks() << endl;
#endif
   }

   unblock_threads_if_locks_are_empty();

   currentlyLockedLocksRef().clear();
}

//----------------------------------------------------------------------------
//
// PRE-CONDITION: latm_lock(), general_lock() and inflight_lock() are obtained
//                prior to calling this method.
//
//----------------------------------------------------------------------------
inline bool boost::stm::transaction::mutex_is_on_obtained_tx_list(Mutex *mutex)
{
   for (ThreadMutexSetContainer::iterator iter = threadObtainedLocks_.begin();
   threadObtainedLocks_.end() != iter; ++iter)
   {
      if (iter->second->find(mutex) != iter->second->end())
      {
         return true;
      }
   }
   return false;
}

//----------------------------------------------------------------------------
//
// PRE-CONDITION: latm_lock(), general_lock() and inflight_lock() are obtained
//                prior to calling this method.
//
//----------------------------------------------------------------------------
inline void boost::stm::transaction::unblock_conflicting_threads(Mutex *mutex)
{
   // if the mutex is on the latm locks map, we can't unblock yet
   if (latmLockedLocksAndThreadIdsMap_.find(mutex) != latmLockedLocksAndThreadIdsMap_.end()) 
   {
#if LOGGING_BLOCKS
      logFile_ << "\ncannot unlock <" << mutex << ">, in latmLockedLocksAndThreadIdsMap_" << endl << endl;
#endif
      return;
   }

   // if the mutex is in any tx threads, we can't unblock yet
   if (mutex_is_on_obtained_tx_list(mutex))
   {
#if LOGGING_BLOCKS
      logFile_ << "\ncannot unlock <" << mutex << ">, in mutex_is_on_obtained_tx_list" << endl << endl;
#endif
      return;
   }

   thread_conflicting_mutexes_set_all_cnd(mutex, false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::unblock_threads_if_locks_are_empty()
{
#if 0
   // if the size is 0, unblock everybody
   if (latmLockedLocksOfThreadMap_.empty())
   {
      for (ThreadMutexSetContainer::iterator it = threadObtainedLocks_.begin(); 
      it != threadObtainedLocks_.end(); ++it)
      {
         if (!it->second->empty()) return;
      }
      thread_conflicting_mutexes_set_all(false);
   }
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline bool boost::stm::transaction::doing_full_lock_protection()
{
   return eFullLatmProtection == eLatmType_;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::do_full_lock_protection()
{
   eLatmType_ = eFullLatmProtection;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline bool boost::stm::transaction::doing_tm_lock_protection()
{
   return eTmConflictingLockLatmProtection == eLatmType_;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::do_tm_lock_protection()
{
   eLatmType_ = eTmConflictingLockLatmProtection;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline bool boost::stm::transaction::doing_tx_lock_protection()
{
   return eTxConflictingLockLatmProtection == eLatmType_;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::do_tx_lock_protection()
{
   eLatmType_ = eTxConflictingLockLatmProtection;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline std::string const boost::stm::transaction::latm_protection_str()
{
   using namespace boost::stm;

   switch (eLatmType_)
   {
   case eFullLatmProtection: 
      return "full_protect";
   case eTmConflictingLockLatmProtection:
      return "tm_protect";
   case eTxConflictingLockLatmProtection:
      return "tx_protect";
   default:
      throw "invalid LATM type";
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::tm_lock_conflict(Mutex *inLock)
{
   if (!doing_tm_lock_protection()) return;

   lock(&latmMutex_);

   //-------------------------------------------------------------------------
   // insert can throw an exception
   //-------------------------------------------------------------------------
   try { tmConflictingLocks_.insert(inLock); } 
   catch (...) 
   {
      unlock(&latmMutex_);
      throw;
   }
   unlock(&latmMutex_);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::clear_tm_conflicting_locks()
{
   lock(&latmMutex_);
   tmConflictingLocks_.clear();
   unlock(&latmMutex_);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::must_be_in_tm_conflicting_lock_set(Mutex *inLock)
{
   if (tmConflictingLocks_.find(inLock) == tmConflictingLocks_.end()) 
   {
      throw "lock not in tx conflict lock set, use add_tm_conflicting_lock";
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::must_be_in_conflicting_lock_set(Mutex *inLock)
{
   if (get_tx_conflicting_locks().find(inLock) == get_tx_conflicting_locks().end()) 
   {
      throw "lock not in tx conflict lock set, use add_tx_conflicting_lock";
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::add_tx_conflicting_lock(Mutex *inLock)
{
   if (!doing_tx_lock_protection()) return;

   {
      var_auto_lock<PLOCK> autol(latm_lock(), general_lock(), inflight_lock(), NULL);

      if (get_tx_conflicting_locks().find(inLock) != get_tx_conflicting_locks().end()) return;
      get_tx_conflicting_locks().insert(inLock);

      if (irrevocable()) return;

      see_if_tx_must_block_due_to_tx_latm();
   }

   if (blocked()) 
   {
      lock_and_abort();
      throw aborted_transaction_exception("aborting transaction");
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void boost::stm::transaction::clear_tx_conflicting_locks()
{
   lock_general_access();
   get_tx_conflicting_locks().clear();
   unlock_general_access();
}

//----------------------------------------------------------------------------
// 
// Exposed client interfaces that act as forwarding calls to the real 
// implementation based on the specific type of lock-aware transactions
// the client chose
//
//----------------------------------------------------------------------------
inline int boost::stm::transaction::pthread_lock(Mutex *mutex)
{
   using namespace boost::stm;

   switch (eLatmType_)
   {
   case eFullLatmProtection: 
      if (direct_updating()) return dir_full_pthread_lock_mutex(mutex);
      else return def_full_pthread_lock_mutex(mutex);
   case eTmConflictingLockLatmProtection:
      if (direct_updating()) return dir_tm_conflicting_lock_pthread_lock_mutex(mutex);
      else return def_tm_conflicting_lock_pthread_lock_mutex(mutex);
   case eTxConflictingLockLatmProtection:
      if (direct_updating()) return dir_tx_conflicting_lock_pthread_lock_mutex(mutex);
      else return def_tx_conflicting_lock_pthread_lock_mutex(mutex);
   default:
      throw "invalid LATM type";
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int boost::stm::transaction::pthread_trylock(Mutex *mutex)
{
   using namespace boost::stm;

   switch (eLatmType_)
   {
   case eFullLatmProtection: 
      if (direct_updating()) return dir_full_pthread_trylock_mutex(mutex);
      else return def_full_pthread_trylock_mutex(mutex);
   case eTmConflictingLockLatmProtection:
      if (direct_updating()) return dir_tm_conflicting_lock_pthread_trylock_mutex(mutex);
      else return def_tm_conflicting_lock_pthread_trylock_mutex(mutex);
   case eTxConflictingLockLatmProtection:
      if (direct_updating()) return dir_tx_conflicting_lock_pthread_trylock_mutex(mutex);
      else return def_tx_conflicting_lock_pthread_trylock_mutex(mutex);
   default:
      throw "invalid LATM type";
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int boost::stm::transaction::pthread_unlock(Mutex *mutex)
{
   using namespace boost::stm;

   switch (eLatmType_)
   {
   case eFullLatmProtection: 
      if (direct_updating()) return dir_full_pthread_unlock_mutex(mutex);
      return def_full_pthread_unlock_mutex(mutex);
   case eTmConflictingLockLatmProtection:
      if (direct_updating()) return dir_tm_conflicting_lock_pthread_unlock_mutex(mutex);
      else return def_tm_conflicting_lock_pthread_unlock_mutex(mutex);
   case eTxConflictingLockLatmProtection:
      if (direct_updating()) return dir_tx_conflicting_lock_pthread_unlock_mutex(mutex);
      else return def_tx_conflicting_lock_pthread_unlock_mutex(mutex);
   default:
      throw "invalid LATM type";
   }
}

//-----------------------------------------------------------------------------
//
//
//
//                TRANSACTION CONFLICTING LOCK LATM PROTECTION METHODS
//
//
//
//-----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//
// ASSUMPTION: latmMutex_ MUST BE OBTAINED BEFORE CALLING THIS METHOD
//
//----------------------------------------------------------------------------
inline void boost::stm::transaction::see_if_tx_must_block_due_to_tx_latm()
{
   //--------------------------------------------------------------------------
   // iterate through all currently locked locks 
   //--------------------------------------------------------------------------
   for (MutexThreadSetMap::iterator iter = latmLockedLocksAndThreadIdsMap_.begin(); 
   latmLockedLocksAndThreadIdsMap_.end() != iter; ++iter)
   {
      //-----------------------------------------------------------------------
      // iterate through this transaction's conflicting mutex ref - if one of
      // the currently locked locks is in this tx's conflicting mutex set,
      // we need to block this tx
      //-----------------------------------------------------------------------
      if (get_tx_conflicting_locks().find(iter->first) != get_tx_conflicting_locks().end())
      {
         this->block(); break;
      }
   }

   block_if_conflict_mutex();

   for (MutexSet::iterator k = get_tx_conflicting_locks().begin(); k != get_tx_conflicting_locks().end(); ++k)
   {
      // if it is locked by our thread, it is ok ... otherwise it is not
      MutexThreadMap::iterator l = latmLockedLocksOfThreadMap_.find(*k);

      if (l != latmLockedLocksOfThreadMap_.end() && 
         THREAD_ID != l->second)
      {
         MutexThreadSetMap::iterator locksAndThreadsIter = latmLockedLocksAndThreadIdsMap_.find(*k);

         if (locksAndThreadsIter == latmLockedLocksAndThreadIdsMap_.end())
         {
            ThreadIdSet s;
            s.insert(THREAD_ID);

            latmLockedLocksAndThreadIdsMap_.insert
            (std::make_pair<Mutex*, ThreadIdSet>(*k, s)); 
         }
         else
         {
            locksAndThreadsIter->second.insert(THREAD_ID);
         }

         this->block(); break;
      }
   }

}

//----------------------------------------------------------------------------
//
// ASSUMPTION: latmMutex_ MUST BE OBTAINED BEFORE CALLING THIS METHOD
//
//----------------------------------------------------------------------------
inline int boost::stm::transaction::
thread_id_occurance_in_locked_locks_map(size_t threadId)
{
   int count = 0;

   for (MutexThreadSetMap::iterator iter = latmLockedLocksAndThreadIdsMap_.begin(); 
      latmLockedLocksAndThreadIdsMap_.end() != iter; ++iter)
   {
      if (iter->second.find(threadId) != iter->second.end()) ++count;
   }

   return count;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
inline boost::stm::transaction* boost::stm::transaction::get_inflight_tx_of_same_thread
(bool hasTxInFlightMutex)
{
   if (!hasTxInFlightMutex) lock_inflight_access();

   for (InflightTxes::iterator i = transactionsInFlight_.begin(); 
      i != transactionsInFlight_.end(); ++i)
   {
      transaction *t = (transaction*)*i;

      //--------------------------------------------------------------------
      // if this tx's thread is the same thread iterating through the in-flight
      // txs, then this lock is INSIDE this tx - don't abort the tx, just
      // make it isolated and ensure it is performing direct updating
      //--------------------------------------------------------------------
      if (t->thread_id() == THREAD_ID)
      {
         if (!hasTxInFlightMutex) unlock_inflight_access();
         return t;
      }
   }

   if (!hasTxInFlightMutex) unlock_inflight_access();
   return NULL;
}

#include <boost/stm/detail/latm_def_full_impl.hpp>
#include <boost/stm/detail/latm_dir_full_impl.hpp>

#endif

#endif // TRANSACTION_LOCK_AWARE_GENERAL_IMPL_H

