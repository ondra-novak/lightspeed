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


class LZWCommonDefs {
public:
	typedef Bin::natural16 ChainCode;

	static const ChainCode clearCode=256;
	static const ChainCode endData=257;
	static const ChainCode firstChainCode = 258;
	static const ChainCode nullChainCode=ChainCode(-1);
	static const natural maxAllowedBits=15;
	static const natural initialBits=9;
};

///simple stream compressor
/** LZW is simple and average effective compressor which can be used to
 * compress stream without buffering. It allows to write compressed and/or read
 * decompressed stream before it is finished. Through the convertor, you can
 * control the data flow on byte resolution.
 *
 */
class LZWCompress: public ConverterBase<byte, byte, LZWCompress>, public LZWCommonDefs {
public:



	LZWCompress(natural maxBits = maxAllowedBits);
	const byte &getNext() ;
	const byte &peek() const;
	void write(const byte &b);
	void flush();

protected:

	static natural packCode(natural chainCode, byte nextByte);

	//the map cointais: packed code (chain+byte), and next chain code
	typedef std::map<natural, natural> DictTable;

	DictTable dictTable;
	///accumulator contains
	natural accumulator;
	///next chain code available to open
	ChainCode nextChainCode;
	///currently opened chain code
	ChainCode curChainCode;
	///count of bits in accumulator
	natural obits;
	///current count of bits
	natural codeBits;
	///when this code is created, full or clear is performed
	ChainCode maxChainCode;
	///code used for partial clean of dictionary. It should be maxChainCode+2
	/** stream decoded by unsupported decompressor will report this code as error */
	ChainCode partialClearCode;

	mutable byte outbyte;


	void writeCode(ChainCode code);
	void writeClearCode(ChainCode code);
	void increaseBits(ChainCode newCode);
};

class LZWDecompress: public ConverterBase<byte, byte, LZWDecompress>, public LZWCommonDefs {
public:



	LZWDecompress();
	const byte &getNext() ;
	const byte &peek() const;
	void write(const byte &b);
	void flush();

protected:

	struct CodeInfo {
		byte outByte;
		natural prevCode;

		CodeInfo(byte outByte,natural prevCode)
			:outByte(outByte),prevCode(prevCode) {}
	};

	typedef AutoArray<CodeInfo> DictTable;
	DictTable dict;

	ChainCode prevCode;

	natural accum;
	natural inbits;
	natural codeBits;
	AutoArray<byte> outbytes;



	void extractCode(natural code, natural reserve);

	bool appendBits(byte bits, ChainCode &codeOut);
	void clearDict();
};



}



#endif /* LIGHTSPEED_UTILS_LZW_H_ */
