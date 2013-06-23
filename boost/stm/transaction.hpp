
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

#ifndef BOOST_STM_TRANSACTION__HPP
#define BOOST_STM_TRANSACTION__HPP

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include <boost/stm/detail/config.hpp>
#include <boost/stm/detail/datatypes.hpp>
#include <boost/stm/detail/transaction_bookkeeping.hpp>
#include <boost/stm/base_transaction.hpp>
#include <boost/stm/detail/bloom_filter.hpp>
#include <boost/stm/detail/vector_map.hpp>
#include <boost/stm/detail/vector_set.hpp>
#include <assert.h>
#include <string>
#include <iostream>
#include <list>
#include <set>
#include <map>
#include <vector>
#include <pthread.h>

#include <boost/stm/detail/transactions_stack.hpp>

#if BUILD_MOVE_SEMANTICS
#include <type_traits>
#endif

#define CAPTURING_PROFILE_DATA 1

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
namespace boost { namespace stm {

   enum LatmType
   {
      kMinLatmType = 0,
      eFullLatmProtection = kMinLatmType,
      eTmConflictingLockLatmProtection,
      eTxConflictingLockLatmProtection,
      kMaxLatmType
   };

   enum TxType
   {
      kMinIrrevocableType = 0,
      eNormalTx = kMinIrrevocableType,
      eIrrevocableTx,
      eIrrevocableAndIsolatedTx,
      kMaxIrrevocableType
   };

   typedef std::pair<base_transaction_object*, base_transaction_object*> tx_pair;

#if 0 // these are not portable for Visual Studio 6
   template <typename MUTEX, MUTEX& mtx>
   struct locker {
       inline locker();
       inline ~locker();
       inline void unlock();
   };
#endif


#if CAPTURING_PROFILE_DATA
///////////////////////////////////////////////////////////////////////////////
class TxFileAndNumber
{
public:
   TxFileAndNumber(char* str, size_t num) { fileName_ = str; lineNumber_ = num; }

   bool operator<(TxFileAndNumber const &rhs) const
   {
      int val = strcmp(fileName_, rhs.fileName_);

      if (val < 0) return true;
      else if (val > 0) return false;

      return lineNumber_ < rhs.lineNumber_;
   }

   char *fileName_;
   size_t lineNumber_;
};
#endif

///////////////////////////////////////////////////////////////////////////////
// transaction Class
///////////////////////////////////////////////////////////////////////////////
class transaction
{
public:
   //--------------------------------------------------------------------------
   // transaction typedefs
   //--------------------------------------------------------------------------

#if PERFORMING_VALIDATION
   typedef std::map<base_transaction_object*, size_t> ReadContainer;
#else
   typedef std::set<base_transaction_object*> ReadContainer;
#endif
   typedef std::map<size_t, ReadContainer*> ThreadReadContainer;
   typedef std::map<size_t, size_t*> ThreadSizetMap;

   typedef std::multimap<base_transaction_object*, base_transaction_object*> MapOfTxObjects;

#ifdef MAP_WRITE_CONTAINER
   typedef std::map<base_transaction_object*, base_transaction_object*> WriteContainer;
#else
   typedef boost::stm::vector_map<base_transaction_object*, base_transaction_object*> WriteContainer;
#endif

#ifndef MAP_NEW_CONTAINER
   typedef boost::stm::vector_set<base_transaction_object*> MemoryContainerList;
#else
   typedef std::list<base_transaction_object*> MemoryContainerList;
#endif

#if CAPTURING_PROFILE_DATA
   typedef std::map<size_t, std::ostringstream*> ThreadOstringStream;
   typedef std::map<size_t, std::map<TxFileAndNumber, size_t>* > ThreadMapTxFileAndNumber;
#endif

   typedef std::map<size_t, TransactionsStack*> ThreadTransactionsStack;
   typedef std::map<size_t, WriteContainer*> ThreadWriteContainer;
   typedef std::map<size_t, TxType*> ThreadTxTypeContainer;

   typedef std::set<transaction*> TContainer;
   typedef std::set<transaction*> InflightTxes;

   typedef std::multimap<size_t, MemoryContainerList > DeletionBuffer;

    typedef std::set<Mutex*> MutexSet;

   typedef std::set<size_t> ThreadIdSet;

   typedef std::map<size_t, MemoryContainerList*> ThreadMemoryContainerList;

   typedef std::pair<size_t, Mutex*> thread_mutex_pair;
#ifndef MAP_THREAD_MUTEX_CONTAINER
   typedef boost::stm::vector_map<size_t, Mutex*> ThreadMutexContainer;
#else
   typedef std::map<size_t, Mutex*> ThreadMutexContainer;
#endif

   typedef std::map<size_t, MutexSet* > ThreadMutexSetContainer;
   typedef std::map<size_t, boost::stm::bloom_filter*> ThreadBloomFilterList;
#ifdef BOOST_STM_BLOOM_FILTER_USE_DYNAMIC_BITSET
typedef std::map<size_t, boost::dynamic_bitset<>*> ThreadBitVectorList;
#else
typedef std::map<size_t, boost::stm::bit_vector*> ThreadBitVectorList;
#endif
   typedef std::pair<size_t, int*> thread_bool_pair;
#ifndef MAP_THREAD_BOOL_CONTAINER
   typedef boost::stm::vector_map<size_t, int*> ThreadBoolContainer;
#else
   typedef std::map<size_t, int*> ThreadBoolContainer;
#endif

   typedef std::map<Mutex*, ThreadIdSet > MutexThreadSetMap;
   typedef std::map<Mutex*, size_t> MutexThreadMap;

   typedef std::set<transaction*> LockedTransactionContainer;

   typedef InflightTxes in_flight_transaction_container;
   typedef in_flight_transaction_container in_flight_trans_cont;

#ifdef USE_SINGLE_THREAD_CONTEXT_MAP
   struct tx_context
   {
      MemoryContainerList newMem;
      MemoryContainerList delMem;
      WriteContainer writeMem;
      bloom_filter wbloom;
      bloom_filter bloom;
      TxType txType;

      int abort;
   };

   typedef std::map<size_t, tx_context*> tss_context_map_type;
#endif

   //--------------------------------------------------------------------------
   // transaction static methods
   //--------------------------------------------------------------------------
   static void initialize();
   static void initialize_thread();
   static void terminate_thread();
   inline static void contention_manager(base_contention_manager *rhs) { delete cm_; cm_ = rhs; }
   inline static base_contention_manager* get_contention_manager() { return cm_; }

   inline static void enableLoggingOfAbortAndCommitSetSize() { bookkeeping_.setIsLoggingAbortAndCommitSize(true); }
   inline static void disableLoggingOfAbortAndCommitSetSize() { bookkeeping_.setIsLoggingAbortAndCommitSize(false); }

   inline static const transaction_bookkeeping & bookkeeping() { return bookkeeping_; }

   inline static bool early_conflict_detection() { return !directLateWriteReadConflict_ && direct_updating(); }
   inline static bool late_conflict_detection() { return directLateWriteReadConflict_ || !direct_updating(); }

   inline static bool enable_dynamic_priority_assignment()
   {
      return dynamicPriorityAssignment_ = true;
   }

   inline static bool disable_dynamic_priority_assignment()
   {
      return dynamicPriorityAssignment_ = false;
   }

   inline static bool using_move_semantics() { return usingMoveSemantics_; }
   inline static bool using_copy_semantics() { return !using_move_semantics(); }

   inline static void enable_move_semantics()
   {
      if (!kDracoMoveSemanticsCompiled)
         throw "move semantics off - rebuild with #define BUILD_MOVE_SEMANTICS 1";
      usingMoveSemantics_ = true;
   }

   inline static void disable_move_semantics()
   {
      usingMoveSemantics_ = false;
   }

   inline static bool doing_dynamic_priority_assignment()
   {
      return dynamicPriorityAssignment_;
   }

   static bool do_early_conflict_detection()
   {
      if (!transactionsInFlight_.empty()) return false;
      if (deferred_updating()) return false;
      else return !(directLateWriteReadConflict_ = false);
   }

