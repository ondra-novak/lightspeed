/*
 * base64.h
 *
 *  Created on: Oct 15, 2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_BASE85_H_
#define LIGHTSPEED_UTILS_BASE85_H_
#include "../base/iter/iterConv.h"

#pragma once

namespace LightSpeed {

	///Conversion from bytes to base64
	/** Use with ConvertWriteIter or ConvertReadIter or convertString() function
	 * It require type 'byte' as input and returns char as output. If you have different types,
	 * you have to use BinaryConvert or NativeConvert to convert types
	 */
	class ByteToBase85Convert: public ConverterBase<byte, char, ByteToBase85Convert> {
	public:

		///constructs default byte to base64 convertor
		ByteToBase85Convert();

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

	protected:
		///temporary buffer and output buffer (at once)
		char outchars[6];

		Bin::natural32 accum;
		///current write stage
		byte wrpos;
		///current read position
		byte readpos;

		byte maxreadpos;

private:
	void reset();
};

	class Base85ToByteConvert: public ConverterBase<char, byte, Base85ToByteConvert> {
	public:
		Base85ToByteConvert();

		const byte &getNext();
		const byte &peek() const;
		void write(const char &item);
		void flush();

	protected:
		byte outBytes[4];
		Bin::natural32 accum;
		byte readpos;
		byte maxreadpos;
		byte wrpos;

private:
	void reset();
};

}

#endif /* LIGHTSPEED_UTILS_BASE85_H_ */
