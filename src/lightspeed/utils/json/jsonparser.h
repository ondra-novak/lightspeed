/*
 * jsonparser.h
 *
 *  Created on: 11. 3. 2016
 *      Author: ondra
 */

#ifndef LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_JSONPARSER_H_
#define LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_JSONPARSER_H_

namespace LightSpeed {
namespace JSON {

///Generic JSON parser
/** It accepts any character iterator and returns JSON::Value
 *
 *
 * @tparam type of iterator. Must inherit Iterator<char, T>
 *
 * parser performs easyest way to parser JSON using recursive calls
 * while object or array is detected. It reads iterator directly without
 * buffering and complex analysis
 */
template<typename T>
class Parser {
public:


	///Construct parser
	/**
	 * @param iter iterator which will be read
	 * @param factory factory that will be used to construct objects
	 */
	Parser(IIterator<char, T> &iter, PFactory factory)
			:factory(factory),iter(iter),storedChar(0) {}


	///Parse JSON
	virtual Value parse();
protected:

	///Allows to override own version of parseString
	virtual Value parseString();
	///Allows to override by own verison of parseString
	virtual Value parseObject(INode *container);
	virtual Value parseArray(INode *container);
	virtual Value parseValue(char firstChar);
	void parseCheck(const char *str);

protected:
	AutoArray<char> strBuff;
	PFactory factory;
	IIterator<char, T> &iter;
	char storedChar;

	void parseRawString();
	char getNextEatW();
	char getNextWithStored();
	ConstStrA arrayIndexStr(natural i);

	char getNextChr();

	JSON::Value sharedNull;
	JSON::Value sharedTrue;
	JSON::Value sharedFalse;
	JSON::Value sharedEmptyStr;
	JSON::Value sharedZero;
};

///Stream parser allows you to inspect and use already parsed values when whole JSON is not yet complete
/** You can define several callbacks that will be called, when particular
 *  value has been parsed. This allows you to process for example an items of a large array
 *  read from the stream, during the data are slowly arriving the application.
 */
template<typename T>
class StreamParser : public Parser<T> {
public:

	///Callback interface
	class ICallback {
	public:
		///Called on begin of an object or an array.
		/**
		 * Everytime object or array is opened, this function is called. The function
		 * should install new callback handler, which will be used to
		 * monitor items creating at this level. Function can return self,or
		 * create new object for that, or return NULL. When NULL is returned, monitoring
		 * is disable for all deeper levels
		 *
		 * @param container container opened. It is always empty, but you can test its type
		 * @param parentContainer parent container. It is set to NULL for root container
		 * @param name contains name of the value in the parent object. Valid only for object,
		 * 			it is empty for arrays
		 * @return pointer to new callback object. It can be self, a newly created instance or NULL.
		 *
		 */
		virtual ICallback *onBeginContainer(JSON::INode *container, JSON::INode *parentContainer, ConstStrA name) = 0;
		///Called when container is completed - i.e no more values will be added in it
		/**
		 *
		 * @param container container being closed
		 * @param parentContainer parent container
		 *
		 * @note function is called through the instance of ICallback created for
		 * this container, not parent container. For example. If you have a callback instance
		 * for parent container named as l1, and for inner container as l2, the function
		 * onBeginContainer is callled with l1, however, the function onContainerCompleted
		 * is called with l2.
		 */
		virtual void onContainerCompleted(JSON::INode *container, JSON::INode *parentContainer) = 0;
		///Called when new value is added to the current container
		/**
		 *
		 * @param value new value
		 * @param container container where value will be put
		 * @return You should return value. However, you can change it and store
		 * complete different value than has been parsed. Don't return NULL. You
		 * cannot remove this value, if you want to mark value deleted,
		 * return JSON's null
		 */
		virtual JSON::Value onValue(JSON::INode *value, JSON::INode *container) = 0;
	};

	StreamParser(IIterator<char, T> &iter, PFactory factory):Parser<T>(iter, factory) {}

	Value parse(ICallback *cb);
protected:

	ICallback *curCB;
	JSON::INode *prevContainer;

	virtual Value parseObject(INode *container);
	virtual Value parseArray(INode *container);
	virtual Value parse();


};

template<typename T>
Value parse(IIterator<char, T> &iter, const PFactory &factory) {
	Parser<T> s(iter,factory);
	return s.parse();
}
template<typename T>
Value parse(const IIterator<char, T> &iter, const PFactory &factory) {
	T copy(static_cast<const T &>(iter));
	Parser<T> s(copy,factory);
	return s.parse();
}


}
}


#endif /* LIBS_LIGHTSPEED_SRC_LIGHTSPEED_UTILS_JSON_JSONPARSER_H_ */
