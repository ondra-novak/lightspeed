#include <cstdlib>
#include <stddef.h>
#include <wchar.h>
#include <dirent.h>
#include <unistd.h>
#include "../streams/fileio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <string.h>
#include "../exceptions/unsupportedFeature.h"
#include "../exceptions/systemException.h"
#include "../exceptions/fileExceptions.h"
#include "../containers/string.tcc"
#include "../text/textFormat.tcc"
#include "../../mt/atomic.h"
#include "../memory/staticAlloc.h"
#include <sys/file.h>
#include "fileio.h"
#include "../exceptions/invalidParamException.h"
#include "../../mt/linux/systime.h"
#include <sys/mman.h>

#include "../interface.tcc"
#include "linuxhttp.h"
#include "../streams/tcpHandler.h"
#include "../streams/abstractFileIOService.h"



namespace LightSpeed {

	FileDescriptorSeq::FileDescriptorSeq(int fd, StringA name)
		:fd(fd),name(name) {}
	FileDescriptorSeq::~FileDescriptorSeq() {
		if (fd > 2) close(fd);
	}


    class FileDescriptor: public FileDescriptorSeq,
                          public IRndFileHandle {
    public:
        bool deleteOnClose;
		bool maskRead,maskWrite;
    
        FileDescriptor(int fd, StringA name, bool deleteOnClose, bool maskRead, bool maskWrite);
        virtual ~FileDescriptor();
        virtual natural write(const void *buffer,  natural size);
        using FileDescriptorSeq::read;
        using FileDescriptorSeq::write;
        virtual natural read(void *buffer,  natural size, FileOffset offset) const;
        virtual natural write(const void *buffer,  natural size, FileOffset offset);
        virtual void setSize(FileOffset size);        
        virtual FileOffset size() const;
		virtual FileOffset getPosition() const;
		virtual void setPosition(FileOffset ofs);
		virtual natural peek(void *buffer, natural size) const;
		virtual bool canRead() const;
		virtual bool canWrite() const;
		virtual void closeOutput();
		virtual void flush() {FileDescriptorSeq::flush();}

	protected:
		mutable FileOffset cachedSize;

    };
    
    class LinuxFileServices: public AbstractFileIOService {
    public:
    	LinuxFileServices() {
    		this->setHandler(L"http://",&http);
    		this->setHandler(L"https://",&https);
    		this->setHandler(L"tcp://",&tcp);

    	}

        virtual PInOutStream openSeqFile(ConstStrW ofn, FileOpenMode mode, OpenFlags::Type flags);
        virtual PInOutStream openStdFile(StdFile stdfile);
        virtual void createPipe(PInputStream &readEnd, POutputStream &writeEnd);
        virtual PRndFileHandle openRndFile(ConstStrW ofn, FileOpenMode mode, OpenFlags::Type flags);
        virtual bool canOpenFile(ConstStrW name, FileOpenMode mode) const;
        virtual PInOutStream openSeqFile(const void *handle, size_t handleSize, FileOpenMode mode);
        virtual PRndFileHandle openRndFile(const void *handle, size_t handleSize, FileOpenMode mode);
        virtual PFolderIterator openFolder(ConstStrW pathname) ;
        virtual void createFolder(ConstStrW folderName, bool parents);
        virtual void removeFolder(ConstStrW folderName, bool recursive);
        virtual void copy(ConstStrW from, ConstStrW to, bool overwrite);
        virtual void move(ConstStrW from, ConstStrW to, bool overwrite);
        virtual void link(ConstStrW linkName, ConstStrW target, bool overwrite);
        virtual void remove(ConstStrW what);
        virtual PMappedFile mapFile(ConstStrW filename, FileOpenMode mode);
        virtual String findFile(ConstStrW filename, ConstStrW pathList,natural position);
        virtual String findFile(ConstStrW filename, ConstStrW pathList,natural position, bool executable);
        virtual String findExec(ConstStrW execName);
        virtual PTemporaryFile createTempFile(ConstStrW prefix, bool rndfile = true) ;
        virtual PFolderIterator getFileInfo(ConstStrW pathname) ;


        LinuxHttpHandler http,https;
        TcpHandler tcp;
    };

	class PipeDescriptor: public FileDescriptorSeq {
	public:
		PipeDescriptor(int fd, ConstStrA name, bool maskRead, bool maskWrite)
			:FileDescriptorSeq(fd,StringA(name)),maskRead(maskRead)
			,maskWrite(maskWrite),peekBuffUsage(0),peekBuffPos(0) {}

		natural bytesInBuff() const {
			return peekBuffUsage - peekBuffPos;
		}

		virtual natural dataReady() const {
			return bytesInBuff();
		}

		void readToBuffer() const {
			natural sz = const_cast<PipeDescriptor *>(this)->FileDescriptorSeq::read(peekBuff,peekBuffSize);
			peekBuffPos = 0;
			peekBuffUsage = sz;
			if (sz == 0)
				maskRead = true;

		}

		virtual natural peek(void *buffer, natural size) const {
			if (maskRead) return 0;
			if (buffer == 0) {
					return bytesInBuff();
			}
			if (!dataReady()) {
				readToBuffer();
			}
			natural toCopy = bytesInBuff();
			if (toCopy > size) toCopy = size;
			if (toCopy > 0) memcpy(buffer,peekBuff+peekBuffPos,toCopy);
			return toCopy;
		}


		virtual natural read(void *buffer,natural size) {
			if (maskRead) return 0;
			if (buffer == 0) throwNullPointerException(THISLOCATION);
			natural toCopy = bytesInBuff();
			if (toCopy > 0) {
				if (toCopy > size) toCopy = size;
				memcpy(buffer,peekBuff+peekBuffPos,toCopy);
				peekBuffPos+=toCopy;
				return toCopy;
			} else {
				natural res = FileDescriptorSeq::read(buffer,size);
				if (res == 0)
					maskRead = true;
				return res;
			}
		}