   static bool do_late_conflict_detection()
   {
      if (!transactionsInFlight_.empty()) return false;
      else return directLateWriteReadConflict_ = true;
   }

   inline static std::string conflict_detection_string()
   {
      if (validating()) return "val";
      else return "inval";
   }

   inline static bool validating()
   {
#ifdef PERFORMING_VALIDATION
      return true;
#endif
      return false;
   }

   inline static bool invalidating() { return !validating(); }

   inline static bool direct_updating() { return direct_updating_; }
   inline static bool& direct_updating_ref() { return direct_updating_; }
   inline static bool deferred_updating() { return !direct_updating_; }

   //--------------------------------------------------------------------------
   // make all transactions direct as long as no transactions are in flight
   //--------------------------------------------------------------------------
   static bool do_direct_updating()
   {
      if (!transactionsInFlight_.empty()) return false;
      else direct_updating_ref() = true;
      return true;
   }

   //--------------------------------------------------------------------------
   // make all transactions deferred as long as no transactions are in flight
   //--------------------------------------------------------------------------
   static bool do_deferred_updating()
   {
      if (!transactionsInFlight_.empty()) return false;
      else direct_updating_ref() = false;
      return true;
   }

   static std::string update_policy_string()
   {
      return direct_updating() ? "dir" : "def";
   }

   //--------------------------------------------------------------------------
   // Lock Aware Transactional Memory support methods
   //--------------------------------------------------------------------------
   inline static LatmType const latm_protection() { return eLatmType_; }
   static std::string const latm_protection_str();
   static void do_full_lock_protection();
   static void do_tm_lock_protection();
   static void do_tx_lock_protection();

   static bool doing_full_lock_protection();
   static bool doing_tm_lock_protection();
   static bool doing_tx_lock_protection();


#ifdef WIN32
   template <typename T>
   inline static int lock_(T *lock) { throw "unsupported lock type"; }

   template <typename T>
   inline static int trylock_(T *lock) { throw "unsupported lock type"; }

   template <typename T>
   inline static int unlock_(T *lock) { throw "unsupported lock type"; }

   template <Mutex>
   inline static int lock_(Mutex &lock) { return pthread_lock(&lock); }

   template <Mutex>
   inline static int lock_(Mutex *lock) { return pthread_lock(lock); }

   template <Mutex>
   inline static int trylock_(Mutex &lock) { return pthread_trylock(&lock); }

   template <Mutex>
   inline static int trylock_(Mutex *lock) { return pthread_trylock(lock); }

   template <Mutex>
   inline static int unlock_(Mutex &lock) { return pthread_unlock(&lock); }

   template <Mutex>
   inline static int unlock_(Mutex *lock) { return pthread_unlock(lock); }
#else
   inline static int lock_(PLOCK &lock) { return pthread_lock(&lock); }
   inline static int lock_(PLOCK *lock) { return pthread_lock(lock); }

   inline static int trylock_(PLOCK &lock) { return pthread_trylock(&lock); }
   inline static int trylock_(PLOCK *lock) { return pthread_trylock(lock); }

   inline static int unlock_(PLOCK &lock) { return pthread_unlock(&lock); }
   inline static int unlock_(PLOCK *lock) { return pthread_unlock(lock); }
#endif

   static int pthread_lock(Mutex *lock);
   static int pthread_trylock(Mutex *lock);
   static int pthread_unlock(Mutex *lock);

   inline size_t const & commits() const { return *commits_ref_; }
   inline size_t & commits() { return *commits_ref_; }

   //##########################################################################
   // begin transaction::bind() implementation
   //##########################################################################

   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------
   void if_bound_perform_subreads(base_transaction_object* lhs)
   {
      lock_general_access();

      MapOfTxObjects::iterator i = threadBoundObjects_.find(lhs);

      if (i != threadBoundObjects_.end())
      {
         this->read(*(i->second));

         for (++i; i != threadBoundObjects_.end(); ++i)
         {
            if (i->first != lhs) break;
            this->read(*(i->second));
         }
      }

      unlock_general_access();
   }

   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------
   static void bind(base_transaction_object * lhs, 
      base_transaction_object * rhs)
   {
      //-----------------------------------------------------------------------
      // because bind might be called before the TM is initialized (due to 
      // global space allocation), we need to force a call to initialiaze()
      // so our general lock will be initialized, if initialized_ is false
      //-----------------------------------------------------------------------
      if (!initialized_) initialize();

      lock_general_access();
      threadBoundObjects_.insert(MapOfTxObjects::value_type(lhs, rhs));
      unlock_general_access();
   }

   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------
   static void unbind(base_transaction_object * lhs)
   {
      //-----------------------------------------------------------------------
      // because bind might be called before the TM is initialized (due to 
      // global space allocation), we need to force a call to initialiaze()
      // so our general lock will be initialized, if initialized_ is false
      //-----------------------------------------------------------------------
      if (!initialized_) initialize();

      lock_general_access();

      unlock_general_access();
   }

   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------
   static void unbind(base_transaction_object * lhs,
      base_transaction_object * rhs)
   {
      //-----------------------------------------------------------------------
      // because bind might be called before the TM is initialized (due to 
      // global space allocation), we need to force a call to initialiaze()
      // so our general lock will be initialized, if initialized_ is false
      //-----------------------------------------------------------------------
      if (!initialized_) initialize();

      lock_general_access();

      unlock_general_access();
   }

   //##########################################################################
   // end transaction::bind() implementation
   //##########################################################################

   //--------------------------------------------------------------------------
#if PERFORMING_LATM
   //--------------------------------------------------------------------------
   inline static void tm_lock_conflict(Mutex &lock)
   {
      tm_lock_conflict(&lock);
   }
   static void tm_lock_conflict(Mutex *lock);

   static void clear_tm_conflicting_locks();
   inline static MutexSet get_tm_conflicting_locks() { return tmConflictingLocks_; }

   void must_be_in_conflicting_lock_set(Mutex *inLock);
   static void must_be_in_tm_conflicting_lock_set(Mutex *inLock);

#if USING_TRANSACTION_SPECIFIC_LATM
   void see_if_tx_must_block_due_to_tx_latm();

   inline void lock_conflict(Mutex &lock)
   { add_tx_conflicting_lock(&lock); }

   inline void lock_conflict(Mutex *lock)
   { add_tx_conflicting_lock(lock); }

   inline void add_tx_conflicting_lock(Mutex &lock)
   {
      add_tx_conflicting_lock(&lock);
   }
   void add_tx_conflicting_lock(Mutex *lock);

   void clear_tx_conflicting_locks();
   //MutexSet get_tx_conflicting_locks() { return conflictingMutexRef_; }
#endif

   void add_to_obtained_locks(Mutex* );
   static void unblock_conflicting_threads(Mutex *mutex);
   static bool mutex_is_on_obtained_tx_list(Mutex *mutex);
   static void unblock_threads_if_locks_are_empty();
   void clear_latm_obtained_locks();

   void add_to_currently_locked_locks(Mutex* m);
   void remove_from_currently_locked_locks(Mutex *m);
   bool is_currently_locked_lock(Mutex *m);
   bool is_on_obtained_locks_list(Mutex *m);
#endif

   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------
   transaction();
   ~transaction();


   bool check_throw_before_restart() const
   {
      // if this transaction is in-flight or it committed, just ignore it
      if (in_flight() || committed()) return true;

      //-----------------------------------------------------------------------
      // if there is another in-flight transaction from this thread, and we
      // are using closed nesting with flattened transactions, we should throw
      // an exception here because restarting the transactions will cause it to
      // infinitely fail
      //-----------------------------------------------------------------------
      lock_inflight_access();
      if (other_in_flight_same_thread_transactions())
      {
         unlock_inflight_access();
         throw aborted_transaction_exception("closed nesting throw");
      }
      unlock_inflight_access();

      return true;
   }

