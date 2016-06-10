/*
 * map.h
 *
 *  Created on: 13.9.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_MAP_H_
#define LIGHTSPEED_CONTAINERS_MAP_H_

#include "avltree.h"
#include "linkedList.h"
#include "../exceptions/exception.h"
#include <functional>
namespace LightSpeed {


	class NotFoundException: public Exception {
	public:
		NotFoundException(const ProgramLocation &loc):Exception(loc) {}

		static const char *keyNotFound_msgText;

		virtual ~NotFoundException() throw() {}

	};

	template<typename Key>
	class KeyNotFoundException: public NotFoundException {
	public:

		LIGHTSPEED_EXCEPTIONFINAL;

		KeyNotFoundException(const ProgramLocation &loc, const Key &key)
			:NotFoundException(loc),key(key) {}

		const Key &getKey() const {return key;}

		virtual ~KeyNotFoundException() throw () {}

	protected:
		Key key;

		void message(ExceptionMsg &msg) const {
			msg(keyNotFound_msgText); //TODO: << str(key);
		}
	};



	///Container with associative access
	/**
	 * You can specify type of key and type of value. For each key there can be one value
	 *
	 * @tparam Key name of class used as key
	 * @tparam Value name of class used for value
	 * @tparam Cmp comparsion operator
	 * @tparam Factory class used as factory for nodes
	 */
	template<typename Key, typename Value, typename Cmp = std::less<Key> >
	class Map {

	public:


		///This struct contains key with the value.
		/** You will need this while processing items using iterator
		 *
		 */

		struct KeyValue
		{
			///key cannot be changed
			const Key key;
			///value can be changed even if container is const
			mutable Value value;

			///constructor
			KeyValue(const Key &key, const Value &value):key(key),value(value) {}
#if __cplusplus >= 201103L
			KeyValue(Key &&key, Value &&value):key(std::move(key)),value(std::move(value)) {}
			KeyValue(KeyValue &&x):key(std::move(x.key)),value(std::move(x.value)) {}
			KeyValue(const KeyValue &x):key(x.key),value(x.value) {}
#endif


			template<typename Archive>
			void serialize(Archive &arch) {
				arch(key);
				arch(value);
			}
		};

		typedef KeyValue Entity;

	protected:


		struct CmpData {
			Cmp cmp;

			bool operator()(const KeyValue &a, const KeyValue &b) const {
				return cmp(a.key,b.key);
			}
			CmpData(const Cmp &cmp):cmp(cmp) {}
		};

		typedef AVLTree<KeyValue,CmpData> Tree;

	public:

		///map constructor
		Map():tree(CmpData(Cmp())) {}
		///map constructor
		/**
		 * @param cmp instance of compare operator
		 * @param factory instance of factory
		 * @return
		 */
		Map(const Cmp &cmp)
			:tree(CmpData(cmp),createDefaultNodeAllocator()) {}

		Map(const Cmp &cmp, SharedPtr<IRuntimeAlloc> factory )
			:tree(CmpData(cmp),factory) {}

		Map(SharedPtr<IRuntimeAlloc> factory )
			:tree(CmpData(Cmp()),factory) {}

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

#if __cplusplus >= 201103L
		Iterator insert(Key &&key,Value &&value, bool *exist = 0) {
			return tree.insert(KeyValue(std::move(key),std::move(value)),exist);
		}
#endif


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

		Value *find(const Key &key)  {
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
		Iterator seek(const Key &key, bool *found = 0, Direction::Type dir = Direction::forward) const {
			const KeyValue &e = reinterpret_cast<const KeyValue &>(key);
			return tree.seek(e,found,dir);
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

		///access item for reading
		/**
		 * associative access to the item.
		 *
		 * @param key key
		 * @return reference to found value
		 * @exception KeyNotFoundException - thrown, when key not found
		 */
		const Value &operator[](const Key &key) const {
			const Value *v = find(key);
			if (v == 0) throw KeyNotFoundException<Key>(THISLOCATION,key);
			else return *v;
		}

		///access item for reading and writing - creates the item
		/**
		 * @param key key
		 * @return reference to value. Operator can be used as l-value.
		 *    If key doesn't exists, it is created
		 */
		Value &operator()(const Key &key) {

			Iterator x = insert(key, Value());
			return x.getNext().value;
		}


		Value &operator()(const Key &key, const Value &val) {

			Iterator x = insert(key, val);
			return x.getNext().value;
		}

		///loads data into the container
		template<typename K>
		void insert(IIterator<KeyValue,K> &in) {
			while (in.hasItems()) tree.insert(in.getNext());
		}


		///replaces value under given key
		/**
		 * @param key key to replace. If not exists. it is created
		 * @param value new value for the key
		 * @return iterator refers the key
		 *
		 */
		Iterator replace(const Key &key, const Value &value) {
			bool exist = false;
			Iterator x = insert(key,value,&exist);
			if (exist) x.peek().value = value;
			return x;
		}

		template<typename Arch>
		void serialize(Arch &arch) {
			typename Arch::Array arr(arch,size());
			if (arch.storing()) {
				Iterator x = getFwIter();
				while (x.hasItems() && arr.next()) {
					const KeyValue &e = x.getNext();
					arch << e.key;
					arch << e.value;
				}
			} else {
				clear();
				while (arr.next()) {
					Key k;
					Value v;
					arch >> k;
					arch >> v;
					insert(k,v);
				}
			}
		}

		NodeAlloc getFactory() const {return tree.getFactory();}

		Map operator+(const Map &other) const {
			return Map(*this,other,include);
		}
		Map operator+(const KeyValue &item) const {
			return Map(*this,item,include);
		}
		Map operator-(const Map &other) const {
			return Map(*this,other,exclude);
		}
		Map operator-(const KeyValue &item) const{
			return Map(*this,item,exclude);
		}
		Map &operator+=(const Map &other) {
			for (typename Map::Iterator iter = other.getFwIter();iter.hasItems();)
				(*this) += iter.getNext();
			return *this;
		}
		Map &operator+=(const KeyValue &item) {
			insert(item.key,item.value);			
			return *this;
		}
		Map &operator-=(const Map &other) {
			for (typename Map::Iterator iter = other.getFwIter();iter.hasItems();)
				(*this) -= iter.getNext();
			return *this;
		}
		Map &operator-=(const KeyValue &item) {
			erase(item.key);
			return *this;
		}

		CompareResult compare(const Map &other) const {
			Iterator iter1 = getFwIter();
			Iterator iter2 = other.getFwIter();

			while(iter1.hasItems() && iter2.hasItems()) {
				const KeyValue &a = iter1.getNext();
				const KeyValue &b = iter2.getNext();
				if (tree.cmpOper(a.key,b.key)) return cmpResultLess;
				if (tree.cmpOper(b.key,a.key)) return cmpResultGreater;
			}
			if (iter1.hasItems()) return cmpResultGreater;
			else if (iter2.hasItems()) return cmpResultLess;
			else return cmpResultEqual;
		}

		void swap(Map& other) {
			tree.swap(other.tree);
		}

	protected:
		enum Exclude{exclude};
		enum Include{include};

		Map(const Map &other, const Map &other2, Exclude ) {
			(*this)-=other;
			(*this)-=other2;
		}
		Map(const Map &other, const Map &other2, Include ) {
			(*this)+=other;
			(*this)+=other2;
		}
		Map(const Map &other, const KeyValue &other2, Exclude ) {
			(*this)-=other;
			(*this)-=other2;
		}
		Map(const Map &other, const KeyValue &other2, Include ) {
			(*this)+=other;
			(*this)+=other2;
		}

	protected:
		Tree tree;

	};


	///Container, that allows multiple values under single key
	/**
	 * MultiMap is implemented as map which contains linked list of values
	 *
	 * Keys are always unique. Once key exist, it is not replace, because
	 * container assumes, that equal keys are same. Under each key there can
	 * be multiple values. Values are ordered in order of storing. First
	 * value is stored as first, last value is stored as last.
	 */
	template<typename Key, typename Value, typename Cmp = std::less<Key> >
	class MultiMap: public Map<Key,LinkedList<Value>,Cmp>  {
	public:
		typedef Map<Key,LinkedList<Value>,Cmp> Super;
		typedef LinkedList<Value> ValueList;

		typedef typename Super::Iterator MapIter; ///< obsolette
		typedef typename Super::Iterator KeyIter;
		typedef typename Super::KeyValue KeyKeyValue;
		typedef typename ValueList::Iterator ListIter;


	public:

		///constructor
		MultiMap() {}

		///constructor with instances of helper objects
		/**
		 * @param cmp instance of compare operator
		 * @param factory instance of factory
		 */
		MultiMap(const Cmp &cmp, NodeAlloc factory)
			:Super(cmp,factory),valueSrc(factory) {}

		MultiMap(const Cmp &cmp)
			:Super(cmp) {}

		MultiMap(NodeAlloc factory)
			:Super(factory),valueSrc(factory) {}


		///This structure is used in the iterator.
		/**
		 * This KeyValue is virtual, in fact keys and values are stored
		 * separately. While processing items using iterator, each item
		 * with the same key is returned as standalone KeyValue containing
		 * key same for these values and different value for each item.
		 *
		 * This allows to use MultiMap as replacement of Map and vice versa
		 */
		struct KeyValue {
			const Key &key;
			 Value &value;

			 KeyValue(const Key &key,
				   Value &value):key(key),value(value) {}

		};

		///Backward compatibility
		typedef KeyValue Entity;

		///Iterator
		/** iterates through structure
		 *
		 * @note Values under single key are processed in the same order
		 * regardless on direction. If items are processed backward,
		 * keys appears in backward order, but values under keys are
		 * processed in order of inserting for both directions.
		 */
		class Iterator: public IteratorBase<KeyValue, Iterator> {
		public:

			friend class MultiMap;

			Iterator(const MapIter &mapIter,
					const ListIter &listIter)
					:mapIter(mapIter),listIter(listIter) {}
			Iterator(const MapIter &mapIter)
					:mapIter(mapIter),
					 listIter(mapIter.hasItems()?
							 mapIter.peek().value.getFwIter():ListIter(0)) {}

			bool hasItems() const {return mapIter.hasItems();}
			const KeyValue &getNext(){
				const KeyValue &out = getKeyValue(mapIter.peek().key,
							const_cast<Value &>(listIter.peek()));
				listIter.skip();
				adjustAfterSkip();
				return out;
			}

			const KeyValue &peek() const {
				return getKeyValue(mapIter.peek().key,
						const_cast<Value &>(listIter.peek()));
			}
			bool equalTo(const Iterator &iter) const {
				return mapIter.equalTo(iter.mapIter)
						&& listIter.equalTo(iter.listIter);
			}

			bool lessThan(const Iterator &iter) const {
				return mapIter.lessThan(iter.mapIter)
					|| (mapIter.equalTo(iter.mapIter)
						&& listIter.lessThan(iter.listIter));
			}

			bool setDir(Direction::Type e) {
				return mapIter.setDir(e);
			}
			Direction::Type getDir() const {
				return mapIter.getDir();
			}

		protected:
			MapIter mapIter;
			ListIter listIter;
			mutable byte item[sizeof(KeyValue)];

			const KeyValue &getKeyValue(const Key &key,  Value &value) const  {
				return *(new(reinterpret_cast<void *>(item)) KeyValue(key,value));
			}

			void adjustAfterSkip() {
				if (!listIter.hasItems()) {
					mapIter.skip();
					if (mapIter.hasItems())
						listIter = mapIter.peek().value.getFwIter();
				}
			}
		};

		friend class Iterator;

		///Insert key and value
		/**
		 * @param key key. If key exists, it is not replaced
		 * @param value new value
		 * @return iterator refers newly created item
		 */
		Iterator insert(const Key &key, const Value &value) {
			MapIter x = Super::insert(key,valueSrc);
			ListIter y = x.peek().value.add(value);
			return Iterator(x,y);
		}

		///Erase the value
		/** erases value from the key. If key contains empty list, it is also removed
		 *
		 * @param iter iterator points to value to remove
		 */
		void erase(Iterator &iter) {

			ValueList &lst = iter.mapIter.peek().value;
			iter.listIter = lst.erase(iter.listIter);
			if (lst.empty()) {
				Super::erase(iter.mapIter);
				if (iter.mapIter.hasItems())
					iter.listIter = iter.mapIter.peek().value.getFwIter();
			} else {
				iter.adjustAfterSkip();
			}
		}

		///Erases whole key with all values
		using Super::erase;

		///Finds values for the key
		/**
		 * @param key key to find
		 * @return iterator through the values. If key not found, result is empty iterator
		 */
		ListIter find(const Key &key) const {
			const ValueList *x = Super::find(key);
			if (x == 0) return valueSrc.getFwIter();
			else return x->getFwIter();
		}

		///Seeks to key
		/**
		 * Seeks to key or first key after requested key
		 * @param key key to seek
		 * @param found sets variable to true, if key has been found
		 * @return iterator to first value under this key
		 */
		Iterator seek(const Key &key, bool *found = 0) const {
			return Super::seek(key,found);
		}

		///returns count of keys
		/**
		 * @return count of keys in O(1) time. This number
		 * will be lower or equal than size() because there can be more
		 * values than keys.
		 */
		natural keyCount() const {return Super::size();}
		///returns count of items
		/**
		 * @return count of items in O(n) time.
		 *
		 * @note watch out to complexity! To retrieve count of items, function must
		 * count all items under all keys
		 */
		natural size() const {
			natural x = 0;
			for (MapIter i = Super::getFwIter(); i.hasItems();)
				x += i.getNext().value.size();
			return x;
		}

		///Retrieves forward iterator
		Iterator getFwIter() const {return Iterator(Super::getFwIter());}
		///Retrieves backward iterator
		Iterator getBkIter() const {return Iterator(Super::getBkIter());}

		///Retrieves forward iterator
		MapIter getKeyFwIter() const {return Super::getFwIter();}
		///Retrieves backward iterator
		MapIter getKeyBkIter() const {return Super::getBkIter();}


		template<typename K>
		void insert(IIterator<KeyValue,K> &in) {
			while (in.hasItems()) {
				const KeyValue &e = in.getNext();
				Super::insert(e.key,e.value);
			}
		}


	protected:
		ValueList valueSrc;

	};

}  // namespace LightSpeed


#endif /* LIGHTSPEED_CONTAINERS_MAP_H_ */

