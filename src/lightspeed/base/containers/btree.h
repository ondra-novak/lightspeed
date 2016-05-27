/*
 * btree.h
 *
 *  Created on: 9. 4. 2016
 *      Author: ondra
 */
#include <assert.h>
#include <functional>
#include <algorithm>

#ifndef LIBS_LIGHTSPEED_SRC_LIGHTSPEED_BASE_CONTAINERS_BTREE_H_
#define LIBS_LIGHTSPEED_SRC_LIGHTSPEED_BASE_CONTAINERS_BTREE_H_

namespace LightSpeed {

template <class T, class Compare> class BTree;
template <class T, class Compare> class BTreeNode;
template <class T, class Compare> class BTreeIterator;

/**
The Generic templated B-tree

There are three classes related to the generic B-tree:
@li BTree, a class that encapsulates B-tree algorithms;
@li BTreeNode, which defines the structure of a node in a B-tree;
@li BTreeIterator, an object that  allows  inspection of  the items
in a B-tree in ascending order;

The  algorithms implemented  here assume that items are stored in the
internal nodes  as  well as in  the leaves;  the definitions of the
B-tree concepts are based on the ones in Cormen, Leiserson and Rivest's
book Introduction to Algorithms.

The BTreeNode class encapsulates a single node of the Generic
B-tree. It is intended primarily for use by the NodeSpace, not by the
user of the GenericBTree class.
*/


template <class T, class Compare=std::less<T> >
class BTreeNode
{
protected:

  short               _keyCount;	///< # keys in node
  bool                _isLeaf;		///< Is this node a leaf?
  long                _subtreeSize;	///< # keys in subtree rooted at this node

  short               _order;

  T * _item;		    		///< Vector of items of type T
  BTreeNode<T,Compare> **_subtree;		///< Vector of subtrees
  BTree<T,Compare>    &_owner;

  friend BTree<T,Compare>;
  friend BTreeIterator<T,Compare>;

public:

  // ------------------ Access and Manipulation -----------------

  /// Return the number of items in this node.
  long Size() const {return _keyCount;};

  /// Return the "i"-th item.
  /** The value "i" must be such that 0 <= i < {\tt Size()}.*/
  T Item (short i) const {return _item[i];};

  /// Return the handle of the "i"-th subtree.
  /** The value "i" must be such that
  0 <= i <= {\tt Size()}. */
  BTreeNode *Subtree (short i) const {return _subtree[i];}

  /// Return the number of keys in the subtree rooted at this node.
  /** This
  method consults an instance variable, and therefore takes constant
  time; it does not need to  traverse the subtree.*/
  long SubtreeSize() const {return _subtreeSize;}

  /// Search the node for the given key;
  /**return greatest "i" such that
  {\tt key[i]} <= {\tt key}. Return true if {\tt key[i] = key}, false
  otherwise.*/
  bool Search (const T &itm, short& index) const;

  // --------------------- End public protocol -----------------------


  // ---------------- Construction and destruction ---------------

  BTreeNode (BTree<T,Compare> &owner, short order, bool leaf)
    : _keyCount(0),
    _isLeaf(leaf),
    _subtreeSize(0),
    _order(order),
    _item(reinterpret_cast<T *>(operator new(2 * order * sizeof(T) ))),
    _subtree(leaf?0:new BTreeNode<T,Compare>*[2 * order]),
    _owner(owner)
  {
//    memset(_subtree,0,sizeof(BTreeNode<T,Compare>*)*(2 * order));
  }

  BTreeNode (BTree<T,Compare> &owner, const BTreeNode<T,Compare> &other)
    : _keyCount(other._keyCount),
    _isLeaf(other._isLeaf),
    _subtreeSize(other._subtreeSize),
    _order(other._order),
    _item(new T[2 * other._order /*- 1*/]),
    _subtree(other._isLeaf?0:new BTreeNode<T,Compare>*[2 * other._order]),
    _owner(owner)
  {
    for (int i=0;i<(_keyCount);i++) new(_item+i) T(other._item[i]);
    if (!_isLeaf) {
    	for (int i=0;i<(_keyCount+1);i++)
    		_subtree[i]=other._subtree[i]?new BTreeNode(owner,*other._subtree[i]):0;
    }
  }

  // Constructor: create a node of the B-tree with given order.
  // The constructor is protected, because only BTrees may
  // create new nodes.

  ~BTreeNode()
  {
	  for (int i = 0; i < _keyCount; i++) _item[i].~T();
    operator delete(_item);
    if (!_isLeaf)
    {
      for (int i=_keyCount;i>=0;i--) delete _subtree[i];
    }
    delete [] _subtree;
  }
  // Destructor.

