/*
 * InteractiveLog.cpp
 *
 *  Created on: 9.10.2013
 *      Author: ondra
 */

#include "livelog.h"
#include "../memory/staticAlloc.h"

namespace LightSpeed {


LiveLogStream::LiveLogStream(
		ILogOutput::LogType logLevel):logLevel(logLevel) {
}

LiveLogStream::~LiveLogStream() {
	if (slottedIn) {
		LiveLog *s = slottedIn;
		slottedIn = 0;
		s->remove(this);
	}
}

LiveLog::LiveLog() {
	maxLogLevel = (ILogOutput::LogType)0;
}

LiveLog::~LiveLog() {
	for (Channels::Iterator iter = channels.getFwIter(); iter.hasItems();) {
		LiveLogStream *s = iter.getNext();
		s->onUnregister();
	}
}

void LiveLog::add(LiveLogStream* stream) {
	{
		Sync _(lock);
		channels.add(stream);
		LogType p = stream->getLogLevel();
		if (p > maxLogLevel) maxLogLevel = p;
		stream->onRegister(this);
	}
	LogObject lg(THISLOCATION);
	lg.note("Livelog: connected listener %1") << (natural)stream;
}

void LiveLog::remove(LiveLogStream* stream) {
	Sync _(lock);
	removeLk(stream);
}
void LiveLog::removeLk(LiveLogStream* stream) {
	maxLogLevel = (ILogOutput::LogType)0;
	stream->onUnregister();
	for (natural i = 0; i < channels.length(); i++) {
		LiveLogStream *s = channels[i];
		if (s == stream) {
			channels.erase(i);
			i--;
		} else {
			LogType p = s->getLogLevel();
			if (p > maxLogLevel) maxLogLevel = p;

		}
	}
}

void LiveLog::logLine(ConstStrA line, ILogOutput::LogType curLogType ,natural curLogLevel) {
	if (curLogType > maxLogLevel) {
		StdLogOutput::logLine(line,curLogType,curLogLevel);
	} else {
		if (StdLogOutput::checkLogLevel(curLogType,curLogLevel)) {
			StdLogOutput::logLine(line,curLogType,curLogLevel);
		}
		Sync _(lock);
		DbgLog::Redirect logred(DbgLog::getNoLogProvider());
		Channels failedChans;
		for (Channels::Iterator iter = channels.getFwIter(); iter.hasItems();) {
			LiveLogStream *s = iter.getNext();
			if (s->getLogLevel() >= curLogType) {
				try {
					s->onData(ConstStringT<byte>(reinterpret_cast<const byte *>(line.data()),line.length()));
				} catch (...) {
					failedChans.add(s);
				}
			}
		}
		for (Channels::Iterator iter = failedChans.getFwIter(); iter.hasItems();) {
			LiveLogStream *s = iter.getNext();
			removeLk(s);
			try {
				if (checkLogLevel(logNote,1)) {
					TextFormatBuff<char, StaticAlloc<256> > line;

					line("Livelog: disconnected %1\n") << (natural)s;
					SyncReleased<FastLock> _(lock);
					logLine(line.write(),logNote,1);
				}
			} catch (...) {
			}
		}
	}
}

bool LiveLog::checkLogLevel(LogType type,natural level) {
	if (type > maxLogLevel) return StdLogOutput::checkLogLevel(type,level);
	else return true;
}

} /* namespace snapytap */
