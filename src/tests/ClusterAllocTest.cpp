#include "ClusterAllocTest.h"
#include "../lightspeed/base/memory/dynobject.h"
#include "../lightspeed/base/streams/standardIO.tcc"
#include "../lightspeed/base/text/textOut.tcc"
#include "../lightspeed/base/memory/clusterAlloc.h"
#include "../lightspeed/base/streams/random.h"
#include "../lightspeed/base/countof.h"
#include "../lightspeed/base/timestamp.h"
#include "../lightspeed/base/exceptions/errorMessageException.h"

namespace LightSpeedTest {

ClusterAllocTest test;


class TestClass: public DynObject, public RefCntObj {
public:
	TestClass(natural i) {
		if (this->i == 0xFEEEFEEE || this->i == 0xFDFDFDFD)
			throw ErrorMessageException(THISLOCATION,"Out of heap");
		this->i = i;

	}

	natural i;
};

RefCntPtr<TestClass>  testArray[1000];

class ReportAlloc: public IRuntimeAlloc {
public:
	LIGHTSPEED_CLONEABLECLASS;
	ReportAlloc(IRuntimeAlloc &next, SeqTextOutW &out):next(next),out(out) {}
	virtual void *alloc(natural objSize) {
		TextOut<SeqTextOutW> txt(out);
		txt("************ MemAlloc: %1\n") << objSize;
		return next.alloc(objSize);
	}

	virtual void dealloc(void *ptr, natural objSize) {
		TextOut<SeqTextOutW> txt(out);
		txt("************ MemDeallocc: %1\n") << objSize;
		return next.dealloc(ptr,objSize);
	}
	virtual void *alloc(natural objSize, IRuntimeAlloc * &owner) {
		TextOut<SeqTextOutW> txt(out);
		txt("************ MemAlloc: %1\n") << objSize;
		return next.alloc(objSize,owner);

	}




protected:
	IRuntimeAlloc &next;
	SeqTextOutW &out;

};


LightSpeed::integer ClusterAllocTest::start( const Args & )
{
	ConsoleW console;
	TextOut<SeqTextOutW> conerr(console.err), conout(console.out);
	ReportAlloc rpalloc(StdAlloc::getInstance(),console.out);
	ClusterAlloc alloc(rpalloc);

	TimeStamp tm1 = TimeStamp::now();
	conout("Allocating \n");
	Random<natural> rnd(0,countof(testArray));
	for (natural i = 0; i < 10000000; i++) {
		natural k = rnd.getNext();
		if (testArray[k] == nil) {
//			conout("Allocating %1 \n") << i;
			testArray[k] = new(alloc) TestClass(i);
//			testArray[k] = new TestClass(i);
		} else {
//			conout("Deallocating %1 \n") << testArray[k]->i;
			testArray[k] = nil;
		}
	}

	conout("Deallocating all \n");
	for (natural i = 0; i < 1000; i++) {
		testArray[i] = nil;
	}
	conout("Exit \n");
	TimeStamp tm2 = TimeStamp::now();
	TimeStamp diff = tm2 - tm1;
	conout("Time: %1.%2\n") << (diff.getTime()/1000) << (diff.getTime() % 1000);


	return 0;
}

}
