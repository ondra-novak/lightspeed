/*
 * numtext.h
 *
 *  Created on: 12.10.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEXT_NUMTEXT_H_
#define LIGHTSPEED_TEXT_NUMTEXT_H_
#include "../containers/autoArray.h"
#include "../memory/smallAlloc.h"
#include "../containers/constStr.h"


namespace LightSpeed {

	///general conversion anything to string
	/** Template has two arguments
	 * @tparam T type from which convert
	 * @tparam ChT type of string. There can be either char or wchar_t, or another
	 * type representing one character
	 *
	 * Result is object, which implements ConstStringT and can be used everywhere
	 * string is requested. For char and wchar_t, strings are terminated by zero
	 * character, so it should be easy to extract string and use it in standard
	 * C library
	 *
	 * @code
	 * textOut(ToString<natural>(10));
	 * @endcode
	 *
	 * Call textOut with text containing converted number
	 *
	 * Every conversion object should have standard constructor, which accept
	 * single argument - object to convert, and optionally extended constructor
	 * where you can specify more informations
	 *
	 * 	 */
	template<typename T, typename ChT = char>
	class ToString;

	template<typename ChT, natural bufBaseSize>
	class ToStringBase: public ConstStringT<ChT> {
	protected:

		typedef AutoArrayStream<ChT, SmallAlloc<bufBaseSize> > Buffer;
		Buffer buffer;

		void commitBuffer() {
			this->len = buffer.length();
			StringBase<ChT>::writeZeroChar(buffer);
			this->refdata = buffer.data();
		}

		operator const ChT *() const {return this->data();}

	};

	template<typename NumType, typename ChT, natural bufsize>
	class ToStringUnsigned: public ToStringBase<ChT,bufsize>{
	public:
		ToStringUnsigned(NumType val, natural base, bool up) {extractFirst(val,base,up);}
	protected:
		void extractFirst(NumType val, natural base, bool up) {
			if (val == 0) this->buffer.write(ChT('0'));
			else extract(val,base,up);
			this->commitBuffer();
		}
		void extract(NumType val, natural base, bool up) {
			if (val >= base) extract(val/base,base,up);
			natural x = val % base;
			if (x > 9) this->buffer.write(ChT((up?'A':'a')+x - 10));
			else this->buffer.write(ChT('0'+x));
		}
	};

	template<typename NumType, typename ChT, natural bufsize>
	class ToStringSigned : public  ToStringBase<ChT, bufsize>{
	public:
		ToStringSigned(NumType val, natural base, bool up) {extractFirst(val,base,up);}
	protected:
		void extractFirst(NumType val, natural base, bool up) {
			if (val == 0) this->buffer.write(ChT('0'));
			else {
				if (val < 0) {
					this->buffer.write(ChT('-'));
					this->extract(-val,base,up);
				} else {
					this->extract(val,base,up);
				}
			}
			this->commitBuffer();
		}
		void extract(NumType val, natural base, bool up) {
			if (val >= (NumType)base) extract(val / base, base, up);
			natural x = val % base;
			if (x > 9) this->buffer.write(ChT((up ? 'A' : 'a') + x - 10));
			else this->buffer.write(ChT('0' + x));
		}

	};
	template<typename ChT>
	class ToString<natural,ChT>: public ToStringUnsigned<natural,ChT,23> {
	public:
		 ToString(natural val, natural base=10, bool up = false)
			:ToStringUnsigned<natural,ChT,23>(val,base,up) {}
	};

	template<typename ChT>
	class ToString<integer,ChT>: public ToStringSigned<integer,ChT,23> {
	public:
		 ToString(integer val, natural base=10, bool up = false)
			:ToStringSigned<integer,ChT,23>(val,base,up) {}
	};
#ifdef LIGHTSPEED_ENV_32BIT

	template<typename ChT>
	class ToString<lnatural,ChT>: public ToStringUnsigned<lnatural,ChT,23> {
	public:
		 ToString(lnatural val, natural base=10, bool up = false)
			:ToStringUnsigned<lnatural,ChT,23>(val,base,up) {}
	};

	template<typename ChT>
	class ToString<linteger,ChT>: public ToStringSigned<linteger,ChT,23> {
	public:
		 ToString(linteger val, natural base=10, bool up = false)
			:ToStringSigned<linteger,ChT,23>(val,base,up) {}
	};

#endif

	template<>
	class ToString<float,char>: public ToStringBase<char,50> {
	public:
		 ToString(float val, integer precision, bool sci = false);
		 ToString(float val);
	};

	template<>
	class ToString<double,char>: public ToStringBase<char,50> {
	public:
		 ToString(double val, integer precision, bool sci = false);
		 ToString(double val);
	};

	template<typename TcH>
	class ToString<float,TcH>: public ToStringBase<TcH,50> {
	public:
		 ToString(float val, integer precision, bool sci = false) {
			 ToString<float,char> tmp(val,precision,sci);
			 ToString<float,char>::Iterator iter = tmp.getFwIter();
			 this->buffer.copy(iter);
			 this->commitBuffer();
		 }
		 ToString(float val) {
			 ToString<float,char> tmp(val);
			 ToString<float,char>::Iterator iter = tmp.getFwIter();
			 this->buffer.copy(iter);
			 this->commitBuffer();
		 }
	};

	template<typename TcH>
	class ToString<double,TcH>: public ToStringBase<TcH,50> {
	public:
		 ToString(double val, integer precision, bool sci = false) {
			 ToString<double,char> tmp(val,precision,sci);
			 ToString<double,char>::Iterator iter = tmp.getFwIter();
			 this->buffer.copy(iter);
			 this->commitBuffer();
		 }
		 ToString(double val) {
			 ToString<double,char> tmp(val);
			 ToString<double,char>::Iterator iter = tmp.getFwIter();
			 this->buffer.copy(iter);
			 this->commitBuffer();
		 }
	};

}

#endif /* LIGHTSPEED_TEXT_NUMTEXT_H_ */
