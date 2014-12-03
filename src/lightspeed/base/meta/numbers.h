#ifndef LIGHTSPEED_META_NUMBERS_H_
#define LIGHTSPEED_META_NUMBERS_H_

namespace LightSpeed {

	template<class T> struct IsIntegerNumber: public MFalse {};
	template<> struct IsIntegerNumber<char>: public MTrue {};
	template<> struct IsIntegerNumber<unsigned char>: public MTrue {};
	template<> struct IsIntegerNumber<short>: public MTrue {};
	template<> struct IsIntegerNumber<unsigned short>: public MTrue {};
	template<> struct IsIntegerNumber<int>: public MTrue {};
	template<> struct IsIntegerNumber<unsigned int>: public MTrue {};
	template<> struct IsIntegerNumber<long>: public MTrue {};
	template<> struct IsIntegerNumber<unsigned long>: public MTrue {};
	template<> struct IsIntegerNumber<long long>: public MTrue {};
	template<> struct IsIntegerNumber<unsigned long long>: public MTrue {};
	
	template<class T> struct IsRealNumber: public MFalse {};
	template<> struct IsRealNumber<float>: public MTrue {};
	template<> struct IsRealNumber<double>: public MTrue {};


}

#endif /*NUMBERS_H_*/