		virtual bool canRead() const {
			if (maskRead) return false;
			if (bytesInBuff() > 0) return true;
			readToBuffer();
			return bytesInBuff() > 0;
		}

		virtual bool canWrite() const {
			if (maskWrite) return false;
			return true;
		}

		virtual void closeOutput()  {
			if (!maskWrite) {
				flush();
				if (fd > 2 && maskRead) {
					close(fd);
					fd = -1;
				}
				maskWrite = true;
			}
		}

	protected:
		mutable bool maskRead;
		mutable bool maskWrite;
		static const natural peekBuffSize = 128;
		mutable byte peekBuff[peekBuffSize];
		mutable byte peekBuffUsage;
		mutable byte peekBuffPos;


	};

    StringA wideToUtf8(ConstStrW src) {
    	WideToUtf8Reader<ConstStrW::Iterator> rd1(src.getFwIter());
    	natural cnt = 0;
    	while (rd1.hasItems()) {cnt++;rd1.skip();}

    	StringA o;
    	StringA::WriteIterator wr = o.createBufferIter(cnt);
    	WideToUtf8Reader<ConstStrW::Iterator> rd2(src.getFwIter());
    	wr.copy(rd2);
    	return o;

    }

	void FileDescriptor::closeOutput()  {
		if (!maskWrite) {
			flush();
			if (fd > 2 && maskRead) {
				close(fd);
				fd = -1;
			}
			maskWrite = true;
		}
	}



    FileDescriptor::FileDescriptor(int fd, StringA name, bool deleteOnClose, bool maskRead, bool maskWrite)
        :FileDescriptorSeq(fd,name),deleteOnClose(deleteOnClose),maskRead(maskRead),
         maskWrite(maskWrite),cachedSize(lnaturalNull) {}

    FileDescriptor::~FileDescriptor() {
        if (deleteOnClose) {
            ::remove(this->name.c_str());
            if (fd > 2) {
                close(fd);
                fd = 0;
            }
        }

    }

//    static LinuxFileServices defaultSvc;
    static IFileIOServices  *currentSvc = &Singleton<LinuxFileServices,true>::getInstance();
    

    IFileIOServices &IFileIOServices::getIOServices() {
        return *currentSvc;
    }

    void IFileIOServices::setIOServices(IFileIOServices *newServices) {
        if (newServices == 0)
            currentSvc = &Singleton<LinuxFileServices,true>::getInstance();
        else
            currentSvc = newServices;
    }

    static void createFolders(StringA cname) {

    	natural pos = cname.findLast('/');
    	if (pos == naturalNull) return;
    	StringA dirName = cname.head(pos);
    	if (dirName.empty() || access(dirName.c_str(),F_OK) == 0 ) return;
    	try {
    		createFolders(dirName);
    	} catch (FolderCreationException &e) {
    		throw FolderCreationException(THISLOCATION,e.getErrNo(),String(dirName))
    				<< e;
    	}
    	if (mkdir(dirName.c_str(),0777) != 0)
    		throw FolderCreationException(THISLOCATION,errno,String(dirName));
    }

    StringA getFDName(int fd, bool canRead, bool canWrite, bool isPipe) {
    	char buff[256];
    	sprintf(buff,"#fd:%c%c%c%d",
    			canRead?'r':'-',
    			canWrite?'w':'-',
    			isPipe?'p':'f',
    			fd);
    	return buff;
    }

    class FileDescriptorCommitOnClose: public FileDescriptor {
    public:
    	FileDescriptorCommitOnClose(FileDescriptor *originDesc, String targetName)
    		:FileDescriptor(originDesc->fd, originDesc->name,false,
    				originDesc->maskWrite,originDesc->maskRead)
    		,targetFile(targetName)
    	{
    		originDesc->fd = -1;
    		delete originDesc;
    	}

    	void closeOutput() {
    		if (fd!=-1) {
    			flush();
    			IFileIOServices &svcs = IFileIOServices::getIOServices();
    			close(fd);
    			fd = -1;
/*    			String oldver = targetFile + ConstStrW(L".bak");
    			if (svcs.canOpenFile(targetFile,IFileIOServices::fileAccessible))
    				svcs.move(targetFile,oldver,true);*/
    			svcs.move(String(name),targetFile,true);
    		}
    	}

    	~FileDescriptorCommitOnClose() {
    		if (!std::uncaught_exception())
    			closeOutput();
    	}
    protected:
    	String targetFile;
    };

    static FileDescriptorSeq *safeOpenFile(ConstStrW ofn, int openFlags, OpenFlags::Type flags) {

    	using namespace OpenFlags;


        bool deleteOnClose = false;
        StringA cname =String::getUtf8(ofn);
        if (cname.length() > 4 && cname.head(4) == ConstStrA("#fd:")) {
        	int fx;
        	char r,w,p;
        	if (sscanf(cname.data(),"#fd:%c%c%c%d",&r,&w,&p,&fx) == 4) {
        		fcntl(fx,F_SETFD,0);
        		if (p == 'p')
        			return new PipeDescriptor(fx,cname,r == '-', w == '-');
        		else
        			return new FileDescriptor(fx,cname,false,r == '-', w == '-');
        	}
        }
    	if ((flags & commitOnClose) && (openFlags == O_WRONLY)) {
    		String tmpname = ofn+ConstStrW(L".new");
    		FileDescriptorSeq *f = safeOpenFile(tmpname,openFlags,(flags & ~commitOnClose & ~deleteOnClose) | truncate | create);
    		return new FileDescriptorCommitOnClose(
    				&dynamic_cast<FileDescriptor &>(*f),
    				ofn);
    	}

#ifdef O_CLOEXEC
        openFlags |= O_CLOEXEC;
#endif


    	int createFlags =  S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IWOTH|S_IROTH;
        if (flags & writeThrough) openFlags|=O_SYNC;
        if (flags & deleteOnClose) deleteOnClose = true;
        if (flags & append) openFlags|=O_APPEND;
        if (flags & create) {
        	openFlags|=O_CREAT;
        	if (flags & createFolder) try {
        		createFolders(cname);
        	} catch (Exception &e) {
        		throw FileOpenError(THISLOCATION,errno,ofn) << e;
        	}
        }
        if (flags & truncate) openFlags|=O_TRUNC;
        if (flags & newFile) openFlags|=O_EXCL;
        
        int fd = ::open(cname.c_str(), openFlags, createFlags);
        if (fd == -1) {
            throw FileOpenError(THISLOCATION,errno,ofn);
        }
#ifndef O_CLOEXEC
#ifdef FD_CLOEXEC
        fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
#endif
        return new FileDescriptor(fd,cname,deleteOnClose,
        		openFlags == O_WRONLY,openFlags == O_RDONLY);
        
    }
    
