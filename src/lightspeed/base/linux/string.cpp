/*
 * string.cpp
 *
 *  Created on: 17.3.2010
 *      Author: ondra
 */

#include "../containers/string.tcc"
#include <strings.h>
#include <wchar.h>
#include <iconv.h>
#include "../exceptions/stringException.h"
#include <memory>
#include "seh.h"
#include <string.h>

using std::min;

extern "C" { int wcsncasecmp(const wchar_t *s1, const wchar_t *s2, size_t n); }



namespace LightSpeed {

template struct StrCmpCS<byte>;
template struct StrCmpCS<char>;
template struct StrCmpCS<wchar_t>;

CompareResult StrCmpCI<char>::operator()(const ConstStringT<char> &a, const ConstStringT<char> &b) const {
	natural la = a.length();
	natural lb = b.length();
	natural cnt = std::min(la,lb);
	int res = strncasecmp(a.data(), b.data(), cnt);
	if (res == 0) {
		return cnt == la?(cnt == lb?cmpResultEqual:cmpResultLess):cmpResultGreater;
	} else {
		return res < 0? cmpResultLess:cmpResultGreater;
	}
}


CompareResult StrCmpCI<wchar_t>::operator()(const ConstStringT<wchar_t> &a, const ConstStringT<wchar_t> &b) const {
	natural la = a.length();
	natural lb = b.length();
	natural cnt = std::min(la,lb);
	int res = wcsncasecmp(a.data(), b.data(), cnt);
	if (res == 0) {
		return cnt == la?(cnt == lb?cmpResultEqual:cmpResultLess):cmpResultGreater;
	} else {
		return res < 0? cmpResultLess:cmpResultGreater;
	}
}



void  convertWideToCP(ConstStrW input, ICharConvertOutput<char> &output, CodePageDef cp) {
	iconv_t iconvHndl = iconv_open("WCHAR_T", cp);
	if (iconvHndl == (iconv_t)-1)
		throw UnknownCodePageException(THISLOCATION,cp,errno);

	const natural buffsz = 4096;
	char buff[buffsz];

	char *iconvin = reinterpret_cast<char *>(const_cast<wchar_t *>(input.data()));
	size_t iconvinsz = input.length()*sizeof(wchar_t);
	char *iconvout = buff;
	size_t iconvoutsz = buffsz ;
	while (iconv(iconvHndl,&iconvin,&iconvinsz,&iconvout,&iconvoutsz) == (size_t)-1) {
		int err = errno;
		if (err == E2BIG) {
			output(ConstStrA(buff,buffsz ));
			iconvout = buff;
			iconvoutsz = buffsz ;
		} else  {
			iconv_close(iconvHndl);
			throw InvalidCharacterException(THISLOCATION, *iconvin, err);
		}
	}
	output(ConstStrA(buff,buffsz - iconvoutsz));
	iconv_close(iconvHndl);
}
void convertCPToWide(ConstStrA input, ICharConvertOutput<wchar_t> &output, CodePageDef cp) {

	iconv_t iconvHndl = iconv_open( cp, "WCHAR_T");
	if (iconvHndl == (iconv_t)-1)
		throw UnknownCodePageException(THISLOCATION,cp,errno);

	const natural buffsz = 4096;
	wchar_t buff[buffsz];

	char *iconvin = const_cast<char *>(input.data());
	size_t iconvinsz = input.length();
	char *iconvout = reinterpret_cast<char *>(buff);
	size_t iconvoutsz = buffsz * sizeof(wchar_t);
	while (iconv(iconvHndl,&iconvin,&iconvinsz,&iconvout,&iconvoutsz) == (size_t)-1) {
		int err = errno;
		if (err == E2BIG) {
			output(ConstStrW(buff,buffsz ));
			iconvout = reinterpret_cast<char *>(buff);
			iconvoutsz = buffsz * sizeof(wchar_t);
		} else  {
			iconv_close(iconvHndl);
			throw InvalidCharacterException(THISLOCATION, *iconvin, err);
		}
	}
	output(ConstStrW(buff,buffsz - (iconvoutsz/sizeof(wchar_t))));
	iconv_close(iconvHndl);

}

static char zeroStringA[1]="";
static wchar_t zeroStringW[1]=L"";

bool safeCompareMemory ( const void * a, const void *b, natural count ) {
	__seh_try {
		return memcmp(a,b,count) == 0;
	} __seh_except(x) {
		(void)x;
		return false;
	}
	return false;
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
