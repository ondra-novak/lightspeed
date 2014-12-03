/*
 * simpleList.h
 *
 *  Created on: 13.9.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_SIMPLELIST_H_
#define LIGHTSPEED_CONTAINERS_SIMPLELIST_H_

#include <utility>
#include "../memory/stdFactory.h"
#include "../iter/iterator.h"
#include "../containers/move.h"
#include "../memory/nodeAlloc.h"

namespace LightSpeed {


	///Implementation of single linked list
	/**
	 * Implements linked list. It is single linked, so it allows process
	 * items in one direction.
	 *
	 * @tparam T type of value stored in the list
	 * @tparam Factory class used to create nodes
	 */
	template<typename T>
	class LinkedList {
	public:

		///A list node
		/** In most of cases, this structure is intended for internal usage.
		 * If you wish to create and insert nodes manually, you probably
		 * want to know it.
		 *
		 * Manual creation and insert of nodes allows to implement
		 * linked list without allocations for example above statically
		 * allocated objects
		 *
		 */
		struct ListItem {
			T value;
			ListItem *next;

			ListItem(const T &value):value(value),next(0) {}
		};


		///Constructor
		LinkedList():first(0),last(0),fact(createDefaultNodeAllocator()) {}

		///Constructor with factory
		/**
		 * @param fact specify factory instance
		 */
		LinkedList(NodeAlloc fact):first(0),last(0)
				,fact(fact) {}


		///Allows to iterate through linked list
		class Iterator: public IteratorBase<T,Iterator> {
		public:

			Iterator(const ListItem *cur):cur(cur),prev(0) {}
			Iterator(const ListItem *cur,const ListItem *prev):cur(cur),prev(prev) {}
			bool hasItems() const {return cur != 0;}
			const T &getNext() {
				if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
				prev = cur;
				cur = cur->next;
				return prev->value;
			}
			const T &peek() const {
				if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
				return cur->value;
			}
			bool equalTo(const Iterator &other) const {
				return cur == other.cur;
			}
			bool lessThan(const Iterator &other) const {
				if (cur == 0) return false;
				const ListItem *k = cur;
				do {
					k = k->next;
				} while (k && k != other.cur);
				return k == other.cur;
			}
			///Retrieves previous value,
			const T &getPrev() const {
				if (!prev)throwIteratorNoMoreItems(THISLOCATION,typeid(T));
				return prev->value;
			}


		protected:
			const ListItem *cur;
			const ListItem *prev;

			friend class LinkedList;
		};

		///Copy constructor
		/**
		 * Creates copy.
		 * @param other source instance
		 */
		LinkedList(const LinkedList &other):first(0),last(0),fact(other.fact) {
			for (Iterator i = other.getFwIter();i.hasItems();)
				insertFirst(i.getNext());
		}

		LinkedList(MoveConstruct, LinkedList &other)
			:first(other.first),last(other.last),fact(other.fact) {
			other.first = 0;
			other.last = 0;
		}

		///swap two lists
		void swap(LinkedList &other) {
			std::swap(first,other.first);
			std::swap(last,other.last);
		}

		///assignment operator
		LinkedList &operator=(const LinkedList &other) {
			clear();
			for (Iterator i = other.getFwIter();i.hasItems();)
				add(i.getNext());
			return *this;
		}


		///Inserts item as first item of list
		/**
		 * @param val value of item
		 * @return iterator that refers the newly inserted item
		 */
		Iterator insertFirst(const T &val) {
			ListItem *x = fact->createInstanceUsing<ListItem,T>(val);
			return insertFirst(x);
		}

		///Inserts item after item pointed by the iterator
		/**
		 * @param iter iterator that points to item
		 * @param val value of the new item
		 * @return iterator that points to newly created item
		 */
		Iterator insertAfter(const Iterator &iter, const T &val) {
			if (!iter.hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			ListItem *x = fact.createInstance(val);
			return insertAfter(iter,x);
		}
		///Inserts item before item pointed by the iterator
		/**
		 * @param iter iterator that points to item
		 * @param val value of the new item
		 * @return iterator that points to newly created item
		 */
		Iterator insertBefore(const Iterator &iter, const T &val) {
			if (!iter.hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			ListItem *x = fact.createInstance(val);
			return insertBefore(iter,x);
		}

		///Adds item to the list
		/** Item is added at the end of list
		 *
		 * @param val new value
		 * @return iterator that points to newly created item
		 */
		Iterator add(const T &val) {
			ListItem *x = fact->createInstanceUsing<ListItem,T>(val);
			return add(x);
		}

		///Inserts new node as first node of the list
		/**
		 * @param item pointer to the new node.
		 * @return iterator that points to the node
		 */
		Iterator insertFirst(ListItem *item) {
			item->next = first;
			first = item;
			if (last == 0) last = item;
			return Iterator(item);
		}

		///Inserts new node as after the node pointed by the iterator
		/**
		 * @param iter iterator that points to the item
		 * @param item pointer to the new node.
		 * @return iterator that points to the node
		 */
		Iterator insertAfter(const Iterator &iter, ListItem *item) {
			if (!iter.hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			item->next = const_cast<ListItem *>(iter.cur->next);
			const_cast<ListItem *>(iter.cur->next) = item;
			if (iter.cur == last) last = item;
			return Iterator(item);
		}

		///Inserts new node as before the node pointed by the iterator
		/**
		 * @param iter iterator that points to the item
		 * @param item pointer to the new node.
		 * @return iterator that points to the node
		 */
		Iterator insertBefore(const Iterator &iter, ListItem *item) {
			if (!iter.hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			if (iter.prev == 0) return insert(item);

			item->next = const_cast<ListItem *>(iter.cur);
			const_cast<ListItem *>(iter.prev->next) = item;
			const_cast<Iterator &>(iter).prev = item;
			return Iterator(item);
		}


		///Appends node
		Iterator add(ListItem *x) {
			x->next = 0;
			if (last == 0) first = last = x;
			else {
				last->next = x;
				last = x;
			}
			return Iterator(x);
		}

		///Removes node and returs it
		ListItem *removeFirst() {
			if (first) {
				ListItem *out = first;
				first = first->next;
				if (first == 0) last = 0;
				return out;
			} else {
				return 0;
			}
		}

		///Erases first value
		void eraseFirst() {
			ListItem *x = removeFirst();
			if (x) fact->destroyInstance(x);
		}

		///Remove node refered by the iterator and returns it
		ListItem *remove(const Iterator &iter) {
			if (iter.prev == 0) return removeFirst();
			ListItem *prv = const_cast<ListItem *>(iter.prev);
			ListItem *ret = const_cast<ListItem *>(iter.cur);
			prv->next = ret->next;
			ret->next = 0;
			return ret;
		}

		///Erases node refered by the iterator and returns it
		Iterator erase(const Iterator &iter) {
			ListItem *rem = remove(iter);
			if (rem) {
				fact->destroyInstance(rem);
				if (iter.prev == 0) return getFwIter();
				else return Iterator(iter.prev->next,iter.prev);
			}
			return Iterator(rem);
		}

		///Retrieves first item
		const T &getFirst() const {
			if (first) return first->value;
			else throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			throw;
		}

		///Retrieves last item
		const T &getLast() const {
			if (last) return first->value;
			else throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			throw;
		}

		///Retrieves first item
		T &getFirst() {
			if (first) return first->value;
			else throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			throw;
		}

		///Retrieves last item
		T &getLast()  {
			if (last) return last->value;
			else throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			throw;
		}

		///Retrieves forward iterator
		Iterator getFwIter() const {
			return Iterator(first);
		}

		///returns true, if container is empty
		bool empty() const {return first == 0;}


		///erases all items
		void clear() {
			///TODO exception handler
			while (!empty()) eraseFirst();
		}

		///returns count of items (slow, calculates processing all items)
		natural size() const {
			Iterator k = getFwIter();
			natural cnt = 0;
			while (k.hasItems()) {
				cnt++;
				k.skip();
			}
			return cnt;
		}

		T &getItem(const Iterator &iter) {
				return const_cast<T &>(iter.peek());
		}

		const T &getItem(const Iterator &iter) const {
				return iter.peek();
		}

		~LinkedList() {clear();}

		template<typename Archive>
		void serialize(Archive &arch) {
			if (arch.storing()) {
				typename Archive::Array arr(arch,size());
				Iterator iter = getFwIter();
				while (iter.hasItems() && arr.next())
					arch << iter.getNext();
			} else {
				clear();
				typename Archive::Array arr(arch,naturalNull);
				while (arr.next()) {
					T x;
					arch >> x;
					add(x);
				}
			}
		}

		NodeAlloc getAllocator() const {return fact;}

	protected:

		ListItem *first;
		ListItem *last;

		NodeAlloc fact;


	};

	template<typename T>
	class MoveObject<LinkedList<T> >: public MoveObject_Construct {};

}  // namespace LightSpeed


#endif /* LIGHTSPEED_CONTAINERS_SIMPLELIST_H_ */
