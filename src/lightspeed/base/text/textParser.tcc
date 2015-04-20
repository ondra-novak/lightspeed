/*
 * textParser.tcc
 *
 *  Created on: 2.2.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEXT_TEXTPARSER_TCC_
#define LIGHTSPEED_TEXT_TEXTPARSER_TCC_


#pragma once

#include "../containers/autoArray.tcc"
#include "../memory/stdAlloc.h"
#include "../memory/smallAlloc.h"
#include "textParser.h"
#include "../streams/utf.h"
#include "../memory/staticAlloc.h"
#include "../iter/vtiterator.h"

namespace LightSpeed {



template<typename T>
inline bool isWS(const T &val) {
	return val == ' ' || val == '\t' || val == '\n' || val == '\r' || val == '\b' || val == '\a';
}

template<typename T, typename Alloc, typename Cmp>
template<typename Impl>
bool TextParser<T,Alloc,Cmp>::operator()(Str pattern, const ArrayT<T,Impl> &source) {
	typedef typename ArrayT<T,Impl>::Iterator SourceIter;

	PatternIter patitr = pattern.getFwIter();
	SourceIter srcitr = source.getFwIter();
	data.clear();
	fragments.clear();

	return parse(patitr,srcitr);
}

template<typename T, typename Alloc, typename Cmp>
template<typename Impl>
bool TextParser<T,Alloc,Cmp>::operator()(Str pattern, const IIterator<T, Impl> &source) {
	Impl srcitr = source._invoke();
	PatternIter patitr = pattern.getFwIter();
	data.clear();
	fragments.clear();
	return parse(patitr,srcitr);
}


template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parse(PatternIter &patiter, SourceIter &srciter) {

	inclass = false;
	while (patiter.hasItems()) {
		const T &p = patiter.getNext();
		if (p == '%') {
			if (patiter.hasItems() ) {
				const T &n = patiter.peek();
				if (n == '%' || n == '_' || n == ' ' || n == '\n' || n == '\t' || n == '\b') {
					if (!srciter.hasItems() || patiter.peek() != srciter.peek())
						return false;

					patiter.skip();
					srciter.skip();
				} else {
					return parseArg(patiter,srciter);

				}
			} else {
				return true;
			}
		} else if (p == '\b') {
			bool once = false;
			while (srciter.hasItems() && isWS(srciter.peek())) {
				srciter.skip();
				once = true;
			}
			if (!once) return false;
		} else if (p == '\t') {
			bool once = false;
			while (srciter.hasItems() && (srciter.peek() == ' ' || srciter.peek() == '\t')) {
				srciter.skip();
				once = true;
			}
			if (!once) return false;
		} else if (p == '\n') {
			typename ConstStringT<T>::Iterator iter = nl.getFwIter();
			while (iter.hasItems()) {
				if (!srciter.hasItems() || iter.getNext() != srciter.getNext()) return false;
			}
		} else if (p == ' ') {
			if (ws.empty()) {
				while (srciter.hasItems() && (srciter.peek() == ' ' || srciter.peek() == '\t')) {
					srciter.skip();
				}
			} else {
				while (srciter.hasItems() && (ws.find(srciter.peek()) != naturalNull)) {
					srciter.skip();
				}
			}
		} else if (p == '_') {
			bool once = false;
			if (ws.empty()) {
				while (srciter.hasItems() && (srciter.peek() == ' ' || srciter.peek() == '\t')) {
					srciter.skip();
					once = true;
				}
			} else {
				while (srciter.hasItems() && (ws.find(srciter.peek()) != naturalNull)) {
					srciter.skip();
					once = true;
				}
			}
			if (!once) return false;
		} else {
			if (srciter.hasItems() && cmp.equal(srciter.peek(), p)) {
				srciter.skip();
			} else {
				return false;
			}
		}
	}
	return !srciter.hasItems();
}

template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseArg(PatternIter &patiter, SourceIter &srciter) {

	StepState state(data.length());
	return parseArg(patiter,srciter,state);

}

template<typename T, typename Alloc, typename Cmp>
void TextParser<T,Alloc,Cmp>::skipCharlist(PatternIter &iter) {
	const T &x = iter.getNext();
	if (x == '*') return skipCharlist(iter);
	if (x == '^') return skipCharlist(iter);
	if (x == ']') return skipCharlist(iter);
	while (iter.hasItems()) {
		const T &x = iter.getNext();
		if (x == ']') return;
		const T &y = iter.peek();
		if (y == '-') {iter.skip();iter.skip();		}
	}
}
template<typename T, typename Alloc, typename Cmp>
void TextParser<T,Alloc,Cmp>::skipRange(PatternIter &iter) {
	while (iter.hasItems()) {
		const T &x = iter.getNext();
		if (x == ')') return;
	}
}
template<typename T, typename Alloc, typename Cmp>
void TextParser<T,Alloc,Cmp>::skipQuoted(PatternIter &iter) {
	 iter.skip();
	 iter.skip();
}

template<typename T, typename Alloc, typename Cmp>
void TextParser<T,Alloc,Cmp>::skipPattern(PatternIter &iter) {	
	while (iter.hasItems()) {
		const T &p = iter.getNext();
		if (p == '[') {
			skipCharlist(iter);
		} else if (p == '(') {
			skipRange(iter);
		} else if (p == '\'' || p == 'q') {
			skipQuoted(iter);
		} else if (p =='%') {
			return;
		} else if (p >= '0' && p <= '9') {
			while (iter.hasItems()) {
				const T &p = iter.peek();
				if (p == '%') {
					iter.skip();
					return;
				} else if (p < '0' || p > '9') {
					return;
				}
			}
		}
	}

}

template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseArg(PatternIter &patiter, SourceIter &srciter, StepState &state) {

	if (patiter.hasItems()) {

		const T &p = patiter.peek();
		if (p == '<') {
			patiter.skip();
			return parseAlternateBranch(patiter,srciter);
		} else if (p == '|') {
			patiter.skip();
			return skipAlternateBranch(patiter,srciter);
		} else if (p == '>') {
			patiter.skip();
			return parse(patiter,srciter);
		} else if (p == '[') {
			patiter.skip();
			state.defaultAsterix = false;
			return parseCharList(patiter,srciter,state);
		} else if (p == '(') {
			patiter.skip();
			return parseRange(patiter,state.range) && parseArg(patiter,srciter,state);
		} else if (p == '*') {
			patiter.skip();
			state.defaultAsterix = false;
			return parseAsterix(patiter,srciter,state);
		} else if (p == '\'' || p == 'q') {
			patiter.skip();
			state.defaultAsterix = false;
			return parseQuoted(patiter,srciter,state);
		} else if (p == '%') { //end of extraction without storing
			if (state.defaultAsterix) {
				state.defaultAsterix = false;
				return parseAsterix(patiter,srciter,state);
			}
			patiter.skip();
			data.erase(state.dataBeg,data.length() - state.dataBeg);
			return parse(patiter,srciter);
		} else if (p >= '0' && p <='9') {
			if (state.defaultAsterix) {
				state.defaultAsterix = false;
				return parseAsterix(patiter,srciter,state);
			}
			natural fragId = 0;
			while (patiter.hasItems()) {
				const T &p = patiter.peek();
				if (p < '0' || p > '9') break;
				fragId = fragId * 10 + (p - '0');
				patiter.skip();
			}
			if (patiter.hasItems() && patiter.peek() == '%') patiter.skip();

			if (fragId != 0) {
				if (fragments.length() <= fragId - 1) fragments.resize(fragId,Fragment(data,0,0));
				fragments(fragId-1) = Fragment(data,state.dataBeg,data.length() - state.dataBeg);
			} else {
				data.erase(state.dataBeg,data.length() - state.dataBeg);
			}
			bool res =  parse(patiter,srciter);
			if (res == false) {
				if (fragId != 0)
					fragments(fragId-1) = Fragment(data,0,0);
			}
			return res;
		} else {
			patiter.skip();
			state.defaultAsterix = false;
			return handleCustomPatterns(p,patiter,srciter,state);
		}
	}

	return inclass;
}
template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseClass(PatternIter &patiter, SourceIter &srciter, StepState &state, ConstStrA pattern) {
	typedef AutoArrayStream<T,StaticAlloc<256> > Buffer;
	Buffer buffer;
	pattern.render(buffer);
	inclass = true;
	ConstStringT<T> newpatt(buffer.getArray());
	PatternIter newiter = newpatt.getFwIter();
	bool res = parseArg(newiter,srciter,state);
	inclass = false;
	if (res == true) {
		patiter.skip();
		return parseArg(patiter,srciter,state);
	} else {
		return false;
	}
}


template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseQuoted(PatternIter &patiter, SourceIter &srciter, StepState &state) {
	const T &qmark = patiter.getNext();
	const T &esc = patiter.getNext();
//	natural dataEnd = data.length();
	bool same = qmark == esc;
	if (srciter.hasItems() && cmp.equal(srciter.peek(),qmark)) {
		srciter.skip();
		while (srciter.hasItems()) {
			const T &a = srciter.getNext();
			if (cmp.equal(a, esc)) {
				if (srciter.hasItems() && cmp.equal(srciter.peek(),qmark)) {
					data.add(srciter.getNext());
				} else if (same) {
					break;
				} else {
					data.add(srciter.getNext());
				}
			} else if (cmp.equal(a,qmark)) {
				break;
			} else {
				data.add(a);
			}
		}
	} else return false;
//	if (data.length() - dataEnd < state.range.first || data.length() - dataEnd > state.range.second) return false;
	return parseArg(patiter,srciter,state);

}

template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseCharList(PatternIter &patiter, SourceIter &srciter, StepState &state) {
	if (!patiter.hasItems()) return false;
	Str list( &patiter.getNext(),1);
	bool cond = true;
	bool asterix = false;
	if (list[0] == '*') {asterix = true; list = Str( &patiter.getNext(),1);}
	if (list[0] == '^') {cond = false; list = Str( &patiter.getNext(),1);}
	if (list[0] == '\\') {list = Str( &patiter.getNext(),1);}
	while (patiter.hasItems() && patiter.peek() != ']')
		{patiter.skip();list = Str(list.data(),list.length()+1);}

 	if (patiter.hasItems()) patiter.skip();

	natural dataPos = data.length();

	if (asterix && state.range.first == 0) {
		if (parseAsterixEnter(patiter,srciter,state)) return true;
	}


	while (srciter.hasItems() && data.length() - dataPos < state.range.second) {
		const T &p = srciter.peek();
		bool okaj = false;
		typename Str::Iterator iter = list.getFwIter();
		while (iter.hasItems() && !okaj) {
			const T &a = iter.getNext();
			if (iter.hasItems() && iter.peek() == '-') {
				const T &_ = iter.getNext();
				if (iter.hasItems()) {
					const T &b = iter.getNext();
					if (!cmp.less(p , a) && !cmp.less(b,p) == cond) {
						okaj = true;
					}
				} else {
					if ((cmp.equal(p, a) || cmp.equal(p, _)) == cond) {
						okaj = true;
					}
				}
			} else {
				if (cmp.equal(p,a) ) {
					okaj = true;
				}
			}
		}
		if (okaj == cond) {
			data.add(p);
			srciter.skip();
			if (asterix && state.range.first <= data.length() - dataPos) {
				if (parseAsterixEnter(patiter,srciter,state)) return true;
			}
		} else {
			break;
		}
	}

	if (data.length() - dataPos < state.range.first) return false;
	else return parseArg(patiter,srciter,state);
}

template<typename T, typename Alloc, typename Cmp>
natural TextParser<T,Alloc,Cmp>::parseRange(PatternIter &patiter, Range &range) {
	bool num1 = false, num2 = false;
	natural r1 = 0, r2 = 0;
	natural cnt = 0;
	bool *bfeed = &num1;
	natural *nfeed = &r1;
	while (patiter.hasItems()) {
		const T &k = patiter.getNext();
		cnt++;
		if (k == ',') {
			bfeed = &num2;
			nfeed = &r2;
		} else if (k >= '0' && k <= '9') {
			natural dig = k - '0';
			*nfeed = *nfeed * 10 + dig;
			*bfeed = true;
		} else if (k == ')') {
			break;
		} else if (k == '*' && bfeed== &num2 ) {
			*nfeed = naturalNull;
			*bfeed = true;
		} else if (bfeed== &num1 && r1 == 0 &&patiter.peek() == ')' ) {
			if (k == '*') {
				r1 = 0;r2 = naturalNull;
			} else if (k == '+') {
				r1 = 1;r2 = naturalNull;
			} else if (k == '?') {
				r1 = 0;r2 = 1;
			} else if (k == '.') {
				r1 = 1;r2 = 1;
			} 
			num1 = num2 = true;
			patiter.skip();
			cnt++;
			break;
		}
	}

	if (num1) range.first = r1;
	if (num2) range.second = r2;
	return cnt;

}

template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseAsterixEnter(PatternIter &patiter, SourceIter &srciter, StepState &state) {
	natural pos = data.length();
	PatternIter patiternew = patiter;
	SourceIter srciternew = srciter;
	StepState statenew  = state;
	if (parseArg(patiternew,srciternew,statenew)) return true;
	if (data.length() > pos)
		data.erase(pos,data.length() - pos);
	return false;
}


template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseAsterix(PatternIter &patiter, SourceIter &srciter, StepState &state) {

	natural cnt = 0;
	while (srciter.hasItems()) {
		if (cnt >= state.range.first) {
			if (parseAsterixEnter(patiter,srciter,state)) return true;
		}
		if (cnt >= state.range.second) {
			return false;
		}
		data.add(srciter.getNext());
		cnt++;
	}
	if (cnt < state.range.first) return false;

	return parseArg(patiter,srciter,state);
}



namespace _intr {

template<typename T, typename Alloc>
StoreToCharBuff<T,Alloc>::StoreToCharBuff(ConstStringT<T> par) {
	typename Super::WriteIter writr = this->getWriteIterator();
	typename ConstStringT<T>::Iterator rditr = par.getFwIter();
	writr.copy(rditr);
}

template<typename Alloc>
StoreToCharBuff<wchar_t,Alloc>::StoreToCharBuff(ConstStringT<wchar_t> par) {
	typename Super::WriteIter writr = this->getWriteIterator();
	typedef typename ConstStringT<wchar_t>::Iterator CItr;
	WideToUtf8Reader<CItr> rditr(par.getFwIter());
	writr.copy(rditr);
}

}

template<typename T, typename Alloc, typename Cmp>
ConstStringT<T> TextParser<T,Alloc,Cmp>::Result::str() const {
	return owner.fragments[index];
}

template<typename T, typename Alloc, typename Cmp>
natural TextParser<T,Alloc,Cmp>::Result::hex() const {
	ConstStringT<T> text = str();
	natural numb = 0;
	parseUnsignedNumber(text.getFwIter(),numb,16);
	return numb;
}

template<typename T, typename Alloc, typename Cmp>
natural TextParser<T,Alloc,Cmp>::Result::basedNum(natural base) const {
	ConstStringT<T> text = str();
	natural numb = 0;
	parseUnsignedNumber(text.getFwIter(),numb,base);
	return numb;
}

template<typename T, typename Alloc, typename Cmp>
TextParser<T,Alloc,Cmp>::Result::operator typename TextParser<T,Alloc,Cmp>::Str() const {
	return owner.fragments[index];
}
template<typename T, typename Alloc, typename Cmp>
TextParser<T,Alloc,Cmp>::Result::operator T() const {
	if (owner.fragments[index].length() != 1) throw;//TODO exception;
	return owner.fragments[index][0];
}
template<typename T, typename Alloc, typename Cmp>
TextParser<T,Alloc,Cmp>::Result::operator int() const {
	_intr::StoreToCharBuff<T,SmallAlloc<256> > buff(owner.fragments[index]);
	return _intr::stringToSignedNumber<int>(buff,10);
}
template<typename T, typename Alloc, typename Cmp>
TextParser<T,Alloc,Cmp>::Result::operator unsigned int() const {
	_intr::StoreToCharBuff<T,SmallAlloc<256> > buff(
			ConstStringT<T>(owner.fragments[index]));
	return _intr::stringToUnsignedNumber<unsigned int>(buff,10);
}
template<typename T, typename Alloc, typename Cmp>
TextParser<T,Alloc,Cmp>::Result::operator lnatural() const {
	_intr::StoreToCharBuff<T,SmallAlloc<256> > buff(owner.fragments[index]);
	return _intr::stringToUnsignedNumber<lnatural>(buff,10);
}
template<typename T, typename Alloc, typename Cmp>
TextParser<T,Alloc,Cmp>::Result::operator linteger() const {
	_intr::StoreToCharBuff<T,SmallAlloc<256> > buff(owner.fragments[index]);
	return _intr::stringToSignedNumber<linteger>(buff,10);
}
template<typename T, typename Alloc, typename Cmp>
TextParser<T,Alloc,Cmp>::Result::operator float() const {
	_intr::StoreToCharBuff<T,SmallAlloc<256> > buff(owner.fragments[index]);
	return _intr::stringToFloatNumber<float>(buff);
}
template<typename T, typename Alloc, typename Cmp>
TextParser<T,Alloc,Cmp>::Result::operator double() const {
	_intr::StoreToCharBuff<T,SmallAlloc<256> > buff(owner.fragments[index]);
	return _intr::stringToFloatNumber<double>(buff);

}

template<typename T, typename Alloc, typename Cmp>
typename TextParser<T,Alloc,Cmp>::Result TextParser<T,Alloc,Cmp>::operator[](int pos) const {
	if (pos <1 || pos > (int)fragments.length())
		throwRangeException_FromTo<natural>(THISLOCATION,1,fragments.length(),pos);
	return typename TextParser<T,Alloc,Cmp>::Result(*this,pos-1);

}



template<typename Iter, typename NumT>
bool parseUnsignedNumber(Iter &iter, NumT &number,natural base) {
	number = 0;
	bool okaj = false;
	while (iter.hasItems()) {
		char c = (char)iter.peek();
		NumT a;
		if (c >= '0' && c <= '9') a = (c - '0');
		else if (c >= 'A' && c<='Z') a = (c - 'A') + 10 ;
		else if (c >= 'a' && c<='z') a = (c - 'a') + 10;
		else break;
		if ((natural)a >= base) break;
		number = number * base + a;
		iter.skip();
		okaj = true;
	}
	return okaj;
}

template<typename Iter, typename NumT>
bool parseUnsignedNumber(const Iter &iter, NumT &number,natural base) {
	Iter i = iter;
	return parseUnsignedNumber(i,number,base);
}
template<typename Iter, typename NumT>
bool parseSignedNumber(Iter &iter, NumT &number,natural base) {
	if (iter.hasItems()) {

		char c = (char)iter.peek();
		if (c == '+') {
			iter.skip();
			return parseUnsignedNumber(iter,number,base);
		} else if (c == '-') {
			iter.skip();
			NumT tmp;
			if (!parseUnsignedNumber(iter,tmp,base)) return false;
			number = -tmp;
			return true;
		} else {
			return parseUnsignedNumber(iter,number,base);
		}

	} else return false;
}
template<typename Iter, typename NumT>
bool parseSignedNumber(const Iter &iter, NumT &number,natural base) {
	Iter i = iter;
	return parseSignedNumber(i,number,base);
}

template<typename T, typename Alloc, typename Cmp>
bool TextParser<T,Alloc,Cmp>::skipToNextBranch(PatternIter &patiter) {
	while (patiter.hasItems()) {
		const T &c = patiter.getNext();
		if (c == '%' && patiter.hasItems()) {
			const T &d = patiter.peek();
			if (d == c) {
				patiter.skip();
			} else if (d == '|') {
				patiter.skip();
				return true;
			} else if (d == '>') {
				patiter.skip();
				return false;
			} else if (d == '<') {
				bool x = skipToNextBranch(patiter);
				while (x) x = skipToNextBranch(patiter);
			} else {
				skipPattern(patiter);
			}
		}
	}
	return false;
}


template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool  TextParser<T,Alloc,Cmp>::parseAlternateBranch(PatternIter &patiter,SourceIter &srciter) {
	PatternIter savePat(patiter);
	if (parse(patiter,srciter)) return true;
	else {
		if (skipToNextBranch(savePat))
			return parseAlternateBranch(savePat,srciter);
		else
			return parse(savePat,srciter);
	}
}

template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::skipAlternateBranch(PatternIter &patiter,SourceIter &srciter) {
	while(skipToNextBranch(patiter)) {}
	return parse(patiter,srciter);
}

template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::handleCustomPatterns(const T &command, PatternIter &patiter, SourceIter &srciter, StepState &state) {

	class CustomRelease {
	public:
		ICustomParser *p;
		void *dataptr;
		CustomRelease(ICustomParser *p):p(p),dataptr(0) {}
		~CustomRelease() {if (dataptr) p->release(&dataptr);}
	};

	state.defaultAsterix = false;
	natural dataLen = data.length();
	if (command == 'd') {
		if (!parseNumber(srciter,state.range.second,true,10)) return false;
	} else if (command == 'u') {
		if (!parseNumber(srciter,state.range.second,false,10)) return false;
	} else if (command == 'f') {
		if (!parseFloat(srciter,state.range.second)) return false;
	} else if (command == 'x') {
		if (!parseNumber(srciter,state.range.second,true,16)) return false;
	} else if (command == 'o') {
		if (!parseNumber(srciter,state.range.second,true,8)) return false;
	} else if (command == 'i') {
		if (!parseIdentifier(srciter,state.range.second)) return false;
	} else if (custom != nil) {
		VtIterator<PatternIter &> vtpatiter(patiter);
		VtIterator<SourceIter &> vtsrciter(srciter);
		typename AutoArray<T, Alloc>::WriteIter wr(data,state.range.second);
		VtWriteIterator<typename AutoArray<T, Alloc>::WriteIter &> dataiter(wr);
		Alloc alloc(data.getAllocator());
		CustomRelease d(custom);
		bool res = custom->parse(command,vtpatiter,vtsrciter,dataiter,alloc,cmp,false,&d.dataptr);
		if (res == false || data.length() - dataLen < state.range.first) return false;
		while (res == true) {
			PatternIter nxpat = patiter;
			SourceIter nxsrc = srciter;
			if (parseArg(nxpat,nxsrc,state)) return true;
			res = custom->parse(command,vtpatiter,vtsrciter,dataiter,alloc,cmp,false,&d.dataptr);
		}
		return res;
	} else {
		return false;
	}
	natural newDataLen = data.length();
	if (newDataLen- dataLen < state.range.first) {
		return false;
	} else {
		return parseArg(patiter,srciter,state);
	}

}

template<typename T>
bool checkNumber(const T &x, natural base) {
	if (base > 10) {
		if (x >= T('0') && x <=T('9')) return true;
		if (x >= T('A') && x <T('A'+(base-10)) ) return true;
		if (x >= T('a') && x <T('a'+(base-10)) ) return true;
		return false;
	} else {
		if (x >= T('0') && x <T('0'+base)) return true;
		return false;
	}
}

template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseNumber(SourceIter &srciter, natural rangeMax, bool negativeSign, natural base) {

	if (rangeMax == 0) return false;
	if (!srciter.hasItems()) return false;
	const T &sign = srciter.peek();
	if (sign == '+' || sign == '-') {
		if (negativeSign == false) return false;
		data.add(sign);
		srciter.skip();
		rangeMax--;
	}
	if (rangeMax == 0 || !srciter.hasItems()) return false;
	if (!checkNumber(srciter.peek(),base)) return false;
	data.add(srciter.getNext());
	rangeMax--;
	while (rangeMax > 0 && srciter.hasItems()) {
		if (!checkNumber(srciter.peek(),base)) break;
		data.add(srciter.getNext());
		rangeMax--;
	}

	return true;
}

template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseFloat(SourceIter &srciter, natural rangeMax) {
	natural dp = data.length();
	if (!parseNumber(srciter,rangeMax,true,10)) return false;
	rangeMax -= data.length() - dp;
	if (rangeMax == 0 || !srciter.hasItems()) return true;
	T k = srciter.peek();
	if (k == '.') {
		data.add(k);srciter.skip();
		rangeMax --;
		if (rangeMax == 0 || !srciter.hasItems()) return false;
		dp = data.length();
		if (!parseNumber(srciter,rangeMax,false,10)) return false;
		rangeMax -= data.length() - dp;
		if (rangeMax == 0 || !srciter.hasItems()) return true;
		k = srciter.peek();
	}
	if (k == 'e' || k == 'E') {
		data.add(k);srciter.skip();
		rangeMax --;
		if (rangeMax == 0 || !srciter.hasItems()) return false;
		dp = data.length();
		if (!parseNumber(srciter,rangeMax,true,10)) return false;
		rangeMax -= data.length() - dp;
	}
	return true;
}


template<typename T, typename Alloc, typename Cmp>
template<typename SourceIter>
bool TextParser<T,Alloc,Cmp>::parseIdentifier( SourceIter &srciter, natural rangeMax) {
	if (rangeMax && srciter.hasItems()) {
		const T &k = srciter.peek();
		if (k == '_' || (k >= 'a' && k <='z') || (k >= 'A' && k <= 'Z')) {
			rangeMax--;
			data.add(k);
			srciter.skip();
			while (rangeMax && srciter.hasItems()) {
				T k = srciter.peek();
				if (k == '_' || (k >= 'a' && k <='z') || (k >= 'A' && k <= 'Z') || (k >= '0' && k <= '9')) {
					data.add(k);
					srciter.skip();
					rangeMax--;
				} else {
					break;
				}
			}
			return true;
		}
	}
	return false;
}


}  // namespace LightSpeed


#endif /* LIGHTSPEED_TEXT_TEXTPARSER_TCC_ */
