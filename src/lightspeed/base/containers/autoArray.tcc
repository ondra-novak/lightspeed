/*
 * autoArray.tcc
 *
 *  Created on: 29.7.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_AUTOARRAY_TCC_
#define LIGHTSPEED_CONTAINERS_AUTOARRAY_TCC_

#include "autoArray.h"
#include "../exceptions/stdexception.h"
#include "../exceptions/throws.tcc"
#include "../objmanip.h"


namespace LightSpeed {

template<typename T, typename Alloc>
AutoArrayT<T,Alloc>::AutoArrayT():allocBk(Alloc(),0),usedSz(0) {}


template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::addExtra(natural items) {
	//calculate final count of items
	natural fin = usedSz + items;
	//get current available space
	natural curcount = allocBk.getSize();
	//if required space is above current?
	if (fin > curcount) {
		//try expand block first
		if (!allocBk.expand(fin)) {
			//if failed, calculate reallocation size
			natural max = usedSz*3/2;
			//if required count if above reallocation size, use required count
			if (fin > max) max = fin;
			//allocate temporary block
			AllocatedMemory tmp(allocBk,max);
			//prepare empty block
			AllocatedMemory newblk(allocBk,0);
			//test, whether we are able to move block from temp to new
			if (tmp.move(newblk)) {
					//if we are able, move all items from old block to new block
					move(allocBk.getBase(),usedSz,newblk.getBase());
					//finally, move new block to old instance
					newblk.move(allocBk);
			} else {
				//when we are unable to move newly created block
				throwAllocatorLimitException(THISLOCATION,max,allocBk.getSize(),typeid(AllocatedMemory));
			}
		}
	}

}
template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::freeExtra() {
	if (usedSz < allocBk.getSize()) {
		if (!allocBk.shrink(usedSz)) {
			//if failed, calculate reallocation size
			natural max = usedSz;
			//allocate temporary block
			AllocatedMemory tmp(allocBk,max);
			//prepare empty block
			AllocatedMemory newblk(allocBk,0);
			//test, whether we are able to move block from temp to new
			if (tmp.move(newblk)) {
					//if we are able, move all items from old block to new block
					move(allocBk.getBase(),usedSz,newblk.getBase());
					//finally, move new block to old instance
					newblk.move(allocBk);
			}
		}
	}
}



template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::reserve(natural items) {
	if (items > usedSz) addExtra(items - usedSz);
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::add(const T &item) {
	addExtra(1);
	new((void *)(allocBk.getBase()+usedSz)) T(item);
	++usedSz;

}


template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::add(const T &item, natural count) {
	addExtra(count);
	for (natural i = 0; i < count; i++) {
    	new((void *)(allocBk.getBase()+usedSz)) T(item);
    	++usedSz;
	}
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::resize(natural items) {
	if (items > usedSz) {
		items -= usedSz;
		addExtra(items);
		construct<T>(allocBk.getBase()+usedSz,items);
		usedSz += items;
	} else if (items < usedSz) trunc(usedSz - items);
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::resize(natural items, const T &init) {
	if (items > usedSz) add(init, items - usedSz);
	else if (items < usedSz) trunc(usedSz - items);
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::insert(natural at, const T &init) {
	return insert(at,init,1);
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::insert(natural at, const T &init, natural count) {
	if (at > usedSz)
		throwRangeException_FromTo<natural>(THISLOCATION,0,usedSz,at);
	if (at == usedSz)
		return add(init, count);
	WriteInsertIter writ(*this,at,count);
	while (count--) writ.write(init);
}

template<typename T, typename Alloc>
template<typename A, typename B>
void AutoArrayT<T,Alloc>::insertArray(natural at, const ArrayT<A,B> &items) {
	if (at > usedSz)
		throwRangeException_FromTo<natural>(THISLOCATION,0,usedSz,at);
	if (at == usedSz)
		return append(items);
	WriteInsertIter writ(*this,at,items.length());
	writ.copy(items.getFwIter());
}

template<typename T, typename Alloc>
template<typename A, typename B>
void AutoArrayT<T,Alloc>::append(IIterator<A,B> &arr) {
	while (arr.hasItems()) add(arr.getNext());
}

template<typename T, typename Alloc>
template<typename A, typename B>
void AutoArrayT<T,Alloc>::append(IIterator<A,B> &arr, natural limit) {
	while (arr.hasItems() && limit) {add(arr.getNext());--limit;}
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::erase(natural at) {
	return erase(at,1);
}


template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::erase(const typename Super::Iterator &iter) {
	return erase(iter.tell());
}


template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::erase(natural at, natural count) {

    if (at > usedSz) {
		throwRangeException_FromTo<natural>(THISLOCATION,0,usedSz,at);
    }
	if (at+count > usedSz) count = usedSz - at;
	if (count) {
		destruct<T>(allocBk.getBase()+at,count);
		unreserveAt(at,count);
	}
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::trunc(natural count) {
	if (count >= usedSz) count = usedSz;
	erase(usedSz - count, count);
}


template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::clear() {
	trunc(usedSz);
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::reserveAt(natural pos, natural count) {

	//add space for new items
	this->addExtra(count);
	try {
		//move items at position
		move(allocBk.getBase()+pos,usedSz - pos,allocBk.getBase()+pos+count);
	} catch (...) {

	}
	//update size of array
	usedSz+=count;
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::unreserveAt(natural pos, natural count) {

	try {
		//move items at position
		move(allocBk.getBase()+pos+count,usedSz - pos - count,allocBk.getBase()+pos);
	} catch (...) {
		throw;
	}

	usedSz-=count;
}


template<typename T, typename Alloc>
AutoArrayT<T,Alloc>::AutoArrayT(const AutoArrayT &arr)
			:allocBk(arr.allocBk,arr.usedSz),usedSz(0) {
    append(arr);
}

template<typename T, typename Alloc>
AutoArrayT<T,Alloc> &AutoArrayT<T,Alloc>::operator=(const AutoArrayT &arr) {
    clear();
    natural sz = arr.length();
    if (sz > 0) {
        reserve(sz);
        WriteIter writr(*this);
        arr.render(writr);
    }
    return *this;
}

template<typename T, typename Alloc>
AutoArrayT<T, Alloc>::~AutoArrayT() {
  clear();
}

template<typename T, typename Alloc>
void AutoArrayT<T,Alloc>::swap(AutoArrayT<T,Alloc> &other) {

	if (!allocBk.swap(other.allocBk)) {
		if (other.length() > length()) other.swap(*this);
		other.reserve(length());
		::LightSpeed::swap(data(),other.data(),other.length());
		if (other.length() < length()) {
			::LightSpeed::move(data()+other.length(),length() - other.length(),other.data()+other.length());
		} else  if (other.length() > length()) {
			::LightSpeed::move(other.data()+other.length(),length() - other.length(),data()+other.length());
		}

	}
	std::swap(usedSz,other.usedSz);

}

template<typename T, typename Alloc>
template<typename A, typename B>
void AutoArrayT<T,Alloc>::append(const ArrayT<A,B> &arr) {
    addExtra(arr.length());
    typename ArrayT<A,B>::Iterator iter = arr.getFwIter();
    while (iter.hasItems()) {
        add(iter.getNext());
    }
}


template<typename T, typename Alloc>
AutoArray<T, Alloc >::AutoArray(natural count, const T &val, const Alloc &alloc):Super(alloc) {
	Super::resize(count,val);
}



}


#endif /* AUTOARRAY_TCC_ */
