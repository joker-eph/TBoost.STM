//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Mehdi Amini 2013.
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
#ifndef RED_BLACK_TREEV2_H
#define RED_BLACK_TREEV2_H

#ifndef NULL
#define NULL 0
#endif

#include <string>
#include <sstream>
#include <fstream>
#include <boost/stm/transaction.hpp>

void TestRedBlackTreeV2();
void TestRedBlackTreeV2WithMultipleThreads();

///////////////////////////////////////////////////////////////////////////////
namespace nRedBlackTreeV2
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

class ProtectedColor : boost::stm::transaction_object<ProtectedColor>
{

};

///////////////////////////////////////////////////////////////////////////////
template <typename T>
class UnprotectedRedBlackNode
{
public:
   typedef boost::stm::protected_ptr<UnprotectedRedBlackNode> protected_ptr_node;
   typedef T type;

    ////////////////////////////////////////////////////////////////////////////
   UnprotectedRedBlackNode(T const &val = T())
     : value_(val), color_(eRed),
      parent_(0), left_(0), right_(0) {
   }
   UnprotectedRedBlackNode(UnprotectedRedBlackNode<T> const &in)
     : value_(in.value_), color_(in.color_),
      parent_(in.parent_), left_(in.left_), right_(in.right_) {
   }

   T const & value() const { return value_; }

   NodeColor & color() { return color_; }
   NodeColor const & color() const { return color_; }
   bool is_red() const { return color_==eRed; }
   bool is_black() const { return color_==eBlack; }

   protected_ptr_node const &  right() const { return right_; }
   protected_ptr_node & right()  { return right_; }
   protected_ptr_node const & left() const { return left_; }
   protected_ptr_node & left() { return left_; }
   protected_ptr_node const &  parent() const { return parent_; }
   protected_ptr_node & parent() { return parent_; }


private:
//   void operator= (const UnprotectedRedBlackNode<T> &);

   NodeColor color_; // FIXME MAYBE UNSAFE, should be protected ?
   T const &value_;

   protected_ptr_node left_;
   protected_ptr_node right_;
   protected_ptr_node parent_;
};



///////////////////////////////////////////////////////////////////////////////
template <typename T>
class RedBlackTree
{
public:

   // FIXME T is not necessary an int... replace -1 and -2
   RedBlackTree() : root_(-1), sentinel(-2)
   {
     root_.color() = eBlack;
     root_.left().unsafe_reset(&sentinel);
     root_.right().unsafe_reset(&sentinel);
   }

   //--------------------------------------------------------------------------
   bool insert(T const &value)
   {
      using namespace boost::stm;
      // FIXME: use macro?
      for (transaction t; ; t.restart())
      {
         try { return internal_insert(value, t); }
         catch (boost::stm::aborted_transaction_exception&) {}
      }
      assert(0 && "unreachable!");
   }

   //--------------------------------------------------------------------------
   bool lookup(T const &v, T* found)
   {
      using namespace boost::stm;
      // FIXME: use macro?
      for (transaction t; ; t.restart())
      {
         try { return internal_lookup(v, found, t); }
         catch (boost::stm::aborted_transaction_exception&) {}
      }
      assert(0 && "unreachable!");
   }

   bool remove(T const &node);

   void clear();
   void cheap_clear();

   void print(std::ofstream &o);
   void print();

   int node_count();

private:
   typedef typename UnprotectedRedBlackNode<T>::protected_ptr_node ptr_node;
   typedef UnprotectedRedBlackNode<T> raw_node;
   raw_node root_; // Fake root, the root of the tree is on the left
   raw_node sentinel; // End of the tree. It's hooked to every leafs

   ptr_node &root() { return root_.left(); };
   ptr_node const &root() const { return root_.left(); };
   bool is_sentinel(raw_node const *node) const { return node==&sentinel; }

   int internal_node_count(ptr_node const &cur, boost::stm::transaction &t);
   bool internal_remove(T const &inNode, boost::stm::transaction &inT);
   bool internal_insert(T const &val, boost::stm::transaction &inT);
   bool internal_lookup(T const &v, const T* found, boost::stm::transaction &inT);

   void internal_cheap_clear(raw_node *cur);
   void internal_clear(raw_node *cur, boost::stm::transaction &t);
   void internal_remove_help(raw_node *cur, boost::stm::transaction &t);
   const raw_node* get_successor(raw_node *cur, boost::stm::transaction &t) const;

