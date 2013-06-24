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

///////////////////////////////////////////////////////////////////////////////
#ifndef TEST_LINKED_LIST_H
#define TEST_LINKED_LIST_H

#include "main.h"
#include <boost/stm/transaction.hpp>

#include <fstream>

///////////////////////////////////////////////////////////////////////////////
template <typename T>
class list_node : public boost::stm::transaction_object< list_node<T> >
{
public:

   list_node() : value_(0), next_(0) {
       }
   explicit list_node(T const &rhs) : value_(rhs), next_(NULL) {
       }

   // zero initialization for native types
   void clear() { value_ = T(); next_ = NULL; }

   T &value() { return value_; }
   T const &value() const { return value_; }

   list_node const *next() const { return next_; }

   void next(list_node const *rhs, boost::stm::transaction &t)
   {
      if (NULL == rhs) next_ = NULL;
      else next_ = &t.find_original(*(list_node<T>*)rhs);
   }

   void next_for_new_mem(list_node const *rhs, boost::stm::transaction &t)
   {
      if (NULL == rhs) next_ = NULL;
      else next_ = &t.find_original(*(list_node<T>*)rhs);
   }

#if BUILD_MOVE_SEMANTICS
   list_node& operator=(list_node const & rhs)
   {
      value_ = rhs.value_;
      next_ = rhs.next_;
      return *this;
   }

   list_node(list_node &&rhs) : next_(rhs.next_), value_(boost::stm::draco_move(rhs.value_))
   { rhs.next_ = 0; }

   list_node& operator=(list_node&& rhs)
   {
      value_ = boost::stm::draco_move(rhs.value_);
      std::swap(next_, rhs.next_);
      return *this;
   }
#endif

private:

   T value_;
   list_node *next_;
};

////////////////////////////////////////////////////////////////////////////
template <typename T>
class LinkedList
{
public:

   LinkedList() { head_.value() = T(); }

   ~LinkedList() { quick_clear(); }

   bool move(list_node<T> const &node1, list_node<T> const &node2)
   {
      using namespace boost::stm;
      bool succeeded1 = true, succeeded2 = true;
      transaction_state state = e_no_state;

      do
      {
         try
         {
            transaction t;
            succeeded1 = internal_remove(node1);
            succeeded2 = internal_insert(node2);
            t.end();
         }
         catch (aborted_transaction_exception&) {}

         if (!succeeded1 || !succeeded2)
         {
            return false; // auto abort of t
         }

      } while (e_committed != state);

      return true;
   }

