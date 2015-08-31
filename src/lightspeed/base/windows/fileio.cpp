#include "winpch.h"
#include "fileio.h"
#include "../exceptions/fileExceptions.h"
#include "../text/textFormat.tcc"
#include "../memory/smallAlloc.h"
#include "../exceptions/invalidParamException.h"
#include "../memory/staticAlloc.h"
#include "../interface.tcc"
#include <algorithm>
#include "../countof.h"
#include "../exceptions/errorMessageException.h"

#undef min
#undef max

namespace LightSpeed{

static	bool checkZeroAtString( ConstStrW str ) 
{
	__try {
		if (str.data() == 0) return false;
		return (str.data()[str.length()] == 0);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}


WindowsFileCommon::WindowsFileCommon( String fileName, HANDLE hFile)
									 :fileName(fileName),hFile(hFile)
{
}


WindowsFileCommon::~WindowsFileCommon()
{
	if (hFile) CloseHandle(hFile);
}

WindowsFile::WindowsFile( String fileName, HANDLE hFile, 
						bool enableWrite, bool enableRead )
	:WindowsFileCommon(fileName,hFile),enableWrite(enableWrite)
		,enableRead(enableRead)
{
}


static inline DWORD limitSize(natural size) {
	return DWORD(size>0x7FFFFFFF?0x7FFFFFFF:size);
}

natural WindowsFile::read( void *buffer, natural size )
{

	DWORD rd;
	BOOL res = ReadFile(hFile,buffer,limitSize(size),&rd,0);
	if (res == FALSE) {
		DWORD err = GetLastError();
		if (err == ERROR_HANDLE_EOF) {
			return 0;
		} else {
			throw FileIOError(THISLOCATION,err,fileName);
		}
	}
	return rd;
}

natural WindowsFile::read( void *buffer, natural size, FileOffset offset ) const
{
	OVERLAPPED ovr;
	initStruct(ovr);
	ovr.Offset = reinterpret_cast<LARGE_INTEGER *>(&offset)->LowPart;
	ovr.OffsetHigh = reinterpret_cast<LARGE_INTEGER *>(&offset)->HighPart;
	DWORD rd;
	BOOL res = ReadFile(hFile,buffer,limitSize(size),&rd,&ovr);
	if (res == FALSE) {
		DWORD err = GetLastError();
		if (err == ERROR_HANDLE_EOF) {
			return 0;
		} else {
			throw FileIOError(THISLOCATION,err,fileName);
		}
	} else 
		return rd;

}
natural WindowsFile::write( const void *buffer, natural size )
{

	DWORD wr;
	BOOL res = WriteFile(hFile,buffer,limitSize(size),&wr,0);
	if (res == FALSE) {
		DWORD err = GetLastError();
		throw FileIOError(THISLOCATION,err,fileName);
	}
	return wr;
}

natural WindowsFile::write( const void *buffer, natural size, FileOffset offset )
{
	OVERLAPPED ovr;
	initStruct(ovr);
	ovr.Offset = reinterpret_cast<LARGE_INTEGER *>(&offset)->LowPart;
	ovr.OffsetHigh = reinterpret_cast<LARGE_INTEGER *>(&offset)->HighPart;
	DWORD rd;
	BOOL res = WriteFile(hFile,buffer,limitSize(size),&rd,&ovr);
	if (res == FALSE) {
		DWORD err = GetLastError();
		throw FileIOError(THISLOCATION,err,fileName);
	} else 
		return rd;
}
natural WindowsFile::peek( void *buffer, natural size ) const
{
	natural x = const_cast<WindowsFile *>(this)->read(buffer,size);
	SetFilePointer(hFile,-LONG(x),0,FILE_CURRENT);
	return x;
}

bool WindowsFile::canRead() const
{
	if (enableRead) {
		LARGE_INTEGER curPos;
		LARGE_INTEGER zero;
		LARGE_INTEGER size;

		initStruct(zero);
		BOOL res = SetFilePointerEx(hFile,zero,&curPos,FILE_CURRENT);
		if (res == FALSE) throw FileIOError(THISLOCATION,GetLastError(),fileName);
		res = GetFileSizeEx(hFile,&size);
		if (res == FALSE) throw FileIOError(THISLOCATION,GetLastError(),fileName);
		return curPos.HighPart < size.HighPart ||
			curPos.HighPart == size.HighPart && curPos.LowPart < size.LowPart;
	} else {
		return false;
	}
}

bool WindowsFile::canWrite() const
{
	return enableWrite;
}

void WindowsFile::flush()
{
	BOOL res = FlushFileBuffers(hFile);
	if (res == FALSE) throw FileIOError(THISLOCATION,GetLastError(),fileName);

}

natural WindowsFile::dataReady() const
{
	return 0;
}

void WindowsFile::getCurPosition( LARGE_INTEGER &curpos ) const
{
	LARGE_INTEGER zero;
	initStruct(zero);
	BOOL res = SetFilePointerEx(hFile,zero,&curpos,FILE_CURRENT);
	if (res == FALSE) throw FileIOError(THISLOCATION,GetLastError(),fileName);
}

WindowsFile::FileOffset WindowsFile::getCurPosition() const
{
	FileOffset ofs;
	getCurPosition(*reinterpret_cast<LARGE_INTEGER *>(&ofs));
	return ofs;		
}
size_t WindowsFileCommon::getHandle( void *buffer, size_t bufferSize )
{
	if (buffer == 0) return sizeof(HANDLE);
	if (bufferSize < sizeof(HANDLE)) return 0;
	*reinterpret_cast<HANDLE *>(buffer) = hFile;
	return sizeof(HANDLE);
}


String getFDName(HANDLE fd, bool canRead, bool canWrite) {

	TextFormatBuff<wchar_t, SmallAlloc<1024> > fmt;
	fmt.setBase(16)(L"handle://%1%%2") << (Bin::natural64)fd <<
				(canRead?(canWrite?"rw":"ro"):(canWrite?"wo":"oo"));
	return String(fmt.write());
}

void WindowsFile::setSize( FileOffset size )
{

	SetFilePointerEx(hFile,*reinterpret_cast<LARGE_INTEGER *>(&size),0,FILE_BEGIN);
	SetEndOfFile(hFile);

}

WindowsFile::FileOffset WindowsFile::size() const
{
	LARGE_INTEGER cursz;
	BOOL res = GetFileSizeEx(hFile,&cursz);
	if (res == FALSE) throw FileIOError(THISLOCATION,GetLastError(),fileName);
	return *reinterpret_cast<FileOffset *>(&cursz);
}

void WindowsFile::closeOutput()
{
	enableWrite = false;
}

struct PipeOverlappedInfo {
	DWORD dwError;
	DWORD dwBytes;
	PipeOverlappedInfo ():dwBytes(INFINITE) {}
	operator bool() const {return dwBytes == INFINITE;}
};

static VOID CALLBACK winPipeComplettion(
									  __in     DWORD dwErrorCode,
									  __in     DWORD dwNumberOfBytesTransfered,
									  __inout  LPOVERLAPPED lpOverlapped
									  ) {
	PipeOverlappedInfo *bb = (PipeOverlappedInfo *)(lpOverlapped->hEvent);
	bb->dwBytes = dwNumberOfBytesTransfered;
	bb->dwError = dwErrorCode;
}



natural WindowsPipe::read( void *buffer, natural size)
{
	PipeOverlappedInfo info;
	OVERLAPPED ovr;
	initStruct(ovr).hEvent = hEvent;
	ResetEvent(hEvent);
	DWORD rd;
	BOOL res = ReadFile(hFile,buffer,limitSize(size),&rd,&ovr);
	return postIO(res, ovr, rd);
}

natural WindowsPipe::write( const void *buffer, natural size )
{
	PipeOverlappedInfo info;
	OVERLAPPED ovr;
	initStruct(ovr).hEvent = hEvent;
	ResetEvent(hEvent);
	DWORD rd;
	BOOL res = WriteFile(hFile,buffer,limitSize(size),&rd,&ovr);
	return postIO(res, ovr, rd);
}

natural WindowsPipe::peek( void *buffer, natural size ) const
{
	DWORD bb;
	BOOL res = PeekNamedPipe(hFile,buffer,limitSize(size),&bb,0,0);
	if (res == FALSE) {
		DWORD err = GetLastError();
		if (err == ERROR_BROKEN_PIPE) return 0;
		throw FileIOError(THISLOCATION,err,fileName);
	} else  {
		if (bb == 0) {
			ReadFile(hFile,0,0,&bb,0);//should block until data are ready
			return peek(buffer,size);
		}
		return bb;
	}
}

bool WindowsPipe::canRead() const
{
	if (!output && hFile != 0) {
		byte k;
		return peek(&k,1) != 0;
	} else {
		return false;
	}
}

bool WindowsPipe::canWrite() const
{
	return output && hFile != 0;
}

void WindowsPipe::flush()
{
	//NOTHING
}

natural WindowsPipe::dataReady() const
{
	//not for pipes
	return 0;
}


WindowsPipe::WindowsPipe( String pipeName, HANDLE hPipe, bool output )
:WindowsFileCommon(pipeName,hPipe),output(output)
{
	hEvent = CreateEvent(0,TRUE,0,0);

/*	///Detect whether pipe is async
	HANDLE hDup;
	DuplicateHandle(GetCurrentProcess(),hPipe,GetCurrentProcess(),&hDup,0,
		FALSE,DUPLICATE_SAME_ACCESS);
	HANDLE hCompl = CreateIoCompletionPort(hDup,0,0,0);
	if (hCompl == NULL) {
		hndType = sync;
		CloseHandle(hDup);
	} else {
		hndType = async;
		CloseHandle(hDup);
		CloseHandle(hCompl);
	}
*/}

void WindowsPipe::createPipeOtherSide( PInOutStream & otherEnd)
{
	OVERLAPPED ovr;
	HANDLE hEvent = CreateEvent(0,TRUE,0,0);
	initStruct(ovr).hEvent = hEvent;
	BOOL res = ConnectNamedPipe(hFile,&ovr);
	if (res == FALSE) {		
		DWORD err = GetLastError();
		CloseHandle(hEvent);
		if (err != ERROR_IO_PENDING) throw FileOpenError(THISLOCATION,err,fileName);
	}
	HANDLE h = CreateFileW(fileName.c_str(),output?GENERIC_READ:GENERIC_WRITE,0,0,OPEN_EXISTING,FILE_FLAG_OVERLAPPED ,0);
	if (h == 0 || h == INVALID_HANDLE_VALUE) {
		CancelIo(hFile);
		CloseHandle(hEvent);
		throw FileOpenError(THISLOCATION,GetLastError(),fileName);
	}
	WaitForSingleObject(hEvent,INFINITE);
	otherEnd = new WindowsPipe(fileName,h,!output);
//	CloseHandle(hEvent);
}

LightSpeed::natural WindowsPipe::postIO( BOOL res, OVERLAPPED &ovr, DWORD rd )
{
	if (res == FALSE) {
		DWORD err = GetLastError();
		if (err == ERROR_IO_PENDING) {
			GetOverlappedResult(hFile,&ovr,&rd,TRUE);
			return rd;
		} else if (err == ERROR_BROKEN_PIPE) {
			return 0;
		} else
			throw FileIOError(THISLOCATION,err,fileName);
	} else {
		return rd;
	}
}

WindowsPipe::~WindowsPipe()
{
	CloseHandle(hEvent);
}

void WindowsPipe::closeOutput()
{
	if (output) {
		CloseHandle(hFile);
		hFile = 0;
	}
}

PInOutStream WindowsFileService::openStdFile( StdFile stdfile )
{
	int id;
	FileOpenMode mode;
	const char *name;
	switch (stdfile) {
		case stdInput : id = STD_INPUT_HANDLE; mode = fileOpenRead; name = "stdin"; break;
		case stdOutput : id = STD_OUTPUT_HANDLE; mode = fileOpenWrite; name = "stdout"; break;
		case stdError : id = STD_ERROR_HANDLE; mode = fileOpenWrite; name = "stderr"; break;
		default: 
			throw InvalidParamException(THISLOCATION,1,"Unknown stdfile type");
	}

	return new WindowsStdConsole(name,id);
}

void WindowsFileService::createPipe( PInputStream &readEnd, POutputStream &writeEnd )
{
	static LONG pipeIdCounter=0;
	TextFormatBuff<wchar_t, StaticAlloc<256> >fnt;
	fnt(L"\\\\.\\pipe\\lightspeed-pipe-%1-%2") << (natural)GetCurrentProcessId() 
		<< (natural)InterlockedIncrement(&pipeIdCounter);
	String name = fnt.write();

	HANDLE p1 = CreateNamedPipeW(name.c_str(),PIPE_ACCESS_OUTBOUND|
		FILE_FLAG_FIRST_PIPE_INSTANCE|FILE_FLAG_OVERLAPPED ,
		PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,
		1,
		4096,4096,0,0);

	if (p1 == 0 || p1 == INVALID_HANDLE_VALUE)
		throw FileOpenError(THISLOCATION,GetLastError(),name);
	WindowsPipe *wrpipe = new WindowsPipe(name,p1,true);
	writeEnd = wrpipe;
	PInOutStream rd;

	wrpipe->createPipeOtherSide(rd);
	readEnd = rd.get();
}

LightSpeed::PInOutStream WindowsFileService::openSeqFile( const void *handle, size_t handleSize, FileOpenMode mode )
{
	if (handleSize != sizeof(HANDLE)) throw InvalidParamException(THISLOCATION,2,"Unknown handle type");
	if (handle == 0) throw InvalidParamException(THISLOCATION,1,"Invalid handle address");
	HANDLE h = *reinterpret_cast<const HANDLE *>(handle);
	if (h == 0 || h == INVALID_HANDLE_VALUE)
		throw InvalidParamException(THISLOCATION,1,"Invalid handle value");

	TextFormatBuff<wchar_t,StaticAlloc<256> >fmt;
	fmt.setBase(16)("handle://%1") << (Bin::natural64)h;
	return openSeqFileByHandle(h, mode, fmt.write());

}

class WindowsFileCommitOnClose: public IFileExtractHandle
	,public IInOutStream,
				public IRndFileHandle {
public:	
	WindowsFileCommitOnClose(PInOutStream hndl, String name)
		:hndl(&hndl->getIfc<WindowsFile>()),name(name),opened(true) {}
	~WindowsFileCommitOnClose() {
		if (!std::uncaught_exception()) {
			closeOutput();
		} else if (opened) {
			try {
				String partName = hndl->getName();
				hndl = nil;
				DeleteFileW(partName.c_str());
			} catch (...) {
				return;
			}
		}
	}
	virtual size_t getHandle(void *buffer, size_t bufferSize) {
		return hndl->getHandle(buffer,bufferSize);
	}
	virtual natural read(void *buffer,  natural size) {
		if (opened) return hndl->read(buffer,size); else return 0;
	}
	virtual natural write(const void *buffer,  natural size) {
		if (opened) return hndl->write(buffer,size); else return 0;
	}
	virtual natural peek(void *buffer, natural size) const {
		if (opened) return hndl->peek(buffer,size); else return 0;
	}

	virtual bool canRead() const {return opened && hndl->canRead();}
	virtual bool canWrite() const {return opened && hndl->canWrite();}
	virtual void flush() {return hndl->flush();}
	virtual natural dataReady() const {return hndl->dataReady();}
	virtual natural read(void *buffer,  natural size, FileOffset offset) const {
		if (opened) return hndl->read(buffer,size,offset); else return 0;
	}
	virtual natural write(const void *buffer,  natural size, FileOffset offset) {
		if (opened) return hndl->write(buffer,size,offset); else return 0;
	}
	virtual void setSize(FileOffset size) {
		return hndl->setSize(size);
	}
	virtual FileOffset size() const {
		return hndl->size();
	}
	virtual void closeOutput() {
		if (opened) {
			String partName = hndl->getName();
			hndl = nil;
			BOOL res = MoveFileExW(partName.c_str(),name.c_str(),MOVEFILE_REPLACE_EXISTING);
			if (res == FALSE) {
				throw FileMsgException(THISLOCATION,GetLastError(),name,"Cannot create/overwrite file");
			}
		}
	}

protected:
	RefCntPtr<WindowsFile> hndl;
	String name;
	bool opened;

};
PInOutStream WindowsFileService::openSeqFile( ConstStrW fname, FileOpenMode mode, OpenFlags::Type flags )
{	
	if (fname.head(7) == ConstStrW(L"http://")) {
		return new WinHttpStream(fname,httpSettings);
	}
	if (fname.head(8) == ConstStrW(L"https://")) {
		return new WinHttpStream(fname,httpsSettings);
	}


	if (flags & OpenFlags::commitOnClose 
				&& mode == fileOpenWrite) {

		String tmpName = fname + ConstStrW(L".$$$");
		PInOutStream hndl = openSeqFile(tmpName,mode,flags & ~OpenFlags::commitOnClose);
		return new WindowsFileCommitOnClose(hndl,fname);
	}

	String name = fname;
	DWORD openMode;
	DWORD openAction = 0;
	DWORD openFlags = 0;
	DWORD shareMode = 0;
	bool app;

	if (flags & OpenFlags::append) {
		if (mode == fileOpenWrite)
			openMode = FILE_APPEND_DATA;
		else
			throw InvalidParamException(THISLOCATION,2,"Cannot use append with fileOpenRead or fileOpenReadWrite");
	} else  switch (mode) {
		case fileOpenRead: openMode = GENERIC_READ;break;
		case fileOpenWrite: openMode = GENERIC_WRITE;break;
		case fileOpenReadWrite: openMode = GENERIC_READ|GENERIC_WRITE;break;
		default: throw InvalidParamException(THISLOCATION,2,"Invalid open mode");
	}


	using namespace OpenFlags;

	if (flags & accessSeq ) openFlags |= FILE_FLAG_SEQUENTIAL_SCAN ;
	else if (flags & accessRnd) openFlags |= FILE_FLAG_RANDOM_ACCESS;
	
	if (flags & writeThrough) openFlags |= FILE_FLAG_WRITE_THROUGH;
	if (flags & temporary) openFlags |= FILE_ATTRIBUTE_TEMPORARY;
	if (flags & deleteOnClose) openFlags |= FILE_FLAG_DELETE_ON_CLOSE;
	if (flags & shareRead) shareMode |= FILE_SHARE_READ;
	if (flags & shareWrite) shareMode |= FILE_SHARE_WRITE;
	if (flags & shareDelete) shareMode |= FILE_SHARE_DELETE;
	if (flags & OpenFlags::createFolder) {
		natural p = name.findLast('\\');
		if (p != naturalNull && !canOpenFile(name.head(p),fileAccessible)) {
			createFolder(name.head(p),true);
		}
	}
	if (flags & commitOnClose) {
		throw UnsupportedFeatureOnClass<WindowsFileService>(
			THISLOCATION,"commitOnClose");
	}
	app = (flags & append) != 0;
	if (flags & create) {		
		if (flags & newFile)
			openAction = CREATE_NEW;
		else if (flags & truncate)
			openAction = CREATE_ALWAYS;
		else
			openAction = OPEN_ALWAYS;
	} else {
		if (flags & truncate)
			openAction = TRUNCATE_EXISTING;
		else
			openAction = OPEN_EXISTING;
	}

	HANDLE h = CreateFileW(name.c_str(),openMode,shareMode,0,openAction,DWORD(flags),0);
	if (h == 0 || h == INVALID_HANDLE_VALUE) {
		DWORD res = GetLastError();
		throw FileOpenError(THISLOCATION,res,name);
	}


	
	return openSeqFileByHandle(h,mode,name);
}

LightSpeed::PRndFileHandle WindowsFileService::openRndFile( const void *handle, size_t handleSize, FileOpenMode mode )
{
	PInOutStream sh = openSeqFile(handle,handleSize,mode)	;
	IRndFileHandle *rf = sh->getIfcPtr<IRndFileHandle>();
	if (rf == 0)
		throw InvalidParamException(THISLOCATION,1,"Cannot open such a handle as random access file");
	return rf;
}

LightSpeed::PRndFileHandle WindowsFileService::openRndFile( ConstStrW ofn, FileOpenMode mode, OpenFlags::Type flags )
{
	PInOutStream sh = openSeqFile(ofn,mode,flags);
	IRndFileHandle *rf = sh->getIfcPtr<IRndFileHandle>();
	if (rf == 0)
		throw InvalidParamException(THISLOCATION,1,"File cannot be opened in random access");
	return rf;
}

LightSpeed::PInOutStream WindowsFileService::openSeqFileByHandle( HANDLE h, FileOpenMode mode, String name )
{
	DWORD type = GetFileType(h);


	switch (type) {
	case FILE_TYPE_UNKNOWN:
		throw InvalidParamException(THISLOCATION,1,"Unknown handle value");
	case FILE_TYPE_DISK:
		switch (mode) {
			case fileOpenRead: return new WindowsFile(name,h,false,true);
			case fileOpenWrite: return new WindowsFile(name,h,true,false);
			case fileOpenReadWrite: return new WindowsFile(name,h,true,true);
			default: break;
		}
		break;
	case FILE_TYPE_PIPE:
		switch (mode) {
			case fileOpenRead: return new WindowsPipe(name,h,false);
			case fileOpenWrite: return new WindowsPipe(name,h,true);
			default: break;
		}
		break;

	case FILE_TYPE_CHAR:
		switch (mode) {
			case fileOpenRead:return new WindowsConsole(name,h,false);
			case fileOpenWrite:return new WindowsConsole(name,h,true);
			default: break;
		}
		break;
	default: break;
	}

	throw InvalidParamException(THISLOCATION,1,"Cannot open handle with such a mode");
}

bool WindowsFileService::canOpenFile( ConstStrW name, FileOpenMode mode ) const
{
	if (!checkZeroAtString(name)) return canOpenFile(String(name),mode);

	int testFlag;
	switch (mode) {
		case fileAccessible: testFlag = 00;break;
		case fileOpenRead: testFlag = 04;break;
		case fileOpenWrite: testFlag = 02;break;
		case fileOpenReadWrite: testFlag = 06;break;
		case fileExecutable: testFlag = 02;
			if (name.length() < 4) return false;
			ConstStrW ext = name.tail(3);
			StrCmpCI<wchar_t> cmp;
			if (cmp(ext,L"exe") && cmp(ext,L"com") 
					&& cmp(ext,L"bat") && cmp(ext,L"cmd")) return false;
	}

	errno_t err = _waccess_s(name.data(),testFlag);
	return err == 0;
}

LightSpeed::PFolderIterator WindowsFileService::openFolder( ConstStrW pathname ) 
{
	return new WindowsDirectoryIterator(pathname,*this,false);
}

void WindowsFileService::createFolder( ConstStrW folderName, bool parents )
{
	if (!checkZeroAtString(folderName)) return createFolder(String(folderName),parents);
	if (parents) {

		natural k = folderName.findLast('\\');
		if (k == naturalNull || (k == 2 && folderName[k-1] == ':' )) {
			createFolder(folderName,false);
		} else {
			String path = folderName.head(k);
			ConstStrW name = folderName.offset(k+1);
			if (!canOpenFile(path,fileAccessible)) try {
				createFolder(path,true);
			} catch (const ErrNoException &e) {
				throw FolderCreationException(THISLOCATION,e.getErrNo(),folderName) << e;
			} catch (const Exception &e) {
				throw FolderCreationException(THISLOCATION,0,folderName) << e;
			}
			createFolder(folderName,false);
		}
	} else {
		BOOL res = CreateDirectoryW(folderName.data(),0);
		if (res == FALSE) throw FolderCreationException(THISLOCATION,GetLastError(),folderName);
	}
}

void WindowsFileService::removeFolder( ConstStrW folderName, bool recursive )
{
	if (!checkZeroAtString(folderName)) return removeFolder(String(folderName),recursive);

	//try to erase folder directly = because it may be junction point and it cannot be traversed recursive

	BOOL res = RemoveDirectoryW(folderName.data());
	if (res == FALSE) {
		DWORD err = GetLastError();
		if (err == ERROR_DIR_NOT_EMPTY && recursive) {
			//perform recursive removing
			WIN32_FIND_DATAW fndinfo;
			String wild=folderName+ConstStrW(L"\\*.*");
			HANDLE h = FindFirstFileW(wild.data(),&fndinfo);
			if (h != INVALID_HANDLE_VALUE) {
				do {
					ConstStrW name(fndinfo.cFileName);
					if (name != ConstStrW(L".") && name != ConstStrW(L"..")) {
						String fullName = folderName + ConstStrW(L"\\") + ConstStrW(fndinfo.cFileName);
						if (fndinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
							try {
								removeFolder(fullName,true);
							} catch (const Exception &e) {
								FindClose(h);
								throw FolderNotEmptyException(THISLOCATION,0,folderName) << e;
							}
						} else {
							res = DeleteFileW(fullName.c_str());
							if (res == FALSE) {
								err = GetLastError();
								FindClose(h);
								throw FileDeletionException(THISLOCATION,err,fullName);
							}
						}
					}
				} while (FindNextFileW(h,&fndinfo));
				FindClose(h);
			}
			removeFolder(folderName,false);
		} else if (err == ERROR_DIR_NOT_EMPTY) {
			throw FolderNotEmptyException(THISLOCATION,err,folderName);
		} else {
			throw FileDeletionException(THISLOCATION,err,folderName);
		}
	}
}

void WindowsFileService::copyDirectories( ConstStrW from, ConstStrW to ) 
{
	try {
		PFolderIterator iter = openFolder(from);
		if (!canOpenFile(to,fileAccessible)) createFolder(to,true);
		while (iter->getNext()) {
			String target = to + ConstStrW('\\') + iter->entryName;
			String source = iter->getFullPath();
			if (iter->type == IFolderIterator::directory) {
				copyDirectories(source,target);
			} else if (iter->type == IFolderIterator::file) {
				copy(source,target,true);
			}
		}
	} catch (Exception &e) {
		e.appendReason(ErrorMessageException(THISLOCATION,
			String("Copying directory '") + from + String("' to '") + to + String("'")));
		throw;
	}
}

void WindowsFileService::copy( ConstStrW from, ConstStrW to, bool overwrite )
{
	if (!checkZeroAtString(from)) return copy(String(from),to,overwrite);
	if (!checkZeroAtString(to)) return copy(from,String(to),overwrite);
	WIN32_FIND_DATAW data;
	HANDLE h = FindFirstFileW(from.data(),&data);
	if (h != NULL && h != INVALID_HANDLE_VALUE) {
		FindClose(h);
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (overwrite && canOpenFile(to,fileAccessible)) 
				FileIOError(THISLOCATION,ERROR_ALREADY_EXISTS,to);
			copyDirectories(from,to);
			return;
		}
	}
	BOOL res = CopyFileW(from.data(),to.data(),overwrite?FALSE:TRUE);
	if (res == FALSE)
		throw FileIOError(THISLOCATION,GetLastError(),to);
}

void WindowsFileService::move( ConstStrW from, ConstStrW to, bool overwrite )
{
	if (!checkZeroAtString(from)) return move(String(from),to,overwrite);
	if (!checkZeroAtString(to)) return move(from,String(to),overwrite);
	BOOL res = MoveFileExW(from.data(),to.data(),overwrite?MOVEFILE_REPLACE_EXISTING:0);
	if (res == FALSE) {
		DWORD err = GetLastError();
		if (err == ERROR_NOT_SAME_DEVICE) {
			copy(from,to,overwrite);
			remove(from);
		} else {
			throw FileIOError(THISLOCATION,GetLastError(),to);
		}
	}

}

void WindowsFileService::link( ConstStrW linkName, ConstStrW target, bool overwrite )
{
	if (!checkZeroAtString(linkName)) return move(String(linkName),target,overwrite);
	if (!checkZeroAtString(target)) return move(linkName,String(target),overwrite);
	BOOL res = MoveFileExW(target.data(),linkName.data(),(overwrite?MOVEFILE_REPLACE_EXISTING:0)|MOVEFILE_CREATE_HARDLINK);
	if (res == FALSE)
		throw FileIOError(THISLOCATION,GetLastError(),target);

}

void WindowsFileService::remove( ConstStrW what )
{
	if (!checkZeroAtString(what)) return remove(String(what));
	BOOL res = DeleteFileW(what.data());
	if (res == FALSE)
		throw FileDeletionException(THISLOCATION,GetLastError(),what);

}

LightSpeed::PMappedFile WindowsFileService::mapFile( ConstStrW filename, FileOpenMode mode )
{
	return new WindowsMappedFile(filename,mode);
}

LightSpeed::String WindowsFileService::findFile( ConstStrW filename, ConstStrW pathList, natural position /*= 1*/ )
{
	return filename;
}

LightSpeed::String WindowsFileService::findExec( ConstStrW execName )
{
	return execName;
}

LightSpeed::PTemporaryFile WindowsFileService::createTempFile(ConstStrW prefix, bool rndfile)
{
	if (!checkZeroAtString(prefix)) return createTempFile(String(prefix));
	wchar_t tempPath[MAX_PATH*2];
	wchar_t tempFile[MAX_PATH*2];
	GetTempPathW(DWORD(countof(tempPath)),tempPath);
	if (rndfile) {
		GetTempFileNameW(tempPath,prefix.data(),0,tempFile);
		String filename(tempFile);
		PInOutStream hnd = openSeqFile(filename,fileOpenWrite,
			OpenFlags::truncate|OpenFlags::temporary|OpenFlags::shareRead|OpenFlags::shareWrite|OpenFlags::shareDelete);
		return new TemporaryFile(*this,hnd,filename);
	} else {
		TextFormatBuff<wchar_t> fmt;
		fmt("%1\\%2") << tempPath << prefix;
		String filename(fmt.write());
		try {
			PInOutStream hnd = openSeqFile(filename,fileOpenWrite,
				OpenFlags::create|OpenFlags::temporary|OpenFlags::shareRead|OpenFlags::shareWrite);
			return new TemporaryFile(*this,hnd,filename);
		} catch (FileOpenError &e) {
			if (e.getErrNo() == ERROR_FILE_EXISTS) 
				return new TemporaryFile(*this,nil,filename);
			else 
				throw;
		}
	}
}

LightSpeed::PFolderIterator WindowsFileService::getFileInfo( ConstStrW pathname ) 
{
	PFolderIterator iter = new WindowsDirectoryIterator(pathname,*this,true);
	if (!iter->getNext())
		throw FileOpenError(THISLOCATION,2,pathname);
	return iter;
}
LightSpeed::natural WindowsConsole::read( void *buffer, natural size )
{
	if (peekBuffSz > 0) {
		natural rd = std::min(size,peekBuffSz);
		memcpy(buffer,peekBuff,rd);
		peekBuffSz-=rd;
		if (peekBuffSz>0) memcpy(peekBuff,peekBuff+rd,peekBuffSz);			
		return rd;
	} else {
		DWORD rd;
		BOOL res = ReadFile(hFile,buffer,limitSize(size),&rd,0);
		if (res == FALSE) {
			throw FileIOError(THISLOCATION,GetLastError(),fileName);
		}
		return rd;
	}
}

LightSpeed::natural WindowsConsole::write( const void *buffer, natural size )
{
	DWORD rd;
	BOOL res = WriteFile(hFile,buffer,limitSize(size),&rd,0);
	if (res == FALSE) {
		throw FileIOError(THISLOCATION,GetLastError(),fileName);
	}
	return rd;
}

LightSpeed::natural WindowsConsole::peek( void *buffer, natural size ) const
{
	if (size > sizeof(peekBuff)) size = sizeof(peekBuff);
	if (size > peekBuffSz) {
		natural remain = size - peekBuffSz;
		DWORD rd;
		BOOL res = ReadFile(hFile,peekBuff+peekBuffSz,DWORD(remain),&rd,0);
		if (res == FALSE) {
			throw FileIOError(THISLOCATION,GetLastError(),fileName);
		}
		peekBuffSz+=rd;
	}
	memcpy(buffer,peekBuff,size);
	return size;

}

bool WindowsConsole::canRead() const
{
	return !output && hFile != 0;
}

bool WindowsConsole::canWrite() const
{
	return output && hFile != 0;
}

void WindowsConsole::flush()
{
	//nothing
}

natural WindowsConsole::dataReady() const
{
	return (WaitForSingleObject(hFile, 0) == WAIT_OBJECT_0) ? 1 : 0;
}


WindowsConsole::WindowsConsole( String name, HANDLE hConsole, bool output )
:WindowsFileCommon(name,hConsole),output(output),peekBuffSz(0)
{

}

void WindowsConsole::closeOutput()
{
	if (output) CloseHandle(hFile);
}

static WindowsFileService winfilesvc;
static IFileIOServices *curServices = &winfilesvc;

IFileIOServices &IFileIOServices::getIOServices() {
	return *curServices;
}
void IFileIOServices::setIOServices(IFileIOServices *newServices) {
	if (newServices == 0) curServices = &winfilesvc;
	else curServices = newServices;
}


WindowsDirectoryIterator::WindowsDirectoryIterator( ConstStrW path, IFileIOServices &svc, bool thisFile )
:hFind(0),path(path),thisFile(thisFile),eof(false),svc(svc)
{
	source = this->path;
}

WindowsDirectoryIterator::~WindowsDirectoryIterator()
{
	rewind();
}

bool WindowsDirectoryIterator::getNext()
{
	if (eof) return false;

	if (hFind == 0) {
		String mask;
		natural ppos;
		{
			LPWSTR name;
			DWORD l = GetFullPathNameW(path.cStr(), 0, 0, 0);
			StringW expath;
			wchar_t *buff = expath.createBuffer(l);
			GetFullPathNameW(path.cStr(), l, buff, &name);
			path = expath;
			ppos = name - buff - 1;
		}
		if (thisFile) {
			mask = path;
			path = path.head(ppos);
			
		} else {
			mask = path + ConstStrW(L"\\*");
		}
		hFind = FindFirstFileW(mask.c_str(),&fileInfo.fndData);
		if (hFind == 0 || hFind == INVALID_HANDLE_VALUE) {eof = true; return false; }
		updateFindData();
		return true;
	} else {
		BOOL res = FindNextFileW(hFind,&fileInfo.fndData);
		if (res == FALSE) {
			eof = true;
			FindClose(hFind);
			hFind = 0;
			return false;
		}
		updateFindData();
		return true;
	}
}

void WindowsDirectoryIterator::updateFindData()
{
	source = path;
	entryName = fileInfo.fndData.cFileName;
	link = false;
	hidden = (fileInfo.fndData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
	extra = 0;
	
	if (fileInfo.fndData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		if (entryName == ConstStrW(L"..") || entryName == ConstStrW(L"."))
			type = dots;
		else
			type = directory;
	} else {
		type = file;
	}

}

LightSpeed::lnatural WindowsDirectoryIterator::getSize() const
{
	return ((lnatural) fileInfo.fndData.nFileSizeHigh << 32) 
		+ fileInfo.fndData.nFileSizeLow;
}

const WindowsDirectoryIterator::IFileInformation & WindowsDirectoryIterator::getInfo() const
{
	return fileInfo;
}

TimeStamp WindowsDirectoryIterator::getModifiedTime() const
{
	return TimeStamp::fromWindows(fileInfo.fndData.ftLastAccessTime.dwLowDateTime,
				fileInfo.fndData.ftLastAccessTime.dwHighDateTime);
}

IFileIOServices::FileOpenMode WindowsDirectoryIterator::getAllowedOpenMode() const
{
	String full = getFullPath();
	if (svc.canOpenFile(full,IFileIOServices::fileOpenReadWrite)) return IFileIOServices::fileOpenReadWrite;
	if (svc.canOpenFile(full,IFileIOServices::fileOpenRead)) return IFileIOServices::fileOpenRead;
	if (svc.canOpenFile(full,IFileIOServices::fileOpenWrite)) return IFileIOServices::fileOpenWrite;
	return IFileIOServices::fileAccessible;
}

LightSpeed::PFolderIterator WindowsDirectoryIterator::openFolder() const
{
	return new WindowsDirectoryIterator(String(getFullPath()),svc,false);

}

LightSpeed::String WindowsDirectoryIterator::getFullPath() const
{
	return path+ConstStrW(L"\\")+entryName;
}

void WindowsDirectoryIterator::rewind()
{
	if (hFind) FindClose(hFind);
	hFind = 0;
}

LightSpeed::PInOutStream WindowsDirectoryIterator::openSeqFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags)
{
	return svc.openSeqFile(getFullPath(),mode,flags);
}

LightSpeed::PRndFileHandle WindowsDirectoryIterator::openRndFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags)
{
	return svc.openRndFile(getFullPath(),mode,flags);
}

