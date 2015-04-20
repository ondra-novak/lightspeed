#pragma once

#include "flatArray.h"
#include "../types.h"


namespace LightSpeed {



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
			if (cnt > n) cnt = n;
			for (natural i = 0; i < cnt; i++) array[i] = arr[i];
		}

		CArray(ConstStringT<T> arr) {
			natural cnt = arr.length();
			if (cnt > n) cnt = n;
			for (natural i = 0; i < cnt; i++) array[i] = arr[i];

		}

		template<natural c>
		CArray(const CArray<T,c> &arr) {
			natural cnt = c;
			if (cnt > n) cnt = n;
			for (natural i = 0; i < cnt; i++) array[i] = arr[i];
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



	};
}


