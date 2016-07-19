// $Id: textstream.h 573 2014-09-12 15:51:50Z ondrej.novak $

#include "../iter/iteratorFilter.h"
#include "../streams/utf.h"
#include "textOut.h"
#include "textIn.h"
#include "../memory/smallAlloc.h"
#include "../streams/fileio.h"
#include "textLineReader.h"
#include "../iter/iterConv.h"
#include "../iter/iterConvBytes.h"

namespace LightSpeed {


	typedef ConvertReadIter<BytesToCharsConvert, SeqFileInput> SeqTextInA;
	typedef ConvertWriteIter<CharsToBytesConvert, SeqFileOutput> SeqTextOutA;
	typedef ConvertReadChain<Utf8ToWideConvert, SeqTextInA> SeqTextInW;
	typedef ConvertWriteChain<WideToUtf8Convert, SeqTextOutA> SeqTextOutW;
	typedef ConvertReadIter<BytesToWideCharsConvert, SeqFileInput> SeqTextInWW;
	typedef ConvertWriteIter<WideCharsToBytesConvert, SeqFileOutput> SeqTextOutWW;
	
/*	typedef Filter<TypeConversionFilter<byte,char> > ByteToCharFilter;
	typedef Filter<TypeConversionFilter<char,byte> > CharToByteFilter;	

	///convert output byte stream to output ansi stream
	typedef FltChain<SeqFileOutput, CharToByteFilter::Write<SeqFileOutput> >SeqTextOutA;
	///convert output byte stream to output wide-char stream (utf-8)
	typedef FltChain<SeqTextOutA, WideToUtf8Writer<SeqTextOutA> > SeqTextOutW;
	///convert input byte stream to input ansi stream
	typedef FltChain<SeqFileInput, ByteToCharFilter::Read<SeqFileInput> > SeqTextInA;
	///convert input byte stream to input wide-char stream (utf-8)
	typedef FltChain<SeqTextInA, Utf8ToWideReader<SeqTextInA> > SeqTextInW;
	///convert output byte stream to output wide-char stream (UCS-2)
	typedef FltChain<SeqFileOutput, Filter<WideToBytes>::Read<SeqFileOutput> > SeqTextOutWW;
	///convert input byte stream to input wide-char stream (UCS-2)
	typedef FltChain<SeqFileInput, Filter<BytesToWide>::Read<SeqFileInput> > SeqTextInWW;
*/
	typedef TextOut<SeqTextOutA, SmallAlloc<256> > PrintTextA;
	typedef TextOut<SeqTextOutW, SmallAlloc<256> > PrintTextW;
	typedef TextIn<SeqTextInA, SmallAlloc<256> > ScanTextA;
	typedef TextIn<SeqTextInW, SmallAlloc<256> > ScanTextW;
	typedef TextOut<SeqTextOutWW, SmallAlloc<256> > PrintTextWW;
	typedef TextIn<SeqTextInWW, SmallAlloc<256> > ScanTextWW;

	typedef TextLineReader<SeqTextInA> SeqTextLineInA;
	typedef TextLineReader<SeqTextInW> SeqTextLineInW;
	typedef TextLineReader<SeqTextInWW> SeqTextLineInWW;

}