   void internal_print(int const &i, std::string const &outputStr, 
       raw_node *cur);
   void internal_print(int const &i, std::string const &outputStr,
       raw_node *cur, std::ofstream &o, boost::stm::transaction &t);
   raw_node* binary_insert(T const &val, boost::stm::transaction &t);

   // Rotation
   // The direction is given by the template parameters
   // rotate<LEFT,RIGHT> is a left rotation
   // rotate<RIGHT,LEFT> is a right rotation
   template<ptr_node &(raw_node::*LEFT)(), ptr_node &(raw_node::*RIGHT)()>
   void rotate(raw_node *node, boost::stm::transaction &t);

   template<ptr_node &(raw_node::*LEFT)(), ptr_node &(raw_node::*RIGHT)()>
   void fixup(raw_node *&grandParent,
       raw_node *&parent,
       raw_node *&newNode, boost::stm::transaction &t);
};




//--------------------------------------------------------------------------
template <typename T>
inline bool RedBlackTree<T>::internal_lookup(T const &v, const T* found, boost::stm::transaction &t)
{
   raw_node *cur = root().get(t);

   while (!is_sentinel(cur))
   {
      if (NULL == cur) throw boost::stm::aborted_transaction_exception("altered tree");
      const T &curVal = cur->value();
      if (curVal== v)
      {
         found = &cur->value();
         t.end();
         return true;
      }

      cur = curVal > v ? cur->left().get(t) : cur->right().get(t);
   }

   return false;
}