   ////////////////////////////////////////////////////////////////////////////
   bool insert(list_node<T> const &node)
   {
      using namespace boost::stm;

      for (transaction t; ;t.restart())
      {
         try { return internal_insert(node, t); }
         catch (aborted_transaction_exception&) {}
      }
   }
#ifdef BOOST_STM_USES_PARAM
   bool insert(T val)
#else
   bool insert(const T& val)
#endif
   {
      using namespace boost::stm;

      for (transaction t; ;t.restart())
      {
         try { return internal_insert(val, t); }
         catch (aborted_transaction_exception&) {}
      }
   }
   ////////////////////////////////////////////////////////////////////////////
#ifdef BOOST_STM_USES_PARAM
   bool lookup(T val)
#else
   bool lookup(T const &val)
#endif
   {
      using namespace boost::stm;

      for (transaction t; ; t.restart())
      {
         try { return internal_lookup(val, t); }
         catch (aborted_transaction_exception&) {}
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   bool remove(list_node<T> const &node)
   {
      using namespace boost::stm;

      for (transaction t; ; t.restart())
      {
         try { return internal_remove(node, t); }
         catch (aborted_transaction_exception&) {}
      }
   }

#ifdef BOOST_STM_USES_PARAM
   bool remove(T val)
#else
   bool remove(T const &val)
#endif
   {
      using namespace boost::stm;

      for (transaction t; ; t.restart())
      {
         try { return internal_remove(val, t); }
         catch (aborted_transaction_exception&) {}
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   void outputList(std::ofstream &o)
   {
      int i = 0;
      for (list_node<T> const *cur = head_.next(); cur != NULL; cur = cur->next())
      {
         o << "element [" << i++ << "]: " << cur->value() << std::endl;
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   int walk_size()
   {
      int i = 0;
      for (list_node<T> const *cur = head_.next(); cur != NULL; cur = cur->next())
      {
         ++i;
      }

      return i;
   }

   ////////////////////////////////////////////////////////////////////////////
   void quick_clear()
   {
      for (list_node<T> const *cur = head_.next(); cur != NULL;)
      {
         list_node<T> const *prev = cur;
         cur = cur->next();
         delete prev;
      }

      head_.clear();
   }

   ////////////////////////////////////////////////////////////////////////////
   void clear()
   {
      boost::stm::transaction t;

      for (list_node<T> const *cur = t.read(head_).next(); cur != NULL;)
      {
         list_node<T> const *prev = &t.read(*cur);
         cur = t.read(*cur).next();
         t.delete_memory(*prev);
      }

      t.write(head_).clear();

      t.end();
   }

private:

   //--------------------------------------------------------------------------
   // find the location to insert the node. if the value already exists, bail
   //--------------------------------------------------------------------------
   bool internal_insert(list_node<T> const &rhs, boost::stm::transaction &t)
   {
      list_node<T> const *headP = &t.read(head_);

      if (NULL != headP->next())
      {
         list_node<T> const *prev = headP;
         list_node<T> const *cur = t.read_ptr(headP->next());
         T val = rhs.value();

         while (true)
         {
            if (cur->value() == val) return false;
            else if (cur->value() > val || !cur->next()) break;

            prev = cur;

            list_node<T> const *curNext = t.read_ptr(cur->next());

            if (NULL == curNext) break;

            cur = curNext;
         }

         list_node<T> *newNode = t.new_memory_copy(rhs);

         //--------------------------------------------------------------------
         // if cur->next() is null it means our newNode value is greater than
         // cur, so insert ourselves after cur.
         //--------------------------------------------------------------------
         if (val > cur->value()) t.write_ptr((list_node<T>*)cur)->next_for_new_mem(newNode, t);
         //--------------------------------------------------------------------
         // otherwise, we are smaller than cur, so insert between prev and cur
         //--------------------------------------------------------------------
         else
         {
            newNode->next(cur, t);
            t.write_ptr((list_node<T>*)prev)->next_for_new_mem(newNode, t);
         }
      }
      else
      {
         list_node<T> *newNode = t.new_memory_copy(rhs);
         t.write(head_).next_for_new_mem(newNode, t);
      }

      t.end();
      return true;
   }

#ifdef BOOST_STM_USES_PARAM
   bool internal_insert(T val, boost::stm::transaction &t)
#else
   bool internal_insert(const T& val, boost::stm::transaction &t)
#endif
   {
      //T val = valr;
      list_node<T> const *headP = &t.read(head_);

      if (NULL != headP->next())
      {
         list_node<T> const *prev = headP;
         list_node<T> const *cur = t.read_ptr(headP->next());

         while (true)
         {
            if (cur->value() == val) return false;
            else if (cur->value() > val || !cur->next()) break;

            prev = cur;

            list_node<T> const *curNext = t.read_ptr(cur->next());

            if (NULL == curNext) break;

            cur = curNext;
         }
#ifndef BOOST_STM_USES_AS_NEW
         list_node<T> node(val);
         list_node<T> *newNode = t.new_memory_copy(node);
#else
         t.throw_if_forced_to_abort_on_new();
         list_node<T> *newNode = t.as_new(new list_node<T>(val));
#endif
         //--------------------------------------------------------------------
         // if cur->next() is null it means our newNode value is greater than
         // cur, so insert ourselves after cur.
         //--------------------------------------------------------------------
         if (val > cur->value()) t.write_ptr((list_node<T>*)cur)->next_for_new_mem(newNode, t);
         //--------------------------------------------------------------------
         // otherwise, we are smaller than cur, so insert between prev and cur
         //--------------------------------------------------------------------
         else
         {
            newNode->next(cur, t);
            t.write_ptr((list_node<T>*)prev)->next_for_new_mem(newNode, t);
         }
      }
      else
      {
#ifndef BOOST_STM_USES_AS_NEW
         list_node<T> node(val);
         list_node<T> *newNode = t.new_memory_copy(node);
#else
         t.throw_if_forced_to_abort_on_new();
         list_node<T> *newNode = t.as_new(new list_node<T>(val));
#endif
         t.write(head_).next_for_new_mem(newNode, t);
      }

      t.end();
      return true;
   }
   //--------------------------------------------------------------------------
   // find the location to insert the node. if the value already exists, bail
   //--------------------------------------------------------------------------
#ifdef BOOST_STM_USES_PARAM
   bool internal_lookup(T val, boost::stm::transaction &t)
#else
   bool internal_lookup(T const &val, boost::stm::transaction &t)
#endif
   {
      list_node<T> const *cur = &t.read(head_);

      for (; true ; cur = t.read(*cur).next() )
      {
         list_node<T> const *trueCur = &t.read(*cur);

         if (trueCur->value() == val)
         {
            t.end();
            return true;
         }

         if (NULL == trueCur->next()) break;
      }

      t.end();
      return false;
   }

   ////////////////////////////////////////////////////////////////////////////
   bool internal_remove(list_node<T> const &rhs, boost::stm::transaction &t)
   {
      list_node<T> const *prev = &t.read(head_);

      for (list_node<T> const *cur = prev; cur != NULL; prev = cur)
      {
         cur = t.read(*cur).next();

         if (NULL == cur) break;

         if (cur->value() == rhs.value())
         {
            list_node<T> const *curNext = t.read(*cur).next();

            t.delete_memory(*cur);
            t.write(*(list_node<T>*)prev).next(curNext, t);
            t.end();
            return true;
         }
      }

      return false;
   }

#ifdef BOOST_STM_USES_PARAM
   bool internal_remove(T value, boost::stm::transaction &t)
#else
   bool internal_remove(T const &value, boost::stm::transaction &t)
#endif
   {
      list_node<T> const *prev = &t.read(head_);

      for (list_node<T> const *cur = prev; cur != NULL; prev = cur)
      {
         cur = t.read(*cur).next();

         if (NULL == cur) break;

         if (cur->value() == value)
         {
            list_node<T> const *curNext = t.read(*cur).next();

            t.delete_memory(*cur);
            t.write(*(list_node<T>*)prev).next(curNext, t);
            t.end();
            return true;
         }
      }

      return false;
   }
   //--------------------------------------------------------------------------
   // find the location to insert the node. if the value already exists, bail
   //--------------------------------------------------------------------------
   bool internal_insert(list_node<T> const &rhs)
   {
      using namespace boost::stm;
      transaction t;

      list_node<T> const *headP = &t.read(head_);

      if (NULL != headP->next())
      {
         list_node<T> const *prev = headP;
         list_node<T> const *cur = t.read_ptr(headP->next());
         T val = rhs.value();

         while (true)
         {
            if (cur->value() == val) return false;
            else if (cur->value() > val || !cur->next()) break;

            prev = cur;

            list_node<T> const *curNext = t.read_ptr(cur->next());

            if (NULL == curNext) break;

            cur = curNext;
         }

         list_node<T> *newNode = t.new_memory_copy(rhs);

         //--------------------------------------------------------------------
         // if cur->next() is null it means our newNode value is greater than
         // cur, so insert ourselves after cur.
         //--------------------------------------------------------------------
         if (NULL == cur->next()) t.write_ptr((list_node<T>*)cur)->next_for_new_mem(newNode, t);
         //--------------------------------------------------------------------
         // otherwise, we are smaller than cur, so insert between prev and cur
         //--------------------------------------------------------------------
         else
         {
            newNode->next(cur, t);
            t.write_ptr((list_node<T>*)prev)->next_for_new_mem(newNode, t);
         }
      }
      else
      {
         list_node<T> *newNode = t.new_memory_copy(rhs);
         t.write(head_).next_for_new_mem(newNode, t);
      }

      t.end();
      return true;
   }

   ////////////////////////////////////////////////////////////////////////////
   bool internal_remove(list_node<T> const &rhs)
   {
      using namespace boost::stm;
      transaction t;

      list_node<T> const *prev = &t.read(head_);

      for (list_node<T> const *cur = prev; cur != NULL;
           prev = cur, cur = t.read(*cur).next())
      {
         if (cur->value() == rhs.value())
         {
            t.write(*(list_node<T>*)prev).next(t.read_ptr(cur)->next(), t);
            t.delete_memory(*cur);
            t.end();
            return true;
         }
      }

      return false;
   }

   list_node<T> head_;
};

void Test1writerNreadersWithMultipleThreads();
void TestLinkedListWithMultipleThreads();
void* TestThreadLinkedList(void *threadId);

#endif // TEST_LINKED_LIST_H
