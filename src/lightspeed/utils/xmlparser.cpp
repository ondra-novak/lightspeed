#include "xmlparser.h"
#include "../base/countof.h"
#include "../base/memory/staticAlloc.h"
#include "../base/text/textIn.tcc"
#include "../base/streams/utf.tcc"
#include "../base/text/textstream.tcc"
#include "../base/iter/iteratorFilter.tcc"

namespace LightSpeed {


	static void setupScanner(ScanTextW &txtin) {
		txtin.nxChain().enableSkipInvalidChars(true); //skip invalid characters - all is treat as UTF-8 but document can be in different encoding
	}

	static void setupScanner(ScanTextWW &) {
	}

template<typename Scanner>
XMLIteratorT<Scanner>::XMLIteratorT( SeqFileInput &sfi,bool skipComments )
	:cur(&p1),next(&p2),txtin(sfi),curState(textBegin),skipComments(skipComments)
{
	setupScanner(txtin);
	loadNext();
}

template<typename Scanner>
bool XMLIteratorT<Scanner>::hasItems() const
{
	return next != 0;
}

template<typename Scanner>
const XMLEntity & XMLIteratorT<Scanner>::getNext()
{
	if (next == 0) throwIteratorNoMoreItems(THISLOCATION,typeid(XMLEntity));
	std::swap(cur,next);
	loadNext();
	return *cur;
}

template<typename Scanner>
const XMLEntity & XMLIteratorT<Scanner>::peek() const
{
	if (next == 0) throwIteratorNoMoreItems(THISLOCATION,typeid(XMLEntity));
	return *next;
}

template<typename Scanner>
void XMLIteratorT<Scanner>::loadNext()
{	
	switch (curState) {
		case textBegin:
			eatSpace();
			curState = textNext; //there is no break
		case textNext:
			if (!txtin.hasItems()) {
				next = 0;
				return;
			}
			if (txtin(L" <%")) {
				enterTag();
				if (next->type == XMLEntity::commentText && skipComments) {
					skipBlock();
				}
				return;
			}
			if (txtin(L"&%(1,10)1;%")) {
				next->chr = loadEntity(txtin[1].str());
				next->type = XMLEntity::textchar;
				return;
			} else {
				next->chr = txtin.getNext();
				next->type = XMLEntity::textchar;
				return;
			}

		case textCData:
			if (!txtin.hasItems()) {
				next = 0;
				return;
			}
			if (txtin(L"]]>%")) {
				curState = textNext;
				loadNext();
			}
			next->chr = txtin.getNext();
			next->type = XMLEntity::textchar;
			return;
		case incomment:
			if (txtin(L"-->%")) {
				curState = textNext;
				loadNext();
			}
			next->type = XMLEntity::commentText;
			next->chr = txtin.getNext();
			return;
		case intag:
			readAttr();
			return;
		case inendtag: 
			next->type = XMLEntity::opentag;
			next->endTag = true;
			next->tag = curTag;
			curState = inclosetag;
			return;
		case inclosetag:
			next->type = XMLEntity::closetag;
			next->endTag = true;
			next->tag = curTag;
			curState = textBegin;
			return;
		case incloseendtag:
			return;

	}
}

template<typename Scanner>
void XMLIteratorT<Scanner>::eatSpace()
{
	txtin(L" ");
}

template<typename Scanner>
wchar_t XMLIteratorT<Scanner>::loadEntity(ConstStrW entityName)
{
	if (entityName == ConstStrW(L"lt")) return '<';
	if (entityName == ConstStrW(L"gt")) return '>';
	if (entityName == ConstStrW(L"amp")) return '&';
	if (entityName == ConstStrW(L"apos")) return '\'';
	if (entityName == ConstStrW(L"quot")) return '"';
	if (entityName[0] == '#') {
		natural r;
		if (entityName.length()>1 && entityName[1] == 'h') {
			parseUnsignedNumber<ConstStrW::Iterator,natural>(entityName.getFwIter(),r,16);
		} else {
			parseUnsignedNumber<ConstStrW::Iterator,natural>(entityName.getFwIter(),r,10);
		}
		return (wchar_t)r;
	} 
	return '&';
}

template<typename Scanner>
void XMLIteratorT<Scanner>::readAttr()
{
	if (txtin(L" />%")) {
		next->type = XMLEntity::closetag;
		next->tag = curTag;
		next->endTag = cur->endTag;
		curState = inendtag;
		return;
	} else if (txtin(L" >%") || txtin(L" ?>%")) {
		next->type = XMLEntity::closetag;
		next->tag = curTag;
		next->endTag = cur->endTag;
		curState = textBegin;
		return;
	} else if (txtin(L" %[*^>]1='%[*^>]2'%") || txtin(L" %[*^>]1=\"%[*^>]2\"%") || txtin(L" %[*^>]1=%[*^>]2 %")) {
		next->type = XMLEntity::attribute;
		next->tag = curTag;		
		curAttr = fixEntities(txtin[1].str());
		curValue = fixEntities(txtin[2].str());
		next->attr = curAttr;
		next->value = curValue;
		return;
	} else {
		next->type = XMLEntity::closetag;
		next->tag = curTag;
		next->endTag = cur->endTag;
		curState = textBegin;
		return;

	}
}

template<typename Scanner>
void XMLIteratorT<Scanner>::enterTag()
{
	if (txtin(L"[CDATA[%")) {
		curState = textCData;
		loadNext();
	} else if (txtin(L"--")) {
		curState = incomment;
		loadNext();
	} else if (txtin(L"/%[0-9a-zA-Z:_]1 >%")) {
		curTag = txtin[1].str();
		next->type = XMLEntity::opentag;
		next->tag = curTag;
		next->endTag = true;
		curState = inclosetag;
		return;

	} else if (txtin(L"%[!?0-9a-zA-Z:_]1%%")) {
		curTag = txtin[1].str();
		next->type = XMLEntity::opentag;
		next->tag = curTag;
		next->endTag = false;
		curState = intag;
		return;
	} else {
		next->type = XMLEntity::textchar;
		next->chr = '<';
		curState = textNext;
	}
}

template<typename Scanner>
String XMLIteratorT<Scanner>::fixEntities(ConstStrW text)
{
	natural x = text.find('&');
	if (x == naturalNull) return text;
	AutoArrayStream<wchar_t> buff;
	while (x != naturalNull) {
		buff.blockWrite(text.head(x),true);
		text = text.offset(x+1);
		natural y = text.find(';');
		if (y != naturalNull) {
			buff.write(loadEntity(text.head(y)));
			text = text.offset(y+1);
		} else {
			buff.write('&');
		}
		x = text.find('&');
	}
	buff.blockWrite(text.constRef(),true);
	return buff.getArray();
}

template<typename Scanner>
XMLIteratorT<Scanner> &XMLIteratorT<Scanner>::skipBlock()
{
	XMLEntity::Type t  = cur->type;
	while (peek().type == t) this->skip();
	return *this;
}

template<typename Scanner>
String XMLIteratorT<Scanner>::readText()  {
	const XMLEntity *e = &this->peek();
	AutoArrayStream<wchar_t, SmallAlloc<100> > buffer;
	while (e->isText()) {
		buffer.write(e->chr);
		this->skip();
		e = &this->peek();
	}
	return buffer.getArray();
}


template<typename Scanner>
XMLIteratorT<Scanner> &LightSpeed::XMLIteratorT<Scanner>::skipTag()
{
	while (peek().type == XMLEntity::attribute || peek().type == XMLEntity::closetag) this->skip();
	return *this;
}

template class XMLIteratorT < ScanTextW > ;
template class XMLIteratorT < ScanTextWW > ;

}
