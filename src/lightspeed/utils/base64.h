/*
 * base64.h
 *
 *  Created on: Oct 15, 2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_BASE64_H_
#define LIGHTSPEED_UTILS_BASE64_H_

#include "../base/iter/iteratorFilter.h"
#include "../base/containers/arrayref.h"
#include "../base/containers/flatArray.h"
#include "../base/containers/constStr.h"

#pragma once

namespace LightSpeed {

	template<typename Bin, typename Char>
	class Base64EncoderT: public IteratorFilterBase<Bin, Char, Base64EncoderT<Bin,Char> > {
	public:
		Base64EncoderT(bool urlencode = false);
        bool needItems() const;
        void input(const Bin &x);
        bool hasItems() const;
        Char output();
        natural calcToWrite(natural srcCount) const;
        natural calcToRead(natural trgCount) const;
        void flush();
	protected:
        natural buffer;
        char cycle;
        char eqchr;
        const byte *encodeTable;
	};

	typedef Base64EncoderT<byte,char> Base64Encoder;
	typedef Base64EncoderT<char,char> Base64EncoderChr;
	typedef Base64EncoderT<byte,byte> Base64EncoderBin;


	template<typename Char, typename Bin>
	class Base64DecoderT: public IteratorFilterBase<Char, Bin, Base64DecoderT<Char,Bin> > {
	public:
		Base64DecoderT();
        bool needItems() const;
        bool canAccept(const Char &x) const;
        void input(const Char &x);
        bool hasItems() const;
        Bin output();
        natural calcToWrite(natural srcCount) const;
        natural calcToRead(natural trgCount) const;
        void flush();
	protected:
        natural buffer;
        char cycle;
        char eqchr;
	};

	typedef Base64DecoderT<char,byte> Base64Decoder;
	typedef Base64DecoderT<char,char> Base64DecoderChr;
	typedef Base64DecoderT<byte,byte> Base64DecoderBin;



	template<typename Iter>
	class ReadAndEncodeBase64: public Filter<Base64Encoder>::Read<Iter> {
	public:
		ReadAndEncodeBase64(Iter iter):Filter<Base64Encoder>::Read<Iter>(iter) {}
	};

	template<typename Iter>
	class ReadAndDecodeBase64: public Filter<Base64Decoder>::Read<Iter> {
	public:
		ReadAndDecodeBase64(Iter iter):Filter<Base64Decoder>::Read<Iter>(iter) {}
	};

	template<typename Iter>
	class WriteAndEncodeBase64: public Filter<Base64Encoder>::Write<Iter> {
	public:
		WriteAndEncodeBase64(Iter iter):Filter<Base64Encoder>::Write<Iter>(iter) {}
	};

	template<typename Iter>
	class WriteAndDecodeBase64: public Filter<Base64Decoder>::Write<Iter> {
	public:
		WriteAndDecodeBase64(Iter iter):Filter<Base64Decoder>::Write<Iter>(iter) {}
	};


	
	template<typename In = byte, typename Out = char>
	class HexEncoder: public IteratorFilterBase<In,Out,HexEncoder<In,Out> > {
	public:
		HexEncoder():idx(2),hexchars("0123456789abcdef") {}
		HexEncoder(ConstStrA hexchars):idx(2),hexchars(hexchars) {}
        bool needItems() const;
        void input(const In &x);
        bool hasItems() const;
        Out output();
        natural calcToWrite(natural srcCount) const;
        natural calcToRead(natural trgCount) const;
        void flush();
	protected:
        byte buff[2];
        int idx;
        ConstStrA hexchars;


	};
	
	template<typename In = char, typename Out = byte>
	class HexDecoder: public IteratorFilterBase<In,Out,HexDecoder<In,Out> > {
	public:
		HexDecoder():idx(0) {loadHexChars("0123456789abcdef");}
		HexDecoder(ConstStrA hexchars):idx(2) {loadHexChars(hexchars);}
        bool needItems() const;
        void input(const In &x);
        bool hasItems() const;
        Out output();
        natural calcToWrite(natural srcCount) const;
        natural calcToRead(natural trgCount) const;
        void flush();
	protected:
        byte buff[2];
        int idx;
        byte hexchars[256];
        void loadHexChars(ConstStrA hexchars);


	};

template<typename In,  typename Out >
inline bool HexEncoder<In, Out>::needItems() const {
	return idx == 2;
}

template<typename In,  typename Out >
inline void HexEncoder<In, Out>::input(const In& x) {
	byte c = (byte)x;
	buff[0] = c >> 4;
	buff[1] = c & 0xf;
	idx=0;
}

template<typename In,  typename Out >
inline bool HexEncoder<In, Out>::hasItems() const {
	return idx != 2;
}

template<typename In,  typename Out >
inline Out HexEncoder<In, Out>::output() {
	byte c = buff[idx++];
	return hexchars[c];
}
template<typename In,  typename Out >
inline void HexEncoder<In, Out>::flush() {

}


template<typename In, typename Out>
inline bool HexDecoder<In, Out>::needItems() const {
	return idx < 2;
}

template<typename In, typename Out>
inline void HexDecoder<In, Out>::input(const In& x) {
	buff[idx] = hexchars[(byte)x];
	++idx;
}

template<typename In, typename Out>
inline bool HexDecoder<In, Out>::hasItems() const {
	return idx >= 2;
}

template<typename In, typename Out>
inline Out HexDecoder<In, Out>::output() {
	idx = 0;
	return (buff[0] << 4) | buff[1];
}

template<typename In, typename Out>
inline natural HexDecoder<In, Out>::calcToWrite(
		natural srcCount) const {
	return srcCount / 2;
}

template<typename In, typename Out>
inline natural HexDecoder<In, Out>::calcToRead(
		natural trgCount) const {
	return trgCount * 2;
}

template<typename In, typename Out>
inline void HexDecoder<In, Out>::flush() {
	if (idx == 1) idx++;
}


template<typename In , typename Out >
inline void LightSpeed::HexDecoder<In, Out>::loadHexChars(ConstStrA hexchars) {
	for (natural i = 0; i < hexchars.length(); i ++) {
		this->hexchars[(byte)hexchars[i]] = i;
	}
}

}

#endif /* LIGHTSPEED_UTILS_BASE64_H_ */
