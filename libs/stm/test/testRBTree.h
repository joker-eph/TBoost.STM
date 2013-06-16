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
#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#ifndef NULL
#define NULL 0
#endif

#include <string>
#include <sstream>
#include <fstream>
#include <boost/stm/transaction.hpp>

void TestRedBlackTree();
void TestRedBlackTreeWithMultipleThreads();

///////////////////////////////////////////////////////////////////////////////
namespace nRedBlackTree
{
   std::string const kRootStr =  "root ";
   std::string const kLeftStr =  "left ";
   std::string const kRightStr = "right";

   enum NodeColor
   {
      eRed,
      eBlack,
      eMaxColors
   };

   std::string const colorStrings[eMaxColors] =
   { " red ",
     "black"
   };
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
class RedBlackNode : public boost::stm::transaction_object < RedBlackNode<T> >
{
public:

   typedef T type;

    ////////////////////////////////////////////////////////////////////////////
   RedBlackNode(T const &in) : value_(in), color_(nRedBlackTree::eRed), 
      parent_(&sentinel), left_(&sentinel), right_(&sentinel) {}

   ////////////////////////////////////////////////////////////////////////////
   RedBlackNode() : value_(T()), color_(nRedBlackTree::eRed), parent_(&sentinel),
      left_(&sentinel), right_(&sentinel) {}

   T const & const_value(boost::stm::transaction &t) const
   { return t.read(*const_cast<RedBlackNode<T>*>(this)).value_; }

   T & value(boost::stm::transaction &t) 
   { return t.write(*const_cast<RedBlackNode<T>*>(this)).value_; }
   T & value() { return value_; }
   T const & value() const { return value_; }

   nRedBlackTree::NodeColor & color(boost::stm::transaction &t) 
   { return t.write(*const_cast<RedBlackNode<T>*>(this)).color_; }
   nRedBlackTree::NodeColor const & read_color(boost::stm::transaction &t) 
   { return t.read(*const_cast<RedBlackNode<T>*>(this)).color_; }

   nRedBlackTree::NodeColor & color() { return color_; }
   nRedBlackTree::NodeColor const & color() const { return color_; }

   RedBlackNode<T>* read_right(boost::stm::transaction &t)
   { return t.read(*const_cast<RedBlackNode<T>*>(this)).right_; }
   RedBlackNode<T> const * read_right(boost::stm::transaction &t) const
   { return t.read(*const_cast<RedBlackNode<T>*>(this)).right_; }

   RedBlackNode<T>*& right(boost::stm::transaction &t) 
   { return t.write(*const_cast<RedBlackNode<T>*>(this)).right_; }
   RedBlackNode<T>*& right(boost::stm::transaction &t) const
   { return t.write(*const_cast<RedBlackNode<T>*>(this)).right_; }

   RedBlackNode<T> *& right() { return right_; }
   RedBlackNode<T> const * right() const { return right_; }

   RedBlackNode<T>* read_left(boost::stm::transaction &t)
   { return t.read(*const_cast<RedBlackNode<T>*>(this)).left_; }
   RedBlackNode<T> const * read_left(boost::stm::transaction &t) const
   { return t.read(*const_cast<RedBlackNode<T>*>(this)).left_; }

   RedBlackNode<T>*& left(boost::stm::transaction &t)
   { return t.write(*const_cast<RedBlackNode<T>*>(this)).left_; }
   RedBlackNode<T>*& left(boost::stm::transaction &t) const
   { return t.write(*const_cast<RedBlackNode<T>*>(this)).left_; }

   RedBlackNode<T> *& left() { return left_; }
   RedBlackNode<T> const * left() const { return left_; }

   RedBlackNode<T>* read_parent(boost::stm::transaction &t)
   { return t.read(*const_cast<RedBlackNode<T>*>(this)).parent_; }

   RedBlackNode<T>*& parent(boost::stm::transaction &t) 
   { return t.write(*const_cast<RedBlackNode<T>*>(this)).parent_; }
   RedBlackNode<T>*& parent(boost::stm::transaction &t) const
   { return t.write(*const_cast<RedBlackNode<T>*>(this)).parent_; }

   RedBlackNode<T> *& parent() { return parent_; }
   RedBlackNode<T> const * parent() const { return parent_; }

   ////////////////////////////////////////////////////////////////////////////
   static void initializeSentinel()
   {
      sentinel.left_ = NULL;
      sentinel.right_ = NULL;
      sentinel.color_ = nRedBlackTree::eBlack;
   }

   static RedBlackNode<T> sentinel;

private:

