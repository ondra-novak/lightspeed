/*
 * textParserComposer.h
 *
 *  Created on: 24.7.2009
 *      Author: ondra
 */

#include "../exceptions/exception.h"

#ifndef LIGHTSPEED_STREAMS_TEXTPARSERCOMPOSER_HDR_H_
#define LIGHTSPEED_STREAMS_TEXTPARSERCOMPOSER_HDR_H_

#include <math.h>
#include "../iter/iteratorFilter.h"
#include "../containers/genericArray.h"
#include "../containers/oneitemarray.h"
#include "../containers/carray.h"
#include "../countof.h"


namespace LightSpeed {

    struct NumberIntegerFormat {
        ///Symbol for positive value
        Ascii pos;
        ///Symbol for negative value
        Ascii neg;
        ///zero padding character
        Ascii zeroPad;
        ///space padding character
        Ascii spacePad;

        NumberIntegerFormat():pos('+'),neg('-'),zeroPad('0'),spacePad(' ') {}

    };

    ///Defines real number format
    struct NumberFormat: NumberIntegerFormat {
        ///Symbol for positive exponent
        Ascii epos;
        ///Symbol for negative exponent
        Ascii eneg;
        ///Symbol for decimal dot
        Ascii dot;
        ///Symbol for thousand separator
        Ascii thousand;
        ///Symbol for exponent separator ('E')
        Ascii eMain;
        ///Alternative symbol for exponent ('e')
        Ascii eAlt;

#define LIGHTSPEED_NUMBER_FORMAT_INIT \
                epos('+'),eneg('-'),dot('.'),thousand(0),eMain('E'),eAlt('e')

        NumberFormat():LIGHTSPEED_NUMBER_FORMAT_INIT {}
        NumberFormat(const NumberIntegerFormat &integerFormat)
            : NumberIntegerFormat(integerFormat), LIGHTSPEED_NUMBER_FORMAT_INIT {}

#undef LIGHTSPEED_NUMBER_FORMAT_INIT
    };



    struct NumberIntegerStyle {
        ///How many digits will be displayed/reserved before dot
        /**
         * Defines count of digis, or empty space characters for displayed
         * before the dot. It doesn't include space for sign character
         * and also excludes dot character itself and anything after it.
         *
         * Default value is naturalNull, which causes, that no extra
         * spaces or zeroes will be pretend to the number
         */
        natural beforeDotDigits;
        ///Set true, if you want to display plus before positive value. Default is false
        bool plusSign;
        ///Set true, if you want fill left padding with zeroes
        bool zeroPad;

        NumberIntegerStyle():beforeDotDigits(0),plusSign(false),zeroPad(false) {}
    };

    ///Defines parameters to compose real number
    struct NumberStyle: public NumberIntegerStyle {

        enum Mode {
            ///automatic select best format
            fmtAuto,
            ///use fixed decimal number
            fmtFix,
            ///use exponent with fixed count of mantisa digits
            fmtSci,
        };
        ///How many digits will be displayed after dot
        /**
        * Defines count of digis will be displayed after dot. If
        * fmtFix, it specifies count of decimals, in fmtFix, it specifies
        * count of decimals of the number before E. Use zero to remove
        * decimals and dot character. You can use negative value, to
        * remove digits before dot, these digits will be replaced by
        * zeroes and remain part of number will be rounded (for example,
        * number 12345678 with afterDotDigits=-2 will be displayed as
        * 12345700).
        *
        * Default value is integerNull (do not use naturalNull),
        * which chooses optimal number of decimals depend on number.
        */
        integer afterDotDigits;
        ///Specifies display mode
        Mode mode;
        ///Set true, if you want to disable thousand separator.
        /**
         * It is useful to temporary disable thousand separator without need
         * to modify number format. This should be turned on, when
         * zeroPad is true, because separators are not added into
         * padding zeroes
         */
        bool noThousands;
#define LIGHTSPEED_NUMBER_FORMAT_INIT \
        afterDotDigits(integerNull),mode(fmtAuto),noThousands(false)

        NumberStyle():LIGHTSPEED_NUMBER_FORMAT_INIT {}
        NumberStyle(const NumberIntegerStyle &style):
            NumberIntegerStyle(style), LIGHTSPEED_NUMBER_FORMAT_INIT {}
#undef LIGHTSPEED_NUMBER_FORMAT_INIT

        static NumberStyle defStyle() {return NumberStyle();}
        static NumberStyle intNumb(natural count, bool zeroes = false, bool sign = false) {
        	NumberStyle x;
        	x.zeroPad = zeroes;
        	x.beforeDotDigits = count;
        	x.plusSign = sign;
        	return x;
        }
        static NumberStyle floatNumb(natural beforeDot,
									 natural afterDot,
									 Mode mode = fmtAuto,
									 bool zeroes = false,
									 bool sign = false,
									 bool noThs = false) {
        	NumberStyle x;
        	x.zeroPad = zeroes;
        	x.beforeDotDigits = beforeDot;
        	x.plusSign = sign;
        	x.afterDotDigits = afterDot;
        	x.mode = mode;
        	x.noThousands = noThs;
        	return x;

        }

    };


