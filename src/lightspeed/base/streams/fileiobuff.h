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




namespace LightSpeed {


	class IBufferFlush {
	public:
		virtual void flush() = 0;
		virtual ~IBufferFlush() {}
	};


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
	class IOBuffer: public ISeqFileHandle, public IBufferFlush {
	public:

		///Constructs buffer object
		/**
		 * @param hndl source/target stream,
		 *
		 */
		IOBuffer(ISeqFileHandle *hndl);

		IOBuffer(IInOutStream *hndl);

		IOBuffer(IInputStream *hndl);

		IOBuffer(IOutputStream *hndl);


		virtual ~IOBuffer() {try {
				autoflush = nil;
				flush();
			} catch (...) {
				if (!std::uncaught_exception()) throw;
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

		PSeqFileHandle getTarget() const {return dynamic_cast<ISeqFileHandle *>(targetIn.get());}
		PInputStream getInputStream() const {return targetIn;}
		POutputStream getOutputStream() const {return targetOut;}

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
        SeqFileInBuff(const SeqFileInput &input)
        	:SeqFileInput(nil),buffr(input.getStream())
        		{connectBuff();}

		SeqFileInBuff(ISeqFileHandle *handle)
			:SeqFileInput(nil),buffr(handle)
		{connectBuff();}


		SeqFileInBuff(ConstStrW fname, natural openFlags)
			:SeqFileInput(nil),buffr(SeqFileInput(fname,openFlags).getStream())
				{connectBuff();}

		~SeqFileInBuff() {
			SeqFileInput::operator=(SeqFileInput(nil));
		}
        template<natural x>
        void setAutoflush( SeqFileOutBuff<x> &o) {
        	buffr.setAutoflush(&o.buffr);
        }

        void setAutoflush(IBufferFlush *o) {
        	buffr.setAutoflush(o);
        }

    protected:
        IOBuffer<bufferSize> buffr;
        void connectBuff() {
        	PSeqFileHandle x(&buffr);
        	buffr.setStaticObj();
        	SeqFileInput::operator=(SeqFileInput(x));
        }
    };

    template<natural bufferSize >
    class SeqFileOutBuff: public SeqFileOutput {
    public:
    	template<natural X> friend class SeqFileInBuff;
    	SeqFileOutBuff(const SeqFileOutput &output)
        	:SeqFileOutput(nil),buffr(output.getStream())
    		{connectBuff();}

		SeqFileOutBuff(ConstStrW fname, natural openFlags)
			:SeqFileOutput(nil),buffr(SeqFileOutput(fname,openFlags).getStream())
		{connectBuff();}

		SeqFileOutBuff(ISeqFileHandle *handle)
			:SeqFileOutput(nil),buffr(handle)
		{connectBuff();}

		~SeqFileOutBuff() {
			SeqFileOutput::operator=(SeqFileOutput(nil));
		}


    protected:
        IOBuffer<bufferSize> buffr;

        void connectBuff() {
        	PSeqFileHandle x(&buffr);
        	x.manualAddRef();
        	SeqFileOutput::operator=(SeqFileOutput(x));
        }
};



/*
    template< typename Iter>
    class OutputAutoflush: public WriteIteratorBase<typename OriginT<Iter>::T::ItemT,  OutputAutoflush<Iter> > {
    public:
    	typedef WriteIteratorBase<typename OriginT<Iter>::T::ItemT,  OutputAutoflush<Iter> > Super;
    	typedef typename Super::ItemT ItemT;

    	OutputAutoflush(Iter iter):iter(iter) {}
    	OutputAutoflush(Iter iter, ItemT flushChar):iter(iter),flushChar(flushChar) {}

    	void setFlushChar(ItemT flushChar) {this->flushChar = flushChar;}
    	void unsetFlushChar() {this->flushChar.unset();}

    	bool hasItems() const {return iter.hasItems();}
    	bool lessThan(const OutputAutoflush &other) {return iter.lessThan(other.iter);}
    	bool equalTo(const OutputAutoflush &other) {return iter.equalTo(other.iter);}

    	void write(const ItemT &x) {
    		iter.write(x);
    		if (flushChar.isSet() && x == flushChar.get()) iter.flush();
    	}

    protected:
    	Iter iter;
    	Optional<typename Super::ItemT> flushChar;

    };

    template< template<class> class CharConvertor =  WideToUtf8Writer>
    class OutputTextFile:
        public FltChain<FltChain<SeqFileOutput, CharConvertor<SeqFileOutput > >, OutputAutoflush<FltChain<SeqFileOutput, CharConvertor<SeqFileOutput > > > > {
        typedef FltChain<FltChain<SeqFileOutput, CharConvertor<SeqFileOutput > >, OutputAutoflush<FltChain<SeqFileOutput, CharConvertor<SeqFileOutput > > > >  Super;
     public:

        OutputTextFile(const SeqFileOutput &output):Super(output) {
            setFlushChar('\n');
        }

        void setFlushChar(const byte &chr) {Super::setFlushChar(chr);}
        void unsetFlushChar() {Super::unsetFlushChar();}
        bool isSetFlushChar() const {return Super::isSetFlushChar();}
        const byte &getFlushChar() const {return Super::getFlushChar();}

     };

    template<template<class> class CharConvertor =  Utf8ToWideReader>
    class InputTextFile:
        public FltChain<SeqFileInput, CharConvertor<SeqFileInput > > {

        typedef FltChain<SeqFileInput, CharConvertor<SeqFileInput > > Super;
     public:
        typedef typename Super::ItemT ItemT;

        InputTextFile(const SeqFileInput &output):Super(output) {}

        void connectOutput(IOutputControl *o) {Super::nxChain().connectOutput(o);}
        IOutputControl *getConnectedOutput() const {return Super::nxChain().getConnectedOutput();}

     };

*/
}

#endif /* LIGHTSPEED_STREAMS_FILEIOBUFF_H_ */