   inline bool committed() const { return state() == e_committed || state() == e_hand_off; }
   inline bool aborted() const { return state() == e_aborted; }
   inline bool in_flight() const { return state() == e_in_flight; }

   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------
   template <typename T>
   T& find_original(T& in)
   {
      //-----------------------------------------------------------------------
      // if transactionThread_ is not invalid it means this is the original, so
      // we can return it. Otherwise, we need to search for the original in
      // our write set
      //-----------------------------------------------------------------------
      if (1 == in.new_memory()) return in;
      if (in.transaction_thread() == boost::stm::kInvalidThread) return in;

      base_transaction_object *inPtr = (base_transaction_object*)&in;

      for (WriteContainer::iterator i = writeList().begin(); i != writeList().end(); ++i)
      {
         if (i->second == (inPtr))
         {
            return *static_cast<T*>(i->first);
         }
      }

      //-----------------------------------------------------------------------
      // if it's not in our original / new list, then we need to except
      //-----------------------------------------------------------------------
      throw aborted_transaction_exception("original not found");
   }

   //--------------------------------------------------------------------------
   // The below methods change their behavior based on whether the transaction
   // is direct or deferred:
   //
   //    read           - deferred_read or direct_read
   //    write          - deferred_write or direct_write
   //    delete_memory  - deferred_delete_memory or direct_delete_memory
   //    commit         - deferred_commit or direct_commit
   //    abort          - deferred_abort or direct_abort
   //
   // In addition, a number of commit or abort methods must be made deferred or
   // direct as their version / memory management is entirely different if the
   // transaction is direct or deferred.
   //
   //--------------------------------------------------------------------------
#ifndef DISABLE_READ_SETS
   inline size_t const read_set_size() const { return readListRef_.size(); }
#endif

   inline size_t const writes() const { return write_list()->size(); }
   inline size_t const reads() const { return reads_; }

   template <typename T> T const * read_ptr(T const * in)
   {
      if (0 == in) return 0;
      return &read(*in);
   }
   template <typename T> T const & r(T const & in) { return read(in); }


   //--------------------------------------------------------------------------
   // attempts to find the written value of T based on
   //--------------------------------------------------------------------------
   template <typename T>
   T* get_written(T const & in)
   {
      if (direct_updating())
      {
         if (in.transaction_thread() == threadId_) return (T*)(&in);
         else return 0;
      }
      else
      {
         WriteContainer::iterator i = writeList().find
            ((base_transaction_object*)(&in));
         if (i == writeList().end()) return 0;
         else return static_cast<T*>(i->second);
      }
   }

   //--------------------------------------------------------------------------
   template <typename T>
   inline bool has_been_read(T const & in)
   {
#ifndef DISABLE_READ_SETS
      ReadContainer::iterator i = readList().find
         (static_cast<base_transaction_object*>(&in));
      //----------------------------------------------------------------
      // if this object is already in our read list, bail
      //----------------------------------------------------------------
      return i != readList().end();
#else
      return bloom().exists(&in);
#endif
   }

   //--------------------------------------------------------------------------
   template <typename T>
   inline T const & read(T const & in)
   {
      if (direct_updating())
      {
#if PERFORMING_VALIDATION
         throw "direct updating not implemented for validation yet";
#else
         return direct_read(in);
#endif
      }
      else
      {
         return deferred_read(in);
      }
   }

   //--------------------------------------------------------------------------
   template <typename T> T* write_ptr(T* in)
   {
      if (0 == in) return 0;
      return &write(*in);
   }
   template <typename T> T& w(T& in) { return write(in); }

   //--------------------------------------------------------------------------
   template <typename T>
   inline T& write(T& in)
   {
      if (direct_updating())
      {
#if PERFORMING_VALIDATION
         throw "direct updating not implemented for validation yet";
#else
         return direct_write(in);
#endif
      }
      else
      {
         return deferred_write(in);
      }
   }

   //--------------------------------------------------------------------------
   template <typename T>
   inline void delete_memory(T &in)
   {
      if (direct_updating())
      {
#if PERFORMING_VALIDATION
         throw "direct updating not implemented for validation yet";
#else
         direct_delete_memory(in);
#endif
      }
      else
      {
         deferred_delete_memory(in);
      }
   }

   //--------------------------------------------------------------------------
   // allocation of new memory behaves the same for both deferred and direct
   // transaction implementations
   //--------------------------------------------------------------------------

   //--------------------------------------------------------------------------
   template <typename T>
   T* new_shared_memory(T*)
   {
      if (forced_to_abort())
      {
         if (!direct_updating())
         {
            deferred_abort(true);
            throw aborted_tx("");
         }

#ifndef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
         cm_->abort_on_new(*this);
#endif
      }

      make_irrevocable();

      T *newNode = new T;
      newNode->transaction_thread(threadId_);
      newNode->new_memory(1);
      newMemoryList().push_back(newNode);

      return newNode;
   }

   //--------------------------------------------------------------------------
   template <typename T>
   T* new_memory(T*)
   {
      if (forced_to_abort())
      {
         if (!direct_updating())
         {
            deferred_abort(true);
            throw aborted_tx("");
         }

#ifndef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
         cm_->abort_on_new(*this);
#endif
      }
      T *newNode = new T();
      newNode->transaction_thread(threadId_);
      newNode->new_memory(1);
      newMemoryList().push_back(newNode);

      return newNode;
   }

   //--------------------------------------------------------------------------
   template <typename T>
   T* new_memory_copy(T const &rhs)
   {
      if (forced_to_abort())
      {
         if (!direct_updating())
         {
            deferred_abort(true);
            throw aborted_tx("");
         }
#ifndef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
         cm_->abort_on_new(*this);
#endif
      }
      T *newNode = new T(rhs);
      newNode->transaction_thread(threadId_);
      newNode->new_memory(1);
      newMemoryList().push_back(newNode);

      return newNode;
   }

    void throw_if_forced_to_abort_on_new() {
        if (forced_to_abort()) {
            if (!direct_updating()) {
                deferred_abort(true);
                throw aborted_tx("");
            }
#ifndef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
            cm_->abort_on_new(*this);
#endif
        }
    }

   template <typename T>
   T* as_new(T *newNode)
   {
      //std::auto_ptr<T> newNode(ptr);
      newNode->transaction_thread(threadId_);
      newNode->new_memory(1);
      newMemoryList().push_back(newNode);

      return newNode;
   }

   void begin();
   bool restart();

   inline bool restart_if_not_inflight()
   {
      if (in_flight()) return true;
      else return restart();
   }

   void end();
   void no_throw_end();

   void force_to_abort()
   {
      // can't abort irrevocable transactions
      if (irrevocable()) return;

      forced_to_abort_ref() = true;

#ifdef PERFORMING_COMPOSITION
#ifndef USING_SHARED_FORCED_TO_ABORT
      // now set all txes of this threadid that are in-flight to force to abort
      for (InflightTxes::iterator j = transactionsInFlight_.begin();
      j != transactionsInFlight_.end(); ++j)
      {
         transaction *t = (transaction*)*j;

         // if this is a parent or child tx, it must abort too
         if (t->threadId_ == this->threadId_) t->forced_to_abort_ref() = true;
      }
#endif
#endif
   }

   inline void unforce_to_abort() { forced_to_abort_ref() = false; }

   //--------------------------------------------------------------------------
   void lock_and_abort();

   inline size_t writeListSize() const { return write_list()->size(); }

   inline size_t const &priority() const { return priority_; }
   inline void set_priority(uint32 const &rhs) const { priority_ = rhs; }
   inline void raise_priority()
   {
      if (priority_ < size_t(-1))
      {
         ++priority_;
      }
   }

   inline static InflightTxes const & in_flight_transactions() { return transactionsInFlight_; }

   void make_irrevocable();
   void make_isolated();
   bool irrevocable() const;
   bool isolated() const;

   typedef size_t thread_id_t;
   inline size_t const & thread_id() const { return threadId_; }

private:

#ifdef LOGGING_BLOCKS
   static std::string outputBlockedThreadsAndLockedLocks();
#endif
   //--------------------------------------------------------------------------
   inline int const& hasLock() const { return hasMutex_; }
   void lock_tx();
   void unlock_tx();

