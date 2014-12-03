#pragma once

#include "../streams/fileio_ifc.h"
#include "winhttp.h"

namespace LightSpeed {

	class WindowsFileCommon: public IFileExtractHandle {

	public:
		WindowsFileCommon(String fileName, HANDLE hFile);
		~WindowsFileCommon();

		virtual size_t getHandle(void *buffer, size_t bufferSize);

		String getName() const {return fileName;}

	protected:
		String fileName;
		String inheritName;
		HANDLE hFile;
		
	};

	class WindowsFile: public ISeqFileHandle,
//		public ILockFileInterface,
		public IRndFileHandle,
		public WindowsFileCommon
	{
	public:

		///handle must be asynchronous
		WindowsFile(String fileName, HANDLE hFile, 
			bool enableWrite, bool enableRead);

		virtual natural read(void *buffer,  natural size);
		virtual natural write(const void *buffer,  natural size);
		virtual natural peek(void *buffer, natural size) const;

		virtual bool canRead() const;
		virtual bool canWrite() const;
		virtual void flush();
		virtual natural dataReady() const;
// 		virtual bool lockShared(natural timeout = naturalNull);
// 		virtual bool lockExclusive(natural timeout = naturalNull);
// 		virtual bool lock(bool exclusive, natural timeout = naturalNull);
// 		virtual void unlock();
		virtual natural read(void *buffer,  natural size, FileOffset offset) const;
		virtual natural write(const void *buffer,  natural size, FileOffset offset);
		virtual void setSize(FileOffset size);
		virtual FileOffset size() const;
		virtual void closeOutput();


	protected:


		void getCurPosition( LARGE_INTEGER &curpos ) const;
		FileOffset getCurPosition() const;

		bool enableWrite;
		bool enableRead;


	};


	struct PipeOverlappedInfo;

	class WindowsPipe: public ISeqFileHandle,
					   public WindowsFileCommon{
	public:
		WindowsPipe(String pipeName, HANDLE hPipe, 
						bool output);

		~WindowsPipe();
		virtual natural read(void *buffer,  natural size);

		virtual natural postIO( BOOL res, OVERLAPPED &ovr, DWORD rd );
		virtual natural write(const void *buffer,  natural size);
		virtual natural peek(void *buffer, natural size) const;

		virtual bool canRead() const;
		virtual bool canWrite() const;
		virtual void flush();
		virtual natural dataReady() const;
		void createPipeOtherSide( PSeqFileHandle & otherEnd);

		virtual void closeOutput();

		HANDLE getWaitHandle() const {return hEvent;}

	protected:	
		HANDLE hEvent;
		bool output;

	};

	class WindowsConsole: public ISeqFileHandle,
						  public WindowsFileCommon {
	public:

		WindowsConsole(String name, HANDLE hConsole, bool output);

		virtual natural read(void *buffer,  natural size);
		virtual natural write(const void *buffer,  natural size);
		virtual natural peek(void *buffer, natural size) const;

		virtual bool canRead() const;
		virtual bool canWrite() const;
		virtual void flush();
		virtual natural dataReady() const;
		virtual void closeOutput();


	protected:
		const wchar_t * getSharingNameSpec() const;

		mutable byte peekBuff[80];
		mutable natural peekBuffSz;
		bool output;

	};

	class WindowsStdConsole: public WindowsConsole {
	public:
		WindowsStdConsole(String name, DWORD type);

		virtual natural read(void *buffer,  natural size);

		virtual natural write(const void *buffer,  natural size);
		virtual natural peek(void *buffer, natural size) const;

		virtual size_t getHandle(void *buffer, size_t bufferSize);

	protected:
		DWORD type;
		void updateHandle();

	};

	class WindowsFileService: public IFileIOServices {
	public:

