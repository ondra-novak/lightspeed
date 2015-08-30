/*
 * fileiobuff.tcc
 *
 *  Created on: 7.6.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_STREAMS_FILEIOBUFF_TCC_
#define LIGHTSPEED_STREAMS_FILEIOBUFF_TCC_

#include "fileiobuff.h"
#include "../containers/autoArray.tcc"
#include "../iter/iteratorFilter.tcc"
#include <string.h>
#include <memory>

namespace LightSpeed {



template<natural bufferSize>
IOBuffer<bufferSize>::IOBuffer(ISeqFileHandle *hndl):targetIn(hndl), targetOut(hndl)
	,rdpos(bufferSize),wrpos(0),rdend(bufferSize),wrbeg(0),eof(false),outputClosed(false),writeReserve(0) {}

template<natural bufferSize>
IOBuffer<bufferSize>::IOBuffer(IInputStream *hndl):targetIn(hndl)
	,rdpos(bufferSize),wrpos(0),rdend(bufferSize),wrbeg(0),eof(false),outputClosed(true),writeReserve(0) {}

template<natural bufferSize>
IOBuffer<bufferSize>::IOBuffer(IInOutStream *hndl):targetIn(hndl), targetOut(hndl)
	,rdpos(bufferSize),wrpos(0),rdend(bufferSize),wrbeg(0),eof(false),outputClosed(false),writeReserve(0) {}


template<natural bufferSize>
IOBuffer<bufferSize>::IOBuffer(IOutputStream *hndl):targetOut(hndl)
	,rdpos(bufferSize),wrpos(0),rdend(bufferSize),wrbeg(0),eof(true),outputClosed(false),writeReserve(0) {}

template<natural bufferSize>
natural IOBuffer<bufferSize>::read(void *buffer,  natural size) {
	//if there are data in buffer
	if (rdpos < rdend) {
		//calculate how many
		natural s = rdend - rdpos;
		//adjust size to requested size
		if (s > size) s = size;
		//copy data
		memcpy(buffer,buff.data()+rdpos,s);
		//update read position
		rdpos+=s;
		//return count of bytes read
		return s;
	//no data and eof found
	} else if (eof) {
		//return eof
		return 0;
	//no data and not eof
	} else {
		//fetch data from target stream
		intFetch();
		//repeat function
		return read(buffer,size);
	}
}


template<natural bufferSize>
void IOBuffer<bufferSize>::intFetch() const {
	//if eof, do nothing
	if (eof) return;
	//flush output buffer - we have always flush any buffered data before reading to prevent deadlock
	intFlush();
	//also flush any connected external buffer
	if (autoflush != nil) autoflush->flush();
	//setup read pointers
	rdpos = rdend = writeReserve;
	//read to buffer - store count of bytes
	natural f = targetIn->read(buff.data()+rdpos,bufferSize-writeReserve);
	//if zero returned, set eof to true
	eof = f == 0;
	//adjust rdend - mark end of read data
	rdend+=f;
	//adjust write position depend on how many bytes left available
	adjustWritePos();
}

template<natural bufferSize>
natural IOBuffer<bufferSize>::write(const void *buffer,  natural size) {
	//calculate available space
	//write buffer can be behind read buffer or beyond.
	natural s = ((wrbeg < rdpos && rdpos < rdend)?rdpos:bufferSize) - wrpos;
	//if no space available
	if (s == 0) {
		//anything to flush?
		if (wrpos > wrbeg) {
			//flush now
			intFlush();
			//repeat function
			return write(buffer,size);
		} else {
			//no space and no buffer - forward write to target stream
			return targetOut->write(buffer,size);
		}
		
	}
	//buffer is empty and request want to write more then available space
	if (wrpos == wrbeg && s <= size) {
		//write direct to target string
		return targetOut->write(buffer,size);
	} else {
		//calculate how many bytes can be written - should be at least 1
		if (s > size) s = size;
		//copy bytes
		memcpy(buff.data()+wrpos,buffer,s);
		//update write pos
		wrpos += s;
		//return count of bytes written
		return s;
	}
}

template<natural bufferSize>
natural IOBuffer<bufferSize>::fetch() const {

	//if eof, no more fetch() is available
	if (eof) return 0;

	//calculate current size of the buffer
	natural  datasz = rdend - rdpos;
	//flush any output
	intFlush();
	//flush any other output buffer, because next data can depend on it
	if (autoflush != nil) autoflush->flush();
	//now, there is now writes, we can move buffer is necesery
	if (rdend == bufferSize) {
		//if there is no space to move, return zero
		if (rdpos == 0) return 0;
		//move data to the begin of the buffer
		memmove(buff.data(), buff.data()+rdpos, datasz);
		//update pointers
		rdpos = 0;
		rdend = rdpos + datasz;
	}
	//remaining space
	natural remain = bufferSize - rdend;
	//read to remaining space - can block
	natural x = targetIn->read(buff.data()+ rdend, remain);
	//update pointer
	rdend+=x;
	//update write pointers
	adjustWritePos();
	//mark eof if reached
	if (x == 0) eof = true;
	//return count of characters
	return x;
}


template<natural bufferSize>
natural IOBuffer<bufferSize>::peek(void *buffer, natural size) const {
	//we can now support peek buffer expansion
	if (size == 0) {
		if (canRead()) return rdpos - rdend;
		else return 0;
	}

	//calculate available data
	natural s = rdend - rdpos;
	//if less than required data are available
	while (s < size) {
		//try to fetch more
		natural k = fetch();
		//in case of unsucess, break cycle
		if (k == 0) break;
		//update size of available data
		s = s + k;
	}

	//whether available is above required
	if (s > size) s = size;
	//copy data 
	memcpy(buffer, buff.data() + rdpos, s);
	//return count of copied data
	return s;
}

template<natural bufferSize>
natural IOBuffer<bufferSize>::lookup(ConstBin seq, natural fetchedCount) const {
	//we cannot search empty array;
	if (seq.empty()) return 0;
	//calculate count of bytes to lookup
	natural s = rdend - rdpos;
	//adjust fetchedCount
	if (fetchedCount > s) fetchedCount = s;
	else {
		//adjust fetchedCount depend on length of seq;
		fetchedCount+=seq.length() - 1;
		//adjust depend on available bytes
		if (fetchedCount > s) fetchedCount = s;
	}
	//calculate start of search
	natural start = s - fetchedCount;
	//search now
	return buff.head(rdend).offset(rdpos).find(seq,start);

}


template<natural bufferSize>
bool IOBuffer<bufferSize>::canRead() const {
	//we can read, if there are bytes in the buffer
	if ( rdpos < rdend ) return true;
	//we cannot read, if there is eof
	if (eof) return false;
	//there is no source stream, return false;
	if (targetIn == nil) return false;
	//fetch bytes from the input stream
	/* We don't want to check available bytes in the target stream, because this
	 * operation is often simulated by reading into small internal buffer. This limits
	 * buffering to smallest internal buffer available. So fetch bytes now!
	 */
	intFetch();
	//only eof can stop reading
	return !eof;
}