   nRedBlackTree::NodeColor color_;
   
   T value_;

   mutable RedBlackNode<T> *left_;
   mutable RedBlackNode<T> *right_;
   mutable RedBlackNode<T> *parent_;
};

///////////////////////////////////////////////////////////////////////////////
template <typename T>
RedBlackNode<T> RedBlackNode<T>::sentinel;

///////////////////////////////////////////////////////////////////////////////
template <typename T>
class RedBlackTree
{
public:

   RedBlackTree() : root(new RedBlackNode<T>(T())) 
   { root->color() = nRedBlackTree::eBlack; RedBlackNode<T>::initializeSentinel(); }

   //--------------------------------------------------------------------------
   bool insert(RedBlackNode<T> const &node)
   {
      using namespace boost::stm;

      for (transaction t; ; t.restart())
      {
         try { return internal_insert(node, t); }
         catch (boost::stm::aborted_transaction_exception&) {}
      }
   }

   //--------------------------------------------------------------------------
   bool lookup(T const &v, T* found)
   {
      using namespace boost::stm;

      for (transaction t; ; t.restart())
      {
         try { return internal_lookup(v, found, t); }
         catch (boost::stm::aborted_transaction_exception&) {}
      }
   }

   bool remove(RedBlackNode<T> const &node);

   boost::stm::transaction_state clear();
   void cheap_clear();

   void print(std::ofstream &o);

   int walk_size();
   int internal_walk_size(int &i, RedBlackNode<T> *cur);

private:

   bool internal_remove(RedBlackNode<T> const &inNode, boost::stm::transaction &inT);
   bool internal_insert(RedBlackNode<T> const &node, boost::stm::transaction &inT);
   bool internal_lookup(T const &v, T* found, boost::stm::transaction &inT);

   void internal_cheap_clear(RedBlackNode<T> *cur);
   void internal_clear(RedBlackNode<T> &cur, boost::stm::transaction &t);
   void internal_remove_help(RedBlackNode<T> *x, boost::stm::transaction &t);
   RedBlackNode<T>* get_successor(RedBlackNode<T> *x, boost::stm::transaction &t) const;

   void internal_print(int const &i, std::string const &outputStr, 
      RedBlackNode<T> *cur, std::ofstream &o);
   RedBlackNode<T>* binary_insert(RedBlackNode<T> const &newEntry, boost::stm::transaction &t);
   void left_rotate(RedBlackNode<T> *node, boost::stm::transaction &t);
   void right_rotate(RedBlackNode<T> *node, boost::stm::transaction &t);