		virtual PSeqFileHandle openSeqFile(ConstStrW fname,  FileOpenMode mode, OpenFlags::Type flags) ;
		virtual PSeqFileHandle openStdFile(StdFile stdfile) ;
		virtual void createPipe(PSeqFileHandle &readEnd, PSeqFileHandle &writeEnd) ;
		virtual PRndFileHandle openRndFile(ConstStrW ofn, FileOpenMode mode, OpenFlags::Type flags) ;
		virtual PSeqFileHandle openSeqFile(const void *handle, size_t handleSize, FileOpenMode mode);

		PSeqFileHandle openSeqFileByHandle( HANDLE h, FileOpenMode mode, String name );
		virtual PRndFileHandle openRndFile(const void *handle, size_t handleSize, FileOpenMode mode) ;
		virtual bool canOpenFile(ConstStrW name, FileOpenMode mode) const ;
		virtual PDirectoryIterator openDirectory(ConstStrW pathname);
		virtual PDirectoryIterator getFileInfo(ConstStrW pathname);
		virtual void createFolder(ConstStrW folderName, bool parents)  ;
		virtual void removeFolder(ConstStrW folderName, bool recursive) ;
		virtual void copy(ConstStrW from, ConstStrW to, bool overwrite) ;
		virtual void move(ConstStrW from, ConstStrW to, bool overwrite) ;
		virtual void link(ConstStrW linkName, ConstStrW target, bool overwrite) ;
		virtual void remove(ConstStrW what) ;
		virtual PMappedFile mapFile(ConstStrW filename, FileOpenMode mode) ;
		virtual String findFile(ConstStrW filename, ConstStrW pathList,
			natural position = 1)  ;
		virtual String findExec(ConstStrW execName) ;
		virtual PTemporaryFile createTempFile(ConstStrW prefix, bool randomfile = true);

		void WindowsFileService::copyDirectories( ConstStrW from, ConstStrW to );

		HTTPSettings httpSettings,httpsSettings;
	};

	class WindowsDirectoryIterator: public IDirectoryIterator {
	public:
		virtual bool getNext();
		virtual void rewind();
		virtual lnatural getSize() const ;
		virtual const IFileInformation &getInfo() const;
		virtual TimeStamp getModifiedTime() const ;
		virtual IFileIOServices::FileOpenMode getAllowedOpenMode() const ;
		virtual PDirectoryIterator openDirectory() const ;
		virtual String getFullPath() const;

		WindowsDirectoryIterator(ConstStrW path, IFileIOServices &svc, bool thisFile);
		~WindowsDirectoryIterator();
		void updateFindData();

		virtual PSeqFileHandle openSeqFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags);

		virtual PRndFileHandle openRndFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags);

		virtual void remove(bool recursive = false);

		virtual PMappedFile mapFile(IFileIOServices::FileOpenMode mode);

		virtual void copy(ConstStrW to, bool overwrite);

		virtual void move(ConstStrW to, bool overwrite);

		class FileInfo: public IFileInformation {
		public:
			WIN32_FIND_DATAW fndData;
		};

	protected:
		FileInfo fileInfo;
		HANDLE hFind;
		String path;
		bool thisFile;
		bool eof;		
		IFileIOServices &svc;

	};

	class WindowsMappedFile: public IMappedFile {
	public:

		WindowsMappedFile(ConstStrW filename, IFileIOServices::FileOpenMode mode);
		WindowsMappedFile(HANDLE hMapping, HANDLE hFile);
		~WindowsMappedFile();

		virtual MappedRegion map(IRndFileHandle::FileOffset offset, natural size,
			IFileIOServices::FileOpenMode mode, bool copyOnWrite);
		virtual MappedRegion map(IFileIOServices::FileOpenMode mode, 
			bool copyOnWrite) ;

		DWORD getMapAccess( IFileIOServices::FileOpenMode mode, bool copyOnWrite );
	protected:
		HANDLE hFile;
		HANDLE hMapping;

		static DWORD allocGrain;

		virtual void unmap(MappedRegion &reg);
		virtual void sync(MappedRegion &reg);
		virtual void lock(MappedRegion &reg);
		virtual void unlock(MappedRegion &reg);
	};

	
	String getFDName(HANDLE fd, bool canRead, bool canWrite);
}



