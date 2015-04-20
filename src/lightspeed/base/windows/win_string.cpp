/*
 * string.cpp
 *
 *  Created on: 17.3.2010
 *      Author: ondra
 */

#include "winpch.h"
#include "../containers/string.tcc"
#include <string.h>
#include <wchar.h>
#include "../exceptions/stringException.h"
#include <memory>

#undef min
#undef max

using std::min;


namespace LightSpeed {


	template struct StrCmpCS<wchar_t>;
	template struct StrCmpCS<char>;

CompareResult StrCmpCI<char>::operator()(const ConstStringT<char> &a, const ConstStringT<char> &b) const {
	natural la = a.length();
	natural lb = b.length();
	natural cnt = std::min(la,lb);
	CompareResult res;
	if (cnt != 0) {		
		res = (CompareResult)_strnicmp(a.data(), b.data(), cnt);
	} else {
		res = cmpResultEqual;
	}
		
	if (res == cmpResultEqual) {
		return cnt == la?(cnt == lb?cmpResultEqual
				:cmpResultLess):cmpResultGreater;
	} else {
		return res;
	}
}


CompareResult StrCmpCI<wchar_t>::operator()(const ConstStringT<wchar_t> &a, const ConstStringT<wchar_t> &b) const {
	natural la = a.length();
	natural lb = b.length();
	natural cnt = std::min(la,lb);
	int res = _wcsnicmp(a.data(), b.data(), cnt);
	if (res == 0) {
		return cnt == la?(cnt == lb?cmpResultEqual:cmpResultLess):cmpResultGreater;
	} else {
		return (CompareResult)res;
	}
}



void  convertWideToCP(ConstStrW input, ICharConvertOutput<char> &output, CodePageDef cp) {
	throw UnsupportedFeatureOnClass<String>(THISLOCATION,"");
}
void convertCPToWide(ConstStrA input, ICharConvertOutput<wchar_t> &output, CodePageDef cp) {
	throw UnsupportedFeatureOnClass<String>(THISLOCATION,"");
}

static char zeroStringA[1]="";
static wchar_t zeroStringW[1]=L"";

bool safeCompareMemory ( const void * a, const void *b, natural count ) {
	__try {
		return memcmp(a,b,count) == 0;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}	
}

const char * cStr( ConstStrA source, StringA &tmp )
{
	
	if (source.empty()) return zeroStringA;
	const char *p = source.data() + source.length();
	char zero = 0;
	if (safeCompareMemory(p,&zero,sizeof(char))) return source.data();
	else {
		tmp = source;
		return tmp.c_str();
	}

	
}

const wchar_t * cStr( ConstStrW source, StringW &tmp )
{
	if (source.empty()) return zeroStringW;
	const wchar_t *p = source.data() + source.length();
	wchar_t zero = 0;
	if (safeCompareMemory(p,&zero,sizeof(char))) return source.data();
	else {
		tmp = source;
		return tmp.c_str();
	}


}


}  // namespace LightSpeed
