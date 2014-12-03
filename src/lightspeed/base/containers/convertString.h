#pragma once
#include "../iter/iteratorFilter.h"
#include "stringBase.h"

namespace LightSpeed {

	///Convert string to another using filter
	template<typename Flt, typename In, typename Out>
	StringCore<Out> convertString(const IIteratorFilter<In,Out,Flt> &fltin, ConstStringT<In> input, IRuntimeAlloc &alloc) {

		//calculate required space;
		natural reqSpace = 0;
		{
			Flt flt = fltin._invoke();
			bool noexit = true;
			for (typename ConstStringT<In>::Iterator iter = input.getFwIter(); iter.hasItems() && noexit;) {
				noexit = false;
				while (flt.needItems() && iter.hasItems()) {flt.input(iter.getNext());noexit = true;}
				while (flt.hasItems()) {flt.output();reqSpace++;noexit = true;}
			}
			flt.flush();
			while (flt.hasItems()) {flt.output();reqSpace++;}
		}

		StringCore<Out> res;
		typename StringCore<Out>::WriteIterator writer = res.createBufferIter(reqSpace,alloc);

		{		
			Flt flt = fltin._invoke();
			bool noexit = true;
			for (typename ConstStringT<In>::Iterator iter = input.getFwIter(); iter.hasItems() && noexit;) {
				noexit = false;
				while (flt.needItems() && iter.hasItems()) {flt.input(iter.getNext());noexit = true;}
				while (flt.hasItems()) {writer.write(flt.output());noexit = true;}
			}
			flt.flush();
			while (flt.hasItems()) writer.write(flt.output());		
		}
		return res;
	}

	template<typename Flt, typename In, typename Out>
	StringCore<Out> convertString(const IIteratorFilter<In,Out,Flt> &fltin, ConstStringT<In> input) {
		return convertString(fltin,input,getStringDefaultAllocator());
	}


}
