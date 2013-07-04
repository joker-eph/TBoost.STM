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

// String for prettyprint the tree
std::string const kRootStr =  "root ";
std::string const kLeftStr =  "left ";
std::string const kRightStr = "right";
std::string const colorStrings[] = {"red","black"};
enum NodeColor { eRed, eBlack, };

// This class handle the color of a node, protected by a transaction
class ProtectedColor : public boost::stm::transaction_object<ProtectedColor>
{
   NodeColor color;
  public:
   ProtectedColor(NodeColor c) : color(c) {}
   NodeColor get(boost::stm::transaction &t) const {
     return t.read(*this).color;
   }
   NodeColor set(NodeColor c, boost::stm::transaction &t) {
      return t.write(*this).color = c;
   }
   NodeColor unsafeSet(NodeColor c) {
      return color = c;
   }
};



/**
 * Node in the graph.
 *
 * Has a reference to a value (template gives the type), a color, and
 * links to its parent and to its left and right children. The value is
 * immutable while the other attributes are protected and need a
 * transaction to be manipulated.
 *
 * It inherit from transaction_object<> so that it can be created or deleted
 * in the context of a transaction.
 */
template <typename T>
class RedBlackNode : public boost::stm::transaction_object<ProtectedColor>
{
  // Non copyable
  void operator= (const RedBlackNode<T> &);
  RedBlackNode(RedBlackNode const &);

public:
   // Protected pointer, need a transaction to access
   typedef boost::stm::protected_ptr<RedBlackNode> protected_ptr_node;
   // Type of the value
   typedef T type;

   // Ctor
   RedBlackNode(T const &val = T())
     : value_(val), color_(eRed),
      parent_(0), left_(0), right_(0) {
   }

   // Accessors
   T const & value() const { return value_; }
   ProtectedColor &color() { return color_; }
   ProtectedColor const &color() const { return color_; }
   protected_ptr_node const &  right() const { return right_; }
   protected_ptr_node & right()  { return right_; }
   protected_ptr_node const & left() const { return left_; }
   protected_ptr_node & left() { return left_; }
   protected_ptr_node const &  parent() const { return parent_; }
   protected_ptr_node & parent() { return parent_; }


private:
   // Attributes
   T const &value_;              // Value of the node (immutable)
   ProtectedColor color_;        // Color of the node (red/black)
   protected_ptr_node left_;     // Pointer to the left child node
   protected_ptr_node right_;    // Pointer to the right child node
   protected_ptr_node parent_;   // Pointer to the parent node
};



/** The Tree structure
 *
 * The value contained in the nodes have to be of the type given as template
 * parameter.
 * The tree keeps a node as "sentinel" (i.e. to detect leaf) instead of a NULL
 * pointer. It also keeps a fake root as entry point to the tree, the real
 * root (if any) is the first left child.
 */
template <typename T>
class RedBlackTree
{
public:

   // Ctor
   // FIXME T is not necessary an int => replace (remove?) -1 and -2
   RedBlackTree() : root_(-1), sentinel(-2)
   {
     root_.color().unsafeSet(eBlack);
     root_.left().unsafe_reset(&sentinel);
     root_.right().unsafe_reset(&sentinel);
   }


   /*
    * Insertion: a node is created for the value, and inserted in the tree
    */
   bool insert(T const &value)
   {
      bool result;
      atomic(t) {
         result=internal_insert(value, t);
      } end_atom
      return result;
   }

   /*
    * Lookup: the value is searched in the tree. True is returned if it is
    * found, false if it is not.
    */
   bool contains(T const &v)
   {
      bool result;
      atomic(t) {
         result=internal_lookup(v, t);
      } end_atom
   }

   /*
    * Remove the node containing the given value in the tree.
    * Returns true if the value was present, false if it is not.
    */
   bool remove(T const &val);

   /*
    * Empty the tree
    */
   void clear();

   /*
    * Empty the tree, without protecting with a transaction!
    */
   void unsafe_clear();

   /*
    * Print the tree to a stream
    */
   void print(std::ofstream &o);

