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

#ifndef BOOST_STM_dataTypes_header_file
#define BOOST_STM_dataTypes_header_file

 
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#define WINOS
#elif WIN64
#define WINOS
#else
#define UNIX
#endif

#include <stdlib.h>
#include <pthread.h>

#ifndef BOOST_STM_USE_BOOST_THREAD_ID
#ifdef WINOS
#pragma warning (disable:4786)
#define THREAD_ID (size_t)pthread_self().p
#else
#define THREAD_ID (size_t) pthread_self()
#endif
#else

#define THREAD_ID boost::this_thread::get_id()
#endif


#ifndef BOOST_STM_USE_BOOST_SLEEP
#ifdef WINOS
#define SLEEP(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP(x) usleep(x*1000)
#endif
#else
#define SLEEP(x) boost::this_thread::sleep(boost::posix_time::milliseconds(x))
#endif

/////////////////////////////////////////////////////////////////////////////
// types
/////////////////////////////////////////////////////////////////////////////
typedef bool bool8;
typedef char char8;

typedef unsigned char uchar8;

typedef char int8;
typedef unsigned char uint8;

typedef short int int16;
typedef unsigned short int uint16;

typedef long int int32;
typedef unsigned long int uint32;

#endif // dataTypes_header_file
