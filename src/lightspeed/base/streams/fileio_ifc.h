#ifndef LIGHTSPEED_STREAMS_FILEIO_IFC_H_
#define LIGHTSPEED_STREAMS_FILEIO_IFC_H_

#include "../interface.h"
#include "openFlags.h"
#include "../timestamp.h"
#include "../memory/refcntifc.h"

namespace LightSpeed {        
    



	///The simplest and very basic and the low-level input stream
	/** This is replacement of old ISeqFileHandle. Interface defines functions
	 * that should implement every input stream, which is able to read bytes a from device.
	 */
	class IInputStream: virtual public IRefCntInterface {
	public:
        ///Reads bytes into buffer
        /** @param buffer pointer to buffer that will receive binary data
         *  @param size size of buffer in bytes. Don't set to zero, this state is not defined by interface
         *  @retval 1..size count of bytes read. In case of blocking I/O
         *  function must be able to read atleast one byte. Otherwise, it
         *  can block operation. If blocking operation si not available, function must
         *  throw appropriate exception.
         *  @retval 0 Found end of stream. Implementation have to return 0 for every following
         *     calls, until state of stream is not changed. Caller must handle this return
         *     value to prevent infinite loop of reading
         *
         *
         *  @exception IOException thrown on any I/O error
         */
        virtual natural read(void *buffer,  natural size) = 0;
		///Reads data from the stream, but did not remove them
		/**
		 * @param buffer pointer to buffer, which receives data
		 * @param size size of buffer
		 * @return count of bytes written to the buffer.
		 *
		 * @note function should store at least one byte. Stream don't need to
		 * support function for more than one byte. If there is no bytes
		 * in internal buffer, function starts waiting to receive at least
		 * one byte.
		 *
		 * @note Return value is similar to read() including how EOF is handled
		 *
		  */
		virtual natural peek(void *buffer, natural size) const = 0;
		///Returns true, when stream can be read
		/**
		 * @retval true can read the stream
		 * @retval false cannot read stream. stream is not opened for reading, or EOF reached
		 */

		virtual bool canRead() const = 0;

		///Check, if there are data read to read immediatelly without blocking
		/**
		 * @return 0 no data are ready, reading causes I/O operation with external device (disk, network, pipe)
		 * @retval 1 at least one byte is ready, can be more, because no all implementations can determine how many bytes
		 *              can be read immediatelly
		 * @retval >1 at least returned bytes are available for immediate reading.
		 *
		 * @note recomendation: disk file should always return 0 unless it is internally buffered.
		 * Function is designed for software buffers.
		 *
		 * @note If stream is in eof state (no more data to read), function SHOULD return 1 (because EOF is readable state).
		 * This will enforce caller to check stream state (and receive end of stream state)
		 *
		 *
		 */
		virtual natural dataReady() const = 0;
		///Reads all bytes
		/**
		 *  Ordinary read() can read up to requested bytes. This means,
		 *  they can read less than requested bytes, but at least one byte will
		 *  be always read.
		 *
		 *  This function ensures, that all bytes will be read
		 *
		 *  @param buffer pointer to buffer
		 *  @param size count of bytes
		 *  @return count of bytes has been read. Remaining bytes cannot
		 *    be read, because function read() returned zero.
		 */
		natural readAll(void *buffer, natural size) {
			byte *p = reinterpret_cast<byte *>(buffer);
			natural r = size;
			while (r) {
				natural x =read(p,r);
				if (x == 0) break;
				r-=x;p+=x;
			}
			return size - r;
		}
	};


	///The simplest and very basic and the low-level output stream
	/** This is replacement of old ISeqFileHandle. Interface defines functions
	 * that should implement every output stream, which is able to send bytes to a device.
	 */
	class IOutputStream: virtual public IRefCntInterface {
	public:
        /// Writes bytes from buffer to file
        /** @param buffer pointer to buffer that contains data to write
         *  @param size size of buffer in bytes
         *  @return count of bytes written. In case of blocking I/O
         *  function must be able to write atleast one byte. Otherwise, it
         *  can block operation. In case of non-blocking I/O, function
         *  can return zero to report, that operation would block
         *  @exception IOException thrown on any I/O error
         */
        virtual natural write(const void *buffer,  natural size) = 0;

        ///Returns true, when stream can be written
		/**
		 * @retval true you can write
		 * @retval false you cannot write
		 */
		 
