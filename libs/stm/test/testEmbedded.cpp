
//#include <boost/thread.hpp>
//#include <boost/random.hpp>
#include <boost/stm.hpp>
#include <cstdlib>
#include <ctime>
#include "testEmbedded.h"
#include "main.h"

namespace stm = boost::stm;

using namespace stm;

class C1 : public transaction_object<C1>
{
public:
   int i;
};

class C2 : public transaction_object<C2>
{
public:
   int i;
};

class E : public transaction_object<E>
{
public:
   E() { c1.i = -1; c2.i = -1; i = -1; }

   int i;
   C1 c1;
   C2 c2;
};

E e;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void* embedded1(void* threadId)
{
   stm::transaction::initialize_thread();

   atomic(t)
   {
      t.w(e).i = 0;

      //------------------------------------------------------
      // lines of code that would be generated by:
      // boost::stm::bind(this, c1);
      // boost::stm::bind(this, c2);
      //------------------------------------------------------

      t.w(e.c1);
      t.w(e.c2); 

      atomic(t)
      {

         t.w(e.c2).i = 5;
      } end_atom
   } end_atom

   return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void* embedded2(void* threadId)
{
   stm::transaction::initialize_thread();

   atomic(t)
   {
      t.w(e.c2).i = 5;

      atomic(t)
      {

         t.w(e).i = 0;

         //------------------------------------------------------
         // lines of code that would be generated by:
         // boost::stm::bind(this, c1);
         // boost::stm::bind(this, c2);
         //------------------------------------------------------

         t.w(e.c1);
         t.w(e.c2);

      } end_atom
   } end_atom

   return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int testEmbedded()
{
   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "Processing embedded1: " << std::endl << std::endl;
   std::cout << "Initial value of    e.i: " << e.i << endl;
   std::cout << "Initial value of e.c1.i: " << e.c1.i << endl;
   std::cout << "Initial value of e.c2.i: " << e.c2.i << endl;

   stm::transaction::initialize();
   stm::transaction::initialize_thread();
   srand((unsigned int)(time(NULL)));

   int mainThreadId = kMaxThreads-1;

   embedded1((void*)&mainThreadId);

   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "Ending processing." << std::endl;
   std::cout << "Ending value of    e.i: " << e.i << endl;
   std::cout << "Ending value of e.c1.i: " << e.c1.i << endl;
   std::cout << "Ending value of e.c2.i: " << e.c2.i << endl;

   
   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "Resetting e" << std::endl;
   std::cout << "-----------------------------------------------" << std::endl;

   e.i = -1;
   e.c1.i = -1;
   e.c2.i = -1;

   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "Processing embedded2: " << std::endl << std::endl;
   std::cout << "Initial value of    e.i: " << e.i << endl;
   std::cout << "Initial value of e.c1.i: " << e.c1.i << endl;
   std::cout << "Initial value of e.c2.i: " << e.c2.i << endl;

   embedded1((void*)&mainThreadId);

   std::cout << "-----------------------------------------------" << std::endl;
   std::cout << "Ending processing." << std::endl;
   std::cout << "Ending value of    e.i: " << e.i << endl;
   std::cout << "Ending value of e.c1.i: " << e.c1.i << endl;
   std::cout << "Ending value of e.c2.i: " << e.c2.i << endl;


   return 0;
}

