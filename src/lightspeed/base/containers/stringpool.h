#include "flatArray.h"
#include "constStr.h"
#include "../memory/stdAlloc.h"
#include "autoArray.h"
#pragma once

namespace LightSpeed {



///Universal string reference
/** Can be used instead String in the map. Itself can be
 * created from ConstStringT<T>, which refers parameter given in the
 * constructor. Reference is valid until referred string exists. To create
 * reference with copying string into the pool use add() method
 *
 * @note object takes 12 bytes under 32-bit and 16 bytes under 64-bit. In both environments
 * string pool can hold strings of total 4294967296 characters. Especially under 64-bit environment this limit can
 * be easy reached. If this is the problem, use multiple pools.


 */	
	extern void *ptrStringPoolNull;

	template<typename T>
	class StringPoolStrRef: public FlatArrayBase<const T, StringPoolStrRef<T> >  {
	public:
		///Constructs string reference from ConstStringT object
		/**
		 * @param str reference to ConstStringT object. Note that original ConstStringT object must
		 * remain valid for whole lifetime of this reference. New object makes reference to internal ConstStringT
		 * pointer and sets offset and length. Result is object that looks like allocated in string-pool. But neither
		 * allocation nor copying is necessary
		 */
		StringPoolStrRef(const ConstStringT<T> &str):anchor(getPtrToData(str)),offset(0),len(Bin::natural32(str.length()))  {}

		///Constructs string from string pool
		/**
		 * @param anchor pointer to pointer to begin of pool. Because location of pool can change during allocations
		 * 	reference keeps pointer to variable that contains address of the pool
		 * @param offset offset in the pool
		 * @param len length of the string
		 */
		StringPoolStrRef(const T *const *anchor, Bin::natural32 offset, Bin::natural32  len)
			:anchor(anchor),offset(offset),len(len) {}
		StringPoolStrRef():anchor(reinterpret_cast<const T *const *>(&ptrStringPoolNull)),offset(0),len(0) {}
		const T *data() const {return *anchor + offset;}
		natural length() const {return len;}
		void clear() { len = 0; }
		bool empty() const { return len == 0; }

	protected:
		const T *const * anchor;
		Bin::natural32 offset;
		Bin::natural32 len;	


		static const T *const * getPtrToData(const ConstStringT<T> &x) {

			class X: public ConstStringT<T> {
			public:
				const T *const * getPtrRef() const {return &this->refdata;}
			};
			return static_cast<const X &>(x).getPtrRef();
		}
	};
	///StringPool object is useful when you working with many constant strings
	/**
	 * Right use of this object is to allocate and keep strings in the memory.
	 * The most common way to solve this situation is to allocate String object
	 * to every single string. This can cause memory fragmentation and slow down
	 * loading and accessing strings. Especially when strings are used in the search
	 * maps, every search need creation of String object, because you need
	 * object of the same type for comparison
	 *
	 * To use StringPool, declare this object with a search map. Use StringPool::Str
	 * type for string reference in the map (can be key or value as you wish). Str class
	 * can be created from ConstStringT, which creates constant reference to the string.
	 * This is useful when you searching the string. To store string, use method add()
	 * or simply operator() to convert ConstStringT to Str object. String is copied
	 * into the pool and reference to it is returned as result.
	 *
	 * There is no way to remove strings from the pool, because they are allocated
	 * incrementally. You can copy all live strings to second pool. There is also two methods mark() and
	 * release() useful to deallocate temporary strings
	 *
	 *
	 */
	template<typename T, typename Alloc = StdAlloc>
	class StringPool {
	public:

		typedef StringPoolStrRef<T> Str;
		///Adds new string into the pool
		/**
		 * @param str new string
		 * @return reference to the string as universal string reference
		 */
		Str add(ConstStringT<T> str);

		///Removes all strings.
		/** ensure that there is no reference into the pool before invoking the command */
		void clear();

		///Releases extra allocated memory
		/** Function uses all possible ways to decrease memory usage which includes allocating new memory block, copying
		 * context and deallocate old larger block. This can be slow. Use only if you really need to free some memory.
		 */
		void freeExtra();

		///Adds new string into the pool
		/**
		 * @param str new string
		 * @return reference to the string as universal string reference
		 */
		Str operator()(ConstStringT<T> str);

		///Marks state of string heap.
		/**
		 * @return function returns current size of the pool. Returned value can be used in function release()
		 */
		natural mark();
		///Releases memory allocated for strings stored after mark()
		/**
		 * @param mark value returned by mark()
		 *
		 * @note Use function only if there is no string reference created after mark(). Otherwise accessing these
		 * references can cause unpredictable results;
		 *
		 * @note Memory after release() is not returned to the allocator. It is still ready to additional allocations
		 * in the future. To shrink allocated memory, use freeExtra()
		 */
		void release(natural mark);

		///Creates reference to string which did start on marked position
		/** You can use mark() to markdown top of pool and then start to
		 * load a text into buffer per-parted using add() method. When you
		 * are finish, use fromMark() function to create refernce to such text
		 *
		 * @param mark
		 * @return
		 */
		Str fromMark(natural mark);

		template<typename X>
		Str add(IIterator<T,X> &iter);

		template<typename X>
		Str add(const IIterator<T,X> &iter);

		class WriteIterator: public WriteIteratorBase<T, WriteIterator> {
		public:
			WriteIterator(StringPool &owner):owner(owner),startMark(owner.pool.length()) {}
			bool hasItems() const {return true;}
			void write(const T &x) {owner.pool.add(x);}
			Str finish();

		protected:
			StringPool &owner;
			natural startMark;

		};

		WriteIterator getWriteIterator() {
			return WriteIterator(*this);
		}

	protected:
		AutoArray<T, Alloc> pool;
		const T * poolRef;
	};


	typedef StringPool<char> StringPoolA;
	typedef StringPool<wchar_t> StringPoolW;




}