		virtual bool canWrite() const = 0;
		///Flushes any internal buffers
		/** Function should flush any buffer on the way between caller
		 * and kernel. If stream is chained, function should first flush
		 * buffers on current instance and then call flush() on
		 * instance next in the chain
		 */
		virtual void flush() = 0;
		///Closes output of this stream
		/** Function closes output (available only when output is opened).
		 * It causes, that future writes will be unavailble. It also
		 * closes any associated resource with this stream opened
		 * in output mode. For example, when stream is file opened for
		 * write, function closes it for writing. Function don't need
		 * to release resource complete (so for example locks can be
		 * still held). If stream is pipe or connection, function closes
		 * write end of the connection.
		 *
		 * After output is closed, reading is still available, but may
		 * be closed soon by the other side once the closing event is
		 * recognized by the other side.
		 *
		 * Function flushes all output buffer before output is closed, so you
		 * don't need to flush() it explicitly. Closing output on already
		 * closed connection causes exception. Error during flush can also
		 * cause exception.
		 *
		 * Closed output should be indicated in function canWrite() which should
		 * return false;
		 */
		virtual void closeOutput() = 0;

		///Writes all bytes
		/**
		 *  Ordinary write can write up to requested bytes. This means,
		 *  they can write less than requested bytes, but at least one byte will
		 *  be always written.
		 *
		 *  This function ensures, that all bytes will be written
		 *
		 *  @param buffer pointer to buffer
		 *  @param size count of bytes
		 *  @return count of bytes has been written. Remaining bytes cannot
		 *    be written, because function write() returned zero.
		 */
		 
		natural writeAll(const void *buffer, natural size) {
			const byte *p = reinterpret_cast<const byte *>(buffer);
			natural r = size;
			while (r) {
				natural x =write(p,r);
				if (x == 0) break;
				r-=x;p+=x;				
			}
			return size - r;
		}

	};


	///Defines interface for full-duplex streams
	class IInOutStream: public IInputStream, public IOutputStream {

	};


    ///Low-level object to manipulate with the sequential file
    /** This interface is actually deprecated, but it is still in use in many classes. Interface
     * has been replaced by IInOutStream
     */
    class ISeqFileHandle: public IInOutStream {
    public:
	};

    class IInputBlockStream: public IInputStream {
    public:

    	///closes current block and opens new one
    	/**
    	 *
    	 * @retval true new block opened
    	 * @retval false no more blocks available
    	 */
    	virtual bool nextBlock() = 0;
    };


    ///extends IInputStream or IOutputStream with position control
    /** To retrieve this interface, use getIfc<IStreamPosition>
     *
     * Not all streams supports position. Buffered streams can
     * support moving about limited distance
     */
    class IStreamPosition {
    public:

    	///Marks current position in the file
    	/**
    	 * @return function returns "mark index". It don't need to be exact position,
    	 * so don't try to interpret it as position. It can be intex into
    	 * some table of positions.
    	 *
    	 * Don't forget to unmark position, if you don't needed it more.
    	 */
    	virtual natural mark() = 0;

    	///Removes mark from the stream releasing all resources associated with it
    	virtual void unmark(natural markId) = 0;

    	///Seeks in the stream to marked position and specified offset
    	virtual bool seek(natural markId, integer offset = 0) = 0;

    	///returns distance of two marks in bytes
    	/**
    	 * @param mark1 first mark
    	 * @param mark2 second mark
    	 * @return positive number if mark2 > mark1, negative number if mark1 > mark2. Absolute value
    	 *  contains count of bytes between marks. (it says, how many bytes you should process to
    	 *  move pointer from mark1 to mark2.
    	 */
    	virtual integer distance(natural mark1, natural mark2) const = 0;

    	///Removes all marks active at current file
    	virtual void unmarkAll() = 0;
    };


    class IOutputBlockStream: public IOutputStream {
    public:

    	///Closes current block and creates new onetor
    	/**
    	 * Function closes current block and allows to open new block with first write on it.
    	 * Multiple call of next() without writting to the block doesn't cause to create empty
    	 * blocks. You have to write at least one byte to new block.
    	 *
    	 * If you really need to create empty block, use closeOutput() after next(). Function closeOutput()
    	 * should always create new block and then immediatelly closes it.
    	 *
    	 *
    	 * @note that closeOutput() should not commit the block. It should only block additional writes.
    	 */
    	virtual void closeBlock() = 0;

    	///Closes current block and creates new one
    	/**
    	 * Function closes current block and allows to open new block with first write on it.
    	 * Multiple call of next() without writting to the block doesn't cause to create empty
    	 * blocks. You have to write at least one byte to new block.
    	 *
    	 * If you really need to create empty block, use closeOutput() after next(). Function closeOutput()
    	 * should always create new block and then immediatelly closes it.
    	 *
    	 * @param sizeOfNextBlock specifies size of new block. Object should not allow to write beiond this
    	 * size. If currently closing block is smaller then specified size, object can pad this block
    	 * with zeroes.
    	 *
    	 * @note that closeOutput() should not commit the block. It should only block additional writes.
    	 */
    	virtual void closeBlock(natural sizeOfNextBlock) = 0;

    };


    ///Low-level object to manipulate with the random access file
    class IRndFileHandle: virtual public IRefCntInterface {
    public:
        
        typedef uint64_t FileOffset;
        
