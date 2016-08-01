/*
 * mapreduce.h
 *
 *  Created on: 10. 4. 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_CONTAINERS_MAPREDUCE_H_
#define LIGHTSPEED_BASE_CONTAINERS_MAPREDUCE_H_

#include <functional>
#include "../containers/constStr.h"
#include "../containers/optional.h"
#include "avltree.h"


namespace LightSpeed {



///MapReduce is special map which keeps partial results at nodes to calculate fast reduce operation
/** @tparam Key key
 *  @tparam Value value
 *  @tparam ReducedResult special class, which holds result. There must be defined two operations
 *  to make MapReduce work. First, there must be constructor ReducedResult(Key,Value) which constructs
 *  instance of the ReductedResult from the key and value. Second, there
 *  must be operator+ which combines two results into one. The operator + can
 *  also return some intermediate result which is used to perform reduce during
 *  construction of final result - because nodes are always reduced from three source:
 *    left + center + right => left branch + node value + right branch.
 *
 *  ReducedResult must also have a default constructor which is returned if map is empty
 */
template<typename Key, typename Value, typename ReducedResult, typename CompareKeys = std::less<Key> >
class MapReduce {
public:


	///This struct contains key with the value.
	/** You will need this while processing items using iterator
	 *
	 */

	struct KeyValue
	{
		///key cannot be changed
		const Key key;
		///value. Note that value cannot be changed, because it is connected with reduced result
		const Value value;
		///Contains reduced result for this node (left + current + right)
		ReducedResult rres;
		///constructor
		KeyValue(const Key &key, const Value &value):key(key),value(value) {}
#ifdef LIGHTSPEED_ENABLE_CPP11
		KeyValue(Key &&key, Value &&value):key(std::move(key)),value(std::move(value)) {}
		KeyValue(KeyValue &&x):key(std::move(x.key)),value(std::move(x.value)) {}
		KeyValue(const KeyValue &x):key(x.key),value(x.value) {}
#endif

	};

protected:


	struct CmpData {
		CompareKeys cmp;

		bool operator()(const KeyValue &a, const KeyValue &b) const {
			return cmp(a.key,b.key);
		}
		CmpData(const CompareKeys &cmp):cmp(cmp) {}
	};

	typedef AVLTree<KeyValue,CmpData> Tree;
public:
	///map constructor
	MapReduce()
		:tree(CmpData(CompareKeys())) {}
	///map constructor
	/**
	 * @param cmp instance of compare operator
	 * @param factory instance of factory
	 * @return
	 */
	MapReduce(const CompareKeys &cmp)
		:tree(CmpData(cmp),createDefaultNodeAllocator()) {}

	MapReduce(const CompareKeys &cmp, SharedPtr<IRuntimeAlloc> factory )
		:tree(CmpData(cmp),factory) {}

	MapReduce(SharedPtr<IRuntimeAlloc> factory )
		:tree(CmpData(CompareKeys()),factory) {}

	typedef typename Tree::Iterator Iterator;

	///Inserts item
	/**
	 * @param key new key
	 * @param value new value
	 * @param exist pointer to bool, which will be set to true, if key already exists
	 * @return iterator which refers item with the same key
	 *
	 * @note if key already exist, function will not replace the value
	 */
	Iterator insert(const Key &key, const Value &value, bool *exist = 0) {
		return tree.insert(KeyValue(key,value),exist);
	}


	///Erases the key and value
	bool erase(const Key &key) {
		const KeyValue &e = reinterpret_cast<const KeyValue &>(key);
		return tree.erase(e);
	}

	///Finds value by the key
	/**
	 * @param key contains key
	 * @return returns pointer to value, or NULL, if not exist
	 */
	const Value *find(const Key &key) const {
		const KeyValue &e = reinterpret_cast<const KeyValue &>(key);
		const KeyValue *res = tree.find(e);
		return res?&res->value:0;
	}

	///Seeks position of key
	/**
	 * Function seeks position for the key. If key exist, iterator
	 * will point to the key.  Otherwise, iterator will point to
	 * the next key in order
	 * @param key key to search
	 * @param found used to store state, whether key has been found
	 * @return iterator that refers the found item
	 */
	Iterator seek(const Key &key, bool *found = 0) const {
		const KeyValue &e = reinterpret_cast<const KeyValue &>(key);
		return tree.seek(e,found);
	}

	///removes all items
	void clear() {tree.clear();}
	///tests, whether container is empty
	bool empty() const {return tree.empty();}
	///retrieves count of items (in O(1) time)
	natural size() const {return tree.size();}
	natural length() const {return tree.size();}
	///retrieves forward iterator
	Iterator getFwIter() const {return tree.getFwIter();}
	///retrieves backward iterator
	Iterator getBkIter() const {return tree.getBkIter();}

	///erase item by iterator
	void erase(Iterator &iter) {
		tree.erase(iter);
	}

	NodeAlloc getFactory() const {return tree.getFactory();}


	CompareResult compare(const MapReduce &other) const {
		Iterator iter1 = getFwIter();
		Iterator iter2 = other.getFwIter();

		while(iter1.hasItems() && iter2.hasItems()) {
			const KeyValue &a = iter1.getNext();
			const KeyValue &b = iter2.getNext();
			if (tree.getCompareOperator()(a,b)) return cmpResultLess;
			if (tree.getCompareOperator()(b,a)) return cmpResultGreater;
		}
		if (iter1.hasItems()) return cmpResultGreater;
		else if (iter2.hasItems()) return cmpResultLess;
		else return cmpResultEqual;
	}

	void swap(MapReduce& other) {
		tree.swap(other.tree);
	}

	///Performs reduce in given key range
	/**
	 * @param fromKey starting key. Key is included. If key doesn't exists, function
	 *  starts with nearest key at specified direction
	 * @param toKey ending key. Key is excluded always excluded. You should always specify
	 *  higher key then starting key, otherwise result can be inaccure
	 *
	 * @return reduced result
	 *
	 * @note function increases speed for multiple queries, because particular
	 * results are stored on nodes. Function also updates changed nodes made during
	 * updates. This also means, that changing data doesn't trigger complete recalculation.
	 * Only changed nodes are recalculated
	 *
	 * @note even the function is marked const, it is not thread safe and reading it
	 * should performed with exclusive lock, unless update() is called during modification
	 * phase
	 */
	ReducedResult reduce(const Key &fromKey, const Key &toKey) const;
	ReducedResult reduce(const Iterator &from, const Iterator &toKey) const;

	///Performs reduce on whole tree
	/** function updates tree if necesery. */
	ReducedResult reduce() const;

	///Updates the object performing global map-reduce phase to ensure, that all dirty nodes are recalculated
	/** After this function is returned, any call of reduce() is MT safe until the
	 * map is modified again
	 */
	void update();

protected:
	Tree tree;

	static ReducedResult &updateNode(AvlTreeNode<KeyValue> *node);

	class FakeTree;
	friend class FakeTree;
};




}


#endif /* LIGHTSPEED_BASE_CONTAINERS_MAPREDUCE_H_ */
