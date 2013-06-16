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

#ifndef BOOST_STM_TRANSACTION_BOOKKEEPING_H
#define BOOST_STM_TRANSACTION_BOOKKEEPING_H

#include <iostream>
#include <vector>
#include <map>
#include <pthread.h>
#include <boost/stm/detail/datatypes.hpp>

#if 0 //TBR
#ifdef WINOS
#pragma warning (disable:4786)
#define SLEEP(x) Sleep(x)
#define THREAD_ID (size_t)pthread_self().p
#else
#include <unistd.h>
#define SLEEP(x) usleep(x*1000)
#define THREAD_ID (size_t) pthread_self()
#endif
#endif

//-----------------------------------------------------------------------------
class ThreadIdAndCommitId
{
public:

   ThreadIdAndCommitId(uint32 const &threadId, uint32 const &commitId) :
      threadId_(threadId), commitId_(commitId) {}

   uint32 threadId_;
   uint32 commitId_;

   bool operator==(ThreadIdAndCommitId const &rhs) const 
   { return threadId_ == rhs.threadId_ && commitId_ == rhs.commitId_; }
   
   bool operator<(ThreadIdAndCommitId const &rhs) const 
   {
      if (threadId_ < rhs.threadId_) return true;
      if (threadId_ == rhs.threadId_)
      {
         if (commitId_ < rhs.commitId_) return true;
      }
      return false;
   }
};

//-----------------------------------------------------------------------------
class transaction_bookkeeping
{
public:

   typedef std::map<uint32, uint32> thread_commit_map;
   typedef std::map<ThreadIdAndCommitId, uint32> CommitHistory;
   typedef std::map<ThreadIdAndCommitId, uint32> AbortHistory;

   transaction_bookkeeping() : aborts_(0), writeAborts_(0), readAborts_(0), 
      abortPermDenied_(0), commits_(0), handOffs_(0), newMemoryCommits_(0), 
      newMemoryAborts_(0), deletedMemoryCommits_(0), deletedMemoryAborts_(0),
      readStayedAsRead_(0), readChangedToWrite_(0), commitTimeMs_(0), lockConvoyMs_(0)
   {
      //abortTrackingMutex_ = PTHREAD_MUTEX_INITIALIZER;
   }

   uint32 const & lockConvoyMs() const { return lockConvoyMs_; }
   uint32 const & commitTimeMs() const { return commitTimeMs_; }
   uint32 const & readAborts() const { return readAborts_; }
   uint32 const & writeAborts() const { return writeAborts_; }
   uint32 const & abortPermDenied() const { return abortPermDenied_; }
   uint32 const totalAborts() const { return readAborts_ + writeAborts_ + abortPermDenied_; }
   uint32 const & commits() const { return commits_; }
   uint32 const & handOffs() const { return handOffs_; }
   uint32 const & newMemoryAborts() const { return newMemoryAborts_; }
   uint32 const & newMemoryCommits() const { return newMemoryCommits_; }
   uint32 const & deletedMemoryAborts() const { return deletedMemoryAborts_; }
   uint32 const & deletedMemoryCommits() const { return deletedMemoryCommits_; }
   uint32 const & readChangedToWrite() const { return readChangedToWrite_; }
   uint32 const & readStayedAsRead() const { return readStayedAsRead_; }

   void inc_read_aborts() { ++readAborts_; }
   void inc_write_aborts() { ++writeAborts_; }

   void inc_thread_commits(uint32 threadId) 
   {
#if 0
      std::map<uint32, uint32>::iterator i = threadedCommits_.find(threadId);

      if (threadedCommits_.end() == i) threadedCommits_[threadId] = 1;
      else i->second = i->second + 1;
#endif
   }

   void inc_thread_aborts(uint32 threadId) 
   {
#if 0
      std::map<uint32, uint32>::iterator i = threadedAborts_.find(threadId);

      if (threadedAborts_.end() == i)
      {
         threadedAborts_.insert(std::make_pair(threadId, 1));
      }
      else
      {
         i->second += 1;
      }
#endif
   }

   thread_commit_map const & threadedCommits() const { return threadedCommits_; }
   thread_commit_map const & threadedAborts() const { return threadedAborts_; }

   void inc_lock_convoy_ms(uint32 const &rhs) { lockConvoyMs_ += rhs; }
   void inc_commit_time_ms(uint32 const &rhs) { commitTimeMs_ += rhs; }
   void inc_commits() { ++commits_; inc_thread_commits(THREAD_ID); }
   void inc_abort_perm_denied(uint32 const &threadId) { ++abortPermDenied_; inc_thread_aborts(threadId); }
   void inc_handoffs() { ++handOffs_; }
   void inc_new_mem_aborts_by(uint32 const &rhs) { newMemoryAborts_ += rhs; }
   void inc_new_mem_commits_by(uint32 const &rhs) { newMemoryCommits_ += rhs; }
   void inc_del_mem_aborts_by(uint32 const &rhs) { deletedMemoryAborts_ += rhs; }
   void inc_del_mem_commits_by(uint32 const &rhs) { deletedMemoryCommits_ += rhs; }
   void incrementReadChangedToWrite() { ++readChangedToWrite_; }
   void incrementReadStayedAsRead() { ++readStayedAsRead_; }

