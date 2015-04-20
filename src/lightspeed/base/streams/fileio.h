#ifndef LIGHTSPEED_STREAMS_FILEIO_H_
#define LIGHTSPEED_STREAMS_FILEIO_H_

#include "fileio_ifc.h"
#include "../exceptions/throws.h"

namespace LightSpeed {        

class RndFileBase {
public:
	typedef IRndFileHandle::FileOffset FileOffset;
	RndFileBase(PRndFileHandle rndFile, IRndFileHandle::FileOffset offset)
		:rndFile(rndFile),offset(offset) {}

	FileOffset getOffset() const {return offset;}
	void setOffset(FileOffset offset) {this->offset = offset;}
protected:
	PRndFileHandle rndFile;
	FileOffset offset;
};

///Converts IRndFileHandle to IInputStream
class RndFileReader: public IInputStream, public RndFileBase {
public:
	RndFileReader(PRndFileHandle rndFile, IRndFileHandle::FileOffset offset)
		:RndFileBase(rndFile,offset) {}

    virtual natural read(void *buffer,  natural size) {
    	FileOffset fsize = rndFile->size();
    	if (offset > fsize || offset + size  > fsize) {
        	if (offset > fsize) return 0;
        	if (offset + size > fsize) size = (natural)(fsize - offset);
    	}
    	natural rd = rndFile->read(buffer,size,offset);
    	offset+=rd;
    	return rd;

    }
	virtual natural peek(void *buffer, natural size) const  {
    	FileOffset fsize = rndFile->size();
    	if (offset > fsize || offset + size  > fsize) {
        	if (offset > fsize) return 0;
        	if (offset + size > fsize) size = (natural)(fsize - offset);
    	}
    	natural rd = rndFile->read(buffer,size,offset);
    	return rd;
	}
	virtual bool canRead() const {
    	FileOffset fsize = rndFile->size();
		return offset < fsize;
	}
	virtual natural dataReady() const {return 0;}

};

///Converts IRndFileHandle to IOutputStream
class RndFileWriter: public IOutputStream, public RndFileBase {
public:
	RndFileWriter(PRndFileHandle rndFile, IRndFileHandle::FileOffset offset)
		:RndFileBase(rndFile,offset),canWr(true) {}
    virtual natural write(const void *buffer,  natural size) {
    	if (!canWr) throwWriteIteratorNoSpace(THISLOCATION,typeid(*this));
    	natural rd = rndFile->write(buffer,size,offset);
    	offset+=rd;
    	return rd;

    }
	virtual bool canWrite() const {
		return true;
	}
	virtual void flush()  {rndFile->flush();}
	IRndFileHandle::FileOffset getOffset() const {return offset;}
	void setOffset(IRndFileHandle::FileOffset offset) {this->offset = offset;}
	virtual void closeOutput() {
		canWr = false;
	}
protected:
	bool canWr;

};

    
class SeqFileInput: public IteratorBase<byte, SeqFileInput> {
public:
	SeqFileInput(PSeqFileHandle fhnd):fhnd(fhnd.get()) {}
	SeqFileInput(IInputStream *fhnd):fhnd(fhnd) {}
	SeqFileInput(PInputStream fhnd):fhnd(fhnd) {}
	SeqFileInput(NullType) {}
	SeqFileInput(PRndFileHandle fhnd, IRndFileHandle::FileOffset offset):fhnd(new RndFileReader(fhnd,offset)) {}
	SeqFileInput(ConstStrW ofn, OpenFlags::Type flags = 0)
		:fhnd(IFileIOServices::getIOServices().openSeqFile(ofn,
				IFileIOServices::fileOpenRead,flags)) {}


	bool hasItems() const {
		return fhnd->canRead();
	}

	const byte &getNext() {
		natural r = fhnd->read(&b,1);
		if (r == 0)
			throwIteratorNoMoreItems(THISLOCATION,typeid(byte));
		return b;
	}

	const byte &peek() const  {
		natural r = fhnd->peek(&b,1);
		if (r == 0)
			throwIteratorNoMoreItems(THISLOCATION,typeid(byte));
		return b;
	}


	template<class Traits>
	natural blockRead(FlatArrayRef<byte,Traits> buffer, bool all = false) {
		natural rd =  all?fhnd->readAll(buffer.data(),buffer.length())
					:fhnd->read(buffer.data(),buffer.length());
		if (all && rd != buffer.length()) {
			throwIteratorNoMoreItems(THISLOCATION,typeid(byte));
		}
		return rd;
	}

	natural blockRead( void *ptr, natural size, bool all = true) {
		if (size == 0) return 0;
		natural x = all?fhnd->readAll(ptr,size):fhnd->read(ptr,size);
		if (x < size && all) {
			throwIteratorNoMoreItems(THISLOCATION,typeid(byte));
		} else if (x == 0 && !fhnd->canRead()) {
			throwIteratorNoMoreItems(THISLOCATION,typeid(byte));
		}
		return x;
	}

	PSeqFileHandle getHandle() const {return dynamic_cast<ISeqFileHandle *>(fhnd.get());}
	PInputStream getStream() const {return fhnd;}

	natural dataReady() const {return fhnd->dataReady();}

protected:
	mutable PInputStream fhnd;
	mutable byte b;
};

class SeqFileOutput: public WriteIteratorBase<byte, SeqFileOutput> {
public:

