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

#ifndef BOOST_STM_CONTENTION_MANAGER_H
#define BOOST_STM_CONTENTION_MANAGER_H

#include <boost/stm/transaction.hpp>

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
enum eContentionManager
{
   eMinContentionManager = 0,
   iAggr = eMinContentionManager,
   iPrio,
   iFair,
   threadFair,
   iBalanced,
   eMaxContentionManager
};

extern eContentionManager currentCm;

//#define iAggr 1
//#define iPrio 1
//#define iFair 1
//#define threadFair 1
//#define iBalanced 1


////////////////////////////////////////////////////////////////////////////
//
// this class does nothing on abort notices for writes and reads
//
// this is an important concept as sometimes the overhead from the exception
// handling is more severe than simply aborting at "commit" time.
//
////////////////////////////////////////////////////////////////////////////
class NoExceptionOnAbortNoticeOnReadWritesCM : public boost::stm::base_contention_manager
{
public:
   /////////////////////////////////////////////////////////////////////////
   // do nothing for both of these interfaces
   /////////////////////////////////////////////////////////////////////////
   void abort_on_write(boost::stm::transaction &t,
      boost::stm::base_transaction_object const &in) {}
   void abort_on_read(boost::stm::transaction const &t,
      boost::stm::base_transaction_object const &in) {}
   void abort_on_delete(boost::stm::transaction const &t,
      boost::stm::base_transaction_object const &in) {}
   void abort_on_new(boost::stm::transaction const &t) {}
};

////////////////////////////////////////////////////////////////////////////
//
// this class backs off (and sleeps) when aborting. this is commonly known
// as "exponential backoff" for locking mechanisms.
//
////////////////////////////////////////////////////////////////////////////
class ExceptAndBackOffOnAbortNoticeCM : public boost::stm::base_contention_manager
{
public:

   ExceptAndBackOffOnAbortNoticeCM(int const initialSleepTime, int const sleepIncrease,
      int const maxIncreases)
      : sleepTime_(initialSleepTime), kSleepFactorIncrease_(sleepIncrease),
        kMaxIncreases_(maxIncreases), initialSleepTime_(initialSleepTime)
   {
      kMaxSleepTime_ = kSleepFactorIncrease_ * sleepTime_ * kMaxIncreases_;
   }

   ////////////////////////////////////////////////////////////////////////////
   void abort_on_write(boost::stm::transaction &t,
      boost::stm::base_transaction_object const &in);
   void abort_on_read(boost::stm::transaction const &t,
      boost::stm::base_transaction_object const &in);
   void abort_on_delete(boost::stm::transaction const &t,
      boost::stm::base_transaction_object const &in);
   void abort_on_new(boost::stm::transaction const &t);

   ////////////////////////////////////////////////////////////////////////////
   void perform_isolated_tx_wait_priority_promotion(boost::stm::transaction &lhs)
   {
      lhs.raise_priority();
   }

   ////////////////////////////////////////////////////////////////////////////
   void perform_irrevocable_tx_wait_priority_promotion(boost::stm::transaction &lhs)
   {
      lhs.raise_priority();
   }

   //--------------------------------------------------------------------------
   // this code is only ever called if "validation" is on not "invalidation"
   // so don't worry about this code EVER stopping invalidation from committing
   //--------------------------------------------------------------------------
   virtual bool abort_before_commit(boost::stm::transaction const &t)
   {
      using namespace boost::stm;

      for (transaction::in_flight_trans_cont::const_iterator i =
           transaction::in_flight_transactions().begin();
      i != transaction::in_flight_transactions().end(); ++i)
      {
         if (t.priority() < (*i)->priority()) return true;
      }

      return false;
   }

   virtual bool permission_to_abort
      (boost::stm::transaction const &lhs, boost::stm::transaction const &rhs)
   {
      return true;
#if 0
      if (lhs.priority() + 100 > rhs.priority()) return true;

      rhs.set_priority(rhs.priority() + 1000);
      return false;
#endif
      //return lhs.writes() * lhs.priority() >= rhs.writes() * rhs.priority();
      //return lhs.writes() + lhs.reads() >= rhs.writes() + rhs.reads();
   }