  static BTreeNode<T,Compare> **subtreeAlloc(int size)
  {
    return new BTreeNode<T,Compare>*[size];	// this will get the right (ie: local) new[] operator
  }
protected:
  void modified() {}
  // perform housekeeping when node is modified

  // Shift all the keys and subtrees, beginning at position {\tt pos}
  // right by the given amount. Note that the subtree to the left of
  // {\tt key[pos]} is {\it also\/} moved.
  void ShiftRightAt (short pos, short amount = 1);

  // Shift all the keys and subtrees, beginning at position {\tt pos},
  // left by the given amount. Note that the subtree to the left of
  // {\tt key[pos]} is {\it also\/} moved.
  void ShiftLeftAt (short pos, short amount = 1);


  // MoveSubNode: Move {\tt nkeys\/} keys, and their left and right
  // subtrees, beginning from position {\tt pos} in node "x" into this node
  // beginning at position {\tt ourPos}.
  void MoveSubNode (const BTreeNode& x, short pos, short ourPos, short nkeys);
};

template <class T, class Compare=std::less<T> >
class BTree
{
public:
  enum {max_btree_height=50};
  // --------------------- Construction and destruction ------------------

  /// Create a new B-tree of given order.
  /** Duplicate items are not
  allowed. The first parameter specifies the Comparator to be used when
  comparing two cells. The {\tt order} parameter must be at least 2;
  anything less than 2 is taken to be 2.

  The NodeSpace {\tt space} may by created by the derived
  class and
  passed to this constructor; if it is NULL, a default in-memory node
  space is created. If the derived class passes a non-null NodeSpace,
  it is the responsibility of the derived class to destroy the
  NodeSpace object.
  */
  BTree (short order = 40)
    : _order(order > 2?order:2),
    _root(NULL)
  {}

  /// Destructor: tells the NodeSpace to destroy all the nodes.
  ~BTree ()
  {
    delete _root;
  }

  BTree(const BTree &other):_order(other._order)
  {
    _root=(other._root?new BTreeNode<T,Compare>(*this,*other._root):0);
  }

  BTree<T,Compare> &operator=(const BTree &other)
  {
    delete _root;
    BTree<T,Compare> copy(other);
    _root=copy._root;
    copy._root=0;
    _order=copy._order;
    return *this;
  }

  // ----------------------- Search and related methods ------------------

  /// Search the tree for the given item.
  /** Return a pointer to the found item in situ if the search was successful.
  If the search fails, the return value is NULL.
  The algorithm used is a standard B-tree search algorithm that takes
  log_d n time in an n-item B-tree of order d.*/
  T *Find(const T &item) const;


  /// Find and return the minimum item.
  /**
  If the tree is empty,
  the null pointer is returned. The implementation simply returns the
  value {\tt ItemWithRank (0)}.
  */
  T *Smallest() const {return ItemWithRank (0);}

  /// Find and return the maximum item.
  /** If the tree is empty,
  the null pointer is returned. The implementation simply returns the
  value {\tt ItemWithRank (Size()-1)}.
  */
  T *Largest() const {return ItemWithRank (Size()-1);};


  /// Given an index "i" between 0 and {\tt Size()}-1, return the element of rank
  /** "i", i.e., the element that has "i" elements less than it in the tree.
  If i <= 0, this returns the smallest element, and if i >= {\tt
  Size()}, this returns the largest element. If the tree is empty,
  the null value of the base type is returned. The implementation
  examines only the nodes on the path from the root to the one
  containing the key sought, and therefore takes no more than log
  n time steps with "n" keys in the tree.

  @note Note that it is possible to iterate through the elements of the tree
  via calls to this method, varying the index from 0 to {\tt Size()}-1;
  however, this is much less efficient than using the BTreeIterator.*/
  T *ItemWithRank (long rank) const;

  /// Return the number of elements in the tree that are less than the parameter.
  long RankOf (T item) const;

  /// Return the size of the tree (number of items currently present).
  /** The implementation needs constant time regardless of tree size.*/
  long Size () const {return _root?_root->_subtreeSize:0;}


  // ------------------------ Modification ------------------------------

  /// Add the item to the tree.
  /** @return true if successfully added, false if the item was already in the tree. */
  bool Add(const T &item);

  /// Remove the specified item from the tree.
  /** @return NULL if the item was not found in the tree, and the found item otherwise.

  The implementation needs (in the worst case) two passes over the path
  to the key, and so takes 2log n time steps with "n" keys in the
  tree.

  It immediately coalesces any underflowing nodes along the path
  from the root to the deleted key.*/
  bool Remove (const T &key);

  /// Remove and return the smallest item in the tree.
  /**@return NULL if if the tree is empty.*/
  //T ExtractMin ();

  /// Removes all items from BTree
  void Clear() {delete _root;_root=NULL;}

