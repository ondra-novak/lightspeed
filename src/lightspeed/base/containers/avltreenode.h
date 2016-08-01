/*
 * avlnode.h
 *
 *  Created on: 8.9.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_AVLNODE_H_
#define LIGHTSPEED_CONTAINERS_AVLNODE_H_


#include "../memory/factory.h"
#include "../iter/iterator.h"
#include "../memory/pointer.h"
#include <utility>


namespace LightSpeed {


template<typename T>
struct AvlTreeNodeTraits;
template<typename T, typename Traits = AvlTreeNodeTraits<T> >
class AvlTreeNode;


template<typename T>
struct AvlTreeNodeTraits {
	typedef AvlTreeNode<T> *Link;
};
///Instance of AVL tree node
/**
 * This class is used to construct AVL Binary search tree.
 * It can be used standalone to constructs special AVL trees.
 * It is also used in AVLTree class
 *
 * Node doesn't contain any data. It is only pair of pointers. You
 * have to extend node with data and also define ordering operator
 * which is used with some functions declared in this class
 *
 */

template<typename T, typename Traits>
class AvlTreeNode {
public:

	AvlTreeNode(const T &val):left(0),right(0),balance(0),dirty(true),data(val) {
	}

#ifdef LIGHTSPEED_ENABLE_CPP11
	AvlTreeNode(T &&val):left(0),right(0),balance(0),dirty(true),data(std::move(val)) {
	}
#endif

	typedef AvlTreeNode *PNode;
	typedef typename Traits::Link Link;

	///Implements node iterator to walk over nodes in-order
	class Iterator: public IteratorBase<PNode, Iterator> {
	public:


		Iterator():pathlen(0),dir(0) {}
		///Constructs in-order iterator from given root
		/**
		 *
		 * @param root root node.
		 * @param dir direction of walking. It can be
		 * 		Direction::forward or Direction::backward.
		 */
		Iterator(PNode root, Direction::Type dir);

		///Constructs in-order iterator as a copy of another iterator
		/**
		 *
		 * @param other source iterator, which internal state will be used
		 * @param d defines new direction. Default value causes, that direction will not be changed
		 */
		Iterator(const Iterator &other,
					Direction::Type d = Direction::undefined);
		///Constructs in-order iterator as result of search the node
		/**
		 * @param cmp ordering operator similar to std::less
		 * @param root root of tree
		 * @param search node to search, doesn't need to be part of tree
		 * @param dir direction of final iterator
		 * @param found optional parameter can contain address, where
		 * 		will be stored search state. It is true, when item
		 * 		has been found exactly. When false is stored, iterator
		 * 		is positioned on the first item greater then requested item
		 */
		template<typename Cmp>
		Iterator(const Cmp &cmp, const AvlTreeNode *root,
					const AvlTreeNode *search,
				Direction::Type dir, bool *found = 0);

		///Constructs in-order iterator as result of search the node
		/**
		 * @param cmp ordering operator similar to std::less
		 * @param root root of tree
		 * @param search item to search
		 * @param dir direction of final iterator
		 * @param found optional parameter can contain address, where
		 * 		will be stored search state. It is true, when item
		 * 		has been found exactly. When false is stored, iterator
		 * 		is positioned on the first item greater then requested item
		 */
		template<typename Cmp>
		Iterator(const Cmp &cmp, const AvlTreeNode *root, const T &search,
				Direction::Type dir, bool *found = 0);


		///Creates iterator using path
		/**
		 *
		 * @param path pointer to path
		 * @param len length of path.
		 * @param d direction for this iterator
		 */
		Iterator(PNode *path, natural len, Direction::Type d);

		///Creates iterator initializing at given index position
		/**
		 * @param root root of tree
		 * @param pos position as index. Position is aproximated and it is
		 * 	compatible to return value getPosition()
		 * @param d direction set to iterator
		 */
		Iterator(AvlTreeNode *root, natural pos, Direction::Type d = Direction::forward);

