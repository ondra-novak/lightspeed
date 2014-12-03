#pragma once
#include "../memory/refCntPtr.h"
#include "../objmanip.h"
#include "flatArray.h"
#include "constStr.h"
#include "../memory/stdAlloc.h"
#include "../memory/sharedResource.h"

namespace LightSpeed {


	template<typename T>
	class StringBufferHdrBase: public RefCntObj {
	protected:
		IRuntimeAlloc *alloc;
		natural charCount;
		natural initCount;
	};

	///This object holds string itself
	/** 
	 * Every string is object with dynamic size. It cannot be allocated by new and destroyed by delete.
	 * Object consists from header and data. This is the header, data are placed immediately after the heade.
	 *
	 * Header contains:
	 *  - reference counter
	 *  - pointer to allocator, responsible to deallocate memory
	 *  - capacity
	 *  - current string length
	 *
	 */
	template<typename T>
	class StringBufferHdr: public StringBufferHdrBase<T> {
	protected:
		///this is not alocated - useful only for peek string in the debbugger
		T stringData[1000];
	
	private:
		void *operator new(size_t);
		void *operator new(size_t,void *p) {return p;}
		void operator delete(void *,void *) {}
		void operator delete(void *);
	public:

		static StringBufferHdr *createStringBuffer(natural charCount, IRuntimeAlloc &alloc)
		{
			natural needSz = calcSize(charCount);
			IRuntimeAlloc *owner;
			StringBufferHdr *hdr = static_cast<StringBufferHdr *>(new(alloc.alloc(needSz,owner)) StringBufferHdrBase<T>);
			hdr->alloc = owner;
			hdr->charCount = charCount;
			hdr->initCount = 0;
			return hdr;
		}

		static void destroyStringBuffer(StringBufferHdr *hdr)
		{
			natural needSz=calcSize(hdr->charCount);
			IRuntimeAlloc *owner = hdr->alloc;
			hdr->~StringBufferHdr();
			owner->dealloc(hdr,needSz);
		}


		T *getStringData() {return stringData;}
		const T *getStringData() const {return stringData;}
		void destroy() {
			destruct(getStringData(),length());
			destroyStringBuffer(this);
		}
		natural length() const {return this->initCount;}
		natural capacity() const {return this->charCount;}
		IRuntimeAlloc *getAllocator() const {return this->alloc;}

		StringBufferHdr *createStringBuffer(natural charCount)
		{
			return createStringBuffer(charCount,*this->alloc);
		}

		static natural calcSize( natural charCount ) 
		{
			return sizeof(StringBufferHdrBase<T>)+charCount * sizeof(T);
		}

		StringBufferHdr *resizeStringBuffer(natural newCharCount) {
			if (newCharCount < this->initCount) newCharCount = this->initCount;
			StringBufferHdr *newBuff = createStringBuffer(newCharCount);
			copy(getStringData(),length(),newBuff->getStringData());
			newBuff->initCount = this->initCount;
			return newBuff;
		}

		StringBufferHdr *copyString() {
			StringBufferHdr *newBuff = createStringBuffer(this->initCount);
			copy(getStringData(),this->initCount,newBuff->getStringData());
			newBuff->initCount = this->initCount;
			return newBuff;
		}

		natural add(const T *data, natural count) {
			if (count + this->initCount > this->charCount) count = this->charCount - this->initCount;
			copy(data,count,getStringData()+this->initCount);
			this->initCount+=count;
			return count;
		}
		natural add(const T &data) {
			if (this->initCount >= this->charCount) return 0;
			copy(&data,getStringData()+this->initCount);
			this->initCount+=1;
			return 1;
		}
		natural init(natural count, const T &item) {
			if (count + this->initCount > this->charCount) count = this->charCount - this->initCount;
			construct(getStringData()+this->initCount,count,item);
			this->initCount+=count;
			return count;
		}

		natural init(natural count) {
			if (count + this->initCount > this->charCount) count = this->charCount - this->initCount;
			construct<T>(getStringData()+this->initCount,count);
			this->initCount+=count;
			return count;
		}

		void init() {
			init(this->charCount - this->initCount);
		}
	};

	///Responsible to destroy string buffer through RefCntPtr
	template<typename T>
	class StringBufferHdrDestroyer {
	public:
		static void destroyInstance(StringBufferHdr<T> *hdr) {
			if (hdr) hdr->destroy();
		}
	};

