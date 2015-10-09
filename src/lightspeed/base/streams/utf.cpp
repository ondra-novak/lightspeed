/*
 * utf.cc
 *
 *  Created on: 26.12.2010
 *      Author: ondra
 */


#include "utf.tcc"




namespace LightSpeed {




  bool Utf8ToWideFilter::canAccept(const char & x) const
 {
     if (state == 0) return false;
     else if (state == naturalNull) {
         return ((x & 0xC0) != 0x80);
     } else {
         return (x & 0x80) != 0;
     }
 }



  void Utf8ToWideFilter::input(const char & x)
 {
     byte a = x;
     if (state == 0)
         throw InvalidUTF8Character(THISLOCATION,x);
     if (state == naturalNull) {
         if (a < 0x80) {
             //for single byte
             accum = a;
             state = 0;
         } else if (a >= 0x80 && a < 0xc0) {
			 if (!skipInvChars)
				throw InvalidUTF8Character(THISLOCATION, a);
			 accum = a;
			 state = 0;
         } else if (a >= 0xC0 && a < 0xE0) {
             accum = a & 0x1F, state = 1;
         } else if (a >= 0xE0 && a < 0xF0) {
             accum = a & 0x0F, state = 2;
         } else if (a >= 0xF0 && a < 0xF8) {
             accum = a & 0x07, state = 3;
         } else if (a >= 0xF8 && a < 0xFC) {
             accum = a & 0x03, state = 4;
         } else if (a >= 0xFC && a < 0xFE) {
             accum = a & 0x01, state = 5;
         } else if (a >= 0xFE && a < 0xFF) {
             accum = 0, state = 6;
         }
     } else {
         if (a >= 0x80 && a < 0xc0) {
             accum = (accum << 6) | (a & 0x3F);
             state--;
         } else {
             if (!skipInvChars) throw InvalidUTF8Character(THISLOCATION, a);
			 accum = a;
			 state = 0;
         }
     }
 }


  void Utf8ToWideFilter::flush()
 {
     if (!hasItems() && state != naturalNull)
         throw InvalidUTF8Character(THISLOCATION, 0);
 }



  void WideToUtf8Filter::input(const wchar_t & rdChar)
 {
     if (!needItems())
         throwWriteIteratorNoSpace(THISLOCATION,typeid(wchar_t));
     natural c = rdChar;

     byte fbyte;
     natural parts;
     rdpos = sizeof(data);
     if (c < 128) {
             fbyte = (char)c;
             parts = 0;
     } else if (c < 0x7ff) { // xx011 1111 1111
         fbyte = (char)(c >> 6) | 0xC0;
         parts = 1;
     } else if (c < 0xffff) {
         fbyte  = (char)(c >> 12) | 0xE0;
         parts = 2;
     } else if (c < 0x1fffff) {
         fbyte = (char)(c >> 18) | 0xF0;
         parts = 3;
     } else if (c < 0x3ffffff) {
         fbyte = (char)(c >> 24) | 0xF8;
         parts = 4;
     } else if (c < 0x7ffffff) {
         fbyte = (char)(c >> 30) | 0xFC;
         parts = 5;
     } else  {
         fbyte = 0xFE;
         parts = 6;
     }
     for (natural i = 0; i < parts; i++) {
         data[--rdpos] = 0x80 | (c & 0x3F);
         c >>= 6;
     }
     data[--rdpos] = fbyte;
 }



}