		///allows change iterator direction
		/**
		 * @param d new direction. There can be forward or backward
		 * @retval true direction set
		 * @retval false invalid direction
		 *
		 * @note iterator will not change its position. After changing
		 * direction, peek() and getNext() will still return item
		 * in previous directio, because this item is already ready. After
		 * getNext() or skip() iterator will move its position in the new
		 * direction. You cannot change direction, when hasItems() returns false
		 */


		bool setDir(Direction::Type d);
		///Retrieves current direction
		Direction::Type getDir() const;

		bool hasItems() const {return pathlen > 0;}
		const PNode &peek() const;
		const PNode &getNext();
		bool equalTo(const Iterator &other) const;
		bool lessThan(const Iterator &other) const;

		///retrieves position of iterator as index from the beginning
		/**
		 * This allows to count items between current position and beginning.
		 * You can easy calculate distance between two iterators.
		 *
		 * Note that position is approximated to retrieve result faster.
		 * To get exact position takes O(n) time complexity. This function
		 * takes O(log n) complexity, but sometimes can return more items
		 * then it is really it. Function assumes, that tree is perfectly
		 * balanced, but condition is not offten reached perfectly.
		 *
		 * @return count of items between begin and current position
		 */
		natural getPosition() const;

		///returns depth of iterator - depth in tree structure
		natural getMaxDepth() const;

		///Retrieves node at given depth
		/**
		 * Use this to explore nodes in the way to the selected item
		 *
		 * @param depth depth must be between 0 and getMaxDepth()
		 * @return node at given depth
		 */
		const AvlTreeNode *getNode(natural depth) const;

	protected:
		static const natural maxPathLen = 64;
		PNode path[maxPathLen]; //should be enough 2^64 nodes
		Bin::natural8 pathlen;
		Bin::integer8 dir;
		mutable PNode retNode;

		void initPath(PNode root, Bin::natural8 pos = 0);

		template<typename Cmp>
		void initPathSearch(const Cmp &cmp, const AvlTreeNode *root,
				const AvlTreeNode *search, bool *found);
		PNode getCurNode() const {
			return path[pathlen - 1];
		}

		void goNext();


	};

	///Retrieves left node
	AvlTreeNode *getLeft() const {return left;}
	///Retrieves right node
	AvlTreeNode *getRight() const {return right;}

	Link &link(int lnk) {return lnk?right:left;}
	const Link &link(int lnk) const {return lnk?right:left;}




protected:

	///left and right side of tree
	Link left, right;
	///tree balance -2,-1,0,1,2
	Bin::integer8 balance;
public:
	///everytime is node chanted (changed link), this flag is set to true.
	bool dirty;
	///user payload
	T data;
protected:
	///Perform single rotation
	/**
	 * @param dir dir=0 left, dir=1 right
	 * @return new root after rotation.
	 *
	 * @note will not adjust balances
	 */
	AvlTreeNode *rotateSingle(int dir);
	///Perform double rotation
	/**
	 * @param dir dir=0 left, dir=1 right
	 * @return new root after rotation.
	 *
	 * @note will not adjust balances
	 */
	AvlTreeNode *rotateDouble(int dir);
	///adjusts balance for double rotation
	/**
	 * @param dir dir=0 left, dir=1 right
	 * @param bal initial balance
	 */
	void adjustBalance(int dir, int bal);
	///Balances node after insert
	/**
	 * @param dir dir=0 left, dir=1 right
	 * @return new root after balance
	 */
	AvlTreeNode *insertBalance(int dir);
	///Balances node after remove
	/**
	 * @param dir dir=0 left, dir=1 right
	 * @param done done flag reporting end of balancing
	 * @return new root after balance
	 */
	AvlTreeNode *removeBalance(int dir, bool &done);


	///Balances subtree after remove
	/**
	 * @param root root of tree
	 * @param done done flag
	 * @param dir direction of deleting
	 * @return new root of tree
	 */
	static AvlTreeNode *balanceAfterRemove(AvlTreeNode *root, bool &done, int dir);