  // --------------------- End public protocol -----------------------


protected:

  //------------------- protected helper methods ---------------------

  /// update subtree sizes along a search path
  void updSubtree(BTreeNode<T,Compare>** stack, int sp);

  enum DeleteActionEnum {NoAction, RotateLeft, RotateRight, Merge};

  /// Ensure that the node {\tt n1} is non-full, and recurse into it while inserting.
  bool _InsertNonFull (BTreeNode<T,Compare>* x, const T &item);

  void _SplitChild (BTreeNode<T,Compare>* x, short i, BTreeNode<T,Compare>*y);

  BTreeNode<T,Compare>* _DescendInto (BTreeNode<T,Compare>*node,
    short subtreeIndex,
    DeleteActionEnum& action);

  BTreeNode<T,Compare>* _Adjust (BTreeNode<T,Compare>* node, short index,
    BTreeNode<T,Compare>* c0,
    BTreeNode<T,Compare>* c1, DeleteActionEnum& action);

  //------------ Instance data -----------------------------

  short		_order;
  BTreeNode<T,Compare>	*_root;

  friend BTreeNode<T,Compare>;
  friend BTreeIterator<T,Compare>;


  BTreeNode<T,Compare>   *Root () const {return _root;}

  Compare less;

};

/** A search path is a sequence of pairs of the form <node#, subtree#>, with
the  first pair <root,  subtree#> and the   last pair being  of the form
<node#,  key#>. It completely   specifies the path   from the root  to a
particular key in the tree. */

template <class T, class Compare=std::less<T> >
class PathStruct
{
public:
  BTreeNode<T,Compare> *_handle;
  short              _indexInNode;
};

/** The BTreeIterator provides iteration over a BTree, with
the {\tt Reset}, {\tt Next} amd {\tt More} methods.

The  BTreeIterator  remembers and   manipulates  the  search path  to  a
particular key in the tree.

The Iterator maintains the invariant that the path specified by the
current values in the array represents the path to the key that was
returned by the most recent call to Next().*/

template <class T, class Compare=std::less<T> >
class BTreeIterator
{


public:

  /// Constructor: create a BTreeIterator for the given tree {\tt t}.
  BTreeIterator (const BTree<T,Compare>& tree)
    :_tree (tree)
  {
    _length = 0;
    Reset();
  }


  /// Copy constructor.
  /** The copy inspects the same B-tree as {\tt itr}, and
  (unless reset) begins  its iteration at the item at which {\tt itr}
  is currently positioned.*/
  BTreeIterator (const BTreeIterator<T,Compare>& itr)
    : _tree (itr._tree)
  {
    _length = itr._length;
    for (short i = 0; i < _length; i++)
      _path[i] = itr._path[i];
    _index = itr._index;
  }


  /// Destructor.
  ~BTreeIterator()
  {
  }

  /// Reset the iterator to the leftmost (smallest) item.
  void Reset()
  {
    _length = 1;
    PathStruct<T,Compare>* path = _path;
    path[0]._handle = _tree.Root();
    path[0]._indexInNode = -1;
    _index = -1;
  }

  /// Begin the iteration from the given item.
  /** The next call to {\tt Next}
  will return the given item (or the one immediately
  larger, if the given item isn't in the tree).*/
  void BeginFrom (const T &item)
  {
    short pos;
    bool found;


    _length = 0;
    _index  = -1;
    if (_tree.Size() <= 0)
      return;

    PathStruct<T,Compare>* path = _path;

    BTreeNode<T,Compare>* tmp_ptr, *p;
    tmp_ptr = _tree.Root();
    do
    {
      found = tmp_ptr->Search (item, pos);
      path[_length]._handle = tmp_ptr;
      _index += path[_length]._indexInNode = found ? pos : pos+1;
      _length++;
      if (tmp_ptr->_isLeaf) break;
      for (long i = 0; i <= pos; i++)
      {
        BTreeNode<T,Compare>* p = tmp_ptr->_subtree[i];
        _index += p->_subtreeSize;
      }
      if (found) break;
      tmp_ptr = tmp_ptr->_subtree [pos+1];
    } while (1);

    if (!tmp_ptr->_isLeaf)
    {
      // We're in an internal node; so move down to the leaf
      p = tmp_ptr->_subtree[pos];
      do
      {
        path[_length]._handle = p;
        path[_length]._indexInNode = p->_keyCount;
        _length++;
        p = p->_subtree[p->_keyCount]; // Rightmost subtree
      } while (p);
    }
    path[_length-1]._indexInNode--;
    // So that the first call to Next gives
    // the nearest key >= the given key
  }


  /// Tell whether there are more items in the iteration.
  bool More() const {return _index < _tree.Size()-1;}

