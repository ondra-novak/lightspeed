#pragma once

#include "flatArray.h"
#include "../types.h"


namespace LightSpeed {

	template<typename A,typename B> struct VarArg;


	template<typename T, typename H, typename B>
	struct CArrayLoader;


	template<typename T, natural n>
	class CArray: public FlatArrayBase<T, CArray<T, n> > {
	public:

		typedef FlatArrayBase<T, CArray> Super;
		typedef typename Super::ItemT ItemT;
		typedef typename Super::ConstItemT ConstItemT;

		CArray() {}

		CArray(ConstItemT *arr) {
			for (natural i = 0; i < n; i++) array[i] = arr[i];
		}

		CArray(ConstItemT *arr, natural cnt) {
			if (cnt > n) throwRangeException_To(THISLOCATION,n,cnt);
			for (natural i = 0; i < cnt; i++) array[i] = arr[i];
		}

		CArray(ConstStringT<T> arr) {
			if (arr.length() > n) throwRangeException_To(THISLOCATION,n,arr.length());
			for (natural i = 0; i < arr.length(); i++) array[i] = arr[i];

		}


		template<typename H,typename B>
		CArray(const VarArg<H,B> &args) {
			CArrayLoader<T,H,B> ldr;
			ldr(array,args,n);
		}

		static natural length()  {
			return n;
		}

		ConstItemT *data() const {
			return array;
		}

		ItemT *data() {
			return array;
		}

	protected:

		T array[n];

/*
		template<typename H,typename B>
		natural loadArray(const VarArg<H,B> &args);
		natural loadArray(const VarArg<void,void> &args);;
*/


	};
}


