/*
 * binjson.cpp
 *
 *  Created on: 5. 3. 2016
 *      Author: ondra
 */


#include "../../base/streams/fileio.h"
#include "binjson.h"

#include "../../base/streams/utf.h"
#include "../../base/text/textstream.h"
#include "../../base/text/toString.h"
#include "../../mt/atomic.h"
#include "jsondefs.h"
#include "jsonexception.h"
#include "jsonserializer.tcc"
#include "../../base/containers/map.tcc"

namespace LightSpeed  {
namespace JSON {

JsonToBinary::JsonToBinary():nxtCode(firstCode) {
}



void JsonToBinary::serialize(const JSON::Value& val, SeqFileOutput& output) {
	serializeValue(*val, output);
}

void JsonToBinary::clearEnumTable() {
	strMap.clear();
	nxtCode = firstCode;
}

void JsonToBinary::addEnum(ConstStrA string) {
	strMap.insert(string,nxtCode++);
}

void JsonToBinary::serializeValue(const JSON::INode& val,
		SeqFileOutput& output) {

	JSON::NodeType t = val.getType();
	switch(t) {
	case ndArray: serializeArray(val,output);break;
	case ndObject: serializeObject(val,output);break;
	case ndFloat: serializeFloat(val,output);break;
	case ndInt: serializeInteger(val,output);break;
	case ndBool: serializeBool(val,output);break;
	case ndNull: serializeNull(val,output);break;
	case ndString: serializeString(val,output);break;
	default: throwUnsupportedFeature(THISLOCATION,this,"Unsupported JSON-node type");break;
	}

}

void JsonToBinary::serializeArray(const JSON::INode& val, SeqFileOutput& output) {
	output.write(opcArray);
	for (JSON::ConstIterator iter = val.getFwConstIter(); iter.hasItems();) {
		serializeValue(*iter.getNext(),output);
	}
	output.write(opcEnd);
}

void JsonToBinary::serializeObject(const JSON::INode& val, SeqFileOutput& output) {
	output.write(opcObject);
	for (JSON::ConstIterator iter = val.getFwConstIter(); iter.hasItems();) {
		const ConstKeyValue &kv = iter.getNext();
		writeString(kv.getStringKey(),output);
		serializeValue(*kv,output);
	}
	output.write(opcEnd);
}

void JsonToBinary::serializeFloat(const JSON::INode& val,SeqFileOutput& output) {
	output.write(opcFloat64);
	double v = val.getFloat();
	output.blockWrite(&v,sizeof(v),true);
}

void JsonToBinary::serializeInteger(const JSON::INode& val,SeqFileOutput& output) {
	linteger v = val.getLongInt();
	if (v < 0) {
		lnatural n = (lnatural)(-v);
		writeNumber(opcNegInt1,n,output);
	} else if (v > 0) {
		lnatural n = (lnatural)(-v);
		writeNumber(opcPosInt1,n,output);
	} else {
		output.write(opcZero);
	}
}

void JsonToBinary::serializeBool(const JSON::INode& val,SeqFileOutput& output) {
	bool b = val.getBool();
	if (b) {
		output.write(opcTrue);
	} else {
		output.write(opcFalse);
	}
}

void JsonToBinary::serializeNull(const JSON::INode& ,SeqFileOutput& output) {
	output.write(opcNull);
}

void JsonToBinary::serializeString(const JSON::INode& val,SeqFileOutput& output) {
	ConstStrA str = val.getStringUtf8();
	writeString(str,output);
}

void JsonToBinary::writeString(ConstStrA string, SeqFileOutput& output) {
	if (string.empty()) {
		output.write(opcEmptyString);
		return;
	}
	const natural *p = strMap.find(string);
	if (p == 0) {
		natural ln = string.length();
		writeNumber(opcString1, ln, output);
		output.blockWrite(string.data(),string.length(),true);
	} else {
		wchar_t code = (wchar_t)*p;
		WideToUtf8Writer<SeqTextOutA> w(output);
		w.write(code);
	}
}

void JsonToBinary::writeNumber(OpCode op, lnatural v, SeqFileOutput& output) {
	if (v<=255) {
		output.write(op);
		output.write((byte)(v & 0xFF));
	} else if (v <= 0xFFFF) {
		output.write(op+1);
		output.write((byte)(v & 0xFF));
		output.write((byte)((v>>8) & 0xFF));
	} else if (v <= 0xFFFFFFFF) {
		output.write(op+2);
		output.write((byte)(v & 0xFF));
		output.write((byte)((v>>8) & 0xFF));
		output.write((byte)((v>>16) & 0xFF));
		output.write((byte)((v>>24) & 0xFF));
	} else  {
		output.write(op+3);
		output.write((byte)(v & 0xFF));
		output.write((byte)((v>>8) & 0xFF));
		output.write((byte)((v>>16) & 0xFF));
		output.write((byte)((v>>24) & 0xFF));
		output.write((byte)((v>>32) & 0xFF));
		output.write((byte)((v>>40) & 0xFF));
		output.write((byte)((v>>48) & 0xFF));
		output.write((byte)((v>>56) & 0xFF));
	}
}


BinaryToJson::BinaryToJson() {
}

class EnumNode: public LeafNode, public DynObject {
public:
	EnumNode(ConstStrA text):text(text),wide(0) {}
	~EnumNode() {
		if (wide) delete wide;
	}

