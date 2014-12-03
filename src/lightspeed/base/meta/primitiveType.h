#ifndef LIGHTSPEED_META_PRIMITIVETYPE_H_
#define LIGHTSPEED_META_PRIMITIVETYPE_H_

#include "metaConst.h"
#include "../types.h"
#include "truefalse.h"

namespace LightSpeed {

	template<class T> struct IsPrimitiveType: public MFalse {};
	template<> struct IsPrimitiveType<bool>: public MTrue {};
	template<> struct IsPrimitiveType<char>: public MTrue {};
	template<> struct IsPrimitiveType<unsigned char>: public MTrue {};
	template<> struct IsPrimitiveType<signed char>: public MTrue {};
	template<> struct IsPrimitiveType<unsigned short>: public MTrue {};
	template<> struct IsPrimitiveType<signed short>: public MTrue {};
	template<> struct IsPrimitiveType<unsigned int>: public MTrue {};
	template<> struct IsPrimitiveType<signed int>: public MTrue {};
	template<> struct IsPrimitiveType<unsigned long>: public MTrue {};
	template<> struct IsPrimitiveType<signed long>: public MTrue {};
	template<> struct IsPrimitiveType<unsigned long long>: public MTrue {};
	template<> struct IsPrimitiveType<signed long long>: public MTrue {};
	template<> struct IsPrimitiveType<float>: public MTrue {};
	template<> struct IsPrimitiveType<double>: public MTrue {};
	template<> struct IsPrimitiveType<long double>: public MTrue {};
	template<> struct IsPrimitiveType<wchar_t>: public MTrue {};
	template<class T> struct IsPrimitiveType<T *>: public MTrue {};
	template<class T> struct IsPrimitiveType<T &>: public MTrue {};
	template<> struct IsPrimitiveType<NullType>: public MTrue {};
	
	
}

#endif /*PRIMITIVETYPE_H_*/
