#ifndef LIGHTSPEED_SERIALIZER_BINFORMAT_H_
#define LIGHTSPEED_SERIALIZER_BINFORMAT_H_

#include "../iter/iterator.h"
#include "../types.h"
#include "formatter_concept.h"
#include "../streams/compressNumb.h"
#include "../streams/memoryStream.h"
#include "../exceptions/serializerException.h"
#include "../streams/utf.h"

namespace LightSpeed
{

	namespace BinarySerializer {
		static const byte trueVal = 1;
		static const byte falseVal = 0;
		static const byte nullSection = 0;
		static const byte beginSection = 0x55;
		static const byte endSection = 0xE5;
		static const byte beginArray = 0xAA;
		static const byte endArray = 0xEA;
	}

	template<typename Iterator>
	class BinaryWriter:public Formatter_Concept {

    public:

		typedef Iterator IteratorType ;

		BinaryWriter(IteratorType iterator):iterator(iterator) {}


		void exchange(const bool &object) {
			writeByte(object?BinarySerializer::trueVal:BinarySerializer::falseVal);
		}
		void exchange(const signed char &object) {
			writeByte((byte)object);
		}
		void exchange(const unsigned char &object) {
			writeByte((byte)object);
		}
		void exchange(const char &object) {
			writeByte((byte)object);
		}
		void exchange(const signed short &object) {
			writeSigned(object);
		}
		void exchange(const unsigned short &object) {
			writeUnsigned(object);
		}
		void exchange(const signed int &object) {
			writeSigned(object);
		}
		void exchange(const unsigned int &object) {
			writeUnsigned(object);
		}
		void exchange(const signed long &object) {
			writeSigned(object);
		}
		void exchange(const unsigned long &object) {
			writeUnsigned(object);
		}
		void exchange(const signed long long &object) {
			writeSigned(object);
		}
		void exchange(const unsigned long long &object) {
			writeUnsigned(object);
		}
		void exchange(const float &x) {
			writeBytes(reinterpret_cast<const byte *>(&x),sizeof(x));
		}
		void exchange(const double &x) {
			writeBytes(reinterpret_cast<const byte *>(&x),sizeof(x));
		}
		void exchange(const wchar_t &object) {
			WideToUtf8Filter flt;
			flt.input(object);
			while (flt.hasItems()) writeByte((byte)flt.output());
		}
            
       
		bool isStoring() const {return true;}

		bool openSection(const char *section) {return true;}

		void closeSection(const char *section) {}

		const char *openDynSection(const char *blockName) {
			writeByte(BinarySerializer::beginSection);

			if (blockName != NULL) 
			{
				for (int i = 0; blockName[i]; i++)
					writeByte((byte)blockName[i]);
				writeByte(0);
			} else 
				writeByte(BinarySerializer::nullSection);
			return blockName;
		}
        
        
		void closeDynSection(const char *blockName) {
			writeByte(BinarySerializer::endSection);
		}
               
		bool openSectionDefValue(const char *name, bool cmpResult) {
			writeByte(cmpResult?1:0);
			return cmpResult;
		}

		natural openArray(natural count) {
			writeByte(BinarySerializer::beginArray);
			writeUnsigned(count); 
			return count;
		}

		bool nextArrayItem() {return true;}

		void closeArray() {
			writeByte(BinarySerializer::endArray);
		}


	protected:
		Iterator iterator;

		void writeByte(byte b) {
			iterator.write(b);
		}

		void writeBytes(const byte *b, natural count) {
			ArrayRef<const byte> arr(b,count);
			ArrayRef<const byte>::Iterator iter = arr.getFwIter();
			iterator.copy(iter);
		}
		void writeSigned(Bin::integer64 numb) {
			byte buffer[10];
			natural count = CompressNumb::compressSigned(numb,buffer);
			writeBytes(buffer,count);

		}
		void writeUnsigned(Bin::natural64 numb) {
			byte buffer[10];
			natural count = CompressNumb::compress(numb,buffer);
			writeBytes(buffer,count);
		}
    };


	class BinReaderSyncException: public SerializerException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		BinReaderSyncException(const ProgramLocation &loc, const std::type_info &iterType)
			:SerializerException(loc),iterType(iterType) {}
		const std::type_info &getIteratorType() const {return iterType;}
	protected:
		const std::type_info &iterType;