  /// Return the next item in the iteration sequence.
  /** @return the NULL pointer if no more items.*/
  T *Next()
  {
    if (_index >= _tree.Size())
      return 0;

    if (_length == 0)
      return (T*)0;

    PathStruct<T,Compare>* path = _path;
    BTreeNode<T,Compare>* node = path[_length-1]._handle;
    short  ndx = path[_length-1]._indexInNode;

    _index++;
    if (node==NULL) return 0;
    if (! node->_isLeaf)
    {
      // Move to the next right subtree
      path[_length-1]._indexInNode++;
      BTreeNode<T,Compare> *handle = node->_subtree[ndx+1];
      if (!node->_isLeaf) {
    	  for(;;) {
    	        path[_length]._handle = handle;
    	        path[_length]._indexInNode = 0;
    	        _length++;
    	        node = handle;
    	        if (node->_isLeaf) break;
    	        handle = node->_subtree[0];
    	  }
      }
      return node->_item;
    }

    // We're in a leaf
    if (ndx >= node->_keyCount-1)
    {
      // We're at far right of the leaf, so move up
      do
      {
        _length--;
        if (_length <= 0) break;
        node = path[_length-1]._handle;
        ndx = path[_length-1]._indexInNode;
      } while (ndx >= node->_keyCount);

      T *retVal;
      if (_length)
      {
        retVal = node->_item + ndx;
      }
      else
        retVal = NULL;
      return retVal;
    }

    // We're in the middle or at left end of a leaf
    path[_length-1]._indexInNode++;
    return node->_item + path[_length-1]._indexInNode;
  }

  /// Return the previous item in the iteration sequence.
  /** @return the NULL pointer if no more items.
    @note Before you start backward enumeration, call Next immediatelly after BeginFrom.
      Function BeginFrom excepted forward enumeration, so it places pointer before
      first item. Previous without Next will cause that you will get second item
      before item specified by BeginFrom. If you want to get the first item on backward
      enumeration, you have to also use Next, but check, whether the returned item is equal
      to item specifed as parameter in BeginFrom. If returned item is not equal, you will
      get the first item after first Previous command. Otherwise (if item is equal),
      the first item is item returned by Next command.
  */
  T *Previous()
  {
    if (_index <= 0)
      return 0;

    if (_length==0)
    {
      _path[0]._handle=_tree._root;
      _path[0]._indexInNode=_tree._root->_keyCount+1;
      _length++;
    }

    PathStruct<T,Compare>* path = _path;
    BTreeNode<T,Compare>* node = path[_length-1]._handle;
    short  ndx = path[_length-1]._indexInNode;


    _index--;
    if (node==NULL) return 0;
    if (! node->_isLeaf)
    {
      // Move to the next left subtree
      path[_length-1]._indexInNode--;
      BTreeNode<T,Compare> *handle = node->_subtree[ndx-1];
      while (!node->_isLeaf)
      {
        path[_length]._handle = handle;
        path[_length]._indexInNode = handle->_keyCount;
        _length++;
        node = handle;
        handle = node->_subtree[node->_keyCount];
      }
      path[_length-1]._indexInNode--;
      return node->_item+node->_keyCount-1;
    }

    // We're in a leaf
    if (ndx <= 0)
    {
      // We're at far left of the leaf, so move up
      do
      {
        _length--;
        if (_length <= 0) break;
        node = path[_length-1]._handle;
        ndx = path[_length-1]._indexInNode;
      } while (ndx <= 0);

      T *retVal;
      if (_length)
      {
        retVal = node->_item + ndx-1;
      }
      else
        retVal = NULL;
      return retVal;
    }

    // We're in the middle or at left end of a leaf
    path[_length-1]._indexInNode--;
    return node->_item + path[_length-1]._indexInNode;
  }


  /// Return the rank of the element that was returned by the most recent call to {\tt Next()}.
  long CurrentRank () const {return _index;}

  const BTree<T,Compare> &GetTree() const {return _tree;}