   static void lock_latm_access();
   static void unlock_latm_access();

   static void lock_inflight_access();
   static void unlock_inflight_access();

   static void lock_general_access();
   static void unlock_general_access();

   inline static PLOCK* latm_lock() { return &latmMutex_; }
   inline static PLOCK* general_lock() { return &transactionMutex_; }
   inline static PLOCK* inflight_lock() { return &transactionsInFlightMutex_; }

   bool irrevocableTxInFlight();
   bool isolatedTxInFlight();
   void commit_deferred_update_tx();

   bool canAbortAllInFlightTxs();
   bool abortAllInFlightTxs();
   void put_tx_inflight();
   bool can_go_inflight();
   static transaction* get_inflight_tx_of_same_thread(bool);

#if !PERFORMING_VALIDATION
   //--------------------------------------------------------------------------
   //                      DIRECT UPDATING SECTION
   //--------------------------------------------------------------------------
   template <typename T>
   T const & direct_read(T const & in)
   {
      if (in.transaction_thread() == threadId_) return in;

      if (forced_to_abort())
      {
#ifndef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
         // let the contention manager decide to throw or not
         cm_->abort_on_write(*this, (base_transaction_object&)(in));
#endif
      }

      //--------------------------------------------------------------------
      // otherwise, see if its in our read list
      //--------------------------------------------------------------------
#ifndef DISABLE_READ_SETS
      ReadContainer::iterator i = readList().find((base_transaction_object*)&in);
      if (i != readList().end()) return in;
#endif
#if USE_BLOOM_FILTER
      if (bloom().exists(&in)) return in;
#endif

      //--------------------------------------------------------------------
      // if we want direct write-read conflict to be done late (commit time)
      // perform the following section of code
      //--------------------------------------------------------------------
      if (directLateWriteReadConflict_)
      {
         //--------------------------------------------------------------------
         // if transaction_thread() is not invalid (and not us), we get original
         // object from the thread that is changing it
         //--------------------------------------------------------------------
         lock(&transactionMutex_);
         lock_tx();

         if (in.transaction_thread() != boost::stm::kInvalidThread)
         {
            //lockThreadMutex(in.transaction_thread());
            Mutex& m=mutex(in.transaction_thread());
            stm::lock(m);

            WriteContainer* c = write_lists(in.transaction_thread());
            WriteContainer::iterator readMem = c->find((base_transaction_object*)&in);
            if (readMem == c->end())
            {
               std::cout << "owner did not contain item in write list" << std::endl;
            }

#ifndef DISABLE_READ_SETS
            readList().insert((base_transaction_object*)readMem->second);
#endif
#if USE_BLOOM_FILTER
            bloom().insert(readMem->second);
#endif
            unlock(&transactionMutex_);
            unlock_tx();
            //unlockThreadMutex(in.transaction_thread());
            stm::unlock(m);

            ++reads_;

            // before leaving, we need to verify this object isn't bound
            // to subobjects
            if_bound_perform_subreads(readMem->second);

            return *static_cast<T*>(readMem->second);
         }

         // already have locked us above - in both if / else
#ifndef DISABLE_READ_SETS
         readList().insert((base_transaction_object*)&in);
#endif
#if USE_BLOOM_FILTER
         bloom().insert(&in);
#endif
         unlock(&transactionMutex_);
         unlock_tx();
         ++reads_;
         return in;
      }
      else
      {
         //--------------------------------------------------------------------
         // if we want direct write-read conflict to be done early, bail
         // if someone owns this
         //--------------------------------------------------------------------
         if (in.transaction_thread() != boost::stm::kInvalidThread)
         {
            // let the contention manager decide to throw or not
            cm_->abort_on_write(*this, (base_transaction_object&)(in));
         }

         lock_tx();
         // already have locked us above - in both if / else
#ifndef DISABLE_READ_SETS
         readList().insert((base_transaction_object*)&in);
#endif
#if USE_BLOOM_FILTER
         bloom().insert(&in);
#endif
         unlock_tx();
         ++reads_;
      }
      return in;
   }

   //--------------------------------------------------------------------------
   template <typename T>
   T& direct_write(T& in)
   {
      // if this is our memory (new or mod global) just return
      if (in.transaction_thread() == threadId_) return in;

      if (forced_to_abort())
      {
#ifndef DELAY_INVALIDATION_DOOMED_TXS_UNTIL_COMMIT
         cm_->abort_on_write(*this, static_cast<base_transaction_object&>(in));
#endif
      }
      //-----------------------------------------------------------------------
      // first we globally lock all threads before we poke at this global
      // memory - since we need to ensure other threads don't try to
      // manipulate this at the same time we are going to
      //-----------------------------------------------------------------------
      lock(&transactionMutex_);

      // we currently don't allow write stealing in direct update. if another
      // tx beat us to the memory, we abort
      if (in.transaction_thread() != boost::stm::kInvalidThread)
      {
         unlock(&transactionMutex_);
         throw aborted_tx("direct writer already exists.");
      }

      in.transaction_thread(threadId_);
      writeList().insert(tx_pair((base_transaction_object*)&in, new T(in)));
#if USE_BLOOM_FILTER
      bloom().insert(&in);
#endif
      unlock(&transactionMutex_);
      return in;
   }

   //--------------------------------------------------------------------------
   template <typename T>
   void direct_delete_memory(T &in)
   {
      if (in.transaction_thread() == threadId_)
      {
         //deletedMemoryList().push_back(&(T)in);
         deletedMemoryList().push_back((base_transaction_object*)&in);
         return;
      }

      //-----------------------------------------------------------------------
      // if we're here this item isn't in our writeList - get the global lock
      // and see if anyone else is writing to it. if not, we add the item to
      // our write list and our deletedList
      //-----------------------------------------------------------------------
      lock(&transactionMutex_);

      if (in.transaction_thread() != boost::stm::kInvalidThread)
      {
         unlock(&transactionMutex_);
         cm_->abort_on_write(*this, (base_transaction_object&)(in));
      }
      else
      {
         in.transaction_thread(threadId_);
         unlock(&transactionMutex_);
         // is this really necessary? in the deferred case it is, but in direct it
         // doesn't actually save any time for anything
         //writeList()[(base_transaction_object*)&in] = 0;

         deletedMemoryList().push_back((base_transaction_object*)&in);
      }
   }
#endif


   //--------------------------------------------------------------------------
   //                      DEFERRED UPDATING SECTION
   //--------------------------------------------------------------------------
   template <typename T>
   T const & deferred_read(T const & in)
   {
      if (forced_to_abort())
      {
         deferred_abort(true);
         throw aborted_tx("");
      }

      //----------------------------------------------------------------
      // always check if the writeList size is 0 first, since if it is
      // it can save construction of an iterator and search setup.
      //----------------------------------------------------------------
#if PERFORMING_WRITE_BLOOM
      if (writeList().empty() ||
         (writeList().size() > 16 &&
         !wbloom().exists(&in))) return insert_and_return_read_memory(in);
#else
      if (writeList().empty()) return insert_and_return_read_memory(in);
#endif

      WriteContainer::iterator i = writeList().find
         ((base_transaction_object*)(&in));
      //----------------------------------------------------------------
      // always check to see if read memory is in write list since it is
      // possible to have already written to memory being read now
      //----------------------------------------------------------------
      if (i == writeList().end()) return insert_and_return_read_memory(in);
      else return *static_cast<T*>(i->second);
   }

   //-------------------------------------------------------------------
   template <typename T>
   T& insert_and_return_read_memory(T& in)
   {
#ifndef DISABLE_READ_SETS
      ReadContainer::iterator i = readList().find
         (static_cast<base_transaction_object*>(&in));
      //----------------------------------------------------------------
      // if this object is already in our read list, bail
      //----------------------------------------------------------------
      if (i != readList().end()) return in;
#else
      if (bloom().exists(&in)) return in;
#endif
      lock_tx();
#ifndef DISABLE_READ_SETS
#if PERFORMING_VALIDATION
      readList()[(base_transaction_object*)&in] = in.version_;
#else
      readList().insert((base_transaction_object*)&in);
#endif
#endif
#if USE_BLOOM_FILTER
      bloom().insert(&in);
#endif
      unlock_tx();
      ++reads_;
      return in;
   }