	///Removes and takes rightmost item of subtree
	/**
	 * @param root root of subtree
	 * @param rm variable that receives pointer to rightmost node
	 * @param done done flag used during rebalancing
	 * @return pointer to new root after node is removed (due rebalancing)
	 */
	static AvlTreeNode *takeRightMost(AvlTreeNode *root, AvlTreeNode *&rm, bool &done);

	///Removes root node from the subtree
	/**
	 * @param root root of subtree. This node will be also removed
	 * @return new root after removing
	 */
	static AvlTreeNode *removeRootNode(AvlTreeNode *root, bool &done);

	///Helps to manipulate with the path inside of iterator
	class PrivateIter: public AvlTreeNode::Iterator {
	public:

		//allocates new path item and returns pointer to
		PNode *allocPath() {
			PNode *ret = this->path+this->pathlen;
			if (this->pathlen < this->maxPathLen) this->pathlen++;
			return ret;
		}

		///removes allocated path
		void trimPath() {
			if (this->pathlen > 0) this->pathlen--;
		}

		//gets path at index
		PNode getPath(natural index) const {
			if (index >= this->pathlen) return 0;
			else return this->path[index];
		}

		//sets path at index
		void setPath(natural index, PNode node) {
			if (index > this->pathlen) return;
			if (node == 0) {
				this->pathlen = (Bin::natural8)index;
			} else {
				this->path[index] = node;
			}
		}
	};


	void resetLinks() {
		left = right = 0;
		balance = 0;
		dirty = true;
	}

	static const AvlTreeNode *getSearchNode(const T &value);

public:
	///Inserts new node into the tree, where this item is root
	/**
	 *
	 * @param cmp compare operator
	 * @param newNode new node inserting into the tree
	 * @param done done temporary variable used during inserting, MUST BE INITIALIZED TO FALSE
	 * @param path if used, function stores path to the new item into referred array. Array must be large
	 * 	enough. 64 items should be enough, because tree with 64 levels will have 2^64 items, which is
	 *  currently impossible to fit into any memory. Path is terminated by zero.
	 * @return pointer to new root. It can be this, if root doesn't changed.
	 */
	template<typename NodeCmp>
	AvlTreeNode *insert(const NodeCmp &cmp, AvlTreeNode *newNode,
			bool &done, Iterator *toStorePath = 0) {

		if (toStorePath && !toStorePath->hasItems()) {
			PNode *pathStore = toStorePath?
					static_cast<PrivateIter *>(toStorePath)->allocPath():0;
			PNode res = insert(cmp,newNode,done,toStorePath);
			if (res) *pathStore = res; else *pathStore = this;
			return res;
		}

		//if this is NULL, return newNode as new root
		if (checkNull(this)) {
			return newNode;
		}
		int dir;

		//select direction
		if (cmp(newNode,this)) dir = 0;
		else if (cmp(this,newNode)) dir = 1;
		else return 0;


		PNode *pathStore = toStorePath?
				static_cast<PrivateIter *>(toStorePath)->allocPath():0;
		//insert into subtree
		AvlTreeNode *nd = link(dir)->insert(cmp,newNode,done,toStorePath);
		//if NULL returned, item has not been inserted, because already exists
		if (nd == 0) {
			if (toStorePath)
				*pathStore = link(dir);
			return 0;
		}
		//update root
		link(dir) = nd;
		//make whole path from down to the top dirty
		dirty = true;

		if (pathStore) *pathStore = nd;

		//perform balancing
		if (!done) {
			balance += dir == 0?-1:1;
			if (balance == 0)
				done = true;
			else if (balance < -1 || balance > 1) {
				done = true;
				return insertBalance(dir);
			}
		}
		return this;
	}