        ///Reads bytes into buffer
        /** @param buffer pointer to buffer that will receive binary data
         *  @param size size of buffer in bytes
         *  @param offset offset in the file to read. If offset is equal or 
         *   above to file length, function can return 0 as count of bytes read.
         *  @return count of bytes read
         *  @exception IOException thrown on any I/O error
         * 
         */
        virtual natural read(void *buffer,  natural size, FileOffset offset) const = 0;
        /// Writes bytes from buffer to file
        /** @param buffer pointer to buffer that contains data to write
         *  @param size size of buffer in bytes
         *  @param offset offset in the file to write. If offset is equal or 
         *   above to file length, function extends file to reach required size
         *  @return count of bytes written
         *  @exception IOException thrown on any I/O error
         */
        virtual natural write(const void *buffer,  natural size, FileOffset offset) = 0;
        
        ///Sets new size of file in the bytes.
        /**@param size required size in the bytes. If requested size
         * if above the curent size, file is expanded. New areas are filled
         * by zeroes. 
         * 
         * @exception IOException thown on any I/O error including while
         *  file cannot be expanded to full size. Operation is always blocking.
         *
         */
        virtual void setSize(FileOffset size) = 0;
        
        virtual FileOffset size() const = 0;
        
        virtual void flush() = 0;

        virtual ~IRndFileHandle() {}
    };

    ///Handles locking files on the filesystem
    class ILockFileInterface {
    public:
    	///Acquires shared lock for the file
    	/**
    	 * Shared lock can be acquired only when there is no exclusive lock kept.
    	 * When shared lock is kept, other processes are able to acquire shared lock,
    	 * but none of them will able to acquire exclusive lock
    	 * @param timeout specifies timeout, default value is "infinite"
    	 * @retval true success
    	 * @retval false failed - timeout
    	 * @exception SystemException may be thrown on error
    	 */
    	virtual bool lockShared(natural timeout = naturalNull) = 0;
    	///Acquires exclusive lock for the file
    	/**
    	 * Exclusive lock can be acquired only when there is no other exclusive lock kept and no
    	 * other shared lock kept.
    	 * When exclusive lock is kept, none of other processes are able to request any type of lock
    	 * @param timeout specifies timeout, default value is "infinite"
    	 * @retval true success
    	 * @retval false failed - timeout
    	 * @exception SystemException may be thrown on error
    	 */
    	virtual bool lockExclusive(natural timeout = naturalNull) = 0;
    	///Acquires the lock
    	/**
    	 *
    	 * @param exclusive specifies type of lock. When true, exclusive lock is acquired (
    	 * 		see lockExclusive() ), when false, shared lock is acquired (see lockShared() )
    	 * @param timeout specifies timeout, default value is "infinite"
    	 * @retval true success
    	 * @retval false failed - timeout
    	 * @exception SystemException may be thrown on error
    	 */
    	virtual bool lock(bool exclusive, natural timeout = naturalNull) = 0;
    	///Unlocks locked filed
    	/**
    	 * @exception SystemException may be thrown on error
    	 */
    	virtual void unlock() = 0;

    	virtual ~ILockFileInterface() {}
    };

    ///Use this interface to retrieve underlying handle for this stream
    /**
     * This is useful to cooperate directly with platform or 3rd part library.
     * Using underlying handles makes portability harder, but can solve
     * many situations, where underlying handle must be accessible.
     *
     * To access this interface, use dynamic_cast on object implementing
     * ISeqFileHandle or IRndFileHandle. If object cannot be casted to this
     * interface, it is not probably implementing handles at all.
     *
     */
    class IFileExtractHandle {
    public:

    	///Retrieves handle of current stream
    	/**
    	 * @param buffer pointer to buffer, which receives the handle value
    	 * @param bufferSize size of the buffer. It used to check, whether there
    	 * is enough space to store handle value
		 * @return count of bytes written. If buffer is NULL, returns count of bytes required
    	 *
    	 */
    	virtual size_t getHandle(void *buffer, size_t bufferSize) = 0;

    	virtual ~IFileExtractHandle() {}

    };


    class IDirectoryIterator;

    class IMappedFile;



    ///Shared pointer. You can share the object. 
    typedef RefCntPtr<ISeqFileHandle> PSeqFileHandle;
    ///Shared pointer. You can share the object. 
    typedef RefCntPtr<IRndFileHandle> PRndFileHandle;
    ///Shared pointer. You can share the object.
    typedef RefCntPtr<IDirectoryIterator> PDirectoryIterator;
    ///Shared pointer. You can share the object.
    typedef RefCntPtr<IMappedFile> PMappedFile;

    typedef RefCntPtr<IInputStream> PInputStream;
    typedef RefCntPtr<IOutputStream> POutputStream;
    typedef RefCntPtr<IInOutStream> PInOutStream;

