#include "jsondefs.h"
#include "jsonparser.h"


namespace LightSpeed {

double strtod(const char *string, char **endPtr);

namespace JSON {



template<typename T>
Value Parser<T>::parse() {

		while (true) {
			char x = iter.getNext();
			switch (x) {
				case '"': return parseString();
				case '{': return parseObject(factory->object());
				case '[': return parseArray(factory->array());
				case 't': parseCheck(strTrue+1);
						  if (sharedTrue == null) sharedTrue = factory->newValue(true);
						  return sharedTrue;
				case 'f': parseCheck(strFalse+1);
						  if (sharedFalse == null) sharedFalse = factory->newValue(false);
						  return sharedFalse;
				case 'n': parseCheck(strNull+1);
						  if (sharedNull == null) sharedNull = factory->newNullNode();
						  return sharedNull;
				case ' ':
				case '\t':
				case '\r':
				case '\n':
				case '\b':
				case '\a':
				case '\0': break;
				default: return parseValue(x);
			}
		}
	}


template<typename T>
PNode Parser<T>::parseString() {
	parseRawString();
	if (strBuff.empty()) {
		if (sharedEmptyStr == null) sharedEmptyStr = factory->newValue(ConstStrA());
		return sharedEmptyStr;
	} else {
		return factory->newValue(ConstStrA(strBuff));
	}
}


template<typename T>
void Parser<T>::parseRawString() {
	char x = iter.getNext();
		strBuff.clear();
		while (x != '"') {
			if (x != '\\') {
				strBuff.add(x);
			} else {
				x = iter.getNext();
				char y;

				switch (x) {
					case '"': y = x;break;
					case '\\':y = x;break;
					case '/': y = x ;break;
					case 'b': y  = '\b';break;
					case 'f': y = '\f';break;
					case 'n': y = '\n'; break;
					case 'r': y = '\r';break;
					case 't': y = '\t';break;
					case 'u': {
							char buff[5];
							buff[4] = 0;
							for (int i = 0; i < 4; i++) {
								char z  = iter.getNext();
								if ((z >= '0' && z <= '9')||(z >='A' && z <= 'F') || (z >= 'a' && z <= 'f')) {
									buff[i] = (char)z;
								} else {
									throw ParseError_t(THISLOCATION,strBuff);
								}
							}
							wchar_t wy = (wchar_t)::LightSpeed::_intr::stringToUnsignedNumber<natural>(buff,16);
							WideToUtf8Filter flt;
							flt.input(wy);
							while (flt.hasItems()) strBuff.add(flt.output());
							x = iter.getNext();
							continue;
						}
					default: throw ErrorMessageException(THISLOCATION, "Neocekavana escape sekvence");
					//TODO: vymyslet lepsi vyjimku
				}
				strBuff.add(y);
			}
			x = iter.getNext();
		}
}


template<typename T>
PNode Parser<T>::parseObject(INode *container) {

	AutoArray<char, SmallAlloc<256> > name;
	Value obj = container;

	char c=iter.getNext();
	while (isspace(c)) c = iter.getNext();
	///empty object can be there
	if (c == '}') return obj;


	bool cont;
	do {

		while (isspace(c)) c= iter.getNext();
		if (c != '"') throw ParseError_t(THISLOCATION,ConstStrA());
		parseRawString();
		name.append(strBuff);

		char dcol = iter.getNext();
		while (isspace(dcol)) dcol = iter.getNext();
		if (dcol != ':')
				throw ParseError_t(THISLOCATION, ConstStrA(name));
		try {
			Value v = parse();
			obj->add(ConstStrA(name),v);
		} catch (Exception &e) {
			throw ParseError_t(THISLOCATION,name) << e;
		}
		name.clear();
		bool rep;
		do {
			c = iter.getNext();
			switch (c) {
			case ',': rep = false; cont = true;c = iter.getNext();break;
			case '}': rep = false; cont = false;break;
			default: if (isspace(c)) rep = true;
					else throw ParseError_t(THISLOCATION,name);
			}
		}while (rep);
	} while (cont);

	return obj;
}

template<typename T>
PNode Parser<T>::parseArray(INode *container) {

	Value arr = container;

	natural i = 0;

	char c=iter.peek();
	while (isspace(c)) {iter.skip();c = iter.peek();}
	///empty array can be there
	if (c == ']') {
		iter.skip();
		return arr;
	}


	bool cont;
	do {
		try {
			Value v = parse();
			arr->add(v);
		} catch (Exception &e) {
			throw ParseError_t(THISLOCATION,arrayIndexStr(i)) << e;
		}
		bool rep;
		do {
			char e = iter.getNext();
			switch (e) {
			case ',': rep = false; cont = true;break;
			case ']': rep = false; cont = false;break;
			default: if (isspace(e)) rep = true;
					else throw ParseError_t(THISLOCATION,arrayIndexStr(i));
			}
		}while (rep);
		i++;
	} while (cont);

	return arr;
}

template<typename T>
ConstStrA Parser<T>::arrayIndexStr(natural i) {
	char buff[100];
	strBuff.clear();
	strBuff.add('[');
	strBuff.append(::LightSpeed::_intr::numberToString(i,buff,100,10));
	strBuff.add(']');
	return strBuff;
}



template<typename T>
PNode Parser<T>::parseValue(char firstChar) {

	strBuff.clear();
	char c = iter.peek();
	if (isspace(firstChar)) {
		while(isspace(c)) {
			iter.skip();
			c = iter.peek();
		}
	} else {
		strBuff.add(firstChar);
	}

	bool isFloat = false;

	while (c != ',' && c != ']' && c != '}' && c != 0 && !isspace(c)) {
		if (c == '.' || c == 'E' || c == 'e') isFloat = true;
		strBuff.add(c);
		iter.skip();
		if (iter.hasItems()) c = iter.peek();
		else c = 0;
	}

	if (isFloat == false) {
		AutoArray<char>::Iterator iter = strBuff.getFwIter();
		if (strBuff.length() <= ((sizeof(integer)*10000+1818)/3636)) {
			integer x;
			if (!parseSignedNumber(iter,x,10) || iter.hasItems())
				throw ParseError_t(THISLOCATION,strBuff);
			if (x == 0) {
				if (sharedZero == null) sharedZero = factory->newValue((natural)0);
				return sharedZero;
			} else {
				return factory->newValue(x);
			}
		} else if (strBuff.length() <= ((sizeof(linteger)*10000+1818)/3636)) {
			linteger x;
			if (!parseSignedNumber(iter,x,10) || iter.hasItems())
				throw ParseError_t(THISLOCATION,strBuff);
			return factory->newValue(x);
		}
	}

	double v = ::LightSpeed::_intr::stringToFloatNumber<double>(strBuff);
		return factory->newValue(v);
}


template<typename T>
void Parser<T>::parseCheck(const char* str) {
	while (*str != 0) {
		char x = iter.getNext();
		if (x != *str) throw ParseError_t(THISLOCATION,"Unexpected character");
		str++;
	}
}

template<typename T>
JSON::Value StreamParser<T>::parse(ICallback *cb) {
	curCB = cb;
	prevContainer = 0;
	return parse();
}

template<typename T>
JSON::Value StreamParser<T>::parseObject(INode *container) {
	ICallback *save = curCB;
	INode *saveParent = prevContainer;
	if (save) curCB = save->onBeginContainer(container, prevContainer, this->strBuff);
	prevContainer = container;
	JSON::Value out = Parser<T>::parseObject(container);
	if (curCB) curCB->onContainerCompleted(container, saveParent);
	prevContainer = saveParent;
	if (curCB) curCB = save;
	return out;
}

template<typename T>
JSON::Value StreamParser<T>::parseArray(INode *container) {
	ICallback *save = curCB;
	INode *saveParent = prevContainer;
	if (save) curCB = save->onBeginContainer(container, prevContainer,ConstStrA());
	prevContainer = container;
	JSON::Value out = Parser<T>::parseArray(container);
	if (curCB) curCB->onContainerCompleted(container, saveParent);
	prevContainer = saveParent;
	if (curCB) curCB = save;
	return out;
}

template<typename T>
JSON::Value StreamParser<T>::parse() {
	JSON::Value out = Parser<T>::parse();
	if (curCB) out = curCB->onValue(out,prevContainer);
	return out;
}


}
}
