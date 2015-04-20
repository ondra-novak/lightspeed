/*
 * sshHandler.cpp
 *
 *  Created on: 24. 2. 2015
 *      Author: ondra
 */

#include "sshHandler.h"

#include "../../base/text/textOut.tcc"
#include "../../base/iter/iteratorFilter.tcc"
#include "../../mt/process.h"
#include "../../utils/queryParser.h"
#include "../exceptions/fileExceptions.h"
#include "../text/textstream.tcc"
#include "../containers/carray.tcc"
namespace LightSpeed {


struct RefInfo {

	StringA sshlink;
	StringA path;
	StringA options;


};


static RefInfo parseSSHRef(ConstStrW fname) {

	StringA utf8name = String::getUtf8(fname);
	TextParser<char> p;
	RefInfo rfinfo;
	if (!p("ssh://%[^@]1@%[^:]2:/(*)%3",utf8name)) {
		if (!p("ssh://[%[^]]4]%[^@]1@%[^:]2:/(*)%3",utf8name)) {
			throw FileOpenError(THISLOCATION,2,fname);
		}
		rfinfo.options = p[4].str();
	}

	rfinfo.sshlink = p[1].str() + ConstStrA('@') + p[2].str();
	StringA path = ConstStrA("/")+p[3].str();

	return rfinfo;
}

static String findSSH() {
	IFileIOServices &svc = IFileIOServices::getIOServices();
	return svc.findExec(L"ssh");
}

static void prepareProcess(Process& proc,const  RefInfo& refInfo, const String &cmdLine) {
	proc.cmdLine(refInfo.options,true);
	proc.arg(refInfo.sshlink);
	proc.arg(cmdLine);
}

static void performAction(RefInfo &refInfo, String cmdline) {

	Process proc(findSSH());


	prepareProcess(proc, refInfo, cmdline);
	SeqFileInput errStr(0);
	AutoArrayStream<char> result;
	proc.stdOutErr(errStr);

	proc.start();
	SeqTextInA errText(errStr);
	result.copy(errText);

	int res = proc.join();
	if (res != 0) {
		throw ErrNoWithDescException(THISLOCATION,res,ConstStrA(result.getArray()));
	}
}

class SSHStreamCommon: public IInOutStream {
public:
	SSHStreamCommon(String fname):fname(fname) {}

protected:
	mutable PInputStream ssherror;

	void readError() const {
		AutoArrayStream<char> msg;
		SeqFileInput errin(ssherror);
		msg.copy(errin);
		throw FileMsgException(THISLOCATION,1,fname,String(ConstStrA(msg.getArray())));
	}

	String fname;
};

static String escapeFname(ConstStrA x) {
	return Process(x).getCmdLine();
}

class SSHStreamOutput: public SSHStreamCommon {
public:

	SSHStreamOutput(String fname, OpenFlags::Type flags):SSHStreamCommon(fname), proc(findSSH()) {
		LightSpeed::ConstStrW sep = ConstStrW(L" && ");


		RefInfo refInfo = parseSSHRef(fname);

		StringA origName = refInfo.path;
		StringA targetName = (flags& OpenFlags::commitOnClose)?StringA(refInfo.path+ConstStrA(".part")):origName;

		SeqFileOutput out(NULL);
		SeqFileInput err(NULL);
		String cmdline = (flags & OpenFlags::append)?(ConstStrW(L"cat >> ")+escapeFname(targetName)):(ConstStrW(L"cat > ")+escapeFname(targetName));
		if (flags & OpenFlags::commitOnClose) {
			cmdline =cmdline + sep+ Process("mv").arg("-f").arg(targetName).arg(
									origName).getCmdLine();
			if (flags & OpenFlags::append) {
				cmdline = Process("cp").arg("-f").arg(origName).arg(targetName).getCmdLine() + sep + cmdline;
			}
		}
		if (flags & OpenFlags::createFolder) {
			natural pos = refInfo.path.findLast('/');
			if (pos > 1) {
				ConstStrA path = refInfo.path.head(pos);
				cmdline = Process("mkdir").arg("-p").arg(path).getCmdLine() + sep + cmdline;
			}
		}
		prepareProcess(proc,refInfo,cmdline);
		proc.stdIn(out);
		proc.stdOutErr(err);
		proc.exec();
		sshstream = out.getStream();
		ssherror = err.getStream();

	}

