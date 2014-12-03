/*
 * binFilter.h
 *
 *  Created on: 22.8.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_BINFILTER_H_
#define LIGHTSPEED_UTILS_BINFILTER_H_
#include <string.h>
#include "../base/containers/arrayref.h"
#include "../base/containers/flatArray.h"
#include "../base/containers/constStr.h"


namespace LightSpeed {

	class IBinFilter {
	public:
		///Begins fetch operation
		/** Actually it is reading operation used during fetching data from the input stream.
		 *
		 * Function should check state of input and output buffer and perform filter operation as necesery
		 * keeping focus to have input buffer largest as possible.
		 *
		 * @retval true success
		 * @retval false no space for buffers - caller should throw an exception.
		 *
		 *
		 */
		virtual bool startFetch() = 0;
		///Retrievs maximum space for fetch
		/** Function should be called after startFetch.
		 * However, you can call this function before startFetch if you want to determine whether
		 * there is still chance to pass some data to the buffers. This value can be increased
		 * after startFetch as filter can consume some data from the input stream.
		 *
		 *
		 * @return Count of bytes available in the input buffer. If it is zero, try to first
		 * empty output buffer begin with function startFeed()
		 */
		virtual natural getFetchSpace() const = 0;

		///Retrieves pointer to buffer where application should place the data
		/** You have to call startFetch before call
		 *
		 * @return pointer to buffer. You can retrieve size of buffer by calling function getFetchSpace()
		 */
		virtual byte *getFetchBuffer() = 0;

		///Finishes fetch operation
		/**
		 * Finishes fetch operation and/or perform filter operation as necesery
		 *
		 * @param count count of bytes written into fetchg buffer
		 */
		virtual void fetchCommit(natural count) = 0;

		///Begins feed operation
		/** Actually it is writting operation used during the feeding output stream by the data
		 *
		 * Function should check state of output and input buffer and perform the filtering operation
		 * as it is necesary keeping focus to have output buffer as much filled as possible
		 *
		 * @return true success
		 * @retval false there is nothing useful as feed (no data in buffer)
		 */
		virtual bool startFeed() = 0;

		///Retrieves count of bytes ready in the feed buffer
		virtual natural getFeedLength() const = 0;

		///Retrieves pointer to first byte in the feed buffer
		virtual const byte *getFeedBuffer() = 0;

		///Commits feed  operation
		/**
		 *
		 * @param count count of bytes removed from the feed buffer
		 */
		virtual void feedCommit(natural count) = 0;

		///Flushes the filter
		/**
		 * Function is in most of cases implemented as flag causing that next startFeed or startFlush
		 * will perform flush before it starts. You should not call function inside feed or fetch operation
		 */
		virtual void flush();

		natural read(void *buffer, natural size) {
			if (!startFeed()) throwIteratorNoMoreItems(THISLOCATION,typeid(*this));
			natural s = getFeedLength();
			const byte *c = getFeedBuffer();
			if (size < s) s = size;
			memcpy(buffer,c,s);
			feedCommit(s);
			return s;
		}

		natural write(const void *buffer, natural size) {
			if (!startFetch()) throwWriteIteratorNoSpace(THISLOCATION,typeid(*this));
			natural s = getFetchSpace();
			byte *c = getFetchBuffer();
			if (size < s) s = size;
			memcpy(c,buffer,s);
			fetchCommit(s);
			return s;
		}
		bool needItems() const {
			return getFetchSpace() > 0;
		}

		bool hasItems() const {
			return getFeedLength() > 0;
		}

		virtual ~IBinFilter() {}
};

	class BinFilter: public IteratorFilterBase<byte,byte,IBinFilter> {
	public:

		BinFilter(IBinFilter *flt, bool deleteOnDestroy):flt(flt),deleteOnDestroy(deleteOnDestroy) {}
		~BinFilter() {
			if (deleteOnDestroy) delete flt;
		}

        bool needItems() const {
        	return flt->needItems();
        }

        void input(const byte &x) {
        	flt->write(&x,1);
        }

        template<typename Traits>
        natural blockInput(const FlatArray<byte,Traits> &block) {
        	return flt->write(block.data(),block.length());
        }
        template<typename Traits>
        natural blockInput(const FlatArray<const byte,Traits> &block) {
        	return flt->write(block.data(),block.length());
        }
        template<typename Traits>
        natural blockFetch(IIterator<byte,Traits> &iter) {
        	if (!flt->startFetch()) throwWriteIteratorNoSpace(THISLOCATION,typeid(*this));
        	byte *c = flt->getFetchBuffer();
    		natural s = flt->getFetchSpace();
        	ArrayRef<byte>b(c,s);
        	natural wr = iter.blockRead(b);
        	flt->fetchCommit(wr);
        	return wr;
        }
        template<typename Traits>
        natural blockFetch(IIterator<const byte,Traits> &iter) {
        	if (!flt->startFetch()) throwWriteIteratorNoSpace(THISLOCATION,typeid(*this));
        	byte *c = flt->getFetchBuffer();
    		natural s = flt->getFetchSpace();
        	ArrayRef<byte>b(c,s);
        	natural wr = iter.blockRead(b);
        	flt->fetchCommit(wr);
        	return wr;
        }
        bool hasItems() const {
        	return flt->hasItems();
        }
        byte output() {
        	byte b;
        	flt->read(&b,1);
        	return b;
        }

        template<typename Traits>
        natural blockOutput(FlatArrayRef<byte,Traits> block) {
        	return flt->read(block.data(),block.length());
        }
        template<typename Traits>
        natural blockFeed(IWriteIterator<byte,Traits> &iter) {
        	if (!flt->startFeed()) throwIteratorNoMoreItems(THISLOCATION,typeid(*this));
        	const byte *c = flt->getFeedBuffer();
        	natural s= flt->getFeedLength();
        	ConstStringT<byte>b(c,s);
        	natural wr = iter.blockWrite(b);
        	flt->feedCommit(wr);
        	return wr;
        }

	protected:
		IBinFilter *flt;
		bool deleteOnDestroy;


	};

}



#endif /* LIGHTSPEED_UTILS_BINFILTER_H_ */
