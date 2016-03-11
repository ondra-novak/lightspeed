/*
 * jsonserializer.tcc
 *
 *  Created on: 11. 3. 2016
 *      Author: ondra
 */

#ifndef LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_JSONSERIALIZER_TCC_
#define LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_JSONSERIALIZER_TCC_

#include "jsonserializer.h"
#include "../../base/interface.tcc"
#include "../../base/text/textParser.tcc"
#include "../../base/text/textstream.tcc"
#include "../../base/containers/autoArray.tcc"


namespace LightSpeed {

namespace JSON {


template<typename T>
Serializer<T>::Serializer(IWriteIterator<char, T> &iter, bool escapeUTF8)
	:iter(iter),escapeUTF8(escapeUTF8) {}


template<typename T>
void Serializer<T>::serializeArray(const INode *json) {
	iter.write('[');
	Iterator oiter = json->getFwIter();
	if (oiter.hasItems()) {
		const NodeInfo &n = oiter.getNext();
		serialize(n.node);
		while (oiter.hasItems()) {
			iter.write(',');
			const NodeInfo &n = oiter.getNext();
			serialize(n.node);
		}
	}
	iter.write(']');
}

template<typename T>
void Serializer<T>::serializeBool(const INode *json) {
	if (json->getBool()) {
		iter.blockWrite(ConstStrA(strTrue));
	} else {
		iter.blockWrite(ConstStrA(strFalse));
	}
}

template<typename T>
void Serializer<T>::serializeDeleted(const INode *) {
		iter.blockWrite(ConstStrA(strDelete));
}

template<typename T>
void Serializer<T>::serializeNull(const INode *) {
		iter.blockWrite(ConstStrA(strNull));
}


template<typename T>
void Serializer<T>::serializeString(ConstStrA str) {

	iter.write('"');
	for (ConstStrA::Iterator oiter = str.getFwIter(); oiter.hasItems();) {
		writeChar<char>(oiter.getNext(),iter);
	}
	iter.write('"');

}

template<typename T>
void Serializer<T>::serializeString(ConstStrW str) {
	iter.write('"');
	for (ConstStrW::Iterator oiter = str.getFwIter(); oiter.hasItems();) {
		wchar_t c = oiter.getNext();
		if (c > 0x7F) {
			TextOut<IWriteIterator<char, T> &, StaticAlloc<32> > fmt(iter);
			fmt.setBase(16);
			fmt("\\u%{04}1") << (unsigned int)c;
		} else {
			writeChar<char>((char)c,iter);
		}
	}
	iter.write('"');
}

template<typename T>
void Serializer<T>::serializeStringEscUTF(ConstStrA str) {
	iter.write('"');
	for (ConstStrA::Iterator oiter = str.getFwIter(); oiter.hasItems();) {
		signed char c = oiter.getNext();
		if (c < 0) {
			Utf8ToWideFilter flt;
			flt.input(c);
			while (!flt.hasItems()) flt.input(oiter.getNext());
			flt.flush();
			while (flt.hasItems()) {
				wchar_t cu = flt.output();
				TextOut<IWriteIterator<char, T> &, StaticAlloc<32> > fmt(iter);
				fmt.setBase(16);
				fmt("\\u%{04}1") << (unsigned int)cu;
			}
		} else {
			writeChar<char>(c,iter);
		}
	}
	iter.write('"');
}

template<typename T>
void Serializer<T>::serializeObjectNode(const NodeInfo &n) {
	if (escapeUTF8)
		serializeStringEscUTF(n.key->getString());
	else
		serializeString(n.key->getString());

	iter.write(':');
	serialize(n.node);
}

template<typename T>
void Serializer<T>::serializeObject(const INode *json) {
	iter.write('{');
	Iterator oiter = json->getFwIter();
	if (oiter.hasItems()) {
		const NodeInfo &n = oiter.getNext();
		serializeObjectNode(n);
		while (oiter.hasItems()) {
			iter.write(',');
			const NodeInfo &n = oiter.getNext();
			serializeObjectNode(n);
		}
	}
	iter.write('}');
}

template<typename T>
void Serializer<T>::serializeString(const INode *json) {

	if (json->isUtf8()) {
		if (escapeUTF8)
			serializeStringEscUTF(json->getStringUtf8());
		else
			serializeString(json->getStringUtf8());
	}else
		serializeString(json->getString());
}

template<typename T>
void Serializer<T>::serializeFloat(const INode *json) {

	TextOut<IWriteIterator<char, T> &, StaticAlloc<140> > fmt(iter);
	fmt() << json->getFloat();
}


template<typename T>
void Serializer<T>::serializeInt(const INode *json) {

	TextOut<IWriteIterator<char, T> &, StaticAlloc<140> > fmt(iter);
	fmt() << json->getInt();
}

template<typename T>
void Serializer<T>::serializeCustom(const INode *json) {
	const ICustomNode &nd = json->getIfc<ICustomNode>();
	VtWriteIterator<IWriteIterator<char, T> &> witer(iter);
	nd.serialize(witer,escapeUTF8);
}

template<typename T>
void Serializer<T>::serialize(const INode *json) {
	if (json == 0) throwNullPointerException(THISLOCATION);

	CycleDetector d(json,top);
	if (d.cycleDetected) {
		if (json->isObject()) {
			iter.blockWrite(ConstStrA("{\"error\":\"<infinite_recursion>\"}"),true);
		} else if (json->isArray()) {
			iter.blockWrite(ConstStrA("[\"<infinite_recursion>\"]"),true);
		} else {
			iter.blockWrite(ConstStrA("<infinite_recursion>"),true);;
		}
	} else {
		top = &d;
		NodeType t = json->getType();
		switch (t) {
		case ndArray: serializeArray(json); break;
		case ndBool: serializeBool(json); break;
		case ndObject: serializeObject(json);break;
		case ndDelete: serializeDeleted(json);break;
		case ndFloat: serializeFloat(json);break;
		case ndInt: serializeInt(json);break;
		case ndNull: serializeNull(json);break;
		case ndString: serializeString(json);break;
		default: serializeCustom(json);break;
		}
		top = d.top;
	}

}

template<typename T>
template<typename X>
void Serializer<T>::writeChar(X c, IWriteIterator<char, T> & iter) {
	switch (c) {
	case '\r': iter.write('\\');iter.write('r');break;
	case '\f': iter.write('\\');iter.write('f');break;
	case '\n': iter.write('\\');iter.write('n');break;
	case '\t': iter.write('\\');iter.write('t');break;
	case '\b': iter.write('\\');iter.write('b');break;
	case '\\': iter.write('\\');iter.write('\\');break;
	case '"': iter.write('\\');iter.write('\"');break;
	default:
		if (c>=0 && c < 32) {
			TextOut<IWriteIterator<char, T> &, StaticAlloc<32> > fmt(iter);
			fmt.setBase(16);
			fmt("\\u%{04}1") << (unsigned int)c;
		} else {
			iter.write(c);
		}
	}
}






}}



#endif /* LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_JSONSERIALIZER_TCC_ */
