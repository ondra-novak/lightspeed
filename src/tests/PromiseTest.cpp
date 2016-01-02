#include "../lightspeed/mt/thread.h"
#include "../lightspeed/base/framework/app.h"
#include "../lightspeed/base/actions/message.h"
#include "../lightspeed/base/debug/dbglog.h"
#include <utility>

#include "../lightspeed/base/actions/promise.tcc"
#include "../lightspeed/base/exceptions/exception.h"
#include "../lightspeed/base/exceptions/errorMessageException.h"


namespace LightSpeedTest {

using namespace LightSpeed;


class PromiseTest: public App {
public:

	virtual integer start( const Args &args );


	void test_thenCall(int value);
	int test_thenStd(int value);
	Future<int> test_thenPromise(int value);
	void test_catchCall(const PException &e);
	const PException & test_catchStd(const PException &e);
	Future<int> test_catchPromise(const PException &e);

	Thread th;
	Thread th2;

	typedef Message<void,int> Action;
	typedef Message<int,int> IntAction;
	typedef Message<Future<int>,int> PromiseAction;

	typedef Message<void,const PException &> ExceptionAction;
	typedef Message<const PException &,const PException &> ExceptionExceptionAction;
	typedef Message<Future<int>,const PException &> PromiseExceptionAction;

	Future<int> callTestAsync();
	Future<int> callRejectAsync();
};


static void testThread(Promise<int> resolution) {

	LS_LOGOBJ(lg);
	lg.progress("Async task started");
	Thread::deepSleep(5000);
	lg.progress("Async task finished");
	resolution.resolve(Constructor1<int,int>(42));

}

static void rejectThread(Promise<int> resolution) {

	LS_LOGOBJ(lg);
	lg.progress("Rejecting");
	resolution.reject(ErrorMessageException(THISLOCATION,"rejected"));

}


Future<int> PromiseTest::callTestAsync() {

	Future<int> p;
	th.start(ThreadFunction::create(&testThread,p.getPromise()));
	return p;


}

Future<int> PromiseTest::callRejectAsync() {

	Future<int> p;
	th.start(ThreadFunction::create(&rejectThread, p.getPromise()));
	return p;


}



void PromiseTest::test_thenCall( int value )
{
	LS_LOG.progress("Called thenCall with value: %1") << value;
}

int PromiseTest::test_thenStd( int value )
{
	LS_LOG.progress("Called thenStr with value: %1") << value;
	return value*value;
}

static void asyncCall(std::pair<int, Promise<int> > arg) {
	LS_LOG.progress("async call start");
	Thread::deepSleep(2500);
	LS_LOG.progress("async call finished");
	arg.second.resolve(arg.first + 200);

}

Future<int> PromiseTest::test_thenPromise( int value )
{
	LS_LOG.progress("Called thenPromise with value: %1") << value;
	Future<int> res;

	th2.start(ThreadFunction::create(&asyncCall,std::make_pair(value,res.getPromise())));
	return res;

}

void PromiseTest::test_catchCall( const PException &e )
{
	LS_LOG.error("Promise rejected %1") << e->what();
}

const PException &PromiseTest::test_catchStd( const PException &e )
{
	LS_LOG.error("Promise rejected %1") << e->what();
	return e;
}

Future<int> PromiseTest::test_catchPromise( const PException &e )
{
	LS_LOG.error("Promise rejected %1, supplying alternative result") << e->what();
	Future<int> res;
	//resolve promise now!
	res.getPromise().resolve(0);
	return res;
	
}

static PromiseTest theApp;

void promiseTestLink() {
	theApp.start(PromiseTest::Args());
}


integer PromiseTest::start( const Args & )
{
	LS_LOG.progress("Testing promise") ;

	Future<int> res = callTestAsync()
		.thenCall(Action::create(this,&PromiseTest::test_thenCall))
		.then(IntAction::create(this, &PromiseTest::test_thenStd))
		.then(PromiseAction::create(this, &PromiseTest::test_thenPromise));


	int val = res.wait();
	LS_LOG.progress("Testing finished: %1") << val;

	th.join();
	th2.join();


	Future<int> res2 = callRejectAsync()
		.thenCall(Action::create(this,&PromiseTest::test_thenCall))
		.then(IntAction::create(this, &PromiseTest::test_thenStd))
		.onExceptionCall(ExceptionAction(this,&PromiseTest::test_catchCall))
		.onException(ExceptionExceptionAction(this,&PromiseTest::test_catchStd))
		.then(PromiseAction::create(this, &PromiseTest::test_thenPromise),
				PromiseExceptionAction(this,&PromiseTest::test_catchPromise));

	int val2 = res2.wait();
	LS_LOG.progress("Testing finished: %1") << val2;

	th.join();
	th2.join();

	Future<int> res3 = callRejectAsync()
		.thenCall(Action::create(this,&PromiseTest::test_thenCall))
		.then(IntAction::create(this, &PromiseTest::test_thenStd))
		.then(PromiseAction::create(this, &PromiseTest::test_thenPromise))
		.onExceptionCall(ExceptionAction(this,&PromiseTest::test_catchCall))
		.onException(ExceptionExceptionAction(this,&PromiseTest::test_catchStd));

	int val3 = res3.wait();
	LS_LOG.progress("Testing finished: %1") << val3;

	return 0;
}

}


