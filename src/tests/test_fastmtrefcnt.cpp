/*
 * test_fastmtrefcnt.cpp
 *
 *  Created on: 11. 1. 2016
 *      Author: ondra
 */

#include "../lightspeed/base/framework/testapp.h"
#include "../lightspeed/base/memory/fastMtRefCnt.h"
#include "../lightspeed/mt/thread.h"
#include "../lightspeed/base/text/textstream.tcc"

namespace LightSpeedTest {

using namespace LightSpeed;


static void basicIncDec(PrintTextA &out) {

	class TestObj: public FastMTRefCntObj {
	public:
		TestObj(PrintTextA &print):print(print) {}
		~TestObj() {print("done");}
		PrintTextA &print;
	};


	TestObj *x = new TestObj(out);
	for (natural i = 0; i < 200; i++) x->addRef();
	for (natural i = 0; i < 200; i++) x->releaseRef();

	//flush pointer table to finish test
	FastMTRefCntObj::commitAllRefs();
}

static void basicIncDecLastCommit(PrintTextA &print) {

	class TestObj: public FastMTRefCntObj {
	public:
		TestObj(PrintTextA &print):print(print) {}
		~TestObj() {print("done");}
		PrintTextA &print;
	};


	TestObj *x = new TestObj(print);
	for (natural i = 0; i < 200; i++) x->addRef();
	for (natural i = 0; i < 199; i++) x->releaseRef();
	x->commitRef();
	//flush pointer table to finish test
	x->releaseRef();
	print("<<<");
	FastMTRefCntObj::commitAllRefs();
}

static void threadFn(FastMTRefCntObj *x) {
	for (natural i = 0; i < 200; i++) x->addRef();
	for (natural i = 0; i < 200; i++) x->releaseRef();
}

static void otherThreadTest(PrintTextA &print) {

	class TestObj: public FastMTRefCntObj {
	public:
		TestObj(PrintTextA &print):print(print) {}
		~TestObj() {print("done");}
		PrintTextA &print;
	};

	Thread thr;
	TestObj *x = new TestObj(print);
	FastMTRefCntObj *bx = x;
	thr.start(ThreadFunction::create(&threadFn,bx));
	thr.join();
}




defineTest test_basicIncDec("fastMTRefCntObj.basicIncDec","done",&basicIncDec);
defineTest test_basicIncDecLastCommit("fastMTRefCntObj.basicIncDecLastCommit","done<<<",&basicIncDecLastCommit);
defineTest test_threaded("fastMTRefCntObj.threaded","done",&otherThreadTest);

}


