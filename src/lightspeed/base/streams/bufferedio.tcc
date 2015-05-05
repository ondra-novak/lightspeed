/*
 * bufferedio.tcc
 *
 *  Created on: 9. 3. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_STREAMS_BUFFEREDIO_TCC_
#define LIGHTSPEED_BASE_STREAMS_BUFFEREDIO_TCC_

#include "bufferedio.h"

#include "../exceptions/iterator.h"
namespace LightSpeed {

template<natural bufsz>
ArrayRef<byte> StreamBuffer<bufsz>::getSpace() {
	if (isempty) return ArrayRef<byte>(buffdata,bufsz);
	else return ArrayRef<byte>(buffdata+wrpos,getContAvailable());
}

template<natural bufsz>
bool StreamBuffer<bufsz>::isEmpty() const {return isempty;}
template<natural bufsz>
bool StreamBuffer<bufsz>::isFull() const {return !isempty && wrpos == rdpos;}
template<natural bufsz>
natural StreamBuffer<bufsz>::size() const {return !isempty?(wrpos <= rdpos?bufsz - rdpos + wrpos:wrpos - rdpos):0;}
template<natural bufsz>
natural StreamBuffer<bufsz>::getAvailable() const {return bufsz - size();}
template<natural bufsz>
natural StreamBuffer<bufsz>::getContAvailable() const {
	if (isempty) return bufsz;
	else if (wrpos <= rdpos) return rdpos - wrpos;
	else return bufsz-wrpos;
}

template<natural bufsz>
void StreamBuffer<bufsz>::commitWrite(natural count) {
	wrpos = (wrpos + count) % bufsz;
	isempty = false;
}

template<natural bufsz>
bool StreamBuffer<bufsz>::tryExpand() {
	return false;
}


template<natural bufsz>
const byte &StreamBuffer<bufsz>::getByte() const {
	return buffdata[rdpos];
}

template<natural bufsz>
ConstBin StreamBuffer<bufsz>::getData() const {
	if (isempty) {
		return ConstBin();
	} else if (wrpos <= rdpos) {
		return ConstBin(buffdata+rdpos, bufsz-rdpos);
	} else {
		return ConstBin(buffdata+rdpos, wrpos - rdpos);
	}
}



template<natural bufsz>
void StreamBuffer<bufsz>::commitRead(natural count) {
	rdpos = (rdpos + count) % bufsz;
	isempty = rdpos == wrpos;
	if (isempty) rdpos = wrpos = 0;
}

template<natural bufsz>
natural StreamBuffer<bufsz>::lookup(ConstBin data, natural fromPos) const {
	if (isempty || data.empty()) return naturalNull;
	natural b = (rdpos + fromPos) % bufsz;
	natural cnt = fromPos;
	do {
		if (buffdata[b] == data[0]) {
			natural c = b = (b + 1) % bufsz;
			natural i;
			for (i = 1; i < data.length() && c != wrpos; i++, c = (c + 1) % bufsz) {
				if (data[i] != buffdata[c]) break;
			}
			if (i == data.length()) return cnt;
			if (c == wrpos) return naturalNull;
		} else {
			b = (b + 1) % bufsz;
		}
		cnt++;
	} while (b != wrpos);
	return naturalNull;
}

template<natural bufsz>
void StreamBuffer<bufsz>::putBack(byte b) {
	if (rdpos == 0) rdpos = bufsz;
	rdpos--;
	buffdata[rdpos] = b;
	isempty = false;
}

template<natural bufsz>
void StreamBuffer<bufsz>::putByte(byte b) {
	buffdata[wrpos] = b;
	wrpos = (wrpos + 1) % bufsz;
	isempty = false;
}


template<typename BufferImpl>
 BufferedInputStream<BufferImpl>::BufferedInputStream(PInputStream source)
	:source(source), lastFetchPos(0), eof(false)
{
}

template<typename BufferImpl>
 IInputStream* BufferedInputStream<BufferImpl>::getSrouce() const {
	return source;
}

template<typename BufferImpl>
 natural BufferedInputStream<BufferImpl>::fetch() {
	natural k = _fetch();
	if (k == 0) {
		if (buffer.tryExpand())
			k = _fetch();
	}
	return k;

}

template<typename BufferImpl>
 natural BufferedInputStream<BufferImpl>::_fetch() {
	natural curSize = buffer.size();
	ArrayRef<byte> space = buffer.getSpace();
	if (space.empty() || eof) return 0;
	if (autoFlush != nil) autoFlush->flush();
	natural rd = source->read(space.data(),space.length());
	if (rd == 0) {
		eof = true;
	} else {
		buffer.commitWrite(rd);
		lastFetchPos = curSize;
	}
	return rd;
}

template<typename BufferImpl>
 natural BufferedInputStream<BufferImpl>::lookup(ConstBin seq,bool recentFetched) {
	natural startPos = (seq.length() > lastFetchPos || !recentFetched)?0:(lastFetchPos - seq.length() + 1);
	return buffer.lookup(seq,startPos);
}


template<typename BufferImpl>
 natural BufferedInputStream<BufferImpl>::discard(natural count) {
	natural candiscard =buffer.size();
	if (count > candiscard) count = candiscard;
	buffer.commitRead(count);
	lastFetchPos = 0;
	return count;

}

template<typename BufferImpl>
 bool BufferedInputStream<BufferImpl>::putBack(ConstBin bytes) {
	if (buffer.getAvailable() < bytes.length()) return false;
	for (natural i = bytes.length(); i > 0;) {
		--i;
		buffer.putBack(bytes[i]);
	}
	lastFetchPos = 0;
	return true;
}

template<typename BufferImpl>
 natural BufferedInputStream<BufferImpl>::read(void* buf,natural size) {
	lastFetchPos = 0;
	if (!buffer.isEmpty()) {
		if (size == 1) {
			*(byte *)buf = buffer.getByte();
		} else {
			ConstBin data = buffer.getData();
			if (size > data.length()) size = data.length();
			memcpy(buf,data.data(),size);
		}
		buffer.commitRead(size);
		return size;
	} else if (eof) {
		return 0;
	} else if (buffer.getContAvailable() < size) {
		if (autoFlush != nil) autoFlush->flush();
		return source->read(buf,size);
	} else {
		if (_fetch()) return read(buf,size);
		else return 0; //failed fetch on empty buffer is eof
	}
}

template<typename BufferImpl>
 natural BufferedInputStream<BufferImpl>::peek(void* buf,natural size) const {
	if (!buffer.isEmpty()) {
		if (size == 1) {
			*(byte *)buf = buffer.getByte();
		} else {
			ConstBin data = buffer.getData();
			if (size > data.length()) size = data.length();
			memcpy(buf,data.data(),size);
		}
		return size;
	} else if (eof) {
		return 0;
	} else if (buffer.getContAvailable() < size) {
		if (autoFlush != nil) autoFlush->flush();
		return source->peek(buf,size);
	} else {
		if (const_cast<BufferedInputStream<BufferImpl> *>(this)->_fetch()) return peek(buf,size);
		else return 0; //failed fetch on empty buffer is eof
	}

}

template<typename BufferImpl>
 bool BufferedInputStream<BufferImpl>::canRead() const {
	if (buffer.isEmpty()) {
		if (eof) return false;
		if (source->dataReady()) return true;
		if (autoFlush != nil) autoFlush->flush();
		return !eof && source->canRead();
	}
	else return true;
}

template<typename BufferImpl>
 natural BufferedInputStream<BufferImpl>::dataReady() const {
	if (buffer.isEmpty()) return eof?1:source->dataReady();
	else return buffer.size();
}

template<typename BufferImpl>
 IOutputStream* BufferedOutputStream<BufferImpl>::getTarget() const {
	return target;
}

template<typename BufferImpl>
 natural BufferedOutputStream<BufferImpl>::sweep() {
	if (buffer.isEmpty()) return 0;
	ConstBin d = buffer.getData();
	natural w = target->write(d.data(),d.length());
	if (w == 0) {
		canWriteStream = false;
		buffer.commitRead(d.length());
		return 0;
	}
	buffer.commitRead(w);
	return buffer.size();
}

template<typename BufferImpl>
 natural BufferedOutputStream<BufferImpl>::getAvaiable() const {
	return buffer.getAvailable();
}

template<typename BufferImpl>
 natural BufferedOutputStream<BufferImpl>::write(const void* buf, natural size) {
	if (!canWriteStream) throw IteratorNoMoreItems(THISLOCATION,typeid(*this));
	if (!buffer.isEmpty() || size < buffer.getAvailable()) {
		if (size == 1) {
			buffer.putByte(*(byte *)buf);
		} else {
			ArrayRef<byte> sp = buffer.getSpace();
			size =  sp.length() < size?sp.length():size;
			memcpy(sp.data(),buf,size);
			buffer.commitWrite(size);
		}
		if (buffer.isFull()) {
			sweep();
		}
		return size;
	} else {
		return target->write(buf,size);
	}
}

template<typename BufferImpl>
 bool BufferedOutputStream<BufferImpl>::canWrite() const {
	return canWriteStream;
}

template<typename BufferImpl>
 void BufferedOutputStream<BufferImpl>::flush() {
	while (canWriteStream && !buffer.isEmpty()) {
		sweep();
	}
	if (autoFlush) autoFlush->flush();
}

template<typename BufferImpl>
 void BufferedOutputStream<BufferImpl>::closeOutput() {
	flush();
	target->closeOutput();
	canWriteStream = false;
}


}


#endif /* LIGHTSPEED_BASE_STREAMS_BUFFEREDIO_TCC_ */