	///Inserts new node into the tree, where this item is root
	/**
	 *
	 * @param cmp compare operator
	 * @param newNode new node inserting into the tree
	 * @param path if used, function stores path to the new item into referred array. Array must be large
	 * 	enough. 64 items should be enough, because tree with 64 levels will have 2^64 items, which is
	 *  currently impossible to fit into any memory. Path is terminated by zero.
	 * @return pointer to new root. It can be this, if root doesn't changed.
	 */
	template<typename NodeCmp>
	AvlTreeNode *insert(const NodeCmp &cmp, AvlTreeNode *newNode,
			Iterator *toStorePath = 0) {
		bool done = false;
		return insert(cmp,newNode,done,toStorePath);
	}


	///Removes node by value
	/**
	 * @param cmp compare operator
	 * @param toDel node to delete, not referenced, only passed to compare operator
	 * @param done done flag, must be set to false
	 * @param removedNode pointer to node, which has been removed
	 * @return new root after remove
	 */
	template<typename NodeCmp>
	AvlTreeNode *remove(const NodeCmp &cmp, const AvlTreeNode *toDel,
			bool &done, AvlTreeNode *& removedNode) {

		if (checkNull(this)) {
			removedNode = 0;
			done = true;
			return this;
		}
		int dir;
		if (cmp(toDel,this)) dir = 0;
		else if (cmp(this,toDel)) dir = 1;
		else {
			removedNode = this;
			AvlTreeNode *ret = removeRootNode(this,done);
			resetLinks();
			return ret;
		}

		link(dir) = link(dir)->remove(cmp,toDel,done,removedNode);
		//make whole path from down to the top dirty
		dirty = true;

		return balanceAfterRemove(this,done,dir);
	}

	///Removes node by value
	/**
	 * @param cmp compare operator
	 * @param toDel node to remove for comparsion
	 * @param removedNode node has been removed
	 * @return new root after removing
	 */
	template<typename NodeCmp>
	AvlTreeNode *remove(const NodeCmp &cmp, const AvlTreeNode *toDel,
					AvlTreeNode *& removedNode) {
		bool done = false;
		return remove(cmp,toDel,done,removedNode);
	}

	///Removes node by value
	/**
	 * @param cmp compare operator
	 * @param toDel node to remove for comparsion
	 * @param removedNode node has been removed
	 * @return new root after removing
	 */
	template<typename NodeCmp>
	AvlTreeNode *remove(const NodeCmp &cmp, const T &toDel,
					AvlTreeNode *& removedNode) {
		bool done = false;
		return remove(cmp,getSearchNode(toDel),done,removedNode);
	}

	///Removes node by path
	/**
	 * @param iter iterator which points to item to remove
	 * @param done done flag must be set to false
	 * @param pathIndex used during recursion, must be set to 0 on initializing
	 * @return new root after removing
	 *
	 * @note iterator is not modified! You have to reconstruct iterator state
	 * if the iterator. You can use iter.peek() to retrieve item to remove and
	 * after removing initialize iterator next or previous item after the removed
	 */
	AvlTreeNode *remove(const Iterator &iter, bool &done, natural pathIndex = 0);

	///Removes node by path
	/**
	 * @param iter iterator which points to item to remove.
	 * @param pathIndex used during recursion, must be set to 0 on initializing
	 * @return new root after removing
	 * @note iterator is not modified! You have to reconstruct iterator state
	 * if the iterator. You can use iter.peek() to retrieve item to remove and
	 * after removing initialize iterator next or previous item after the removed
	 */
	AvlTreeNode *remove(const Iterator &iter) {
		bool done = false;
		return remove(iter,done);
	}

	template<typename Eraser>
	void clear(const Eraser &eraser) {
		if (this->left) this->left->clear(eraser);
		if (this->right) this->right->clear(eraser);
		eraser(this);
	}


	///returns max height
	/** @note do not call on null pointer, it will not return zero */
	natural maxHeight() const;
	///returns aproximate count of items - always highter
	/**
	 * This is faster than count(), but can return greater number than reallity
	 * @note do not call on null pointer, it will not return zero */
	natural aproxCount() const;
	///returns count of items
	/**
	 * Function must process all nodes and count items in it. It has N complexity.
	 * It is better to count items inserted and deleted
	 *
	 * @note do not call on null pointer, it will not return zero */
	natural count() const;

