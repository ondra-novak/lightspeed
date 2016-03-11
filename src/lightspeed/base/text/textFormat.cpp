/*
 * textOut.cpp
 *
 *  Created on: 24.12.2010
 *      Author: ondra
 */

#include "textFormat.tcc"
#include "textOut.tcc"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "../../mt/platform.h"

namespace LightSpeed {




namespace _intr {

	static char hexChars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";


template<typename UNum>
char *formatUnsigned(UNum num, char *buffer, natural size, natural base) {
	if (num == 0) return buffer;
	if (size == 0) return 0;
	char *c = formatUnsigned(num / base,buffer,size,base);
	if (c) {
		*c = hexChars[num % base];
		return c+1;
	} else {
		return c;
	}
}


template<typename T>
ConstStrA numberToStringUnsigned(T num, char *string, natural size, natural base) {
	if (size < 2) return nil;
	if (num == 0) {
		string[0] = '0';
		string[1] = 0;
		return string;
	} else {
		char *x = formatUnsigned(num,string,size-1,base);
		if (x) {
			*x = 0;
			return ConstStrA(string,x-string);
		} else {
			return nil;
		}
	}
}

template<typename T>
ConstStrA numberToStringSigned(T num, char *string, natural size, natural base) {
	if (size < 3) return nil;
	if (num == 0) {
		string[0] = '0';
		string[1] = 0;
		return string;
	} else {
		if (num < 0) {
			*string='-';
			ConstStrA z = numberToString(-num,string+1,size-1,base);
			if (z == nil) return nil;
			return ConstStrA(string,1+z.length());
		}
		char *x = formatUnsigned(num,string,size-1,base);
		if (x) {
			*x = 0;
			return ConstStrA(string,x-string);
		} else {
			return nil;
		}
	}
}

template<> ConstStrA numberToString(short num, char *string, natural size, natural base) {
	return numberToStringSigned(num,string,size,base);
}
template<> ConstStrA numberToString(int num, char *string, natural size, natural base) {
	return numberToStringSigned(num,string,size,base);
}
template<> ConstStrA numberToString(long num, char *string, natural size, natural base) {
	return numberToStringSigned(num,string,size,base);
}
template<> ConstStrA numberToString(long long num, char *string, natural size, natural base) {
	return numberToStringSigned(num,string,size,base);
}


template<> ConstStrA numberToString(unsigned short num, char *string, natural size, natural base) {
	return numberToStringUnsigned(num,string,size,base);
}
template<> ConstStrA numberToString(unsigned int num, char *string, natural size, natural base) {
	return numberToStringUnsigned(num,string,size,base);
}
template<> ConstStrA numberToString(unsigned long num, char *string, natural size, natural base) {
	return numberToStringUnsigned(num,string,size,base);
}
template<> ConstStrA numberToString(unsigned long long num, char *string, natural size, natural base) {
	return numberToStringUnsigned(num,string,size,base);
}

static ConstStrA mantisaToString(double num, char *string, natural size, natural decimals) {
	if (size < 3) return nil;
	string[size-1] = 0;
	bool removeZeroes = decimals == naturalNull;
	if (removeZeroes) decimals = 20;
#ifdef LIGHTSPEED_PLATFORM_WINDOWS
	sprintf_s(string,size,"%1.*f",(unsigned int)decimals,num);
#else
	snprintf(string,size,"%1.*f",(unsigned int)decimals,num);
#endif

	if (string[size-1] != 0) return nil;
	return ConstStrA(string);
}

#ifdef _WIN32
inline double exp10(integer e) {
	return pow(10.0,int(e));
}
#endif

ConstStrA realToString(double num, char *string, natural size, natural decimals, integer scimin, integer scimax) {
	if (num < 0) {
		if (size < 1) return ConstStrA();
		*string='-';
		ConstStrA res = realToString(-num,string+1,size-1,decimals,scimin,scimax);
		return ConstStrA(res.data()-1,res.length()+1);
	}
	integer e = (integer)floor(log10(num));
	if ((e <= scimin || e >= scimax) && e > integerNull) {
		double mantisa = num/exp10(e);
		ConstStrA strm = mantisaToString(mantisa,string,size,decimals);
		if (strm == nil || strm.length()+2 >= size) return nil;
		char *estr = string + strm.length();
		*estr++ = 'E';
		if (e >= 0) *estr++ = '+';
		ConstStrA stre = numberToStringSigned(e,estr,size - (estr - string),10);
		return ConstStrA(string, stre.data()+stre.length() - string);

	} else {
		ConstStrA strm = mantisaToString(num,string,size,decimals);
		return strm;
	}


}


}

template class TextFormat<char,StdAlloc>;
template class TextFormat<wchar_t,StdAlloc>;
template class TextOut<AutoArray<wchar_t>::WriteIter>;



}
