/*
 * textOut.tcc
 *
 *  Created on: 24.12.2010
 *      Author: ondra
 */


#ifndef LIGHTSPEED_TEXT_TEXTFORMAT_TCC_
#define LIGHTSPEED_TEXT_TEXTFORMAT_TCC_


#pragma once

#include "textFormat.h"
#include "../containers/autoArray.tcc"
#include "../exceptions/throws.tcc"
#include "../streams/utf.h"
#include "../iter/iteratorFilter.tcc"
#include "toString.tcc"


namespace LightSpeed {



template<typename Impl>
Impl &TextFormatBase<Impl>::operator()(ConstStringT<char> pattern) {
	return this->_invoke().setPatternChar(pattern);
}
template<typename Impl>
Impl &TextFormatBase<Impl>::operator()(const char *pattern) {
	return this->_invoke().setPatternChar(pattern);
}

template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(ConstStringT<char> text) {
	return this->_invoke().setArgChar(text);
}
template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(const char *text) {
	return this->_invoke().setArgChar(text);
}
template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(const std::string &text) {
	return this->_invoke().setArgChar(text.c_str());
}

template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(char chr) {
	return this->_invoke().setArgChar(chr);
}


template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(ConstStringT<wchar_t> text) {
	return this->_invoke().setArgWChar(text);
}
template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(const wchar_t *text) {
	return this->_invoke().setArgWChar(text);
}
template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(const std::wstring &text) {
	return this->_invoke().setArgWChar(text.c_str());
}

template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(wchar_t chr) {
	return this->_invoke().setArgWChar(chr);
}

template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(natural num) {
	return this->_invoke().setArgNum(num);
}
template<typename Impl>
Impl &TextFormatBase<Impl>::operator <<(integer num) {
	return this->_invoke().setArgNum(num);
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator()(ConstStringT<T> pattern) {
	this->pattern = pattern;
	resetArgs();
	return *this;
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator()(const T *pattern) {
	this->pattern = pattern;
	resetArgs();
	return *this;
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator()() {
	this->pattern = ConstStringT<T>();
	resetArgs();
	return *this;
}


template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator<<(ConstStringT<T> text) {
	paramList.add(ParamItem(text.data(),text.length()));
	return *this;
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator << (const T *text) {
	return operator<<(ConstStringT<T>(text));
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator << (const T c) {
	buff.add(c);
	paramList.add(buff.length());
	return *this;
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator << (lnatural num) {
	char buff[100];
	ConstStrA txt = _intr::numberToString(num,buff,100,base);
	if (formatter != nil) txt = (*formatter)(txt);
	return setArgChar(txt);
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator << (int num) {
	char buff[100];
	ConstStrA txt = _intr::numberToString(num,buff,100,base);
	if (formatter != nil) txt = (*formatter)(txt);
	return setArgChar(txt);
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator << (unsigned int num) {
	char buff[100];
	ConstStrA txt = _intr::numberToString(num,buff,100,base);
	if (formatter != nil) txt = (*formatter)(txt);
	return setArgChar(txt);
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator << (linteger num) {
	char buff[100];
	ConstStrA txt = _intr::numberToString(num,buff,100,base);
	if (formatter != nil) txt = (*formatter)(txt);
	return setArgChar(txt);
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator << (float num) {
	ToString<float> str(num,decimals,sciFormat);
	ConstStrA txt =  str;
	if (formatter != nil) txt = (*formatter)(txt);
	return setArgChar(txt);
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::operator << (double num) {
	ToString<double> str(num,decimals,sciFormat);
	ConstStrA txt =  str;
	if (formatter != nil) txt = (*formatter)(txt);
	return setArgChar(txt);
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setBase(natural b) {
	base = b;
	return *this;
}
template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setPrecision(natural p) {
	decimals = p;
	return *this;
}

template<typename T, typename Alloc>
TextFormat<T,Alloc>  &TextFormat<T,Alloc>::setFixed(natural p) {
	sciFormat = false;
	decimals = p;
	return *this;
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setSci(natural p) {
	sciFormat = true;
	decimals = p;
	return *this;
}


template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setNumberFormat(INumberFormatter *fmt) {
	formatter = fmt;
	return *this;
}

template<typename T>
struct FormatInfo {
	natural space;
	const T *fillchar;
	bool alignRight;
	bool forceSign;
	bool signBeforeFill;

	FormatInfo():space(8),fillchar(0),alignRight(false),forceSign(false),signBeforeFill(false) {}
};

template<typename T, typename Iter>
bool parseFormatInfo(Iter &iter, FormatInfo<T> &nfo) {
	const T &x = iter.peek();
	if (x != '{') return false;
	iter.skip();							// {
	if (!iter.hasItems()) return false;
	char z = (char)iter.peek();
	if (z == '+') {
		nfo.forceSign = true;
		nfo.alignRight = true;
		iter.skip();						// +
		if (!iter.hasItems()) return false;
		z = (char)iter.peek();
	} else if (z == '-') {
		nfo.alignRight = true;
		nfo.forceSign = false;
		iter.skip();						// -
		if (!iter.hasItems()) return false;
		z = (char)iter.peek();
	} else {
		nfo.forceSign = false;
		nfo.alignRight = false;
	}
	if (z < '1' || z > '9') {
		const T &ff = iter.getNext();		//fillchar
		nfo.fillchar = &ff;
		if (!iter.hasItems()) return false;
		z = (char)iter.peek();
		nfo.signBeforeFill = nfo.alignRight;
		nfo.alignRight = true;
	} else {
		nfo.signBeforeFill = false;
		nfo.fillchar = 0;
	}


	natural sp = 0;
	while (z>='0' && z<='9') {
		sp = sp * 10 + (z - '0');
		iter.skip();							// 0-9
		if (!iter.hasItems()) return false;
		z = (char)iter.peek();
	}
	if (z != '}') {
		const T &ff = iter.getNext();		//fillchar
		nfo.fillchar = &ff;
		if (!iter.hasItems()) return false;
		z = (char)iter.peek();
	}
	if (z != '}') return false;
	iter.skip();								// }
	nfo.space = sp;
	return true;
}

template<typename T, typename Alloc>
template<typename WriteIter>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::output(WriteIter &iter) {

	if (pattern.empty()) {
		buff.render(iter);
	} else {

		typedef typename Pattern::Iterator RdIter;
		RdIter rditer = pattern.getFwIter();
		while (rditer.hasItems()) {
			const T &x = rditer.getNext();
			if (x == '\n') {
				newline.render(iter);
			} else if (x != '%') {
				iter.write(x);
			} else if (!rditer.hasItems()) {
				iter.write(x);
			} else if (rditer.peek() == '%' || rditer.peek() == '\n') {
				iter.write(rditer.getNext());
			} else {
				const T &y = rditer.peek();
				FormatInfo<T> nfo;
				bool formatted = false;
				if (y == '{') {
					formatted = parseFormatInfo(rditer,nfo);
				}
				natural idx = 0;
				while (rditer.hasItems() && rditer.peek() >= '0' && rditer.peek() <= '9') {
					char c = (char)rditer.getNext();
					idx = idx * 10 + (c - '0');
				}
				if (rditer.hasItems() && rditer.peek() == '%') rditer.skip();
				ConstStringT<T> fragment;
				if (idx >= 1 && idx <= paramList.length()) {
					idx--;
					const ParamItem &itm = paramList[idx];
					if (itm.isDirectRef())
						fragment = ConstStringT<T> (itm.ptr, itm.length);
					else if (idx == 0)
						fragment = buff.head(itm.length);
					else {
						natural to = itm.length;
						natural from = 0;
						natural p = idx;
						do {
							p--;
							if (!paramList[p].isDirectRef()) {
								from = paramList[p].length;
								break;
							}
						} while (p != 0);
						fragment = buff.mid(from,to - from);
					}
				}
				if (!formatted) {
					fragment.render(iter);
				} else {
					natural rnsp;
					if (nfo.space < fragment.length()) rnsp = 0;
					else rnsp = nfo.space - fragment.length();
					bool hasSign = !fragment.empty() && (fragment[0] == '-' || fragment[0] == '+');
					bool reserveSign = nfo.forceSign || (!fragment.empty() && fragment[0] == '-');
					if (reserveSign && rnsp>0 && !hasSign) rnsp--;
					if (nfo.signBeforeFill && reserveSign) {
						if (hasSign) iter.write(fragment[0]);
						else iter.write('+');
					}

					if (nfo.alignRight) {
						if (nfo.fillchar)
							for (natural x = 0; x < rnsp; x++) iter.write(*nfo.fillchar);
						else
							for (natural x = 0; x < rnsp; x++) iter.write(' ');
					}
					if (!nfo.signBeforeFill && reserveSign) {
						if (hasSign) iter.write(fragment[0]);
						else iter.write('+');
					}
					if (reserveSign && hasSign) {fragment = fragment.offset(1);}
					fragment.render(iter);
					if (!nfo.alignRight) {
						if (nfo.fillchar)
							for (natural x = 0; x < rnsp; x++) iter.write(*nfo.fillchar);
						else
							for (natural x = 0; x < rnsp; x++) iter.write(' ');
					}
				}
			}
}
	}
	resetArgs();
	return *this;
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setPatternChar(const char *text) {
	if (text == 0) throwNullPointerException(THISLOCATION);
	patternBuffer.clear();
	_intr::LoadCharToBuff<T>::load(ConstStrA(text),patternBuffer);
	return (*this)(patternBuffer);
}
template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setPatternChar(ConstStringT<char> text) {
	patternBuffer.clear();
	_intr::LoadCharToBuff<T>::load(text,patternBuffer);
	return (*this)(patternBuffer);

}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setArgChar(const char *text) {
	if (text == 0) throwNullPointerException(THISLOCATION);
	_intr::LoadCharToBuff<T>::load(ConstStrA(text),buff);
	paramList.add(buff.length());
	return *this;
}
template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setArgChar(ConstStringT<char> text) {
	_intr::LoadCharToBuff<T>::load(text,buff);
	paramList.add(buff.length());
	return *this;

}
template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setArgChar(char chr) {
	_intr::LoadCharToBuff<T>::load(ConstStrA(&chr,1),buff);
	paramList.add(buff.length());
	return *this;
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setArgWChar(const wchar_t *text) {
	if (text == 0) throwNullPointerException(THISLOCATION);
	_intr::LoadCharToBuff<T>::load(ConstStrW(text),buff);
	paramList.add(buff.length());
	return *this;
}
template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setArgWChar(ConstStringT<wchar_t> text) {
	_intr::LoadCharToBuff<T>::load(text,buff);
	paramList.add(buff.length());
	return *this;

}
template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setArgWChar(wchar_t chr) {
	_intr::LoadCharToBuff<T>::load(ConstStrW(&chr,1),buff);
	paramList.add(buff.length());
	return *this;
}

template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setArgNum(natural num) {
	char buff[100];
	ConstStrA txt = _intr::numberToString(num,buff,100,base);
	if (formatter != nil) txt = (*formatter)(txt);
	return setArgChar(txt);
}
template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setArgNum(integer num) {
	char buff[100];
	ConstStrA txt = _intr::numberToString(num,buff,100,base);
	if (formatter != nil) txt = (*formatter)(txt);
	return setArgChar(txt);

}
template<typename T, typename Alloc>
TextFormat<T,Alloc> &TextFormat<T,Alloc>::setNL(const ConstStringT<T> &nl) {
	newline = nl;
	return *this;
}

namespace _intr {

	template<typename T>
	template<typename Alloc>
	void LoadCharToBuff<T>::load(ConstStrA text, AutoArray<T,Alloc> &out) {
		for (ConstStrA::Iterator iter = text.getFwIter(); iter.hasItems();)
			out.add(iter.getNext());
	}

	template<typename Alloc>
	void LoadCharToBuff<wchar_t>::load(ConstStrA text, AutoArray<wchar_t,Alloc> &out) {
		Utf8ToWideReader<ConstStrA::Iterator> iter(text.getFwIter());
		iter.enableSkipInvalidChars(true);
		while (iter.hasItems()) {
			out.add(iter.getNext());
		}
	}

	template<typename Alloc>
	void LoadCharToBuff<wchar_t>::load(ConstStrW text, AutoArray<wchar_t,Alloc> &out) {
		ConstStrW::Iterator iter(text.getFwIter());
		while (iter.hasItems()) {
			out.add(iter.getNext());
		}
	}

	template<typename Alloc>
	void LoadCharToBuff<char>::load(ConstStrW text, AutoArray<char,Alloc> &out) {
		WideToUtf8Reader<ConstStrW::Iterator> iter(text.getFwIter());
		while (iter.hasItems()) {
			out.add(iter.getNext());
		}
	}

	template<typename Alloc>
	void LoadCharToBuff<char>::load(ConstStrA text, AutoArray<char,Alloc> &out) {
		ConstStrA::Iterator iter(text.getFwIter());
		while (iter.hasItems()) {
			out.add(iter.getNext());
		}
	}


}

template<typename T, typename Alloc >
template<typename X>
TextFormat<T,Alloc> &TextFormat<T, Alloc>::operator <<(IIterator<T, X> *iter) {
	while (iter->hasItems()) buff.add(iter->getNext());
	paramList.add(buff.length());
	return *this;

}


template<typename T, typename Alloc>
void TextFormat<T,Alloc>::resetArgs() {
	paramList.clear();
	buff.clear();
}




}
#endif /* LIGHTSPEED_TEXT_TEXTFORMAT_TCC_ */
