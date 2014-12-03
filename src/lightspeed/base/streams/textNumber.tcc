/*
 * textParserComposer.h
 *
 *  Created on: 24.7.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_STREAMS_TEXTPARSERCOMPOSER_H_
#define LIGHTSPEED_STREAMS_TEXTPARSERCOMPOSER_H_

#include "textNumber.h"
#include "../exceptions/iterator.h"
#include "../exceptions/throws.tcc"
#include "../iter/iteratorFilter.tcc"

#ifdef _WIN32
inline double exp10(double x) {
	return pow(10.0,x);
}
inline double round(double x) {
	return floor(x+0.5);
}
#endif

namespace LightSpeed {


    //--------------------------------- IMP ----------------------------------------


	template<class T>
	TextParseUnsigned<T>::TextParseUnsigned(byte base):rdnumber(0),base(base),flushState(false),validNumber(false) {}

	template<class T>
	bool TextParseUnsigned<T>::needItems() const {
		return !flushState;
	}
	template<class T>
	T TextParseUnsigned<T>::charToDigit(Ascii v) const {
    	//in lower chars... transfer to big upper chars
    	if (v > 'a' && v < 'z') v = v - ('a' - 'A');
    	//above '9'
    	if (v > '9') {
    		//if below 'A' mark not-a-digit
    		if (v < 'A') return naturalNull;
    		//otherwise, move to follow '9'
    		else v = v - ('A' - '9') + 1;
    	}
    	//above '0'
    	if (v >= '0')
    		//move to zero
    		v = v - '0';
    	else
    		//otherwise, mark not-a-digit
    		return naturalNull;

    	if (v < base) return v;
    	else return naturalNull;
	}

	template<class T>
	bool TextParseUnsigned<T>::canAccept(const Ascii &x) const {
		return charToDigit(x) != (T)naturalNull;
	}

	template<class T>
    void TextParseUnsigned<T>::input(const Ascii &x) {
    	//we are in the flush state ... sot throw exception busy
    	if (flushState) throw FilterIteratorBusyException(THISLOCATION);
    	//convert character to digit
    	T digit = charToDigit(x);
    	//digit cannot be converted? throw exception
    	if (digit == (T)naturalNull) throw WriteIteratorNotAcceptable(THISLOCATION);
    	//add digit to value
   		rdnumber = rdnumber * base + digit;
   		//and this number is valid
   		validNumber = true;
    }
	template<class T>
    bool TextParseUnsigned<T>::hasItems() const {
    	//we can supply any valid number
    	return validNumber;
    }

	template<class T>
    T TextParseUnsigned<T>::output() {
    	//if there is no valid number, throw exception
    	if (!validNumber) throw IteratorNoMoreItems(THISLOCATION,typeid(T));
    	//store result
    	T res = rdnumber;
    	//reset value
    	rdnumber = 0;
    	//it is not valid number
    	validNumber = false;
    	//reset any flush state
    	flushState = false;
    	//return result
    	return res;
    }

	template<class T>
    void TextParseUnsigned<T>::flush() {
    	//in flush, switch to flush state, if there is valid number
    	flushState = validNumber;
    }


	template<class T>
    TextComposeUnsigned<T>::TextComposeUnsigned(natural base):
        base((byte)base),pos(0),count(0) {

    }

	template<class T>
    bool TextComposeUnsigned<T>::needItems() const {
		return pos == count;
    }
	template<class T>
    bool TextComposeUnsigned<T>::canAccept(const T &x) const {
    	return true;
    }
	template<class T>
    void TextComposeUnsigned<T>::input(const T &x) {
		if (x == 0) {
			buffer[0] = '0';
			pos = 0;
			count = 1;
		} else {
		    count = (byte)renderNumber(x,buffer,countof(buffer));
		}
	}
	template<class T>
    bool TextComposeUnsigned<T>::hasItems() const {
		return pos < count;
    }
	template<class T>
    Ascii TextComposeUnsigned<T>::output() {
		if (pos >= count)
			throw IteratorNoMoreItems(THISLOCATION,typeid(Ascii));
		else
			return buffer[pos++];
    }
	template<class T>
    void TextComposeUnsigned<T>::flush() {
		//empty, because filter is either empty or in flush state
    }


    inline Ascii _intr::digitToChar(byte digit) {
		if (digit < 10) return (Ascii)(digit + '0');
		else return (Ascii)(digit - 10 + 'A');
	}

    inline byte _intr::charToDigit(Ascii chr) {
        if (chr > 'a') return charToDigit(chr - 'a' + 'A');
        if (chr > 'A') return (byte)chr - 'A' + 10;
        return (byte)chr - '0';
    }

    template<class T>
    natural TextComposeUnsigned<T>::renderNumber(lnatural numb, Ascii *buffer,
                                             natural count) {
        using namespace _intr;

        if (numb == 0) return 0;
        natural sz = renderNumber(numb/base,buffer,count);
        Ascii x = digitToChar(numb%base);
        if (sz > count) return sz;
        buffer[sz] = x;
        return sz+1;

    }

    template<class T>
    TextParseSigned<T>::TextParseSigned(byte base)
        :digitParser(base), signum(0)
    {
    }

    template<class T>
    TextParseSigned<T>::TextParseSigned(byte base, const NumberIntegerFormat &fmt)
        :digitParser(base), fmt(fmt),signum(0)
    {
    }


    template<class T>
    bool TextParseSigned<T>::needItems() const
    {
        return digitParser.needItems();
    }



    template<class T>
    bool TextParseSigned<T>::canAccept(const Ascii & x) const
    {
        return digitParser.canAccept(x) || x==fmt.neg || x == fmt.pos
                || x == fmt.zeroPad || x == fmt.spacePad;
    }



    template<class T>
    void TextParseSigned<T>::input(const Ascii & x)
    {
        try {
            if (this->signum == 0) {
                if (x == fmt.pos || x == fmt.neg) {
                    this->signum = x;
                    return;
                }
                if (x == fmt.spacePad)
                    return;
                if (x == fmt.zeroPad) {
                    this->signum = fmt.pos;
                    return digitParser.input('0');
                }
				this->signum = fmt.pos;
            }
            return digitParser.input(x);
        } catch (WriteIteratorNotAcceptable &e) {
            throw WriteIteratorNotAcceptable(THISLOCATION) << e;
        }
    }



    template<class T>
    bool TextParseSigned<T>::hasItems() const
    {
        return digitParser.hasItems();
    }



    template<class T>
    T TextParseSigned<T>::output()
    {
        Ascii s = signum;
        if (s == fmt.neg) return -digitParser.output();
        else return digitParser.output();
    }



    template<class T>
    void TextParseSigned<T>::flush()
    {
        digitParser.flush();
    }


    template<class T>
    TextComposeSigned<T>::TextComposeSigned(natural base)
        :digitComposer(base),sigChar(0),stage(stageEmpty) {}

    template<class T>
    TextComposeSigned<T>::TextComposeSigned(natural base,
                                const NumberIntegerStyle &style)
        :digitComposer(base),style(style),sigChar(0),stage(stageEmpty) {

    }

    template<class T>
    TextComposeSigned<T>::TextComposeSigned(natural base,
                                const NumberIntegerStyle &style,
                                const NumberIntegerFormat &fmt)
        :digitComposer(base),fmt(fmt),style(style),sigChar(0),stage(stageEmpty) {

    }


    template<class T>
    bool TextComposeSigned<T>::needItems() const
    {
        return stage == stageEmpty;
    }



    template<class T>
    bool TextComposeSigned<T>::canAccept(const T & x) const
    {
        return digitComposer.canAccept(x);
    }



    template<class T>
    void TextComposeSigned<T>::input(const T & x)
    {
        if (x < 0) {
            sigChar = fmt.neg;
            digitComposer.input((natural)-x);
        } else {
            if (style.plusSign) sigChar = fmt.pos;
            else sigChar = fmt.spacePad;
            digitComposer.input((natural)x);
        }
        natural w = digitComposer.getWidth();
        if (w < style.beforeDotDigits) {
            remainWidth = (unsigned short)(style.beforeDotDigits - w);
            stage = style.zeroPad?stageZeroPadding:stageSpacePadding;
        } else {
            stage = stageSignum;
            remainWidth = 0;
        }

    }



    template<class T>
    bool TextComposeSigned<T>::hasItems() const
    {
        return stage != stageEmpty;
    }



    template<class T>
    Ascii TextComposeSigned<T>::output()
    {
        if (stage == stageEmpty) {
            throw IteratorNoMoreItems(THISLOCATION, typeid(Ascii));
        }
        switch (stage) {
            case stageEmpty:
                throw IteratorNoMoreItems(THISLOCATION, typeid(Ascii));
            case stageSpacePadding:
                if (--remainWidth == 0)
                    stage = stageSignum;
                return fmt.spacePad;
            case stageSignum:
                if (remainWidth)
                    stage = stageZeroPadding;
                else
                    stage = stageDigit;
                return sigChar;
            case stageZeroPadding:
                if (--remainWidth == 0)
                    stage = stageDigit;
                return fmt.zeroPad;
            case stageDigit: {
                Ascii res = digitComposer.output();
                if (!digitComposer.hasItems())
                    stage = stageEmpty;
                return res;
            }
            default:
                throwUnsupportedFeature(THISLOCATION,this,
                        "Unknown stage, probably unintialized class");
                throw;
        }
    }



    template<class T>
    void TextComposeSigned<T>::flush()
    {
        digitComposer.flush();
    }

    template<class T>
    TextParseReal<T>::TextParseReal()
        :numb(0),expval(0),dotPos(0),stage(beforeDot)
        ,nrsign(0),expsign(0)
        {}
    template<class T>
    TextParseReal<T>::TextParseReal(const NumberFormat &fmt)
        :fmt(fmt),numb(0),expval(0),dotPos(0),stage(beforeDot)
        ,nrsign(0),expsign(0)
        {}

    template<class T>
    bool TextParseReal<T>::needItems() const {
        return true;
    }
    template<class T>
    bool TextParseReal<T>::canAccept(const Ascii &x) const {
        if (x == 0) return false;
        if (x >='0' && x <='9') return true;
        if (stage == beforeDot) {
            if (x == fmt.dot || x == fmt.thousand || x == fmt.eMain || x == fmt.eAlt
                    || ((x == fmt.pos|| x==fmt.neg || x == fmt.spacePad)
                            && nrsign == 0) || x == fmt.zeroPad)
                return true;
        } else if (stage == afterDot) {
            return (x == fmt.eMain || x == fmt.eAlt);
        } else if (stage == exponent) {
            if (expsign == 0 && (x == fmt.epos|| x == fmt.eneg))
                return true;
        }
        return false;

    }
    template<class T>
    void TextParseReal<T>::input(const Ascii &x) {
        switch (stage) {
            case beforeDot:
                if (x >= '0' && x <= '9') {
                    numb = numb * 10 + (x - '0');
                    if (nrsign == 0) nrsign = 1;
                } else if (x == fmt.thousand || x == fmt.zeroPad) {
                    return;
                } else if (x == 'E' || x == 'e') {
                    stage = exponent;
                    expval = 0;
                    expsign = 0;
                } else if (x == fmt.dot) {
                    stage = afterDot;
                    dotPos = 0;
                } else if (x == fmt.pos && nrsign == 0) {
                    nrsign = 1;
                } else if (x == fmt.neg && nrsign == 0) {
                    nrsign = -1;
                } else if (x == fmt.spacePad && nrsign == 0) {
                    return;
                } else
                    throw WriteIteratorNotAcceptable(THISLOCATION);
                break;
            case afterDot:
                if (x >= '0' && x <= '9') {
                    numb = numb * 10 + (x - '0');
                    dotPos++;
                } else if (x == fmt.eMain || x == fmt.eAlt) {
                    stage = exponent;
                    expval = 0;
                    expsign = 0;
                } else
                    throw WriteIteratorNotAcceptable(THISLOCATION);
                break;
            case exponent:
                if (x >= '0' && x <= '9') {
                    expval = expval * 10 + (x - '0');
                    if (expsign == 0) expsign = 1;
                } else if (x == fmt.epos && expsign == 0) {
                    expsign = 1;
                } else if (x == fmt.eneg && expsign == 0) {
                    expsign = -1;
                } else
                    throw WriteIteratorNotAcceptable(THISLOCATION);
        }
    }

    template<class T>
    bool TextParseReal<T>::hasItems() const {
        return nrsign != 0;
    }
    template<class T>
    T TextParseReal<T>::output()  {
        if (nrsign == 0) nrsign = 1;
        T res = numb*exp10((integer)expval*expsign - (integer)dotPos) * nrsign;
        numb = 0;
        dotPos = 0;
        expval = 0;
        expsign = 0;
        nrsign = 0;
        stage = beforeDot;
        return res;

    }

    template<class T>
    TextComposeReal<T>::TextComposeReal():iter(wholeNumber,0,0,0) {}
    template<class T>
    TextComposeReal<T>::TextComposeReal(const NumberFormat &nfmt)
        :fmt(nfmt),iter(wholeNumber,0,0,0) {}
    template<class T>
    TextComposeReal<T>::TextComposeReal(const NumberStyle &mode)
        :mode(mode),iter(wholeNumber,0,0,0) {}
    template<class T>
    TextComposeReal<T>::TextComposeReal(const NumberStyle &mode,
                                        const NumberFormat &nfmt)
        :fmt(nfmt),mode(mode),iter(wholeNumber,0,0,0) {}
    template<class T>
    bool TextComposeReal<T>::needItems() const{
        return !iter.hasItems();
    }
    template<class T>
    bool TextComposeReal<T>::canAccept(const T &x) const{
        return true;
    }
    template<class T>
    void TextComposeReal<T>::input(const T &x){

        BeforeE &beforeE = wholeNumber.getArg1();
        LPadPaddedNumber &beforeDot = beforeE.getArg1();
        CharacterRep &place1 = beforeDot.getArg1();
        PaddedNumber &afterPlace1 = beforeDot.getArg2();
        CharacterRep &place2 = afterPlace1.getArg1();
        RPaddedNumberWithThsSeps &sepPlace = afterPlace1.getArg2();
        RPaddedNumber &afterPlace2 = sepPlace.getArg();
        BuffStr &beforeDotDigits = afterPlace2.getArg1();
        CharacterRep &place4 = afterPlace2.getArg2();
        PaddedNumber &afterDot = beforeE.getArg2();
        CharacterRep &dotPlace = afterDot.getArg1();
        RPaddedNumberWithThsSeps &sepPlace2= afterDot.getArg2();
        RPaddedNumber &afterDotPlace = sepPlace2.getArg();
        BuffStr &afterDotDigits = afterDotPlace .getArg1();
        CharacterRep &place6 = afterDotPlace.getArg2();
        LPadLPaddedNumber &afterE = wholeNumber.getArg2();
        LPaddedNumber &rightMost = afterE.getArg2();
        CharacterRep &eChar = afterE.getArg1();
        CharacterRep &signEChar = rightMost.getArg1();
        BuffStr &expText = rightMost.getArg2();
        lnatural intval;
        NumberStyle::Mode curMode = mode.mode;
        float expVal;
        integer lpadfix = 0;

        sepPlace.setSep(mode.noThousands?0:fmt.thousand);

        T tmp(x);
        bool negVal = tmp < 0;
        if (negVal) tmp = -tmp;

        if (x == 0) {
            intval = 0;
            expVal = 0.0f;
        } else {



            expVal = (float) floor(log10(tmp));

        }
        int nExpVal = (int) expVal;

        if (curMode == NumberStyle::fmtAuto) {
            if (expVal>9.0f || expVal<-4.0f) curMode = NumberStyle::fmtSci;
            else curMode = NumberStyle::fmtFix;
        }


        if (curMode == NumberStyle::fmtSci) {
            tmp = tmp / (T)exp10(expVal);
            int nexpVal = nExpVal;
            if (nexpVal < 0) {
                nexpVal = -nexpVal;
                signEChar = CharacterRep(fmt.eneg,1);
            } else {
                signEChar = CharacterRep(fmt.pos,1);
            }
            eChar = CharacterRep(fmt.eMain,1);
            natural sz = renderNumber(nexpVal,expdig,countof(expdig));
            if (sz == 0) {
                expdig[0] = '0';
                sz = 1;
            }
            expText = BuffStr(expdig,sz);
            nExpVal = 0;
            expVal = 0;
        } else {
            eChar = CharacterRep();
            signEChar = CharacterRep();
            expText = BuffStr();
        }

        {
            //any decimals?
            integer decm = mode.afterDotDigits;
            const integer autoDecm = sizeof(T)*5/4;
            if (decm == integerNull) decm = -nExpVal+autoDecm;
            integer origdecm = decm;
            intval = roundAt(tmp,decm);
            while (intval == lnaturalNull) {
                decm--;
                intval = roundAt(tmp,decm);
            }

            natural sz = renderNumber(intval,digits,countof(digits));
            if (mode.afterDotDigits == integerNull) {
                while (sz > 0 && digits[sz-1] == '0' && decm > 0) {
                    sz--;
                    decm--;
                }
                origdecm = decm;
            }
            integer dotPos = (integer)sz - decm;
            if (dotPos <= 0) {
                if (sz == sizeof(digits)) sz--;
                digits[sz] = fmt.zeroPad;
                //zero before dot
                beforeDotDigits = BuffStr(digits+sz,1);
                //use beforedot r padding to display dot
                place4 = CharacterRep(fmt.dot,1);
                //use whole after dot place
                afterDotDigits = BuffStr(digits,sz);
                //use dot place to display padded left zeroes
                dotPlace = CharacterRep('0',-dotPos);
                //fillup r-pad if decimals has been truncated
                place6 = CharacterRep('0',origdecm - decm);
                lpadfix = -1;

            } else if (dotPos >= (integer)sz) {
                //nothing after dot
                afterDotDigits = BuffStr();

                beforeDotDigits = BuffStr(digits,sz);

                int realdecm = decm > 0? 0: decm;

                place4 = CharacterRep('0', intval == 0?1:-realdecm);

                //display dot, if decimals are requested
                dotPlace = CharacterRep(fmt.dot, origdecm>0?1:0);

                place6 = CharacterRep('0', origdecm>0?origdecm:0);
            } else {
                beforeDotDigits = BuffStr(digits,dotPos);
                afterDotDigits = BuffStr(digits+dotPos, sz - dotPos);
                place4 = CharacterRep();
                place6 = CharacterRep('0',origdecm - decm);
                dotPlace = CharacterRep(fmt.dot,1);
            }

        }

        place1.setCount(0);
        place2.setCount(0);
        natural lsize = beforeDot.size() + lpadfix;
        natural lpad = mode.beforeDotDigits == naturalNull?
                            0:(
                                lsize < mode.beforeDotDigits?
                                        mode.beforeDotDigits - lsize:0
                            );
        Ascii nsign = negVal?
                fmt.neg:(mode.plusSign?fmt.pos:fmt.spacePad);

        if (mode.zeroPad) {
                place1 = CharacterRep(nsign,1);
                place2 = CharacterRep(fmt.zeroPad,lpad);
            } else {
                place1 = CharacterRep(fmt.spacePad,lpad);
                place2 = CharacterRep(nsign,1);

            }

        iter.setParams(0,wholeNumber.size()-1,1);
    }
    template<class T>
    bool TextComposeReal<T>::hasItems() const{
        return iter.hasItems();

    }
    template<class T>
    Ascii TextComposeReal<T>::output() {
        return iter.getNext();
    }

    template<class T>
    void TextComposeReal<T>::prepareZero() {

    }
    template<class T>
    natural TextComposeReal<T>::renderNumber(lnatural numb, Ascii *buffer,
                                             natural count) {

        if (numb == 0) return 0;
        natural sz = renderNumber(numb/10,buffer,count);
        Ascii x = (Ascii)((numb%10)+'0');
        if (sz > count) return sz;
        buffer[sz] = x;
        return sz+1;

    }

    static double safeExp10(integer val) {
        if (val>0) {
            double a = 1;
            while (val--) {
                a = a * 8.0 + a * 2.0;
            }
            return a;
        } else if (val < 0) {
            double a = 1.0/safeExp10(-val);
            return a;
        } else {
            return 1.0;
        }

    }

    template<class T>
    lnatural TextComposeReal<T>::roundAt(T numb, integer pos) {
        T rval = (T)round(numb * safeExp10(pos));
        if (rval > (T)lnaturalNull) return lnaturalNull;
        else return (lnatural)(rval);
    }


}

#endif /* TEXTPARSERCOMPOSER_H_ */
