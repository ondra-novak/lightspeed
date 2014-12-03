/*
 * deque.tcc
 *
 *  Created on: 17.9.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_DEQUE_TCC_
#define LIGHTSPEED_CONTAINERS_DEQUE_TCC_

#pragma once

#include "deque.h"


namespace LightSpeed {

	template<typename T,  natural citems>
	void Deque<T,citems>::pushFront(const T &item) {
		if (head == 0) addClusterFront();
		Cluster &k = list.getFirst();
		new(k.getBase()+head) T(item);
		head--;
	}

	template<typename T,  natural citems>
	void Deque<T,citems>::pushBack(const T &item) {
		if (tail == citems) addClusterBack();
		Cluster &k = list.getLast();
		new(k.getBase()+tail) T(item);
		tail++;
	}

	template<typename T,  natural citems>
	T &Deque<T,citems>::pushFront() {
		if (head == 0) addClusterFront();
		Cluster &k = list.getFirst();
		T *res = new(k.getBase()+head) T();
		head--;
		return *res;
	}

	template<typename T,  natural citems>
	T &Deque<T,citems>::pushBack() {
		if (tail == citems) addClusterBack();
		Cluster &k = list.getLast();
		T *res = new(k.getBase()+tail) T();
		tail++;
		return *res;
	}


	template<typename T,  natural citems>
	const T &Deque<T,citems>::front() const {
		const Cluster &k = list.getFirst();
		return k.getBase()[head];
	}
	template<typename T,  natural citems>
	const T &Deque<T,citems>::back() const {
		const Cluster &k = list.getLast();
		return k.getBase()[tail-1];
	}

	template<typename T,  natural citems>
	T &Deque<T,citems>::front()  {
		const Cluster &k = list.getFirst();
		return k.getBase()[head];
	}
	template<typename T,  natural citems>
	T &Deque<T,citems>::back() {
		Cluster &k = list.getLast();
		return k.getBase()[tail-1];
	}

	template<typename T,  natural citems>
	void Deque<T,citems>::popFront() {
		const Cluster &k = list.getFirst();
		k.getBase()[head].~T();
		head++;
		if (head == citems) list.eraseFirst();
	}
	template<typename T,  natural citems>
	void Deque<T,citems>::popBack() {
		const Cluster &k = list.getFirst();
		k.getBase()[tail-1].~T();
		head--;
		if (tail == 0) {
			//NOTE remove items from the back is slow
			typename List::Iterator iter = list.getFwIter();
			while (&iter.peek() != &list.getLast()) iter.skip();
			list.erase(iter);
		}

	}
	template<typename T,  natural citems>
	bool Deque<T,citems>::empty() const {
		return list.empty();
	}
	template<typename T,  natural citems>
	natural Deque<T,citems>::length() const {
		if (list.empty()) return 0;
		return list.size() * citems + tail -  head - citems;
	}

	template<typename T,  natural citems>
	typename Deque<T,citems>::ConstRef Deque<T,citems>::at(natural index) const {
		const Cluster &k = seek(index);
		return k.getBase()[index];
	}
	template<typename T,  natural citems>
	typename Deque<T,citems>::Ref Deque<T,citems>::mutableAt(natural index) {
		Cluster &k = seek(index);
		return k.getBase()[index];
	}
	template<typename T,  natural citems>
	Deque<T,citems> &Deque<T,citems>::set(natural index, ConstRef val) {
		Cluster &k = seek(index);
		k.getBase()[index] = val;
		return *this;
	}
	
	template<typename T,  natural citems>
	void Deque<T,citems>::clear() {
		while (!empty()) popFront();
	}

	template<typename T,  natural citems>
	template<typename Fn>
	bool Deque<T,citems>::forEach(Fn functor, natural from, natural count) const {
		natural idx = from;
		typename List::Iterator iter = seekIter(idx);
		while (count && iter.hasItems()) {
			const Cluster &k = iter.getNext();
			while (idx < citems && count) {
				const T &x = k.getBase()[idx];
				if (functor(x)) return true;
				idx++;
				count--;
			}
			idx = 0;
		}
		return false;
	}

	template<typename T,  natural citems>
	template<typename Impl>
	Deque<T,citems>::Deque(IIterator<T,Impl> &iter)
		:head(0),tail(citems)
	{
		writeBack().copy(iter);
	}
	template<typename T,  natural citems>
	template<typename Impl>
	Deque<T,citems>::Deque(const IIterator<T,Impl> &iter)
		:head(0),tail(citems)
	{
		writeBack().copy(iter);
	}
	
	template<typename T,  natural citems>
	template<typename Impl>
	Deque<T,citems>::Deque(IIterator<T,Impl> &iter,NodeAlloc alloc)
		:head(0),tail(citems),list(alloc)
	{
		writeBack().copy(iter);
	}
	template<typename T,  natural citems>
	template<typename Impl>
	Deque<T,citems>::Deque(const IIterator<T,Impl> &iter,NodeAlloc alloc)
		:head(0),tail(citems),list(alloc)
	{
		writeBack().copy(iter);
	}

	template<typename T,  natural citems>
	Deque<T,citems>::Deque(const Deque &other)
		:head(0),tail(citems),list(other.getAllocator()) {
		other.forEach(Loader(*this));
	}

	template<typename T,  natural citems>
	Deque<T,citems> &Deque<T,citems>::operator=(const Deque &other) {
		clear();
		other.forEach(Loader(*this));
		return *this;
	}

	template<typename T,  natural citems>
	void Deque<T,citems>::addClusterFront() {
		list.insertFirst(Cluster());
		head+=citems;
	}
	template<typename T,  natural citems>
	void Deque<T,citems>::addClusterBack() {
		list.add(Cluster());
		tail-=citems;
	}
	template<typename T,  natural citems>
	typename Deque<T,citems>::Cluster &Deque<T,citems>::seek(natural &idx) {
		typename List::Iterator iter = seekIter(idx);
		Cluster &k = list.getItem();
		return k;
	}
	template<typename T,  natural citems>
	const typename Deque<T,citems>::Cluster &Deque<T,citems>::seek(natural &idx) const {
		typename List::Iterator iter = seekIter(idx);
		const Cluster &k = iter.getNext();
		return k;		
	}

	template<typename T,  natural citems>
	typename Deque<T,citems>::List::Iterator Deque<T,citems>::seekIter(natural &idx) const {
		typename List::Iterator iter = list.getFwIter();
		idx += head;
		while (idx >= citems) {
			idx-=citems;
			iter.skip();
		}
		return iter;
	}


}
#endif /* LIGHTSPEED_CONTAINERS_DEQUE_TCC_ */

