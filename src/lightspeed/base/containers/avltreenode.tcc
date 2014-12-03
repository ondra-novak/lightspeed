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


template<typename LinkT>
AvlTreeNode<LinkT>::Iterator::Iterator(PNode root, Direction::Type dir) {
	if (setDir(dir))
		initPath(root);
	else
		pathlen = 0;
	retNode = 0;
}

template<typename LinkT>
AvlTreeNode<LinkT>::Iterator::Iterator(const Iterator &other, Direction::Type d)
	:pathlen(other.pathlen),dir(other.dir),retNode(other.retNode) {
		for (Bin::natural8 i = 0; i < pathlen;i ++)
			path[i] = other.path[i];
	setDir(d);
}

template<typename LinkT>
AvlTreeNode<LinkT>::Iterator::Iterator(PNode *path, natural len, Direction::Type d)
	:dir(0),retNode(0)
{
	if (len > maxPathLen) len = maxPathLen;
	for (natural i = 0; i <len; i++)
		this->path[i] = path[i];
	pathlen = (Bin::natural8)len;
	setDir(d);
}

template<typename LinkT>
bool AvlTreeNode<LinkT>::Iterator::setDir(Direction::Type d) {
	if (d == Direction::forward) this->dir = 0;
	else if (d == Direction::backward) this->dir = 1;
	else return false;

	return true;
}

template<typename LinkT>
Direction::Type AvlTreeNode<LinkT>::Iterator::getDir() const {
	return dir == 0?Direction::forward:Direction::backward;
}

template<typename LinkT>
const typename AvlTreeNode<LinkT>::PNode &AvlTreeNode<LinkT>::Iterator::peek() const {
	if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION, typeid(PNode));
	return retNode = getCurNode();
}

template<typename LinkT>
const typename AvlTreeNode<LinkT>::PNode &AvlTreeNode<LinkT>::Iterator::getNext() {
	if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION, typeid(PNode));
	retNode = getCurNode();
	goNext();
	return retNode;
}

template<typename LinkT>
bool AvlTreeNode<LinkT>::Iterator::equalTo(const Iterator &other) const {
	if (pathlen != other.pathlen) return false;
	for (natural i = 0; i < pathlen; i++)
		if (path[i] != other.path[i]) return false;
	return true;
}

template<typename LinkT>
bool AvlTreeNode<LinkT>::Iterator::lessThan(const Iterator &other) const {
	natural cmplen = pathlen < other.pathlen? pathlen: other.pathlen;
	if (cmplen == 0) return false;
	for (natural i = 0; i <cmplen; i++)
		if (path[i] != other.path[i]) {
			if (i == 0) return false;
			return other.path[i-1]->link[dir] == path[i];
		}
	if (pathlen < other.pathlen) {
		return path[cmplen] == other.path[cmplen - 1]->link[dir];
	} else if (pathlen > other.pathlen) {
		return path[cmplen-1]->link[1-dir] == other.path[cmplen];
	} else
		return false;
}

template<typename LinkT>
void AvlTreeNode<LinkT>::Iterator::initPath(PNode root, Bin::natural8 pos) {
	pathlen = pos;
	while (root && pathlen < maxPathLen) {
		path[pathlen++] = root;
		root = root->link[dir];
	}
}

template<typename LinkT>
void AvlTreeNode<LinkT>::Iterator::goNext() {
	if (pathlen == 0) return;
	PNode cur = getCurNode();
	if (cur->link[1-dir] && pathlen < maxPathLen) {
		initPath(cur->link[1-dir],pathlen);
	} else {
		PNode nx;
		while (pathlen > 1) {
			pathlen --;
			nx = getCurNode();
			if (nx->link[dir] == cur) return;
			cur = nx;
		}
		pathlen--;
	}
}

template<typename LinkT>
AvlTreeNode<LinkT> *AvlTreeNode<LinkT>::rotateSingle(int dir) {
	AvlTreeNode<LinkT> *save = link[1-dir];
	link[1-dir] = save->link[dir];
	save->link[dir] = this;
	return save;
}

template<typename LinkT>
AvlTreeNode<LinkT> *AvlTreeNode<LinkT>::rotateDouble(int dir) {
	AvlTreeNode<LinkT> *save = link[1-dir]->link[dir];
	link[1-dir]->link[dir] = save->link[1-dir];
	save->link[1-dir] = link[1-dir];
	link[1-dir] = save;

	save = link[1-dir];
	link[1-dir] = save->link[dir];
	save->link[dir] = this;
	return save;
}

