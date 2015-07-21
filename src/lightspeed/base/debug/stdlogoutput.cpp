#include "stdlogoutput.h"
#include "../exceptions/errorMessageException.h"

namespace LightSpeed {

	void StdLogOutput::setFile(SeqFileOutput out, ConstStrW fname)
	{
		Sync _(lock);
		outFile = out;
		logFilename = fname;
	}



	void StdLogOutput::setFile(ConstStrW fname)
	{
		using namespace OpenFlags;
		SeqFileOutput newlog(fname,append|createFolder|create|shareRead|shareWrite);
		setFile(newlog,fname);


	}




	void StdLogOutput::enableStderr(bool enable)
	{
		Sync _(lock);
		stdErrEnabled = enable;
		if (!stdErrEnabled) stdErr = SeqFileOutput(nil);

	}


	void StdLogOutput::logRotate() {
		String x = logFilename;
		if (logFilename.empty()) throw ErrorMessageException(THISLOCATION, "Cannot rotate logfile, because log is not opened using filename");
		setFile(logFilename);
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
