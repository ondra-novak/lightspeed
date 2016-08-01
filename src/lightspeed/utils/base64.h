/*
 * base64.h
 *
 *  Created on: Oct 15, 2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_BASE64_H_
#define LIGHTSPEED_UTILS_BASE64_H_

#include "../base/iter/iteratorFilter.h"
#include "../base/iter/iterConv.h"
#include "../base/containers/arrayref.h"
#include "../base/containers/flatArray.h"
#include "../base/containers/constStr.h"


#pragma once

namespace LightSpeed {

	///Conversion from bytes to base64
	/** Use with ConvertWriteIter or ConvertReadIter or convertString() function
	 * It require type 'byte' as input and returns char as output. If you have different types,
	 * you have to use BinaryConvert or NativeConvert to convert types
	 */
	class ByteToBase64Convert: public ConverterBase<byte, char, ByteToBase64Convert> {
	public:

		///constructs default byte to base64 convertor
		ByteToBase64Convert();
		///constructs convertor with custom table and custom padding char
		/**
		 * @param table character table. You can use standardTable or forUrlTable. You can also
		 *  define own character table
		 * @param paddingChar defines padding char. You canuse standardPadding or noPadding, or any
		 *  other padding char as you need.
		 */
		ByteToBase64Convert(ConstStrA table, char paddingChar);

		///Retrieves next character
		/**
		 * @return next character. You can read next character only when IConverter::hasItems is true
		 */
		const char &getNext();
		///Retrieves next character without discarding it
		/**
		 * @return next character. You can read next character only when IConverter::hasItems is true
		 */
		const char &peek() const;
		///Writes byte to the converter
		/**
		 * @param item byte to write
		 * @note you can write only if needItems is true
		 */
		void write(const byte &item);
		///Finishes Base64 sequence adding the padding.
		void flush();

		///Standard base64 table
		static const ConstStrA standardTable;
		///table useful for url encoding
		static const ConstStrA forUrlTable;
		///standard padding
		static const char standardPadding='=';
		///no padding
		static const char noPadding=0;


	protected:
		///selected table
		const ConstStrA table;
		///padding character
		const char padChar;
		///temporary buffer and output buffer (at once)
		char outchars[4];
		///current write stage
		byte wrbyte;
		///current read position
		byte readpos;


	};

	class Base64ToByteConvert: public ConverterBase<char, byte, Base64ToByteConvert> {
	public:
		Base64ToByteConvert();
		Base64ToByteConvert(ConstStrA table, char paddingChar);

		const byte &getNext();
		const byte &peek() const;
		void write(const char &item);
		void flush();

		///Standard base64 table
		static const ConstStrA standardTable;
		///table useful for url encoding
		static const ConstStrA forUrlTable;
		///standard padding
		static const char standardPadding='=';
		///no padding
		static const char noPadding=0;
	protected:
		byte table[256];
		char padChar;
		byte outBytes[3];
		byte readpos;
		byte wrpos;

		void initTable(ConstStrA table);
	};

//--------------- deprecated code ------------------------------------

	template<typename Bin, typename Char>
	class Base64EncoderT: public IteratorFilterBase<Bin, Char, Base64EncoderT<Bin,Char> > {
	public:
		Base64EncoderT(bool urlencode = false, bool noPadding = false);
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
		char eos;
        const byte *encodeTable;
		bool noPadding;
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
		char eos;
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
