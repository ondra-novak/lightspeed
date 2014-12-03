#include "cmdLineIterator.h"
#include "../exceptions/throws.h"
#include "../text/textParser.tcc"
#include "../exceptions/invalidNumberFormat.h"

namespace LightSpeed {


CmdLineIterator::CmdLineIterator( const IApp::Args &args, natural offset )
:args(args),argPos(offset),charPos(0),readRaw(false)
{

}

const CmdLineItem & CmdLineIterator::getNext()
{
	const CmdLineItem &ret = peek();
	if (ret.itemType == CmdLineItem::shortSwitch) {
		if (charPos == 0) charPos+=2; else charPos++;
		if (charPos >= args[argPos].length()) {
			charPos = 0;
			argPos++;
		}
	} else {
		argPos++;
		charPos = 0;
	}
	return ret;
}

const CmdLineItem & CmdLineIterator::peek() const
{
	if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(CmdLineItem));
	ConstStrW a = args[argPos];
	if (charPos == 0) {
		natural len = a.length();
		if (a[0] == '-') {
			if (len>1) {
				if (a[1] == '-') {
					tmp = CmdLineItem(CmdLineItem::longSwitch,a.offset(2));
					return tmp;
				} else {
					tmp = CmdLineItem(CmdLineItem::shortSwitch,a.mid(1,1));
					return tmp;
				}
			} 
		}
		tmp = CmdLineItem(CmdLineItem::text, a);
		return tmp;
	} else {
		tmp = CmdLineItem(CmdLineItem::shortSwitch,a.mid(charPos,1));
		return tmp;
	}
}

bool CmdLineIterator::hasItems() const
{
	return argPos < args.length();		
}

bool CmdLineIterator::equalTo( const CmdLineIterator &other ) const
{
	return argPos == other.argPos && charPos == other.charPos;
}

bool CmdLineIterator::lessThan( const CmdLineIterator &other ) const
{
	return argPos < other.argPos ||
		(argPos == other.argPos && charPos < other.charPos);
}

ConstStrW CmdLineIterator::peekText() const
{
	if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(ConstStrW));
	return args[argPos].offset(charPos);
}

ConstStrW CmdLineIterator::getNextText()
{
	ConstStrW res = peekText();
	charPos = 0;
	argPos++;
	return res;
}

IApp::Args CmdLineIterator::getRemainArgs()
{
	
	return IApp::Args(args.offset(argPos));
}

integer CmdLineIterator::getNextNumber(natural base) {
	ConstStrW text = getNextText();
	integer out;
	if (!parseSignedNumber(text.getFwIter(),out,base)) {
		throw InvalidNumberFormatException(THISLOCATION,text);
	}
	return out;
}

}