	~SSHStreamOutput() {
		if (proc.isRunning()) {
			proc.stop(true);
		}
		proc.join();
	}

	Process proc;

    virtual natural read(void *,  natural ) {return 0;}
	virtual natural peek(void *, natural ) const {return 0;}
	virtual bool canRead() const {return false;}
	virtual natural dataReady() const {return true;}
    virtual natural write(const void *buffer,  natural size) {
    	try {
    		return sshstream->write(buffer,size);
    	} catch (Exception &e) {
    		if (ssherror->dataReady()) readError();
    		throw FileIOError(THISLOCATION,1,fname) << e;

    	}

    }
	virtual bool canWrite() const {
    	try {
    		return sshstream->canWrite();
    	} catch (Exception &e) {
    		if (ssherror->dataReady()) readError();
    		throw FileIOError(THISLOCATION,1,fname) << e;
    	}
	}
	virtual void flush() {
    	try {
			return sshstream->flush();
		} catch (Exception &e) {
			if (ssherror->dataReady()) readError();
			throw FileIOError(THISLOCATION,1,fname) << e;
		}
	}
	virtual void closeOutput() {
		try {
			sshstream->closeOutput();
		} catch (Exception &e) {
			if (ssherror->dataReady()) readError();
			else throw FileIOError(THISLOCATION,1,fname) << e;
		}
	}

	POutputStream sshstream;

};

class SSHStreamInput: public SSHStreamCommon {
public:

	SSHStreamInput(String fname):SSHStreamCommon(fname), proc(findSSH()) {

		RefInfo refInfo = parseSSHRef(fname);
		String escname = Process(refInfo.path).getCmdLine();
		SeqFileInput in(NULL);
		SeqFileInput err(NULL);
		String cmdline = ConstStrW(L"cat ")+escname;
		prepareProcess(proc,refInfo,cmdline);
		proc.stdOut(in);
		proc.stdErr(err);
		proc.exec();
		sshstream = in.getStream();
		ssherror = err.getStream();

	}

	~SSHStreamInput() {
		if (proc.isRunning()) {
			proc.stop(true);
		}
		proc.join();
	}

	Process proc;

    virtual natural read(void *buffer,  natural size) {
    	natural res = sshstream->read(buffer,size);
    	if (res == 0 && ssherror->dataReady()) {
    		readError();
    	}
    	return res;
    }
	virtual natural peek(void *buffer, natural size) const {
		natural res = sshstream->peek(buffer,size);
    	if (res == 0 && ssherror->dataReady()) {
    		readError();
    	}
    	return res;
	}
	virtual bool canRead() const {
		return sshstream->canRead();
	}
	virtual natural dataReady() const {
		return sshstream->dataReady();
	}
    virtual natural write(const void *,  natural ) {return 0;}
	virtual bool canWrite() const {return false;}
	virtual void flush() {}
	virtual void closeOutput() {}