	SeqFileOutput(PSeqFileHandle fhnd):fhnd(fhnd.get()) {}
	SeqFileOutput(POutputStream fhnd):fhnd(fhnd) {}
	SeqFileOutput(IOutputStream *fhnd):fhnd(fhnd) {}
	SeqFileOutput(NullType) {}
	SeqFileOutput(PRndFileHandle fhnd, IRndFileHandle::FileOffset offset):fhnd(new RndFileWriter(fhnd,offset)) {}
	SeqFileOutput(ConstStrW ofn, OpenFlags::Type flags = OpenFlags::create|OpenFlags::truncate)
		:fhnd(IFileIOServices::getIOServices().openSeqFile(ofn,
				IFileIOServices::fileOpenWrite,flags)) {}
	bool hasItems() const {
		return fhnd->canWrite();
	}
	void write(const byte &item) {
		natural x = fhnd->write(&item,1);
		if (x == 0) throwWriteIteratorNoSpace(THISLOCATION, typeid(byte));
	}

	template<class Traits>
	natural blockWrite(const FlatArray<byte,Traits> &buffer, bool all = true) {
		return blockWrite(buffer.data(),buffer.length(),all);
	}

	template<class Traits>
	natural blockWrite(const FlatArray<const byte,Traits> &buffer, bool all = true) {
		return blockWrite(buffer.data(),buffer.length(),all);
	}

	natural blockWrite(const void *ptr, natural size, bool all = true) {
		if (size == 0) return 0;
		natural x = all?fhnd->writeAll(ptr,size):fhnd->write(ptr,size);
		if (x == 0 || (x < size && all))
			throwWriteIteratorNoSpace(THISLOCATION, typeid(byte));
		return x;
	}

	PSeqFileHandle getHandle() const {return dynamic_cast<ISeqFileHandle *>(fhnd.get());}
	POutputStream getStream() const {return fhnd;}
	void flush() {fhnd->flush();}
	void closeOutput() {fhnd->closeOutput();}
	void reserve(natural itemCount) {
		for (natural i = 0; i <itemCount; i++) write((unsigned char)0);
	}
protected:
	mutable POutputStream fhnd;
};

class StdInput: public SeqFileInput {
public:
	StdInput():SeqFileInput(IFileIOServices::getIOServices().openStdFile(
			IFileIOServices::stdInput)) {}
};

class StdOutput: public SeqFileOutput {
public:
	StdOutput():SeqFileOutput(IFileIOServices::getIOServices().openStdFile(
			IFileIOServices::stdOutput)) {}
};

class StdError: public SeqFileOutput {
public:
	StdError():SeqFileOutput(IFileIOServices::getIOServices().openStdFile(
			IFileIOServices::stdError)) {}
};

///Joins two stream unidirectional streams to one bidirectional
class SeqBidirStream: public IInOutStream {
public:
	SeqBidirStream(PInputStream input, POutputStream output)
		:input(input),output(output) {}

	virtual natural read(void *buffer,  natural size) {
		return input->read(buffer,size);
	}
	virtual natural write(const void *buffer,  natural size) {
		return output->write(buffer,size);
	}
	virtual natural peek(void *buffer, natural size) const {
		return input->peek(buffer,size);
	}
	virtual bool canRead() const {return input->canRead();}
	virtual bool canWrite() const {return output->canWrite();}
	virtual void flush() {output->flush();}
	virtual natural dataReady() const {return input->dataReady();}

	PInputStream getInput() const {return input;}
	POutputStream getOuput() const {return output;}
	virtual void closeOutput() {output->closeOutput();}


protected:
	PInputStream input;
	POutputStream output;

};

///Creates standard pipe
/**
 * Pipe is stream with two sides. Data written into one side can be
 * read on other side. It useful to use pipe between threads or processes, while
 * one thread/process is writing data and other thrad/process is reading data at
 * same time.
 *
 * Class Pipe helps to create two connected streams, but instance don't need to
 * exists during whole life time of the pipe. Pipe continues to exists when
 * there is at least one opened stream. Pipe can work if there are both streams, but
 * pipe still exists when one side of pipe is closed. Other side can still read
 * all pending data in the pipe.
 */
class Pipe {
public:

	///Constructs pipe
	Pipe() {
		IFileIOServices::getIOServices().createPipe(pread,pwrite);
	}

	///Retrieves read end of the pipe
	/**
	 * @return Read end of pipe, can read data written into the pipe using
	 *  the write end of the pipe. When write-end of the pipe is closed,
	 *  this object reports EOF (hasItems() returns false)
	 */
	SeqFileInput getReadEnd() const {return SeqFileInput(pread);}

	///Retrieves write end of the pipe
	/**
	 * @return Write-end of the pipe. Data written into this object can be read
	 * using read-end of the pipe. If read-end is closed, writing into this object
	 * causes exception (you cannot test hasItems() because this situation is obviously not tested
	 * by the function.
	 */
	SeqFileOutput getWriteEnd() const {return SeqFileOutput(pwrite);}


	///Detaches object from the streams
	/**
	 * After detaching, object no more refers to created streams and
	 * starts returning nil for both functions getReadEnd and getWriteEnd.
	 *
	 * This is useful, when you cannot destroy object at current stack
	 * frame, but need to close one of created streams. Note that
	 * dropping streams doesn't close streams automatically. It only decreases
	 * reference counter and only when there is no reference to the streams
	 * causes closing them
	 *
	 */
	void detach() {pread = nil; pwrite = nil;}
protected:
	PInputStream pread;
	POutputStream pwrite;
};

}



#endif /*FILEIO_H_*/