	bool checkNull(AvlTreeNode *x) {
		return x == 0;
	}
};

template<typename NodeOrder, typename T, typename LinkT = AvlTreeNodeTraits<T> >
class AvlTreeBasic {
public:
	AvlTreeBasic(const NodeOrder &order)
		:tree(0),order(order),count(0) {}
	AvlTreeBasic()
		:tree(0),count(0) {}

	typedef typename AvlTreeNode<T, LinkT>::Iterator Iterator;


	Iterator insert(AvlTreeNode<T, LinkT> *nd, bool *exists = 0) {
		Iterator out;
		AvlTreeNode<T, LinkT> *x = tree->insert(order,nd,&out);
		if (exists) *exists = x == 0;
		if (x) {tree = x; count++;}
		return out;
	}

	AvlTreeNode<T, LinkT> *find(const AvlTreeNode<T, LinkT> &nd) const {
		bool found = false;
		Iterator out(order,tree,&nd,Direction::forward, &found);
		if (found) return out.getNext();
		else return 0;
	}

	Iterator seek(const AvlTreeNode<T, LinkT> &nd, Direction::Type dir, bool *found = 0) {
		return Iterator(order,tree,&nd,dir,found);
	}

	AvlTreeNode<T, LinkT> *remove(const AvlTreeNode<T, LinkT> &nd)  {
		AvlTreeNode<T, LinkT> *out;
		tree = tree->remove(order,&nd,out);
		count--;
		return out;
	}

	AvlTreeNode<T, LinkT> *remove(Iterator &iter) {
		bool done = false;
		AvlTreeNode<T, LinkT> *nd = iter.peek();
		tree = static_cast<AvlTreeNode<T, LinkT> *>(tree->remove(iter,done));
		count--;
		return nd;
	}

	bool empty() const {return tree == 0;}

	natural size() const {return count;}

	template<typename Eraser>
	void clear(const Eraser &eraser) {
		if (tree) {
			tree->clear(eraser);
			tree = 0;
		}
	}

	///Retrieves iterator to walk all items in order
	Iterator getFwIter() const {
		return Iterator(tree,Direction::forward);
	}

	///Retrieves iterator to walk all items in backward order
	Iterator getBkIter() const {
		return Iterator(tree,Direction::backward);
	}

protected:

	AvlTreeNode<T, LinkT> *tree;
	NodeOrder order;
	natural count;
};

template<typename T, typename Traits>
template<typename Cmp>
AvlTreeNode<T,Traits>::Iterator::Iterator(const Cmp &cmp,
			const AvlTreeNode *root, const AvlTreeNode *search,
		Direction::Type dir, bool *found):dir(0) {
	if (setDir(dir))
		initPathSearch(cmp, root,search, found);
	else
		pathlen = 0;
}

template<typename T, typename Traits>
template<typename Cmp>
AvlTreeNode<T,Traits>::Iterator::Iterator(const Cmp &cmp,
			const AvlTreeNode *root, const T &search,
		Direction::Type dir, bool *found):dir(0) {
	if (setDir(dir))
		initPathSearch(cmp, root, AvlTreeNode<T,Traits>::getSearchNode(search), found);
	else
		pathlen = 0;
}

template<typename T,typename Traits>
template<typename Cmp>
void AvlTreeNode<T,Traits>::Iterator::initPathSearch(const Cmp &cmp,
		const AvlTreeNode *root,
		const AvlTreeNode *search, bool *found) {

	pathlen = 0;
	int dir = 0;
	while (root) {
		path[pathlen ++] = const_cast<PNode>(root);
		if (cmp(search,root)) dir = 0;
		else if (cmp(root,search)) dir = 1;
		else {
			if (found) *found = true;
			return;
		}
		root = root->link(dir);
	}
	if (found) *found = false;
	if (dir != this->dir) goNext();
}

}

#endif /* AVLNODE_H_ */