   template <typename T>
   T& deferred_write(T& in)
   {
      //----------------------------------------------------------------------
      // The order of the three below events is very important. Do not change
      // them unless you have learned something new.
      //----------------------------------------------------------------------
      if (forced_to_abort())
      {
         deferred_abort(true);
         throw aborted_tx("");
      }

      //----------------------------------------------------------------------
      // (1) check to see if the memory is already in our write list. We
      //     perform this check first because it does not require the
      //     transaction's lock if the write element is already in our list.
      //     if it isn't, we'll need to lock the transaction.
      //----------------------------------------------------------------------
      WriteContainer::iterator i = writeList().find
         (static_cast<base_transaction_object*>(&in));

      if (i != writeList().end()) return *static_cast<T*>(i->second);

      //----------------------------------------------------------------------
      // (2) must lock thread to check if the write element is something we
      //     are already writing to. this is because another tx may be committing
      //     and updating this memory location. if that occurs, this tx might
      //     see the other tx's tm memory.
      //----------------------------------------------------------------------
      lock_tx();
      //----------------------------------------------------------------
      // if transactionThread_ is not invalid, then already writing to
      // non-global memory - so succeed.
      //----------------------------------------------------------------
      if (in.transaction_thread() != boost::stm::kInvalidThread)
      {
         unlock_tx();
         return in;
      }

      //----------------------------------------------------------------------
      // (3) the memory element is is not in our write list and it is not tx 
      //     memory, so we need to add it to our tx.
      //----------------------------------------------------------------------
#if USE_BLOOM_FILTER
      bloom().insert(&in);
#endif
#if PERFORMING_WRITE_BLOOM
      wbloom().set_bv1(bloom().h1());
      wbloom().set_bv2(bloom().h2());
      //sm_wbv().set_bit((size_t)&in % sm_wbv().size());
#endif
      base_transaction_object* returnValue = new T(in);
      returnValue->transaction_thread(threadId_);
      writeList().insert(tx_pair((base_transaction_object*)&in, returnValue));

      unlock_tx();

      return *static_cast<T*>(returnValue);
   }

   //--------------------------------------------------------------------------
   template <typename T>
   void deferred_delete_memory(T &in)
   {
      if (forced_to_abort())
      {
         deferred_abort(true);
         throw aborted_tx("");
      }
      //-----------------------------------------------------------------------
      // if this memory is true memory, not transactional, we add it to our
      // deleted list and we're done
      //-----------------------------------------------------------------------
      if (in.transaction_thread() != boost::stm::kInvalidThread)
      {
         lock_tx();
         bloom().insert(&in);
         unlock_tx();
         writeList().insert(tx_pair((base_transaction_object*)&in, (base_transaction_object*)0));
      }
      //-----------------------------------------------------------------------
      // this isn't real memory, it's transactional memory. But the good news is,
      // the real version has to be in our write list somewhere, find it, add
      // both items to the deletion list and exit
      //-----------------------------------------------------------------------
      else
      {
         lock_tx();
         bloom().insert(&in);
         unlock_tx();
         // check the ENTIRE write container for this piece of memory in the
         // second location. If it's there, it means we made a copy of a piece
         for (WriteContainer::iterator j = writeList().begin(); writeList().end() != j; ++j)
         {
            if (j->second == (base_transaction_object*)&in)
            {
               writeList().insert(tx_pair(j->first, (base_transaction_object*)0));
               deletedMemoryList().push_back(j->first);
            }
         }
      }

      deletedMemoryList().push_back((base_transaction_object *)&in);
   }

   //--------------------------------------------------------------------------
   void verifyReadMemoryIsValidWithGlobalMemory();
   void verifyWrittenMemoryIsValidWithGlobalMemory();

   //--------------------------------------------------------------------------
   inline void abort() throw() { direct_updating() ? direct_abort() : deferred_abort(); }
   inline void deferred_abort(bool const &alreadyRemovedFromInflightList = false) throw();
   inline void direct_abort(bool const &alreadyRemovedFromInflightList = false) throw();

   //--------------------------------------------------------------------------
   void validating_deferred_commit();
   void invalidating_deferred_commit();
   void validating_direct_commit();
   void invalidating_direct_commit();

   //--------------------------------------------------------------------------
   void lockThreadMutex(size_t threadId);
   void unlockThreadMutex(size_t threadId);
   static void lock_all_mutexes_but_this(size_t threadId);
   static void unlock_all_mutexes_but_this(size_t threadId);

   //--------------------------------------------------------------------------
   // side-effect: this unlocks all mutexes including its own. this is a slight
   // optimization over unlock_all_mutexes_but_this() as it doesn't have an
   // additional "if" to slow down performance. however, as it will be
   // releasing its own mutex, it must reset hasMutex_
   //--------------------------------------------------------------------------
   void lock_all_mutexes();
   void unlock_all_mutexes();


#ifndef DISABLE_READ_SETS
   inline bool isReading() const { return !readListRef_.empty(); }
#endif
   inline bool isWriting() 
   { return ! (write_list()->empty() && deletedMemoryList().empty() && newMemoryList().empty()) ; }

   inline bool is_only_reading() { return !isWriting(); }
   inline bool is_only_writing() { return isWriting() && reads_ == 0; }
   inline bool is_reading_and_writing() { return isWriting() && reads_ > 0; }

   // undefined and hidden - never allow these - bad things would happen
   transaction& operator=(transaction const &);
   transaction(transaction const &);

   bool otherInFlightTransactionsWritingThisMemory(base_transaction_object *obj);
   bool other_in_flight_same_thread_transactions() const throw();
   bool otherInFlightTransactionsOfSameThreadNotIncludingThis(transaction const * const);

   void removeAllSharedValuesOfThread(base_transaction_object *key);
   void copyOtherInFlightComposableTransactionalMemoryOfSameThread();
   bool setAllInFlightComposableTransactionsOfThisThreadToAbort();

   //--------------------------------------------------------------------------
   void forceOtherInFlightTransactionsWritingThisWriteMemoryToAbort();
   void forceOtherInFlightTransactionsReadingThisWriteMemoryToAbort();
   bool forceOtherInFlightTransactionsAccessingThisWriteMemoryToAbort(bool, transaction* &);

   void unlockAllLockedThreads(LockedTransactionContainer &);

   //--------------------------------------------------------------------------
   // direct and deferred transaction method for version / memory management
   //--------------------------------------------------------------------------
   void directCommitTransactionDeletedMemory() throw();
   size_t earliest_start_time_of_inflight_txes();
   void doIntervalDeletions();

   void deferredCommitTransactionDeletedMemory() throw();
   void directCommitTransactionNewMemory() { deferredCommitTransactionNewMemory(); }
   void deferredCommitTransactionNewMemory();

   void directAbortTransactionDeletedMemory() throw();
   void deferredAbortTransactionDeletedMemory() throw() { deletedMemoryList().clear(); }
   void directAbortTransactionNewMemory() throw() { deferredAbortTransactionNewMemory(); }
   void deferredAbortTransactionNewMemory() throw();

   void directCommitWriteState();
   void deferredCommitWriteState();

#ifndef DISABLE_READ_SETS
   void directCommitReadState() { readList().clear(); }
   void deferredCommitReadState() { readList().clear(); }
#endif

   void directAbortWriteList();
   void deferredAbortWriteList() throw();

#ifndef DISABLE_READ_SETS
   void directAbortReadList() { readList().clear(); }
   void deferredAbortReadList() throw() { readList().clear(); }
#endif

   void validating_direct_end_transaction();
   void invalidating_direct_end_transaction();

   void validating_deferred_end_transaction();
   void invalidating_deferred_end_transaction();

