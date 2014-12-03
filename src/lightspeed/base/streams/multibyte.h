#ifndef LIGHTSPEED_STREAMS_MULTIBYTE_H_
#define LIGHTSPEED_STREAMS_MULTIBYTE_H_

#include "../containers/buffer.h"
#include "../iter/iteratorFilter.h"

namespace LightSpeed {

    ///Multibyte ordering
    namespace MultibyteOrder {
        ///enumeration
        enum Enum {
            ///current endian will be used, class will detect it by calling detect() function
            curEndian = 0,
            ///data will be interpreted as little endian
            littleEndian = 1,
            ///data will be interpreted as big endian
            bigEndian = 2,
        };
        
        ///Detects multibyte ordering
        /**
         * @return littleEndian Ordering is litleEndian
         * @return bigEndian Ordering is bigEndian
         */
        static inline Enum detect() {
            //create variable
            natural val = 0;
            //set first byte of variable 
            byte *p = reinterpret_cast<byte *>(&val);
            //to value 1
            *p = 1;
            //if the multibyte result is 1, we have littleEndian ordering
            if (val == 1) return littleEndian;
            //otherwise, we have bigEndian ordering
            else return bigEndian;
        }


        ///Allows manipulate with bytes of multibyte value depend on order mode
        /** 
         * @param T multibyte type, should be basic type, such a int or natural
         * @param mode multibyte ordering mode
         */
        template<class T, MultibyteOrder::Enum mode = MultibyteOrder::curEndian>
        class Value {
        public:
            Value():swapOrder(mode != MultibyteOrder::curEndian 
                                 && mode != detect()) {}
            
            ///Sets the byte of multibyte value
            /** 
             * @param index natural m index specifies index of byte, meaning depends on
             *  mode teplate parameter
             * @param val new value of this byte
             */
            void setByte(natural index, byte val) {
                byte *x = reinterpret_cast<byte *>(&value);
                x[swapOrder?(sizeof(T) - index - 1):index] = val;
            }
            
            ///Gets the byte of multibyte value
            /** 
             * @param index specifies index of byte, meaning depends on
             *  mode teplate parameter
             * @return value of this byte
             */
            byte getByte(natural index) const {
                const byte *x = reinterpret_cast<const byte *>(&value);
                return x[swapOrder?(sizeof(T) - index - 1):index];                
            }
            
            ///sets value
            void setValue(const T &v) {value = v;}
            ///gets value
            const T &getValue() const {return value;} 
            
        protected:
            T value;
            bool swapOrder;
            
        };
        
    }
    

    ///Implements filter that reads bytes and builds multibyte value
    /**
     * @param MBType multibyte type to build from the bytes
     * @param mode order mode, it can be curEndian, littleEndian and bigEndian
     */
    template<typename MBType = natural,
            MultibyteOrder::Enum mode = MultibyteOrder::curEndian>
    class BytesToMultibyte:
        public IteratorFilterBase<byte,MBType,BytesToMultibyte<MBType,mode> >
    {
    public:
        bool needItems() const {
            return pos < sizeof(MBType);
        }
        void input(const byte &x) {
            if (!needItems())
                throw WriteIteratorNoSpace(THISLOCATION,typeid(byte));
            value.setByte(pos++,x);
        }
        bool hasItems() const {
            return pos == sizeof(MBType);
        }
        MBType output() {
            if (!hasItems())
                throw IteratorNoMoreItems(THISLOCATION,typeid(MBType));
            pos = 0;
            return value.getValue();

        }
        BytesToMultibyte():pos(0) {}
    protected:
        MultibyteOrder::Value<MBType,mode> value;
        natural pos;
    };

    ///Implements filter that reads multibyte value and extracts it to bytes
    /**
     * @param MBType multibyte type to build from the bytes
     * @param mode order mode, it can be curEndian, littleEndian and bigEndian
     */
    template<typename MBType = natural,
            MultibyteOrder::Enum mode = MultibyteOrder::curEndian>
    class MultibyteToBytes:
        public IteratorFilterBase<byte,MBType,MultibyteToBytes<MBType,mode> >
    {
    public:
        bool needItems() const {
            return pos == sizeof(MBType);
        }
        void input(const MBType &x) {
            if (!needItems())
                throw WriteIteratorNoSpace(THISLOCATION,typeid(MBType));
            value.setValue(pos++,x);
            pos = 0;
        }
        bool hasItems() const {
            return pos < sizeof(MBType);
        }
        MBType output() {
            if (!hasItems())
                throw IteratorNoMoreItems(THISLOCATION,typeid(byte));
            return value.getByte(pos++);

        }
        MultibyteToBytes():pos(sizeof(MBType)) {}
    protected:
        MultibyteOrder::Value<MBType,mode> value;
        natural pos;
    };


    ///Implements iterator able to read bytes and return multibyte values
    template<typename Iterator,
            typename MBType = natural,
            MultibyteOrder::Enum mode = MultibyteOrder::curEndian>
    class BytesToMBReader:
        public Filter<BytesToMultibyte<MBType,mode> >::Read<Iterator>
    {
    public:
        typedef typename Filter<BytesToMultibyte<MBType,mode> >::Read<Iterator> Super;
        BytesToMBReader(Iterator iter):Super(iter) {}
    };

    ///Implements iterator able to write bytes to a multibyte output
    template<typename Iterator,
            typename MBType = natural,
            MultibyteOrder::Enum mode = MultibyteOrder::curEndian>
    class BytesToMBWriter:
        public Filter<BytesToMultibyte<MBType,mode> >::Writer<Iterator>
    {
    public:
        typedef typename Filter<BytesToMultibyte<MBType,mode> >::Writer<Iterator> Super;
        BytesToMBWriter(Iterator iter):Super(iter) {}
    };


    ///Implements iterator able to read multibyte input and return bytes
    template<typename Iterator,
            typename MBType = natural,
            MultibyteOrder::Enum mode = MultibyteOrder::curEndian>
    class MBToBytesReader:
        public Filter<MultibyteToBytes<MBType,mode> >::Read<Iterator>
    {
    public:
        typedef typename Filter<MultibyteToBytes<MBType,mode> >::Read<Iterator> Super;
        MBToBytesReader(Iterator iter):Super(iter) {}
    };

    ///Implements iterator able to write multibyte values and separate it to bytes on output
    template<typename Iterator,
            typename MBType = natural,
            MultibyteOrder::Enum mode = MultibyteOrder::curEndian>
    class MBToBytesWriter:
        public Filter<MultibyteToBytes<MBType,mode> >::Writer<Iterator>
    {
    public:
        typedef typename Filter<MultibyteToBytes<MBType,mode> >::Writer<Iterator> Super;
        MBToBytesWriter(Iterator iter):Super(iter) {}
    };
} // namespace LightSpeed


#endif /*MULTIBYTE_H_*/
