/*
 * stringpool.tcc
 *
 *  Created on: Sep 18, 2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_STRINGPOOL_TCC_
#define LIGHTSPEED_STRINGPOOL_TCC_

#include "stringpool.h"
#include "autoArray.tcc"


namespace LightSpeed {


template<typename T, typename Alloc>
inline typename StringPool<T, Alloc>::Str StringPool<T, Alloc>::add(ConstStringT<T> str) {
	natural curLen = pool.length();
	if (str.length() == 0) return Str(&poolRef,0,0);
	pool.append(str);
	if (StringBase<T>::needZeroChar) {
		typename AutoArray<T, Alloc>::WriteIter iter=pool.getWriteIterator();
		StringBase<T>::writeZeroChar(iter);
	}
	poolRef = pool.data();

	return Str(&poolRef,Bin::natural32(curLen),Bin::natural32(str.length()));
}

template<typename T, typename Alloc>
inline typename StringPool<T, Alloc>::Str StringPool<T, Alloc>::fromMark(natural mark) {
	return Str(&poolRef,Bin::natural32(mark),Bin::natural32(pool.length() - mark));
}

template<typename T, typename Alloc>
inline void StringPool<T, Alloc>::clear() {
	pool.clear();
	poolRef = pool.data();
}

template<typename T, typename Alloc>
inline typename StringPool<T, Alloc>::Str StringPool<T, Alloc>::operator ()(ConstStringT<T> str) {
	return add(str);
}


template<typename T, typename Alloc >
inline void StringPool<T, Alloc>::freeExtra() {
	pool.freeExtra();
	poolRef = pool.data();
}

template<typename T, typename Alloc >
inline natural StringPool<T, Alloc>::mark() {
	return pool.length();
}

template<typename T, typename Alloc >
inline void StringPool<T, Alloc>::release(natural mark) {
	if (mark < pool.length()) {
		pool.resize(mark);
		poolRef = pool.data();
	}
}

template<typename T, typename Alloc >
template<typename X>
inline typename StringPool<T, Alloc>::Str StringPool<T, Alloc>::add(IIterator<T, X>& iter) {
	natural k = pool.length();
	while (iter.hasItems()) pool.add(iter.getNext());
	poolRef = pool.data();
	if (StringBase<T>::needZeroChar) {
		typename
		AutoArray<T, Alloc>::WriteIter iter=pool.getWriteIterator();
		StringBase<T>::writeZeroChar(iter);
	}
	poolRef = pool.data();
	return Str(&poolRef,k,pool.length() - k);
}

template<typename T, typename Alloc >
template<typename X>
inline typename StringPool<T, Alloc>::Str StringPool<T, Alloc>::add(const IIterator<T, X>& iter) {
	X cpy = iter._invoke();
	natural k = pool.length();	
	while (cpy.hasItems()) pool.add(cpy.getNext());
	natural e = pool.length();
	if (StringBase<T>::needZeroChar) {
		typename 
		AutoArray<T, Alloc>::WriteIter iter=pool.getWriteIterator();
		StringBase<T>::writeZeroChar(iter);
	}
	poolRef = pool.data();

	return Str(&poolRef,k,e - k);
}



}



#endif /* LIGHTSPEED_STRINGPOOL_TCC_ */
