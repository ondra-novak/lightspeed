/*
 * arraySet.h
 *
 *  Created on: 26.8.2010
 *      Author: ondra
 */

#ifndef _LIGHTSPEED_CONTAINERS_ARRAYSET_H_
#define _LIGHTSPEED_CONTAINERS_ARRAYSET_H_

#pragma once

#include "autoArray.h"
#include <functional>

namespace std {
	template<typename T>
	struct less;
}

namespace LightSpeed {

class StdAlloc;

template<typename T, typename Cmp = std::less<T>, typename Allocator=StdAlloc>
class ArraySet: public AutoArray<T,Allocator> {
public:

	enum OrderControl {
		///will detect ordering processing all data in one single cycle
		detectOrder = 0,
		///order the data regardless current ordering
		orderNow,
		///order the data and allow duplicates
		orderNowAllowDups,
		///assume data ordered (may cause problem, if is not true)
		alreadyOrdered

	};

	typedef AutoArray<T,Allocator> Super;

	ArraySet():orderedCount(0),duplicates(false) {}
	explicit ArraySet(const Cmp &cmp, const Allocator &alloc = Allocator())
		:Super(alloc),orderedCount(0),cmp(cmp),duplicates(false) {}

	template<typename Imp>
	explicit ArraySet(IIterator<T,Imp> &dataSrc, const Cmp &cmp = Cmp(),
			OrderControl orderCntr = detectOrder, const Allocator &alloc=Allocator())
		:Super(dataSrc,alloc),orderedCount(0),cmp(cmp),duplicates(false) {
		orderControl(orderCntr);
	}

	explicit ArraySet(const Super &data,OrderControl orderCntr = detectOrder,
			const Cmp &cmp = Cmp())
		:Super(data),orderedCount(0),cmp(cmp),duplicates(false) {
		orderControl(orderCntr);
	}

	template<typename X, typename Impl>
	explicit ArraySet(const ArrayT<X,Impl> &dataSrc,
			const Cmp &cmp = Cmp(),
			OrderControl orderCntr = detectOrder,
			const Allocator &alloc=Allocator())
			:Super(dataSrc,alloc),orderedCount(0),cmp(cmp),duplicates(false) {
		 orderControl(orderCntr);
		}


	template<typename Q>
	class TSearchResult {
		Q item;
		natural pos;
	public:

		TSearchResult(Q item, natural pos):item(item),pos(pos) {}
		Q operator->() const;
		bool found() const;
		natural getPos() const {return pos;}
		operator Q() const {return item;}
	};

	typedef TSearchResult<const T *> ConstSearchResult;
	typedef TSearchResult<T *> SearchResult;

	SearchResult find(const T &obj);
	ConstSearchResult find(const T &obj) const;

	///Enables duplicates
	/** default duplicates are disabled and are discarded during ordering
	 * You can enable duplicates using this function
	 * @param enable set true to enable duplicates
	 *
	 */
	void enableDuplicates(bool enable) {duplicates = enable;}

	///Retrieves, whether duplicates are enabled
	/**
	 * @retval true duplicated items are enabled
	 * @retval false duplicated items are disabled
	 */
	bool duplicatesEnabled() const {return duplicates;}


	ArraySet getOrdered() const;

	void makeOrdered();
	///returns true, if set is whole ordered

	bool isOrdered() const;
	///Creates array, where items are filtered through filter
	/**
	 * @param filter source filter. It should be ordered
	 * @param negfilter negative filter. Items match the filter are removed
	 * @return filterd array;
	 */
	ArraySet filter(const ArraySet &filter, bool negfilter = false) const;

	///Merges two sets
	/** Operation is fastest, when sets are ordered. If
	 *
	 * @param other other set
	 * @return result
	 *
	 * if both sets are ordered, result is also ordered. if
	 * one of sets is not ordered, result is not ordered. If duplicates
	 * are disabled, function use first set as filter. It sorts
	 * filter internally
	 *
	 */
	ArraySet merge(const ArraySet &other) const;


	///Returns set where items from right set are missing
	/**Function works similar to filter, only checks, whether right
	 * set is ordered. If not, it creates temporary ordered set and use
	 * filter function to remove items
	 *
	 * @param other items to remove from the set
	 * @return result
	 */
	ArraySet remove(const ArraySet &other) const;


	///Returns set where are items contained also in right set
	/**Function works similar to filter, only checks, whether right
	 * set is ordered. If not, it creates temporary ordered set and use
	 * filter function to remove items
	 *
	 * @param other items to crop from the set
	 * @return result
	 */
	ArraySet crop(const ArraySet &other) const;

	void insert(natural at, const T &init);

    void insert(natural at, const T &init, natural count);

    void erase(natural at);

    void erase(natural at, natural count);

    void trunc(natural count = 1);

    void clear();

protected:

	natural orderedCount;
	Cmp cmp;
	bool duplicates;

	bool searchOrdered(const T &obj, natural &pos) const;
	bool searchUnordered(const T &obj, natural &pos) const;

	natural checkOrder() const;

	void removeDuplicatesAfterOrder();

	void orderControl(OrderControl cntr);
};

}




#endif /* ARRAYSET_H_ */