void WindowsDirectoryIterator::remove(bool recursive /*= false*/)
{
	if (type == directory) svc.removeFolder(getFullPath(),recursive);
	else svc.remove(getFullPath());
}

LightSpeed::PMappedFile WindowsDirectoryIterator::mapFile(IFileIOServices::FileOpenMode mode)
{
	return svc.mapFile(getFullPath(),mode);
}

void WindowsDirectoryIterator::copy(ConstStrW to, bool overwrite)
{
	svc.copy(getFullPath(),to,overwrite);
}

void WindowsDirectoryIterator::move(ConstStrW to, bool overwrite)
{
	svc.move(getFullPath(),to,overwrite);
}

WindowsMappedFile::WindowsMappedFile( ConstStrW filename, IFileIOServices::FileOpenMode mode )
{
	bool write = mode == IFileIOServices::fileOpenReadWrite 
		|| mode == IFileIOServices::fileOpenWrite;
	String fname = filename;
	HANDLE h = CreateFileW(fname.c_str(), GENERIC_READ|(write?GENERIC_WRITE:0),
		FILE_SHARE_DELETE|FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	if (h == 0 || h == INVALID_HANDLE_VALUE)
		throw FileOpenError(THISLOCATION,GetLastError(),fname);

	hFile = h;
	hMapping = CreateFileMappingW(hFile,0,write?PAGE_READWRITE:PAGE_READONLY,0,0,0);
	if (hMapping == 0) {
		DWORD err = GetLastError();
		CloseHandle(hFile);
		throw FileMsgException(THISLOCATION,err,fname,L"Mapping failed");		
	}
}

WindowsMappedFile::WindowsMappedFile( HANDLE hMapping, HANDLE hFile )
	:hMapping(hMapping),hFile(hFile)
{

}
DWORD WindowsMappedFile::allocGrain = 0;

WindowsMappedFile::MappedRegion WindowsMappedFile::map( 
	IRndFileHandle::FileOffset offset, natural size, 
	IFileIOServices::FileOpenMode mode, bool copyOnWrite )
{
	DWORD access = getMapAccess(mode, copyOnWrite);
	
	if (allocGrain == 0) {
		SYSTEM_INFO sinfo;
		GetSystemInfo(&sinfo);
		allocGrain = sinfo.dwAllocationGranularity;
	}
	DWORD allocOffset = offset % allocGrain;
	IRndFileHandle::FileOffset fileOffset = offset - allocOffset;
	const LARGE_INTEGER &lofs = reinterpret_cast<const LARGE_INTEGER &>(fileOffset);
	DWORD mapSize = DWORD(size + allocOffset);

	//TODO: solve problem with offset in mapping
	LPVOID res = MapViewOfFile(hMapping,access,lofs.HighPart,lofs.LowPart,mapSize);
	if (res == 0) {
		throw ErrNoException(THISLOCATION,GetLastError());
	}
	return MappedRegion(this,reinterpret_cast<byte *>(res) - allocOffset,size,offset);
}

WindowsMappedFile::MappedRegion WindowsMappedFile::map( IFileIOServices::FileOpenMode mode, bool copyOnWrite )
{
	DWORD access = getMapAccess(mode, copyOnWrite);

	LPVOID res = MapViewOfFile(hMapping,access,0,0,0);
	if (res == 0) {
		throw ErrNoException(THISLOCATION,GetLastError());
	}
	MEMORY_BASIC_INFORMATION info;
	initStruct(info);
	VirtualQuery(res,&info,sizeof(info));

	return MappedRegion(this,res ,info.RegionSize,0);

}
void WindowsMappedFile::unmap( MappedRegion &reg )
{
	DWORD allocOffset = (DWORD)reg.offset;
	if (allocOffset) allocOffset %= allocGrain;
	LPVOID mdaddr = reinterpret_cast<byte *>(reg.address) - allocOffset;
	UnmapViewOfFile(mdaddr);
}

void WindowsMappedFile::sync( MappedRegion &reg )
{
	DWORD allocOffset = reg.offset % allocGrain;
	LPVOID mdaddr = reinterpret_cast<byte *>(reg.address) - allocOffset;
	FlushViewOfFile(mdaddr,reg.size+ allocOffset);
}

void WindowsMappedFile::lock( MappedRegion &reg )
{
	DWORD allocOffset = reg.offset % allocGrain;
	LPVOID mdaddr = reinterpret_cast<byte *>(reg.address) - allocOffset;
	VirtualLock(mdaddr,reg.size + allocOffset);
}

void WindowsMappedFile::unlock( MappedRegion &reg )
{
	DWORD allocOffset = reg.offset % allocGrain;
	LPVOID mdaddr = reinterpret_cast<byte *>(reg.address) - allocOffset;
	VirtualUnlock(mdaddr,reg.size + allocOffset);
}

WindowsMappedFile::~WindowsMappedFile()
{
	CloseHandle(hFile);
	CloseHandle(hMapping);
}

DWORD WindowsMappedFile::getMapAccess( IFileIOServices::FileOpenMode mode, bool copyOnWrite )
{
	DWORD access = 0;
	switch (mode) {
	case IFileIOServices::fileOpenRead: access|=FILE_MAP_READ;break;
	case IFileIOServices::fileOpenWrite: access|=FILE_MAP_WRITE;break;
	case IFileIOServices::fileOpenReadWrite: access|=FILE_MAP_ALL_ACCESS;break;
	default: throw InvalidParamException(THISLOCATION,3,"");
	}
	if (copyOnWrite) access|=FILE_MAP_COPY;
	return access;
}

WindowsStdConsole::WindowsStdConsole( String name, DWORD type ):
	WindowsConsole(name,INVALID_HANDLE_VALUE,type == STD_INPUT_HANDLE?false:true)
		,type(type)
{

}

LightSpeed::natural WindowsStdConsole::read( void *buffer, natural size )
{
	if (hFile == INVALID_HANDLE_VALUE) {
		updateHandle();
	}
	return WindowsConsole::read(buffer,size);
}

LightSpeed::natural WindowsStdConsole::write( const void *buffer, natural size )
{
	if (hFile == INVALID_HANDLE_VALUE) {
		updateHandle();
	}
	return WindowsConsole::write(buffer,size);

}

LightSpeed::natural WindowsStdConsole::peek( void *buffer, natural size ) const
{
	if (hFile == INVALID_HANDLE_VALUE) {
		const_cast<WindowsStdConsole *>(this)->updateHandle();
	}
	return WindowsConsole::peek(buffer,size);

}

void WindowsStdConsole::updateHandle()
{
	HANDLE h = GetStdHandle(type);
	if (h == 0 || h == INVALID_HANDLE_VALUE) {
		AllocConsole();
		h = GetStdHandle(type);
		if (h == 0) h = INVALID_HANDLE_VALUE;
	}
	if (h == INVALID_HANDLE_VALUE)
		throw FileOpenError(THISLOCATION,GetLastError(),getName());

	HANDLE hDup;
	if (DuplicateHandle(GetCurrentProcess(),h,GetCurrentProcess(),&hDup,0,FALSE,
		DUPLICATE_SAME_ACCESS) == FALSE) 
		throw FileOpenError(THISLOCATION,GetLastError(),getName());
	hFile = hDup;
}

size_t WindowsStdConsole::getHandle( void *buffer, size_t bufferSize )
{
	updateHandle();
	return WindowsConsole::getHandle(buffer,bufferSize);
}

}