   virtual bool permission_to_abort
      (boost::stm::transaction const &lhs, 
       std::list<boost::stm::transaction*> &rhs)
   {
      //############################################################################
      //############################################################################
      switch (currentCm)
      {
      //############################################################################
      //############################################################################
      case iAggr:
      {
         return true;
         break;
      }

      //############################################################################
      //############################################################################
      case iPrio:
      {
         int setSize = (lhs.writes() * lhs.priority()) +
                       (lhs.reads() * lhs.priority());
         double abortSetSize = 0;
         double abortPriority = 0;
         double decrementing = 1.0;

         double highestPriority = 0;

         bool hasLargestReadSet = true;

         for (std::list<boost::stm::transaction*>::iterator iter = rhs.begin();
              iter != rhs.end(); ++iter)
         {
            if ((*iter)->priority() > highestPriority) 
            {
               highestPriority = (*iter)->priority();
            }

            if ((*iter)->reads() > lhs.reads()) hasLargestReadSet = false;
            if ((*iter)->writes() > 0) return true;

           abortSetSize += (double)(*iter)->reads() / decrementing;
           abortPriority += (double)(*iter)->priority() / decrementing;
           decrementing += 0.5;
         }

         if (lhs.priority() >= highestPriority) return true;

         if (hasLargestReadSet) return true;

         if (setSize >= abortPriority + abortSetSize)
         {
            return true;
         }
         else
         {
            return false;
         }
         break;

      }

      //############################################################################
      //############################################################################
      case threadFair:
      {
         for (std::list<boost::stm::transaction*>::iterator iter = rhs.begin(); 
              iter != rhs.end(); ++iter)
         {
            if ((*iter)->commits() < lhs.commits()) return false;
         }

         return true;

         break;
      }

      //############################################################################
      //############################################################################
      case iFair:
      {
         double setSize = (lhs.writes() * lhs.priority()) + 
            (lhs.reads() * lhs.priority());
         double abortSetSize = 0;
         double abortPriority = 0;
         bool hasLargestReadSet = true;

         int mem = lhs.reads() + (lhs.writes() * 10);

         double decrementing = 1.0;
         for (std::list<boost::stm::transaction*>::iterator iter = rhs.begin(); 
              iter != rhs.end(); ++iter)
         {
            if ((*iter)->reads() > (unsigned int)mem) hasLargestReadSet = false;

            if ((*iter)->writes() > 0) return true;

            if (lhs.reads() < (*iter)->reads() / 8 &&
                lhs.priority() * 100 < (*iter)->priority()) return false;

           abortSetSize += (double)(*iter)->reads() / decrementing;
           abortPriority += (double)(*iter)->priority() / decrementing;
           decrementing += 0.5;
         }

         if (hasLargestReadSet) return true;

         if (setSize >= 
            (abortPriority / setSize) + (abortSetSize / setSize))
         {
            return true;
         }
         else
         {
            return false;
         }

         break;
      }

      //############################################################################
      //############################################################################
      case iBalanced:
      {
         int setSize = (lhs.writes() * (lhs.priority() + 3)) +
                       (lhs.reads() * (lhs.priority() + 1));

         double abortSetSize = 0;
         double abortPriority = 0;
         double decrementing = 1.0;

         double highestPriority = 0;

         bool hasLargestReadSet = true;

         //---------------------------------------------------------------------
         // added for rb tree profiling test
         //---------------------------------------------------------------------
         bool hasFewestCommits = true;

         for (std::list<boost::stm::transaction*>::iterator iter = rhs.begin();
              iter != rhs.end(); ++iter)
         {
            if ((*iter)->commits() < lhs.commits()) hasFewestCommits = false;

            if ((*iter)->priority() > highestPriority)
            {
               highestPriority = (*iter)->priority();
            }

            if ((*iter)->reads() > lhs.reads()) hasLargestReadSet = false;
            //if ((*iter)->writes() > 0) return true;

           abortSetSize += ((double)(*iter)->writes() + (double)(*iter)->reads());
           abortPriority += ((double)(*iter)->priority() + 1.0);
           //decrementing += 1.0;
         }

         int thisPriority = lhs.priority() * (hasFewestCommits ? 3 : 1);

         if (thisPriority >= highestPriority) return true;

         if (hasLargestReadSet || hasFewestCommits) return true;

         if (hasFewestCommits) setSize *= 3;

         if (setSize >= abortPriority + abortSetSize)
         {
            return true;
         }
         else
         {
            return false;
         }
         break;
      }

      //############################################################################
      //############################################################################
      default:
      {
         return true;
         break;
      }
      }
   }


   virtual bool allow_lock_to_abort_tx
   (int const & lockWaitTime, int const &lockAborted,
   bool txTryingToAbortIsIrrevocable, boost::stm::transaction const &rhs)
   {
      if (txTryingToAbortIsIrrevocable) return true;

#ifndef DISABLE_READ_SETS
      if ((size_t)lockWaitTime > rhs.read_set_size() + 100 * rhs.writes())
#else
      if ((size_t)lockWaitTime > 100 * rhs.writes())
#endif
      {
         return true;
      }
      else return false;
   }

private:

   int sleepTime_;
   int const kSleepFactorIncrease_;
   int kMaxSleepTime_;
   int const kMaxIncreases_;
   int const initialSleepTime_;
};

#endif // CONTENTION_MANAGER_H