	///Controls temporary file and automatically removes file when it no longer needed
	/**
	  * To create instance of this class, call IFileIOServices::createTempFile().
	  * Temporary file is deleted in the destructor of this class
	  */
	 
	 
	class IFileIOServices;

	class TemporaryFile: public RefCntObj {
	public:

		///Constructs temporary file
		/**
		 * @param svc reference to file services responsible on this file
		 * @param handle file handle opened for writing - you can use it to write data on it
		 * @param filename name of the file
		 */
		TemporaryFile(IFileIOServices &svc, PSeqFileHandle handle, String filename)
			:filename(filename)
			,writeHandle(handle)
			,svc(svc) {}
			

		///Retrieves handle of file, which can be passed to SeqFileOutput;
		PSeqFileHandle getStream() const {return writeHandle;}
		///Retrieves path-name of file. You can pass name to other process to open this file
		String getFilename() const {return filename;}
		///Closes the file
		/**
		 * You have to close the file before it can be opened by another process. Function
		 * detaches the file handle from this instance, and if it is the last reference
		 * file is closed. You have ensure, that there is no more references. After
		 * file is closed, function getStream() starts to return nil. There is no way
		 * how to set handle back. To reopen file, use standard SeqFileInput or SeqFileOutput
		 * classes with name retrieved by function getFilename()
		 */
		void close() {writeHandle = nil;}
		///dtor - destroys file
		~TemporaryFile();
		///detaches this class from the file.
		/** This is the way, how to prevent removing temporary file by destructor.
		 * note that you should remove file manually later. */
		void detach() {filename.clear();}

		///Renames temporary file
		/** Function also closes the file (file cannot be renamed while it is opened)
		    @param newName new name
			@param overwrite allow overwrite target
			
			@note File is still tracked and will be removed on destruction. Function is useful
			when you need to change name or extension to change type of the file (for example in Windows)
		*/
		void rename(ConstStrW newName, bool overwrite);

	protected:

		String filename;
		PSeqFileHandle writeHandle;
		IFileIOServices &svc;

	};

	typedef RefCntPtr<TemporaryFile> PTemporaryFile;
    ///interface that implements I/O services
    class IFileIOServices: virtual public IInterface {
    public:
        
        ///File open mode
        enum FileOpenMode {
        	///do not open file, only test accessibility. Used in special cases only
        	fileAccessible = 0,
            ///open file for reading only
            fileOpenRead = 1,
            ///open file for writing only
            fileOpenWrite = 2,
            ///open file for reading and writing (not applyable to sequential files)
            fileOpenReadWrite =3,
            ///open file for executing, only test accessibility, Used in special cases only
            fileExecutable = 4,
        };

        ///Type of standard file
        enum StdFile {
            ///Standard input
            stdInput = 0,
            ///Standard output
            stdOutput = 1,
            ///Standard error
            stdError = 2
        };


        


        ///Opens sequential file
        /**
         * @param ofn name and parameters of file as OpenFile object
         * @param mode open mode (only fileOpenRead and fileOpenWrite are valid)
         * @return shared pointer to I/O interface
         * @exception IOException I/O error
         * @exception FileNotFoundException File or path not found
         */
        virtual PSeqFileHandle openSeqFile(ConstStrW fname,  FileOpenMode mode, OpenFlags::Type flags) = 0;
        
        ///Opens standard file
        /**
         * @param stdfile standard file type
         * @return shared pointer to I/O interface
         */
        virtual PSeqFileHandle openStdFile(StdFile stdfile) = 0;
        
        ///Creates simple pipe
        /** Pipe allows to transfer data between threads or processes.
         * @param readEnd reference to uninitialized pointer that will be
         *      filled with pointer to interface to the reading end of pipe
         * @param writeEnd reference to uninitialized pointer that will be
         *      filled with pointer to interface to the writing end of pipe
         * @exception IOException I/O error
         */        
        virtual void createPipe(PSeqFileHandle &readEnd, PSeqFileHandle &writeEnd) = 0;

		///Creates temporary file
		/**
		 * @param prefix any text used as prefix to filename. It should contain only alphabetic and numeric
		 * characters. Operation system can truncate too long prefixes. Prefix also can be empty.
		 * @param rndfile specifies, whether to create file with random name. Default value is
		 *  true. If false specified, prefix is used to name of the file. Note that
		 *  function will not open existing file. In this case, object is created
		 * with closed stream. But you can still receive full filename
		 */
		virtual PTemporaryFile createTempFile(ConstStrW prefix, bool rndfile = true) = 0;

         ///Opens random access file
         /**
         * @param ofn name and parameters of file as OpenFile object
         * @param mode open mode 
         * @return shared pointer to I/O interface
         * @exception IOException I/O error
         * @exception FileNotFoundException File or path not found
         */
        virtual PRndFileHandle openRndFile(ConstStrW ofn, FileOpenMode mode, OpenFlags::Type flags) = 0;

		


