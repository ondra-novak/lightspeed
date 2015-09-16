/*
 * fileiobuff.h
 *
 *  Created on: 28.12.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_STREAMS_FILEIOBUFF_H_
#define LIGHTSPEED_STREAMS_FILEIOBUFF_H_

#pragma once

#include "../iter/iteratorFilter.h"
#include "fileio.h"
#include "../containers/buffer.h"
#include "autoflush.h"
#include "utf.h"
#include "../containers/carray.h"

#include "fileiobuff_ifc.h"




namespace LightSpeed {




	///Implements simple buffering for streams that extends ISeqFileHandle
	/** This allows to include buffering into SeqFileInput and SeqFileOutput
	 *
	 *  Class contains one buffer, which can be used for both reading and
	 *  writing. Buffer is split dynamically depend on how many bytes are need
	 * to be written or read. Anytime that object need to preload buffer,
	 * all buffered written bytes are flushed. This helps to prevent deadlocks
	 * on network stream and bidirectional pipes.
	 *
	 *  Buffer tries to reduce count of communications of the original stream:
	 *  read request can return less bytes than requested when there are
	 *  not enough bytes in the buffer instead to requesting original stream
	 *  to read additional bytes. Writing can also write less bytes than
	 *  requested because there is no more space in buffer without
	 *  need to flush the bytes. You can still use readAll and writeAll to
	 *  read or write complete count of bytes.
	 *
	 *
	 *  */

	

	template<natural bufferSize = 4096>
	class IOBuffer : public IInOutBuffer {
	public:

		IOBuffer(IInOutStream *hndl);

		IOBuffer(IInputStream *hndl);

		IOBuffer(IOutputStream *hndl);


		virtual ~IOBuffer() {try {
				autoflush = nil;
				flush();
			} catch (...) {

			}
		}

        virtual natural read(void *buffer,  natural size);
        virtual natural write(const void *buffer,  natural size);
		virtual natural peek(void *buffer, natural size) const;
		virtual bool canRead() const;
		virtual bool canWrite() const;
		virtual void flush();
		///Returns count of bytes can be read without blocking
		/** Currently, function returns count of bytes available in the buffer.
		 * It doesn't ask source stream for the value unless buffer is empty.
		 * This results to better performance reducing count of possible syscalls
		 *
		 * @return
		 */
		virtual natural dataReady() const;
		virtual void closeOutput();

		///sets object to be flushed while reading operation is requested
		/**
		 * @param autoflush pointer to object which will be flushed when
		 * read operation is requested.
		 *
		 * This allows to synchronize output and input stream to prevent
		 * deadlocking when reading is requested while there are unwritten
		 * data in output buffer
		 */
		void setAutoflush(IBufferFlush *autoflush) {this->autoflush = autoflush;}

		virtual POutputStream getTarget() const {return targetOut;}
		virtual PInputStream getSource() const { return targetIn; }


		///Preloads buffer from the input stream
		/**
		 * Function performs one cycle of reading from the input stream without changing state of the
		 * reading.
		 * @return Count of bytes preloaded. If there is no space in the buffer, or if EOF has
		 *  been reached, function returns 0.
		 */
		natural fetch() const ;
		///Searches the buffer for given sequence
		/**
		 * @param seq binary sequence to search.
		 * @param fetchedCount count of fetched bytes to search. It is hint to speed operation.
		 * 			Default value causes, that whole buffer will be searched. You can specify
		 * 			result of last fetch operation to search only recent fetched bytes. If you
		 * 			fetched multiple times, you can sum these results into one value and pass
		 * 			this value as argument. Function also counts with length of the sequence, it
		 * 			can adjust the value if necesery.
		 *
		 * @return position in the buffer. Informs you, how many bytes you need to read
		 *  to reach the searched sequence. Function retuns naturalNull if sequence is not found.
		 *
		 * function can be used along with fetch()
		 *
		 * @code
		 * natural pos = buff.lookup(seq);
		 * while (pos == naturalNull) {
		 * 	   natural fetched = buff.fetch();
		 *     if (fetched == 0) throw ErrorMessageException(THISLOCATION, "Not found");
		 *     pos = buff.lookup(seq,fetched);
		 * }
		 * @endcode
		 */
		natural lookup(ConstBin seq, natural fetchedCount = naturalNull) const ;

		///Puts bytes on top of the buffer
		/**
		 * Function puts some bytes on top of the buffer. Put bytes will be read by next read operation prior to
		 * original data from the source. You can cal this function multiple-times until buffer is full.
		 * @param bytes
		 * @retval true success
		 * @retval false failure, no space in the buffer. No bytes were put back
		 */
	void putBack(ConstBin seq);

	///Reserves space for writing
	/**
	 * Function is useful when buffer is used for both reading and writing operation.
	 * If buffer is used for writing only, you don't need to set this value. Function
	 * ensures, that there will be always reserved specified count of bytes for writing. It
	 * means that function reduces space for reading.
     *
	 * @param sz count of bytes reserved for writing. Value is limited to half of the buffer.
	 * You can set naturalNull to reserve much buffer as possible.
	 *
	 * @note in some cases, there is still allowed to temporary lower this value for specific reading
	 * operations. For example function putBack() can use reserved area to put data there. The value
	 * actually sets reserved space for putBack() bytes.
	 * Functions fetch() and peek() can also temporary claim the reserved area to receive maximum available
	 * bytes to achieve required function (when it runs out of space).
	 * You have to include into your calculations.
	 *
	 *
	 */
	void reserveWrite(natural sz);

	virtual IBufferFlush * getAutoflush() const { return autoflush; }

	virtual natural getInputLength() const { return rdend - rdpos; }
	virtual natural getInputCapacity() const { return bufferSize; }
	virtual ConstBin getInputBuffer() const { return ConstBin(buff.head(rdend).offset(rdpos)); }
	virtual void discardInput(natural count) {
		natural l = rdend - rdpos;
		if (l < count) count = l;
		rdpos += count;
	}

	virtual natural getOutputLength() const { return wrpos - wrbeg; }
	virtual natural getOutputCapacity() const { return bufferSize;  }
	virtual natural getOutputAvailable() const {
		natural s = ((wrbeg < rdpos && rdpos < rdend) ? rdpos : bufferSize) - wrpos;
		return s;
	}
	virtual void discardOutput(natural count) {
		natural s = wrpos - wrbeg;
		if (count > s) wrpos = wrbeg; else wrpos -= count;
	}


	protected:
		///handle of stream 
		PInputStream targetIn;
		POutputStream targetOut;
		///pointer to object connected to buffer doing automatic flush
		Pointer<IBufferFlush>autoflush;	
		typedef CArray<byte,bufferSize> BuffImpl;
		///buffer
		mutable BuffImpl buff;
		///current read position
		mutable natural rdpos;
		///current write position
		mutable natural wrpos;
		///end of read data
		mutable natural rdend;
		///begin of written data
		mutable natural wrbeg;
		///true, if eof already found and waiting after buffer is emptied
		mutable bool eof;

		mutable bool outputClosed;

		natural writeReserve;


		void intFetch() const;
		void intFlush() const;
		//note - expected flushed output
		void adjustWritePos() const;

	private:
		///buffer cannot be copied
		IOBuffer(const IOBuffer &) {}
		///buffer cannot be assigned
		void operator=(const IOBuffer &) {}



	};


    template<natural bufferSize = 4096>
    class SeqFileOutBuff;

    template<natural bufferSize = 4096>
    class SeqFileInBuff: public SeqFileInput {
    public:
    	explicit SeqFileInBuff(const SeqFileInput &input)
			:SeqFileInput(new IOBuffer<bufferSize>(input.getStream())) {}

    	explicit SeqFileInBuff(IInputStream *handle)
			:SeqFileInput(new IOBuffer<bufferSize>(handle)){}


    	explicit SeqFileInBuff(ConstStrW fname, natural openFlags)
		:SeqFileInput(new IOBuffer<bufferSize>(SeqFileInput(fname, openFlags).getStream())){}

        template<natural x>
        void setAutoflush( SeqFileOutBuff<x> &o) {
        	getBuffer().setAutoflush(&o.buffr);
        }

        void setAutoflush(IBufferFlush *o) {
			getBuffer().setAutoflush(o);
        }

		const IOBuffer<bufferSize> &getBuffer() const { return static_cast<const IOBuffer<bufferSize> &>(*this->fhnd); }
		IOBuffer<bufferSize> &getBuffer() { return static_cast<IOBuffer<bufferSize> &>(*this->fhnd); }
    };

    template<natural bufferSize >
    class SeqFileOutBuff: public SeqFileOutput {
    public:
    	template<natural X> friend class SeqFileInBuff;
    	explicit SeqFileOutBuff(const SeqFileOutput &output)
			:SeqFileOutput(new IOBuffer<bufferSize>(output.getStream())) {}

    	explicit SeqFileOutBuff(ConstStrW fname, natural openFlags)
			:SeqFileOutput(new IOBuffer<bufferSize>(SeqFileOutput(fname, openFlags).getStream())) {}

    	explicit SeqFileOutBuff(IOutputStream *handle)
			: SeqFileOutput(new IOBuffer<bufferSize>(handle)) {}


		const IOBuffer<bufferSize> &getBuffer() const { return static_cast<const IOBuffer<bufferSize> &>(*this->fhnd); }
		IOBuffer<bufferSize> &getBuffer() { return static_cast<IOBuffer<bufferSize> &>(*this->fhnd); }


	};



}

#endif /* LIGHTSPEED_STREAMS_FILEIOBUFF_H_ */
