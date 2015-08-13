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

#include "bufferedio.tcc"
namespace LightSpeed {



template<natural bufferSize>
IOBuffer<bufferSize>::IOBuffer(ISeqFileHandle *hndl):targetIn(hndl), targetOut(hndl)
	,rdpos(bufferSize),wrpos(0),rdend(bufferSize),wrbeg(0) {}

template<natural bufferSize>
IOBuffer<bufferSize>::IOBuffer(IInputStream *hndl):targetIn(hndl)
	,rdpos(bufferSize),wrpos(0),rdend(bufferSize),wrbeg(0) {}

template<natural bufferSize>
IOBuffer<bufferSize>::IOBuffer(IInOutStream *hndl):targetIn(hndl), targetOut(hndl)
	,rdpos(bufferSize),wrpos(0),rdend(bufferSize),wrbeg(0) {}


template<natural bufferSize>
IOBuffer<bufferSize>::IOBuffer(IOutputStream *hndl):targetOut(hndl)
	,rdpos(bufferSize),wrpos(0),rdend(bufferSize),wrbeg(0) {}

template<natural bufferSize>
natural IOBuffer<bufferSize>::read(void *buffer,  natural size) {
	if (rdpos < rdend) {
		natural s = rdend - rdpos;
		if (s > size) s = size;
		memcpy(buffer,buff.data()+rdpos,s);
		rdpos+=s;
		return s;
	} else {
		flush();
		if (autoflush != nil) autoflush->flush();
		rdpos = 0;
		rdend = targetIn->read(buff.data(),bufferSize);
		if (rdend == 0) return 0;
		return read(buffer,size);
	}
}

template<natural bufferSize>
natural IOBuffer<bufferSize>::write(const void *buffer,  natural size) {
	natural s = ((wrbeg < rdpos && rdpos < rdend)?rdpos:bufferSize) - wrpos;
	if (s == 0) {
		if (wrpos > wrbeg) {
			flush();
			return write(buffer,size);
		} else {
			return targetOut->write(buffer,size);
		}
		
	}
	if (wrpos == wrbeg && s < size) {
		return targetOut->write(buffer,size);
	} else {
		if (s > size) s = size;
		memcpy(buff.data()+wrpos,buffer,s);
		wrpos += s;
		return s;
	}
}


template<natural bufferSize>
natural IOBuffer<bufferSize>::peek(void *buffer, natural size) const {
	//we can now support peek buffer expansion

	IOBuffer<bufferSize> *mthis = const_cast<IOBuffer<bufferSize> *>(this);

	if (size == 0) {
		if (canRead()) return rdpos - rdend;
		else return 0;
	}

	//calculate available data
	natural s = rdend - rdpos;
	//if less than required data are available
	if (s < size) {
		//first flush any output
		mthis->flush();
		//also flush any other connected output
		if (autoflush != nil) mthis->autoflush->flush();
		//now, assume wrbeg == wrpos

		//if there is space at the begin of the buffer and no space at the end
		if (bufferSize < rdend + (size - s) && rdpos > 0) {

			// >>|----#data#?????|<<
			//move current data to the beginning of the buffer if required
			if (s) {
				memmove(buff.data(), buff.data() + rdpos, s);
			}
			//update pointers
			rdend -= rdpos;
			rdpos = 0;
			//now we have >>|#data# -----|<<
		}
		//if there is space
		if (bufferSize > rdend) {
			//read from the input as much as possible - function may block
			natural x = targetIn->read(buff.data() + rdend, bufferSize - rdend);
			//update rdend - stream can return 0 for end of file, we will not check here, because peek
			rdend += x;
			//calculate new s
			s = rdend - rdpos;
			//move wrpos and wrbeg at the end of the buffer
			wrpos = wrbeg = rdend;
		}
	}
	//whether available is above required
	if (s > size) s = size;
	//copy data 
	memcpy(buffer, buff.data() + rdpos, s);
	//return count of copied data
	return s;

}


template<natural bufferSize>
bool IOBuffer<bufferSize>::canRead() const {
	if ( rdpos < rdend ) return true;
	if (wrpos > wrbeg) {
		const_cast<IOBuffer<bufferSize> *>(this)->flush();
		if (autoflush != nil) autoflush->flush();
	}
	return targetIn->canRead();
}

template<natural bufferSize>
bool IOBuffer<bufferSize>::canWrite() const {
	return targetOut->canWrite();
}

template<natural bufferSize>
void IOBuffer<bufferSize>::flush() {
	if (wrpos > wrbeg) {
		while (wrpos > wrbeg) {
			natural s = wrpos - wrbeg;
			natural d = targetOut->write(buff.data()+wrbeg,s);
			if (d == 0) break; //some iterators can mistakenly return 0 when they cannot write. Solve this by discarding rest of data
			wrbeg+=d;
		}
		if (rdend > rdpos && bufferSize - rdend > rdpos) {
			wrbeg = wrpos = rdend;
		} else {
			wrbeg = wrpos = 0;
		}
	}
}

template<natural bufferSize>
void IOBuffer<bufferSize>::closeOutput() {
	try {
		flush();
	} catch (...) {
		try {
			targetOut->closeOutput();
		} catch (...) {

		}
		throw;
	}
	targetOut->closeOutput();
}



template<natural bufferSize>
natural IOBuffer<bufferSize>::dataReady() const {
	if (rdpos >= rdend)
		return targetIn->dataReady();
	else
		return (rdend-rdpos);
}

}

#endif /* LIGHTSPEED_STREAMS_FILEIOBUFF_TCC_ */
