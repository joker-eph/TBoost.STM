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

#ifndef MAIN_H
#define MAIN_H

#include <boost/stm/transaction.hpp>

namespace nMain
{
   int const kStartingTime = -1;
}

enum eWorkType
{
   kBalancedWork,
   kTxIntenseWork,
   kLockIntenseWork
};

extern eWorkType gWorkLoadType;

extern int kMaxInserts;
extern bool kInsertSameValues;
extern bool kDoRemoval;
extern bool kDoLookup;
extern bool kDoMove;
extern bool kMoveSemantics;
extern std::string bench;

extern int kMaxThreads;
extern int kMainThreadId;
extern int kMaxArrSize;
extern int kMaxArrIter;

////////////////////////////////////////////////////////////////////////////
//
// global variables all of which are used in the main transaction iteration
// 
////////////////////////////////////////////////////////////////////////////
class String;
class Integer;

extern Mutex outputMutex;
extern String globalString;

extern boost::stm::native_trans<int> threadsFinished;
extern boost::stm::native_trans<int> threadsStarted;
extern time_t startTimer;
extern time_t endTimer;

void idleUntilAllThreadsHaveReached(int const &threadId);
void finishThread(int const &threadId);
void finishThread();
void logCommitsAndAborts(std::string const &typeOfRun);

#endif // MAIN_H
