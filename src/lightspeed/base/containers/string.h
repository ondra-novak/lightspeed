#include "../exceptions/exception.h"

#ifndef LIGHTSPEED_CONTAINERS_STRING_H_
#define LIGHTSPEED_CONTAINERS_STRING_H_

#include "stringBase.h"
#include "../memory/sharedResource.h"
#include "../meta/isConvertible.h"


namespace LightSpeed {


//------------------ case insensitivy and compare-------------------
    
	template<typename T, template<class> class Compare >
	class StringTC: public StringCore<T> {
	public:
		typedef StringCore<T> Super;

		typedef typename ConstObject<T>::Remove NCT;
		typedef typename ConstObject<T>::Add CT;

        ///Create empty string
		StringTC() {}

		///Creates empty string (initialized by nil)
		StringTC(NullType) {}



		///Creates instance from pointer to zero terminated string
		/**
		 * @param text pointer to terminated string.
		 * @note For char and wchar_t, character zero is searched. Otherwise,
		 * you have to specialize StringBase<T> for your type, in order
		 * to not receive linking error
		 */
		StringTC(const T *text):Super(text) {}
		///Creates instance from pointer to zero terminated string using allocator instance
		/**
		 * @param text pointer to terminated string.
		 * @param alloc instance of allocator used to allocate memory
		 *
		 * @note For char and wchar_t, character zero is searched. Otherwise,
		 * you have to specialize StringBase<T> for your type, in order
		 * to not receive linking error
		 */
		explicit StringTC(const T *text, const IRuntimeAlloc &alloc):Super(text,alloc) {}

		///Creates string from ConstStringT object
		/**
		 * @param text instance of ConstStringT. It holds begin and length of
		 * 	array of characters. Characters are copied into the instance
		 */
        StringTC(ConstStringT<T> text):Super(text) {}
		///Creates string from ConstStringT object using allocator instance
		/**
		 * @param text instance of ConstStringT. It holds begin and length of
		 * 	array of characters. Characters are copied into the instance
		 *
		 * @param alloc instance of an allocator used to allocate memory
		 */
        StringTC(ConstStringT<T> text, IRuntimeAlloc &alloc):Super(text,alloc) {}


        ///Create copy of the string
        /**
         * Strings are always copied using COW (copy-on-write) method. Until
         * content of string is changed, it is shared between instances and
         * kept only once in the memory. Most operations with the string doesn't
         * modify content, so strings can be effectively copied without need
         * to physically copying the content.
         *
         *
         * @param other source string
         *
         * @note Beware in multi threading environment. It is not recommended
         * to copy strings between the threads, if there is potential danger,
         * that shared content can be accessed by two threads at one time.
         * Default allocator ShareAlloc doesn't handle sharing between
         * threads. To do this, use MT-safe ShareAlloc allocator. But
         * performing MT sharing can cause worse performance. Optimal solution
         * is using MT unsafe sharing, prevent sharing string between threads
         * and use isolate() method to perform full copy while string is
         * being given to an different thread.
         */
		StringTC(const StringTC &other):Super(other) {}


		//@{
		///Creates string from GenArray
		/** this allows to create strings from array expression. It used
		 * after string expression, for example concatenate multiple strings
		 * or retrieving substrings
		 * @param arr array
		 * @param alloc allocator used to allocate memory

		 */
		template<typename B>
		StringTC(const ArrayT<CT,B> &arr, IRuntimeAlloc &alloc):Super(arr,alloc) {}
		template<typename B>
		StringTC(const ArrayT<CT,B> &arr):Super(arr) {}
		template<typename B>
		StringTC(const ArrayT<NCT,B> &arr, IRuntimeAlloc &alloc):Super(arr,alloc) {}
		template<typename B>
		StringTC(const ArrayT<NCT,B> &arr):Super(arr) {}

		StringTC(const StringCore<T> &arr):Super(arr) {}
		//@}