        ///Opens file using raw handle received from 3rd part library or operation system
        /**
         * @param handle pointer to handle. Depends on curren platform
         * @param handleSize size of handle. It used to check, whether handle has expected size
         * @param mode how to open handle.
         * @return in success, returns pointer to the stream.
         *
         * @note Destructor closes the handle
         *
         * @note Under linux, file descriptor (int) is expected. Under Windows,
         * HANDLE is expected
         */
        //@{
        virtual PSeqFileHandle openSeqFile(const void *handle, size_t handleSize, FileOpenMode mode) = 0;

        virtual PRndFileHandle openRndFile(const void *handle, size_t handleSize, FileOpenMode mode) = 0;
        //@}

        virtual ~IFileIOServices() {}
        
 
        virtual bool canOpenFile(ConstStrW name, FileOpenMode mode) const = 0;


        ///Opens directory for reading entries
        /**
         *
         * @param pathname
         * @return object which can be used to iterator through the directory.
         *
         * @note function will not return nil. In case of error, throws exception
         */
        virtual PDirectoryIterator openDirectory(ConstStrW pathname)  = 0;

		///Retrieves information about file or directory
		/**
		 * @param pathname path to directory or file
		 * @return function returns DirectoryIterator containing one specified
		 * file or folder. You can retrieve information about the file. 
		 * Note that getNext() function will return false;
		 * Function throws exception in case that file is not found
		 */
		 
		virtual PDirectoryIterator getFileInfo(ConstStrW pathname)  = 0;


        ///Retrieves reference to the object implementing this interface
        /** Implementation is different in each platform. Implementation
         * can be also changed in runtime by calling setIOServices()
         * 
         * @return reference to the service object
         */
        static IFileIOServices &getIOServices();
        
        ///Changes implementation of FileIOServices
        /**
         * @param newServices pointer to object, that implementing the
         * FileIOServices. If you specify NULL, original object will
         * be returned.
         */
        static void setIOServices(IFileIOServices *newServices);


        ///Creates specified folder
        /**
         * @param folderName name of folder
         * @param parents if true, creates all parents folders. In this case, function will
         * not fail, when target already exists and is folder
         */
        virtual void createFolder(ConstStrW folderName, bool parents)  = 0;

        ///Removes folder
        /**
         * @param folderName name of the folder including path
         * @param recursive true to remove all files inside of this folder
         */
        virtual void removeFolder(ConstStrW folderName, bool recursive) = 0;

        ///Makes copy of the file
        /**
         * @param from source path
         * @param to target path
         * @param overwrite true to overwrite target
         */
        virtual void copy(ConstStrW from, ConstStrW to, bool overwrite) = 0;

        ///Moves or renames file
        /**
         * @param from old pathname
         * @param to new pathname
         * @param overwrite true to overwrite file at target pathname
         *
         * @note overwritting don't need to be atomic. Function can emulate
         * overwritting by erasing target and then performing the move
         */
        virtual void move(ConstStrW from, ConstStrW to, bool overwrite) = 0;

        ///Makes symbolic link
        /**
         * @param linkName name of the link
         * @param target target file, where link will point
         * @param overwrite true to overwrite file with same name
         */
        virtual void link(ConstStrW linkName, ConstStrW target, bool overwrite) = 0;


        ///Removes file from the filesystem
        /**
         * @param what path-name of file to remove
         */
        virtual void remove(ConstStrW what) = 0;

        ///Creates object which allows to map file into the memory
        /**
         * @param filename filename to map
         * @param how open the file
         * @return pointer to object responsible to memory mapping
         */
        virtual PMappedFile mapFile(ConstStrW filename, FileOpenMode mode) = 0;


        ///Searches file in list of paths
        /**
         * @param filename name or relative path to search
         * @param pathList list of path, separated by specified character
         * @param position if there is more than one result, specifies order
         * @return found file. Returns content of filename in case that file not exists
         */
        virtual String findFile(ConstStrW filename, ConstStrW pathList,
        										natural position = 1)  = 0;

        ///Searches executable
        /**
         * Useful to search paths to shell commands, for example in Linux or Windows.
         * Function returns pathname of command, which can be executed using object Process
         * @param execName name of file to search
         * @return pathname to file
         *
         * Function searches PATH environment variable.
         */
        virtual String findExec(ConstStrW execName) = 0;

	};

	class SysTime;

    ///Simplified iterator for reading directories
    /** It doesn't extends IIterator, but you can easy to create wrapper class
     *
     *
     */
    class IDirectoryIterator: public RefCntObj {
    public:

    	enum DirEntryType {
    		///entry is regular file allows random and sequential acces
    		file,
    		///entry is directory, it can be opened as directory
    		directory,
    		///entry is file allows sequential access only. It can be pipe or socket
    		seqfile,
    		///entry has special meaning, such a block or character device and other
    		special,
    		///type of entry is not known
    		unknown,
			///link to parent or self directory (presented as '.' and '..')
			dots
    	};


