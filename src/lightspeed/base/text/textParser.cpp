/*
 * textParser.cpp
 *
 *  Created on: 1.2.2011
 *      Author: ondra
 */

#include "../memory/stdAlloc.h"
#include "textParser.tcc"
#include "../exceptions/invalidNumberFormat.h"
#include <math.h>

namespace LightSpeed {


double strtod(ConstStrA string, natural *chrRead);

namespace _intr {

template<typename T>
T stringToUnsignedNumber(ConstStrA num, natural base) {
	if (num.empty()) throw InvalidNumberFormatException(THISLOCATION,String(num));
	if (num[0] == '+') return stringToUnsignedNumber<T>(num.offset(1),base);
	T s = 0;
	T tbase = (T)base;
	for (ConstStrA::Iterator iter = num.getFwIter(); iter.hasItems();) {
		const T &x = iter.getNext();
		T q = 0;
		if (x >= '0' && x <= '9') q = (T)(x - '0');
		else if (x >= 'A' && x <= 'Z') q = (T)(x - 'A' + 10);
		else if (x >= 'a' && x <= 'z') q = (T)(x - 'a' + 10);
		else throw InvalidNumberFormatException(THISLOCATION,String(num));
		if (q >= tbase)
			throw InvalidNumberFormatException(THISLOCATION,String(num));
		s = s * tbase + q;
	}
	return s;
}
template<typename T>
T stringToSignedNumber(ConstStrA num, natural base) {
	if (num.empty()) throw InvalidNumberFormatException(THISLOCATION,String(num));
	if (num[0] == '+') return stringToUnsignedNumber<T>(num.offset(1),base);
	else if (num[0] == '-') return -stringToUnsignedNumber<T>(num.offset(1),base);
	else return stringToUnsignedNumber<T>(num,base);
}


template<typename T>
T stringToFloatNumber(ConstStrA num) {
	natural chrRead;
	double d = strtod(num,&chrRead);
	if (chrRead < num.length())
		throw InvalidNumberFormatException(THISLOCATION,String(num));
	return (T)d;
}

template int stringToSignedNumber<int>(ConstStrA,natural);
template long stringToSignedNumber<long>(ConstStrA,natural);
template long long stringToSignedNumber<long long>(ConstStrA,natural);
template unsigned int stringToUnsignedNumber<unsigned int>(ConstStrA,natural);
template unsigned long stringToUnsignedNumber<unsigned long>(ConstStrA,natural);
template unsigned long long stringToUnsignedNumber<unsigned long long>(ConstStrA,natural);
template float stringToFloatNumber<float>(ConstStrA);
template double stringToFloatNumber<double>(ConstStrA);
template wchar_t stringToSignedNumber<wchar_t>(ConstStrA,natural);
}

}

