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

#include <boost/stm/contention_manager.hpp>
#include <boost/stm/transaction.hpp>
#include <pthread.h>

eContentionManager currentCm = iAggr;

using namespace boost::stm;

////////////////////////////////////////////////////////////////////////////
void DefaultContentionManager::abort_on_write
(transaction &t, base_transaction_object const &in)
{
   t.lock_and_abort();
   throw aborted_transaction_exception("aborting transaction");
}

/////////////////////////////////////////////////////////////////////////
void DefaultContentionManager::abort_on_delete(boost::stm::transaction const &t,
   boost::stm::base_transaction_object const &in)
{
}

/////////////////////////////////////////////////////////////////////////
void DefaultContentionManager::abort_on_read
(transaction const &constT, base_transaction_object const &in)
{
   transaction &t = const_cast<transaction&>(constT);

   t.lock_and_abort();
   throw aborted_transaction_exception("aborting transaction");
}

/////////////////////////////////////////////////////////////////////////
void DefaultContentionManager::abort_on_new(boost::stm::transaction const &constT)
{
   transaction &t = const_cast<transaction&>(constT);

   t.lock_and_abort();
   throw aborted_transaction_exception("aborting transaction");
}

////////////////////////////////////////////////////////////////////////////
bool DefaultContentionManager::allow_lock_to_abort_tx
(int const & lockWaitTime, int const &lockAborted, bool txTryingToAbortIsIrrevocable, 
 boost::stm::transaction const &rhs)
{
   if (txTryingToAbortIsIrrevocable) return true;
#ifndef DISABLE_READ_SETS
   if ((size_t)lockWaitTime > rhs.read_set_size() + rhs.writes()) return true;
#else
   if ((size_t)lockWaitTime > 100 + 100 * rhs.writes()) return true;
#endif

   else return false;
}

////////////////////////////////////////////////////////////////////////////
void DefaultContentionManager::
perform_isolated_tx_wait_priority_promotion(boost::stm::transaction &lhs)
{
   lhs.raise_priority();
}

////////////////////////////////////////////////////////////////////////////
void DefaultContentionManager::
perform_irrevocable_tx_wait_priority_promotion(boost::stm::transaction &lhs)
{
   lhs.raise_priority();
}

////////////////////////////////////////////////////////////////////////////
void ExceptAndBackOffOnAbortNoticeCM::abort_on_write
(transaction &t, base_transaction_object const &in)
{
   //SLEEP(initialSleepTime_);
   t.lock_and_abort();
   throw aborted_transaction_exception("aborting transaction");
}

/////////////////////////////////////////////////////////////////////////
void ExceptAndBackOffOnAbortNoticeCM::abort_on_read
(transaction const &constT, base_transaction_object const &in)
{
   transaction &t = const_cast<transaction&>(constT);

   //SLEEP(initialSleepTime_);
   t.lock_and_abort();
   throw aborted_transaction_exception("aborting transaction");
}

/////////////////////////////////////////////////////////////////////////
void ExceptAndBackOffOnAbortNoticeCM::abort_on_delete
(transaction const &constT, base_transaction_object const &in)
{
   transaction &t = const_cast<transaction&>(constT);

   //SLEEP(initialSleepTime_);
   t.lock_and_abort();
   throw aborted_transaction_exception("aborting transaction");
}

/////////////////////////////////////////////////////////////////////////
void ExceptAndBackOffOnAbortNoticeCM::abort_on_new(boost::stm::transaction const &constT)
{
   transaction &t = const_cast<transaction&>(constT);

   //SLEEP(initialSleepTime_);
   t.lock_and_abort();
   throw aborted_transaction_exception("aborting transaction");
}
