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
// LatmDefTmImlp.h
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
#ifndef BOOST_STM_LATM_DEF_TM_IMPL_H
#define BOOST_STM_LATM_DEF_TM_IMPL_H

#if PERFORMING_LATM

//-----------------------------------------------------------------------------
//
//
//
//                  GLOBAL CONFLICTING LOCK LATM PROTECTION METHODS
//
//
//
//-----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//
// ASSUMPTION: latmMutex_ MUST BE OBTAINED BEFORE CALLING THIS METHOD
//
//----------------------------------------------------------------------------
inline bool boost::stm::transaction::def_do_core_tm_conflicting_lock_pthread_lock_mutex
(Mutex *mutex, int lockWaitTime, int lockAborted)
{
   //--------------------------------------------------------------------------
   // if this mutex is on the tmConflictingLocks_ set, then we need to stop
   // transactions which are in-flight.
   //
   // if it isn't we don't interrupt any in-flight transactions
   //--------------------------------------------------------------------------
   if (tmConflictingLocks_.find(mutex) != tmConflictingLocks_.end())
   {
      // if the lock-aware tm lock set is empty, lock the in-flight trans mutex
      // so we can abort all the in-flight transactions
      if (latmLockedLocks_.empty()) 
      {
         var_auto_lock<PLOCK> autlock(general_lock(), inflight_lock(), 0);

         std::list<transaction*> txList;

         for (InflightTxes::iterator i = transactionsInFlight_.begin(); 
            i != transactionsInFlight_.end(); ++i)
         {
            transaction *t = (transaction*)*i;

            if (!t->irrevocable() && 
               cm_->allow_lock_to_abort_tx(lockWaitTime, lockAborted, false, *t))
            {
               txList.push_back(t);
            }
            else return false;
         }

         for (std::list<transaction*>::iterator it = txList.begin(); txList.end() != it; ++it)
         {
            (*it)->force_to_abort();
         }
      }

      latmLockedLocks_.insert(mutex);
   }

   return true;
}

//----------------------------------------------------------------------------
// only allow one thread to execute any of these methods at a time
//----------------------------------------------------------------------------
inline int boost::stm::transaction::
def_tm_conflicting_lock_pthread_lock_mutex(Mutex *mutex)
{
   if (transaction* t = get_inflight_tx_of_same_thread(false))
   {
      transaction::must_be_in_tm_conflicting_lock_set(mutex);
      t->make_isolated();
      t->commit_deferred_update_tx();

      bool hadLock = t->is_currently_locked_lock(mutex);
      t->add_to_currently_locked_locks(mutex);

      lock_latm_access();
      latmLockedLocksOfThreadMap_[mutex] = THREAD_ID;
      unlock_latm_access();

      // TBR if (hadLock) return 0;
      // TBR else return lock(mutex);
      if (hadLock) return 0;
      else lock(mutex);
      return 0;
   }

   int waitTime = 0, aborted = 0;

   for (;;)
   {
      // TRR int val = lock(mutex);
      // TRR if (0 != val) return val;
      lock(mutex);

      lock(&latmMutex_);

      try 
      { 
         //--------------------------------------------------------------------
         // if we are able to do the core lock work, break
         //--------------------------------------------------------------------
         if (def_do_core_tm_conflicting_lock_pthread_lock_mutex
            (mutex, waitTime, aborted)) break;
      }
      catch (...)
      {
         unlock(mutex);
         unlock(&latmMutex_);
         throw;
      }

      //-----------------------------------------------------------------------
      // we weren't able to do the core lock work, unlock our mutex and sleep
      //-----------------------------------------------------------------------
      unlock(mutex);
      unlock(&latmMutex_);

      SLEEP(cm_->lock_sleep_time());
      waitTime += cm_->lock_sleep_time();
      ++aborted;
   }

   latmLockedLocksOfThreadMap_[mutex] = THREAD_ID;
   unlock(&latmMutex_);

   return 0;
}

//----------------------------------------------------------------------------
// only allow one thread to execute any of these methods at a time
//----------------------------------------------------------------------------
inline int boost::stm::transaction::
def_tm_conflicting_lock_pthread_trylock_mutex(Mutex *mutex)
{
   if (transaction* t = get_inflight_tx_of_same_thread(false))
   {
      transaction::must_be_in_tm_conflicting_lock_set(mutex);
      t->make_isolated();
      t->commit_deferred_update_tx();

      bool hadLock = t->is_currently_locked_lock(mutex);
      t->add_to_currently_locked_locks(mutex);

      lock_latm_access();
      latmLockedLocksOfThreadMap_[mutex] = THREAD_ID;
      unlock_latm_access();

      if (hadLock) return 0;
      else return trylock(mutex);
   }

   int val = trylock(mutex);
   if (0 != val) return val;

   lock(&latmMutex_);

   try 
   { 
      //-----------------------------------------------------------------------
      // if !core done, since trylock, we cannot stall & retry - just exit
      //-----------------------------------------------------------------------
      if (!def_do_core_tm_conflicting_lock_pthread_lock_mutex(mutex, 0, 0)) 
      {
         unlock(mutex);
         unlock(&latmMutex_);
         return -1;
      }
   }
   catch (...)
   {
      unlock(mutex);
      unlock(&latmMutex_);
      throw;
   }

   latmLockedLocksOfThreadMap_[mutex] = THREAD_ID;
   unlock(&latmMutex_);
   // note: we do not release the transactionsInFlightMutex - this will prevents 
   // new transactions from starting until this lock is released
   return 0;
}

//----------------------------------------------------------------------------
// only allow one thread to execute any of these methods at a time
//----------------------------------------------------------------------------
inline int boost::stm::transaction::
def_tm_conflicting_lock_pthread_unlock_mutex(Mutex *mutex)
{
   lock(&latmMutex_);
   bool hasLock = true;

   if (transaction* t = get_inflight_tx_of_same_thread(false))
   {
      if (!t->is_on_obtained_locks_list(mutex))
      {
         // this is illegal, it means the transaction is unlocking a lock
         // it did not obtain (e.g., early release) while the transaction
         // is still in-flight. Throw exception
         throw "lock released for transaction that did not obtain it";
      }

      if (!t->is_currently_locked_lock(mutex)) hasLock = false;
      t->remove_from_currently_locked_locks(mutex);
   }

   //--------------------------------------------------------------------------
   // if this mutex is on the tmConflictingLocks_ set, then we need to remove
   // it from the latmLocks and check to see if we allow transactions to
   // continue.
   //--------------------------------------------------------------------------
   if (tmConflictingLocks_.find(mutex) != tmConflictingLocks_.end())
   {
      latmLockedLocks_.erase(mutex);

      if (latmLockedLocks_.empty()) unlock_inflight_access();
   }

   latmLockedLocksOfThreadMap_.erase(mutex);
   unlock(&latmMutex_);

   // TBR if (hasLock) return unlock(mutex);
   // TBR else return 0;
   if (hasLock) unlock(mutex);
   return 0;
}


#endif

#endif // LATM_DIR_TM_IMPL_H