   //--------------------------------------------------------------------------
   //
   // Lock Aware Transactional Memory (LATM) support methods
   //
   //--------------------------------------------------------------------------

   //--------------------------------------------------------------------------
   // deferred updating methods
   //--------------------------------------------------------------------------
   static bool def_do_core_tm_conflicting_lock_pthread_lock_mutex
      (Mutex *mutex, int lockWaitTime, int lockAborted);
   static bool def_do_core_tx_conflicting_lock_pthread_lock_mutex
      (Mutex *mutex, int lockWaitTime, int lockAborted, bool txIsIrrevocable);
   static bool def_do_core_full_pthread_lock_mutex
      (Mutex *mutex, int lockWaitTime, int lockAborted);

   //--------------------------------------------------------------------------
   // direct updating methods
   //--------------------------------------------------------------------------
   static bool dir_do_core_tm_conflicting_lock_pthread_lock_mutex
      (Mutex *mutex, int lockWaitTime, int lockAborted);
   static bool dir_do_core_tx_conflicting_lock_pthread_lock_mutex
      (Mutex *mutex, int lockWaitTime, int lockAborted, bool txIsIrrevocable);
   static bool dir_do_core_full_pthread_lock_mutex
      (Mutex *mutex, int lockWaitTime, int lockAborted);

   static int thread_id_occurance_in_locked_locks_map(size_t threadId);

   static void wait_until_all_locks_are_released(bool);

   //--------------------------------------------------------------------------
   // deferred updating locking methods
   //--------------------------------------------------------------------------
   static int def_full_pthread_lock_mutex(Mutex *mutex);
   static int def_full_pthread_trylock_mutex(Mutex *mutex);
   static int def_full_pthread_unlock_mutex(Mutex *mutex);

   static int def_tm_conflicting_lock_pthread_lock_mutex(Mutex *mutex);
   static int def_tm_conflicting_lock_pthread_trylock_mutex(Mutex *mutex);
   static int def_tm_conflicting_lock_pthread_unlock_mutex(Mutex *mutex);

   static int def_tx_conflicting_lock_pthread_lock_mutex(Mutex *mutex);
   static int def_tx_conflicting_lock_pthread_trylock_mutex(Mutex *mutex);
   static int def_tx_conflicting_lock_pthread_unlock_mutex(Mutex *mutex);

   //--------------------------------------------------------------------------
   // direct updating locking methods
   //--------------------------------------------------------------------------
   static int dir_full_pthread_lock_mutex(Mutex *mutex);
   static int dir_full_pthread_trylock_mutex(Mutex *mutex);
   static int dir_full_pthread_unlock_mutex(Mutex *mutex);

   static int dir_tm_conflicting_lock_pthread_lock_mutex(Mutex *mutex);
   static int dir_tm_conflicting_lock_pthread_trylock_mutex(Mutex *mutex);
   static int dir_tm_conflicting_lock_pthread_unlock_mutex(Mutex *mutex);

   static int dir_tx_conflicting_lock_pthread_lock_mutex(Mutex *mutex);
   static int dir_tx_conflicting_lock_pthread_trylock_mutex(Mutex *mutex);
   static int dir_tx_conflicting_lock_pthread_unlock_mutex(Mutex *mutex);

   //--------------------------------------------------------------------------

   //--------------------------------------------------------------------------
   static DeletionBuffer deletionBuffer_;
   static std::ofstream logFile_;

   static MutexSet tmConflictingLocks_;
   static MutexSet latmLockedLocks_;
   static MutexThreadSetMap latmLockedLocksAndThreadIdsMap_;
   static MutexThreadMap latmLockedLocksOfThreadMap_;
   static ThreadSizetMap threadCommitMap_;
   static LatmType eLatmType_;
   static InflightTxes transactionsInFlight_;

   static Mutex deletionBufferMutex_;
   static Mutex transactionMutex_;
   static Mutex transactionsInFlightMutex_;
   static Mutex latmMutex_;
   static pthread_mutexattr_t transactionMutexAttribute_;

   static bool initialized_;
   static bool directLateWriteReadConflict_;
   static bool dynamicPriorityAssignment_;
   static bool usingMoveSemantics_;
   static transaction_bookkeeping bookkeeping_;
   static base_contention_manager *cm_;

   static MapOfTxObjects threadBoundObjects_;

#ifndef USE_SINGLE_THREAD_CONTEXT_MAP
    static ThreadWriteContainer threadWriteLists_;
    inline static WriteContainer* write_lists(thread_id_t id) {
        ThreadWriteContainer::iterator iter = threadWriteLists_.find(id);
        return iter->second;
    }
   static ThreadReadContainer threadReadLists_;
   static ThreadBloomFilterList threadBloomFilterLists_;
   static ThreadBloomFilterList threadWBloomFilterLists_;
   static ThreadMemoryContainerList threadNewMemoryLists_;
   static ThreadMemoryContainerList threadDeletedMemoryLists_;
   static ThreadTxTypeContainer threadTxTypeLists_;
   static ThreadBoolContainer threadForcedToAbortLists_;
#else
    static tss_context_map_type tss_context_map_;
    inline static WriteContainer* write_lists(thread_id_t thid) {
        tss_context_map_type::iterator iter = tss_context_map_.find(thid);
        return &(iter->second->writeMem);
    }
#endif

   //--------------------------------------------------------------------------
   static bool direct_updating_;
   static size_t global_clock_;
   inline static size_t& global_clock() {return global_clock_;}

   static size_t stalls_;

   //--------------------------------------------------------------------------
   // must be mutable because in cases where reads collide with other txs
   // modifying the same memory location in-flight, we add that memory
   // location ourselves to the modifiedList for us since performing a read
   // when another in-flight transaction is modifying the same location can
   // cause us to end up reading stale data if the other tx commits with
   // its newly modified data that we were reading.
   //
   // thus, we promote our read to a write and therefore need this map to
   // be mutable - see the read() interface for more documentation
   //--------------------------------------------------------------------------

   //--------------------------------------------------------------------------
   // ******** WARNING ******** MOVING threadId_ WILL BREAK TRANSACTION.
   // threadId_ MUST ALWAYS THE FIRST MEMBER OF THIS CLASS. THE MEMBER
   // INITIALIZATION IS ORDER DEPENDENT UPON threadId_!!
   // ******** WARNING ******** MOVING threadId_ WILL BREAK TRANSACTION
   //--------------------------------------------------------------------------
   size_t threadId_;

   //--------------------------------------------------------------------------
   // ******** WARNING ******** MOVING threadId_ WILL BREAK TRANSACTION.
   // threadId_ MUST ALWAYS THE FIRST MEMBER OF THIS CLASS. THE MEMBER
   // INITIALIZATION IS ORDER DEPENDENT UPON threadId_!!
   // ******** WARNING ******** MOVING threadId_ WILL BREAK TRANSACTION
   //--------------------------------------------------------------------------

#ifdef USE_SINGLE_THREAD_CONTEXT_MAP
    tx_context &context_;

#ifdef BOOST_STM_TX_CONTAINS_REFERENCES_TO_TSS_FIELDS
    mutable WriteContainer *write_list_ref_;
    mutable size_t *commits_ref_;
    inline WriteContainer *write_list() {return write_list_ref_;}
    inline const WriteContainer *write_list() const {return write_list_ref_;}

    mutable bloom_filter *bloomRef_;
#if PERFORMING_WRITE_BLOOM
    mutable bloom_filter *wbloomRef_;
    //mutable bit_vector &sm_wbv_;
#endif

    inline bloom_filter& bloom() { return *bloomRef_; }
#if PERFORMING_WRITE_BLOOM
    inline bloom_filter& wbloom() { return *wbloomRef_; }
    //bit_vector& sm_wbv() { return sm_wbv_; }
#endif
    MemoryContainerList *newMemoryListRef_;
    inline MemoryContainerList& newMemoryList() { return *newMemoryListRef_; }

    MemoryContainerList *deletedMemoryListRef_;
    inline MemoryContainerList& deletedMemoryList() { return *deletedMemoryListRef_; }

