#pragma once
#include "../actions/vararg.h"
#include "carray.h"



namespace LightSpeed {

template class CArray<int,10>;

template<typename T, typename H, typename B>
struct CArrayLoader {
	natural operator()(T *array, const VarArg<H,B> &args, natural limit) {
		CArrayLoader<T,typename VarArg<H,B>::OrgNext::ItemT,typename VarArg<H,B>::OrgNext::Next> ldr;
		natural k = ldr(array,args.next,limit) + 1;
		if (k < limit) array[k] = args.value;
		return k;
	}
};


template<typename T>
struct CArrayLoader<T,void,void> {
	natural operator()(T *array, const VarArg<void,void> &args, natural limit) {
		return naturalNull;
	}
};
/*

template<typename H,typename B, typename T, natural n>
natural CArray<T,n>::loadArray(const VarArg<H,B> &args) {
	natural k = loadArray(args.next) + 1;
	array[k] = args.value;
	return k;

}

template<typename T, natural n>
natural CArray<T,n>::loadArray(const VarArg<void,void> &args) {
	return  naturalNull;

}
*/

}
