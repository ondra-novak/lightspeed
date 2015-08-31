#include "../memory/refcntifc.h"
#include "fileio_ifc.h"
namespace LightSpeed {


	class IBufferFlush {
	public:
		virtual void flush() = 0;
		virtual ~IBufferFlush() {}
	};

	class IInputBuffer : public virtual IInputStream  {
	public:

		///connect output stream with input stream to handle flushing correctly
		/**Before input stream can be read from the source, any output stream that can
		   affect the data of the input stream must be flushed. Connect any stream or buffer
		   that supports IBufferFlush with this object. If you need to connect multiple
		   output buffers, you have to implement IBufferFlush fork

		   @note you don't need to connect output part of the same buffer. Bidirectional buffers
		   are handling autoflush internally 
		  */
		virtual void setAutoflush(IBufferFlush *flushStream) = 0;
		///Retrieves current autoflush object
		/** 
		  @return pointer to current autoflush object or nullptr
		  */
		virtual IBufferFlush *getAutoflush() const = 0;
		///Retrieves pointer to source stream
		/** 
		 @return pointer to original stream. You should avoid direct access to data, they
		  can be already buffered.
		*/
		virtual PInputStream getSource() const = 0;
		///Fetches more data from the source stream to the buffer
		/** By default, buffers are trying to fetch as much as possible, but for some streams,
			this can be impossible without blocking. Function commands the buffer object to 
			fetch more data until buffer is full. Function may block waiting for I/O completion, 
			but only if there are no bytes ready. Function returns once at least one byte is fetched. 
			You can repeat the call but each call can cause additional I/O wait

			@return function returns count of bytes fetched. If zero is returned, buffer is probably full,
			or end of stream reached. In this case, additional calls of the function returns zero until the
			some bytes are removed from the buffer
			*/
		virtual natural fetch() const = 0;

		///Allows to search the buffer for specified byte sequence without removing the data
		/**
		  @param seq sequence of bytes to search
		  @param fetchedCount For the very first search, you can leave default value. For additional search 
		    after fetch(), you can put there the return value of that function. This speeds up 
			the search because function don't need to process already processed part of the buffer.


		  @note always put return value of the fetch() as second argument regadless on how long is 
		    the sequence. Function should adjust the argument internally to perform the correct lookup.
		*/
		virtual natural lookup(ConstBin seq, natural fetchedCount = naturalNull) const = 0;

		///Puts bytes back to the buffer, so the can be read as they would appear on the stream
		/** @param seq bytes to put back to the buffer. Note that space in the buffer is limited. 
		      You should always be able to put 1 byte after reading. Putting more bytes back can 
			  cause failure. Some buffer can allow to reserve space for this feature */
		virtual void putBack(ConstBin seq) = 0;
		///returns length of buffered part in bytes
		/**
			@return count of bytes currently in buffer
		*/
		virtual natural getInputLength() const = 0;
		///returns total capacity of the buffer
		/**
		  @return maximum capacity of the buffer
		*/
		virtual natural getInputCapacity() const = 0;
		///retrieves current buffer
		/**
		 @return reference to buffer. Reference is valid until data are read from the buffer */
		virtual ConstBin getInputBuffer() const = 0;
		///discard up count bytes from the buffer
		/**
		 @param count count of bytes to discard

		 @note discard should be faster than skip() of the iterator
		*/
		virtual void discardInput(natural count) = 0;

	};

	class IOutputBuffer : public virtual IOutputStream, public IBufferFlush {
	public:
		///returns current count of bytes already buffered
		virtual natural getOutputLength() const = 0;
		///returns total capacity of the buffer
		virtual natural getOutputCapacity() const = 0;
		///returns available capacity of the buffer before buffer will be flushed to the output stream
		virtual natural getOutputAvailable() const = 0;
		///flush buffer to the output stream
		virtual void flush() = 0;
		///discard count of bytes recently sent to the stream
		/** function throws exception, if specified count of bytes cannot be discarded */
		virtual void discardOutput(natural count) = 0;

		virtual POutputStream getTarget() const = 0;
	};



	class IInOutBuffer : public IInOutStream, public IInputBuffer, public IOutputBuffer {
	public:


	};



}