   RedBlackNode<T> *root;
};

///////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void RedBlackTree<T>::right_rotate(RedBlackNode<T> *y, boost::stm::transaction &t)
{
   RedBlackNode<T> &writeY = t.write(*y);

   RedBlackNode<T> *x = writeY.left();
   RedBlackNode<T> *xRight = x->read_right(t);

   writeY.left() = xRight;

   if (&RedBlackNode<T>::sentinel != xRight) xRight->parent(t) = y;

   RedBlackNode<T> *yParent = writeY.parent();
   x->parent(t) = yParent;

   if (y == yParent->read_left(t)) yParent->left(t) = x;
   else yParent->right(t) = x;

   x->right(t) = y;
   writeY.parent() = x;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
inline void RedBlackTree<T>::left_rotate(RedBlackNode<T> *x, boost::stm::transaction &t) 
{
   RedBlackNode<T> &writeX = t.write(*x);

   RedBlackNode<T> *y = writeX.right();
   RedBlackNode<T> *yLeft = y->read_left(t);

   writeX.right() = yLeft;

   if (&RedBlackNode<T>::sentinel != yLeft) yLeft->parent(t) = x;

   RedBlackNode<T> *xParent = writeX.parent();
   y->parent(t) = xParent;

   if (x == xParent->read_left(t)) xParent->left(t) = y;
   else xParent->right(t) = y;

   y->left(t) = x;
   writeX.parent() = y;
}

//--------------------------------------------------------------------------
template <typename T>
inline bool RedBlackTree<T>::internal_lookup(T const &v, T* found, boost::stm::transaction &t)
{
   //--------------------------------------------------------------------------
   // the actual root node is this->root->left()
   //--------------------------------------------------------------------------
   RedBlackNode<T> *y = root;
   RedBlackNode<T> *x = root->read_left(t);

   while (x != &RedBlackNode<T>::sentinel) 
   {
      if (NULL == x) throw boost::stm::aborted_transaction_exception("aborting transaction");
      if (x->value() == v) 
      {
         found = &x->value();
         t.end();
         return true;
      }

      y = x;
      x = x->value() > v ? x->read_left(t) : x->read_right(t);
   }

   return false;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
inline RedBlackNode<T>* RedBlackTree<T>::binary_insert(RedBlackNode<T> const &newEntry, 
      boost::stm::transaction &t)
{
   //--------------------------------------------------------------------------
   // the actual root node is this->root->left()
   //--------------------------------------------------------------------------
   RedBlackNode<T> *y = root;
   RedBlackNode<T> *x = root->read_left(t);

   T const &val = newEntry.value();

   while (x != &RedBlackNode<T>::sentinel) 
   {
      if (NULL == x) throw boost::stm::aborted_transaction_exception("aborting transaction");
      if (x->value() == val) 
      {
         t.lock_and_abort();
         return NULL;
      }

      y = x;
      x = x->value() > val ? x->read_left(t) : x->read_right(t);
   }

   // create new memory if this element will be inserted
   RedBlackNode<T> *z = t.new_memory_copy(newEntry);

   z->parent() = y;

   // in the event y is the dummy root, z should be to its left()
   if (y == root || y->value() > val) y->left(t) = z;
   else y->right(t) = z;

   return z;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool RedBlackTree<T>::internal_insert(RedBlackNode<T> const & newEntry, boost::stm::transaction &t)
{
   using namespace nRedBlackTree;
   
   //--------------------------------------------------------------------------
   // try to do a binary insert of this element. if the element already exists, 
   // return null and end the transactions since we don't allow duplicates
   //--------------------------------------------------------------------------
   RedBlackNode<T> *x = binary_insert(newEntry, t);

   if (NULL == x) return false;

   RedBlackNode<T> *xParent = x->parent();
   //RedBlackNode<T> *xParentParent = NULL;

   while (eRed == xParent->read_color(t) && xParent != &RedBlackNode<T>::sentinel)
   {
      RedBlackNode<T> *xParentParent = xParent->read_parent(t);

      //-----------------------------------------------------------------------
      // if my parent() is on the left() side of the tree
      //-----------------------------------------------------------------------
      RedBlackNode<T> *xParentParentLeft = xParentParent->read_left(t);
      if (xParent == xParentParentLeft) 
      {
         // then my uncle is to the right() side
         RedBlackNode<T> *y = xParentParent->read_right(t);

         //--------------------------------------------------------------------
         // if my uncle's color is red and my parent()'s color is red,
         // change my parent()'s and my uncle's color to black and
         // make my grandparent() red, then move me to my grandparent()
         //--------------------------------------------------------------------
         if (eRed == y->color()) 
         {
            xParent->color(t) = eBlack;
            y->color(t) = eBlack;
            xParentParent->color(t)= eRed;
            x = xParentParent;
         }
         //--------------------------------------------------------------------
         // otherwise, set me to my parent() and left()-rotate. then set my 
         // parent()'s color to black, my grandparent()'s color to red and 
         // right() rotate my grandparent()
         //--------------------------------------------------------------------
         else 
         {
            if (x == xParent->read_right(t)) 
            {
               // set x up one level
               x = xParent;
               left_rotate(x, t);
            }

            RedBlackNode<T> &writeObj = t.write(*x->read_parent(t));

            writeObj.color() = eBlack;
            writeObj.parent()->color(t) = eRed;
            right_rotate(writeObj.parent(), t);
         } 
      } 

      //-----------------------------------------------------------------------
      // the same as the above if, except right() and left() are exchanged
      //-----------------------------------------------------------------------
      else 
      {
         RedBlackNode<T> *y = xParentParentLeft; // saves a tx read by setting to xParParLeft

         if (eRed == y->color())
         {
            xParent->color(t) = eBlack;
            y->color(t) = eBlack;
            xParentParent->color(t) = eRed;
            x = xParentParent;
         } 
         else 
         {
            if (x == xParent->read_left(t)) 
            {
               // set x up one level
               x = xParent;
               right_rotate(x, t);
            }

            RedBlackNode<T> &writeObj = t.write(*x->read_parent(t));

            writeObj.color() = eBlack;
            writeObj.parent()->color(t) = eRed;
            left_rotate(writeObj.parent(), t);
         }
      }

      xParent = x->read_parent(t);
   }

   // always set the root to black - root->left() *IS* the root. root is
   // just a dummy node (-1 value).
   root->read_left(t)->color(t) = eBlack;

   t.end();
   return true;
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::internal_remove_help(RedBlackNode<T> *x, boost::stm::transaction &t)
{
   using namespace nRedBlackTree;

   RedBlackNode<T> *y = NULL;
   RedBlackNode<T> *xParent = NULL;

   while ( eBlack == x->read_color(t) && root->read_left(t) != x) 
   {
      xParent = x->read_parent(t);

      RedBlackNode<T> *xParentLeft = xParent->read_left(t);
      if (x == xParentLeft)
      {
         y = xParent->read_right(t);

         if (eRed == y->read_color(t)) 
         {
            y->color(t) = eBlack;
            xParent->color(t) = eRed;
            left_rotate(xParent, t);
            //-----------------------------------------------------------------
            // I think we have to read_parent instead of xParent since 
            // right_rotate could have changed xParent
            //-----------------------------------------------------------------
            y = x->read_parent(t)->read_right(t);
         }

         if (eBlack == y->read_right(t)->read_color(t) && 
             eBlack == y->read_left(t)->read_color(t)) 
         {
            y->color(t) = eRed;
            x = xParent;
         } 
         else 
         {
            if (eBlack == y->read_right(t)->read_color(t)) 
            {
               y->read_left(t)->color(t) = eBlack;
               y->color(t) = eRed;
               right_rotate(y, t);
               y = xParent->read_right(t);
            }

            y->color(t) = x->parent(t)->read_color(t);
            xParent->color(t) = eBlack;
            y->read_right(t)->color(t) = eBlack;
            left_rotate(xParent, t);
            break;
         }         
      } 
      else 
      { 
         y = xParentLeft;

         if (eRed == y->read_color(t)) 
         {
            y->color(t)= eBlack;
            xParent->color(t) = eRed;
            right_rotate(xParent, t);
            //-----------------------------------------------------------------
            // I think we have to read_parent instead of xParent since 
            // right_rotate could have changed xParent
            //-----------------------------------------------------------------
            y = x->read_parent(t)->read_left(t);
         }

         if (eBlack == y->read_left(t)->read_color(t) && 
             eBlack == y->read_right(t)->read_color(t)) 
         {
            y->color(t) = eRed;
            x = xParent;
         } 
         else 
         {
            if (eBlack == y->read_left(t)->read_color(t)) 
            {
               y->read_right(t)->color(t) = eBlack;
               y->color(t) = eRed;
               left_rotate(y, t);
               y = xParent->read_left(t);
            }

            y->color(t) = xParent->read_color(t);
            xParent->color(t) = eBlack;
            y->read_left(t)->color(t) = eBlack;
            right_rotate(xParent, t);
            break;
         }
      }
   }

   x->color(t) = eBlack;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
RedBlackNode<T>* RedBlackTree<T>::get_successor(RedBlackNode<T> *x, boost::stm::transaction &t) const
{
   RedBlackNode<T> *y = x->read_right(t);

   if (&RedBlackNode<T>::sentinel != y) 
   {
      RedBlackNode<T> *prevY = y;

      while ((y = y->read_left(t)) != &RedBlackNode<T>::sentinel)
      {
         prevY = y;
      }

      return prevY;
   }
   else 
   {
      y = x->read_parent(t);
   
      while (x == y->read_right(t)) 
      {
         x = y;
         y = y->read_parent(t);
      }

      if (y == root) return &RedBlackNode<T>::sentinel;
      else return y;
   }
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::cheap_clear()
{
   using namespace nRedBlackTree;

   internal_cheap_clear(root->left());

   root->left() = &RedBlackNode<T>::sentinel;
   root->right() = &RedBlackNode<T>::sentinel;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::internal_cheap_clear(RedBlackNode<T> *cur)
{
   if (&RedBlackNode<T>::sentinel == cur) return;

   internal_cheap_clear(cur->left());
   internal_cheap_clear(cur->right());

   delete cur;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
boost::stm::transaction_state RedBlackTree<T>::clear()
{
   using namespace nRedBlackTree; using namespace boost::stm;
   transaction t(begin_transaction);

   RedBlackNode<T> &curLeft = t.write(*root->left());
   internal_clear(curLeft, t);

   root->left() = &RedBlackNode<T>::sentinel;
   root->right() = &RedBlackNode<T>::sentinel;

   return t.end();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::internal_clear(RedBlackNode<T> &cur, boost::stm::transaction &t)
{
   if (&RedBlackNode<T>::sentinel == &cur) return;

   if (&RedBlackNode<T>::sentinel != cur.left())
   {
      RedBlackNode<T> &curLeft = t.write(*cur.left());
      internal_clear(curLeft, t);
   }
   if (&RedBlackNode<T>::sentinel != cur.right())
   {
      RedBlackNode<T> &curRight = t.write(*cur.right());
      internal_clear(curRight, t);
   }

   t.delete_memory(cur);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool RedBlackTree<T>::remove(RedBlackNode<T> const &inNode)
{
   using namespace nRedBlackTree; using namespace boost::stm;

   for (transaction t; ; t.restart())
   {
      try { return internal_remove(inNode, t); }
      catch (boost::stm::aborted_transaction_exception&) {}
   }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool RedBlackTree<T>::internal_remove(RedBlackNode<T> const &inNode, boost::stm::transaction &t)
{
   using namespace nRedBlackTree;
   t.begin();

   RedBlackNode<T> *z = root->left(t);

   // find node inNode in our tree then delete it
   for (; &RedBlackNode<T>::sentinel != z; )
   {
      if (NULL == z) throw boost::stm::aborted_transaction_exception("altered tree");
      if (z->value() == inNode.value()) break;
      z = z->value() > inNode.value() ? z->read_left(t) : z->read_right(t);
   }

   //--------------------------------------------------------------------------
   // if the node wasn't found, exit
   //--------------------------------------------------------------------------
   if (z->value() != inNode.value()) return false;

   RedBlackNode<T> *y = (z->read_left(t) == &RedBlackNode<T>::sentinel 
      || z->read_right(t) == &RedBlackNode<T>::sentinel) ? z : get_successor(z, t);

   RedBlackNode<T> *yLeft = y->read_left(t);

   RedBlackNode<T> *x = (yLeft == &RedBlackNode<T>::sentinel) 
      ? y->read_right(t) : yLeft;

   RedBlackNode<T> *yParent = y->read_parent(t);

   if (root == (x->parent(t) = yParent)) root->left(t) = x;
   else if (y == yParent->read_left(t)) yParent->left(t) = x;
   else yParent->right(t) = x;

   RedBlackNode<T> &writeY = t.write(*y);

   if (y != z) 
   {
      RedBlackNode<T> const *readZ = &t.read(*z);
      RedBlackNode<T> const *zLeft = readZ->left();
      RedBlackNode<T> const *zRight = readZ->right();
      RedBlackNode<T> const *zParent = readZ->parent();

      writeY.left() = (RedBlackNode<T>*)zLeft;
      writeY.right() = (RedBlackNode<T>*)zRight;
      writeY.parent() = (RedBlackNode<T>*)zParent;

      zLeft->parent(t) = y;
      zRight->parent(t) = y;
      
      if (z == zParent->read_left(t)) zParent->left(t) = y; 
      else zParent->right(t) = y;

      if (eBlack == writeY.color()) 
      {
         writeY.color() = readZ->color();
         internal_remove_help(x, t);
      } 
      else
      {
         writeY.color() = readZ->color(); 
      }

      t.delete_memory(*z);
   }
   else 
   {
      if (eBlack == writeY.color()) internal_remove_help(x, t);
      t.delete_memory(*y);
   }

   t.end();
   return true;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
int RedBlackTree<T>::walk_size()
{
   using namespace std;
   using namespace nRedBlackTree;
   int i = 0;
   return internal_walk_size(i, root->left());
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
int RedBlackTree<T>::internal_walk_size(int &i, RedBlackNode<T> *cur)
{
   using namespace std;
   using namespace nRedBlackTree;

   if (&RedBlackNode<T>::sentinel == cur) return i;

   // increment i for this node
   ++i;

   i = internal_walk_size(i, cur->left());
   i = internal_walk_size(i, cur->right());

   return i;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::print(std::ofstream &o)
{
   using namespace std;
   using namespace nRedBlackTree;

   int i = 0;

   o << "[ size: " << walk_size() << " ]" << endl;
   internal_print(i, kRootStr, root->left(), o);
   o << endl;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::internal_print(int const &i, std::string const &outputStr, 
   RedBlackNode<T> *cur, std::ofstream &o)
{
   using namespace std;
   using namespace nRedBlackTree;

   if (&RedBlackNode<T>::sentinel == cur) return;
   else 
   {
      o << "[ " << outputStr << " - " << colorStrings[cur->color()] << i 
        << " ]: " << cur->value() << endl;
   }

   internal_print(i + 1, kLeftStr, cur->left(), o);
   internal_print(i + 1, kRightStr, cur->right(), o);
}

#endif // RED_BLACK_TREE_H