   /*
    * Size of the tree (number of nodes)
    */
   int node_count();

private:
   /* Types for the (transaction protected) pointers to node */
   typedef typename RedBlackNode<T>::protected_ptr_node ptr_node;
   /* Types for the (raw, aka unprotected) nodes */
   typedef RedBlackNode<T> raw_node;

   raw_node root_;    // Fake root, the root of the tree is on the left
   raw_node sentinel; // End of the tree. It's hooked to each leaf

   ptr_node &root() { return root_.left(); };
   ptr_node const &root() const { return root_.left(); };
   bool is_sentinel(raw_node const *node) const { return node==&sentinel; }

   // Internal helpers to implement the public interface
   int internal_node_count(ptr_node const &cur, boost::stm::transaction &t);
   bool internal_remove(T const &val, boost::stm::transaction &inT);
   bool internal_insert(T const &val, boost::stm::transaction &inT);
   bool internal_lookup(T const &v, boost::stm::transaction &inT);
   void internal_unsafe_clear(raw_node *cur);
   void internal_clear(raw_node *cur, boost::stm::transaction &t);
   void internal_remove_help(raw_node *cur, boost::stm::transaction &t);
   void internal_print(int const &i, std::string const &outputStr,
       raw_node *cur, std::ofstream &o, boost::stm::transaction &t);
   raw_node* binary_insert(T const &val, boost::stm::transaction &t);

   /* Walk the tree to find the lexicographic successor for a given node */
   const raw_node* get_successor(raw_node *cur, boost::stm::transaction &t) const;

   /* Rotation
    * The direction is given by the template parameters
    * rotate<LEFT,RIGHT> is a left rotation
    * rotate<RIGHT,LEFT> is a right rotation
    */
   template<ptr_node &(raw_node::*LEFT)(), ptr_node &(raw_node::*RIGHT)()>
   void rotate(raw_node *node, boost::stm::transaction &t);


   /* Fix the tree invariant (coloring, depth)
    * The direction is given by the template parameters
    * fixup<LEFT,RIGHT> is normal and fixup<RIGHT,LEFT> is reversed
    * The algorithm is symmetric, so I rely on the template parameter to
    * instanciate both version.
    */
   template<ptr_node &(raw_node::*LEFT)(), ptr_node &(raw_node::*RIGHT)()>
   void fixup(raw_node *&grandParent,
       raw_node *&parent,
       raw_node *&newNode, boost::stm::transaction &t);
};




/*
 * Helper that implements the lookup of a value in the tree.
 */
template <typename T>
inline bool RedBlackTree<T>::internal_lookup(T const &v, boost::stm::transaction &t)
{
   // Start at the root
   raw_node *cur = root().get(t);

   // Iterate until a leaf is found
   while (!is_sentinel(cur))
   {
      // Should be impossible (assert instead?)
      if (NULL == cur) throw boost::stm::aborted_transaction_exception("altered tree");

      // If the value is found, we end the transaction and return true
      if (cur->value()== v)
      {
         t.end();
         return true;
      }
      // Next node is left if the current one is bigger than v, right else
      cur = cur->value() > v ? cur->left().get(t) : cur->right().get(t);
   }
   // We reached a leaf, v is not present in the tree, return false.
   return false;
}


/*
 * Insert a given value in a new node and hook it in the tree
 * If the value exists in the tree, returns NULL
 */
