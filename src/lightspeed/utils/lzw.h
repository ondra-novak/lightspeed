/*
 * lzw.h
 *
 *  Created on: 21. 7. 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_LZW_H_
#define LIGHTSPEED_UTILS_LZW_H_
#include "../base/iter/iterConv.h"
#include <map>
namespace LightSpeed {


///simple stream compressor
/** LZW is simple and average effective compressor which can be used to
 * compress stream without buffering. It allows to write compressed and/or read
 * decompressed stream before it is finished. Through the convertor, you can
 * control the data flow on byte resolution.
 *
 */
class LZWCompress: public ConverterBase<byte, byte, LZWCompress> {
public:


	LZWCompress();
	const byte &getNext() ;
	const byte &peek() const;
	void write(const byte &b);
	void flush();

protected:

	static natural packCode(natural chainCode, byte nextByte);

	//the map cointais: packed code (chain+byte), and next chain code
	typedef std::map<natural, natural> DictTable;

	DictTable dictTable;
	natural accumulator;
	natural nextChainCode;
	natural curChainCode;
	byte obits;
	byte codeBits;
	mutable byte outbyte;

	static const natural clearCode=256;
	static const natural endData=257;
	static const natural firstChainCode = 258;
	static const byte initialBits=9;
	static const byte maxBits=15;

	void writeCode(natural code);
};



}



#endif /* LIGHTSPEED_UTILS_LZW_H_ */
