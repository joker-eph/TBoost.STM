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

#ifndef TEST_PERSON_H
#define TEST_PERSON_H

#include <iostream>
#include "main.h"
#include <boost/stm/transaction.hpp>

void TestPerson();

template <typename T>
class named_array : 
public boost::stm::transaction_object < named_array<T> >
{
public:

   friend std::ostream& operator<<(std::ostream &str, named_array<T> const &t)
   {
      str << 0 == t.name_ ? "" : t.name();
      return str;
   }

   named_array() : name_(0), array_(0), size_(0) 
   {
      set_name("temp");
      resize(kMaxArrSize);
   }

   ~named_array() { delete [] name_; delete [] array_; size_ = 0; }

   char const * const name() const { return name_; }
   T* array() { return array_; }

   named_array& operator=(T &rhs) 
   { 
      array_[0] = rhs; 
      return *this; 
   }

   bool operator==(T &rhs) { return array_[0] == rhs; }

   bool operator==(named_array const &rhs) 
   { 
      return this->array_[0] == rhs.array_[0]; 
   }

   bool operator>(named_array const &rhs) { return this->array_[0] > rhs.array_[0]; }

   //--------------------------------------------------
   // copy semantics
   //--------------------------------------------------
   named_array(named_array const &rhs) : name_(0), array_(0), size_(0)
   {
      using namespace std;
      //cout << "c()";
      this->operator=(rhs);
   }

   named_array& operator=(named_array const &rhs)
   {
      using namespace std;
      //cout << "c=";
      if (0 != rhs.size())
      {
         T* tempArray = new T[rhs.size()];
         delete [] array_;

         array_ = tempArray;
         size_ = rhs.size();

         for (size_t i = 0; i < size_; ++i) array_[i] = rhs.array_[i];
      }

      if (0 != rhs.name())
      {
         set_name(rhs.name());
      }

      return *this;
   }

#if BUILD_MOVE_SEMANTICS

   //--------------------------------------------------
   // move semantics
   //--------------------------------------------------
   named_array(named_array &&rhs)
   {
      using namespace std;
      //cout << "m()";
      this->array_ = rhs.array_;
      this->size_ = rhs.size_;
      this->name_ = rhs.name_;
      rhs.array_ = 0;
      rhs.name_ = 0;
      rhs.size_ = 0;
   }

   named_array& operator=(named_array &&rhs)
   {
      using namespace std;
      //cout << "m=";
      std::swap(array_, rhs.array_);
      std::swap(name_, rhs.name_);

      int tempSize = rhs.size_;
      rhs.size_ = this->size_;
      this->size_ = tempSize;

      return *this;
   }
#endif

   void set_name(char const * const newName)
   {
      delete [] name_;
      name_ = new char[strlen(newName)+1];
      strcpy(name_, newName);
   }

   size_t size() const { return size_; }
   void resize(size_t newSize)
   { 
      delete [] array_; 
      array_ = new T[newSize];
      size_ = newSize;
   }
   
private:
   char* name_;
   T* array_;
   size_t size_;
};


#endif //TRANSFER_FUN_H