    natural FileDescriptor::read(void *buffer, 
                            natural size,
                            FileOffset offset) const {
        rep:;
        int rd = ::pread(fd,buffer,size,offset);
        if (rd == -1) {
            if (errno == EINTR)
                goto rep;
            else 
                throw FileIOError(THISLOCATION,errno,name);
        }
        return rd;
    }

    natural FileDescriptor::write(const void *buffer, 
                              natural size, 
                              FileOffset offset) {
        rep:;
        int wd = ::pwrite(fd,buffer,size, offset);
        if (wd == -1) {
            if (errno == EINTR)
                goto rep;
            else
                throw FileIOError(THISLOCATION, errno, name);
        }
        if (cachedSize != lnaturalNull && offset + size > cachedSize) {
        	cachedSize = lnaturalNull;
        }
        return wd;
    }

    natural FileDescriptorSeq::read(void *buffer,
                            natural size) {
        rep:;
        int rd = ::read(fd,buffer,size);
        if (rd == -1) {
            if (errno == EINTR)
                goto rep;
            else 
                throw FileIOError(THISLOCATION,errno,name);
        }
        return rd;
    }

    natural FileDescriptorSeq::write(const void *buffer,
                              natural size) {
        rep:;
        int wd = ::write(fd,buffer,size);
        if (wd == -1) {
            if (errno == EINTR)
                goto rep;
            else
                throw FileIOError(THISLOCATION, errno, name);
        }
        return wd;
    }

    natural FileDescriptor::write(const void *buffer,
                              natural size) {
    	cachedSize = lnaturalNull;
    	return FileDescriptorSeq::write(buffer,size);
    }

    void FileDescriptor::setSize(FileOffset size) {
        rep:;
        int wd = ftruncate(fd,size);
        if (wd == -1) {
            if (errno == EINTR)
                goto rep;
            else
                throw FileIOError(THISLOCATION, errno, name);
        }
        cachedSize = size;
    }


    FileDescriptor::FileOffset FileDescriptor::size() const {
    	if (cachedSize != lnaturalNull)
    		return cachedSize;
    	struct stat statBuff;
    	rep:
    	if (::fstat(this->fd,&statBuff) == -1) {
            if (errno == EINTR)
                goto rep;
            else
                throw FileIOError(THISLOCATION, errno, name);
        }
    	return cachedSize = statBuff.st_size;
        
    }

    PInOutStream LinuxFileServices::openSeqFile(
                    ConstStrW ofn, FileOpenMode mode, OpenFlags::Type flags)
    {
    	IFileIOHandler *h = findHandler(ofn);
    	if (h) return h->openSeqFile(ofn,mode,flags);

        if (mode == fileOpenRead) {
        	if (ofn == ConstStrW('-')) return openStdFile(stdInput);
        	else return safeOpenFile(ofn,O_RDONLY, flags);
        } else if (mode == fileOpenWrite) {
        	if (ofn == ConstStrW('-')) return openStdFile(stdOutput);
            return safeOpenFile(ofn,O_WRONLY, flags);
        } else
            throw;//TODO: invalid mode
        
    }

    PRndFileHandle LinuxFileServices::openRndFile(
                    ConstStrW ofn, FileOpenMode mode, OpenFlags::Type flags) {
    	PInputStream sh;

    	IFileIOHandler *h = findHandler(ofn);
    	if (h) return h->openRndFile(ofn,mode,flags);

        if (mode == fileOpenRead)
            sh = safeOpenFile(ofn,O_RDONLY, flags);
        else if (mode == fileOpenWrite)
            sh = safeOpenFile(ofn,O_WRONLY, flags);
        else
            sh = safeOpenFile(ofn,O_RDWR, flags);

        PRndFileHandle out(sh->getIfcPtr<IRndFileHandle>());
        if (out == nil)
        	throw InvalidParamException(THISLOCATION,1,"File cannot be opened in random access mode");
        return out;
    }



    PInOutStream LinuxFileServices::openStdFile(StdFile stdfile) {
        switch(stdfile) {
            case stdInput:return new PipeDescriptor(0,"#0 stdInput",false,true);
            case stdOutput:return new PipeDescriptor(1,"#1 stdOutput",true,false);
            case stdError:return new PipeDescriptor(2,"#2 stdError",true,false);
        }
        throw;//TODO: invalid mode
    }

#ifndef HAVE_PIPE2
#define HAVE_PIPE2 0
#endif

    int pipeCloseOnExec(int *fds) {

#if HAVE_PIPE2
        int res = pipe2(fds,O_CLOEXEC);
#else
        int res = pipe(fds);
#ifdef FD_CLOEXEC
        fcntl(fds[0], F_SETFD, FD_CLOEXEC);
        fcntl(fds[1], F_SETFD, FD_CLOEXEC);
#endif
#endif
        return res;
    }
    
