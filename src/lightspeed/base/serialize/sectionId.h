/*
 * sectionName.h
 *
 *  Created on: 22.11.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_SERIALIZE_SECTIONID_SECTIONNAME_H_
#define LIGHTSPEED_SERIALIZE_SECTIONID_SECTIONNAME_H_

#pragma once

#include "../invokable.h"
#include "../containers/constStr.h"

namespace LightSpeed {


///Section identifier - used to identify sections
template<typename Impl>
class SectionId:public ComparableLessAndEqual<Impl> {
public:

	///Assigns new value to the section
	/** Used operator << because equal sign is reserved. Use while writing parsers */
	SectionId<Impl> &operator << (const SectionId<Impl> &other) {
		this->_invoke() = other._invoke();
		return *this;
	}

	///Every section must be able to represent itself as string
	/** This is need for debuging */
	ConstStrA toString() const {return this->_invoke().toString();}
};


///General string sectoon
/** All sections represented as single string */
template<typename Impl>
class SectionName: public SectionId<Impl> {
public:

	///Retrieves name of section
	ConstStrA getName() const {return this->_invoke().getName();}
	///Retrieves section as string - equal to name
	ConstStrA toString() const {return getName();}
	///Conversion to ConstStrA
	operator ConstStrA() const {return getName();}

	template<typename X>
	bool equalTo(const SectionName<X> &other) const {
		return getName() == other.getName();
	}
	template<typename X>
	bool lessThan(const SectionName<X> &other) const {
		return getName() < other.getName();
	}
	bool isNil() const {
		return getName().empty();
	}

};

///Class used to convert old style sections reprsented as strings
/**
 * It requires, that string is statically allocated or it is string constant
 * It must be valid during serialization. You cannot use dynamically created names
 *
 *
 * Old style sections has been always const char * so this section works similar to old convetion
 */
class StdSectionName: public SectionName<StdSectionName> {
public:

	StdSectionName() {}
	StdSectionName(ConstStrA name):name(name) {}
	StdSectionName(const char *name):name(name) {}
	ConstStrA getName() const {return name;}

protected:
	ConstStrA name;
};

}  // namespace LightSpeed

#endif /* LIGHTSPEED_SERIALIZE_SECTIONID_SECTIONNAME_H_ */
