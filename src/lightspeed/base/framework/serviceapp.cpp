/*
* serviceapp.tcc
*
*  Created on: 23.2.2011
*      Author: ondra
*/

#ifndef LIGHTSPEED_FRAMEWORK_SERVICEAPP_TCC_
#define LIGHTSPEED_FRAMEWORK_SERVICEAPP_TCC_


#include "serviceapp.h"
#include "../memory/staticAlloc.h"
#include "../containers/autoArray.tcc"
#include "../streams/utf.h"
#include "../exceptions/errorMessageException.h"
#include "../iter/iteratorFilter.tcc"
#include "../memory/smallAlloc.h"
#include "../text/textOut.tcc"
#include "../containers/string.tcc"
#include "../text/textParser.tcc"
#include "../text/textstream.h"

namespace LightSpeed {

	namespace {
		const char *gstartCmd = "start";
		const char *gstopCmd = "stop";
		const char *gstartForeCmd = "run";
		const char *grestartCmd = "restart";
		const char *gwaitCmd = "wait";
		const char *gstatusCmd = "status";
		const wchar_t *gdefaultInstanceName = L"default";

	}

	const char * ServiceAppBase::startCmd=gstartCmd;
	const char * ServiceAppBase::stopCmd=gstopCmd;
	const char * ServiceAppBase::restartCmd=grestartCmd;
	const char * ServiceAppBase::startForeCmd=gstartForeCmd;
	const char * ServiceAppBase::waitCmd=gwaitCmd;
	const char * ServiceAppBase::statusCmd=gstatusCmd;
	const wchar_t *ServiceAppBase::defaultInstanceName=gdefaultInstanceName;


	ServiceApp::ServiceApp(integer priority, natural timeout)
		:Super(priority),timeout(timeout),instanceFileSize(0),restartDelaySec(0)
	{}




	integer ServiceApp::start(const Args & args)
	{
		StdError serr;

		if (args.length() < 3) return onCommandLineError();
		appname = args[0];
		String instanceName = args[1];
		if (instanceName == defaultInstanceName)  instanceName = getDefaultInstanceName(appname);
		StringA command = String::getUtf8(args[2]);;
		instance = ProgInstance(instanceName);
		Args rmargs(const_cast<ConstStrW *>(args.data()+3),args.length()-3);
		stopCommand = false;
		return startCommand(command,rmargs,serr);
	}


	integer ServiceApp::startCommand(ConstStrA command, const Args & args, SeqFileOutput &serr) {
		if (command == ConstStrA(startCmd)) {
			createInstanceFile();
			integer x = onMessage(command,args,serr);
			if (x) return x;
			x = initService(args,serr);
			if (x) return x;
			serr = nil;
			instance->enterDaemonMode(restartDelaySec);
			return startService();
		} else if (command == ConstStrA(startForeCmd)) {
			createInstanceFile();
			integer x = onMessage(command,args,serr);
			if (x) return x;
			x = initService(args,serr);
			if (x) return x;
			return startService();
		} else if (command == ConstStrA(restartCmd)){
			try {
				instance->open();
				postMessage(stopCmd,Args(), serr);
				instance->waitForTerminate(timeout);
			} catch (ProgInstance::NotRunningException &) {
			} catch (ProgInstance::TimeoutException &) {
				instance->terminate();
				instance->waitForTerminate(timeout);
			}
			createInstanceFile();
			integer x = onMessage(command,args,serr);
			if (x) return x;
			x = initService(args,serr);
			if (x) return x;
			serr = nil;
			instance->enterDaemonMode(restartDelaySec);
			return startService();
		} else if (command == ConstStrA(stopCmd)) {
			try {
				instance->open();
				integer res = postMessage(command,args, serr);
				if (res == 0) instance->waitForTerminate(timeout);
				return res;
			} catch (ProgInstance::TimeoutException &) {
				instance->terminate();
				return 0;
			}
		} else if (command == ConstStrA(waitCmd)) {
			natural timeout = naturalNull;
			if (!args.empty()) {
				TextParser<wchar_t> parser;
				if (parser(L" %u1 ",args[0])) timeout = parser[1];
			}
			instance->open();
			instance->waitForTerminate(timeout);
		} else {
			instance->open();
			return postMessage(command,args, serr);
		}
		return 0;
	}



	bool ServiceApp::onIdle(natural )
	{
		return false;
	}

	void ServiceApp::errorRestart() {
		instance->restartDaemon();
	}


	integer ServiceApp::onMessage(ConstStrA command, const Args & args, SeqFileOutput output)
	{
		if (command == ConstStrA(restartCmd) || command == ConstStrA(startForeCmd)) {
			return onMessage(startCmd,args,output);
		}

		if (command == ConstStrA(stopCmd)) {
			stop();
			return 0;
		}

		if (command == ConstStrA(statusCmd)) {
			PrintTextA print(output);
			print("Service is running\n");
			return 0;
		}

		if (command == ConstStrA(startCmd)) 
			return 0;

		if (command == ConstStrA("restartDaemon")) {
			instance->restartDaemon();
		}

		return integerNull;
	}