	PInputStream sshstream;
	PInputStream ssherror;



};

PInOutStream LightSpeed::SshHandler::openSeqFile(ConstStrW fname,
		FileOpenMode mode, OpenFlags::Type flags) {

	if (mode == fileOpenRead) return new SSHStreamInput(fname);
	else if (mode == fileOpenWrite) return new SSHStreamOutput(fname,flags);
	else throw;//TODO: invalid mode

}

PRndFileHandle LightSpeed::SshHandler::openRndFile(ConstStrW ,FileOpenMode , OpenFlags::Type ) {

	throwUnsupportedFeature(THISLOCATION,this,"Random Access Files aren't currently supported with current ssh driver");
	return 0;
}

bool LightSpeed::SshHandler::canOpenFile(ConstStrW fname,		FileOpenMode mode) const {

	RefInfo rf = parseSSHRef(fname);
	Process proc("test");
	switch(mode) {
	case fileAccessible: proc.arg("-e");break;
	case fileExecutable:proc.arg("-x");break;
	case fileOpenRead:proc.arg("-r");break;
	case fileOpenWrite:proc.arg("-w");break;
	case fileOpenReadWrite:proc.arg("-w");break;
	}
	proc.arg(rf.path);
	String cmdline = proc.getCmdLine();
	if (mode == fileOpenReadWrite) {
		cmdline = cmdline + ConstStrW(L" && ") + Process("test").arg("-r").arg(rf.path).getCmdLine();
	}
	try {
		performAction(rf,cmdline);
		return true;
	} catch (ErrNoException &e) {
		return false;
	}
}

PFolderIterator LightSpeed::SshHandler::openFolder(ConstStrW ) {
	throwUnsupportedFeature(THISLOCATION,this,"SSH driver doesn't support this operation yet");
	return 0;
}

PFolderIterator LightSpeed::SshHandler::getFileInfo(ConstStrW) {
	throwUnsupportedFeature(THISLOCATION,this,"SSH driver doesn't support this operation yet");
	return 0;
}

void LightSpeed::SshHandler::createFolder(ConstStrW folderName, bool parents) {
	RefInfo rf = parseSSHRef(folderName);
	Process proc("mkdir");
	if (parents) proc.arg("-p");
	proc.arg(rf.path);
	performAction(rf,proc.getCmdLine());

}

void LightSpeed::SshHandler::removeFolder(ConstStrW folderName,
		bool recursive) {
	RefInfo rf = parseSSHRef(folderName);
	if (recursive) {
		Process proc("rm");
		proc.arg("-rf").arg(rf.path);
		performAction(rf,proc.getCmdLine());
	} else {
		Process proc("rmdir");
		proc.arg(rf.path);
		performAction(rf,proc.getCmdLine());
	}

}

void LightSpeed::SshHandler::copy(ConstStrW from, ConstStrW to, bool overwrite) {
	RefInfo rf1 = parseSSHRef(from);
	RefInfo rf2 = parseSSHRef(to);
	if (rf1.sshlink != rf2.sshlink) {

		SeqFileInput in(from,0);
		SeqFileOutput out(to,OpenFlags::commitOnClose);
		CArray<byte, 4096> buffer;
		out.blockCopy(in,buffer,naturalNull);

	} else {
		Process proc("cp");
		if (overwrite) proc.arg("-f"); else proc.arg("-n");
		proc.arg(rf1.path).arg(rf2.path);
		performAction(rf1,proc.getCmdLine());
	}

}

void LightSpeed::SshHandler::move(ConstStrW from, ConstStrW to,
		bool overwrite) {
	RefInfo rf1 = parseSSHRef(from);
	RefInfo rf2 = parseSSHRef(to);
	if (rf1.sshlink != rf2.sshlink) {

		SeqFileInput in(from,0);
		SeqFileOutput out(to,OpenFlags::commitOnClose);
		CArray<byte, 4096> buffer;
		out.blockCopy(in,buffer,naturalNull);
		remove(from);

	} else {
		Process proc("mv");
		if (overwrite) proc.arg("-f"); else proc.arg("-n");
		proc.arg(rf1.path).arg(rf2.path);
		performAction(rf1,proc.getCmdLine());
	}
}

void LightSpeed::SshHandler::link(ConstStrW linkName, ConstStrW target, bool overwrite) {
	RefInfo rf1 = parseSSHRef(linkName);
	RefInfo rf2 = parseSSHRef(target);
	if (rf1.sshlink != rf2.sshlink) {
		throw FileMsgException(THISLOCATION,0,target,"Cannot create link between files on different hosts");
	} else {
		Process proc("ln");
		if (overwrite) proc.arg("-f");
		proc.arg("-s");
		proc.arg(rf2.path).arg(rf1.path);
		performAction(rf1,proc.getCmdLine());
	}


}

void LightSpeed::SshHandler::remove(ConstStrW what) {
	RefInfo rf = parseSSHRef(what);
	Process proc("unlink");
	proc.arg(rf.path);
	performAction(rf,proc.getCmdLine());
}

PMappedFile LightSpeed::SshHandler::mapFile(ConstStrW ,FileOpenMode ) {
	throwUnsupportedFeature(THISLOCATION,this,"SSH driver doesn't support file mapping");
	throw;
}

} /* namespace LightSpeed */