	///Extended pointer to string buffer
	template<typename T>
	class PStringBufferHdr: public RefCntPtr<StringBufferHdr<T>,StringBufferHdrDestroyer<T> > {
	public:
		typedef RefCntPtr<StringBufferHdr<T>,StringBufferHdrDestroyer<T> > Super;
		PStringBufferHdr(natural charCount, IRuntimeAlloc &alloc)
			:Super(StringBufferHdr<T>::createStringBuffer(charCount,alloc)) {}
		PStringBufferHdr() {}
		PStringBufferHdr(const PStringBufferHdr &other):Super(other) {}
		PStringBufferHdr(StringBufferHdr<T> *p):Super(p) {}

		natural resize(natural newCharCount) {
			if ((*this) == nil) return 0;
			(*this) = PStringBufferHdr((*this)->resizeStringBuffer(newCharCount));
			return (*this)->length();
		}

		void isolate() const {
			const_cast<PStringBufferHdr *>(this)->isolate();
		}
		void isolate() {
			if (this->isShared()) (*this) = PStringBufferHdr((*this)->copyString());
		}

		natural length() const {return (*this) == nil?0:(*this)->length();}
		natural capacity() const {return (*this) == nil?0:(*this)->capacity();}
		IRuntimeAlloc *getAllocator() const {return (*this) == nil?0:(*this)->getAllocator();}
		natural add(const T *data, natural count) {return (*this)->add(data,count);}
		natural add(const T &data) {return (*this)->add(&data);}

		PStringBufferHdr &operator=(const RefCntPtr<T> &other) {
			Super::operator =(other);return *this;
		}
		PStringBufferHdr &operator=(NullType) {
			Super::clear();
			return *this;
		}
		PStringBufferHdr getMT() const {
			return PStringBufferHdr(Super::getMT().get());
		}

		void init(natural count, const T &item) {(*this)->init(count,item);}
		void init(natural count) {(*this)->init(count);}
		void init() {(*this)->init();}

	};

	///Retrieves default string allocator
	IRuntimeAlloc &getStringDefaultAllocator();

	void setStringDefaultAllocator(IRuntimeAlloc &alloc);

	template<typename T>
	class StringCore: public FlatArrayBase<const T, StringCore<T> > {
	public:

		typedef FlatArray<const T, StringCore<T> > Super; 

		class WriteIterator: public SharedResource ,
						public WriteIteratorBase<T, WriteIterator>{
		public:

			WriteIterator(PStringBufferHdr<T> p):p(p) {}
			bool hasItems() const {
				return p->length() < (p->capacity() - (StringBase<T>::needZeroChar?1:0));
			}

			void write(const T &itm) {
				p->add(itm);
			}

			~WriteIterator() {
				if (!SharedResource::isShared() && StringBase<T>::needZeroChar) 
					StringBase<T>::writeZeroChar(*this);
			}

			natural getRemain() const {
				return p->capacity() - p->length() - (StringBase<T>::needZeroChar?1:0);
			}
						
			template<typename Traits>
			natural blockWrite(const FlatArray<T,Traits> &arr, bool writeAll) {
				return blockWriteImpl(arr,writeAll);
			}
			template<typename Traits>
			natural blockWrite(const FlatArray<const T,Traits> &arr, bool writeAll) {
				return blockWriteImpl(arr,writeAll);
			}

		protected:
			PStringBufferHdr<T> p;
		private:
			void operator=(WriteIterator &other);

			template<typename X, typename Traits>
			natural blockWriteImpl(const FlatArray<X,Traits> &arr, bool writeAll) {
				natural l = arr.length();
				const X *d = arr.data();
				if (l > getRemain()) {
					if (writeAll)
							throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
					else
						l = getRemain();
				}
				natural k = p->add(d,l);
				return k;
			}
};

		StringCore() {}

		///Creates empty string (initialized by nil)
		StringCore(NullType) {}

		StringCore(const T *text)
			{loadArray(ConstStringT<T>(text),getStringDefaultAllocator());}

		explicit StringCore(const T *text, IRuntimeAlloc &alloc)
			{loadArray(ConstStringT<T>(text),alloc);}

		StringCore(ConstStringT<T> text) {loadArray(text,getStringDefaultAllocator());}

