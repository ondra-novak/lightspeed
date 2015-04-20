#define _CRT_RAND_S
#include "winpch.h"
#include "../streams/secureRandom.h"
#include "../streams/fileio_ifc.h"
#include <stdlib.h>
#include "../../mt/microlock.h"


namespace LightSpeed {



	class SecureRandomStream: public IInputStream {
	public:
	

		SecureRandomStream():bufPos(bufSize) {}
		~SecureRandomStream() {
			memset(buffer,0,sizeof(buffer));
		}

		virtual natural read( void *outbuff, natural size )
		{
			Synchronized<MicroLock> _(lock);
			natural ret = size;
			byte *wrpos = reinterpret_cast<byte *>(outbuff);
			do {
				if (bufPos == bufSize) {
					while (size >= bufSize) {
						rand_s(reinterpret_cast<unsigned int *>(wrpos));
						wrpos += bufSize;
						size -= bufSize;
					}
					if (size) {
						rand_s(reinterpret_cast<unsigned int *>(buffer));
						bufPos = 0;
					}
				} else {
					natural towrite = bufSize - bufPos;
					if (towrite > size) towrite = size;
					memcpy(wrpos,buffer+bufPos,towrite);
					wrpos+=towrite;
					size-=towrite;
					bufPos+=towrite;
				}
			} while (size);
			return ret;
		}

		virtual natural peek( void *outbuff, natural size ) const
		{
			Synchronized<MicroLock> _(lock);
			if (bufPos == bufSize) {
				rand_s(reinterpret_cast<unsigned int *>(buffer));
				bufPos = 0;				
			}
			natural towrite = bufSize - bufPos;
			if (towrite > size) towrite = size;
			memcpy(outbuff,buffer+bufPos,towrite);
			return towrite;
		}

		virtual bool canRead() const
		{
			return true;
		}

		virtual natural dataReady() const
		{
			return 1;
		}
	protected:

		static const natural bufSize =  sizeof(unsigned int);
		mutable byte buffer[bufSize];
		mutable natural bufPos;
		mutable MicroLock lock;

	};

	SecureRandom::SecureRandom():SeqFileInput(new SecureRandomStream()) {	}



}