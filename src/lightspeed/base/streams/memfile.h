/*
 * memfile.h
 *
 *  Created on: 28.2.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_STREAMS_MEMFILE_H_
#define LIGHTSPEED_BASE_STREAMS_MEMFILE_H_

#include "fileio.h"
#include "../containers/autoArray.h"


namespace LightSpeed {


	class StdAlloc;

	template<typename Alloc = StdAlloc>
	class MemFile: public IInOutStream, public IRndFileHandle {
	public:

        virtual natural read(void *buffer,  natural size);
        virtual natural write(const void *buffer,  natural size);
		virtual natural peek(void *buffer, natural size) const;
		virtual bool canRead() const;
		virtual bool canWrite() const;
        virtual natural read(void *buffer,  natural size, FileOffset offset) const;
        virtual natural write(const void *buffer,  natural size, FileOffset offset);
        virtual void setSize(FileOffset size);
        virtual FileOffset size() const;
        void setPos(natural pos) {this->pos = pos;}
        virtual void flush() {}
        virtual natural dataReady() const {return true;}
        virtual void closeOutput() {readOnly = true;}

        MemFile():pos(0),readOnly(false) {}
                

        bool empty() const {return filebuffer.empty();}
        void clear() {filebuffer.clear();pos = 0;}

        AutoArray<byte, Alloc> &getBuffer() {return filebuffer;}
        const AutoArray<byte, Alloc> &getBuffer() const {return filebuffer;}		

		SeqFileInput getReadStream() {return SeqFileInput(this);}
		SeqFileOutput getWriteStream() {return SeqFileOutput(this);}
		void setAppendMode(bool b) {appendMode = b;}
		bool isAppendMode() const {return appendMode;}

	protected:

		typedef AutoArray<byte, Alloc> BufferT;
		BufferT filebuffer;
		natural pos;
		bool appendMode;
		bool readOnly;

	};


	class MemFileStr: public IInputStream, public IRndFileHandle {
	public:

		MemFileStr(ConstStrA text):text(text),pos(0) {}

        virtual natural read(void *buffer,  natural size);
        virtual natural write(const void *buffer,  natural size);
		virtual natural peek(void *buffer, natural size) const;
		virtual bool canRead() const;
		virtual bool canWrite() const;
        virtual natural read(void *buffer,  natural size, FileOffset offset) const;
        virtual natural write(const void *buffer,  natural size, FileOffset offset);
        virtual void setSize(FileOffset size);
        virtual FileOffset size() const;
        void setPos(natural pos) {this->pos = pos;}
        virtual void flush() {}
        virtual natural dataReady() const {return true;}
		virtual void closeOutput() {}


        bool empty() const {return text.empty();}

		SeqFileInput getReadStream() {return SeqFileInput(this);}

	protected:
		ConstStrA  text;
		natural pos;
	};


}



#endif /* LIGHTSPEED_BASE_STREAMS_MEMFILE_H_ */