        StringCore(ConstStringT<T> text, IRuntimeAlloc &alloc)
			 {loadArray(text,alloc);}

		template<typename A, typename B>
		StringCore(const ArrayT<A,B> &arr, IRuntimeAlloc &alloc)
				{loadArray(arr,alloc);}

		template<typename A, typename B>
		StringCore(const ArrayT<A,B> &arr)
		{loadArray(arr,getStringDefaultAllocator());}

		StringCore(const PStringBufferHdr<T> &x):strBuff(x) {}


        operator const ConstStringT<T>() const{
            return  ConstStringT<T>(this->data(),this->length());
        }

		const T *c_str() const {
			if (this->empty()) return StringBase<T>::getEmptyString();
			else return data();
		}

		const T *cStr() const {
			if (this->empty()) return StringBase<T>::getEmptyString();
			else return data();
		}

		WriteIterator createBufferIter(natural count) {
			PStringBufferHdr<T> buff(StringBase<T>::stringLen(count),getStringDefaultAllocator());
			strBuff = buff;
			return WriteIterator(buff);
		}
		WriteIterator createBufferIter(natural count, IRuntimeAlloc &alloc) {
			PStringBufferHdr<T> buff(StringBase<T>::stringLen(count),alloc);
			strBuff = buff;
			return WriteIterator(buff);
		}

		T *createBuffer(natural count) {
			WriteIterator iter = createBufferIter(count);
			strBuff->init(count);
			return data();
		}

		T *createBuffer(natural count, IRuntimeAlloc &alloc) {
			WriteIterator iter = createBufferIter(count,alloc);
			strBuff->init(count);
			return data();
		}

		natural length() const {
			if (strBuff == nil ) return 0;
			else return StringBase<T>::originalLen(strBuff.length());
		}

		const T *data() const {return strBuff->getStringData();}
		T *data() {
			if (strBuff == nil) return 0;
			isolate();
			return strBuff->getStringData();
		}

		const typename Super::ItemT *refData(const typename Super::ItemT *) const {
			return data();
		}

		void clear() {
			strBuff = nil;
		}

		IRuntimeAlloc &getAllocator() const {
			IRuntimeAlloc *k = strBuff.getAllocator();
			if (!k) k = &getStringDefaultAllocator();
			return *k;
		}

		///Isolates string
		/** Because strings can be shared sometimes sharing is not
		 * wanted. To ensure, that string will not shared, call isolate().
		 * Function create copy of the string, when original string is shared,
		 * and replaces original string with the copy.
		 * Then current string contains own copy without sharing.
		 *
		 * Note that function doesn't create copy of string, which is
		 * already isolated (and alone).		
		 */

		void isolate() const {
			if (strBuff != nil) {
				strBuff.isolate();
			}
		}

		///Creates copy of the string and returns it as result
		/**
		 * By default, strings are shared, and creating copy of string
		 * using the standard copy constructor causes, that only pointer
		 * will be copied, and string data is shared. To perform full 
		 * copying, use copy() method. Copying can be very useful, when you
		 * passing string to the thread, because sharing is not MT safe.
		 * 
		 *
		 * @return Object contains copied string
		 *
		 * @note in contrast to isolate(), function alwais creates the
		 * copy of the string
		 */

		StringCore copy() const  {
			StringCore res = *this;
			res.isolate();
			return res;
		}

		///Retrieves string which can be shared between threads
		/** Strings are not thread safe until function getMT is used.
		  * Result of call is String object which can be accessed from
		  * the different thread.
		  *
		  * Function enables multithread-safe sharing for the object.
		  * Function should not allocate any extra memory for this. You
		  * should use this function everytime you want to pass string
		  * into different thread, for example into worker thread or
		  * parallel executor. (This is not necessary when string is
		  * member variable and is accessed from worker thread while
		  * main thread is not accessing it).
		  *
		  */

		StringCore getMT() const {
			return StringCore(strBuff.getMT());
		}

	protected:

		///Loads array into the string
		/**
		 * @param arr array containing the data
		 * @param alloc allocator used to allocate the memory
		 */
		template<typename A, typename B>
		void loadArray(const ArrayT<A,B> &arr, IRuntimeAlloc &alloc) {
			WriteIterator iter = createBufferIter(arr.length(),alloc);
			arr.render(iter);
		}

	protected:
		PStringBufferHdr<T> strBuff;


	};


}