		virtual void message(ExceptionMsg &msg) const {
			msg.string(L"Input stream out of sync reading using iterator: ")
				.cString(iterType.name());
		}

	};

	template<typename Iterator, typename Alloc = StdAlloc>
	class BinaryReader: public Formatter_Concept {

	public:

		typedef Iterator IteratorType ;

		BinaryReader(IteratorType iterator):iterator(iterator) {}


		void exchange(bool &object) {
			byte b = readByte();
			if (b == BinarySerializer::falseVal) object = false;
			else if (b == BinarySerializer::trueVal) object =  true;
			else throw BinReaderSyncException(THISLOCATION,typeid(Iterator));
		}
		void exchange(signed char &object) {
			object = (signed char)readByte();
		}
		void exchange(unsigned char &object) {
			object = (unsigned char)readByte();
		}
		void exchange(char &object) {
			object = (char)readByte();
		}
		void exchange(signed short &object) {
			object = (signed short)readSigned();
		}
		void exchange(unsigned short &object) {
			object = (unsigned short)readUnsigned();
		}
		void exchange(signed int &object) {
			object = (signed int)readSigned();
		}
		void exchange(unsigned int &object) {
			object = (unsigned int)readUnsigned();
		}
		void exchange(signed long &object) {
			object = (signed long)readSigned();
		}
		void exchange(unsigned long &object) {
			object = (unsigned long)readUnsigned();
		}
		void exchange(signed long long &object) {
			object = (signed long long)readSigned();
		}
		void exchange(unsigned long long &object) {
			object = (unsigned long long)readUnsigned();
		}
		void exchange(float &x) {
			readBytes(reinterpret_cast<byte *>(&x),sizeof(x));
		}
		void exchange(double &x) {
			readBytes(reinterpret_cast<byte *>(&x),sizeof(x));
		}
		void exchange(wchar_t &object) {
			Utf8ToWideFilter flt;
			while (!flt.hasItems()) {
				flt.input((char)readByte());
			}
			object = flt.output();
		}


		bool isStoring() const {return false;}

		bool openSection(const char *section) {return true;}

		void closeSection(const char *section) {}

		const char *openDynSection(const char *blockName) {

			//section not loaded?
			if (dynSection.empty()) {				
				if (readByte() != BinarySerializer::beginSection)
					throw BinReaderSyncException(THISLOCATION,typeid(iterator));
				//prepare stream
				MemoryStream<char,1024,MemoryStreamClusterSetup<Alloc> > stream;
				//read section name byte per byte
				byte x = readByte();
				if (x != BinarySerializer::nullSection) {
					while (x != 0) {
						stream.write((char)x);
						x = readByte();
					}
				}
				//copy section name into string
				dynSection = StringT<char,StringTraits::CmpS<char>,Alloc>(
					stream.getIterator(),stream.size());
			}
			//requested blockname is NULL?
			if (blockName == 0) {
				//if reached NULL section, return also NULL
				if (dynSection.empty()) return 0;
				//otherwise, return pointer to section name
				else return dynSection.c_str();
			} else {
				//specified name of section ... is same as found<
				if (ConstStrC(dynSection) == ConstStrC(blockName)) {
					//yes, clear the name - to mark section opened
					dynSection.clear();
					//return blockName as name of opened section
					return blockName;
				} else {
					//return NULL reporting  unexpected section
					return NULL;
				}
			}
		}


		void closeDynSection(const char *blockName) {
			if (readByte() != BinarySerializer::endSection)
				throw BinReaderSyncException(THISLOCATION,typeid(iterator));
		}


		bool openSectionDefValue(const char *name, bool cmpResult) {
			bool res;
			try {
				exchange(res);
				return res;
			} catch (Exception &e) {
				e.rethrow(THISLOCATION);
			}
		}

		natural openArray(natural count) {
			if (readByte() != BinarySerializer::beginArray)
				throw BinReaderSyncException(THISLOCATION,typeid(iterator));
			return (natural)readUnsigned();
		}

		bool nextArrayItem() {return true;}

		void closeArray() {
			if (readByte() != BinarySerializer::endArray)
				throw BinReaderSyncException(THISLOCATION,typeid(iterator));
		}

	protected:
		Iterator iterator;
		StringT<char,StringTraits::CmpS<char>,Alloc> dynSection;

		byte readByte() {
			return (byte)iterator.getNext();
		}


		void readBytes(byte *b, natural count) {
			for (natural i = 0; i < count; i++) {
				b[i] = readByte();
			}
		}
		Bin::integer64 readSigned() {					
			byte buffer[10];
			buffer[0] = readByte();
			natural len = CompressNumb::numberLength(buffer[0]);
			for (natural i = 1; i < len; i++) buffer[i] = readByte();
			return  CompressNumb::decompressSigned(buffer);
		}
		Bin::natural64 readUnsigned() {
			byte buffer[10];
			buffer[0] = readByte();
			natural len = CompressNumb::numberLength(buffer[0]);
			for (natural i = 1; i < len; i++) buffer[i] = readByte();
			return CompressNumb::decompress(buffer);
		}
	};


} // namespace LightSpeed


#endif /*BINFORMAT_H_*/
