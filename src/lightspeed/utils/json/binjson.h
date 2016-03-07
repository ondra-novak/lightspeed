/*
 * binjson.h
 *
 *  Created on: 5. 3. 2016
 *      Author: ondra
 */

#ifndef LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_BINJSON_H_
#define LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_BINJSON_H_

#include "../../base/containers/map.h"
#include "../../base/memory/smallAlloc.h"
#include "../../base/streams/fileio.h"
#include "../../base/text/textstream.h"
#include "json.h"

namespace LightSpeed {


namespace JSON {


enum OpCode {
	///Null value
	opcNull=0,
	///true value
	opcFalse=1,
	///false value
	opcTrue=2,
	///start of array - follows list of values
	opcArray=3,
	///start of object - follows string,value,string,value
	opcObject=4,
	///end of container
	opcEnd=5,
	///pushes value to the stack, follows the value
	opcPush=6,
	///pop value from the stack,
	opcPop=7,
	///string, follows 1 byte as size of string
	opcString1=8,
	///string, follows 2 bytes as size of string
	opcString2=9,
	///string, follows 4 bytes as size of string
	opcString4=10,
	///string, follows 8 bytes as size of string
	opcString8=11,
	///integer, follows 1 byte for positive number
	opcPosInt1=12,
	///integer, follows 2 bytes for positive number
	opcPosInt2=13,
	///integer, follows 4 bytes for positive number
	opcPosInt4=14,
	///integer, follows 8 bytes for positive number
	opcPosInt8=15,
	///integer, follows 1 byte for negative number
	opcNegInt1=16,
	///integer, follows 2 bytes for negative number
	opcNegInt2=17,
	///integer, follows 4 bytes for negative number
	opcNegInt4=18,
	///integer, follows 8 bytes for negative number
	opcNegInt8=19,
	///Number zero (0_
	opcZero=20,
	///Empty String
	opcEmptyString=21,
	///Float number (32 bit)
	opcFloat32=22,
	///Double number (64 bit)
	opcFloat64=23,
	///Binary, follows 1 byte as size
	opcBinary1=24,
	///Binary, follows 2 bytes for negative number
	opcBinary2=25,
	///Binary, follows 4 bytes for negative number
	opcBinary4=26,
	///Binary, follows 8 bytes for negative number
	opcBinary8=27,
	///Pick nth item from the stack, use 1 byte to store n
	opcPick1= 28,
	///Pick nth item from the stack, use 2 bytes to store n
	opcPick2 = 29,
	///Pick nth item from the stack, use 4 bytes to store n
	opcPick4 = 30,
	///Remove n-items from the stack, use 1 byte to store n
	opcRemove1 = 31,

	opcFirstCode=40



};


class JsonToBinary {
public:

	JsonToBinary();

	void serialize(const JSON::Value &val, SeqFileOutput &output);


	void clearEnumTable();
	void addEnum(ConstStrA string);

	template<typename Container>
	void addEnumTable(const Container &containter);

protected:

	void serializeValue(const JSON::INode& val, SeqFileOutput& output);
	void serializeArray(const JSON::INode& val, SeqFileOutput& output);
	void serializeObject(const JSON::INode& val, SeqFileOutput& output);
	void serializeFloat(const JSON::INode& val, SeqFileOutput& output);
	void serializeInteger(const JSON::INode& val, SeqFileOutput& output);
	void serializeBool(const JSON::INode& val, SeqFileOutput& output);
	void serializeNull(const JSON::INode& val, SeqFileOutput& output);
	void serializeString(const JSON::INode& val, SeqFileOutput& output);

	void writeString(ConstStrA string, SeqFileOutput& output);
	void writeNumber(OpCode op, lnatural v, SeqFileOutput& output);

	typedef Map<ConstStrA, natural> StringMap;

	StringMap strMap;

	static const natural firstCode=opcFirstCode;
	natural nxtCode;

};


class BinaryToJson {
public:

	BinaryToJson();

	JSON::Value parse(SeqFileInput &input);

	void convertToText(SeqFileInput &binIn, SeqTextOutA &textOut);

	void clearEnumTable();
	void addEnum(ConstStrA string);

	template<typename Container>
	void addEnumTable(const Container &containter);



public:

	JSON::PFactory factory;
	AutoArray<ConstStrA, SmallAlloc<256> > stringTable;
	AutoArray<JSON::Value, SmallAlloc<256> > stack;

	JSON::Value parseArray(SeqFileInput &input);
	JSON::Value parseObject(SeqFileInput &input);
	JSON::Value parseFloat32(SeqFileInput &input);
	JSON::Value parseFloat64(SeqFileInput &input);
	lnatural readNumber(natural opcode, SeqFileInput &input);
	JSON::Value parseNumber(natural opcode, bool neg, SeqFileInput &input);
	JSON::Value parseString(natural opcode, SeqFileInput &input);

	void convertArrayToText(SeqFileInput &binIn, SeqTextOutA &textOut);
	void convertObjectToText(SeqFileInput &binIn, SeqTextOutA &textOut);
	void convertFloat32ToText(SeqFileInput &binIn, SeqTextOutA &textOut);
	void convertFloat64ToText(SeqFileInput &binIn, SeqTextOutA &textOut);
	void convertStringToText(SeqFileInput &binIn, SeqTextOutA &textOut);

};



template<typename Container>
inline void JsonToBinary::addEnumTable(const Container& container) {

	for (typename Container::Iterator iter = container.getFwIter(); iter.hasItems();) {
		addEnum(iter.getNext());
	}
}

template<typename Container>
inline void BinaryToJson::addEnumTable(
		const Container& container) {
	for (typename Container::Iterator iter = container.getFwIter(); iter.hasItems();) {
		addEnum(iter.getNext());
	}
}

}




}


#endif /* LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_BINJSON_H_ */