    void LinuxFileServices::createPipe(PInputStream &readEnd, POutputStream &writeEnd){
        
        int fds[2];
        int res = pipeCloseOnExec(fds);
        if (res != 0)
            throw PipeOpenError(THISLOCATION,errno);


        char desc[100];
        
        sprintf(desc,"#pipe(%d:%d):read_end",fds[0],fds[1]);
        readEnd = new PipeDescriptor(fds[0],desc,false,true);
        sprintf(desc,"#pipe(%d:%d):write_end",fds[0],fds[1]);
        writeEnd = new PipeDescriptor(fds[1],desc,true,false);
    }

	FileDescriptor::FileOffset FileDescriptor::getPosition() const {
		rep:
		__off_t res = ::lseek(this->fd,0,SEEK_CUR);
		if (res == -1) {
			if (errno == EINTR)
				goto rep;
			else
				throw FileIOError(THISLOCATION,errno,this->name);
		}
		return res;
	}
	void FileDescriptor::setPosition(FileOffset ofs) {
		rep:
		__off_t res = ::lseek(this->fd,ofs,SEEK_SET);
		if (res == -1) {
			if (errno == EINTR)
				goto rep;
			else
				throw FileIOError(THISLOCATION,errno,this->name);
		}

	}

	natural FileDescriptor::peek(void *buffer, natural size) const {
		if (buffer == 0)
		{
			FileOffset pos = getPosition();
			if (cachedSize != lnaturalNull && cachedSize <= pos)
				cachedSize = lnaturalNull;
			FileOffset ofs = this->size() - pos;
			if (ofs > (lnatural)naturalNull) return naturalNull;
			else return (natural)ofs;
		}
		FileOffset ofs = getPosition();
		return read(buffer,size,ofs);

	}
	bool FileDescriptor::canRead() const {
		if (maskRead) return false;
		FileOffset pos = getPosition();
		if (cachedSize != lnaturalNull && cachedSize <= pos)
			cachedSize = lnaturalNull;
		return pos < size();

	}
	bool FileDescriptor::canWrite() const {
		return !maskWrite;
	}

	bool FileDescriptorSeq::lock(int flags, natural timeout) {
		if (timeout == 0) flags |= LOCK_NB;
		if (flock(this->fd,flags)) {
			int err = errno;
			if (err == EWOULDBLOCK) return false;
			else throw ErrNoException(THISLOCATION, err);
		}
		return true;
	}
	bool FileDescriptorSeq::lockShared(natural timeout) {
		return lock(LOCK_SH,timeout);
	}
	bool FileDescriptorSeq::lockExclusive(natural timeout) {
		return lock(LOCK_EX,timeout);
	}
	bool FileDescriptorSeq::lock(bool exclusive, natural timeout ) {
		int flag = exclusive?LOCK_EX:LOCK_SH;
		return lock(flag, timeout);
	}
	void FileDescriptorSeq::unlock() {
		if (flock(this->fd,LOCK_UN)) {
			int err = errno;
			throw ErrNoException(THISLOCATION, err);
		}
	}

	bool LinuxFileServices::canOpenFile(ConstStrW name, FileOpenMode mode) const {

    	IFileIOHandler *h = findHandler(name);
    	if (h) return h->canOpenFile(name,mode);


		StringA fname = wideToUtf8(name);
		switch (mode) {
		case IFileIOServices::fileAccessible: return ::access(fname.c_str(),F_OK) == 0;
		case IFileIOServices::fileOpenRead: return ::access(fname.c_str(),R_OK) == 0;
		case IFileIOServices::fileOpenWrite: return ::access(fname.c_str(),W_OK) == 0;
		case IFileIOServices::fileOpenReadWrite: return ::access(fname.c_str(),R_OK|W_OK) == 0;
		case IFileIOServices::fileExecutable: return ::access(fname.c_str(),X_OK) == 0;
		default: return false;
		}
	}

	PInOutStream LinuxFileServices::openSeqFile(const void *handle, size_t handleSize, FileOpenMode mode) {
		if (handleSize != sizeof(int)) throw InvalidParamException(THISLOCATION,2,"This is not handle");
		int fd = *(int *)handle;
		char buff[256];
		sprintf(buff,"Pipe/file #%d, access %s", fd,
				mode == fileOpenRead? "read":
						mode == fileOpenWrite?"write":
								"read/write");

		return new PipeDescriptor(fd,buff,mode == fileOpenWrite, mode == fileOpenRead);
	}
    PRndFileHandle LinuxFileServices::openRndFile(const void *handle, size_t handleSize, FileOpenMode mode) {
		if (handleSize != sizeof(int)) throw InvalidParamException(THISLOCATION,2,"This is not handle");
		int fd = *(int *)handle;
		char buff[256];
		sprintf(buff,"Pipe #%d, access %s", fd,
				mode == fileOpenRead? "read":
						mode == fileOpenWrite?"write":
								"read/write");

		return new FileDescriptor(fd,buff,false,mode == fileOpenWrite, mode == fileOpenRead);
    }


	size_t FileDescriptorSeq::getHandle(void *buffer, size_t bufferSize) {
		if (bufferSize != sizeof(int))  throw InvalidParamException(THISLOCATION,2,"This is not handle");
		*(int *)buffer = fd;
		return sizeof(int);

	}


	void FileDescriptorSeq::flush() {
		::fdatasync(fd);
	}

	class DirState: public IFolderIterator {

		typedef struct stat64 LinuxStat;
		class StatInfo: public LinuxStat, public ILinuxFileInfo {
		public:
			virtual const struct stat64 &getFileStat() const {return *this;}
		};

    	public:

    		DIR *dir;
    		mutable StatInfo statInfo;
    		struct dirent *entryInfo;
    		char *srcpath;
    		char *buffer;
    		wchar_t *wsrcpath;
    		wchar_t *wbuffer;
    		mutable bool statLoaded;
    		IFileIOServices &svc;