    	class IFileInformation {
    	public:
    		virtual ~IFileInformation() {}
    	};

    	///Contains source path or mask. It refers to parameter used with command openDirectory
    	ConstStrW source;
    	///name of entry currently processed
    	ConstStrW entryName;
    	///type of entry
    	DirEntryType type;
    	///extra attributes, which can have different meaning on different platforms
    	natural extra;
    	///if true, entry is symbolic link, which refers to another file.
    	bool link;
		///true if entry is hidden
		bool hidden;

    	///moves to next file in the directory
    	/**
    	 * @retval true success
    	 * @retval false no more entries
    	 */
    	virtual bool getNext() = 0;
    	///rewinds current pointer to the start
    	virtual void rewind() = 0;
    	///retrieves size of file
    	/**
    	 * @return size of the file in the bytes. Because this information
    	 * is not often available during iteration
    	 */
    	virtual lnatural getSize() const = 0;

    	///Retrieves platform specific information about the entry
    	virtual const IFileInformation &getInfo() const = 0;

    	///Retrieves modification time
    	virtual TimeStamp getModifiedTime() const = 0;

    	///Retrieves allowed open mode
    	/**
     	 * @retval fileAccessible file exists and can be opened with this mode,
    	 * 	but cannot be opened with other modes
    	 * @retval fileOpenRead file can be opened for reading only
    	 * @retval fileOpenWrite file can be opened for writting only.
    	 * @retval fileOpenReadWrite file can be opened for both modes. It
    	 * includes fileOpenRead and fileOpenWrite
    	 *
    	 */
    	virtual IFileIOServices::FileOpenMode getAllowedOpenMode() const = 0;


    	///Allows open current directory to process directories recursive
    	/**
    	 * Current field must be directory, otherwise, function throws exception
    	 * @return
    	 */
    	virtual PDirectoryIterator openDirectory() const = 0;

		///Retrieves full path of current file
    	virtual String getFullPath() const = 0;

		///Opens current file as sequence file
		/**
		 * Function expects object able to be opened as file
		 * @param mode open mode
		 * @param flags open flags
		 */
		virtual PSeqFileHandle openSeqFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags) = 0;
                
		///Opens current file as random access
		/**
		 * Function expects object able to be opened as file
		 * @param mode open mode
		 * @param flags open flags
		 */
        virtual PRndFileHandle openRndFile(IFileIOServices::FileOpenMode mode, OpenFlags::Type flags) = 0;

		///Removes file or folder whatever it is
		/** 
		 * @param recursive if current object is container, removes everything in it, otherwise, it returns error
		 */
		virtual void remove(bool recursive = false) = 0;

		///Maps current file
		virtual PMappedFile mapFile(IFileIOServices::FileOpenMode mode) = 0;


		///Copies current file or folder to new location
		virtual void copy(ConstStrW to, bool overwrite) = 0;


		///Moves current file or folder to new location
		virtual void move(ConstStrW to, bool overwrite) = 0;


    	virtual ~IDirectoryIterator() {}

