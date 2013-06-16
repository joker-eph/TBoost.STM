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

#ifndef BOOST_STM_TX_PTR__HPP
#define BOOST_STM_TX_PTR__HPP

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include <boost/stm/transaction.hpp>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
namespace boost { namespace stm {

template <typename T>
class read_ptr
{
public:
 
   inline read_ptr(transaction &t, T const &tx_obj) : 
      t_(t), tx_ptr_(&const_cast<T&>(t_.read(tx_obj))), written_(false)
   {}

   const T* get() const
   {
      if (t_.forced_to_abort()) 
      {
         t_.lock_and_abort();
         throw aborted_transaction_exception("aborting transaction");
      }

      // we are already holding the written object
      if (written_) return tx_ptr_;

      T* temp = t_.get_written(*tx_ptr_);
 
      // if we found something, store this as the tx_ptr_
      if (0 != temp)
      {
         tx_ptr_ = temp;
         written_ = true;
      }

      return tx_ptr_;
   }

   inline T const & operator*() const { return *get(); }
   inline T const * operator->() const { return get(); }

   inline transaction &trans() { return t_; }

   T* write_ptr()
   {
      if (t_.forced_to_abort()) 
      {
         t_.lock_and_abort();
         throw aborted_transaction_exception("aborting transaction");
      }

      // we are already holding the written object
      if (written_) return tx_ptr_;

      T* temp = t_.get_written(*tx_ptr_);
 
      // if we found something, store this as the tx_ptr_
      if (0 != temp)
      {
         tx_ptr_ = temp;
         written_ = true;
      }
      else
      {
         tx_ptr_ = &t_.write(*tx_ptr_);
         written_ = true;
      }

      return tx_ptr_;
   }

private:
    
   mutable transaction &t_;
   mutable T *tx_ptr_;
   mutable bool written_;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename T>
class write_ptr
{
public:
 
   inline write_ptr(transaction &t, T & tx_obj) : 
      t_(t), tx_obj_(t_.write(tx_obj))
   {}

   inline T& operator*()
   {
      if (t_.forced_to_abort()) 
      {
         t_.lock_and_abort();
         throw aborted_transaction_exception("aborting transaction");
      }
      return tx_obj_;
   }

   inline T* operator->()
   {
      if (t_.forced_to_abort()) 
      {
         t_.lock_and_abort();
         throw aborted_transaction_exception("aborting transaction");
      }
      return &tx_obj_;
   }

private:   
   mutable boost::stm::transaction &t_;
   mutable T &tx_obj_;
};


}}
#endif