    TxType *txTypeRef_;
    inline TxType const tx_type() const { return *txTypeRef_; }
    inline void tx_type(TxType const &rhs) { *txTypeRef_ = rhs; }
    inline TxType&  tx_type_ref() { return *txTypeRef_; }
#else // BOOST_STM_TX_CONTAINS_REFERENCES_TO_TSS_FIELDS
    inline WriteContainer *write_list() {return &context_.writeMem;}
    inline const WriteContainer *write_list() const {return &context_.writeMem;}

    inline bloom_filter& bloom() { return context_.bloom; }
#if PERFORMING_WRITE_BLOOM
    inline bloom_filter& wbloom() { return context_.wbloom; }
    //bit_vector& sm_wbv() { return sm_wbv_; }
#endif
    inline MemoryContainerList& newMemoryList() { return context_.newMem; }
    inline MemoryContainerList& deletedMemoryList() { return context_.delMem; }
    inline TxType const tx_type() const { return context_.txType; }
    inline void tx_type(TxType const &rhs) { context_.txType = rhs; }
    inline TxType&  tx_type_ref() { return context_.txType; }
#endif

#ifdef USING_SHARED_FORCED_TO_ABORT
#ifdef BOOST_STM_TX_CONTAINS_REFERENCES_TO_TSS_FIELDS
    int *forcedToAbortRef_;
public:
    inline int const forced_to_abort() const { return *forcedToAbortRef_; }
private:
    inline int& forced_to_abort_ref() { return *forcedToAbortRef_; }
#else
public:
    inline int const forced_to_abort() const { return context_.abort; }
private:
    inline int& forced_to_abort_ref() { return context_.abort; }
#endif
#else
   int forcedToAbortRef_;
public:
    inline int const forced_to_abort() const { return forcedToAbortRef_; }
private:
    inline int& forced_to_abort_ref() { return forcedToAbortRef_; }
#endif

    static ThreadMutexContainer threadMutexes_;
    Mutex *mutexRef_;
    inline Mutex * mutex() { return mutexRef_; }
    inline static Mutex& mutex(thread_id_t id) {
        ThreadMutexContainer::iterator i = threadMutexes_.find(id);
        return *(i->second);
    }

   static ThreadBoolContainer threadBlockedLists_;
#if PERFORMING_LATM
   int &blockedRef_;
   inline void block() { blockedRef_ = true; }
   inline void unblock() { blockedRef_ = false; }
   inline int const blocked() const { return blockedRef_; }
   inline static int& blocked(thread_id_t id) { return *threadBlockedLists_.find(id)->second; }
#endif


#if PERFORMING_LATM
    static ThreadMutexSetContainer threadConflictingMutexes_;
    static ThreadMutexSetContainer threadObtainedLocks_;
    static ThreadMutexSetContainer threadCurrentlyLockedLocks_;

#if USING_TRANSACTION_SPECIFIC_LATM
    MutexSet &conflictingMutexRef_;
    inline MutexSet& get_tx_conflicting_locks() { return conflictingMutexRef_; }
    inline MutexSet& get_tx_conflicting_locks(thread_id_t id) {
       return *threadConflictingMutexes_.find(threadId_)->second;
    }
    static void thread_conflicting_mutexes_set_all(int b) {
        for (ThreadMutexSetContainer::iterator iter = threadConflictingMutexes_.begin();
            threadConflictingMutexes_.end() != iter; ++iter)
        {
            blocked(iter->first) = b;
        }
    }

    static void thread_conflicting_mutexes_set_all_cnd(Mutex *mutex, int b) {
        for (ThreadMutexSetContainer::iterator iter = threadConflictingMutexes_.begin();
            threadConflictingMutexes_.end() != iter; ++iter)
        {
        // if this mutex is found in the transaction's conflicting mutexes
        // list, then allow the thread to make forward progress again
        // by turning its "blocked" but only if it does not appear in the
        // locked_locks_thread_id_map
            if (iter->second->find(mutex) != iter->second->end() &&
                0 == thread_id_occurance_in_locked_locks_map(iter->first))
            {
                blocked(iter->first) = false;
            }
        }
   }
#endif

    MutexSet &obtainedLocksRef_;
    inline MutexSet &obtainedLocksRef() {return obtainedLocksRef_;}
    inline static MutexSet &obtainedLocksRef(thread_id_t id) {return *threadObtainedLocks_.find(id)->second;}

    void block_if_conflict_mutex() {
        //--------------------------------------------------------------------------
        // iterate through all currently locked locks
        //--------------------------------------------------------------------------
        for (ThreadMutexSetContainer::iterator i = threadObtainedLocks_.begin();
        threadObtainedLocks_.end() != i; ++i)
        {
            // if these are locks obtained by this thread (in a parent tx), don't block
            if (i->first == THREAD_ID) continue;

            for (MutexSet::iterator j = i->second->begin(); j != i->second->end(); ++j)
            {
                //-----------------------------------------------------------------------
                // iterate through this transaction's conflicting mutex ref - if one of
                // the obtained locked locks is in this tx's conflicting mutex set,
                // we need to block this tx
                //-----------------------------------------------------------------------
                if (get_tx_conflicting_locks().find(*j) != get_tx_conflicting_locks().end())
                {
                    this->block(); break;
                }
            }
        }
   }

   MutexSet &currentlyLockedLocksRef_;
   inline MutexSet &currentlyLockedLocksRef() {return currentlyLockedLocksRef_;}
   inline static MutexSet &currentlyLockedLocksRef(thread_id_t id) {return *threadCurrentlyLockedLocks_.find(id)->second;}
#endif

    static ThreadTransactionsStack threadTransactionsStack_;
    TransactionsStack& transactionsRef_;

#if CAPTURING_PROFILE_DATA
   unsigned int txTime();

   static ThreadOstringStream threadOstringStream_;
   std::ostringstream& ostrRef_;

   static ThreadMapTxFileAndNumber threadFileAndNumberMap_;
   std::map<TxFileAndNumber, size_t> &txFileAndNumberMap_;

#endif

   public:
    inline TransactionsStack& transactions() {return transactionsRef_;}
    inline static TransactionsStack &transactions(thread_id_t id, 
       bool const &alreadyHasGeneralLock = false) {

       if (!alreadyHasGeneralLock)
       {
         light_auto_lock auto_general_lock_(general_lock());
         return *threadTransactionsStack_.find(id)->second;
       }
       else return *threadTransactionsStack_.find(id)->second;
    }
    private:

////////////////////////////////////////
#else   // USE_SINGLE_THREAD_CONTEXT_MAP
////////////////////////////////////////

   mutable WriteContainer *write_list_ref_;
   inline WriteContainer *write_list() {return write_list_ref_;}
   inline const WriteContainer *write_list() const {return write_list_ref_;}
#ifndef DISABLE_READ_SETS
   mutable ReadContainer &readListRef_;
#endif
   mutable bloom_filter *bloomRef_;
#if PERFORMING_WRITE_BLOOM
   mutable bloom_filter *wbloomRef_;
   //mutable bit_vector &sm_wbv_;
#endif

   inline bloom_filter& bloom() { return *bloomRef_; }
#if PERFORMING_WRITE_BLOOM
   inline bloom_filter& wbloom() { return *wbloomRef_; }
   //bit_vector& sm_wbv() { return sm_wbv_; }
#endif
   MemoryContainerList *newMemoryListRef_;
   inline MemoryContainerList& newMemoryList() { return *newMemoryListRef_; }

   MemoryContainerList *deletedMemoryListRef_;
   inline MemoryContainerList& deletedMemoryList() { return *deletedMemoryListRef_; }