///////////////////////////////////////////////////////////////////////////////
template <typename T>
inline typename RedBlackTree<T>::raw_node* RedBlackTree<T>::binary_insert(T const &val,
      boost::stm::transaction &t)
{
   raw_node *parent = &root_;
   raw_node *cur = root().get(t);
   while (!is_sentinel(cur))
   {
      // FIXME: NULL Should be impossible by construction, assert here? At least add a comment
      if (NULL == cur) throw boost::stm::aborted_transaction_exception("altered tree");

      const T &curVal = cur->value();
      if (curVal== val)
      {
         t.end();
         return NULL;
      }

      parent = cur;
      cur = curVal > val ? cur->left().get(t) : cur->right().get(t);
   }

   // At exit of the loop, "cur" points to a leaf, we create a node and
   // hook it to parent


   // create a "rawNode" first for efficiency, if we create a ProtectedNode we would have
   // to record it in the transaction to write the parent.
   raw_node *newRawNode = new raw_node(val);
   newRawNode->left().unsafe_reset(&sentinel);
   newRawNode->right().unsafe_reset(&sentinel);
   newRawNode->parent().unsafe_reset(parent);

   // FIXME, do not leak if transaction abort
   // The issue is that the class must inherit from transaction_object
   // and have its destructor virtual. The other options are:
   // - having my own handler in the transansaction loop, i.e. delete myself when it fails
   // - creating the node outside of the transaction
   //newRawNode = t.as_new(newRawNode);

   // in the event parent is the dummy root, newNode should be to its left()
   if(parent==&root_ || parent->value() > val) parent->left().reset(newRawNode,t);
   else parent->right().reset(newRawNode,t);

   return newRawNode;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
template<
typename RedBlackTree<T>::ptr_node &(RedBlackTree<T>::raw_node::*LEFT)(),
typename RedBlackTree<T>::ptr_node &(RedBlackTree<T>::raw_node::*RIGHT)()
>
inline void RedBlackTree<T>::rotate(raw_node *cur, boost::stm::transaction &t)
{
   raw_node *curParent = cur->parent().get(t);
   raw_node *rightChild = (cur->*RIGHT)().get(t);

   // Sanity check, this is the contract with the caller
   assert(rightChild && !is_sentinel(rightChild));

   raw_node *rightLeftGrandChild = (rightChild->*LEFT)().get(t);

   if (!is_sentinel(rightLeftGrandChild))
     rightLeftGrandChild->parent().reset(cur, t);

   rightChild->parent().reset(curParent,t);

   // FIXME having a bool telling if I'm left or right would avoid a spurious read here
   if (cur == (curParent->*RIGHT)().get(t)) {
     (curParent->*RIGHT)().reset(rightChild,t);
   }
   else {
     (curParent->*LEFT)().reset(rightChild,t);
   }

   (rightChild->*LEFT)().reset(cur,t);
   cur->parent().reset(rightChild, t);
   (cur->*RIGHT)().reset(rightLeftGrandChild,t);
}


///////////////////////////////////////////////////////////////////////////////
template <typename T>
template<
typename RedBlackTree<T>::ptr_node &(RedBlackTree<T>::raw_node::*LEFT)(),
typename RedBlackTree<T>::ptr_node &(RedBlackTree<T>::raw_node::*RIGHT)()
>
void RedBlackTree<T>::fixup(
    raw_node *&newNode, raw_node *&parent,raw_node *&grandParent,
    boost::stm::transaction &t)
{
  // Supposing I'm left
  // then my uncle is to the right() side
  raw_node *uncle = (grandParent->*RIGHT)().get(t);

  //--------------------------------------------------------------------
  // if my uncle's color is red and my parent()'s color is red,
  // change my parent()'s and my uncle's color to black and
  // make my grandparent() red, then move me to my grandparent()
  //--------------------------------------------------------------------
  if (uncle->is_red())
  {
     parent->color() = eBlack;
     uncle->color() = eBlack;
     grandParent->color()= eRed;
     newNode = grandParent;
  }
  //--------------------------------------------------------------------
  // otherwise, set me to my parent() and left()-rotate. then set my
  // parent()'s color to black, my grandparent()'s color to red and
  // right() rotate my grandparent()
  //--------------------------------------------------------------------
  else
  {
     if (newNode == (parent->*RIGHT)().get(t))
     {
        // set x up one level
        newNode = parent;
        rotate<LEFT,RIGHT>(newNode, t);
     }

     newNode->parent().get(t)->color() = eBlack;
     grandParent->color() = eRed;
     rotate<RIGHT,LEFT>(grandParent, t);
  }
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool RedBlackTree<T>::internal_insert(T const & val, boost::stm::transaction &t)
{
   //--------------------------------------------------------------------------
   // try to do a binary insert of this element. if the element already exists, 
   // return null and end the transactions since we don't allow duplicates
   //--------------------------------------------------------------------------
   raw_node *newNode = binary_insert(val, t);

   if (NULL == newNode) return false;

   // Insertion succeeded, now verify and enforce Red-Black invariants

   raw_node *parent = newNode->parent().get(t);

   while (parent->is_red() && !is_sentinel(parent))
   {
     raw_node *grandParent = parent->parent().get(t);

      //-----------------------------------------------------------------------
      // if my parent() is on the left() side of the tree
      //-----------------------------------------------------------------------
     if (parent == grandParent->left().get(t))
      {
         fixup<&raw_node::left,&raw_node::right>(newNode,parent,grandParent,t);
      } 
      //-----------------------------------------------------------------------
      // the same as the above if, except right() and left() are exchanged
      //-----------------------------------------------------------------------
      else 
      {
        fixup<&raw_node::right,&raw_node::left>(newNode,parent,grandParent,t);
      }
      parent = newNode->parent().get(t);
  }

   // always set the root to black
   raw_node *r=root().get(t);
   if(r->color() == eRed)
     r->color() = eBlack;

   t.end();
   return true;
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::internal_remove_help(raw_node *cur, boost::stm::transaction &t)
{
   const raw_node *rootRead = root().get(t);

   while ( eBlack == cur->color() && rootRead != cur)
   {
      raw_node *curParent = cur->parent().get(t);
      raw_node *curParentLeft = curParent->left().get(t);

      if (cur == curParentLeft)
      {
         raw_node *sibling = curParent->right().get(t);
         if (eRed == sibling->color())
         {
            sibling->color() = eBlack;
            curParent->color() = eRed;
            rotate<&UnprotectedRedBlackNode<T>::left,&UnprotectedRedBlackNode<T>::right>(curParent, t);
            //-----------------------------------------------------------------
            // I think we have to reread parent instead of curParent since
            // left_rotate has probably changed curParent
            //-----------------------------------------------------------------
            curParent=cur->parent().get(t);
            sibling = curParent->right().get(t);
         }

         if (eBlack == sibling->right().get(t)->color() &&
             eBlack == sibling->left().get(t)->color())
         {
            sibling->color() = eRed;
            cur = curParent;
         }
         else 
         {
            if (eBlack == sibling->color())
            {
               sibling->left().get(t)->color() = eBlack;
               sibling->color() = eRed;
               rotate<&UnprotectedRedBlackNode<T>::right,&UnprotectedRedBlackNode<T>::left>(sibling, t);
               //-----------------------------------------------------------------
               // I think we have to reread parent instead of curParent since
               // right_rotate has probably changed curParent
               //-----------------------------------------------------------------
               curParent=cur->parent().get(t);
               sibling = curParent->right().get(t);
            }
            sibling->color() = curParent->color();
            curParent->color() = eBlack;
            sibling->right().get(t)->color() = eBlack;
            rotate<&UnprotectedRedBlackNode<T>::left,&UnprotectedRedBlackNode<T>::right>(curParent, t);
            break;
         }         
      } 
      else 
      { 
         raw_node *sibling = curParentLeft;

         if (eRed == sibling->color())
         {
            sibling->color() = eBlack;
            curParent->color() = eRed;
            rotate<&UnprotectedRedBlackNode<T>::right,&UnprotectedRedBlackNode<T>::left>(curParent, t);
            //-----------------------------------------------------------------
            // I think we have to read_parent instead of curParent since
            // right_rotate could have changed curParent
            //-----------------------------------------------------------------
            curParent=cur->parent().get(t);
            sibling = curParent->right().get(t);
         }

         if (eBlack == sibling->right().get(t)->color() &&
             eBlack == sibling->left().get(t)->color())
         {
            sibling->color() = eRed;
            cur = curParent;
         } 
         else 
         {
            if (eBlack == sibling->color())
            {
               sibling->right().get(t)->color() = eBlack;
               sibling->color() = eRed;
               rotate<&UnprotectedRedBlackNode<T>::left,&UnprotectedRedBlackNode<T>::right>(sibling, t);
               //-----------------------------------------------------------------
               // I think we have to reread parent instead of curParent since
               // left_rotate has probably changed curParent
               //-----------------------------------------------------------------
               curParent=cur->parent().get(t);
               sibling = curParent->left().get(t);
            }
            sibling->color() = curParent->color();
            curParent->color() = eBlack;
            sibling->left().get(t)->color() = eBlack;
            rotate<&UnprotectedRedBlackNode<T>::right,&UnprotectedRedBlackNode<T>::left>(curParent, t);
            break;
         }
      }
   }

   cur->color() = eBlack;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
const typename RedBlackTree<T>::raw_node* RedBlackTree<T>::get_successor(raw_node *cur, boost::stm::transaction &t) const
{
   raw_node *rightChild = cur->right().get(t);

   if (!is_sentinel(rightChild))
   {
     cur = rightChild;
     raw_node *prev;

      do {
        prev = cur;
        cur = cur->left().get(t);
      } while(!is_sentinel(cur));

      return prev;
   }
   else 
   {
      raw_node *parent = cur->parent().get(t);
   
      while (cur == parent->right().get(t))
      {
         cur = parent;
         parent = parent->parent().get(t);
      }

      if (root().get(t)==parent) return &sentinel;
      else return parent;
   }
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::cheap_clear()
{
   internal_cheap_clear(root().unsafe_get());

   root_.left().unsafe_reset(&sentinel);
   root_.right().unsafe_reset(&sentinel);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::internal_cheap_clear(raw_node *cur)
{
   if (is_sentinel(cur)) return;

   internal_cheap_clear(cur->left().unsafe_get());
   internal_cheap_clear(cur->right().unsafe_get());

   delete cur;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::clear()
{
   using namespace boost::stm;
   transaction t;

   raw_node *curLeft = root().read(t);
   internal_clear(curLeft, t);

   curLeft->left().reset(&sentinel, t);
   curLeft->right().reset(&sentinel, t);

   return t.end();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::internal_clear(raw_node *cur, boost::stm::transaction &t)
{
   if (is_sentinel(cur)) return;
   raw_node const &curRead = cur->get(t);
   if (!is_sentinel(curRead.left()))
   {
      internal_clear(curRead.left(), t);
   }
   if (!is_sentinel(curRead->right()))
   {
      internal_clear(curRead->right(), t);
   }

   t.delete_memory(cur);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool RedBlackTree<T>::remove(T const &inNode)
{
   using namespace boost::stm;

   for (transaction t; ; t.restart())
   {
      try { return internal_remove(inNode, t); }
      catch (boost::stm::aborted_transaction_exception&) {}
   }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool RedBlackTree<T>::internal_remove(T const &inNode, boost::stm::transaction &t)
{
   t.begin(); // FIXME Macros?

   raw_node *cur = root().get(t);

   // find node inNode in our tree then delete it
   for (; !is_sentinel(cur); )
   {
      if (NULL == cur) throw boost::stm::aborted_transaction_exception("altered tree");
      if (cur->value() == inNode) break;
      cur = cur->value() > inNode ? cur->left().get(t) : cur->right().get(t);
   }

   //--------------------------------------------------------------------------
   // if the node wasn't found, exit
   //--------------------------------------------------------------------------
   if (cur->value() != inNode) return false;

   raw_node *succ = (is_sentinel(cur->left().get(t)) || is_sentinel(cur->right().get(t)))
                ? cur : const_cast<raw_node *>(get_successor(cur, t));
   // The previous const_cast is only because get_successor() may return sentinel.
   // then assert here by security
   assert(!is_sentinel(succ) && "Sentinel unexpected here!");

   raw_node *succParent = succ->parent().get(t);
   raw_node *succLeftChild = succ->left().get(t);

   raw_node *succSuccLeftChild = (is_sentinel(succLeftChild))
                             ? succ->right().get(t) : succLeftChild;
   succSuccLeftChild->parent().reset(succParent, t);

   if (root().get(t) == succParent) root().reset(succSuccLeftChild,t);
   else if (succ == succParent->left().get(t)) succParent->left().reset(succSuccLeftChild,t);
   else succParent->right().reset(succSuccLeftChild,t);


   if (succ != cur)
   {
      raw_node *curLeft = cur->left().get(t);
      raw_node *curRight = cur->right().get(t);
      raw_node *curParent = cur->parent().get(t);

      succ->left().reset(curLeft,t);
      succ->right().reset(curRight,t);
      succ->parent().reset(curParent,t);

      curLeft->parent().reset(succ,t);
      curRight->parent().reset(succ,t);
      
      if (cur == curParent->left().get(t)) curParent->left().reset(succ,t);
      else curParent->right().reset(succ,t);

      succ->color() = cur->color(); // FIXME unprotected
   }
   if (eBlack == succ->color())
     internal_remove_help(succSuccLeftChild, t);

//   t.delete_memory(*cur); FIXME??

   t.end();
   return true;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
int RedBlackTree<T>::node_count()
{
   using namespace std;
   int i;
   atomic(t) {
     i = internal_node_count(root(), t);
   } end_atom
   return i;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
int RedBlackTree<T>::internal_node_count(ptr_node const &cur, boost::stm::transaction &t)
{
   using namespace std;

   // Access to the node by reading
   raw_node const *curRead = cur.get(t);

   if (is_sentinel(curRead)) return 0;

   int i=1; // Count this node
   i += internal_node_count(curRead->left(),t);
   i += internal_node_count(curRead->right(),t);

   return i;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::print(std::ofstream &o)
{
   using namespace std;

   int i = 0;

   atomic(t) {
     o << "[ size: " << i << " ]" << endl;
     internal_print(i, kRootStr, root().get(t), o,t);
     o << endl;
     o.close();
   } end_atom // FIXME CATCH and clear "o"
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::internal_print(int const &i, std::string const &outputStr,
   raw_node *cur, std::ofstream &o, boost::stm::transaction &t)
{
   using namespace std;
   if (is_sentinel(cur)) return;
   else 
   {
     o << "[ " << outputStr << " - " << colorStrings[cur->color()] << i
       << " ]: " << cur->value() << "\n";
     o << "[ " << outputStr << " - " << colorStrings[cur->color()] << i
       << " ]: " << cur->value() << "\n";
   }

   internal_print(i + 1, kLeftStr, cur->left().get(t), o,t);
   internal_print(i + 1, kRightStr, cur->right().get(t), o,t);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::print()
{
   using namespace std;

   int i = 0;

   std::cout << "[ size: " << i << " ]" << endl;
   internal_print(i, kRootStr, root().unsafe_get());
   std::cout << endl;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void RedBlackTree<T>::internal_print(int const &i, std::string const &outputStr,
   raw_node *cur)
{
   using namespace std;
   if (is_sentinel(cur)) return;
   else
   {
     std::cout << "[ " << outputStr << " - " << colorStrings[cur->color()] << " " << i
       << " ]: " << cur->value() << "\n";
   }

   internal_print(i + 1, kLeftStr, cur->left().unsafe_get());
   internal_print(i + 1, kRightStr, cur->right().unsafe_get());
}

}

#endif // RED_BLACK_TREEV2_H
