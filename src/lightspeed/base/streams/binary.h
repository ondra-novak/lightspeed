#ifndef LIGHTSPEED_STREAMS_BINARY_H_
#define LIGHTSPEED_STREAMS_BINARY_H_

#include "../types.h"
#include "../iterator.h"
#include "../exceptions/iterator.h"
#include "../qualifier.h"
#include "multibyte.h"

namespace LightSpeed {
    
    
    template<class WrIterator, /* = IWriteIterator<byte> */
             MultibyteOrder::Enum mode = MultibyteOrder::curEndian>
    class BinaryWriter 
    {
        
        typedef typename DeRefType<WrIterator>::T::ItemT ItemT;
        typedef typename DeRefType<WrIterator>::T &IterRef;
        
    public:
        
        BinaryWriter(WrIterator iter):iter(iter),wrcount(0) {}
        
        /// Writes natural number
        /** @param x number to write. Number in the name of function
         * specifies count of bytes to be written. It can be 8,16,32 or 64 bytes
         */
        void nat8(Bin::natural8 x);
        /// Writes natural number
        /** @copydoc nat8 */
        void nat16(Bin::natural16 x);
        /// Writes natural number
        /** @copydoc nat8 */
        void nat32(Bin::natural32 x);
        /// Writes natural number
        /** @copydoc nat8 */
        void nat64(Bin::natural64 x);

        /// Writes integer number
        /** @param x number to write. Number in the name of function
         * specifies count of bytes to be written. It can be 8,16,32 or 64 bytes
         */
        void int8(Bin::integer8 x);
        /// Writes integer number
        /**@copydoc int8 */
        void int16(Bin::integer16 x);
        /// Writes integer number
        /**@copydoc int8 */
        void int32(Bin::integer32 x);
        /// Writes integer number
        /**@copydoc int8 */
        void int64(Bin::integer64 x);
        
        ///Writes floating point (real) number
        /** @param x number to write. Number in the name of function
         * specifies count of bytes to be written. It can be 32 or 64 bytes.
         * 
         * @note In network transfer, there is no standard to transfer floating
         * numbers. They are written as it.
         */
        void real32(Bin::real32 x);
        ///Writes floating point (real) number
        /**@copydoc real32 */
        void real64(Bin::real64 x);

        ///Writes single character as one bute
        /** @param x character to write */
        void ascii(char x);
        ///Writes wide character as UCS-16 (UTF-16). It takes 2 bytes
        /** @param x character to write */
        void wide16(wchar_t x);
        ///Writes wide character as UCS-32 (UTF-32). It takes 4 bytes
        /** @param x character to write */
        void wide32(wchar_t x);
        ///Writes wide character as it is represented in current platform.        
        /** @param x character to write */
        void wide(wchar_t x);
        ///Writes boolean variable (takes 1 byte)         
        /** @param x value to write */
        void boolean(bool x);
        ///Writes extended logic state (takes 1 byte)         
        /** @param x value to write */
        void logic(LogicState x);
        ///Writes sequence of bytes read from address and have specified size
        /**
         * @param ptr address
         * @param size size in the bytes
         * 
         * @note Function is not using block transfer. If you wish
         * to use block transfer, you must connect this writter with
         * buffer, that handles this operation. Also note that
         * writting is fully blocking, so program may stall during
         * writting until full buffer is not accepted by other side
         */
        
        void block(const void *ptr, natural size);
        ///Writes compressed natural number
        void natCompress(natural x);
        ///Writes compressed integer number
        void intCompress(integer x);
        
        ///Retrieves count of bytes processed by last operation
        /** Contain count of bytes written to the stream by
         * last write operation. You can use this function 
         * in cooperation with buffer to implement unwrite operation
         * 
         * @see WriteBuffer::unwrite
         */
        
        natural getWrCnt() const {
            return wrcount;
        }
        
    protected:
        WrIterator iter;
        natural wrcount;
        
        template<class T>
        void writeItem(T x);    
        template<class T>
        void writeDirect(T x);    
        
    };
    
    template<class RdIterator, /* = IReadIterator<byte> */
             MultibyteOrder::Enum mode = MultibyteOrder::curEndian>
    class BinaryReader {
        
        typedef typename DeRefType<RdIterator>::T::ItemT ItemT;
        typedef typename DeRefType<RdIterator>::T &IterRef;
        
    public:
        
        BinaryReader(RdIterator iter):iter(iter) {}
        
        void nat8(Bin::natural8 &x);
        void nat16(Bin::natural16 &x);
        void nat32(Bin::natural32 &x);
        void nat64(Bin::natural64 &x);
        void int8(Bin::integer8 &x);
        void int16(Bin::integer16 &x);
        void int32(Bin::integer32 &x);
        void int64(Bin::integer64 &x);
        void real32(Bin::real32 &x);
        void real64(Bin::real64 &x);
        void ascii(char &x);
        void wide16(wchar_t &x);
        void wide32(wchar_t &x);
        void wide(wchar_t &x);
        void boolean(bool &x);
        void logic(LogicState &x);
        void block(void *ptr, natural size);
        void natCompress(natural &x);
        void intCompress(integer &x);

        Bin::natural8 nat8();
        Bin::natural16 nat16();
        Bin::natural32 nat32();
        Bin::natural64 nat64();
        Bin::integer8 int8();
        Bin::integer16 int16();
        Bin::integer32 int32();
        Bin::integer64 int64();
        Bin::real32 real32();
        Bin::real64 real64();
        char ascii();
        wchar_t wide16();
        wchar_t wide32();
        wchar_t wide();
        bool boolean();
        LogicState logic();
        natural natCompress();
        integer intCopress();
        