template <typename T>
inline typename RedBlackTree<T>::raw_node* RedBlackTree<T>::binary_insert(T const &val,
      boost::stm::transaction &t)
{
   raw_node *parent = &root_;
   raw_node *cur = root().get(t);

   // Walk through the tree until we find a leaf (insertion point)
   while (!is_sentinel(cur))
   {
      // Should be impossible by construction, assert here? At least add a comment
      if (NULL == cur) throw boost::stm::aborted_transaction_exception("altered tree");

      // If the value is already present in the tree, we end the transaction
      // and return NULL
      if (cur->value() == val)
      {
         t.end();
         return NULL;
      }
      // Next node is left if the current one is bigger than v, right else
      cur = cur->value() > val ? cur->left().get(t) : cur->right().get(t);
   }

   // At exit of the loop, "cur" points to a leaf, we create a node and
   // hook it to parent

   // create a "rawNode" first for efficiency, if we create a ProtectedNode we
   // would have to record it in the transaction to write the parent.
   raw_node *newRawNode = new raw_node(val);
   newRawNode->left().unsafe_reset(&sentinel);
   newRawNode->right().unsafe_reset(&sentinel);
   newRawNode->parent().unsafe_reset(parent);

   // Do not leak if transaction aborts: register the new node.
   newRawNode = t.as_new(newRawNode);

   // in the event parent is the dummy root, newNode should be to its left()
   if(parent==&root_ || parent->value() > val) parent->left().reset(newRawNode,t);
   else parent->right().reset(newRawNode,t);

   return newRawNode;
}


/*
 * Rotation
 *
 * During "fixup" of the Red-Black-Tree invariants, some "rotation" are needed.
 * Rotation can be either left or right, since the algorithm is symmetric, it
 * is implemented once for the LEFT rotation and the RIGHT version is obtained
 * by inverting the template parameters.
 */
template <typename T>
template<
typename RedBlackTree<T>::ptr_node &(RedBlackTree<T>::raw_node::*LEFT)(),
typename RedBlackTree<T>::ptr_node &(RedBlackTree<T>::raw_node::*RIGHT)()
>
inline void RedBlackTree<T>::rotate(raw_node *cur, boost::stm::transaction &t)
{
   // !Variables are named for a LEFT rotation!

   // The rotation occurs around "cur"
   // first gather the nodes involved
   raw_node *curParent = cur->parent().get(t);
   raw_node *rightChild = (cur->*RIGHT)().get(t);

   // Sanity check, this is the contract with the caller
   assert(rightChild && !is_sentinel(rightChild));

   raw_node *rightLeftGrandChild = (rightChild->*LEFT)().get(t);

   // My grand child becomes direct child
   if (!is_sentinel(rightLeftGrandChild))
     rightLeftGrandChild->parent().reset(cur, t);
   (cur->*RIGHT)().reset(rightLeftGrandChild,t);

   // My child takes my place, parent link here, reverse link follows
   rightChild->parent().reset(curParent,t);

   // FIXME a bool telling if I'm left or right would avoid a read here
   if (cur == (curParent->*RIGHT)().get(t)) {
     (curParent->*RIGHT)().reset(rightChild,t);
   }
   else {
     (curParent->*LEFT)().reset(rightChild,t);
   }

   // I'm left child of my former child
   (rightChild->*LEFT)().reset(cur,t);
   cur->parent().reset(rightChild, t);
}


/*
 * Fixup check if the current situation for a node respect the tree invariant
 * and "fix" it locally. It involves rotation in the process.
 * Fixup can be LEFT or RIGHT. The function is written for the LEFT version, the
 * RIGHT one can be obtained by reverting the template parameters.
 */
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
  if (uncle->color().get(t)==eRed)
  {
     parent->color().set(eBlack,t);
     uncle->color().set(eBlack,t);
     grandParent->color().set(eRed,t);
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

     newNode->parent().get(t)->color().set(eBlack,t);
     grandParent->color().set(eRed,t);
     rotate<RIGHT,LEFT>(grandParent, t);
  }
}


/*
 * Insert a node in the tree for the given value. It the value already exists
 * in the tree, returns false
 */
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

   while (!is_sentinel(parent) /* root... */ && parent->color().get(t)==eRed)
   {
      raw_node *grandParent = parent->parent().get(t);

      // if my parent() is on the left() side of the tree, I need a "LEFT" fixup
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
      // Walk up through the tree
      parent = newNode->parent().get(t);
  }

   // always set the root to black
   raw_node *r = root().get(t);
   r->color().set(eBlack,t);

   t.end();
   return true;
}