    		DirState(IFileIOServices &svc, ConstStrA name, size_t maxpath)
    			:dir(opendir(name.data()))
    			,entryInfo(reinterpret_cast<struct dirent *>(this+1))
    			,srcpath(reinterpret_cast<char *>(entryInfo) + offsetof(struct dirent, d_name) + maxpath)
    			,buffer(srcpath+name.length()+1)
    			,wsrcpath(reinterpret_cast<wchar_t *>(buffer + maxpath+1))
    			,wbuffer(wsrcpath+name.length()+1)
    			,statLoaded(false)
    			,svc(svc) {
    			if (dir == 0) throw FileOpenError(THISLOCATION, errno, name);
    			strcpy(srcpath,name.data());
    			srcpath[name.length()] = '/';
    			this->source = ConstStrW(wsrcpath,name.length());
    		}

    		void *operator new(size_t sz,size_t maxpath, size_t dirpathlen) {
    			size_t entryLen = offsetof(struct dirent, d_name) +
    					2*maxpath + dirpathlen + 1
    					+ sizeof(wchar_t) * (maxpath + dirpathlen + 1)
    					+ sz + 20;
    			return malloc(entryLen);
    		}
    		void operator delete(void *ptr,const char *) {
    			free(ptr);
    		}
    		void operator delete(void *ptr) {
    			free(ptr);
    		}

    		virtual ~DirState() {
    			closedir(dir);
    		}

    		bool getNext() {
    			struct dirent *res;
    			int i = readdir_r(dir,entryInfo,&res);
    			if (i) throw FileIOError(THISLOCATION,i,source);
    			if (res == 0) return false;
    			if (strcmp(res->d_name,".") == 0 || strcmp(res->d_name,"..") == 0) return getNext();
    			strcpy(buffer,res->d_name);
    			convert(ConstStrA(srcpath),wsrcpath);
    			this->entryName = ConstStrW(wbuffer);
    			link = false;
    			statLoaded = false;
    			switch (res->d_type) {
    			case DT_REG: type = file; break;
    			case DT_DIR: type = directory;break;
    			case DT_FIFO:
    			case DT_CHR:
    			case DT_SOCK: type = seqfile;break;
    			case DT_BLK: type = special;break;
    			case DT_LNK: link = true;
    						determineType();
    						break;
    			case DT_UNKNOWN:
    						determineType();
    						break;
    			}
    			return true;
    		}

    		void determineType() {
    			int i;
    			if (link == false) {
    				i = lstat64(srcpath,&statInfo);
    				if (i != 0) throw FileOpenError(THISLOCATION,errno,wsrcpath);
    				if (S_ISLNK(statInfo.st_mode)) {
    					link = true;
    					i = stat64(srcpath,&statInfo);
        				if (i != 0) throw FileOpenError(THISLOCATION,errno,wsrcpath);
    				}
    			} else {
					i = stat64(srcpath,&statInfo);
    				if (i != 0) FileOpenError(THISLOCATION,errno,wsrcpath);
    			}
    			if (S_ISREG(statInfo.st_mode)) type = file;
    			else if (S_ISDIR(statInfo.st_mode)) type = directory;
    			else if (S_ISFIFO(statInfo.st_mode) || S_ISCHR(statInfo.st_mode) || S_ISSOCK(statInfo.st_mode)) type = seqfile;
    			else if (S_ISBLK(statInfo.st_mode)) type = special;
    			else type = unknown;
    		}

    		void convert(ConstStrA src, wchar_t *wtrg) {

    			Utf8ToWideReader<ConstStrA::Iterator> rd(src.getFwIter());
    			while (rd.hasItems()) {
    				*wtrg++=rd.getNext();
    			}
    			*wtrg = 0;
    		}
        	virtual void rewind() {
        		rewinddir(dir);
        	}

        	virtual lnatural getSize() const {
        		loadStat();
        		return statInfo.st_size;
        	}

        	virtual const IFileInformation &getInfo() const {
        		loadStat();
        		return statInfo;
        	}

        	virtual TimeStamp getModifiedTime() const {
        		loadStat();
        		return TimeStamp::fromUnix(statInfo.st_mtim.tv_sec,statInfo.st_mtim.tv_nsec/1000000);
        	}

        	virtual IFileIOServices::FileOpenMode getAllowedOpenMode() const {
        		if (access(srcpath,R_OK) == 0)
        			if (access(srcpath, W_OK) == 0)
        				return IFileIOServices::fileOpenReadWrite;
        			else
        				return IFileIOServices::fileOpenRead;
        		else if (access(srcpath,W_OK) == 0)
        			return IFileIOServices::fileOpenWrite;
        		else
        			return IFileIOServices::fileAccessible;
        	}

        	void loadStat() const {
        		if (statLoaded) return;
        		if (stat64(srcpath,&statInfo))
        			 throw ErrNoException(THISLOCATION,errno);
        		statLoaded = true;
        	}

        	virtual PFolderIterator openFolder() const {
        		if (type != directory) throw FileMsgException(THISLOCATION,0,wbuffer,ConstStrW(L"Directory not found"));
        		size_t maxpath = pathconf(srcpath, _PC_NAME_MAX) + 1;
        		return new(maxpath,strlen(srcpath)) DirState(svc,srcpath,maxpath);
        	};

        	virtual String getFullPath() const {
        		return wsrcpath;
        	}

