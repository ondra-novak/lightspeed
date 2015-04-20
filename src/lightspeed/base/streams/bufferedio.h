/*
 * bufferedio.h
 *
 *  Created on: 9. 3. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_STREAMS_BUFFEREDIO_H_
#define LIGHTSPEED_BASE_STREAMS_BUFFEREDIO_H_
#include "../containers/autoArray.h"
#include "../memory/smallAlloc.h"
#include "bufferedio_ifc.h"


namespace LightSpeed {

template<natural bufsz>
class StreamBuffer {
public:
	StreamBuffer():wrpos(0),rdpos(0),isempty(true) {}

	ArrayRef<byte> getSpace();
	bool isEmpty() const ;
	bool isFull() const ;
	natural size() const ;
	natural getAvailable() const ;
	natural getContAvailable() const ;
	void commitWrite(natural count) ;
	const byte &getByte() const;
	ConstBin getData() const ;
	void commitRead(natural count);
	natural lookup(ConstBin data, natural fromPos) const;
	void putBack(byte b) ;
	void putByte(byte b) ;
	bool tryExpand();

private:
	byte buffdata[bufsz];
	natural wrpos;
	natural rdpos;
	bool isempty;

};

template<>
class StreamBuffer<0> {
public:
	StreamBuffer();

	ArrayRef<byte> getSpace();
	bool isEmpty() const ;
	bool isFull() const ;
	natural size() const ;
	natural getAvailable() const ;
	natural getContAvailable() const ;
	void commitWrite(natural count) ;
	const byte &getByte() const;
	ConstBin getData() const ;
	void commitRead(natural count);
	natural lookup(ConstBin data, natural fromPos) const;
	void putBack(byte b) ;
	void putByte(byte b) ;
	bool tryExpand();

private:
	AutoArray<byte, SmallAlloc<32> > buffdata;
	natural wrpos;
	natural rdpos;
	bool isempty;
	mutable bool wasFull;
	natural tmmark;

	void checkResize();

};

template<typename BufferImpl = StreamBuffer<4096> >
class BufferedInputStream: public IBufferedInputStream {
public:

	BufferedInputStream(PInputStream source);
	virtual IInputStream *getSrouce() const ;
	virtual natural fetch();
	virtual natural lookup(ConstBin seq, bool recentFetched);
	virtual natural discard(natural count);
	virtual bool putBack(ConstBin bytes) ;
    virtual natural read(void *buffer,  natural size);
	virtual natural peek(void *buffer, natural size) const ;
	virtual bool canRead() const ;
	virtual natural dataReady() const ;

	void setAutoflush(POutputStream output) { autoFlush = output;}

protected:

	virtual natural _fetch();

	PInputStream source;
	POutputStream autoFlush;
//	StreamBuffer<4096> buffer;
	BufferImpl buffer;
	natural lastFetchPos;
	bool eof;

};


template<typename BufferImpl>
class BufferedOutputStream: public IBufferedOutputStream {
public:
	BufferedOutputStream(POutputStream target):target(target),canWriteStream(target->canWrite()) {}

	virtual IOutputStream *getTarget() const;
	virtual natural sweep();
	virtual natural getAvaiable() const;
    virtual natural write(const void *buffer,  natural size);
	virtual bool canWrite() const;
	virtual void flush();
	virtual void closeOutput();

	void setAutoflush(POutputStream output) { autoFlush = output;}
protected:
	POutputStream target;
	POutputStream autoFlush;
//	StreamBuffer<4096> buffer;
	BufferImpl buffer;
	bool canWriteStream;

};


}

#endif /* LIGHTSPEED_BASE_STREAMS_BUFFEREDIO_H_ */