    ///Filter parses unsigned number
    /**
     * Filter expects characters (Ascii) at its input and as result is
     * returned unsigned number. Filter accepts only digits, depend on
     * selected base. It will not accept sign characters.
     *
     */

	template<class T = natural>
	class TextParseUnsigned: public IteratorFilterBase<Ascii, T, TextParseUnsigned<T> > {
	public:
		typedef IteratorFilterBase<Ascii, T, TextParseUnsigned<T> > Super;

		///Constructs the parser
		/**
		 * @param base specify numeric base.
		 */
		TextParseUnsigned(byte base = 10);

        bool needItems() const;
        bool canAccept(const Ascii &x) const;
        void input(const Ascii &x);
        bool hasItems() const;
        T output();
        void flush();
	protected:
        T rdnumber;
        byte base;
        bool flushState;
        bool validNumber;

        T charToDigit(Ascii v) const;
	};


	///Filter composes text representation of unsigned number
	/**
	 * Filter expects number in specified type. As result, it
	 * generates series of character representing the number in text form
	 *
	 * It will output only numbers.
	 *
	 * If you pass signed type as T, don't call filter with negative number.
	 * It will not work as expected. To compose signed number see TextComposeSigned
	 *
	 */
	template<class T>
	class TextComposeUnsigned: public IteratorFilterBase<T, Ascii, TextComposeUnsigned<T> > {
	public:
		typedef IteratorFilterBase<T, Ascii, TextComposeUnsigned<T> > Super;

        ///Constructs composer with specified base
        /**
         * @param base base of the nuber
         */
		TextComposeUnsigned(natural base = 10);

        bool needItems() const;
        bool canAccept(const T &x) const;
        void input(const T &x);
        bool hasItems() const;
        Ascii output() ;
        void flush();
        natural getWidth() const {return count;}
	protected:
        static const natural buffSize = sizeof(T) * 8;
        Ascii buffer[buffSize];
        byte base;
        byte pos;
        byte count;
        natural renderNumber(lnatural numb, Ascii *buffer, natural count);

	};

	namespace _intr {
        Ascii digitToChar(byte digit);
        byte charToDigit(Ascii chr);

	}

    ///Filter parses unsigned number
    /**
     * Filter expects characters (Ascii) at its input and as result is
     * returned signed number. Filter accepts digits, depend on
     * selected base and optionally the signum character (plus or minus)
     * as the first character
     */

	template<class T>
	class TextParseSigned: public IteratorFilterBase<Ascii, T, TextParseSigned<T> > {
	public:
		typedef IteratorFilterBase<Ascii, T, TextParseSigned<T> > Super;

        ///Constructs the parser
        /**
         * @param base specify numeric base.
         */
		TextParseSigned(byte base = 10);
	    TextParseSigned(byte base ,const NumberIntegerFormat &fmt);

        bool needItems() const;
        bool canAccept(const Ascii &x) const;
        void input(const Ascii &x);
        bool hasItems() const;
        T output();
        void flush();
	protected:
        TextParseUnsigned<T> digitParser;
        NumberIntegerFormat fmt;
        Ascii signum;
	};

    ///Filter composes text representation of a signed number
    /**
     * Filter expects number in specified type. As result, it
     * generates series of character representing the number in text form
     *
     * It will output only numbers and optionally sign character
     *
     */

	template<class T = integer>
	class TextComposeSigned: public IteratorFilterBase<T, Ascii, TextComposeSigned<T> > {
	public:
		typedef IteratorFilterBase<T, Ascii, TextComposeSigned<T> > Super;

		///Constructs default composer
		TextComposeSigned(natural base = 10);

		TextComposeSigned(natural base,const NumberIntegerStyle &style);

        TextComposeSigned(natural base,const NumberIntegerStyle &style,
                                const NumberIntegerFormat &fmt);


        bool needItems() const;
        bool canAccept(const T &x) const;
        void input(const T &x);
        bool hasItems() const;
        Ascii output() ;
        void flush();
	protected:
        TextComposeUnsigned<T> digitComposer;
        NumberIntegerFormat fmt;
        NumberIntegerStyle style;
        Ascii sigChar;
        unsigned short remainWidth;

        enum StageState {
            stageEmpty = 0,
            stageSpacePadding = 1,
            stageSignum = 2,
            stageZeroPadding = 3,
            stageDigit = 4
        };
        StageState stage;
	};


	template<class T = double>
	class TextParseReal: public IteratorFilterBase<Ascii, T, TextParseReal<T> >
	{
	public:

