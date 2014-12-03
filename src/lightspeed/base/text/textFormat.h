/*
 * textOut.h
 *
 *  Created on: 24.12.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEXT_TEXTFORMAT_H_
#define LIGHTSPEED_TEXT_TEXTFORMAT_H_

#pragma once

#include "../invokable.h"
#include "../memory/stdAlloc.h"
#include "../containers/autoArray.h"
#include "../containers/constStr.h"
#include "../memory/pointer.h"
#include "textFormatManip.h"
#include <string>
#include "../text/newline.h"
#include "../memory/staticAlloc.h"

namespace LightSpeed {

enum SimpleManipulators {
	m_hex,
	m_dec,
	m_oct,
	m_bin,
	m_push,
	m_pop,
	m_precision0,
	m_precision1,
	m_precision2,
	m_precision3,
	m_precision4,
	m_precision5,
	m_precision6,
	m_precision7,
	m_precision8,
	m_sci,
	m_fixed
};


	template<typename Impl>
	class TextFormatBase: public Invokable<Impl> {
	public:

		Impl &operator()(ConstStringT<char> pattern);
		Impl &operator()(const char *pattern);

		Impl &operator <<(ConstStringT<char> text);
		Impl &operator <<(const char *text);
		Impl &operator <<(char chr);
		Impl &operator <<(const std::string &chr);

		Impl &operator <<(ConstStringT<wchar_t> text);
		Impl &operator <<(const wchar_t *text);
		Impl &operator <<(wchar_t chr);
		Impl &operator <<(const std::wstring &chr);


		Impl &operator <<(natural num);
		Impl &operator <<(integer num);

	};


	class TextFormatManip;


	class INumberFormatter {
	public:
		virtual ConstStrA operator()(ConstStrA number) = 0;
	};

	///Text formatter
	/**
	 * Pattern characters
	 *  - parameter at index %N
	 *  - parameter at index %{S}N  - define space, align left
	 *  - parameter at index %{-S}N - define space, align right
	 *  - parameter at index %{+S}N - define space, align right, force sign
	 *  - parameter at index %{cS}N - define space, align right, fill with c, sign after fill
	 *  - parameter at index %{-cS}N - define space, align right, optional sign, then fill
	 *  - parameter at index %{+cS}N - define space, align right, sign, then fill
	 *  - conditional parameter %{/reg-exp/text}N - show text only if argument matches simplified reg.exp.
	 *  - new line \n
	 *  - CR \r
	 *  - TAB \t
	 */
	template <typename T, typename Alloc = StdAlloc>
	class TextFormat: public TextFormatBase<TextFormat<T,Alloc> > {

	public:

		using TextFormatBase<TextFormat<T,Alloc> >::operator <<;
		using TextFormatBase<TextFormat<T,Alloc> >::operator ();

		TextFormat():base(10),decimals(6),sciFormat(false),newline(DefaultNLString<T>()) {}
		TextFormat(const Alloc &alloc):buff(alloc),patternBuffer(alloc)
				,base(10),decimals(6),newline(DefaultNLString<T>()) {}

		TextFormat &operator()(ConstStringT<T> pattern);
		TextFormat &operator()(const T *pattern);
		TextFormat &operator()();

		TextFormat &operator <<(ConstStringT<T> text);
		TextFormat &operator <<(const T *text);
		TextFormat &operator <<(T chr);
		TextFormat &operator <<(int num);
		TextFormat &operator <<(unsigned int num);
		TextFormat &operator <<(lnatural num);
		TextFormat &operator <<(linteger num);
		TextFormat &operator <<(float num);
		TextFormat &operator <<(double num);
		TextFormat &operator <<(TextFormatManip m) {
			m.doManip(*this);return *this;
		}

		TextFormat &setBase(natural b);
		TextFormat &setPrecision(natural p);
		TextFormat &setSci(natural decimals);
		TextFormat &setFixed(natural decimals);

		TextFormat &setNumberFormat(INumberFormatter *fmt);

		template<typename WriteIter>
		TextFormat &output(WriteIter &iter);

		void resetArgs();

//		void setNLString();

		TextFormat &setNL(const ConstStringT<T> &nl);
	protected:

		friend class TextFormatBase<TextFormat<T,Alloc> >;
		template<typename Iter, typename x>
		friend class TextOutIter;

		struct ParamItem {
			const T *ptr;
			natural length;
			ParamItem(const T *ptr,natural length):ptr(ptr),length(length) {}
			ParamItem(natural length):ptr(0),length(length) {}
		};

		typedef AutoArray<T,Alloc> Buff;
		typedef ConstStringT<T> Pattern;
		typedef AutoArray<ParamItem,StaticAlloc<30> > ParamList;
		typedef Pointer<INumberFormatter > Formatter;


		Pattern pattern;
		Buff buff;
		Buff patternBuffer;
		ParamList paramList;
		natural base;
		natural decimals;
		bool sciFormat;
		Formatter formatter;
		Pattern newline;

		TextFormat &setPatternChar(const char *text);
		TextFormat &setPatternChar(ConstStringT<char> text);

		TextFormat &setArgChar(const char *text);
		TextFormat &setArgChar(ConstStringT<char> text);
		TextFormat &setArgChar(char chr);

		TextFormat &setArgWChar(const wchar_t *text);
		TextFormat &setArgWChar(ConstStringT<wchar_t> text);
		TextFormat &setArgWChar(wchar_t chr);

		TextFormat &setArgNum(natural num);
		TextFormat &setArgNum(integer num);

	};




	namespace _intr {

		template<typename T>
		ConstStrA numberToString(T num, char *string, natural size, natural base);

		ConstStrA realToString(double num, char *string, natural size, natural decimals, integer scimin, integer scimax);

		template<typename T>
		class LoadCharToBuff {
		public:
			template<typename Alloc>
			static void load(ConstStrA text, AutoArray<T,Alloc> &out);
			template<typename Alloc>
			static void load(ConstStrW text, AutoArray<T,Alloc> &out);
		};

		template<>
		class LoadCharToBuff<wchar_t> {
		public:
			template<typename Alloc>
			static void load(ConstStrA text, AutoArray<wchar_t,Alloc> &out);
			template<typename Alloc>
			static void load(ConstStrW text, AutoArray<wchar_t,Alloc> &out);
		};

		template<>
		class LoadCharToBuff<char> {
		public:
			template<typename Alloc>
			static void load(ConstStrA text, AutoArray<char,Alloc> &out);
			template<typename Alloc>
			static void load(ConstStrW text, AutoArray<char,Alloc> &out);
		};
}



	template <typename T, typename Alloc>
	inline TextFormat<T,Alloc> &operator << (TextFormat<T,Alloc> &tt, const TextFormatManip &item) {
		item.doManip(tt);return tt;
	}

	template <typename T, typename Alloc = StdAlloc>
	class TextFormatBuff: public TextFormat<T,Alloc> {
	public:
		typedef AutoArrayStream<T,Alloc> Buff;
		typedef TextFormat<T,Alloc> Super;

		TextFormatBuff() {}
		TextFormatBuff(const Alloc &alloc)
			:TextFormat<T,Alloc>(alloc),outbuff(alloc) {}

		const AutoArray<T,Alloc> &write() {
			outbuff.clear();TextFormat<T,Alloc>::output(outbuff);
			return outbuff.getArray();
		}
		operator ConstStringT<T>() const {
			return write();
		}
		//Required by VS2013
		using TextFormat<T, Alloc>::operator();
		///Writes text into buffer with terminating zero character
		/**
		 * @return pointer to C-string. Note that written string contains terminating zero,
		 *  so returned length will be heighten about 1 character.
		 *
		 * To retrieve pointer later, use method data(). It will simply return pointer to buffer 
	     * without any action.
		 */ 
		 
		const T *writeCStr() {
			outbuff.clear();TextFormat<T,Alloc>::output(outbuff);
			StringBase<T>::writeZeroChar(outbuff);
			return data();
		}
		const AutoArray<T,Alloc> &getBuffer() const {return outbuff.getArray();}
		const T *data() const {return outbuff.getArray().data();}
		natural length() const {return outbuff.getArray().length();}


	protected:
		Buff outbuff;
	};


}


#endif /* LIGHTSPEED_TEXT_TEXTFORMAT_H_ */
