/*
 * string.tcc
 *
 *  Created on: 3.1.2011
 *      Author: ondra
 */

#ifndef LIGTSPEED_CONTAINERS_STRING_TCC_
#define LIGTSPEED_CONTAINERS_STRING_TCC_

#pragma once

#include "string.h"
#include "../streams/utf.h"
#include "../iter/counters.h"
#include "../iter/nullIterator.h"
#include "../iter/iteratorFilter.tcc"
#include "../memory/smallAlloc.h"
#include "autoArray.tcc"
#include "../memory/shareAlloc.h"
#include "../memory/refCntPtr.h"


namespace LightSpeed {





template<typename T>
CompareResult StrCmpCS<T>::operator()(const ConstStringT<T> &a, const ConstStringT<T> &b) const {
	natural la = a.length();
	natural lb = b.length();
	natural c = la < lb?la:lb;
	for (natural i = 0; i < c; i++) {
		const T &ia = a[i];
		const T &ib = b[i];
		if (ia > ib) return cmpResultGreater;
		if (ia < ib) return cmpResultLess;
	}
	if (la > lb) return cmpResultGreater;
	if (la < lb) return cmpResultLess;
	return cmpResultEqual;
}


template<template<class> class StrCmp>
typename UniString<StrCmp>::StrA UniString<StrCmp>::getUtf8() const {

	typedef AutoArrayStream<char, SmallAlloc<1024> > Buff;
	Buff tmpbuff;
	tmpbuff.reserve(utf8length()+1);

	WideToUtf8Writer<Buff &> writr(tmpbuff);
	this->render(writr);


	IRuntimeAlloc &alloc  =this->getAllocator();
	return StrA(tmpbuff.getArray(),alloc);
}

template<template<class> class StrCmp>
natural UniString<StrCmp>::utf8length() const {

	WideToUtf8Reader<typename UniString::Iterator> rd(this->getFwIter());
	natural cnt = 0;
	while (rd.hasItems()) {rd.skip();cnt++;}
	return cnt;
}

template<template<class> class StrCmp>
natural UniString<StrCmp>::utf8length(ConstStrW src)  {

	WideToUtf8Reader<typename ConstStrW::Iterator> rd(src.getFwIter());
	natural cnt = 0;
	while (rd.hasItems()) {rd.skip();cnt++;}
	return cnt;
}


template<typename T>
class ICharConvertOutput {
public:
	virtual void operator()(ConstStringT<T> text) = 0;
	virtual ~ICharConvertOutput() {}
};

template<typename T>
class CharConvertOutput: public ICharConvertOutput<T> {
public:
	typedef AutoArray<T, SmallAlloc<1024> > Buff;
	virtual void operator()(ConstStringT<T> text) {
		buff.append(text);
	}
	virtual ~CharConvertOutput(){}

	Buff buff;
};



void convertWideToCP(ConstStrW input, ICharConvertOutput<char> &output, CodePageDef cp);
void convertCPToWide(ConstStrA input, ICharConvertOutput<wchar_t> &output, CodePageDef cp);

template<template<class> class StrCmp>
typename UniString<StrCmp>::StrA UniString<StrCmp>::getCP(CodePageDef cp) const {


	CharConvertOutput<char> out;

	convertWideToCP(*this,out,cp);
	return StrA(out.buff, this->getAllocator());
}


template<template<class> class StrCmp>
void UniString<StrCmp>::loadUtf8(ConstStrA text) {

	Utf8ToWideReader<ConstStrA::Iterator> rd(text.getFwIter());
	natural cnt = 0;
	while (rd.hasItems()) {rd.skip();cnt++;}

	Utf8ToWideWriter<typename Super::WriteIterator> writr(this->createBufferIter(cnt));
	text.render(writr);
}

template<template<class> class StrCmp>
void UniString<StrCmp>::loadCP(ConstStrA text, CodePageDef cp) {

	CharConvertOutput<wchar_t> out;

	convertCPToWide(text,out,cp);
	*this = UniString(out.buff,this->getAllocator());
}


template<template<class> class StrCmp>
typename UniString<StrCmp>::StrA UniString<StrCmp>::getUtf8(
								ConstStrW src, IRuntimeAlloc &alloc) {

	WideToUtf8Reader<ConstStrW::Iterator> rd1(src.getFwIter());
   	natural cnt = 0;
   	while (rd1.hasItems()) {cnt++;rd1.skip();}

   	StrA o;
   	typename StrA::WriteIterator wr = o.createBufferIter(cnt, alloc);
   	WideToUtf8Reader<ConstStrW::Iterator> rd2(src.getFwIter());
   	wr.copy(rd2);
   	return o;
}


}


#endif /* LIGTSPEED_CONTAINERS_STRING_TCC_ */
