#include "stdlogoutput.h"
#include "../exceptions/errorMessageException.h"

namespace LightSpeed {

	void StdLogOutput::setFile(SeqFileOutput out)
	{
		Sync _(lock);
		outFile = out;
		logFilename.clear();
	}



	void StdLogOutput::setFile(ConstStrW fname)
	{
		Sync _(lock);
		setFileLk(fname);
	}

	void StdLogOutput::setFileLk(ConstStrW fname)
	{
		using namespace OpenFlags;
		logFilename = fname;
		outFile = SeqFileOutput(logFilename,append|createFolder|create|shareRead);
	}



	void StdLogOutput::enableStderr(bool enable)
	{
		Sync _(lock);
		stdErrEnabled = enable;
		if (!stdErrEnabled) stdErr = SeqFileOutput(nil);

	}


	void StdLogOutput::logRotate() {
		if (logFilename.empty()) throw ErrorMessageException(THISLOCATION, "Cannot rotate logfile, because log is not opened using filename");
		Sync _(lock);
		outFile = nil;
		setFileLk(logFilename);
	}


void StdLogOutput::logLine(ConstStrA line, ILogOutput::LogType ,natural ) {
	Sync _(lock);
	ConstStringT<byte> outTextB(reinterpret_cast<const byte *>(line.data()),line.length());
	if (outFile.getHandle() != nil) outFile.blockWrite(outTextB);
	if (stdErrEnabled) {
		if (stdErr.getHandle() == nil)
			stdErr = StdError();
		stdErr.blockWrite(outTextB);
	}

}

}