        ///Retrieves count of bytes processed by last operation
        /** Contain count of bytes read from the stream by
         * last read operation. You can use this function 
         * in cooperation with buffer to implement unget operation
         * 
         * @see WriteBuffer::unget
         */

        natural getRdCnt() const {
            return rdcount;
        }
        
        
    protected:
        RdIterator iter;
        natural rdcount;

        
        template<class T>
        void readItem(T &x);    
        template<class T>
        void readDirect(T &x);    
    
    };
    
    //------------------ IMPLEMENTATION -------------------------
    
    template<class WrIterator, MultibyteOrder::Enum mode>
    template<class T>
    void BinaryWriter<WrIterator,mode>::writeItem(T x) {
        MultibyteOrder::Value<T, mode> val;
        val.setValue(x);
        wrcount=0;
        for (natural i = 0 ; i < sizeof(T); i++) {
            iter.write(ItemT(val.getByte(i)));
            wrcount++;
        }
    }

    template<class RdIterator, MultibyteOrder::Enum mode>
    template<class T>
    void BinaryReader<RdIterator,mode>::readItem(T &x) {
        MultibyteOrder::Value<T, mode> val;
        rdcount=0;
        for (natural i = 0 ; i < sizeof(T); i++) {
            val.setByte(i,iter.getNext());
            rdcount++;
        }
        x = val.getValue();
    }
    
    template<class WrIterator, MultibyteOrder::Enum mode>
    template<class T>
    void BinaryWriter<WrIterator,mode>::writeDirect(T x) {
        block(&x,sizeof(T));
    }

    template<class RdIterator, MultibyteOrder::Enum mode>
    template<class T>
    void BinaryReader<RdIterator,mode>::readDirect(T &x) {
        block(&x,sizeof(T));
    }

    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::nat8(Bin::natural8 x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::nat16(Bin::natural16 x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::nat32(Bin::natural32 x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::nat64(Bin::natural64 x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::int8(Bin::integer8 x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::int16(Bin::integer16 x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::int32(Bin::integer32 x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::int64(Bin::integer64 x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::real32(float x) {writeDirect(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::real64(double x) {writeDirect(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::ascii(char x)  {writeDirect(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::wide16(wchar_t x)  {
        Bin::natural16 a = (Bin::natural16)x;
        writeItem(a);
    }
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::wide32(wchar_t x) {
        Bin::natural32 a = (Bin::natural32)x;
        writeItem(a);
    }
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::wide(wchar_t x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::boolean(bool x) {writeItem(x);}
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::logic(LogicState x)  {
        Bin::natural8 a = (Bin::natural8)x;
        writeItem(a);
    }
    template<class WrIterator, MultibyteOrder::Enum mode>
    void BinaryWriter<WrIterator,mode>::block(const void *ptr, natural size) {
        const byte *x = reinterpret_cast<const byte *>(ptr);
        wrcount = 0;
        while (size--) {
            iter.write(*x++);
            wrcount++;
        }
        
    }

    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::nat8(Bin::natural8 &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::nat16(Bin::natural16 &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::nat32(Bin::natural32 &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::nat64(Bin::natural64 &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::int8(Bin::integer8 &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::int16(Bin::integer16 &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::int32(Bin::integer32 &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::int64(Bin::integer64 &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::real32(float &x) {readDirect(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::real64(double &x) {readDirect(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::ascii(char &x)  {readDirect(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::wide16(wchar_t &x) {
        Bin::natural16 a; readItem(a);x = (wchar_t)a;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::wide32(wchar_t &x) {
        Bin::natural32 a; readItem(a);x = (wchar_t)a;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::wide(wchar_t &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::boolean(bool &x) {readItem(x);}
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::logic(LogicState &x) {
        Bin::natural8 a; readItem(a);x = (LogicState)a;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    void BinaryReader<RdIterator,mode>::block(void *ptr, natural size) {
        const byte *x = reinterpret_cast<const byte *>(ptr);
        rdcount=0;
        while (size--) {
            *x++=iter.getNext();
            rdcount++;
        }
    }

    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::natural8 BinaryReader<RdIterator,mode>::nat8() {
        Bin::natural8 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::natural16 BinaryReader<RdIterator,mode>::nat16() {
        Bin::natural16 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::natural32 BinaryReader<RdIterator,mode>::nat32() {
        Bin::natural32 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::natural64 BinaryReader<RdIterator,mode>::nat64() {
        Bin::natural64 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::integer8 BinaryReader<RdIterator,mode>::int8() {
        Bin::integer8 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::integer16 BinaryReader<RdIterator,mode>::int16() {
        Bin::integer16 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::integer32 BinaryReader<RdIterator,mode>::int32() {
        Bin::integer32 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::integer64 BinaryReader<RdIterator,mode>::int64() {
        Bin::integer64 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::real32 BinaryReader<RdIterator,mode>::real32() {
        Bin::real32 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    Bin::real64 BinaryReader<RdIterator,mode>::real64() {
        Bin::real64 x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    char BinaryReader<RdIterator,mode>::ascii()  {
        char x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    wchar_t BinaryReader<RdIterator,mode>::wide16()  {
        wchar_t x;wide16(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    wchar_t BinaryReader<RdIterator,mode>::wide32() {
        wchar_t x;wide32(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    wchar_t BinaryReader<RdIterator,mode>::wide() {
        wchar_t x;wide(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    bool BinaryReader<RdIterator,mode>::boolean() {
        bool x;readItem(x);return x;
    }
    template<class RdIterator, MultibyteOrder::Enum mode>
    LogicState BinaryReader<RdIterator,mode>::logic() {
        LogicState x;logic(x);return x;
    }
}


#endif /*BINARY_H_*/
