#include "../lightspeed/base/text/textstream.tcc"
#include "../lightspeed/base/text/toString.tcc"
#include "../lightspeed/base/framework/testapp.h"
#include "../lightspeed/base/actions/promise.tcc"
#include <utility>
#include "../lightspeed/mt/thread.h"
#include "../lightspeed/base/actions/message.h"


namespace LightSpeedTest {

using namespace LightSpeed;




class TestObserver : public Future<void>::IObserver {
public:
	TestObserver(PrintTextA &prn, int number)
		:prn(prn), number(number) {}
	virtual void resolve(const PException &) throw() 	{
		prn("%1") << -(integer)number;
	}
	virtual void resolve() throw() {
		prn("%1") << number;
	}
	PrintTextA &prn; int number;
};


static void observerTest(PrintTextA &print) {

	TestObserver a(print,1), b(print,2), c(print,3), d(print,4), e(print,5);
	Future<void> f;
	f.addObserver(&a); // 1
	f.addObserver(&b); // 12
	f.addObserver(&c); // 123
	f.addObserver(&d); // 1234
	f.removeObserver(&c); // 124
	f.removeObserver(&a); // 24
	f.addObserver(&a); // 241
	f.addObserver(&c); // 2413
	f.addObserver(&e); // 24135
	f.removeObserver(&b); // 4135
	f.getPromise().resolve();
	
}

static void singleThreadTest(PrintTextA &print)
{
	Future<const char *> f;

	Promise<const char *> p(f);
	p.resolve("ST");
	const char *result = f.wait(Timeout(0));
	print("%1") << result;
}

static void leftPromise(PrintTextA &print)  {
	Future<void> f;
	TestObserver a(print, 1), b(print, 2), c(print, 3), d(print, 4), e(print, 5);
	f.addObserver(&a); // 1
	f.addObserver(&b); // 12
	f.addObserver(&c); // 123
	f.addObserver(&d); // 1234
	Promise<void> p(f);
	print("%1") << p.getControlInterface()->getState();
}

class ThreadResolve {
public:
	Thread &t;
	ThreadResolve(Thread &t) :t(t) {}
	Future<int> operator()(int value) const {
		Future<int> res;
		t.start(ThreadFunction::create(&ThreadResolve::runResolve, std::make_pair(res.getPromise(), value)));
		return res;
	}
	static void runResolve(std::pair<Promise<int>, int> arg) {
		Thread::sleep(100);
		arg.first.resolve(arg.second + 42);
	}
};

static void produceResult(Promise<int> result) {
	Thread::sleep(100);
	result = 23;
}

static void testThenPromise(PrintTextA &print) {
	TimeStamp start = TimeStamp::now();
	Future<int> f1;
	Thread t,t2;
	ThreadResolve resolveFn(t);
	t2.start(ThreadFunction::create(&produceResult, f1.getPromise()));
	int res = (f1 >> resolveFn).getValue();
	TimeStamp end = TimeStamp::now();
	natural dist = (end - start).getMilis();
	print("%1-%2") << res << (dist>100 && dist < 300);

}

static PException testReturnException(int value) {
	return new ErrorMessageException(THISLOCATION, ToString<integer>(value));
}

static int testDoAlternativeResult(const PException &e) {
	ErrorMessageException *eme = dynamic_cast<ErrorMessageException *>(e.get());
	if (NULL != eme)
	{
		return 42;
	}
	else {
		e->throwAgain(THISLOCATION);
		return 0;
	}
}

static void testAlternativeResult(PrintTextA &print) {
	Future<int> f;
	Thread t;
	t.start(ThreadFunction::create(&produceResult, f.getPromise()));
	int res = f.then(&testReturnException).onException(&testDoAlternativeResult).getValue();
	print("%1") << res;
}


static void chainingTestC11(PrintTextA &print) {
	Future<int> f;
	Promise<int> p = f.getPromise();
	double result;
	f >> [](const int &v){return v * 3;}
		 >> [](natural v){return v/5.0;}
		 >> [&result](double v) {result = v;};

	p.resolve(42);
	print("%1") << result;
}


TestApp promiseTest1("promises.observers", "4135", &observerTest);
TestApp promiseTest2("promises.singleThread", "ST", &singleThreadTest);
TestApp promiseTest3("promises.leftPromise", "0-1-2-3-4", &leftPromise);
TestApp promiseTest4("promises.thenPromise", "65-1", &testThenPromise);
TestApp promiseTest5("promises.alternativeResult", "42", &testAlternativeResult);
defineTest chaining_test("promises.chaining","25.200000", &chainingTestC11);


}