template<typename LinkT>
void AvlTreeNode<LinkT>::adjustBalance(int dir, int bal) {
	AvlTreeNode<LinkT> *n = link[dir];
	AvlTreeNode<LinkT> *nn = n->link[1-dir];

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

template<typename LinkT>
AvlTreeNode<LinkT> *AvlTreeNode<LinkT>::insertBalance(int dir) {
	AvlTreeNode<LinkT> *n = link[dir];
	int bal = dir == 0?-1:1;

	if (n->balance == bal) {
		balance = n->balance = 0;
		return rotateSingle(1-dir);
	} else {
		adjustBalance(dir,bal);
		return rotateDouble(1-dir);
	}
}

template<typename LinkT>
AvlTreeNode<LinkT> *AvlTreeNode<LinkT>::removeBalance(int dir, bool &done) {
	AvlTreeNode<LinkT> *n = link[1-dir];
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


template<typename LinkT>
AvlTreeNode<LinkT> *AvlTreeNode<LinkT>::balanceAfterRemove(AvlTreeNode<LinkT> *root, bool &done, int dir) {
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

template<typename LinkT>
AvlTreeNode<LinkT> *AvlTreeNode<LinkT>::takeRightMost(AvlTreeNode<LinkT> *root, AvlTreeNode<LinkT> *&rm, bool &done) {
	//there is no more right nodes
	if (root->link[1] == 0) {
		//root node is rightmost
		rm = root;
		//left subtree is new root
		return root->link[0];
	}
	//there are more right nodes
	//recursive enter to function, result is new root of right subtree
	root->link[1] = takeRightMost(root->link[1],rm,done);
	//rebalance root if it necesery and return new root
	return balanceAfterRemove(root,done,1);
}

template<typename LinkT>
AvlTreeNode<LinkT> *AvlTreeNode<LinkT>::removeRootNode(AvlTreeNode<LinkT> *root, bool &done) {
	if (root->link[0] == 0 || root->link[1] == 0) {
		int dir = root->link[0] == 0?1:0;
		return root->link[dir];
	} else {
		//there will be stored rightmost node
		AvlTreeNode<LinkT> *rightMostNode;
		//retrieve rightmost node of left subtree
		AvlTreeNode<LinkT> *newroot = takeRightMost(root->link[0], rightMostNode, done);
		//store new left subtree-root to rightmost node
		rightMostNode->link[0] = newroot;
		//stote root of right subtree to new root right substree
		rightMostNode->link[1] = root->link[1];
		//store original balance
		rightMostNode->balance = root->balance;
		//rebalance node if necesery and return root of balanced subtree
		return balanceAfterRemove(rightMostNode,done,0);

	}
	return root;

}

template<typename LinkT>
AvlTreeNode<LinkT> *AvlTreeNode<LinkT>::remove(Iterator &iter, bool &done, natural pathIndex ) {
	AvlTreeNode<LinkT> *pos = static_cast<PrivateIter &>(iter).getPath(pathIndex);
	if (pos == this) {
			pos = remove(iter,done,pathIndex+1);
			static_cast<PrivateIter &>(iter).setPath(pathIndex,pos);
			return pos;
	} if (pos == 0) {
		AvlTreeNode<LinkT> *ret = removeRootNode(this,done);
		this->resetLinks();
		return ret;
	}
	int dir;
	if (link[0] == pos) dir = 0;
	else dir = 1;
	link[dir] = link[dir]->remove(iter,done,pathIndex+1);
	static_cast<PrivateIter &>(iter).setPath(pathIndex,link[dir]);
	return balanceAfterRemove(this,done,dir);
}


template<typename LinkT>
natural AvlTreeNode<LinkT>::maxHeight() const {
	const AvlTreeNode<LinkT> *x = this;
	natural count = 0;
	while (x != 0) {
		count++;
		if (x->balance == 1)
			x = x->link[1];
		else
			x = x->link[0];
	}
	return count;
}

template<typename LinkT>
natural AvlTreeNode<LinkT>::aproxCount() const {
	if (link[0] == 0) return link[1]->aproxCount() + 1;
	if (link[1] == 0) return link[0]->aproxCount() + 1;
	if (balance == 0) {
		return 1 + 2 * link[0]->aproxCount();
	} else if (balance == 1) {
		return 1 + (3 * link[1]->aproxCount()) / 2;
	} else if (balance == -1) {
		return 1 + (3 * link[0]->aproxCount()) / 2;
	} else
		return 1;
}

template<typename LinkT>
natural AvlTreeNode<LinkT>::count() const {
	return 1 + link[0]->count() + link[1]->count();
}

template<typename LinkT>
natural AvlTreeNode<LinkT>::Iterator::getPosition() const {
	if (!hasItems()) return naturalNull;
	natural pos = pathlen;
	const AvlTreeNode<LinkT> *lsubtree = path[pos-1]->link[0];
	natural count = lsubtree->aproxCount();
	pos--;
	while (pos > 0) {
		pos--;
		if (path[pos]->link[dir] == path[pos+1]) {
			count = count+path[pos]->link[1-dir]->aproxCount()+1;
		}
	}
	return count;
}

template<typename LinkT>
AvlTreeNode<LinkT>::Iterator::Iterator(AvlTreeNode<LinkT> *root, natural pos,
		Direction::Type d ) {

	setDir(d);

	do {
		pathlen = 0;
		path[pathlen++] = root;
		natural k = root->link[1-dir]->aproxCount();
		if (k < pos) {
			root = root->link[1];
			pos -= k;
		} else if (k > pos) {
			root = root->link[0];
		} else {
			break;
		}

	} while (pos > 0 && root != 0 && pathlen < maxPathLen);
}


}


#endif /* LIGHTSPEED_CONTAINERS_AVLTREENODE_TCC_ */
