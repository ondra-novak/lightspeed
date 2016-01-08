#pragma once
#include "../containers/constStr.h"
#include "../streams/fileio.h"

namespace LightSpeed {
#ifdef _MSC_VER
#pragma warning( disable : 4290 )
#endif

	class TestCollector;
class TestApp {
public:

	typedef void(*TestMain)(SeqFileOutput testOutput);

	TestApp(const char *testName, const char *expectedOutput, TestMain testMain);



	const char *getTestName() const;
	const char *getExpectedOutput() const;

	class FailedTest : public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL
		FailedTest(const ProgramLocation &loc, StringA expectedOutput, StringA producedOutput);


		StringA getExpectedOutput() const { return expectedOutput; }
		StringA getProducedOutput() const { return producedOutput; }
		virtual ~FailedTest() throw() {}
	protected:
		StringA expectedOutput;
		StringA producedOutput;	

		void message(ExceptionMsg &msg) const {
			msg("Test failed");
		}
	};

	void runTest() const throw(FailedTest);



protected:

	friend class TestCollector;

	const char *testName;
	const char *expectedOutput;
	TestMain testMain;
	TestApp *previousTest;

};


class TestCollector {
public:
	natural runTests(ConstStrW testList);
	natural runTests(ConstStrA testList);
	natural runTests(ConstStrW testList, SeqFileOutput console);
	natural runTests(ConstStrA testList, SeqFileOutput console);
	StringA listTests() const;
	
	void addTest(TestApp *test);
	bool runSingleTest(TestApp * x, SeqFileOutput console);
protected:

	TestApp *firstTest;
	

};



}
