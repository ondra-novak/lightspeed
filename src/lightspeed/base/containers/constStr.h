/*
 * constStr.h
 *
 *  Created on: 18.8.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINER_CONSTSTR_H_
#define LIGHTSPEED_CONTAINER_CONSTSTR_H_

#include "arrayref.h"

namespace LightSpeed {

    ///Declaration of class handles basic operations with ASCIIZ

    template<class T>
    class StringBase {
    public:

        static const bool needZeroChar = false;

        static bool isZeroChar(const T &) {return false;}
        template<class X>
        static void writeZeroChar(IWriteIterator<T,X> &) {}
        static natural stringLen(const T *str);
		static natural stringLen(natural originLen) {return originLen;}
		static natural originalLen(natural stringLen) {return stringLen;}
		static const T *getEmptyString() {return 0;}
		static natural removeZeroChar(const T *, natural length) {return length;}


    };



    namespace _intr {
        template<class T>
        class StringBaseCharAndWide {
        public:
            static const bool needZeroChar = true;

            static bool isZeroChar(const T &itm) {return itm == 0;}
            template<class X>
            static void writeZeroChar(IWriteIterator<T,X> &iter) {
                iter.write(T(0));
            }
            static natural stringLen(const T *str) {
            	if (str == 0) return 0;
                natural sz = 0;
                while (str[sz] != 0) sz++;
                return sz;
            }
			static natural stringLen(natural originLen) {return originLen+1;}
			static natural originalLen(natural stringLen) {return stringLen-1;}
			static const T *getEmptyString();
			static natural removeZeroChar(const T *str, natural length) {return str[length-1]==0?length-1:length;}
        };
    }

    ///Specialization of StringBase for char
    template<> class StringBase<char>: public _intr::StringBaseCharAndWide<char> {};
    ///Specialization of StringBase for wchar_t
    template<> class StringBase<wchar_t>: public _intr::StringBaseCharAndWide<wchar_t> {};

    ///Specialization of StringBase for char
    template<> class StringBase<char const>: public _intr::StringBaseCharAndWide<char const> {};
    ///Specialization of StringBase for wchar_t
    template<> class StringBase<wchar_t const>: public _intr::StringBaseCharAndWide<wchar_t const> {};

    ///Constant string
    /** Useful to fast create GenericArray like container from constant string
     * If used with immediately value, it can be used anytime 
     * If used with const char ptr variable, pointer should be valid
     * until instance is destroyed
     */
    template<class T>
    class ConstStringT: public ArrayRef<const T>
						 {
    public:
        typedef ArrayRef<const T> Super;
        typedef StringBase<const T> StrFn;
        ConstStringT():Super() {}
        ConstStringT(NullType):Super() {}
        ConstStringT(const T *asciiZ):Super(asciiZ,StrFn::stringLen(asciiZ)) {}
        ConstStringT(const T *asciiZ, natural count):Super(asciiZ,count) {}
		template<typename Tr>
		ConstStringT(const FlatArray<typename ConstObject<T>::Remove,Tr> &arr):Super(arr) {}
		template<typename Tr>
		ConstStringT(const FlatArray<typename ConstObject<T>::Add,Tr> &arr):Super(arr) {}
		ConstStringT(const T &obj):Super(&obj,1) {}


		using Super::operator==;
		using Super::operator!=;
		using Super::operator>;
		using Super::operator<;
		using Super::operator>=;
		using Super::operator<=;


		bool operator==(NullType) const {return this->data() == 0;}
		bool operator!=(NullType) const {return this->data() != 0;}

		///Maps const string to the another object
		/** This is useful when it is need to create copy of string which refering to the another source
		 * @param arr another source
		 * @return copy of string refering to the selected source
		 *
		 * */
		template<typename Tr>
		ConstStringT map( const FlatArray<typename ConstObject<T>::Remove,Tr> &arr) {
			natural k = arr.find(*this);
			if (k == naturalNull) return ConstStringT();
			else return arr.mid(k,this->length());
		}
		///Maps const string to the another object
		/** This is useful when it is need to create copy of string which refering to the another source
		 * @param arr another source
		 * @return copy of string refering to the selected source
		 * */
		template<typename Tr>
		ConstStringT map(const FlatArray<typename ConstObject<T>::Add,Tr> &arr) {
			natural k = arr.find(*this);
			if (k == naturalNull) return ConstStringT();
			else return arr.mid(k,this->length());
		}
    };

    typedef ConstStringT<char> ConstStrC;
	typedef ConstStringT<char> ConstStrA;
    typedef ConstStringT<wchar_t> ConstStrW;
	class ConstBin: public ConstStringT<byte> {
	public:
		typedef  ConstStringT<byte> Super;
		ConstBin():Super() {}
		ConstBin(NullType):Super() {}
		ConstBin(const byte *data, natural count):Super(data,count) {}
		template<typename Tr>
		ConstBin(const FlatArray<typename ConstObject<byte>::Remove,Tr> &arr):Super(arr) {}
		template<typename Tr>
		ConstBin(const FlatArray<typename ConstObject<byte>::Add,Tr> &arr):Super(arr) {}
		ConstBin(const byte &obj):Super(&obj,1) {}

		template<typename T>
		explicit ConstBin(const T *pod):Super(reinterpret_cast<const byte *>(pod),sizeof(T)) {}

		ConstBin(const void *ptr, natural len):Super(reinterpret_cast<const byte *>(ptr),len) {}

	};




	template<typename T, int n>
	ConstStringT<T> __arrayToString(const T (&arr)[n]) {
		if (n > 0 && StringBase<T>::needZeroChar 
			&& StringBase<T>::isZeroChar(arr[n-1])) {
				return ConstStringT<T>(arr,n-1);
		} else {
			return ConstStringT<T>(arr,n);
		}
	}


	template<typename T>
	struct StrCmpCS {
		CompareResult operator()(const ConstStringT<T> &a, const ConstStringT<T> &b) const;
	};

	template<typename T>
	struct StrCmpCI;

	template<>
	struct StrCmpCI<char> {
		CompareResult operator()(const ConstStringT<char> &a, const ConstStringT<char> &b) const;
	};

	template<>
	struct StrCmpCI<wchar_t> {
		CompareResult operator()(const ConstStringT<wchar_t> &a, const ConstStringT<wchar_t> &b) const;
	};

	template<typename T, int n>
	inline ConstStringT<T> makeCStr(const T (&str)[n]) {
		return ConstStringT<T>(str,StringBase<T>::removeZeroChar(str,n));
	}

	///Creates ConstStrA from c-like string stored in an char array
	/**
	 * Function just only tries to find terminating zero. If zero found, function returns ConstStrA contain
	 * string up to zero found . If zero is not found, function returns whole array
	 * @param str array for convert to ConstStrA
	 * @return result converted
	 */
	template<int n>
	inline ConstStrA ConstStrFromCStr(const char (&str)[n]) {
		natural k = 0;
		while (k < n && str[k]) k++;
		return ConstStrA(str,k);
	}

	///Creates ConstStrW from c-like string stored in an wchar_t array
	/**
	 * Function just only tries to find terminating zero. If zero found, function returns ConstStrW contain
	 * string up to zero found . If zero is not found, function returns whole array
	 * @param str array for convert to ConstStrW
	 * @return result converted
	 */
	template<int n>
	inline ConstStrW ConstStrFromCStr(const wchar_t (&str)[n]) {
		natural k = 0;
		while (k < n && str[k]) k++;
		return ConstStrW(str,k);
	}

	template<typename T, int n>
	inline bool operator==(const ConstStringT<T> &c, const T (&str)[n]) {return c == makeCStr(str); }
	template<typename T, int n>
	inline bool operator!=(const ConstStringT<T> &c, const T (&str)[n]) {return c != makeCStr(str); }
	template<typename T, int n>
	inline bool operator>(const ConstStringT<T> &c, const T (&str)[n]) {return c > makeCStr(str); }
	template<typename T, int n>
	inline bool operator<(const ConstStringT<T> &c, const T (&str)[n]) {return c < makeCStr(str); }
	template<typename T, int n>
	inline bool operator>=(const ConstStringT<T> &c, const T (&str)[n]) {return c >= makeCStr(str); }
	template<typename T, int n>
	inline bool operator<=(const ConstStringT<T> &c, const T (&str)[n]) {return c >= makeCStr(str); }
	template<typename T, int n>
	inline bool operator>>(const ConstStringT<T> &c, const T (&str)[n]) {return StrCmpCI<T>()(c,makeCStr(str)) == cmpResultGreater; }
	template<typename T, int n>
	inline bool operator<<(const ConstStringT<T> &c, const T (&str)[n]) {return StrCmpCI<T>()(c,makeCStr(str)) == cmpResultLess; }
	template<typename T, int n>
	inline bool operator>>=(const ConstStringT<T> &c, const T (&str)[n]) {return StrCmpCI<T>()(c,makeCStr(str)) != cmpResultLess; }
	template<typename T, int n>
	inline bool operator<<=(const ConstStringT<T> &c, const T (&str)[n]) {return StrCmpCI<T>()(c,makeCStr(str)) != cmpResultGreater; }
	template<typename T, int n>
	inline bool operator^=(const ConstStringT<T> &c, const T (&str)[n]) {return StrCmpCI<T>()(c,makeCStr(str)) == cmpResultEqual; }
	template<typename T, int n>
	inline bool operator/=(const ConstStringT<T> &c, const T (&str)[n]) {CompareResult r = StrCmpCI<T>()(c,makeCStr(str));
																return r != cmpResultEqual || r == cmpResultNotEqual; 
																}

}
#endif /* CONSTSTR_H_ */
