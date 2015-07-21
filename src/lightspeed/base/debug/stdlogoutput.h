#pragma once
#include "dbglog.h"
#include "../containers/constStr.h"
#include "../streams/fileio.h"
#include "../containers/autoArray.h"
#include "../../mt/fastlock.h"
#include "LogProvider.h"

namespace LightSpeed {

class StdLogOutput: public AbstractLogProvider
{
public:

	StdLogOutput():outFile(nil),stdErr(nil),stdErrEnabled(true) {}

	void setFile(ConstStrW fname);
	void enableStderr(bool enable);
	void setFile(SeqFileOutput out, ConstStrW fname);
	//void onTextOut();
	void logRotate();
	void logLine(ConstStrA line, ILogOutput::LogType logType, natural level) ;



protected:
	SeqFileOutput outFile;
	SeqFileOutput stdErr;
	bool stdErrEnabled;
	String logFilename;

	FastLock lock;
	typedef Synchronized<FastLock> Sync;

public:
};

}