            virtual PInOutStream openSeqFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags);
    		virtual PRndFileHandle openRndFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags);
    		virtual void remove(bool recursive = false);
    		virtual PMappedFile mapFile(IFileIOServices::FileOpenMode mode);
    		virtual void copy(ConstStrW to, bool overwrite);
    		virtual void move(ConstStrW to, bool overwrite);

};


    PFolderIterator LinuxFileServices::openFolder(ConstStrW pathname)  {

    	IFileIOHandler *h = findHandler(pathname);
    	if (h) return h->openFolder(pathname);


    	StringA c = wideToUtf8(pathname);
    	if (c.empty()) c=".";
    	if (c.tail(1)[0] == '/') c = c + ConstStrA(".");
		size_t maxpath = pathconf(c.c_str(), _PC_NAME_MAX) + 1;
		return new(maxpath,c.length()) DirState(*this,c,maxpath);


    }


    void LinuxFileServices::createFolder(ConstStrW folderName, bool parents) {

    	IFileIOHandler *h = findHandler(folderName);
    	if (h) return h->createFolder(folderName,parents);


    	StringA fn = wideToUtf8(folderName);
    	if (parents) createFolders(fn);
		if (::mkdir(fn.c_str(),0777) == -1) {
			int err = errno;
			throw FolderCreationException(THISLOCATION,err,folderName);
		}
    }
    void LinuxFileServices::removeFolder(ConstStrW folderName, bool recursive) {

    	IFileIOHandler *h = findHandler(folderName);
    	if (h) return h->removeFolder(folderName,recursive);


    	StringA fn = wideToUtf8(folderName);
    	if (::rmdir(fn.c_str()) == -1) {
    		int err = errno;
    		if (err == ENOTEMPTY && recursive && !folderName.empty()
    				&& folderName!=ConstStrW(L"/") ) {

    			try {
					PFolderIterator iter = openFolder(folderName);
					while (iter->getNext()) {
						if (!iter->link && iter->type == iter->directory) {
							removeFolder(iter->getFullPath(),true);
						} else {
							remove(iter->getFullPath());
						}
					}
    			} catch (const Exception &e) {
    				throw FileIOError(THISLOCATION,EFAULT,folderName) << e;
    			}
    			if (::rmdir(fn.c_str()) == -1)
    				throw FileIOError(THISLOCATION,errno,folderName);

    		} else {
    			throw FileIOError(THISLOCATION, err, folderName);
    		}
    	}


    }
    void LinuxFileServices::copy(ConstStrW from, ConstStrW to, bool overwrite) {

    	IFileIOHandler *h1 = findHandler(from);
    	IFileIOHandler *h2 = findHandler(to);
    	if (h1 != h2) throwUnsupportedFeature(THISLOCATION, this, "Cannot copy files between various filesystems");
    	if (h1) return h1->copy(from,to,overwrite);

    	StringA f = wideToUtf8(from);
    	StringA t = wideToUtf8(to);
    	int ifd = open(f.cStr(),O_RDONLY|O_LARGEFILE|O_CLOEXEC);
    	if (ifd == -1) {int e = errno; throw FileIOError(THISLOCATION,e,from);}
    	struct stat st;
    	if (fstat(ifd,&st) == -1) {int e = errno;close(ifd);throw FileIOError(THISLOCATION,e,from);}
    	int ofd = open(t.cStr(),O_WRONLY|O_LARGEFILE|O_CLOEXEC|O_CREAT|(overwrite?O_TRUNC:O_EXCL), st.st_mode);
    	if (ofd == -1) {int e = errno;close(ifd);throw FileIOError(THISLOCATION,e,to);}

    	int err = 0;
    	ssize_t res = sendfile(ofd,ifd,0,st.st_size);
    	if (res == -1) err = errno;
    	close(ifd);
    	close(ofd);

    	if (err) throw FileCopyException(THISLOCATION, err, from, to);
    }
    void LinuxFileServices::move(ConstStrW from, ConstStrW to, bool overwrite) {
    	if (from == to) return;

    	IFileIOHandler *h1 = findHandler(from);
    	IFileIOHandler *h2 = findHandler(to);
    	if (h1 != h2) throwUnsupportedFeature(THISLOCATION, this, "Cannot move files between various filesystems");
    	if (h1) return h1->move(from,to,overwrite);

    	if (!overwrite && canOpenFile(to,fileAccessible)) {
    		throw FileIOError(THISLOCATION, EEXIST,to);
    	}

    	if (::rename(wideToUtf8(from).cStr(),wideToUtf8(to).cStr()) == -1) {
    		int e = errno;
    		if (e == EXDEV) {try {
    			copy(from,to,overwrite);
    			remove(from);
    		}
    		catch (const Exception &e) {
    			throw FileMsgException(THISLOCATION,EFAULT,to,ConstStrA("Exception while trying to move using copying")) << e;
    		}
    		} else throw FileIOError(THISLOCATION, e, from);
    	}
    }

    void LinuxFileServices::link(ConstStrW linkName, ConstStrW target, bool overwrite) {

    	IFileIOHandler *h1 = findHandler(linkName);
    	IFileIOHandler *h2 = findHandler(target);
    	if (h1 != h2) throwUnsupportedFeature(THISLOCATION, this, "Cannot link files between various filesystems");
    	if (h1) return h1->link(linkName,target,overwrite);


    	if (::symlink(wideToUtf8(target).cStr(),wideToUtf8(linkName).cStr()) == -1) {
    		int e = errno;
    		if (e == EEXIST && overwrite) {
    			try {
    				remove(linkName);
    			} catch (const Exception &e) {
    				throw FileMsgException(THISLOCATION,EFAULT,linkName,ConstStrA("Exception while trying to remove target file")) << e;
    			}
    			link(linkName,target,false);
    		} else {
    			throw FileIOError(THISLOCATION, e, linkName);
    		}
    	}
    }
    void LinuxFileServices::remove(ConstStrW what) {

    	IFileIOHandler *h1 = findHandler(what);
    	if (h1) return h1->remove(what);

    	if (::remove(wideToUtf8(what).cStr()) == -1) {
    		int e = errno;
    		throw FileIOError(THISLOCATION, e, what);
    	}

    }
    PMappedFile LinuxFileServices::mapFile(ConstStrW filename, FileOpenMode mode) {

    	IFileIOHandler *h1 = findHandler(filename);
    	if (h1) return h1->mapFile(filename,mode);


    	class Mapped: public IMappedFile {
    	public:

    		Mapped(int fd, String fname):fd(fd),fname(fname) {}
    		virtual ~Mapped() {close(fd);}

	        int getProto(IFileIOServices::FileOpenMode mode)
	        {
	        	int proto;
	            switch (mode) {
        		case IFileIOServices::fileOpenRead:proto = PROT_READ;break;
        		case IFileIOServices::fileOpenWrite:proto = PROT_WRITE;break;
        		case IFileIOServices::fileOpenReadWrite:proto = PROT_WRITE|PROT_READ;break;
        		default:proto = PROT_NONE;break;
        		}
	            return proto;
	        }

    		virtual MappedRegion map(IFileIOServices::FileOpenMode mode,
    													bool copyOnWrite) {

    			int proto = getProto(mode);
    			struct stat64 st;
    			if (fstat64(fd,&st)) throw FileMsgException(THISLOCATION,errno,fname,"stat failed");

    			if (st.st_size == 0) {
    				return MappedRegion(this,0,0,0);
    			}

        		void *res = mmap(0,st.st_size,proto, (copyOnWrite?MAP_PRIVATE:MAP_SHARED),fd,0);
        		if (res == MAP_FAILED) {
        			int e = errno;
        			throw FileMsgException(THISLOCATION,e,fname,"MMAP failed");
        		}
        		return MappedRegion(this,res,st.st_size,0);


    		}


	        virtual MappedRegion map(IRndFileHandle::FileOffset offset, natural size, IFileIOServices::FileOpenMode mode, bool copyOnWrite)
	        {
	            int proto = getProto(mode);

        		//TODO: fix offset

        		void *res = mmap(0,size,proto, (copyOnWrite?MAP_PRIVATE:MAP_SHARED),fd,offset);
        		if (res == MAP_FAILED) {
        			int e = errno;
        			throw FileMsgException(THISLOCATION,e,fname,"MMAP failed");
        		}
        		return MappedRegion(this,res,size,offset);

        	}
        	virtual void unmap(MappedRegion &reg) {
        		if (reg.size == 0) return;
        		munmap(reg.address,reg.size);
        	}
        	virtual void sync(MappedRegion &reg) {
        		if (reg.size == 0) return;
        		msync(reg.address,reg.size,MS_SYNC);
        	}
        	virtual void lock(MappedRegion &reg) {
        		if (reg.size == 0) return;
        		mlock(reg.address,reg.size);
        	}
        	virtual void unlock(MappedRegion &reg) {
        		if (reg.size == 0) return;
        		munlock(reg.address,reg.size);
        	}


    	protected:
    		int fd;
    		String fname;

    	};

    	try {
    		PRndFileHandle f = openRndFile(filename,mode,0);
    		IFileExtractHandle *ihnd = dynamic_cast<IFileExtractHandle *>(f.get());
    		if (ihnd == 0) throw FileMsgException(THISLOCATION,EFAULT,filename,"File cannot be mapped");
    		int fd;
    		ihnd->getHandle(&fd,sizeof(fd));
    		return new Mapped(dup(fd),filename);


    	} catch (Exception &e) {
    		throw FileMsgException(THISLOCATION,EFAULT,filename,String("Mapping has failed")) << e;
    	}

    }

    String LinuxFileServices::findFile(ConstStrW filename, ConstStrW pathList,natural position, bool exec) {

    	ConstStrW::SplitIterator iter = pathList.split(':');
    	while (iter.hasItems())  {
    		ConstStrW x = iter.getNext();
    		String search;
    		if (!x.empty() && x[x.length()-1] != '/')
    			search = x + ConstStrW(L"/") + filename;
    		else
    			search = x + filename;
    		if ((exec && canOpenFile(search,fileExecutable))
    		   || (!exec && canOpenFile(search,fileAccessible))) {
    				if (position < 2) return search;
    				else position--;
    		}
    	}
    	return String(filename);
    }
    String LinuxFileServices::findFile(ConstStrW filename, ConstStrW pathList,natural position) {
    	return findFile(filename,pathList,position,false);
    }
    String LinuxFileServices::findExec(ConstStrW execName) {
    	String path = getenv("PATH");
    	return findFile(execName,path,1,true);
    }

    PTemporaryFile LinuxFileServices::createTempFile(ConstStrW prefix, bool rndfile) {
		TextFormatBuff<wchar_t, StaticAlloc<256> > fmt;
		if (rndfile) {
			static atomic counter = 0;
			atomic lc = lockInc(counter);
			fmt("/tmp/%1~%2~%3") << prefix << (natural)getpid() << (natural)lc;
			String filename(fmt.write());
			PInOutStream hnd = openSeqFile(filename,fileOpenWrite,
					OpenFlags::create|OpenFlags::truncate|OpenFlags::temporary|OpenFlags::shareRead|OpenFlags::shareWrite|OpenFlags::shareDelete);
			return new TemporaryFile(*this,hnd,filename);
		} else {
			TextFormatBuff<wchar_t> fmt;
			fmt("/tmp/%1") << prefix;
			String filename(fmt.write());
			try {
				PInOutStream hnd = openSeqFile(filename,fileOpenWrite,
					OpenFlags::create|OpenFlags::temporary|OpenFlags::shareRead|OpenFlags::shareWrite);
				return new TemporaryFile(*this,hnd,filename);
			} catch (FileOpenError &e) {
				if (e.getErrNo() == EEXIST)
					return new TemporaryFile(*this,nil,filename);
				else
					throw;
			}
		}
    }

    static StringA getCwd() {
    	AutoArray<char , SmallAlloc<256> > buffer;
    	buffer.resize(256);
    	char *cwd = getcwd(buffer.data(),buffer.length());
    	while (cwd == 0){
    		int e = errno;
    		if (e != ERANGE) throw ErrNoException(THISLOCATION,e);
    		buffer.resize(buffer.length()*3/2);
    		cwd = getcwd(buffer.data(),buffer.length());
    	}
    	natural len = strlen(cwd);
    	if (cwd[len-1] != '/') {cwd[len] = '/';len++;}
    	ConstStrA strcwd(cwd,len);
    	return strcwd;

    }

    typedef struct stat64 FileStat;

    	class FileInfo: public IFolderIterator, public ILinuxFileInfo {
    	public:
    		FileInfo(IFileIOServices &svc, ConstStrW pathname, const FileStat &statinfo)
    			:svc(svc),statinfo(statinfo),fullname(pathname) {


    			natural k = pathname.findLast('/');
    			if (k != naturalNull) {
    				source = pathname.head(k);
    				entryName = pathname.offset(k+1);
    			} else {
    				source = ConstStrW();
    				entryName=pathname;
    			}

    			if (entryName == ConstStrW(L".") || entryName == ConstStrW(L".."))
    				type = dots;
    			else if (S_ISDIR(statinfo.st_mode))
    				type = directory;
    			else if (S_ISREG(statinfo.st_mode))
    				type = file;
    			else if (S_ISSOCK(statinfo.st_mode) || S_ISFIFO(statinfo.st_mode))
    				type = seqfile;
    			else if (S_ISCHR(statinfo.st_mode) || S_ISBLK(statinfo.st_mode))
    				type = special;
    			else
    				type = unknown;

    			link = S_ISLNK(statinfo.st_mode) != 0;
    			hidden = entryName[0] == '.';

    		}
        	virtual bool getNext() {return false;}

        	virtual void rewind() {};

        	virtual lnatural getSize() const {
        		return statinfo.st_size;
        	}
        	virtual const IFolderIterator::IFileInformation &getInfo() const {
        		return *this;
        	}
        	virtual TimeStamp getModifiedTime() const {
        		return TimeStamp::fromUnix(statinfo.st_mtim.tv_sec,statinfo.st_mtim.tv_nsec/1000000);
        	}
        	virtual IFileIOServices::FileOpenMode getAllowedOpenMode() const {
        		if (svc.canOpenFile(fullname,IFileIOServices::fileOpenRead))
        			return IFileIOServices::fileOpenRead;
        		if (svc.canOpenFile(fullname,IFileIOServices::fileOpenReadWrite))
        			return IFileIOServices::fileOpenReadWrite;
        		if (svc.canOpenFile(fullname,IFileIOServices::fileOpenWrite))
        			return IFileIOServices::fileOpenWrite;
        		return IFileIOServices::fileAccessible;
        	}
        	virtual PFolderIterator openFolder() const {
        		return svc.openFolder(fullname);
        	}

        	virtual String getFullPath() const {return fullname;}

    		virtual const struct stat64 &getFileStat() const {
    			return statinfo;
    		}

        virtual PInOutStream openSeqFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags);
		virtual PRndFileHandle openRndFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags);
		virtual void remove(bool recursive = false);
		virtual PMappedFile mapFile(IFileIOServices::FileOpenMode mode);
		virtual void copy(ConstStrW to, bool overwrite);
		virtual void move(ConstStrW to, bool overwrite);


        protected:
        	IFileIOServices &svc;
        	FileStat statinfo;
        	String fullname;

    	};


    PFolderIterator LinuxFileServices::getFileInfo(ConstStrW pathname)  {
    	StringA cname = String::getUtf8(pathname);
    	bool extended;
    	if (cname.empty() || cname[0] != '/') {cname = getCwd() + cname;extended = true;} else {extended = false;}
    	FileStat fst;
    	if (stat64(cname.c_str(),&fst)) {
    		int en = errno;
    		throw FileIOError(THISLOCATION,en,cname);
    	}
    	return new FileInfo(*const_cast<LinuxFileServices *>(this),extended?String(cname):String(pathname),fst);
    }


    LightSpeed::PInOutStream DirState::openSeqFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags)
    {
    	return svc.openSeqFile(getFullPath(),mode,flags);
    }

    LightSpeed::PRndFileHandle DirState::openRndFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags)
    {
    	return svc.openRndFile(getFullPath(),mode,flags);
    }

    void DirState::remove(bool recursive /*= false*/)
    {
    	if (type == directory) svc.removeFolder(getFullPath(),recursive);
    	else svc.remove(getFullPath());
    }

    LightSpeed::PMappedFile DirState::mapFile(IFileIOServices::FileOpenMode mode)
    {
    	return svc.mapFile(getFullPath(),mode);
    }

    void DirState::copy(ConstStrW to, bool overwrite)
    {
    	svc.copy(getFullPath(),to,overwrite);
    }

    void DirState::move(ConstStrW to, bool overwrite)
    {
    	svc.move(getFullPath(),to,overwrite);
    }




    LightSpeed::PInOutStream FileInfo::openSeqFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags)
    {
    	return svc.openSeqFile(getFullPath(),mode,flags);
    }

    LightSpeed::PRndFileHandle FileInfo::openRndFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags)
    {
    	return svc.openRndFile(getFullPath(),mode,flags);
    }

    void FileInfo::remove(bool recursive /*= false*/)
    {
    	if (type == directory) svc.removeFolder(getFullPath(),recursive);
    	else svc.remove(getFullPath());
    }

    LightSpeed::PMappedFile FileInfo::mapFile(IFileIOServices::FileOpenMode mode)
    {
    	return svc.mapFile(getFullPath(),mode);
    }

    void FileInfo::copy(ConstStrW to, bool overwrite)
    {
    	svc.copy(getFullPath(),to,overwrite);
    }

    void FileInfo::move(ConstStrW to, bool overwrite)
    {
    	svc.move(getFullPath(),to,overwrite);
    }

}
