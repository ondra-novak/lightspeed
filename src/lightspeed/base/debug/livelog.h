/*
 * InteractiveLog.h
 *
 *  Created on: 9.10.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_DEBUG_LIVELOG_
#define LIGHTSPEED_BASE_DEBUG_LIVELOG_

#include "stdlogoutput.h"



namespace LightSpeed {


class LiveLog;


///Abstract stream for LiveLog
/** you have to implement some functions there */
class LiveLogStream {
public:

	///constructor of LiveLogStream
	/**
	 * @param logLevel requested log level for this stream. Note that you
	 * can specify simple the level, but not sublevel. Messages from this
	 * level and above are complete allowed while other messages are disallowed
	 * (filtered out)
	 */
	LiveLogStream(ILogOutput::LogType logLevel);

	///Called by LiveLog object to deliver message to the listener
	/**
	 * @param data binary representation of the message. Messages are sent as
	 * plain text so you can cast byte to char to read text stored inside of variable.
	 *
	 * Message is send with new line separator, which can be different on different
	 * platforms. On Linux messages are terminated by \\n while on Windows
	 * messages are terminated by \\r\\n.
	 */
	virtual void onData(ConstStringT<byte> data) = 0;

	///Called to close LiveLogStream when stream is being unregistered
	virtual void onClose() = 0;

	///destructor
	virtual ~LiveLogStream();

	///Retrieves current log level
	ILogOutput::LogType getLogLevel() const {return logLevel;}

protected:
	friend class LiveLog;

	void onRegister(LiveLog *toSlot) {
		slottedIn = toSlot;
	}

	void onUnregister() {
		if (slottedIn) {
			slottedIn = 0;
			onClose();
		}
	}

	LiveLog *slottedIn;
	ILogOutput::LogType logLevel;
};


///Extension of StdLogOutput allows dynamically add and remove listeners to log output
/** LiveLog can be used to implement sneak peek to current server's activity. Listener,
 * for example Http stream can be registered to LiveLog interface and it able to
 * receive log messages as ordinary log output for limited time (until it is disconnected)
 *
 * Every registered LiveLog stream can specify own loglevel which will listen and it
 * can also include levels that are disabled in StdLogOutput.
 *
 * To implement LiveLog stream, read description of LiveLogStream object.
 */
class LiveLog: public StdLogOutput {
public:
	///constructor
	/** You have to construct this object as Singleton<LiveLog>. To use LiveLog as
	 * LogProvider, register Singleton<LiveLog>::castInstance<ILogOutput>() function
	 * as log povider, or supply own function which will return instance of LiveLog
	 * singleton.
	 */
	LiveLog();
	///Destructor - should be called at the very end
	virtual ~LiveLog();

	///Registers stream to the LiveLog interface
	/** Once stream is registered, it starts to receive log messages.
	 * Note that log stream can be accessed from various threads
	 *
	 * @param stream stream to register
	 */
	void add(LiveLogStream *stream);

	///Removes stream from the LiveLog interface
	/**
	 * You don't need to call this function before you destroy the stream. LiveLogStream's
	 * destructor handles this by self.
	 *
	 * @param stream pointer to stream
	 */
	void remove(LiveLogStream *stream);

protected:
	typedef AutoArray<LiveLogStream *> Channels;
	Channels channels;
	ILogOutput::LogType maxLogLevel;

	void logLine(ConstStrA line, ILogOutput::LogType ,natural );

	bool checkLogLevel(LogType type, natural level);

	void removeLk(LiveLogStream *stream);

};

} /* namespace snapytap */
#endif /* LIGHTSPEED_BASE_DEBUG_LIVELOG_ */
