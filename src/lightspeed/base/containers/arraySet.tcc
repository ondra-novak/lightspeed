/*
 * arraySet.tcc
 *
 *  Created on: 26.8.2010
 *      Author: ondra
 */

#include "arraySet.h"
#include "autoArray.tcc"
#include "../iter/iteratorFilter.tcc"
#include "../iter/merge.h"
#include "sort.tcc"
#include <memory>

namespace LightSpeed {


template<typename T, typename Cmp, typename Allocator>
typename ArraySet<T,Cmp,Allocator>::SearchResult ArraySet<T,Cmp,Allocator>::find(const T & obj)
{
	natural pos;
	natural pos2;
	if (searchOrdered(obj,pos)) return SearchResult(&this->mutableAt(pos),pos);
	else if (searchUnordered(obj,pos2)) return SearchResult(&this->mutableAt(pos),pos2);
	else return SearchResult(0,pos);
}



template<typename T, typename Cmp, typename Allocator>
typename ArraySet<T,Cmp,Allocator>::ConstSearchResult ArraySet<T,Cmp,Allocator>::find(const T & obj) const
{
	natural pos;
	natural pos2;
	if (searchOrdered(obj,pos)) return ConstSearchResult(&this->at(pos),pos);
	else if (searchUnordered(obj,pos2)) return ConstSearchResult(&this->at(pos),pos2);
	else return ConstSearchResult(0,pos);
}

template<typename T, typename Cmp>
class SortedMergeFunctor: public MergeFunctor<T> {
public:
	SortedMergeFunctor(const Cmp &cmp, bool duplicates)
		:cmp(cmp),duplicates(duplicates) {}

    MergeSplitResult<T> operator()(const T *left, const T *right) {
    	if (left == 0) return MergeSplitResult<T>::commitR(*right);
    	else if (right == 0) return MergeSplitResult<T>::commitL(*left);
    	else if (cmp(*left,*right)) return MergeSplitResult<T>::commitL(*left);
    	else if (duplicates || cmp(*right,*left))
								return MergeSplitResult<T>::commitR(*right);
    	else return MergeSplitResult<T>::commitB(*left);
    }

protected:
	Cmp cmp;
	bool duplicates;
};

template<typename Cmp, typename Iter1, typename Iter2>
static MergeIterator<Iter1,Iter2,SortedMergeFunctor<typename Iter1::ItemT, Cmp> >
	createMergeIter(Iter1 iter1, Iter2 iter2, Cmp cmp,bool dups) {
		return MergeIterator<Iter1,Iter2,SortedMergeFunctor<typename Iter1::ItemT, Cmp> >(
				iter1, iter2, SortedMergeFunctor<typename Iter1::ItemT,Cmp>(cmp,dups)
		);

}

template<typename T, typename Cmp, typename Allocator>
ArraySet<T,Cmp,Allocator> ArraySet<T,Cmp,Allocator>::getOrdered() const
{
	return ArraySet(*this,cmp,*this,true);
}

template<typename T, typename Cmp, typename Allocator>
void ArraySet<T,Cmp,Allocator>::makeOrdered()
{
		//we don't need to order empty sets
		if (this->empty()) return;
		//one item in array is always ordered
		if (orderedCount == 0) orderedCount++;
		//nothing to order, exit
		if (orderedCount >= this->length()) return;
		//only few items are unordered? - make ordered by inserting
		if (this->length() - orderedCount < 30) {
			//process unordered part
			for (natural i = orderedCount, cnt = this->length(); i < cnt; i++) {
				natural x = i;
				while (x>0 && !cmp((*this)[x-1],(*this)[x])) {
					std::swap((*this)(x-1),(*this)(x));
					x--;
				}
			}
			orderedCount = this->length();
		} else  {
			HeapSort<ArraySet,Cmp> heap(*this,cmp);
			heap.sort();
			if (!duplicates) removeDuplicatesAfterOrder();
		}
		orderedCount = this->length();
}

template<typename T, typename Cmp, typename Allocator>
bool ArraySet<T,Cmp,Allocator>::isOrdered() const {
	return orderedCount == this->length();
}

template<typename T, typename SrcTable>
class ArraySetFilter: IteratorFilterBase<T,T,ArraySetFilter<T,SrcTable> > {
public:
	ArraySetFilter(const SrcTable &filterTable, bool negate)
		:filterTable(filterTable),negate(negate),itemPtr(0) {}