/*
 *  Remove a given node and keep the tree invariant
 *  FIXME: some refactoring is needed to benefit from symmetry
 */
template <typename T>
void RedBlackTree<T>::internal_remove_help(raw_node *cur, boost::stm::transaction &t)
{
   const raw_node *rootRead = root().get(t);

   while ( eBlack == cur->color().get(t) && rootRead != cur)
   {
      raw_node *curParent = cur->parent().get(t);
      raw_node *curParentLeft = curParent->left().get(t);

      if (cur == curParentLeft)
      {
         raw_node *sibling = curParent->right().get(t);
         if (eRed == sibling->color().get(t))
         {
            sibling->color().set(eBlack,t);
            curParent->color().set(eRed,t);
            rotate<&raw_node::left,&raw_node::right>(curParent, t);
            //-----------------------------------------------------------------
            // I think we have to reread parent instead of curParent since
            // left_rotate has probably changed curParent
            //-----------------------------------------------------------------
            curParent=cur->parent().get(t);
            sibling = curParent->right().get(t);
         }

         if (eBlack == sibling->right().get(t)->color().get(t) &&
             eBlack == sibling->left().get(t)->color().get(t))
         {
            sibling->color().set(eRed,t);
            cur = curParent;
         }
         else 
         {
            if (eBlack == sibling->color().get(t))
            {
               sibling->left().get(t)->color().set(eBlack,t);
               sibling->color().set(eRed,t);
               rotate<&raw_node::right,&raw_node::left>(sibling, t);
               //-----------------------------------------------------------------
               // I think we have to reread parent instead of curParent since
               // right_rotate has probably changed curParent
               //-----------------------------------------------------------------
               curParent=cur->parent().get(t);
               sibling = curParent->right().get(t);
            }
            sibling->color().set(curParent->color().get(t),t);
            curParent->color().set(eBlack,t);
            sibling->right().get(t)->color().set(eBlack,t);
            rotate<&raw_node::left,&raw_node::right>(curParent, t);
            break;
         }         
      } 
      else 
      { 
         raw_node *sibling = curParentLeft;

         if (eRed == sibling->color().get(t))
         {
            sibling->color().set(eBlack,t);
            curParent->color().set(eRed,t);
            rotate<&raw_node::right,&raw_node::left>(curParent, t);
            //-----------------------------------------------------------------
            // I think we have to read_parent instead of curParent since
            // right_rotate could have changed curParent
            //-----------------------------------------------------------------
            curParent=cur->parent().get(t);
            sibling = curParent->right().get(t);
         }

         if (eBlack == sibling->right().get(t)->color().get(t) &&
             eBlack == sibling->left().get(t)->color().get(t))
         {
            sibling->color().set(eRed,t);
            cur = curParent;
         } 
         else 
         {
            if (eBlack == sibling->color().get(t))
            {
               sibling->right().get(t)->color().set(eBlack,t);
               sibling->color().set(eRed,t);
               rotate<&raw_node::left,&raw_node::right>(sibling, t);
               //-----------------------------------------------------------------
               // I think we have to reread parent instead of curParent since
               // left_rotate has probably changed curParent
               //-----------------------------------------------------------------
               curParent=cur->parent().get(t);
               sibling = curParent->left().get(t);
            }
            sibling->color().set(curParent->color().get(t),t);
            curParent->color().set(eBlack,t);
            sibling->left().get(t)->color().set(eBlack,t);
            rotate<&raw_node::right,&raw_node::left>(curParent, t);
            break;
         }
      }
   }

   cur->color().set(eBlack,t);
}

/*
 * Walk the tree to find the node containing the next value in the tree.
 * Return sentinel if no successor can be found
 */