template<natural bufferSize>
bool IOBuffer<bufferSize>::canWrite() const {
	return targetOut != nil && !outputClosed;
}

template<natural bufferSize>
void IOBuffer<bufferSize>::flush() {
	intFlush();
}

template<natural bufferSize>
void IOBuffer<bufferSize>::intFlush() const {
	//flush is only useful when there are bytes to flush
	if (wrpos > wrbeg) {
		//total to write
		natural s = wrpos - wrbeg;
		//write all
		natural x = targetOut->writeAll(buff.data()+wrbeg, s);
		//mark if output has been closed;
		if (x == 0) outputClosed = true;
		//adjust write position
		adjustWritePos();
	}
}

template<natural bufferSize>
void IOBuffer<bufferSize>::adjustWritePos() const {
	//if there are data to read and end of the buffer is larger then begin of the buffer
	if (rdend > rdpos && bufferSize - rdend > rdpos) {
		//put write pos to the end of reading
		wrbeg = wrpos = rdend;
	} else {
		//put write pos at the begin of the buffer
		wrbeg = wrpos = 0;
	}
}

template<natural bufferSize>
void IOBuffer<bufferSize>::closeOutput() {
	//first, mark output closed
	outputClosed = true;
	try {
		//flush any data in the buffer
		intFlush();
	} catch (...) {
		//in case of exception during flush
		try {
			//try to close target output
			targetOut->closeOutput();
		} catch (...) {
			//ingnore any exception
		}
		//throw original exception
		throw;
	}
	//after flush success, close target output.
	targetOut->closeOutput();
}

template<natural bufferSize>
void IOBuffer<bufferSize>::putBack(ConstBin seq) {
	//calculate occupied area
	natural sz = rdend - rdpos;
	//reject action, when there is no space to put data
	if (bufferSize - sz < seq.length()) throwWriteIteratorNoSpace(THISLOCATION, typeid(*this));

	//there is space before the read buffer
	if (rdpos > seq.length()) {
		//if there is write buffer and no space to put seq
		if (wrpos < rdpos && wrpos+seq.length() > rdpos) {
			//flush to output
			intFlush();
		}
		//put bytes behind the data
		memcpy(buff.data()+rdpos-seq.length(), seq.data(), seq.length());
		//update rdpos
		rdpos -= seq.length();

	} else {
		//failure - we cannot put bytes back without moving buffer
		/* by definition, it is not recomended to put back more bytes than was read.
		 * You can increase the are using function reserveWrite()
		 */
		throwWriteIteratorNoSpace(THISLOCATION, typeid(*this));		
	}
}

template<natural bufferSize>
natural IOBuffer<bufferSize>::dataReady() const {
	//if no bytes in the buffer
	if (rdpos >= rdend)
		//ask target stream whether data are ready
		//this should be done without blocking
		return targetIn->dataReady();
	else
		//otherwise calculate
		return (rdend-rdpos);
}

template<natural bufferSize>
void IOBuffer<bufferSize>::reserveWrite(natural sz) {
	if (sz>bufferSize/2) sz = bufferSize/2;
	writeReserve = sz;
}

}

#endif /* LIGHTSPEED_STREAMS_FILEIOBUFF_TCC_ */