        ///Assignment operator
        /** Destroys current string and shares new string
         *
         * See StringTC copy constructor for more informations about
         * implicit sharing
         * @param other source string
         * @return instance of string
         *
         * @note allocator instance is copied
         */
		StringTC &operator=(const StringTC &other) {
			Super::operator=(other);
			return *this;
		}

/*		bool operator==(const StringTC &other) const {return compare(other) == 0;}
		bool operator<=(const StringTC &other) const {return compare(other) <= 0;}
		bool operator>=(const StringTC &other) const {return compare(other) >= 0;}
		bool operator<(const StringTC &other) const {return compare(other) < 0;}
		bool operator>(const StringTC &other) const {return compare(other) > 0;}
		bool operator!=(const StringTC &other) const {return compare(other) != 0;}
*/
		bool operator==(const ConstStringT<T> &other) const {return compare(other) == 0;}
		bool operator<=(const ConstStringT<T> &other) const {return compare(other) <= 0;}
		bool operator>=(const ConstStringT<T> &other) const {return compare(other) >= 0;}
		bool operator<(const ConstStringT<T> &other) const {return compare(other) < 0;}
		bool operator>(const ConstStringT<T> &other) const {return compare(other) > 0;}
		bool operator!=(const ConstStringT<T> &other) const {return compare(other) != 0;}


		CompareResult compare(const ConstStringT<T> &other) const {
			ConstStringT<T> me = *this;
			Compare<T> cmp;
			return cmp(me,other);
		}
	};

	
    
    typedef StringTC<wchar_t,StrCmpCS> StringW;
    typedef StringTC<char,StrCmpCS> StringA;
    typedef StringTC<byte,StrCmpCS> StringB;
    typedef StringTC<wchar_t,StrCmpCI> StringWI;
    typedef StringTC<char,StrCmpCI> StringAI;

    typedef const char *CodePageDef;

    template<template<class> class StrCmp>
    class UniString: public StringTC<wchar_t, StrCmp> {
    public:
		typedef StringTC<wchar_t, StrCmp> Super;

        ///Create empty string
		UniString() {}

		///Creates empty string (initialized by nil)
		UniString(NullType) {}

		///Creates instance from pointer to zero terminated string
		/**
		 * @param text pointer to terminated string.
		 * @note For char and wchar_t, character zero is searched. Otherwise,
		 * you have to specialize StringBase<T> for your type, in order
		 * to not receive linking error
		 */
		UniString(const wchar_t *text):Super(text) {}
		UniString(const char *text) {loadUtf8(text);}
		UniString(const char *text, CodePageDef cp) {loadCP(text,cp);}
		///Creates instance from pointer to zero terminated string using allocator instance
		/**
		 * @param text pointer to terminated string.
		 * @param alloc instance of allocator used to allocate memory
		 *
		 * @note For char and wchar_t, character zero is searched. Otherwise,
		 * you have to specialize StringBase<T> for your type, in order
		 * to not receive linking error
		 */
		explicit UniString(const wchar_t *text, const IRuntimeAlloc &alloc):Super(text,alloc) {}
		explicit UniString(const char *text, const IRuntimeAlloc &alloc):Super(alloc) {loadUtf8(text);}
		explicit UniString(const char *text, CodePageDef cp, const IRuntimeAlloc &alloc):Super(alloc) {loadCP(text,cp);}

		///Creates string from ConstStringT object
		/**
		 * @param text instance of ConstStringT. It holds begin and length of
		 * 	array of characters. Characters are copied into the instance
		 */
        UniString(ConstStringT<wchar_t> text):Super(text) {}
        UniString(ConstStringT<char> text) {loadUtf8(text);}
        UniString(ConstStringT<char> text, CodePageDef cp) {loadCP(text,cp);}
		///Creates string from ConstStringT object using allocator instance
		/**
		 * @param text instance of ConstStringT. It holds begin and length of
		 * 	array of characters. Characters are copied into the instance
		 *
		 * @param alloc instance of an allocator used to allocate memory
		 */
        UniString(ConstStringT<wchar_t> text, const IRuntimeAlloc &alloc):Super(text,alloc) {}
        UniString(ConstStringT<char> text, const IRuntimeAlloc &alloc):Super(alloc) {loadUtf8(text);}
        UniString(ConstStringT<char> text, CodePageDef cp, const IRuntimeAlloc &alloc):Super(alloc) {loadCP(text,cp);}


		//@{
		///Creates string from GenArray
		/** this allows to create strings from array expression. It used
		 * after string expression, for example concatenate multiple strings
		 * or retrieveing substrings
		 * @param arr array
		 * @param alloc allocator used to allocate memort

		 */
		template< typename B>
		UniString(const ArrayT<wchar_t,B> &arr, IRuntimeAlloc &alloc):Super(arr,alloc) {}
		template<typename B>
		UniString(const ArrayT<wchar_t,B> &arr):Super(arr) {}
		template< typename B>
		UniString(const ArrayT<const wchar_t,B> &arr, IRuntimeAlloc &alloc):Super(arr,alloc) {}
		template<typename B>
		UniString(const ArrayT<const wchar_t,B> &arr):Super(arr) {}
		//@}
		UniString(const StringCore<wchar_t> &str):Super(str) {}

