#ifndef LIGHTSPEED_EXCEPTIONS_FILEEXCEPTIONS_H_
#define LIGHTSPEED_EXCEPTIONS_FILEEXCEPTIONS_H_

#include "ioexception.h"
#include "systemException.h"

namespace LightSpeed {
    
    
    
    
    
    class FileIOException: public IOException, public ErrNoException {
    public:
        LIGHTSPEED_EXCEPTIONFINAL;

        FileIOException(const ProgramLocation &loc, int errNr, const String &fname)
            :Exception(loc),IOException(loc),ErrNoException(loc,errNr),fname(fname) {}
        
        const String &getFilename() const {
            return fname;
        }
        virtual ~FileIOException() throw() {}
        virtual const char *getStaticDesc() const {
            return "Exception is thrown, when request to an operation with file is rejected "
                    "by the operation system. Error code and filename is stored with this exception";
        }

        
    protected: 
        String fname;

        virtual void message(ExceptionMsg &msg) const{
            return ErrNoException::message(msg);
        }
                
    
    };
    
    ///Exception describes all open error exceptions
    class FileOpenError: public FileIOException {
    public:
        FileOpenError(const ProgramLocation &loc, int errn, const String &name):
            Exception(loc),FileIOException(loc,errn,name) {}

        LIGHTSPEED_EXCEPTIONFINAL;
        
        virtual void message(ExceptionMsg &msg) const{
            
        	msg(msgText) << fname;
            ErrNoException::message(msg);
        }
        virtual ~FileOpenError() throw() {}

        static LIGHTSPEED_EXPORT const char *msgText;;
    };
    
    class FileIOError: public FileIOException {
    public:
        FileIOError(const ProgramLocation &loc, int errn, const String &name):
            Exception(loc),FileIOException(loc,errn,name) {}

        LIGHTSPEED_EXCEPTIONFINAL;
        
        virtual void message(ExceptionMsg &msg) const{

        	msg(msgText) << fname;
            ErrNoException::message(msg);
        }
        virtual ~FileIOError() throw() {}

        static LIGHTSPEED_EXPORT const char *msgText;;
    };
    
    class PipeOpenError: public FileOpenError {
    public:
        PipeOpenError(const ProgramLocation &loc, int errn):
            Exception(loc), FileOpenError(loc,errn,String(L"#pipe")) {}
        LIGHTSPEED_EXCEPTIONFINAL;
    };

	class FileMsgException: public FileIOException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		FileMsgException(const ProgramLocation &loc,
			natural errnr, const String &name,
			const String &message)
			:FileIOException(loc,(int)errnr,name)
			,msg(message) {}

		const String &getExcpMessage() const {return msg;}

		virtual ~FileMsgException() throw() {}
	protected:
		String msg;

		void message(ExceptionMsg &msg) const {
			msg("%1 (%2): ") << this->msg << fname;
			FileIOException::message(msg);
		}
	};

	class FolderCreationException: public FileIOException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		FolderCreationException(const ProgramLocation &loc, int errNr,
				const String &fname)
			:FileIOException(loc,errNr,fname) {}

        virtual void message(ExceptionMsg &msg) const{
        	msg(msgText) << fname;
            return ErrNoException::message(msg);
        }

        static LIGHTSPEED_EXPORT const char *msgText;;

	};

	class FileDeletionException: public FileIOException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		FileDeletionException(const ProgramLocation &loc, int errNr, const String &fname)
			:FileIOException(loc,errNr,fname) {}
		virtual void message(ExceptionMsg &msg) const{
			msg(msgText) << fname;
			return ErrNoException::message(msg);
		}
		static LIGHTSPEED_EXPORT const char *msgText;;
	};

	class FolderNotEmptyException: public FileDeletionException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		FolderNotEmptyException(const ProgramLocation &loc, int errNr, const String &fname)
			:FileDeletionException(loc,errNr,fname) {}
		virtual void message(ExceptionMsg &msg) const{
			msg(msgText) << fname;
			return ErrNoException::message(msg);
		}
		static LIGHTSPEED_EXPORT const char *msgText;;
	};

}

#endif /*FILEEXCEPTIONS_H_*/