	    TextParseReal();
        TextParseReal(const NumberFormat &format);

	    bool needItems() const;
        bool canAccept(const Ascii &x) const;
        void input(const Ascii &x);
        bool hasItems() const;
        T output() ;

	protected:

        enum ParseStage {
            beforeDot,
            afterDot,
            exponent
        };

        NumberFormat fmt;
        T numb;
        natural expval;
        natural dotPos;
        ParseStage stage;
        Ascii nrsign, expsign;
	};


    template<class T = double>
    class TextComposeReal: public IteratorFilterBase<Ascii, T, TextParseReal<T> >
    {
    public:

        ///Constructs composer with default settings
        TextComposeReal();
        ///Constructs composer with specifying number format
        /**
         * @param nfmt format
         */
        TextComposeReal(const NumberFormat &nfmt);
        ///Constructs composer specifying display mode
        /**
         *
         * @param mode See Mode
         */
        TextComposeReal(const NumberStyle &mode);
        ///Constructs composer specifying display mode and number format
        /**
         *
         * @param mode See Mode
         * @param nfmt format
         */
        TextComposeReal(const NumberStyle &mode, const NumberFormat &nfmt);

        bool needItems() const;
        bool canAccept(const T &x) const;
        void input(const T &x);
        bool hasItems() const;
        Ascii output() ;

    protected:

        enum Stage {
            stNoData,
            stSign,
            stMantissa,
            stEChar,
            stESign,
            stExponent
        };

        template<class Arr>
        class ThousandSep: public GenArray<Ascii, ThousandSep<Arr> > {
        public:
            ThousandSep(Arr a, Ascii sep):arr(a),sep(sep) {}
            ThousandSep():sep(0) {}

            const Ascii &at(natural pos) const {
                if (sep == 0) return arr._invoke().at(pos);
                else {
                    natural tgsz = arr._invoke().length();
                    natural sz = tgsz * 4 / 3;
                    sz -= (((sz - 1) & 3) +1)>>2;
                    natural spos = sz - pos - 1;
                    natural rpos = spos - (spos >> 2);
                    natural tpos = tgsz - rpos - 1;
                    if ((spos & 0x3) == 3) return sep;
                    else return arr._invoke().at(tpos);
                }
            }

            natural length() const {
                natural res = arr._invoke().length();
                if (sep == 0) return res;
                else {
                    natural x = res * 4 / 3;
                    x -= (((x - 1) & 3)+1)>>2;
                    return x;
                }
            }

            Arr &getArg() {return arr;}
            const Arr &getArg() const {return arr;}
            void setSep(Ascii sep) {this->sep = sep;}


        protected:
            Ascii sep;
            Arr arr;

        };

		template<typename A, typename B>
		class MySum: public GenArrSumT<A,B> {
		public:
			MySum():GenArrSumT<A,B>(A(),B()) {}
		};

		template<typename A>
		class MyRepeat: public GenArrRepT<A> {
		public:
			MyRepeat():GenArrRepT<A>(A(),0) {}
			MyRepeat(A a, natural count):GenArrRepT<A>(a,count) {}
			void setCount(natural count) {this->count = count;}
		};

        typedef CArray<Ascii> BuffStr;
        typedef OneItemArray<Ascii> Character;
        typedef MyRepeat<Character> CharacterRep; //.....
        typedef MySum<BuffStr,CharacterRep> RPaddedNumber; //123450000
        typedef MySum<CharacterRep,BuffStr> LPaddedNumber; //000012345
        typedef ThousandSep<RPaddedNumber> RPaddedNumberWithThsSeps; //123'450'000
        typedef MySum<CharacterRep, RPaddedNumberWithThsSeps> PaddedNumber; //00001234500000
        typedef MySum<CharacterRep,PaddedNumber> LPadPaddedNumber; //xxx00001234500000
        typedef MySum<CharacterRep,LPaddedNumber> LPadLPaddedNumber;
        //Variants:
        /* _____sNNNNN.NNNNNNEsXXX
         * s00000NNNNN.NNNNNNEsXXX
         * \---------/\-----/\---/
         *  LPadPadd    Padd   LPadLPad
         *
         *
         */


        typedef MySum<LPadPaddedNumber,PaddedNumber> BeforeE;
        typedef MySum<BeforeE,LPadLPaddedNumber > WholeNumber;
        typedef ArrayIterator<Ascii, WholeNumber &> WholeNumberIter;

        Ascii digits[32];
        Ascii expdig[6]; //E+0000
        WholeNumber wholeNumber;



        NumberFormat fmt;
        NumberStyle mode;
        WholeNumberIter iter;

        void prepareZero();
        natural renderNumber(lnatural numb, Ascii *buffer, natural count);
        static lnatural roundAt(T numb, integer pos);
    };


}

#endif /* TEXTPARSERCOMPOSER_H_ */