   TxType *txTypeRef_;
   inline TxType const tx_type() const { return *txTypeRef_; }
   inline void tx_type(TxType const &rhs) { *txTypeRef_ = rhs; }
   inline TxType&  tx_type_ref() { return *txTypeRef_; }

#ifdef USING_SHARED_FORCED_TO_ABORT
   int *forcedToAbortRef_;
public:
    inline int const forced_to_abort() const { return *forcedToAbortRef_; }
private:
    inline int& forced_to_abort_ref() { return *forcedToAbortRef_; }
#else
   int forcedToAbortRef_;
public:
    inline int const forced_to_abort() const { return forcedToAbortRef_; }
private:
    inline int& forced_to_abort_ref() { return forcedToAbortRef_; }
#endif

    static ThreadMutexContainer threadMutexes_;
    Mutex *mutexRef_;
    inline Mutex * mutex() { return mutexRef_; }
    inline static Mutex& mutex(thread_id_t id) {
        ThreadMutexContainer::iterator i = threadMutexes_.find(id);
        return *(i->second);
    }

   static ThreadBoolContainer threadBlockedLists_;
#if PERFORMING_LATM
   int &blockedRef_;
   inline void block() { blockedRef_ = true; }
   inline void unblock() { blockedRef_ = false; }
   inline int const blocked() const { return blockedRef_; }
   inline static int& blocked(thread_id_t id) { return *threadBlockedLists_.find(id)->second; }
#endif


#if PERFORMING_LATM
    static ThreadMutexSetContainer threadConflictingMutexes_;
    static ThreadMutexSetContainer threadObtainedLocks_;
    static ThreadMutexSetContainer threadCurrentlyLockedLocks_;

#if USING_TRANSACTION_SPECIFIC_LATM
    MutexSet &conflictingMutexRef_;
    inline MutexSet& get_tx_conflicting_locks() { return conflictingMutexRef_; }
    inline MutexSet& get_tx_conflicting_locks(thread_id_t id) {
       return *threadConflictingMutexes_.find(threadId_)->second;
    }
    static void thread_conflicting_mutexes_set_all(int b) {
        for (ThreadMutexSetContainer::iterator iter = threadConflictingMutexes_.begin();
            threadConflictingMutexes_.end() != iter; ++iter)
        {
            blocked(iter->first) = b;
        }
    }

    static void thread_conflicting_mutexes_set_all_cnd(Mutex *mutex, int b) {
        for (ThreadMutexSetContainer::iterator iter = threadConflictingMutexes_.begin();
            threadConflictingMutexes_.end() != iter; ++iter)
        {
        // if this mutex is found in the transaction's conflicting mutexes
        // list, then allow the thread to make forward progress again
        // by turning its "blocked" but only if it does not appear in the
        // locked_locks_thread_id_map
            if (iter->second->find(mutex) != iter->second->end() &&
                0 == thread_id_occurance_in_locked_locks_map(iter->first))
            {
                blocked(iter->first) = false;
            }
        }
   }
#endif

    MutexSet &obtainedLocksRef_;
    inline MutexSet &obtainedLocksRef() {return obtainedLocksRef_;}
    inline static MutexSet &obtainedLocksRef(thread_id_t id) {return *threadObtainedLocks_.find(id)->second;}

    void block_if_conflict_mutex() {
        //--------------------------------------------------------------------------
        // iterate through all currently locked locks
        //--------------------------------------------------------------------------
        for (ThreadMutexSetContainer::iterator i = threadObtainedLocks_.begin();
        threadObtainedLocks_.end() != i; ++i)
        {
            // if these are locks obtained by this thread (in a parent tx), don't block
            if (i->first == THREAD_ID) continue;

            for (MutexSet::iterator j = i->second->begin(); j != i->second->end(); ++j)
            {
                //-----------------------------------------------------------------------
                // iterate through this transaction's conflicting mutex ref - if one of
                // the obtained locked locks is in this tx's conflicting mutex set,
                // we need to block this tx
                //-----------------------------------------------------------------------
                if (get_tx_conflicting_locks().find(*j) != get_tx_conflicting_locks().end())
                {
                    this->block(); break;
                }
            }
        }
   }



   MutexSet &currentlyLockedLocksRef_;
   inline MutexSet &currentlyLockedLocksRef() {return currentlyLockedLocksRef_;}
   inline static MutexSet &currentlyLockedLocksRef(thread_id_t id) {return *threadCurrentlyLockedLocks_.find(id)->second;}
#endif

    static ThreadTransactionsStack threadTransactionsStack_;
    TransactionsStack& transactionsRef_;
   public:
    inline TransactionsStack& transactions() {return transactionsRef_;}
    inline static TransactionsStack &transactions(thread_id_t id) {
        light_auto_lock auto_general_lock_(general_lock());
        return *threadTransactionsStack_.find(id)->second;
    }
    private:

////////////////////////////////////////
#endif



   // transaction specific data
   int hasMutex_;
   mutable size_t priority_;
   transaction_state state_;
   size_t reads_;
   mutable size_t startTime_;

   inline transaction_state const & state() const { return state_; }

   inline WriteContainer& writeList() { return *write_list(); }
#ifndef DISABLE_READ_SETS
   inline ReadContainer& readList() { return readListRef_; }
#endif



public:
    inline static transaction* current_transaction() {return transactions(THREAD_ID).top();}


};

inline transaction* current_transaction() {
    return transaction::current_transaction();
}

#if 0 // these are not portable for Visual Studio 6
template <typename MUTEX, MUTEX& mtx>
locker<MUTEX,mtx>::locker() {
    boost::stm::lock(mtx);
}
template <typename MUTEX, MUTEX& mtx>
locker<MUTEX,mtx>::~locker() {}

template <typename MUTEX, MUTEX& mtx>
void locker<MUTEX,mtx>::unlock() {
    boost::stm::unlock(mtx);
}
#endif


//-----------------------------------------------------------------------------
// scoped thread initializer calling transaction::initialize_thread() in the
// constructor and transaction::terminate_thread() in the destructor
//-----------------------------------------------------------------------------
struct thread_initializer {
    thread_initializer() {transaction::initialize_thread();}
    ~thread_initializer() {transaction::terminate_thread();}
};



//---------------------------------------------------------------------------
// do not remove if (). It is necessary a necessary fix for compilers
// that do not destroy index variables of for loops. In addition, the
// rand()+1 check is necessarily complex so smart compilers can't
// optimize the if away
//---------------------------------------------------------------------------
#ifdef BOOST_STM_COMPILER_DONT_DESTROY_FOR_VARIABLES
#define use_atomic(T) if (0==rnd()+1) {} else for (boost::stm::transaction T; !T.committed() && T.restart(); T.end())
#define try_atomic(T) if (0==rnd()+1) {} else for (boost::stm::transaction T; !T.committed() && T.restart(); T.no_throw_end()) try
#define atomic(T)     if (0==rnd()+1) {} else for (boost::stm::transaction T; !T.committed() && T.check_throw_before_restart() && T.restart_if_not_inflight(); T.no_throw_end()) try
#else
#define use_atomic(T) for (boost::stm::transaction T; !T.committed() && T.restart(); T.end())
#define try_atomic(T) for (boost::stm::transaction T; !T.committed() && T.restart(); T.no_throw_end()) try
#define atomic(T)     for (boost::stm::transaction T; !T.committed() && T.check_throw_before_restart() && T.restart_if_not_inflight(); T.no_throw_end()) try
#endif


#define catch_before_retry(E) catch (boost::stm::aborted_tx &E)
#define before_retry catch (boost::stm::aborted_tx &)
#define end_atom catch (boost::stm::aborted_tx &) {}

#define BOOST_STM_NEW(T, P) \
    ((T).throw_if_forced_to_abort_on_new(), \
    (T).as_new(new P))

#define BOOST_STM_NEW_1(P) \
    ((boost::stm::current_transaction()!=0)?BOOST_STM_NEW(*boost::stm::current_transaction(), P):new P)


} // stm namespace
} // boost namespace

#include <boost/stm/detail/transaction_impl.hpp>
#include <boost/stm/detail/latm_general_impl.hpp>
#include <boost/stm/detail/auto_lock.hpp>
#include <boost/stm/detail/tx_ptr.hpp>

///////////////////////////////////////////////////////////////////////////////
#endif // BOOST_STM_TRANSACTION__HPP


