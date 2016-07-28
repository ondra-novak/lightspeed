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
template<typename Impl>
class LZWCompressBase: public ConverterBase<byte, byte, Impl>, public LZWCommonDefs {
public:



	LZWCompressBase(natural maxBits = 12);
	const byte &getNext() ;
	const byte &peek() const;
	void write(const byte &b);
	void flush();

protected:


	struct ChainCodeTrace {
		ChainCode code;
		bool trace;

		ChainCodeTrace(ChainCode code):code(code),trace(false) {}
		operator ChainCode() const {return code;}
		void mark() {trace=true;}
		bool marked() const {return trace;}

	};

	//the map cointais: packed code (chain+byte), and next chain code
	typedef std::map<natural, ChainCodeTrace> DictTable;

	DictTable dictTable;
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

	class OutBuff {
	public:
		byte outbuff[16];
		byte wrpos;
		byte rdpos;
		byte wrbits;
		byte accum;

		OutBuff();
		const byte &getNext();
		const byte &peek();
		void write(ChainCode code, byte bits);
		bool hasItems() const;
		void padzeroes();


	};

	mutable OutBuff outbuff;


	void writeCode(ChainCode code);
	void writeClearCode(ChainCode code);
	void increaseBits(ChainCode newCode);

	//allows to write own handler of clear code - for example if you want to prune dictionary, not clear
	void onClearCode();
};

template<typename Impl>
class LZWDecompressBase: public ConverterBase<byte, byte, Impl>, public LZWCommonDefs {
public:

	LZWDecompressBase();
	const byte &getNext() ;
	const byte &peek() const;
	void write(const byte &b);
	void flush();

protected:

	struct CodeInfo {
		byte outByte;
		natural prevCode;
		mutable bool marked;

		CodeInfo(byte outByte,natural prevCode)
			:outByte(outByte),prevCode(prevCode),marked(false) {}
		void mark() const {marked = true;}
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

	///called when unknown code detected
	/** default implementation will throw an exception. However extension can benefit from it */
	void onUnknownCode(ChainCode code);
	//allows to write own handler of clear code - for example if you want to prune dictionary, not clear
	void onClearCode();

};

class LZWCompress: public LZWCompressBase<LZWCompress> {
public:
	LZWCompress(natural maxBits = 12):LZWCompressBase<LZWCompress>(maxBits) {}
protected:
	void onClearCode();
	friend class LZWCompressBase<LZWCompress>;

};

class LZWDecompress: public LZWDecompressBase<LZWDecompress> {
public:
protected:
	void onClearCode();
	friend class LZWDecompressBase<LZWDecompress>;
};

class LZWpCompress: public LZWCompressBase<LZWpCompress> {
public:
	LZWpCompress(natural maxBits = 12);
	friend class LZWCompressBase<LZWpCompress>;


protected:

	void onClearCode();

	void optimizeDictionary();
	ChainCode optimizeStart;
	ChainCode optimizeCode;

};

class LZWpDecompress: public LZWDecompressBase<LZWpDecompress> {
public:


protected:
	friend class LZWDecompressBase<LZWpDecompress>;

	void onClearCode();
	void optimizeDictionary();

};

}



#endif /* LIGHTSPEED_UTILS_LZW_H_ */
