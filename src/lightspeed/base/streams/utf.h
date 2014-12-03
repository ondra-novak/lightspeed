#ifndef LIGHTSPEED_UTF_H_
#define LIGHTSPEED_UTF_H_

#include "../types.h"
#include "../iter/iteratorFilter.h"
#include "../exceptions/utf.h"
//#include "../qualifier.h"

namespace LightSpeed
{

    
	///Filter that is fed by UTF-8 characters and returns WIDE characters
    class Utf8ToWideFilter: public IteratorFilterBase<char, wchar_t, Utf8ToWideFilter> {
    public:
        typedef IteratorFilterBase<char, wchar_t, Utf8ToWideFilter> Super;
        bool needItems() const  {return state != 0;}
        bool canAccept(const char &x) const;
        void input(const char &x);
        bool hasItems() const {return state == 0;}
        wchar_t output() 	{state = naturalNull;return (wchar_t)accum; }
        natural calcToWrite(natural srcCount) const {return srcCount/4;}
        natural calcToRead(natural trgCount) const {return trgCount*4;}
        void flush();

        Utf8ToWideFilter():accum(0),state(naturalNull),skipInvChars(false) {}
		void enableSkipInvalidChars(bool enable) {skipInvChars = enable;}
		bool isSkipInvalidCharsEnabled() const {return skipInvChars;}
    protected:
        natural accum;
        natural state;
		bool skipInvChars;
		
    };

	///Filter that is fed by WIDE characters and returns UTF-8 characters
    class WideToUtf8Filter: public IteratorFilterBase<wchar_t,char, WideToUtf8Filter> {
    public:
		
        typedef IteratorFilterBase<char, wchar_t, Utf8ToWideFilter> Super;
        bool needItems() const {     return (rdpos == sizeof(data) * sizeof(char));}
        bool canAccept(const wchar_t &x) const {return (natural)x < 0x7FFFFFFF;}
        void input(const wchar_t &x);
        bool hasItems() const {     return rdpos != sizeof(data) * sizeof(char);}
        char output() {   return data[rdpos++];}
        natural calcToWrite(natural srcCount) const {     return srcCount * 4;}
        natural calcToRead(natural trgCount) const {     return trgCount / 4;}
        void flush() {}

        WideToUtf8Filter():rdpos(sizeof(data)) {}
    protected:
        char data[8];
        natural rdpos;
    };


	class BytesToWide : public IteratorFilterBase < byte, wchar_t, BytesToWide > {
	public:
		typedef IteratorFilterBase < byte, wchar_t, BytesToWide >  Super;
		bool needItems() const { return x < 2; }
		bool hasItems() const { return x == 2; }
		void input(const byte &y) { 
			val[x++] = y; 
			if (x == 2 && val[0] >= 0xFE && val[1] >= 0xFE) {
				if (val[0] == 0xFE && val[1] == 0xFF) {
					bigEnd = true; x = 0;
				}
				else if (val[0] == 0xFF && val[1] == 0xFE) {
					bigEnd = false; x = 0;
				}
			}
		}
		wchar_t output() {
			x = 0;
			if (bigEnd) return (wchar_t)val[0] * 256 + (wchar_t)val[1];
			else return (wchar_t)val[0] + 256 * (wchar_t)val[1];
		}
		natural calcToWrite(natural srcCount) const { return srcCount / 2; }
		natural calcToRead(natural trgCount) const { return trgCount * 2; }

		BytesToWide(bool bigEnd = false) :x(0), bigEnd(bigEnd) {}
	protected:
		byte val[2];
		byte x;
		bool bigEnd;
	};

	class WideToBytes : public IteratorFilterBase < wchar_t, byte, WideToBytes > {
	public:
		typedef IteratorFilterBase < wchar_t, byte, WideToBytes > Super;
		bool needItems() const { return x == 2; }
		bool hasItems() const { return x < 2; }
		void input(const wchar_t &y) {
			if (bigEnd) {
				val[0] = (byte)(y >> 8);
				val[1] = (byte)(y & 0xFF);
			}
			else {
				val[1] = (byte)(y >> 8);
				val[0] = (byte)(y & 0xFF);
			}
			x = 0;
		}

		byte output() {
			return val[x++];
		}
		natural calcToWrite(natural srcCount) const { return srcCount * 2; }
		natural calcToRead(natural trgCount) const { return trgCount / 2; }

		WideToBytes(bool bigEnd = false) :x(2), bigEnd(bigEnd) {}
	protected:
		byte val[2];
		byte x;
		bool bigEnd;
	};


    template<typename WrIterator>
    class Utf8ToWideWriter: public FilterWrite<WrIterator, Utf8ToWideFilter> {
    public:
        Utf8ToWideWriter(WrIterator iter)
            :FilterWrite<WrIterator, Utf8ToWideFilter>(iter) {}
		void enableSkipInvalidChars(bool enable) {this->lt.enableSkipInvalidChars(enable);}
		bool isSkipInvalidCharsEnabled() const {return this->flt.isSkipInvalidCharsEnabled();}
    };

    template<typename WrIterator>
    class WideToUtf8Writer: public FilterWrite<WrIterator, WideToUtf8Filter> {
    public:
        WideToUtf8Writer(WrIterator iter)
            :FilterWrite< WrIterator, WideToUtf8Filter>(iter) {}
    };

    template<typename RdIterator>
    class Utf8ToWideReader: public FilterRead<RdIterator, Utf8ToWideFilter> {
    public:
        Utf8ToWideReader(RdIterator iter)
            :FilterRead<RdIterator, Utf8ToWideFilter>(iter) {}
		void enableSkipInvalidChars(bool enable) {this->flt.enableSkipInvalidChars(enable);}
		bool isSkipInvalidCharsEnabled() const {return this->flt.isSkipInvalidCharsEnabled();}
    };

    template<typename RdIterator>
    class WideToUtf8Reader: public FilterRead<RdIterator, WideToUtf8Filter> {
    public:
        WideToUtf8Reader(RdIterator iter)
            :FilterRead<RdIterator, WideToUtf8Filter>(iter) {}
    };

}

 // namespace LightSpeed

#endif /*UTF_H_*/
