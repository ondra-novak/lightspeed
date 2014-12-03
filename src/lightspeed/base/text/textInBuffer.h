/*
 * textInBuffer.h
 *
 *  Created on: 12.3.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEXT_TEXTINBUFFER_H_
#define LIGHTSPEED_TEXT_TEXTINBUFFER_H_

#include "../containers/autoArray.h"

namespace LightSpeed {


	///Class implements buffered stream, where you are able to read stream ahead without discarding already fetched items
	/**
	 * Implements buffer and iterator, which is able to read input stream and store
	 * read data into buffer to allow repeatedly read until data is explicitly discarded.
	 * This creates stream snapshot which works as random access file, even
	 * if the stream doesn't support this feature.
	 *
	 * @tparam Iter source iterator or stream
	 * @tparam Alloc allocator used to allocate space for buffer. Use StdAlloc
	 * to allow infinite buffer, or use StaticAlloc, if you want to specify
	 * maximum size of the buffer.
	 */

	template<typename Iter, typename Alloc>
	class TextInBuffer {
	public:
		typedef typename OriginT<Iter>::T IterRaw;
		typedef typename ConstObject<typename IterRaw::ItemT>::Remove T;

		///Iterates through the buffer and reads ahead without discarding already fetched items
		class Iterator: public IteratorBase<T, Iterator> {
		public:
			friend class TextInBuffer;

			///Constructor. Use getFwIter
			/*
			 * @param owner reference to the owner object which holds the buffer
			 * @param pos staring position.
			 * @param automark true when every read marks the end of usable data
			 */
			Iterator(TextInBuffer &owner, natural pos, bool automark)
					:owner(owner),pos(pos),automark(automark) {}
			const T &getNext();
			const T &peek() const;
			bool hasItems() const;
			natural getRemain() const;
			bool equalTo(const Iterator &other) const;
			bool lessThan(const Iterator &other) const;

		protected:
			TextInBuffer &owner;
			natural pos;
			bool automark;

			natural absPos() const {
				if (pos < owner.commitCounter) return 0;
				else return pos - owner.commitCounter;
			}
		};


		///Constructs the buffer
		/**
		 * @param iter source iterator.
		 */
		TextInBuffer(Iter iter):iter(iter),commitCounter(0),commitPos(0) {}
		///Constructs the buffer
		/**
		 * @param iter source iterator.
		 * @param alloc instance of allocator used to initialize allocator object
		 */
		TextInBuffer(Iter iter, const Alloc &alloc):
				  buffer(alloc),iter(iter),commitCounter(0),commitPos(0) {}

		///Commits reading and discards the uncesseary data
		/** Use this function to commit already processed data to discard them and
		 *  reusing the available space for new data
		 *
		 * @param pos specifies position where new data starts. After
		 * return, data processed by this iterator is deleted.
		 *
		 * @note function doesn't affect active iterators' instances of the iterator, only
		 * the instance which pointed to the discarded area are moved to the
		 * begin of the new area
		 */
		void commit(const Iterator &pos);

		///Commits reading till position marked by mark()
		void commit();
		///Marks position()
		void mark(const Iterator &pos);
		///Preloads count of items to the buffer
		/**
		 * @param pos count of items from the beginning of the buffer. The beginning
		 * position is defined by the iterator returned by the function getFwIter().
		 * Number specifies count of items to preload from that position
		 *
		 * @return count of items successfully preloaded. If result is less than
		 * requested items, no more items are avaialable, stream is probably
		 * ending (hasItems() of source stream will return false). If returned
		 * value is above count of requested items, items has been already
		 * preloaded before function has been called, so function did nothing.
		 */
		natural preload(natural pos) const;

		///Retrieves iterator through the buffer and not-yet-fetched data.
		/**
		 * @return iterator
		 */
		Iterator getFwIter() {
			return Iterator(*this,commitCounter,false);
		}

		///Retrieves iterator - allows specify extra feature
		/**
		 * @param automark enable automark. Calls mark() after each getNext()
		 * @return iterator
		 */
		Iterator getFwIter(bool automark)  {
			return Iterator(*this,commitCounter,automark);
		}

		///Retrieves source iterator
		IterRaw &getSourceIterator() {return iter;}
		///Retrieves source iterator
		const IterRaw &getSourceIterator() const {return iter;}

		bool hasItems() const {return !buffer.empty() || iter.hasItems();}

		///Receives count of commited characters from the beginning of the stream
		natural getCharCount() const {return commitCounter;}

	protected:
		mutable AutoArrayStream<T, Alloc> buffer;
		mutable Iter iter;
		natural commitCounter;
		natural commitPos;

	};



}  // namespace LightSpeed


#endif /* LIGHTSPEED_TEXT_TEXTINBUFFER_H_ */