	virtual ConstStrA getStringUtf8() const {return text;}
	virtual ConstStrW getString() const {
		if (wide) return *wide;
		String *z = new String(text);
		if (lockCompareExchangePtr<String>(&wide,z,0) != 0) {
			delete z;
		}
		return *wide;
	}
	virtual NodeType getType() const {return ndString;}
	virtual integer getInt() const {
		integer x= 0;
		parseSignedNumber(text.getFwIter(),x,10);
		return x;
	}
	virtual linteger getLongInt() const {
		linteger x= 0;
		parseSignedNumber(text.getFwIter(),x,10);
		return x;
	}
	virtual natural getUInt() const {
		natural x= 0;
		parseUnsignedNumber(text.getFwIter(),x,10);
		return x;
	}
	virtual lnatural getLongUInt() const {
		lnatural x= 0;
		parseUnsignedNumber(text.getFwIter(),x,10);
		return x;
	}
	virtual double getFloat() const {
		TextParser<char, SmallAlloc<256> > parser;
		double out = 0;
		if (parser(" %f1 ", text)) out = parser[1];
		return out;
	}
	virtual bool getBool() const {
		return !text.empty();
	}
	virtual bool isNull() const {
		return false;
	}
	virtual bool operator==(const INode &other) const {
		return getStringUtf8() == other.getStringUtf8();
	}
	virtual Value clone(PFactory factory) const {return new(*factory->getAllocator()) EnumNode(text);}


protected:
	ConstStrA text;
	mutable String  *volatile wide;

};


JSON::Value BinaryToJson::parse(SeqFileInput& input) {

	Utf8ToWideReader<SeqTextInA> w(input);
	wchar_t code = (OpCode)(w.getNext());
	switch ((OpCode)code) {
	case opcArray: return parseArray(input);
/*	case opcBinary1:
	case opcBinary2:
	case opcBinary4:
	case opcBinary8: return parseBinary(code-opcBinary1, input);*/
	case opcEmptyString: return factory->newValue(ConstStrA());
	case opcEnd: return null;
	case opcFalse: return factory->newValue(false);
	case opcFloat32: return parseFloat32(input);
	case opcFloat64: return parseFloat64(input);
	case opcNegInt1:
	case opcNegInt2:
	case opcNegInt4:
	case opcNegInt8: return parseNumber(code-opcNegInt1, true, input);
	case opcNull: return factory->newNullNode();
	case opcObject: return parseObject(input);
	case opcPick1:
	case opcPick2:
	case opcPick4: {
				natural n = readNumber(code-opcPick1, input);
				return stack[stack.length()-1-n];
			}
	case opcPop: {
			JSON::Value v = stack[stack.length()-1];
			stack.trunc(1);
			return v;
		}
	case opcPosInt1:
	case opcPosInt2:
	case opcPosInt4:
	case opcPosInt8:return parseNumber(code-opcPosInt1, false, input);
	case opcPush: stack.add(parse(input));return parse(input);
	case opcRemove1: {
			natural n = readNumber(0,input);
			stack.trunc(n);
			return parse(input);
		}
	case opcString1:
	case opcString2:
	case opcString4:
	case opcString8: return parseString(code, input);
	case opcTrue: return factory->newValue(true);
	case opcZero: return factory->newValue((natural)0);
	default: {
		IRuntimeAlloc *alloc = factory->getAllocator();
		if (code < opcFirstCode) {
			throw ParseError_t(THISLOCATION,ConstStrA("<binary> - Invalid opcode:")+ToString<natural>(code));
		}
		natural c = code-opcFirstCode;
		if (c >= stringTable.length()) {
			return factory->newValue(StringA((ConstStrA("Undefined")+ToString<natural>(c))));
		} else {
			return new(*alloc) EnumNode(stringTable[c]);
		}
	}
	}

}

void BinaryToJson::convertToText(SeqFileInput& binIn, SeqTextOutA& textOut) {
	Utf8ToWideReader<SeqTextInA> w(binIn);
	wchar_t code = (OpCode)(w.getNext());
	switch ((OpCode)code) {
	case opcArray: convertArrayToText(binIn,textOut);break;
/*	case opcBinary1:
	case opcBinary2:
	case opcBinary4:
	case opcBinary8: return parseBinary(code-opcBinary1, input);*/
	case opcEmptyString:
			textOut.write('"');
			textOut.write('"');
			break;
	case opcFalse:
			textOut.blockWrite(ConstStrA("false"),true);
			break;
	case opcFloat32: convertFloat32ToText(binIn,textOut);break;
	case opcFloat64: convertFloat64ToText(binIn,textOut);break;
	case opcNegInt1:
	case opcNegInt2:
	case opcNegInt4:
	case opcNegInt8:
			textOut.write('-');
			textOut.blockWrite(ToString<lnatural>(readNumber(code-opcNegInt1,binIn)),true);
				break;
	case opcNull: textOut.blockWrite(ConstStrA("null"),true);
				break;
	case opcObject: convertObjectToText(binIn,textOut);
				break;
	case opcPick1:
	case opcPick2:
	case opcPick4: {
				natural n = readNumber(code-opcPick1, binIn);
				JSON::Value v =  stack[stack.length()-1-n];
				serialize(v,textOut,true);
			}break;
	case opcPop: {
			JSON::Value v = stack[stack.length()-1];
			stack.trunc(1);
			serialize(v,textOut,true);
		}break;
	case opcPosInt1:
	case opcPosInt2:
	case opcPosInt4:
	case opcPosInt8:
		textOut.blockWrite(ToString<lnatural>(readNumber(code-opcPosInt1,binIn)),true);
	case opcPush: stack.add(parse(binIn));convertToText(binIn,textOut);
		break;
	case opcRemove1: {
			natural n = readNumber(0,binIn);
			stack.trunc(n);
			convertToText(binIn, textOut);
			break;
		}
	case opcString1:
	case opcString2:
	case opcString4:
	case opcString8:
			 convertStringToText(code, binIn, textOut);
			 break;
	case opcTrue:
			textOut.blockWrite(ConstStrA("true"),true);break;
	case opcZero:
			textOut.blockWrite(ConstStrA("0"),true);break;
	default: {
		if (code < opcFirstCode) {
			throw ParseError_t(THISLOCATION,ConstStrA("<binary> - Invalid opcode:")+ToString<natural>(code));
		}
		natural c = code-opcFirstCode;
		if (c >= stringTable.length()) {
			textOut.blockWrite(StringA((ConstStrA("Undefined")+ToString<natural>(c))),true);
		} else {
			textOut.write('"');
			for (ConstStrA::Iterator iter = stringTable[c].getFwIter(); iter.hasItems();) {
				writeChar(iter.getNext(),textOut);
			}
			textOut.write('"');
		}
	}
	}

}

void BinaryToJson::clearEnumTable() {
	stringTable.clear();
}

void BinaryToJson::addEnum(ConstStrA string) {
	stringTable.add(string);
}

Value BinaryToJson::parseArray(SeqFileInput& input) {
	JSON::Value a = factory->array();
	byte b = input.peek();
	while (b != opcEnd) {
		JSON::Value z = parse(input);
		a->add(z);
		b = input.peek();
	}
	return a;
}

Value BinaryToJson::parseObject(SeqFileInput& input) {
	JSON::Value a = factory->object();
	Utf8ToWideReader<SeqTextInA> w(input);
	wchar_t code = w.getNext();
	AutoArray<char, SmallAlloc<256> > buffer;
	while (code != opcEnd) {
		if (code < opcFirstCode) {
			natural n = readNumber(code-opcString1, input);
			buffer.resize(n);
			input.blockRead(buffer.data(), n, true);
			a->add(buffer, parse(input));
		} else {
			natural c = code - opcFirstCode;
			if (c >= stringTable.length()) {
				buffer.clear();
				buffer.append(ConstStrA("Key")+ToString<natural>(c));
				a->add(buffer, parse(input));
			} else {
				a->add(stringTable[c], parse(input));
			}
		}
		code = w.getNext();
	}
	return a;
}

JSON::Value BinaryToJson::parseFloat32(SeqFileInput& input) {
	float f;
	input.blockRead(&f,sizeof(f),true);
	return factory->newValue(f);
}

JSON::Value BinaryToJson::parseFloat64(SeqFileInput& input) {
	double f;
	input.blockRead(&f,sizeof(f),true);
	return factory->newValue(f);
}

lnatural BinaryToJson::readNumber(natural opcode, SeqFileInput& input) {
	lnatural out = 0;
	byte b;
	switch (opcode) {
	case 0: b = input.getNext(); out = b;break;
	case 1: b = input.getNext(); out = b;
			b = input.getNext(); out += lnatural(b)<<8;
	break;
	case 2: b = input.getNext(); out = b;
			b = input.getNext(); out += lnatural(b)<<8;
			b = input.getNext(); out += lnatural(b)<<16;
			b = input.getNext(); out += lnatural(b)<<24;
	break;
	case 3: b = input.getNext(); out = b;
			b = input.getNext(); out += lnatural(b)<<8;
			b = input.getNext(); out += lnatural(b)<<16;
			b = input.getNext(); out += lnatural(b)<<24;
			b = input.getNext(); out += lnatural(b)<<32;
			b = input.getNext(); out += lnatural(b)<<40;
			b = input.getNext(); out += lnatural(b)<<48;
			b = input.getNext(); out += lnatural(b)<<56;
	break;
	}
	return out;
}

JSON::Value BinaryToJson::parseNumber(natural opcode, bool neg, SeqFileInput& input) {
	lnatural n = readNumber(opcode, input);
	if (neg) {
		linteger out = -(linteger)n;
		return factory->newValue(out);
	} else {
		return factory->newValue(n);
	}
}

JSON::Value BinaryToJson::parseString(natural opcode, SeqFileInput& input) {
	lnatural len = readNumber(opcode, input);
	AutoArray<char, SmallAlloc<256> > buffer;
	buffer.resize(len);
	input.blockRead(buffer.data(),len,true);
	return factory->newValue(buffer);

}

void BinaryToJson::convertArrayToText(SeqFileInput& binIn,SeqTextOutA& textOut) {
	textOut.write('{');
	byte b = binIn.peek();
	while (b != opcEnd) {
		convertToText(binIn,textOut);
		b = binIn.peek();
		if (b != opcEnd) {
			textOut.write(',');
		}
	}
	textOut.write('}');
}



void BinaryToJson::convertObjectToText(SeqFileInput& binIn, SeqTextOutA& textOut) {
	textOut.write('{');
	byte b = binIn.peek();
	while (b != opcEnd) {
		convertToText(binIn,textOut);
		textOut.write(':');
		convertToText(binIn,textOut);
		if (b != opcEnd) {
			textOut.write(',');
		}
	}
	textOut.write('}');
}

void BinaryToJson::convertFloat32ToText(SeqFileInput& binIn,SeqTextOutA& textOut) {
	float f;
	binIn.blockRead(&f,sizeof(f));
	PrintTextA print(textOut);
	print("%1") << f;
}

void BinaryToJson::convertFloat64ToText(SeqFileInput& binIn,SeqTextOutA& textOut) {
	double f;
	binIn.blockRead(&f,sizeof(f));
	PrintTextA print(textOut);
	print("%1") << f;

}

void BinaryToJson::convertStringToText(natural code, SeqFileInput &binIn, SeqTextOutA &textOut) {
	AutoArray<char, SmallAlloc<256> > buffer;
	ConstStrA toOut;
	if (code < opcFirstCode) {
		natural n = readNumber(code,binIn);
		buffer.resize(n);
		binIn.blockRead(buffer.data(),n,true);
		toOut = buffer;
	} else if (code >= stringTable.length()) {
		buffer.append(ConstStrA("undefined"));
		buffer.append(ToString<natural>(code));
		toOut = buffer;
	} else {
		toOut = stringTable[code - opcFirstCode];
	}
	textOut.write('"');
	for (ConstStrA::Iterator iter = toOut.getFwIter(); iter.hasItems();) {
		writeChar(iter.getNext(),textOut);
	}
	textOut.write('"');

}

}
}
