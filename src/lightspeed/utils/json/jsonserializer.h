/*
 * jsonserializer.h
 *
 *  Created on: 11. 3. 2016
 *      Author: ondra
 */

#ifndef LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_JSONSERIALIZER_H_
#define LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_JSONSERIALIZER_H_
#include "json.h"


namespace LightSpeed {

namespace JSON {


template<typename T>
class Serializer {

	struct CycleDetector {
		const JSON::INode *object;
		CycleDetector *top;

		CycleDetector(const JSON::INode *object, CycleDetector *top)
			:object(object),top(top),cycleDetected(checkCycle(object,top)) {
		}

		const bool cycleDetected;
		static bool checkCycle(const JSON::INode *object, CycleDetector *top) {
			CycleDetector *x = top;
			while (x != 0) {
				if (object == x->object) return true;
				x = x->top;
			}
			return false;
		}
	};

public:
	Serializer(IWriteIterator<char, T> &iter, bool escapeUTF8);

	void serialize(const INode *json);

public:
	template<typename X>
	static void writeChar(X c, IWriteIterator<char, T> & iter);

	void serializeString(ConstStrA str);
	void serializeString(ConstStrW str);
	void serializeStringEscUTF(ConstStrA str);



protected:
	void serializeArray(const INode *json);
	void serializeBool(const INode *json);
	void serializeDeleted(const INode *);
	void serializeNull(const INode *);

	void serializeObjectNode(const ConstKeyValue &n);
	void serializeObject(const INode *json);
	void serializeString(const INode *json);
	void serializeFloat(const INode *json);
	void serializeInt(const INode *json);
	void serializeCustom(const INode *json);

	IWriteIterator<char, T> &iter;
	bool escapeUTF8;
	Pointer<CycleDetector> top;
};


template<typename T>
void serialize(const INode *json, IWriteIterator<char, T> &iter, bool escapeUTF8) {
	Serializer<T> s(iter,escapeUTF8);
	s.serialize(json);
}

template<typename T>
void serialize(const INode *json, IWriteIterator<char, T> &iter, bool escapeUTF8);

template<typename T, typename X>
static void writeChar(X c, IWriteIterator<char, T> & iter) {
	Serializer<T>::template writeChar<X>(c,iter);
}




}
}


#endif /* LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_JSONSERIALIZER_H_ */