   CommitHistory const& getCommitReadSetList() const { return committedReadSetSize_; }
   CommitHistory const& getCommitWriteSetList() const { return committedWriteSetSize_; }
   AbortHistory const& getAbortReadSetList() const { return abortedReadSetSize_; }
   AbortHistory const& getAbortWriteSetList() const { return abortedWriteSetSize_; }

   void pushBackSizeOfReadSetWhenAborting(uint32 const &size) 
   { 
      //lock(&abortTrackingMutex_);

      ThreadIdAndCommitId tcId(THREAD_ID, ++aborts_);

      // if waiting for commit read from thread is already true, it means there
      // was no commit on the last abort, so drop it from the map

      if (waitingForCommitReadFromThread[THREAD_ID])
      {
         abortedReadSetSize_.erase(ThreadIdAndCommitId(THREAD_ID, aborts_-1));
         abortedWriteSetSize_.erase(ThreadIdAndCommitId(THREAD_ID, aborts_-1));
      }

      abortedReadSetSize_[tcId] = size; 
      waitingForCommitReadFromThread[THREAD_ID] = true;
      //unlock(&abortTrackingMutex_);
   }

   void pushBackSizeOfWriteSetWhenAborting(uint32 const &size) 
   { 
      //lock(&abortTrackingMutex_);
      ThreadIdAndCommitId tcId(THREAD_ID, aborts_);
      abortedWriteSetSize_[tcId] = size; 
      waitingForCommitWriteFromThread[THREAD_ID] = true;
      //unlock(&abortTrackingMutex_);
   }

   void pushBackSizeOfReadSetWhenCommitting(uint32 const &size) 
   { 
      //lock(&abortTrackingMutex_);
      ThreadIdAndCommitId tcId(THREAD_ID, aborts_);

      // only insert this commit if an abort made an entry at this commit point
      if (waitingForCommitReadFromThread[THREAD_ID])
      {
         committedReadSetSize_[tcId] = size;
         waitingForCommitReadFromThread[THREAD_ID] = false;
      }
      //unlock(&abortTrackingMutex_);
   }

   void pushBackSizeOfWriteSetWhenCommitting(uint32 const &size) 
   { 
      //lock(&abortTrackingMutex_);
      ThreadIdAndCommitId tcId(THREAD_ID, aborts_);

      // only insert this commit if an abort made an entry at this commit point
      if (waitingForCommitWriteFromThread[THREAD_ID])
      {
         committedWriteSetSize_[tcId] = size;
         waitingForCommitWriteFromThread[THREAD_ID] = false;
      }
      //unlock(&abortTrackingMutex_);
   }

   bool isLoggingAbortAndCommitSize() const { return isLoggingAbortAndCommitSize_; }
   void setIsLoggingAbortAndCommitSize(bool const &in) { isLoggingAbortAndCommitSize_ = in; }

   ////////////////////////////////////////////////////////////////////////////
   friend std::ostream& operator<<(std::ostream& out, transaction_bookkeeping const &that)
   {
      using namespace std;

      out << "########################################" << endl;

      for (thread_commit_map::const_iterator i = that.threadedCommits().begin(); 
         i != that.threadedCommits().end(); ++i)
      {
         out << " thread [" << i->first << "]:  commits: " << i->second << "  aborts: ";

         bool found = false;

         for (thread_commit_map::const_iterator j = that.threadedAborts().begin(); 
            j != that.threadedAborts().end(); ++j)
         {
            if (j->first == i->first)
            {
               out << j->second;
               found = true;
               break;
            }
         }

         if (!found) out << 0;

         out << endl;
      }

      return out;
   }

private:

   bool isLoggingAbortAndCommitSize_;

   AbortHistory abortedReadSetSize_;
   AbortHistory abortedWriteSetSize_;
   CommitHistory committedReadSetSize_;
   CommitHistory committedWriteSetSize_;

   std::map<uint32, bool> waitingForCommitReadFromThread;
   std::map<uint32, bool> waitingForCommitWriteFromThread;

   thread_commit_map threadedCommits_;
   thread_commit_map threadedAborts_;

   uint32 aborts_;
   uint32 writeAborts_;
   uint32 readAborts_;
   uint32 abortPermDenied_;
   uint32 commits_;
   uint32 handOffs_;
   uint32 newMemoryCommits_;
   uint32 newMemoryAborts_;
   uint32 deletedMemoryCommits_;
   uint32 deletedMemoryAborts_;
   uint32 readStayedAsRead_;
   uint32 readChangedToWrite_;
   uint32 commitTimeMs_;
   uint32 lockConvoyMs_;

   //Mutex abortTrackingMutex_;
};


#endif // TRANSACTION_BOOKKEEPING_H

