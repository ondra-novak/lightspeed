#include <sys/stat.h>
/*
 * fileio.h
 *
 *  Created on: 16.4.2010
 *      Author: ondra
 */

#pragma once

struct stat64;

namespace LightSpeed
{
	class FileDescriptorSeq: public ISeqFileHandle, public ILockFileInterface
							, public IFileExtractHandle{
	public:
        int fd;
		StringA name;

		FileDescriptorSeq(int fd, StringA name);
		virtual ~FileDescriptorSeq();
		virtual natural read(void *buffer,  natural size);
		virtual natural write(const void *buffer,  natural size);
    	virtual bool lockShared(natural timeout = naturalNull);
    	virtual bool lockExclusive(natural timeout = naturalNull);
    	virtual bool lock(bool exclusive, natural timeout = naturalNull);
    	virtual void unlock();
    	virtual void flush();
    	virtual natural dataReady() const {return 0;}

    	bool lock(int flags, natural timeout);

    	virtual size_t getHandle(void *buffer, size_t bufferSize) ;

	};



	class ILinuxFileInfo: public IFolderIterator::IFileInformation {
	public:
		virtual const struct stat64 &getFileStat() const = 0;
	};

	StringA getFDName(int fd, bool canRead, bool canWrite, bool isPipe);
}