template <typename T>
const typename RedBlackTree<T>::raw_node* RedBlackTree<T>::get_successor(raw_node *cur, boost::stm::transaction &t) const
{
   raw_node *rightChild = cur->right().get(t);

   /* If I have a right child, the next value is the leftmost node in this
    * subtree!
    */
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
      /* I don't have a right child... Then I have to walk up the tree. I have
       * to walk until I am left to the next parent. This parent is the next
       * node.
       */
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

/*
 * Clear the tree without protecting by any transaction
 */
template <typename T>
void RedBlackTree<T>::unsafe_clear()
{
   internal_unsafe_clear(root().unsafe_get());

   root_.left().unsafe_reset(&sentinel);
   root_.right().unsafe_reset(&sentinel);
}

/*
 * Recursively delete every node in the tree
 */
template <typename T>
void RedBlackTree<T>::internal_unsafe_clear(raw_node *cur)
{
   if (is_sentinel(cur)) return;

   internal_unsafe_clear(cur->left().unsafe_get());
   internal_unsafe_clear(cur->right().unsafe_get());

   delete cur;
}

/*
 * Clear the tree, protected version
 */
template <typename T>
void RedBlackTree<T>::clear()
{
   atomic(t) {
      raw_node *curLeft = root().get(t);
      internal_clear(curLeft, t);
      curLeft->left().reset(&sentinel, t);
      curLeft->right().reset(&sentinel, t);
   } end_atom
}

/*
 * Recursively attach every node in the tree to a transaction, marked for
 * deletion (atomically).
 */
template <typename T>
void RedBlackTree<T>::internal_clear(raw_node *cur, boost::stm::transaction &t)
{
   if (is_sentinel(cur)) return;
   if (!is_sentinel(cur->left().get(t)))
   {
      internal_clear(cur->left().get(t), t);
   }
   if (!is_sentinel(cur->right().get(t)))
   {
      internal_clear(cur->right().get(t), t);
   }

   t.delete_memory(*cur);
}

/*
 * Remove a value from the tree. Return true on success, false if not found.
 */
template <typename T>
bool RedBlackTree<T>::remove(T const &val)
{
   bool result;
   atomic(t) {
      result = internal_remove(val, t);
   } end_atom
   return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename T>
bool RedBlackTree<T>::internal_remove(T const &val, boost::stm::transaction &t)
{
   raw_node *cur = root().get(t);

   // find node val in the tree
   while(!is_sentinel(cur))
   {
      // Should be impossible, assert instead?
      if (NULL == cur) throw boost::stm::aborted_transaction_exception("altered tree");
      if (cur->value() == val) break; // Value is found
      // Walk down the tree
      cur = cur->value() > val ? cur->left().get(t) : cur->right().get(t);
   }

   //--------------------------------------------------------------------------
   // if the node wasn't found, exit
   //--------------------------------------------------------------------------
   if (cur->value() != val) return false;

   //
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

      succ->color().set(cur->color().get(t),t);
   }
   if (eBlack == succ->color().get(t))
     internal_remove_help(succSuccLeftChild, t);

   // Mark for deletion on transaction success
   t.delete_memory(*cur);

   return true;
}

/*
 * Return the number of node in the tree
 */
template <typename T>
int RedBlackTree<T>::node_count()
{
   int ncount;
   atomic(t) {
     ncount = internal_node_count(root(), t);
   } end_atom
   return ncount;
}

/*
 * Return the number of node in the tree
 */
template <typename T>
int RedBlackTree<T>::internal_node_count(ptr_node const &cur, boost::stm::transaction &t)
{
   // Access to the node by reading
   raw_node const *curRead = cur.get(t);

   if (is_sentinel(curRead)) return 0;

   int i=1; // Count this node
   i += internal_node_count(curRead->left(),t);
   i += internal_node_count(curRead->right(),t);

   return i;
}

/*
 * Print a textual representation of the tree
 */
template <typename T>
void RedBlackTree<T>::print(std::ofstream &o)
{
   int i = 0;

   atomic(t) {
     o << "[ size: " << i << " ]" << endl;
     internal_print(i, kRootStr, root().get(t), o,t);
     o << endl;
     o.close();
   } end_atom // FIXME CATCH and clear "o"
}

/*
 * Print a textual representation of the tree
 */
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

}

#endif // RED_BLACK_TREEV2_H
