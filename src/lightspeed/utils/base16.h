/*
 * base64.h
 *
 *  Created on: Oct 15, 2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_BASE16_H_
#define LIGHTSPEED_UTILS_BASE16_H_

#include "../base/containers/constStr.h"
#include "../base/iter/iterConv.h"


#pragma once

namespace LightSpeed {

	///Conversion from bytes to base64
	/** Use with ConvertWriteIter or ConvertReadIter or convertString() function
	 * It require type 'byte' as input and returns char as output. If you have different types,
	 * you have to use BinaryConvert or NativeConvert to convert types
	 */
	class ByteToBase16Convert: public ConverterBase<byte, char, ByteToBase16Convert> {
	public:

		///constructs default byte to base64 convertor
		ByteToBase16Convert();
		///constructs convertor with custom table and custom padding char
		/**
		 * @param table character table. You can use standardTable or forUrlTable. You can also
		 *  define own character table
		 * @param paddingChar defines padding char. You canuse standardPadding or noPadding, or any
		 *  other padding char as you need.
		 */
		ByteToBase16Convert(ConstStrA table);

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

		static const ConstStrA hexUpperCase;
		static const ConstStrA hexLowerCase;


	protected:
		///selected table
		const ConstStrA table;
		///temporary buffer and output buffer (at once)
		char outchars[2];
		///current read position
		byte readpos;


	};

	class Base16ToByteConvert: public ConverterBase<char, byte, Base16ToByteConvert> {
	public:
		Base16ToByteConvert();
		Base16ToByteConvert(ConstStrA table, bool caseSensitive = false);

		const byte &getNext();
		const byte &peek() const;
		void write(const char &item);
		void flush();

		static const ConstStrA hexUpperCase;
		static const ConstStrA hexLowerCase;
	protected:
		byte table[256];
		byte outByte;
		byte wrpos;

		void initTable(ConstStrA table, bool caseSensitive);
	};


}

#endif /* LIGHTSPEED_UTILS_BASE16_H_ */
