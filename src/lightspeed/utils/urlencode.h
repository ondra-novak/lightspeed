/*
 * urlencode.h
 *
 *  Created on: 23.4.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_URLENCODE_H_
#define LIGHTSPEED_UTILS_URLENCODE_H_

#include "../base/iter/iteratorFilter.h"

#pragma once


namespace LightSpeed {

class UrlEncoder: public IteratorFilterBase<char, char, UrlEncoder > {
public:
	UrlEncoder();
    bool needItems() const;
    void input(const char &x);
    bool hasItems() const;
    char output();
    void flush();
protected:
    char buff[4];
    int pt;


};

class UrlDecoder: public IteratorFilterBase<char, char, UrlDecoder > {
public:
	UrlDecoder();
    bool needItems() const;
    void input(const char &x);
    bool hasItems() const;
    char output();
    void flush();
protected:
    char buff[4];
    int pt;
};


inline UrlEncoder::UrlEncoder():pt(0) {
}

inline bool UrlEncoder::needItems() const {
	return pt == 0;
}


inline void UrlEncoder::input(const char& x) {
	if ((x >='A' && x <='Z') || (x >= 'a' && x <= 'z')
			|| (x >='0' && x <='9') || x == '_' || x == '-') {
		buff[0] = x;
		pt=1;
	} else {
		unsigned char z = (unsigned char)x;
		buff[2] = '%';
		buff[1] = (z>>4) >= 10?((z>>4)+'A'-10):((z>>4)+'0');
		buff[0] = (z & 0xF) > 10?((z & 0xF) + 'A'-10):((z & 0xF)+'0');
		pt = 3;
	}
}

inline bool UrlEncoder::hasItems() const {
	return pt != 0;
}

inline char UrlEncoder::output() {
	if (!pt) throwIteratorNoMoreItems(THISLOCATION,typeid(*this));
	return buff[--pt];
}


inline void UrlEncoder::flush() {

}
inline UrlDecoder::UrlDecoder():pt(0) {
}

inline bool UrlDecoder::needItems() const {
	return pt == 0 || (pt < 3 && buff[0]=='%');
}

inline void UrlDecoder::input(const char& x) {
	buff[pt] = x;
	pt++;
}

inline bool UrlDecoder::hasItems() const {
	return pt == 1 || (pt == 3 && buff[0] == '%');
}

inline char UrlDecoder::output() {
	pt = 0;
	if (buff[0] == '%') {
		byte *cc = (byte *)buff+1;
		byte c = (cc[0] >= 'a'?cc[0] - 'a' + 10 : (cc[0] >= 'A' ? cc[0] - 'A' + 10 : cc[0] - '0')) * 16 +
				 (cc[1] >= 'a'?cc[1] - 'a' + 10 : (cc[1] >= 'A' ? cc[1] - 'A' + 10 : cc[1] - '0'));
		return (char)c;
	} else if (buff[0] == '+'){
		return (char)32;
	} else {
		return buff[0];
	}
}

inline void UrlDecoder::flush() {
}

}
#endif /* LIGHTSPEED_UTILS_URLENCODE_H_ */