		bool isDirectory() const { return type == directory; }
		bool isDot() const { return type == dots; }
		bool isFile() const { return type == file; }
	};

    ///Interface for mapping files into memory
    /**
     * You can use this interface to map parts of file, or whole file into the memory.
     * Once file is mapped, you can leave the smart pointer to interface because
     * every mapped region keeps owns copy of this pointer
     *
     */
    class IMappedFile: public RefCntObj {
    public:


    	///Part of file mapped into the memory.
    	/**
    	 * The object is complette read only, once is created, it cannot be modified.
    	 * It can be copied and destroyed. Copying causes sharing mapped region,
    	 * while destroying unmaps only with last instance destruction
    	 *
    	 */
    	class MappedRegion: public SharedResource {
    	public:

    		///Contains pointer to IMappedFile interface
    		const PMappedFile owner;
    		///Contains first address of file content
    		void * const address;
    		///Contains size in bytes of file content
    		const natural size;
    		///Offset in the file mapped to first address
    		/**
    		 * @note offset can be different than requested, because on some platforms, alignment is requested.
    		 * You have to calculate respect this offset file making transformations from file location to address
    		 */
    		const IRndFileHandle::FileOffset offset;

    		///Constructs object manually
    		/**
    		 *
    		 * @param owner owner of this region
    		 * @param address start address
    		 * @param size size in bytes
    		 *
    		 * @note Area must be already mapped at given addresses, constructor
    		 * doesn't perform mapping. Use IMappedFile::map() function to
    		 * create instance of MappedRegion including physically mapping
    		 */
    		MappedRegion(PMappedFile owner, void *address, natural size, IRndFileHandle::FileOffset offset)
    			:owner(owner),address(address),size(size),offset(offset) {}

    		///destructor, unmaps region on destruction, if not shared
    		~MappedRegion() {
    			if (!SharedResource::isShared()) owner->unmap(*this);
    		}
    		///Manual synchronization
    		/**Forces synchronization of content with target file, ensuring
    		 * that all dirty pages will be written back to the disk file.
    		 */
    		void sync() {
    			owner->sync(*this);
    		}
    		///Locks whole area
    		/**
    		 * When area is locked, it cannot be swapped out. Feature depends
    		 * on operation system. If locking is not supported or limited, function
    		 * does noting, emulates, or applies limits silently
    		 */
    		void lock() {
    			owner->lock(*this);
    		}

    		///Unlocks whole area
    		/**
    		 * Feature depends
    		 * on operation system. If locking is not supported or limited, function
    		 * does noting, emulates, or applies limits silently
    		 */
    		void unlock() {
    			owner->unlock(*this);
    		}
};

    	///Creates mapped region of the file
    	/**
    	 * @param offset offset of the file, where mapping starts. Because
    	 *  some platforms require to align this value, function can
    	 *  adjust offset to the nearest lower available address. The result
    	 *  is stored into MappedRegion::offset variable
    	 *
    	 * @param size size in bytes of the mapped area. Because
    	 *  some platforms require to align this value, function can
    	 *  adjust the size to the nearest higher available value. The result
    	 *  is stored into MappedRegion::size variable.
    	 *
    	 * @param mode Which operation will be performed on the mapped area.
    	 *    Note that mode must be conform with requested access mode when
    	 *	  IFileIOServices::mapFile has been called. You cannot request writting,
    	 *	  when reading has been requested while creation of this object.
    	 *	  There is only exception while copyOnWrite is used.
    	 *
    	 * @param copyOnWrite this flag is valid only when writing is requested.
    	 * 	  Function will allow writing into the memory even if
    	 * 	  background file is opened in read/only mode. But every write causes
    	 * 	  making copy of whole page with no effect on original file. Copies
    	 * 	  are visible for this process only and cannot be written back.
    	 *
    	 * @return On success, creates object MappedRegion
    	 */
    	virtual MappedRegion map(IRndFileHandle::FileOffset offset, natural size,
    			IFileIOServices::FileOpenMode mode, bool copyOnWrite) = 0;
		
		///Maps whole file into the memory
		/**
		* @param mode Which operation will be performed on the mapped area.
		*    Note that mode must be conform with requested access mode when
		*	  IFileIOServices::mapFile has been called. You cannot request writting,
		*	  when reading has been requested while creation of this object.
		*	  There is only exception while copyOnWrite is used.
		*
		* @param copyOnWrite this flag is valid only when writing is requested.
		* 	  Function will allow writing into the memory even if
		* 	  background file is opened in read/only mode. But every write causes
		* 	  making copy of whole page with no effect on original file. Copies
		* 	  are visible for this process only and cannot be written back.
		* @return On success, creates object MappedRegion
		*
		*/		 
		virtual MappedRegion map(IFileIOServices::FileOpenMode mode, 
				bool copyOnWrite) = 0;

    	virtual ~IMappedFile() {}
    protected:
    	friend class MappedRegion;
    	virtual void unmap(MappedRegion &reg) = 0;
    	virtual void sync(MappedRegion &reg) = 0;
    	virtual void lock(MappedRegion &reg) = 0;
    	virtual void unlock(MappedRegion &reg) = 0;
    };


    ///Access to global or stream HTTP settings
    /**
     * To access global settings, use IFileIOServices::getIfc<IHTTPSettings>. To access stream settings,
     * use ISeqFileHandle::getIfc<IHTTPSettings>.
     */
    class IHTTPSettings: virtual public IRefCntInterface {
    public:
    	enum ProxyMode {
    		///direct connect (no proxy)
    		pmDirect,
    		///use OS decision. Use system proxy settings
    		pmAuto,
    		///specify proxy manually
    		pmManual,
    	};


		virtual void setUserAgent(const String &uagent) = 0;
		virtual String getUserAgent() const = 0;
		virtual void setProxy(ProxyMode md, String addr, natural port) = 0;
		virtual void setProxy(ProxyMode md, ConstStrA addrport);
		virtual ProxyMode getProxy() const = 0;
		virtual void getProxySettings(String &addr, natural &port) const = 0;
		///Enables cookies
		/**
		 This allows to object save and load cookies internally.
		 Implementation strongly depends on platform, but can
		 be useful, if you want to make multiple requests while
		 cookies are kept between requests without need to implement
		 fully functional cookie store.

		 Under Microsoft Windows, function uses global cookie
		 repository, which is shared by all applications in
		 context of current user. Session cookies are valid until
		 processes exists.

		 Under Linux, cookies are stored into temporary file, which
		 is deleted on process exit.

		 @param enable true to enable cookies, false to disable cookies
		 */
		virtual void enableCookies(bool enable) = 0;
		virtual bool areCookiesEnabled() const = 0;
		virtual natural getIOTimeout() const = 0;
		virtual void setIOTimeout(natural timeout)= 0;
		///Forces connection to use specific IP protocol
		/**
		 * @param ver specify 0 to let service choose best version.
		 * 			To force  IPv4 specify 4.
		 * 			To force  IPv6 specify 6
		 * 			Any other number can generate exception or can be treat as 0.
		 *
		 * Note that depend on service provider, this option can be ignored or exception may occur.
		 * If you forces to IPv6 on system without IPv6 support, exception can occur during the request.
		 */
		virtual void forceIPVersion(natural ver) = 0;
    };

	///This interface is available,when resource is received from HTTP source
	/** To access this interface, use IInterface */

	class IHTTPStream: public ISeqFileHandle, public IHTTPSettings {
	public:


		///Allows to change request method
		/**
		 * Function must be called before first data are read or sent
		 * 
		 * @param method text string contains HTTP method. Default value
		 *   id "GET". You can change it to "HEAD", "POST", "PUT" and other
		 *   methods. Methods that expects body in the request requires to define
		 *  Content-Length: in header and the body is sent using write
		 *  write() called before read().
		 *
		 * @note Not all implementation supports all methods. Function
		 * is useful to change default GET to POST. To perform advanced
		 * task in HTTP protocol you need special library
		 *
		 * @note After writing first byte, method is changed to POST.
		 *
		 * @note Linux's simple implementation doesn't allow to change method
		 * from POST to GET. You can specify POST, which causes appending
		 * empty buffer to HTTP request. HEAD method is emulated cutting down
		 * data stream after request is sent (wget doesn't allow enforce HEAD request).
		 *
		 */
		 
		virtual IHTTPStream &setMethod(ConstStrA method) = 0;

		///Sets additional header
		/**
		 * @param headerName name of header
		 * @param headerValue content of header
		 *
		 * @note existing headers are replaced
		 */
		virtual IHTTPStream &setHeader(ConstStrA headerName, ConstStrA headerValue) = 0;


		///Cancels any pending waiting operation
		/**
		 * Function should be called from different thread. 
		 *                                                  
		 * After canceling request, state of object is undefined. You 
		 * cannot receive reply headers, and stream can be uncompleted.
		 * Some implementation may cause throwing exception from read()
		 * function.
		 */
		virtual IHTTPStream &cancel() = 0;

		///Retrieves reply headers
		/** Headers are available after first byte is sent, or
		 * after connect() is returned. If request is
		 * needs body, headers are available after body is sent
		 */
		virtual StringA getReplyHeaders() = 0;


		///Retrieves header from the reply
		/**
		 * @param field case insensitive field name
		 * @return content of the header
		 * 
		 * Asking for headers causes to send request and flush output stream. 
		 * Function then waits for the reply. 
		 */
		 
		virtual ConstStrA getHeader(ConstStrA field) = 0;


		class IEnumHeaders {
		public:
			virtual bool operator()(ConstStrA field, ConstStrA value) = 0;
		};

		virtual bool enumHeaders(IEnumHeaders &enumHdr) = 0;

		///Retrieves whether object is connected
		/** @retval true object is connected and you can download data.
		 * This state is also reported, after connection of POST request while
		 * sending request body */
		virtual bool inConnected() const = 0;
	
		///Disables automatic redirects
		/**
		 * By default, any redirect is interpreted. In this case
		 * you never get redirect HTTP code as reply. Call this function
		 * to disable automatic redirect for this request. 
		 *
		 * Function must be called before request is sent, otherwise it
		 * has no effect
		 */
		 
		virtual IHTTPStream &disableRedirect() = 0;

		///Disables status exceptions
		/** 
		 * By default, any status different to 200 (and different to redirects,
		 * when automatic redirections are not disabled) are thrown as 
		 * exception. Calling this method causes, that exception is not thrown
		 * and you can read status code using method getStatusCode(). 
		 *
		 * Default settings allows to simulate disk-stream when consumer cannot
		 * manually check status code and does't access this interface
		 */
		virtual IHTTPStream &disableException() = 0;

		///Connects the remote server by address
		/** 
		 * By default, connection made when first I/O is requested. You
		 * can connect manually using this function causing send request
		 * If method requtires body of request, you shoul send whole
		 * body after the request */
		virtual IHTTPStream &connect() = 0;

		///Retrieves status code
		/**
		 * @return status code of operation. Status code is available
		 * after request is sent including the body
		 *		 
		 */
		virtual natural getStatusCode() = 0;
		
	};

	inline TemporaryFile::~TemporaryFile() {
		close();
		if (!filename.empty()) svc.remove(filename);
	}

	inline void TemporaryFile::rename(ConstStrW newName, bool overwrite) {
		close();
		svc.move(filename, newName, overwrite);
		filename = newName;
	}
}
#endif
