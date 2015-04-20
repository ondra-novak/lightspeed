/*
 * buffferedio_ifc.h
 *
 *  Created on: Mar 9, 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_STREAMS_BUFFEREDIO_IFC_H_
#define LIGHTSPEED_BASE_STREAMS_BUFFEREDIO_IFC_H_
#include "fileio_ifc.h"

namespace LightSpeed {


///Autoflush feature
/** Every buffered stream allows to set chain of streams which will be flushed when original stream is forced to flush
 * or when input stream is read. This feature is available only for buffered streams
 */
class IBufferAutoflush {
public:
	virtual void setAutoflush(POutputStream stream) = 0;
};

///Extends IInputStream with many functions avaialble for software buffers
/**
 * Software buffers should implement this interface instead of implementing IInputStream.
 *
 *
 */
class IBufferedInputStream: public IInputStream, public IBufferAutoflush {
public:

	///Retrieves source stream
	/**
	 * @return pointer to stream, which is source of data. You can use
	 * this to determine state of source stream. This can be important
	 * if you need to determine right time to call fetch().
	 *
	 * You should not use returned value to reading because it can skip buffered content
	 */
	virtual IInputStream *getSrouce() const = 0;

	///Fetches additional bytes to the buffer
	/**
	 * By default, buffering is transparent, so data are fetched automatically as
	 * reading request incomes. This function causes that buffer tries to
	 * fetch additional data from the source stream.
	 *
	 * @return count of bytes fetched, if there is no more space, function returns zero
	 *
	 * @note function doesn't check state of source stream. If there are no data ready,
	 * function blocks until at least one byte arrives. If there is timeout defined on the
	 * source stream, function can throw TimeoutException
	 */
	virtual natural fetch() = 0;


	///Performs search inside buffer
	/**
	 *
	 * @param seq sequence of bytes to search
	 * @oaram recentFetcged if true is set, function searches just recently fetched
	 *        data. This can improve performance. Note that any read from the buffer
	 *        resets the internal lookup position and the next lookup starts from the
	 *        beginning
	 * @return returns position of looked sequence in the buffer. You can read given count
	 * of bytes to retrieve all data up to the sequence but without it. Or you can
	 * add length of sequect to the returned value and retrieve all data including the sequence.
	 *
	 * Function returns naturalNull when sequence was not found.
	 */
	virtual natural lookup(ConstBin seq, bool recentFetched = true) = 0;


	///Discards up to count bytes from the buffer
	/** Function has same effect as reading count bytes and throwing them out. Note
	 * that function do not fetch additional data
	 * @param count count of bytes to discard
	 * @return count of bytes discarded
	 */
	virtual natural discard(natural count) = 0;


	///Puts bytes on top of the buffer
	/**
	 * Function puts some bytes on top of the buffer. Put bytes will be read by next read operation prior to
	 * original data from the source. You can cal this function multiple-times until buffer is full.
	 * @param bytes
	 * @retval true success
	 * @retval false failure, no space in the buffer. No bytes were put back
	 */
	virtual bool putBack(ConstBin bytes) = 0;
};

class IBufferedOutputStream: public IOutputStream, public IBufferAutoflush {
public:


	///Retrieves target stream
	/**
	 *
	 * @return pointer to stream which receives data from the buffer.. You can use
	 * this to determine state of target stream. This can be important
	 * if you need to determine right time to call sweep().
	 *
	 * You should not use returned value to writing because it can skip buffered content
	 *
	 */
	virtual IOutputStream *getTarget() const = 0;

	///Sweeps part of the buffer
	/**
	 * Function tries to sweep whole or part of the buffer, it always sweeps at least one byte. Function can block, if
	 * the target stream is not ready to receive additional bytes, so you should first to check target stream state
	 * (use getTarget() to find target stream)
	 * @return count of bytes still remain in the buffer. Function returns 0, if there are no more bytes in the buffer
	 */
	virtual natural sweep() = 0;

	///Returns count of bytes available in the buffer to write without blocking
	/**
	 * @return count of bytes available to write
	 */
	virtual natural getAvaiable() const = 0;



};

}



#endif /* LIGHTSPEED_BASE_STREAMS_BUFFEREDIO_IFC_H_ */