		template<template<class> class X>
		UniString(const StringTC<wchar_t,X> &str):Super(static_cast<const StringCore<wchar_t> &>(str)) {}
		UniString(const Super &str):Super(str) {}
        ///Assignment operator
        /** Destroyes current string and shares new string
         *
         * See UniString copy constructor for more informations about
         * implicit sharing
         * @param other source string
         * @return instance of string
         *
         * @note allocator instance is copied
         */
		UniString &operator=(const UniString &other) {
			Super::operator=(other);
			return *this;
		}


		typedef StringTC<char, StrCmp> StrA;
		
		StrA getUtf8() const;
		StrA getCP(CodePageDef cp) const;

		natural utf8length() const;

        void loadUtf8(ConstStrA text);
        void loadCP(ConstStrA text, CodePageDef cp);

        static StrA getUtf8(ConstStrW src, IRuntimeAlloc &alloc = getStringDefaultAllocator());
        static natural utf8length(ConstStrW src);
    };

    class String: public UniString<StrCmpCS> {
    public:
		typedef UniString<StrCmpCS> Super;

        ///Create empty string
		String() {}

		///Creates empty string (initialized by nil)
		String(NullType) {}

		///Creates instance from pointer to zero terminated string
		/**
		 * @param text pointer to terminated string.
		 * @note For char and wchar_t, character zero is searched. Otherwise,
		 * you have to specialize StringBase<T> for your type, in order
		 * to not receive linking error
		 */
		String(const wchar_t *text):Super(text) {}
		String(const char *text):Super(text) {}
		String(const char *text, CodePageDef cp):Super(text,cp) {}

		///Creates string from ConstStringT object
		/**
		 * @param text instance of ConstStringT. It holds begin and length of
		 * 	array of characters. Characters are copied into the instance
		 */
        String(ConstStringT<wchar_t> text):Super(text) {}
        String(ConstStringT<char> text):Super(text) {}
        String(ConstStringT<char> text, CodePageDef cp):Super(text,cp) {}


		//@{
		///Creates string from GenArray
		/** this allows to create strings from array expression. It used
		 * after string expression, for example concatenate multiple strings
		 * or retrieveing substrings
		 * @param arr array
		 * @param alloc allocator used to allocate memort

		 */
		template<typename A, typename B>
		String(const ArrayT<A,B> &arr):Super(arr) {}


		String(const Super &arr):Super(arr) {}
		String(const Super::Super &arr):Super(arr) {}
		String(const StringCore<wchar_t> &arr):Super(arr) {}
		//@}

        ///Assignment operator
        /** Destroyes current string and shares new string
         *
         * See String copy constructor for more informations about
         * implicit sharing
         * @param other source string
         * @return instance of string
         *
         * @note allocator instance is copied
         */
		String &operator=(const String &other) {
			Super::operator=(other);
			return *this;
		}


    };

	///Retrieves c-string from ConstStrA. 
	/**
	 * Because string in ConstStrA is not always terminated by zero character, function
	 * determines this and fix these situations. That is why there is second argument where
	 * fixed string has been stored. In case, that source contains terminating zero character,
	 * string referred by second argument is untouched and function returns pointer at the
	 * beginning of the original string.
	 *
	 * This is the correct way how to convert ConstStrA to traditional C-string. 
	 *
	 * @param source source string
	 * @param tmp variable used to store string temporary. 
	 * @return pointer to string in C-string style. Note that pointer is valid only if both arguments are valid.

	 *
	 */
	 
	const char *cStr(ConstStrA source, StringA &tmp);
	///Retrieves c-string from ConstStrA. 
	/**
	 * Because string in ConstStrW is not always terminated by zero character, function
	 * determines this and fix these situations. That is why there is second argument where
	 * fixed string has been stored. In case, that source contains terminating zero character,
	 * string referred by second argument is untouched and function returns pointer at the
	 * beginning of the original string.
	 *
	 * This is the correct way how to convert ConstStrW to traditional C-string. 
	 *
	 * @param source source string
	 * @param tmp variable used to store string temporary. 
	 * @return pointer to string in C-string style. Note that pointer is valid only if both arguments are valid.
	 *
	 */
	const wchar_t *cStr(ConstStrW source, StringW &tmp);


}
    

#endif /*STRING_H_*/
























