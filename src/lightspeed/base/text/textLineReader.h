#pragma once


#include "../containers/autoArray.h"
#include "../iter/iterator.h"


namespace LightSpeed {


	///Iterator reads input stream (text, chars, wide chars) and enumerates lines.
	/**
	  @tparam Iterator source iterator. Iterator is copied into object, 
	            if you need to avoid copying, use Iterator & (with ampersant as reference)
	  @tparam Allocator allocator that will be used to allocate buffer to store lines. Default is StdAlloc, which 
	            allows nearly unlimited lines. You can limit lines using different allocator, such a StaticAlloc. If the
				line is longer than secified, exception is thrown. Allocator is used everytime buffer need expansion. Otherwise
				object reuses already allocated memory to reduce allocation overhead
	*/
	template <typename Iterator, typename Allocator = StdAlloc>
	class TextLineReader : public IteratorBase<ConstStringT<typename ConstObject<typename OriginT<Iterator>::T::ItemT>::Remove>, TextLineReader<Iterator, Allocator> > {	
	public:
		typedef typename ConstObject<typename OriginT<Iterator>::T::ItemT>::Remove T;

		///Construct object from given iterator
		/**
		 @param iter iterator to read. Constructor will initialize default new line separator for given type. See DefaultNLString class.

		   - under linux, it will be '\n' under windows '\r\n'
		 */
		TextLineReader(Iterator iter) :iter(iter), sep(DefaultNLString<T>()),needFetch(true) {}
		///Construct object from given iterator. You can specify own new line separator.
		TextLineReader(Iterator iter, ConstStringT<T> sep) :iter(iter), sep(sep), needFetch(true) {}
		///Construct object from given iterator and also initialize buffer with the allocator
		TextLineReader(Iterator iter, Allocator alloc) :iter(iter), sep(DefaultNLString<T>()), buffer(alloc), needFetch(true) {}
		///Construct object from given iterator and also initialize buffer with the allocator. You can specify own new line separator.
		TextLineReader(Iterator iter, ConstStringT<T> sep, Allocator alloc) :iter(iter), sep(sep), buffer(alloc), needFetch(true) {}

		const ConstStringT<T> &peek() const;
		const ConstStringT<T> &getNext();
		bool hasItems() const;

	protected:
		mutable Iterator iter;
		mutable ConstStringT<T> tmp;
		mutable AutoArray<T, Allocator> buffer;
		mutable bool needFetch;

		const ConstStringT<T> sep;

		bool fetchLine() const;


	};

	template <typename Iterator, typename Allocator /*= StdAlloc*/>
	bool LightSpeed::TextLineReader<Iterator, Allocator>::fetchLine() const
	{
		if (needFetch) {			
			if (iter.hasItems()) {
				buffer.clear();
				do {
					const T &ch = iter.getNext();
					buffer.add(ch);
					if (buffer.tail(sep.length()) == sep) {
						buffer.trunc(sep.length());
						break;
					}
				} while (iter.hasItems());
				needFetch = false;
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return true;
		}
	}

	template <typename Iterator, typename Allocator >
	bool LightSpeed::TextLineReader<Iterator, Allocator>::hasItems() const
	{
		return fetchLine();
	}

	template <typename Iterator, typename Allocator >
	const ConstStringT<typename TextLineReader<Iterator, Allocator>::T> & LightSpeed::TextLineReader<Iterator, Allocator>::getNext()
	{
		if (!fetchLine()) throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
		tmp = buffer;
		needFetch = true;
		return tmp;

	}

	template <typename Iterator, typename Allocator >
	const ConstStringT<typename TextLineReader<Iterator, Allocator>::T> & LightSpeed::TextLineReader<Iterator, Allocator>::peek() const
	{
		if (!fetchLine()) throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
		tmp = ConstStringT<T>(buffer);
		return tmp;

	}

}