	bool needItems() const {return itemPtr == 0;}
	void input(const T &p) {
		if (filterTable.find(p).found() != negate)
			itemPtr = &p;
	}
	bool hasItems() const {return itemPtr != 0;}
	T output() {
		if (itemPtr) return *itemPtr;
		else throwIteratorNoMoreItems(THISLOCATION,typeid(T));
		throw;
	}

protected:
	const SrcTable &filterTable;
	bool negate;
	const T *itemPtr;
};

template<typename T, typename Cmp, typename Allocator>
ArraySet<T,Cmp,Allocator> ArraySet<T,Cmp,Allocator> ::filter(
		const ArraySet<T,Cmp,Allocator> &flt, bool negfilter) const {

	typedef ArraySetFilter<T,ArraySet> MyFilter;
	typename Filter<MyFilter>::template Read<typename Super::Iterator> iter(
						this->getFwIter(),MyFilter(flt,negfilter));
	return ArraySet(iter, cmp, *this);
}

template<typename T>
class ConcatMergeFunctor: public MergeFunctor<T> {
public:

    MergeSplitResult<T> operator()(const T *left, const T *right) {
    	if (left != 0) return MergeSplitResult<T>::commitL(*left);
    	else if (right != 0) return MergeSplitResult<T>::commitR(*right);
    	else return MergeSplitResult<T>::stop();
    }

};

template<typename T, typename Cmp, typename Allocator>
ArraySet<T,Cmp,Allocator> ArraySet<T,Cmp,Allocator>::merge(const ArraySet &other) const {

	typedef typename Super::Iterator SIter;

	//if both sets are ordered
	//we can make the fastest merge with complexity O(N+M)
	if (isOrdered() && other.isOrdered()) {
		//prepare merge iterator
		typedef MergeIterator<SIter,SIter,SortedMergeFunctor<T,Cmp> > MergeIter;

		//prepare functor
		SortedMergeFunctor<T,Cmp> functor(cmp,duplicates);
		//get first iterator
		SIter iter1 = this->getFwIter();
		//get second iterator
		SIter iter2 = other.getFwIter();
		//create merge iterator
		MergeIter miter(iter1,iter2,functor);
		//merge to result
		return ArraySet(miter,cmp,*this);
	} else {
		//concat functor - simple concatenates two streams
		ConcatMergeFunctor<T> ccat;
		//if duplicates are allowed
		if (duplicates) {
			//we don't care about duplicates
			//simply append second set after first
			typedef MergeIterator<SIter, SIter, ConcatMergeFunctor<T> > MergeIter;
			SIter iter1 = this->getFwIter();
			SIter iter2 = other.getFwIter();
			MergeIter miter(iter1,iter2,ccat);
			//merge to result
			return ArraySet(miter,cmp,*this);

		}
		//prepare filter types to filter out duplicates
		typedef ArraySetFilter<T,ArraySet> MyFilter;
		typedef typename Filter<MyFilter>::template Read<SIter> FltIter;

		//if true, first is filtered out, otherwise second.
		bool mode;
		//temporary table, may be used for filter table
		ArraySet tmp(cmp,*this);
		//pointer to set that defines the filter
		const ArraySet *refTable;

		if (isOrdered()) {
			//take benefit that first set is ordered
			mode = true;
			//use the first set to filter out
			refTable = this;
		} else if (other.isOrdered()) {
			mode = false;
			refTable = &other;
		} else if (this->length() < other.length()) {
			tmp = *this;
			refTable = &tmp;
			mode = true;
		} else {
			tmp = other;
			refTable = &tmp;
			mode = false;
		}

		MyFilter flt(*refTable,true);

		if (mode) {
			typedef MergeIterator<SIter, FltIter, ConcatMergeFunctor<T> > MergeIter;
			SIter iter1 = this->getFwIter();
			FltIter iter2(other.getFwIter(), flt);
			MergeIter miter(iter1,iter2,ccat);
			return ArraySet(miter,cmp,*this);
		} else {
			typedef MergeIterator<FltIter,SIter, ConcatMergeFunctor<T> > MergeIter;
			SIter iter2 = other.getFwIter();
			FltIter iter1(this->getFwIter(), flt);
			MergeIter miter(iter1,iter2,ccat);
			return ArraySet(miter,cmp,*this);
		}
	}

}


template<typename T, typename Cmp, typename Allocator>
ArraySet<T,Cmp,Allocator> ArraySet<T,Cmp,Allocator>::remove(const ArraySet &other) const {

	if (!other.isOrdered()) {
		return remove(other.getOrdered());
	}
	return filter(other,true);

}


template<typename T, typename Cmp, typename Allocator>
ArraySet<T,Cmp,Allocator> ArraySet<T,Cmp,Allocator>::crop(const ArraySet &other) const {

	if (!other.isOrdered()) {
		return remove(other.getOrdered());
	}
	return filter(other,false);

}

template<typename T, typename Cmp, typename Allocator>
void ArraySet<T,Cmp,Allocator>::insert(natural at, const T &init) {
	Super::insert(at,init);
	if (orderedCount > at) {
		if (at > 0 && !cmp((*this)[at-1],(*this)[at]))
			orderedCount = at;
		else if (at+1 < this->length() && !cmp((*this)[at],(*this)[at+1]))
			orderedCount = at;
		else
			orderedCount++;
	}
}

template<typename T, typename Cmp, typename Allocator>
void ArraySet<T,Cmp,Allocator>::insert(natural at, const T &init, natural count) {
	if (count == 1) insert(at,init);
	else {
		Super::insert(at,init,count);
		orderedCount = at;

	}
}

template<typename T, typename Cmp, typename Allocator>
void ArraySet<T,Cmp,Allocator>::erase(natural at) {
	Super::erase(at);
	if (orderedCount > at) orderedCount--;
}

template<typename T, typename Cmp, typename Allocator>
void ArraySet<T,Cmp,Allocator>::erase(natural at, natural count) {
	Super::erase(at);
	if (orderedCount > at+count) orderedCount-=count;
	else if (orderedCount > at) orderedCount = at;
}

template<typename T, typename Cmp, typename Allocator>
void ArraySet<T,Cmp,Allocator>::trunc(natural count) {
	Super::trunc(count);
	if (orderedCount > this->length()) orderedCount = this->length();
}

template<typename T, typename Cmp, typename Allocator>
void ArraySet<T,Cmp,Allocator>::clear() {
	Super::clear();
	orderedCount = 0;
}

template<typename T, typename Cmp, typename Allocator>
bool ArraySet<T,Cmp,Allocator>::searchOrdered(const T &obj, natural &pos) const {
	natural l = 0, r = std::min(orderedCount,this->length());
	while (l < r) {
		natural m = (l+r)>>1;
		const T &v = (*this)[m];
		if (cmp(obj,v)) {
			r = m;
		}
		else if (cmp(v,obj)) {
			l = m+1;
		}
		else {
			pos = m;
			return true;
		}
	}
	pos = l;
	return false;
}

template<typename T, typename Cmp, typename Allocator>
bool ArraySet<T,Cmp,Allocator>::searchUnordered(const T &obj, natural &pos) const {
	for (natural l = std::min(orderedCount,this->length()),m=this->length();
			l < m; l++) {
		const T &v = (*this)[l];
		if (!cmp(obj,v) && !cmp(v,obj)) {
			pos = l;
			return true;
		}
	}
	pos = this->length();
	return false;
}

template<typename T, typename Cmp, typename Allocator>
natural ArraySet<T,Cmp,Allocator>::checkOrder() const {
	natural orderedCount = 0;
	natural s = this->length()-1;
	while (orderedCount < s) {
		if (!cmp((*this)[orderedCount],(*this)[orderedCount+1])) break;
		orderedCount++;
	}
	return orderedCount;
}

template<typename T, typename Cmp, typename Allocator>
void ArraySet<T,Cmp,Allocator>::removeDuplicatesAfterOrder() {
	if (this->empty() || orderedCount < this->length()) return;
	natural a = 1;
	natural b = 1;
	natural s = this->length();
	const T *ref = &(*this)[0];
	while (a < s) {
		const T &v = (*this)[b];
		bool dup = !cmp(*ref,v);
		if (!dup) {
			if (a > b) {
				moveObject(&(*this)(a),&(*this)(b));
			}
			b++;
		} else {
			(*this)(a).~T();
		}
		a++;
		ref = &(*this)[b-1];
	}
	this->usedSz = b;
	orderedCount = b;
}

template<typename T, typename Cmp, typename Allocator>
void ArraySet<T,Cmp,Allocator>::orderControl(OrderControl cntr) {
	switch(cntr) {
		case detectOrder: orderedCount = checkOrder();break;
		case orderNow: makeOrdered();break;
		case orderNowAllowDups: duplicates = true; makeOrdered();break;
		case alreadyOrdered: orderedCount = this->length();break;
	}
}


}

