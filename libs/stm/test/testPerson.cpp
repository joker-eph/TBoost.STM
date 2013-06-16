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

//---------------------------------------------------------------------------
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
#include "transferFun.h"
#include "main.h"
#include "testRBTree.h"
#include <math.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include "testPerson.h"

//---------------------------------------------------------------------------
using namespace std; using namespace boost::stm;

//---------------------------------------------------------------------------

named_array<int> intArr;
named_array<string> strArr;

//---------------------------------------------------------------------------

void norm_fun()
{
  int const max = 100; int i;

  intArr.set_name("int array");
  strArr.set_name("str array");

  intArr.resize(max);
  strArr.resize(max);

  for (i = 0; i < max; ++i)
  {
    intArr.array()[i] = i;

    ostringstream o;
    o << i;

    strArr.array()[i] = o.str();
  }

  for (i = 0; i < max; ++i)
  {
    cout << intArr.array()[i] << endl;
    cout << strArr.array()[i] << endl;
  }
}

//---------------------------------------------------------------------------
void trans_fun2()
{
  transaction::initialize_thread();

  if (kMoveSemantics) transaction::enable_move_semantics();

  int const max = kMaxArrSize; int i;

  for (transaction t;;)
  {
    try
    {
      t.w(intArr).set_name("int array");
      t.w(intArr).resize(max);
      t.end();
      break;
 
    } catch (aborted_transaction_exception&) {}
  }

  for (i = 0; i < max; ++i)
  {
    for (transaction t;;)
    {
      try
      {
        t.w(intArr).array()[i] = i;
        t.end();
        break;
      } catch (aborted_transaction_exception&) {}
    }
  }
}

//---------------------------------------------------------------------------
void trans_fun()
{
  transaction::initialize_thread();

  if (kMoveSemantics) transaction::enable_move_semantics();

  int const max = kMaxArrSize; int i;

  for (transaction t;;)
  {
    try
    {
      t.w(intArr).set_name("int array");
      t.w(strArr).set_name("str array");
  
      t.w(intArr).resize(max);
      t.w(strArr).resize(max);

      for (i = 0; i < max; ++i)
      {
        t.w(intArr).array()[i] = i;
        ostringstream o;
        o << i;

        t.w(strArr).array()[i] = o.str();
      }

      t.end();
      break;
    } catch (aborted_transaction_exception&) {}
  }
}

//---------------------------------------------------------------------------
void TestPerson()
{
   //------------------------------------------------------------------------
   // this will prevent any thread 2 from causing the idle until all threads
   // have reached from breaking
   //------------------------------------------------------------------------
   kMaxThreads = 2;

   transaction::initialize();
   transaction::initialize_thread();

   //trans_fun();
   trans_fun2();

   cout << "(move semantics, array size, commits, total aborts)\t";
   cout << transaction::using_move_semantics() << "\t";
   cout << kMaxArrSize << "\t";
   cout << transaction::bookkeeping().commits() << "\t";
   cout << transaction::bookkeeping().totalAborts() << endl;

   exit(0);
}