	integer ServiceApp::postMessage(ConstStrA command, const Args & args, SeqFileOutput output)
	{

		if (instance->imOwner()) return onMessage(command,args,output);

		AutoArrayStream<char, StaticAlloc<65536> > buffer;
		if (args.length() > 127) return onCommandLineError();
		buffer.write((char)(args.length()));
		buffer.copy(command.getFwIter());
		buffer.write(0);
		for(natural i = 0, cnt = args.length();i < cnt;i++){
			buffer.copy(WideToUtf8Reader<ConstStrW::Iterator>(args[i].getFwIter()));
			buffer.write(0);
		}
		AutoArray<byte,SmallAlloc<4096> > replybuff;
		replybuff.resize(instance->getReplyMaxSize());
		natural sz = instance->request(buffer.data(), buffer.length(), replybuff.data(), replybuff.length(), timeout);
		if (sz < sizeof(integer))
			throw ErrorMessageException(THISLOCATION,String("Invalid reply"));
		else {
			integer res = *reinterpret_cast<integer *>(replybuff.data());
			ConstStrA msg(reinterpret_cast<char *>(replybuff.data()+sizeof(integer)),sz - sizeof(integer));
			output.copy(msg.getFwIter());
			return res;
		}
	}

	integer ServiceApp::pumpMessages()
	{
/*		if(instance->takeOwnership() == false)
			return -1;*/

		timeout = naturalNull;
		bool go_on = true;
		while(go_on){
			try {
				natural counter = 0;
				bool request = instance->anyRequest();
				while(!request && onIdle(counter)){
					counter++;
					request = instance->anyRequest();
				}
				const void *req = instance->waitForRequest(timeout);
				if(req){
					go_on = processRequest(req);
				} else {
					go_on = !stopCommand;
				}
			}
			catch(const Exception & e){
				onWaitException(e);
			}
		}

		return 0;
	}

	class SeqOutputBuffer : public IOutputStream
	{
	public:

		virtual natural write(const void *b, natural size)
		{
			const byte *bb = reinterpret_cast<const byte*>(b);
			natural x = size;
			if(x + pos > maxsize)
				x = maxsize - pos;

			natural wr = x;
			while(x){
				buffer[pos++] = *bb++;
				x--;
			}
			return wr;
		}


		virtual bool canWrite() const
		{
			return !closed;
		}

		virtual void flush()
		{
		}


		natural length() const
		{
			return pos;
		}

		virtual void closeOutput() {
			closed = true;
		}


		SeqOutputBuffer(void *buffer, natural maxsize)
			:buffer(reinterpret_cast<byte*>(buffer)), pos(0), maxsize(maxsize)
			,closed(false)
		{
		}

	private:
		byte *buffer;
		natural pos;
		natural maxsize;
		bool closed;
	};
	bool ServiceApp::processRequest(const void *request)
	{
		const char *p = reinterpret_cast<const char*>(request);
		natural count = *p++;
		ConstStrA command(p);
		p += command.length() + 1;
		AutoArray<String,StaticAlloc<256> > params;
		AutoArray<ConstStrW,StaticAlloc<256> > wparams;
		for(natural i = 0;i < count;i++){
			ConstStrA param(p);
			p += param.length() + 1;
			params.add(String(param));
			wparams.add(ConstStrW(params[i]));
		}
		AutoArray<byte> output;
		output.resize(instance->getReplyMaxSize());
		SeqOutputBuffer fakeout(output.data() + sizeof (integer), output.length() - sizeof (integer));
		fakeout.setStaticObj();
		integer res = -1;
		bool stop = false;
		SeqFileOutput out(&fakeout);
		try {
			res = onMessage(command, wparams, out);
			stop = command == ConstStrA(stopCmd);
		}
		catch(std::exception & e){
			TextOut<SeqFileOutput> msg(out);
			msg("%1") << e.what();
			res = -1;
		}
		try {
			*reinterpret_cast<integer*>(output.data()) = res;
			instance->sendReply(output.data(), fakeout.length() + sizeof (integer));
		}
		catch(...){
		}
		return !stop;
	}

	void ServiceApp::createInstanceFile()
	{
		if(instanceFileSize == 0)
			instance->create();

		else
			instance->create(instanceFileSize);

	}
	integer ServiceApp::onCommandLineError()
	{
		throw ErrorMessageException(THISLOCATION, "Error in command line. Need two parameters. Instance file and operation name");
	}

	integer ServiceApp::onOperationUnknown(ConstStrA opname)
	{
		throw ErrorMessageException(THISLOCATION, String("Error in command line: '") + String(opname) + String("'. Operation is unknown"));
	}

	integer ServiceApp::startService()
	{
		integer res = pumpMessages();
		return res;
	}



	bool ServiceApp::stop()
	{
		stopCommand = true;
		instance->cancelWaitForRequest();
		return true;
	}


}

#endif /* LIGHTSPEED_FRAMEWORK_SERVICEAPP_TCC_ */
