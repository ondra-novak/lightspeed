/*
 * mapreduce.tcc
 *
 *  Created on: 10. 4. 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_CONTAINERS_MAPREDUCE_TCC_
#define LIGHTSPEED_BASE_CONTAINERS_MAPREDUCE_TCC_

#include "mapreduce.h"
#include "avltree.tcc"

namespace LightSpeed {

template<typename Key, typename Value, typename ReducedResult, typename CompareKeys>
class MapReduce<Key,Value,ReducedResult,CompareKeys>::FakeTree: public MapReduce<Key,Value,ReducedResult,CompareKeys>::Tree {
public:
	AvlTreeNode<KeyValue> *getRoot() {return this->tree;}
	const AvlTreeNode<KeyValue> *getRoot() const {return this->tree;}
};


template<typename Key, typename Value, typename ReducedResult, typename CompareKeys>
void MapReduce<Key,Value,ReducedResult,CompareKeys>::update() {

	if (tree.empty()) return;


	AvlTreeNode<KeyValue> *node = static_cast<FakeTree &>(tree).getRoot();
	updateNode(node);
}

template<typename Key, typename Value, typename ReducedResult, typename CompareKeys>
ReducedResult &MapReduce<Key,Value,ReducedResult,CompareKeys>::updateNode(AvlTreeNode<KeyValue> *nd) {
	if (nd->dirty) {
		if (nd->getLeft() == 0 && nd->getRight()== 0) {
			nd->data.rres = ReducedResult(nd->data.key, nd->data.value);
		} else if (nd->getLeft() != 0 && nd->getRight() == 0) {
			nd->data.rres = ReducedResult(updateNode(nd->getLeft()) + ReducedResult(nd->data.key, nd->data.value));
		} else if (nd->getLeft() == 0 && nd->getRight() != 0) {
			nd->data.rres = ReducedResult(ReducedResult(nd->data.key, nd->data.value) + updateNode(nd->getRight()));
		} else  {
			nd->data.rres = ReducedResult(updateNode(nd->getLeft()) + ReducedResult(nd->data.key, nd->data.value) + updateNode(nd->getRight()));
		}
		nd->dirty = false;
	}
	return nd->data.rres;
}

template<typename Key, typename Value, typename ReducedResult, typename CompareKeys>
ReducedResult MapReduce<Key,Value,ReducedResult,CompareKeys>::reduce() const {
	if (tree.empty()) return ReducedResult();

	const AvlTreeNode<KeyValue> *node = static_cast<const FakeTree &>(tree).getRoot();
	if (node->dirty)
		updateNode(const_cast<AvlTreeNode<KeyValue> *>(node));
	return node->data.rres;
}


}


#endif /* LIGHTSPEED_BASE_CONTAINERS_MAPREDUCE_TCC_ */
