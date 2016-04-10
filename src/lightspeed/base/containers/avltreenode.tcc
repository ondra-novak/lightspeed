/*
 * avltreenode.tcc
 *
 *  Created on: 15.3.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_AVLTREENODE_TCC_
#define LIGHTSPEED_CONTAINERS_AVLTREENODE_TCC_


#include "avltreenode.h"

namespace LightSpeed {


template<typename T, typename LinkT>
AvlTreeNode<T, LinkT>::Iterator::Iterator(PNode root, Direction::Type dir) {
	if (setDir(dir))
		initPath(root);
	else
		pathlen = 0;
	retNode = 0;
}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT>::Iterator::Iterator(const Iterator &other, Direction::Type d)
	:pathlen(other.pathlen),dir(other.dir),retNode(other.retNode) {
		for (Bin::natural8 i = 0; i < pathlen;i ++)
			path[i] = other.path[i];
	setDir(d);
}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT>::Iterator::Iterator(PNode *path, natural len, Direction::Type d)
	:dir(0),retNode(0)
{
	if (len > maxPathLen) len = maxPathLen;
	for (natural i = 0; i <len; i++)
		this->path[i] = path[i];
	pathlen = (Bin::natural8)len;
	setDir(d);
}

template<typename T, typename LinkT>
bool AvlTreeNode<T, LinkT>::Iterator::setDir(Direction::Type d) {
	if (d == Direction::forward) this->dir = 0;
	else if (d == Direction::backward) this->dir = 1;
	else return false;

	return true;
}

template<typename T, typename LinkT>
Direction::Type AvlTreeNode<T, LinkT>::Iterator::getDir() const {
	return dir == 0?Direction::forward:Direction::backward;
}

template<typename T, typename LinkT>
const typename AvlTreeNode<T, LinkT>::PNode &AvlTreeNode<T, LinkT>::Iterator::peek() const {
	if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION, typeid(PNode));
	return retNode = getCurNode();
}

template<typename T, typename LinkT>
const typename AvlTreeNode<T, LinkT>::PNode &AvlTreeNode<T, LinkT>::Iterator::getNext() {
	if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION, typeid(PNode));
	retNode = getCurNode();
	goNext();
	return retNode;
}

template<typename T, typename LinkT>
bool AvlTreeNode<T, LinkT>::Iterator::equalTo(const Iterator &other) const {
	if (pathlen != other.pathlen) return false;
	for (natural i = 0; i < pathlen; i++)
		if (path[i] != other.path[i]) return false;
	return true;
}

template<typename T, typename LinkT>
bool AvlTreeNode<T, LinkT>::Iterator::lessThan(const Iterator &other) const {
	natural cmplen = pathlen < other.pathlen? pathlen: other.pathlen;
	if (cmplen == 0) return false;
	for (natural i = 0; i <cmplen; i++)
		if (path[i] != other.path[i]) {
			if (i == 0) return false;
			return other.path[i-1]->link(dir) == path[i];
		}
	if (pathlen < other.pathlen) {
		return path[cmplen] == other.path[cmplen - 1]->link(dir);
	} else if (pathlen > other.pathlen) {
		return path[cmplen-1]->link(1-dir) == other.path[cmplen];
	} else
		return false;
}

template<typename T, typename LinkT>
void AvlTreeNode<T, LinkT>::Iterator::initPath(PNode root, Bin::natural8 pos) {
	pathlen = pos;
	while (root && pathlen < maxPathLen) {
		path[pathlen++] = root;
		root = root->link(dir);
	}
}

template<typename T, typename LinkT>
void AvlTreeNode<T, LinkT>::Iterator::goNext() {
	if (pathlen == 0) return;
	PNode cur = getCurNode();
	if (cur->link(1-dir) && pathlen < maxPathLen) {
		initPath(cur->link(1-dir),pathlen);
	} else {
		PNode nx;
		while (pathlen > 1) {
			pathlen --;
			nx = getCurNode();
			if (nx->link(dir) == cur) return;
			cur = nx;
		}
		pathlen--;
	}
}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT> *AvlTreeNode<T, LinkT>::rotateSingle(int dir) {
	AvlTreeNode<T, LinkT> *save = link(1-dir);
	link(1-dir) = save->link(dir);
	save->dirty = true;
	save->link(dir) = this;
	dirty = true;
	return save;
}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT> *AvlTreeNode<T, LinkT>::rotateDouble(int dir) {
	AvlTreeNode<T, LinkT> *save = link(1-dir)->link(dir);
	link(1-dir)->link(dir) = save->link(1-dir);
	save->link(1-dir) = link(1-dir);
	link(1-dir) = save;
	save->dirty = true;
	dirty = true;

	save = link(1-dir);
	link(1-dir) = save->link(dir);
	save->link(dir) = this;
	save->dirty = true;
	return save;
}

template<typename T, typename LinkT>
void AvlTreeNode<T, LinkT>::adjustBalance(int dir, int bal) {
	AvlTreeNode<T, LinkT> *n = link(dir);
	AvlTreeNode<T, LinkT> *nn = n->link(1-dir);

	if (nn->balance == 0) {
		balance = n->balance = 0;
	} else if (nn->balance == bal) {
		balance = -bal;
		n->balance = 0;
	} else {
		balance = 0;
		n->balance = bal;
	}

	nn->balance = 0;
}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT> *AvlTreeNode<T, LinkT>::insertBalance(int dir) {
	AvlTreeNode<T, LinkT> *n = link(dir);
	int bal = dir == 0?-1:1;

	if (n->balance == bal) {
		balance = n->balance = 0;
		return rotateSingle(1-dir);
	} else {
		adjustBalance(dir,bal);
		return rotateDouble(1-dir);
	}
}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT> *AvlTreeNode<T, LinkT>::removeBalance(int dir, bool &done) {
	AvlTreeNode<T, LinkT> *n = link(1-dir);
	int bal = dir == 0? -1: 1;

	if (n->balance == -bal) {
		balance = n->balance = 0;
		return rotateSingle(dir);
	} else if (n->balance== bal) {
		adjustBalance(1-dir,-bal);
		return rotateDouble(dir);
	} else {
		balance = -bal;
		n->balance = bal;
		done = true;
		return rotateSingle(dir);
	}
}


template<typename T, typename LinkT>
AvlTreeNode<T, LinkT> *AvlTreeNode<T, LinkT>::balanceAfterRemove(AvlTreeNode<T, LinkT> *root, bool &done, int dir) {
	if (!done) {
		root->balance += dir? -1: 1;
		int absbal = root->balance < 0?-root->balance:root->balance;
		if (absbal == 1)
			done = true;
		else if (absbal > 1)
			root = root->removeBalance(dir,done);
	}
	return root;
}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT> *AvlTreeNode<T, LinkT>::takeRightMost(AvlTreeNode<T, LinkT> *root, AvlTreeNode<T, LinkT> *&rm, bool &done) {
	//there is no more right nodes
	if (root->right == 0) {
		//root node is rightmost
		rm = root;
		//left subtree is new root
		return root->left;
	}
	//there are more right nodes
	//recursive enter to function, result is new root of right subtree
	root->right = takeRightMost(root->right,rm,done);
	//rebalance root if it necesery and return new root
	return balanceAfterRemove(root,done,1);
}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT> *AvlTreeNode<T, LinkT>::removeRootNode(AvlTreeNode<T, LinkT> *root, bool &done) {
	if (root->left == 0 || root->right == 0) {
		int dir = root->left == 0?1:0;
		return root->link(dir);
	} else {
		//there will be stored rightmost node
		AvlTreeNode<T, LinkT> *rightMostNode;
		//retrieve rightmost node of left subtree
		AvlTreeNode<T, LinkT> *newroot = takeRightMost(root->left, rightMostNode, done);
		//store new left subtree-root to rightmost node
		rightMostNode->left = newroot;
		//stote root of right subtree to new root right substree
		rightMostNode->right = root->right;
		//store original balance
		rightMostNode->balance = root->balance;

		rightMostNode->dirty = true;
		//rebalance node if necesery and return root of balanced subtree
		return balanceAfterRemove(rightMostNode,done,0);

	}
	return root;

}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT> *AvlTreeNode<T, LinkT>::remove(const Iterator &iter, bool &done, natural pathIndex ) {
	PNode ppos = static_cast<const PrivateIter &>(iter).getPath(pathIndex);
	AvlTreeNode<T, LinkT> *pos = ppos;
	if (pos == this) {
			pos = remove(iter,done,pathIndex+1);
//			static_cast<PrivateIter &>(iter).setPath(pathIndex,pos);
			return pos;
	} else if (pos == 0) {
		AvlTreeNode<T, LinkT> *ret = removeRootNode(this,done);
		this->resetLinks();
		return ret;
	}
	int dir;
	if (left == pos) dir = 0;
	else dir = 1;
	link(dir) = link(dir)->remove(iter,done,pathIndex+1);
	dirty = true;
//	static_cast<PrivateIter &>(iter).setPath(pathIndex,link(dir));
	return balanceAfterRemove(this,done,dir);
}


template<typename T, typename LinkT>
natural AvlTreeNode<T, LinkT>::maxHeight() const {
	const AvlTreeNode<T, LinkT> *x = this;
	natural count = 0;
	while (x != 0) {
		count++;
		if (x->balance == 1)
			x = x->right;
		else
			x = x->left;
	}
	return count;
}

template<typename T, typename LinkT>
natural AvlTreeNode<T, LinkT>::aproxCount() const {
	if (left == 0) return right->aproxCount() + 1;
	if (right == 0) return left->aproxCount() + 1;
	if (balance == 0) {
		return 1 + 2 * left->aproxCount();
	} else if (balance == 1) {
		return 1 + (3 * right->aproxCount()) / 2;
	} else if (balance == -1) {
		return 1 + (3 * left->aproxCount()) / 2;
	} else
		return 1;
}

template<typename T, typename LinkT>
natural AvlTreeNode<T, LinkT>::count() const {
	natural count = 1;
	if (left != 0) count += left->count();
	if (right != 0) count += right->count();
	return count;
}

template<typename T, typename LinkT>
natural AvlTreeNode<T, LinkT>::Iterator::getPosition() const {
	if (!hasItems()) return naturalNull;
	natural pos = pathlen;
	const AvlTreeNode<T, LinkT> *lsubtree = path[pos-1]->left;
	natural count = lsubtree->aproxCount();
	pos--;
	while (pos > 0) {
		pos--;
		if (path[pos]->link(dir) == path[pos+1]) {
			count = count+path[pos]->link(1-dir)->aproxCount()+1;
		}
	}
	return count;
}

template<typename T, typename LinkT>
AvlTreeNode<T, LinkT>::Iterator::Iterator(AvlTreeNode<T, LinkT> *root, natural pos,
		Direction::Type d ) {

	setDir(d);

	do {
		pathlen = 0;
		path[pathlen++] = root;
		natural k = root->link(1-dir)->aproxCount();
		if (k < pos) {
			root = root->right;
			pos -= k;
		} else if (k > pos) {
			root = root->left;
		} else {
			break;
		}

	} while (pos > 0 && root != 0 && pathlen < maxPathLen);
}


template<typename T, typename LinkT>
const AvlTreeNode<T,LinkT> *AvlTreeNode<T,LinkT>::getSearchNode(const T &value) {
	//construct fake pointer to object
	const AvlTreeNode *k = reinterpret_cast<const AvlTreeNode *>(&value);
	//calculate offset between data and begin of node
	natural ofs = reinterpret_cast<const byte *>(&(k->data)) - reinterpret_cast<const byte *>(k);
	//shift pointer about offset. Returned pointer to fake node, where data are at place,
	//so we can compare them (note that value is const, so we cannot modify anything)
	return reinterpret_cast<const AvlTreeNode *>(reinterpret_cast<const byte *>(&value) - ofs);
}

}


#endif /* LIGHTSPEED_CONTAINERS_AVLTREENODE_TCC_ */
