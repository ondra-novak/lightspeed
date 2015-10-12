/*
 * namedEnums.h
 *
 *  Created on: 27. 2. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_NAMEDENUMS_H_
#define LIGHTSPEED_BASE_NAMEDENUMS_H_
#include "containers/autoArray.h"
#include "containers/string.h"
#include "memory/smallAlloc.h"
#include "containers/sort.h"
#include "exceptions/exception.h"

namespace LightSpeed {

namespace _intr {
	template<typename T>
	struct NamedEnumFieldOrder {
		T value;
		natural order;

		NamedEnumFieldOrder(const T &v):value(v) {}
	};
	template<>
	struct NamedEnumFieldOrder<ConstStrA> {
		ConstStrA value;
		natural order;

		NamedEnumFieldOrder(const ConstStrA &v):value(v) {}
		NamedEnumFieldOrder(const char *v):value(v) {}
	};
}


template<typename Enum>
struct NamedEnumDef {
	///enum specification
	_intr::NamedEnumFieldOrder<Enum> enm;
	///associated name
	_intr::NamedEnumFieldOrder<ConstStrA> name;
};


template<typename Enum>
class NamedEnum {
public:

	typedef NamedEnumDef<Enum> Def;

	template<int n>
	NamedEnum(Def (&arr)[n]);

	Enum operator[](ConstStrA name);
	ConstStrA operator[](Enum enm);
	bool find(ConstStrA name, Enum &result);

	const natural count;
	const NamedEnumDef<Enum> * const items;

	StringA toString(ConstStrA separator = ConstStrA(",")) const;

protected:

	class ContByName {
	public:
		typedef ConstStrA ItemT;
		ContByName(Def *def,natural count):def(def),count(count) {
			for (natural i = 0; i < count; i++) def[i].name.order = i;
		}
		ConstStrA operator[](natural p) const {return def[def[p].name.order].name.value;}
		natural &operator()(natural p) {return def[p].name.order;}
		natural length() const {return count;}

	protected:
		Def *def;
		natural count;
	};

	class ContByEnum {
	public:
		typedef Enum ItemT;
		ContByEnum(Def *def,natural count):def(def),count(count) {
			for (natural i = 0; i < count; i++) def[i].enm.order= i;
		}
		Enum operator[](natural p) const {return def[def[p].enm.order].enm.value;}
		natural &operator()(natural p) {return def[p].enm.order;}
		natural length() const {return count;}

	protected:
		Def *def;
		natural count;
	};

	template<int n>
	static void orderByName(NamedEnumDef<Enum> (&arr)[n]) {
		ContByName nm(arr,n);
		HeapSort<ContByName> hs(nm);
		hs.sort();
	}

	template<int n>
	static void orderByEnum(NamedEnumDef<Enum> (&arr)[n]) {
		ContByEnum nm(arr,n);
		HeapSort<ContByEnum> hs(nm);
		hs.sort();
	}

};

class UnknownEnumName: public LightSpeed::Exception {
public:
	LIGHTSPEED_EXCEPTIONFINAL;

	template<typename Enum>
	UnknownEnumName(const ProgramLocation &loc, ConstStrA name, const NamedEnum<Enum> * const items);
	virtual ~UnknownEnumName() throw () {}

	ConstStrA getName() const;
	ConstStrA getSet() const;

protected:
	StringA name;
	StringA set;

	void message(ExceptionMsg &msg) const;

};


template<typename Enum>
template<int n>
inline NamedEnum<Enum>::NamedEnum(Def (&arr)[n]):count(n),items(arr)
{
	orderByName(arr);
	orderByEnum(arr);
}

template<typename Enum>
inline Enum LightSpeed::NamedEnum<Enum>::operator [](ConstStrA name) {
	Enum r;
	if (find(name, r)) return r;
	else throw UnknownEnumName(THISLOCATION,name,this);
}

template<typename Enum>
inline bool LightSpeed::NamedEnum<Enum>::find(ConstStrA name, Enum &res) {
	natural h = 0;
	natural t = count;
	while (h < t) {
		natural c = (h + t) / 2;
		natural i = items[c].name.order;
		CompareResult cr = name.compare(items[i].name.value);
		if (cr == cmpResultLess) t = c;
		else if (cr == cmpResultGreater) h = c + 1;
		else {
			res = items[i].enm.value;
			return true;
		}
	}
	return false;
}


template<typename Enum>
inline ConstStrA NamedEnum<Enum>::operator [](Enum enm) {
	natural h = 0;
	natural t = count;
	while (h < t) {
		natural c= (h + t)/2;
		natural i = items[c].enm.order;
		Enum chl = items[i].enm.value;
		if (enm < chl) t = c;
		else if (enm > chl) h = c+1;
		else return items[i].name.value;
	}
	return ConstStrA();

}



template<typename Enum>
StringA NamedEnum<Enum>::toString(ConstStrA separator)const {
	AutoArray<char, SmallAlloc<256> > setValues;
	if (count) {
		setValues.append(items[0].name.value);
		for (natural i = 1; i < count;i++) {
			setValues.append(separator);
			setValues.append(items[i].name.value);
		}
	}
	return setValues;
}

template<typename Enum>
inline UnknownEnumName::UnknownEnumName(const ProgramLocation& loc,
		ConstStrA name, const NamedEnum<Enum>* const items)
	:Exception(loc),name(name),set(items->toString())
{


}




}


#endif /* LIGHTSPEED_BASE_NAMEDENUMS_H_ */
