/*
 * app.cpp
 *
 *  Created on: 29.9.2010
 *      Author: ondra
 */


#include "../interface.tcc"
#include "app.h"
#include "../containers/autoArray.tcc"


void LightSpeed::App::addService( TypeInfo nfo, IInterface *ifc )
{
	Services::addService(nfo,ifc);
}

void LightSpeed::App::addService( IService &ifc )
{
	Services::addService(ifc);
}
void LightSpeed::App::removeService( TypeInfo nfo )
{
	Services::removeService(nfo);
}

void LightSpeed::App::removeService( TypeInfo nfo, IInterface *ifc )
{
	Services::removeService(nfo,ifc);

}

void LightSpeed::App::removeService( IService &ifc )
{
	Services::removeService(ifc);
}
void * LightSpeed::App::proxyInterface( IInterfaceRequest &p )
{
	return Services::proxyInterface(p);
}

const void * LightSpeed::App::proxyInterface( const IInterfaceRequest &p ) const
{
	return Services::proxyInterface(p);
}

LightSpeed::IApp & LightSpeed::IApp::current()
{
	return AppBase::current();
}


LightSpeed::IApp * LightSpeed::IApp::currentPtr()
{
	return AppBase::currentPtr();

}


int LightSpeed::AppBase::main_entry( int argc, char *argv[], ConstStrW pathname)
{
	AutoArray<String> params;
	AutoArray<ConstStrW> wparams;
	params.reserve(argc);
	for (int i = 0; i < argc; i++)
		params.add(String(argv[i]));

	wparams.append(params);
	appPathname = pathname;
	return startApp(Args(wparams));
}

int LightSpeed::AppBase::main_entry( int argc, wchar_t *argv[], ConstStrW pathname)
{
	AutoArray<ConstStrW> params;
	params.reserve(argc);
	for (int i = 0; i < argc; i++)
		params.add(ConstStrW(argv[i]));		
	appPathname = pathname;
	return startApp(Args(params));
}

LightSpeed::AppBase & LightSpeed::AppBase::current()
{
	AppPtr p = getCurrentApp();
	if (p == nil)
		throw NoCurrentApplicationException(THISLOCATION);
	return *p;
}

int LightSpeed::AppBase::startApp( const Args &args )
{
#if defined _WIN32 && defined _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	try {
		return (int)start(args);
	} catch (...) {
		handleException();
	}

	do {
		try {
			return (int)restore();
		} catch (...) {
			handleException();
		}

	} while (true);
}

void LightSpeed::AppBase::registerApp()
{
	if (appPriority == integerNull) {
		prevApp = 0;
		return;
	}
	AppPtr &inst = CurAppSingleton::getInstance();
	if (inst != nil) {
		if (inst->appPriority <= appPriority) {
			prevApp = inst;
			inst = this;
		} else {
			AppPtr r1 = inst;
			AppPtr r2 = inst->prevApp;
			while (r2 != nil && r2->appPriority > appPriority) {
				r1 = r2;
				r2 = r2->prevApp;
			}
			prevApp = r2;
			r1->prevApp = this;
		}
	} else {
		inst = this;
	}
}

void LightSpeed::AppBase::unregisterApp()
{
	if (appPriority == integerNull)
		return;
	AppPtr &inst = CurAppSingleton::getInstance();
	if (inst == this) {
		inst = prevApp;
		prevApp = nil;
	} else if (inst != nil) {
		AppPtr r1 = inst;
		AppPtr r2 = inst->prevApp;
		while (r2 != nil && r2 != this) {
			r1 = r2;
			r2 = r2 ->prevApp;
		}
		if (r2 != nil) 
			r1->prevApp = prevApp;
		prevApp = nil;
	}
}

void LightSpeed::AppBase::handleException()
{
	try {
		try {
			throw;
#if defined _DEBUG || defined DEBUG
		} catch (const Exception &e) {
			exceptionDebugLog(e);
		} catch (const std::exception &e) {
			exceptionDebugLog(StdException(THISLOCATION,e));
		} catch (...) {
			exceptionDebugLog(UnknownException(THISLOCATION));
#else
		} catch (const Exception &e) {
			onException(e);
		} catch (const std::exception &e) {
			onException(StdException(THISLOCATION,e));
		} catch (...) {
			onException(UnknownException(THISLOCATION));
#endif
		}
	} catch (const Exception &e) {
		IApp::exceptionDebug(e,this);
	} catch (const std::exception &e) {
		IApp::exceptionDebug(StdException(THISLOCATION,e),this);
	} catch (...) {
		IApp::exceptionDebug(UnknownException(THISLOCATION),this);
	}
}

LightSpeed::AppBase::AppPtr LightSpeed::AppBase::getCurrentApp()
{
	AppPtr &inst = CurAppSingleton::getInstance();
	if (inst != nil && inst->appPriority < 0) inst = nil;
	return inst;
}

LightSpeed::AppBase * LightSpeed::AppBase::currentPtr()
{	
return getCurrentApp();
}