  // --------------------- End public protocol -----------------------


protected:
  PathStruct<T,Compare> _path[BTree<T,Compare>::max_btree_height];		// Stack containing path to current element
  short         _length;	// Length of stack
  long          _index;		// Rank of  element most recently returned by Next
  const      BTree<T,Compare>& 	_tree;  // The tree being inspected
};

/// Search the node for the given key;
/** @return greatest "i" such that
{\tt key[i]} <= {\tt key}. Return true if {\tt key[i] = key}, false
otherwise.*/
template <class T, class Compare>
bool BTreeNode<T,Compare>::Search (const T &itm, short& index) const
{
  if (!_item)
    return false;

  short i;
  if (_keyCount <= 7) { // Small number of keys, do a linear search
    if (_keyCount == 0)
    {
      index = -1;
      return false;
    }
    for (i = 0; i < _keyCount && _owner.less(_item[i],itm); i++)
    {
/*      if (!_owner.less(_item[i],itm))
      {
        break;
      }*/
    }
    if (i<_keyCount && !_owner.less( itm, _item[i]))
    {
      index = i;
      return true;
    }
    else
    {
      index = i-1;
      return false;
    }
  }

  // Do a binary search
  short lo = 0, hi = _keyCount, mid=0;
  while (lo < hi)
  {
    mid = (lo + hi)/2;
    if (_owner.less(itm,_item[mid])) {
    	hi = mid;
    	index = mid-1;

    } else if (_owner.less(_item[mid],itm)) {
    	lo = mid+1;
    	index = mid;
    } else {
    	index = mid;
    	return true;
    }
  }
//  index = (cmp <= 0) ? (mid) :  mid-1;
  return false;
}

/** Shift all the keys and subtrees, beginning at position {\tt pos}
right by the given amount. Note that the subtree to the left of
{\tt key[pos]} is {\it also\/} moved.*/
template <class T, class Compare>
void BTreeNode<T,Compare>::ShiftRightAt (short pos, short amount)
{
  short i;
  for (i = _keyCount-1; i >= pos; i--)
  {
	move(_item+i, _item+i+amount);
//    _item[i+amount] = _item[i];
//    _subtree[i+amount+1] = _subtree[i+1];
  }
  if (!_isLeaf) {
	  for (i = _keyCount-1; i >= pos; i--)
		    _subtree[i+amount+1] = _subtree[i+1];
	  _subtree [pos+amount] = _subtree[pos];
  }
/*  for (i = pos; i < pos+amount; i++)
  {
    _owner.UnsetItem<T>(_item[i]);
  }*/
}

/** Shift all the keys and subtrees, beginning at position {\tt pos},
left by the given amount. Note that the subtree to the left of
{\tt key[pos]} is {\it also\/} moved.*/
template <class T, class Compare>
void BTreeNode<T,Compare>::ShiftLeftAt (short pos, short amount)
{
  short i;
  for (i = pos; i < _keyCount; i++)
  {
	  move(_item+i, _item+i-amount);
//    _item[i-amount] = _item[i];
  }
  if (!_isLeaf) {
	  for (i = pos; i < _keyCount; i++)
	  {
		    _subtree[i-amount] = _subtree[i];
	  }
	  // Now move the rightmost subtree
	  _subtree [_keyCount-amount] = _subtree[_keyCount];
	  for (i = _keyCount-amount+1; i <= _keyCount; i++)
	    _subtree[i] = 0;
  }
/*  for (i = _keyCount-amount; i < _keyCount; i++)
    _owner.UnsetItem(_item[i]);
  _owner.UnsetItem(_item[i]);*/
  _keyCount -= amount;
}

/** MoveSubNode: Move {\tt nkeys\/} keys, and their left and right
subtrees, beginning from position {\tt pos} in node "x" into this node
beginning at position {\tt ourPos}.*/
template <class T, class Compare>
void BTreeNode<T,Compare>::MoveSubNode (const BTreeNode& x, short pos, short ourPos, short nkeys)
{
  short i, j;
  for (i = ourPos, j = pos; i < ourPos + nkeys; i++, j++)
  {
	  move(x._item+j, _item+i);
  }
  if (!_isLeaf) {
	  for (i = ourPos, j = pos; i < ourPos + nkeys; i++, j++) {

		  //    _item[i] = x._item[j];
		_subtree[i] = x._subtree[j];
		//_owner.UnsetItem(x._item[j]);
		x._subtree[j]=0;
	  }
    _subtree[ourPos+nkeys] = x._subtree[pos + nkeys];
    x._subtree[pos + nkeys] = 0;
  }
}

/// Search the tree for the given item.
/** @Return a pointer to the found item in situ if the search was successful.
If the search fails, the return value is NULL.

The algorithm used is a standard B-tree search algorithm that takes
log_d n time in an n-item B-tree of order d.*/
template <class T, class Compare>
T *BTree<T,Compare>::Find(const T &item) const
{
  short pos;
  bool found = false;
  T *ret_val = NULL;

  BTreeNode<T,Compare>* current = _root;
  while (current)
  {
    found = current->Search (item, pos);
    if (found || current->_isLeaf) break;
    current = current->_subtree [pos+1];
  };

  if (found)
    ret_val = current->_item +pos;
  return ret_val;
}

/// Given an index "i" between 0 and {\tt Size()}-1.
/** @return the element of rank

"i", i.e., the element that has "i" elements less than it in the tree.
If i <= 0, this returns the smallest element, and if i >= {\tt
Size()}, this returns the largest element. If the tree is empty,
the null value of the base type is returned. The implementation
examines only the nodes on the path from the root to the one
containing the key sought, and therefore takes no more than log
n time steps with "n" keys in the tree.

@note Note that it is possible to iterate through the elements of the tree
via calls to this method, varying the index from 0 to {\tt Size()}-1;
however, this is much less efficient than using the BTreeIterator.*/
template <class T, class Compare>
T *BTree<T,Compare>::ItemWithRank (long rank) const
{
  BTreeNode<T,Compare>* tmp_ptr, *p1;
  tmp_ptr = _root;
  if (!tmp_ptr || tmp_ptr->_keyCount <= 0)
    return NULL;

  rank=std::max<long>(rank,0);
  rank=std::min<long>(rank,tmp_ptr->_subtreeSize-1);
  //  rank = (rank >? 0) <? (tmp_ptr->_subtreeSize-1);
  do
  {
    if (tmp_ptr->_isLeaf)
    {
      assert ((0 <= rank) && (rank <= tmp_ptr->_keyCount-1));
      //		("Internal error: CL_GenericBTree::ItemWithRank:"
      //	 "bad key count %d rank %ld", tmp_ptr->_keyCount, rank));
      return tmp_ptr->_item+rank;
    }

    // We're in a non-leaf, so find the subtree to descend into
    // (if any)
    short i;
    for (i = 0; i < tmp_ptr->_keyCount; i++)
    {
      p1 = tmp_ptr->_subtree[i];
      if (p1->_subtreeSize > rank)
        break;
      rank -= p1->_subtreeSize; // Account for i-th subtree

      if (rank == 0)
      {
        // We've got the item we wanted
        return tmp_ptr->_item+i;
      }
      rank--;               // Account for i-th key in node
    }

    if (i >= tmp_ptr->_keyCount)
    {
      // Descend into rightmost subtree
      p1 = tmp_ptr->_subtree[i];
    }
    tmp_ptr = p1;
  } while (1);
}

/// Return the number of elements in the tree that are less than the parameter.
template <class T, class Compare>
long BTree<T,Compare>::RankOf (T item) const
{
  short pos;
  bool found;
  long count = 0;

  BTreeNode<T,Compare>* tmp_ptr, *p1;
  tmp_ptr = _root;
  if (!tmp_ptr || tmp_ptr->_keyCount <= 0)
    return 0;

  do
  {
    found = tmp_ptr->Search (item, pos);
    if (tmp_ptr->_isLeaf)
    {
      count += found ? pos : pos+1;
      return count;
    }

    // We're in a non-leaf, so find the subtree to descend into
    short i;
    for (i = 0; i <= pos; i++)
    {
      p1 = tmp_ptr->_subtree[i];
      count += p1->_subtreeSize; // Account for i-th subtree
    }

    if (found)
    {
      return count + pos;
    }

    count += pos+1; // Account for the keys we compared
    p1 = tmp_ptr->_subtree[i];
    tmp_ptr = p1;
  } while (1);
}

// Add the item to the tree.
/** @Return true if successfully added, false if the item was already in the tree.*/
template <class T, class Compare>
bool BTree<T,Compare>::Add(const T &item)
{
  bool        ans;
  BTreeNode<T,Compare>* aNode;
  if (!_root)
  {
    _root = new BTreeNode<T,Compare>(*this,_order,true);
  }
  if (_root->_keyCount < (2*_order - 1))
  {
    ans = _InsertNonFull (_root, item);
    return ans;
  }

  // Root is full; create a new empty root
  aNode = new BTreeNode<T,Compare>(*this,_order,false); // aNode  will be the new root
  aNode->_subtree [0] = _root;
//  aNode->_isLeaf = false;
  aNode->_subtreeSize = _root->_subtreeSize;
  _SplitChild (aNode, 0, _root);

  _root = aNode;

  // Now add the key
  ans = _InsertNonFull (aNode, item);
  return ans;
}

/// Remove the specified item from the tree.
/** @return NULL if the item was not found in the tree, and the found item otherwise.

The implementation needs (in the worst case) two passes over the path
to the key, and so takes 2log n time steps with "n" keys in the
tree.

It immediately coalesces any underflowing nodes along the path
from the root to the deleted key.
*/
template <class T, class Compare>
bool BTree<T,Compare>::Remove (const T &key)
{
  BTreeNode<T,Compare>* root = _root;
  BTreeNode<T,Compare>* node = root;
  bool retVal = false;
  short sp, index;
  bool found;

  if (!node || node->_keyCount == 0) // Empty root
  {
    return false;
  }
  /*
  if (node->_keyCount == 1 && node->_isLeaf)
  {
    // Tree has only one key
    if (Cmp(key, node->_item[0])==0)
    {
      node->_keyCount = 0;
      node->_subtreeSize = 0;
      return node->_item[0];
    }
    T item;
    Compare::UnsetItem(item);
    return item;
  }
*/
  BTreeNode<T,Compare>* stack[max_btree_height];

  // We need a stack for updating the subtree sizes
  sp = 0;
  index = 0;
  found = false;

  BTreeNode<T,Compare>* q;
  // stack[sp++] = node;
  enum {SEARCHING, DESCENDING} state = SEARCHING;
  DeleteActionEnum action;
  while (1)
  {
    if (state == SEARCHING)
    {
      found = node->Search (key, index);
      if (found) retVal = true;
    }
    q = _DescendInto (node, index+1, action);
    if (node == root &&  node->_keyCount == 0)
    {
      delete node;
      root=NULL;
    }
    else
    {
      // We should add the root to the stack only if it wasn't
      // already destroyed
      stack [sp++] = node;
    }
    if (!q) break;

    // _DescendInto may have caused our key to be copied into q.
    // If so, it would be copied into either q->_item[0] or
    // q->_item[_order-1]  (because of a right or left rotation,
    // respectively) or into q->_item[_order-1] (because of a merge).
    if (found)
    {
      state = DESCENDING;
      if (action == RotateRight)
      {
        index = 0;
      }
      else if (action == RotateLeft || action == Merge)
      {
        index = _order-1;
      }
      else // No rotation or merge was done
        break;
    }
    node = q;
  }
  if (!found) return false;
	node->_item[index].~T();
  if (node->_isLeaf)
  {
    // Key found in leaf
    node->ShiftLeftAt (index+1);
  }
  else
  {
    // The key is in an internal node, so we'll replace it by its
    // inorder successor:
    BTreeNode<T,Compare>* p = q;
    while (1)
    {
      stack[sp++] = p;
      if (p->_isLeaf) break;
      p = _DescendInto (p, 0, action);
    }
    move(p->_item, p->_item+index);
//    node->_item[index] = p->_item[0];
    p->ShiftLeftAt(1);
  }

  updSubtree(stack, sp);

  return retVal;
}

#if 0
/// Remove and return the smallest item in the tree.
/** @return NULL if the tree is empty.*/
template <class T, class Compare>
T BTree<T,Compare>::ExtractMin ()
{
  BTreeNode<T,Compare>* stack[max_btree_height];
  // We need a stack for updating the subtree sizes
  short sp = 0;
  BTreeNode<T,Compare>* node = _root;
  if (node->_keyCount == 0)
    return (T)0;

  stack[sp++] = node;
  DeleteActionEnum action;
  while (!node->_isLeaf)
  {
    node = _DescendInto (node, 0, action);
    stack[sp++] = node;
  }
  T item = node->_item[0];
  node->ShiftLeftAt(1);
  for (short i = 0; i < sp; i++)
  {
    stack[i]->_subtreeSize--;
  }
  return item;
}
#endif

/// update subtree sizes along a search path
template <class T, class Compare>
void BTree<T,Compare>::updSubtree(BTreeNode<T,Compare>** stack, int sp)
{
  short i = 0;
  if (stack[0]->_keyCount == 0)
  {
    i = 1;
  }
  for (; i < sp; i++)
  {
    stack[i]->_subtreeSize--;
  }
}

/// Ensure that the node {\tt n1} is non-full, and recurse into it while inserting.
template <class T, class Compare>
bool BTree<T,Compare>::_InsertNonFull (BTreeNode<T,Compare>* x, const T &item)
{
  short pos = 0;
  BTreeNode<T,Compare>* y, *z = x;
  BTreeNode<T,Compare>* stack[max_btree_height];
  // We need a stack for updating the subtree sizes

  short sp = 0;
  bool found = false;

  while (z && !(found = z->Search (item, pos)))
  {
    stack[sp++] = z;
    if (z->_isLeaf) break;
    pos++;
    y =  z->_subtree[pos];
    if (y->_keyCount == 2*_order-1)
    {
      _SplitChild (z, pos, y);
      if (!less(item,z->_item[pos]))
      {
        pos++;
        y = z->_subtree[pos];
      }
    }
    z = y;
  }

  if (!found)
  {
    short n = z->_keyCount;
    if (n > 0)
    {
      z->ShiftRightAt (pos+1);
      new(z->_item+pos+1) T(item);
    }
    else
      new(z->_item) T(item);
    z->_keyCount++;
    for (short i = 0; i < sp; i++)
    {
      stack[i]->_subtreeSize++;
    }
  }
  return !found;
}

template <class T, class Compare>
void BTree<T,Compare>::_SplitChild (BTreeNode<T,Compare>* x, short i, BTreeNode<T,Compare>*y)
{
  BTreeNode<T,Compare>* z = new BTreeNode<T,Compare>(*this,_order,y->_isLeaf);
  z->MoveSubNode (*y, _order, 0, _order-1);
  z->_keyCount = y->_keyCount = _order-1;
  x->ShiftRightAt (i);
  // We shouldn't shift subtree[i], but it shouldn't matter

  x->_subtree[i+1] = z;
  move(y->_item+_order-1, x->_item+i);
//  x->_item [i] = y->_item [_order-1];
  x->_keyCount++;

  // Recompute _subtreeSize for y and z
  long size = 0;
  if (!z->_isLeaf)
  {
    for (short j = 0; j <= z->_keyCount; j++)
    {
      BTreeNode<T,Compare>* p = z->_subtree[j];
      size += p->_subtreeSize;
    }
  }
  size += z->_keyCount;
  z->_subtreeSize = size;
  y->_subtreeSize -= size+1;
  y->modified ();
  x->modified ();
}

template <class T, class Compare>
BTreeNode<T,Compare>* BTree<T,Compare>::_DescendInto (BTreeNode<T,Compare>*node,
                                                      short subtreeIndex,
                                                      DeleteActionEnum& action)
{
	if (node->_isLeaf) {
		action = NoAction;
		return 0;
	}
  BTreeNode<T,Compare>* child, *sibling, *p;
  child = node->_subtree[subtreeIndex];
  if (!child || child->_keyCount >= _order)
  {
    action = NoAction;
    return child;
  }
  if (subtreeIndex == 0)
  {
    sibling = node->_subtree[1];
    p = _Adjust (node, 0, child, sibling, action);
  }
  else
  {
    sibling = node->_subtree[subtreeIndex-1];
    p = _Adjust (node, subtreeIndex-1, sibling, child, action);
  }
  return p;
}

template <class T, class Compare>
BTreeNode<T,Compare>* BTree<T,Compare>::_Adjust (BTreeNode<T,Compare>* node, short index,
                                                 BTreeNode<T,Compare>* c0,
                                                 BTreeNode<T,Compare>* c1, DeleteActionEnum& action)
{
  assert ((c0 != NULL) && (c1 != NULL));
  // ("BTree::Adjust: assertion failed: line %d\n", __LINE__));
  assert ((c0->_keyCount == _order-1) || (c1->_keyCount == _order-1));
  // ("BTree::Adjust: assertion failed: line %d\n", __LINE__));

  if (c0->_keyCount == _order-1 && c1->_keyCount == _order-1)
  {
    // Merge the two nodes
	move(node->_item+index, c0->_item+_order-1);
//    c0->_item[_order-1] = node->_item[index];
    c0->MoveSubNode (*c1, 0, _order, _order-1);
    c0->_keyCount = 2*_order-1;
    c0->_subtreeSize += c1->_subtreeSize+1;

    c1->_keyCount = 0;
    delete c1;
    if (node->_keyCount > 1)
    {
      node->ShiftLeftAt (index+1);
      node->_subtree[index] = c0;
    }
    else
    {
      _root = c0;
      node->_keyCount--;
      node->_subtree[index]=NULL;
    }
    action = Merge;
    return c0;
  }

  if (c0->_keyCount >= _order)
  {
    // Rotate right
    c1->ShiftRightAt (0);
    //    c1->_item[0] = node->_item[index];
    move(node->_item+index, c1->_item);
    c1->_subtree[0] = c0->_subtree[c0->_keyCount];
    //node->_item[index] = c0->_item[c0->_keyCount-1];
    move(c0->_item+(c0->_keyCount-1), node->_item);
    c0->_keyCount--;
    c1->_keyCount++;
    BTreeNode<T,Compare>* p = c1->_subtree[0];
    long xfr = (p) ? p->_subtreeSize+1 : 1;
    c1->_subtreeSize += xfr;
    c0->_subtreeSize -= xfr;
    c0->modified ();
    action = RotateRight;
    return c1;
  }
  else
  {
    // c1->keyCount >= order, so rotate left
    c0->_item[_order-1] = node->_item[index];
    c0->_subtree[_order] = c1->_subtree[0];
    c0->_keyCount++;
    node->_item[index] = c1->_item[0];
    BTreeNode<T,Compare>* p = c0->_subtree[_order];
    long xfr = (p) ? p->_subtreeSize+1 : 1;
    c1->_subtreeSize -= xfr;
    c0->_subtreeSize += xfr;
    c1->ShiftLeftAt(1);
    c1->modified ();
    action = RotateLeft;
    return c0;
  }
}





}



#endif /* LIBS_LIGHTSPEED_SRC_LIGHTSPEED_BASE_CONTAINERS_BTREE_H_ */
