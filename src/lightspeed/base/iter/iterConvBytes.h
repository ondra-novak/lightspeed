/*
 * iterConvBytes.h
 *
 *  Created on: 12. 7. 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ITER_ITERCONVBYTES_H_
#define LIGHTSPEED_BASE_ITER_ITERCONVBYTES_H_
#include "iterConv.h"

namespace LightSpeed {

///Converts binary to binary various bits length
/** You can convert char to byte, or byte to char, int32 to uint32, etc. It is very fast if
 * used with streams. You can also convert natural to bytes, which will split each natural to eight bytes
 * (and vice versa)
 */
template<typename FromT, typename ToT>
class BinaryConvert: public ConverterBase<FromT, ToT, BinaryConvert<FromT, ToT> > {
public:

	BinaryConvert():writePos(0),readPos(0) {}

	void write(const FromT &item) {
		*reinterpret_cast<FromT *>(buffer+writePos) = item;
		writePos+=sizeof(item);
		this->hasItems = writePos > readPos+sizeof(ToT);
		this->needItems = writePos < needBuffer;
	}

	const ToT &getNext() {
		const ToT &ret = *reinterpret_cast<const ToT *>(buffer+readPos);
		readPos+=sizeof(ToT);
		this->hasItems = readPos+sizeof(ToT) < writePos;
		if (readPos >= writePos) {
			readPos = 0;
			writePos = 0;
			this->needItems = true;
			this->hasItems = false;
		}
		return ret;
	}
	const ToT &peek() const {
		const ToT &ret = reinterpret_cast<const ToT *>(buffer+readPos);
	}

	template<typename FlatArray, typename Iter>
	natural blockWrite(const FlatArray &array, Iter &target, bool writeAll = true) {
		const FromT *src = array.data();
		natural len = array.length();
		ConstStringT<ToT> out(reinterpret_cast<const ToT *>(src), len);
		return target.blockWrite(out, writeAll);
	}

	template<typename FlatArray, typename Iter>
	natural blockRead(Iter &source, FlatArray &array, bool readAll = true) {
		ToT *src = array.data();
		natural len = array.length();
		ArrayRef<FromT> in(reinterpret_cast<FromT *>(src), len);
		return source.blockRead(in, readAll);
	}

	template<typename Iter>
	void writeToIter(const FromT &item, Iter &target) {
		target.write((ToT)item);
	}

	template<typename Iter>
	const ToT &getNextFromIter(Iter &source) {
		const FromT &in = source.getNext();
		return reinterpret_cast<const ToT &>(in);
	}


	template<typename Iter>
	const ToT &peekFromIter(const Iter &source) const {
		const FromT &in = source.peek();
		return reinterpret_cast<const ToT &>(in);
	}
protected:
	static const natural inputBytes = sizeof(FromT);
	static const natural outputBytes = sizeof(ToT);
	static const natural needBuffer = inputBytes*outputBytes;

	natural writePos;
	natural readPos;

	byte buffer[needBuffer];

};

typedef BinaryConvert<byte,char> BytesToCharsConvert;
typedef BinaryConvert<char,byte> CharsToBytesConvert;
typedef BinaryConvert<byte,wchar_t> BytesToWideCharsConvert;
typedef BinaryConvert<wchar_t,byte> WideCharsToBytesConvert;

}


#endif /* LIGHTSPEED_BASE_ITER_ITERCONVBYTES_H_ */
