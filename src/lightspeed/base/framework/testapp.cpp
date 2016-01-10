#include "testapp.h"
#include "../streams/memfile.h"
#include "../exceptions/stdexception.h"
#include "../containers/constStr.h"
#include "../streams/standardIO.tcc"
#include "../exceptions/errorMessageException.h"


namespace LightSpeed {

	TestApp::TestApp(const char *testName, const char *expectedOutput, TestMain testMain)
		:testName(testName), expectedOutput(expectedOutput), testMain(testMain), previousTest(0)

	{
		TestCollector &collector = Singleton<TestCollector>::getInstance();
		collector.addTest(this);
	}

	StringA getProducedOutput(MemFile<> &memfile)
	{

		ConstBin out = memfile.getBuffer();
		ConstStrA strout(reinterpret_cast<const char *>(out.data()), out.length());
		return strout;
	}


	const char * TestApp::getTestName() const
	{
		return testName;
	}


	const char * TestApp::getExpectedOutput() const
	{
		return expectedOutput;
	}

	void TestApp::runTest() const throw(FailedTest)
	{
		MemFile<> memFile;
		memFile.setStaticObj();

		try {
			SeqFileOutput out(&memFile);
			PrintTextA txt(out);
			testMain(txt);
			StringA produced = getProducedOutput(memFile);
			if (produced != expectedOutput) throw FailedTest(THISLOCATION, expectedOutput, produced);

		}
		catch (const FailedTest &) {
			throw;
		}
		catch (const Exception &e) {
			throw FailedTest(THISLOCATION, expectedOutput, getProducedOutput(memFile)) << e;
		}
		catch (const std::exception &e) {
			throw FailedTest(THISLOCATION, expectedOutput, getProducedOutput(memFile)) << StdException(THISLOCATION, e);
		}
		catch (...) {
			throw FailedTest(THISLOCATION, expectedOutput, getProducedOutput(memFile)) << UnknownException(THISLOCATION);
		}
	}

	natural TestCollector::runTests(ConstStrW testList, SeqFileOutput console) {
		return runTests(String::getUtf8(testList), console);
	}
	natural TestCollector::runTests(ConstStrA testList) {
		return runTests(testList, SeqFileOutput(ConsoleA::getStdOutput()));
	}
	natural TestCollector::runTests(ConstStrW testList) {
		return runTests(testList, SeqFileOutput(ConsoleA::getStdOutput()));
	}
	natural TestCollector::runTests(ConstStrA testList, SeqFileOutput console) {		
		natural retval = 0;
		if (testList.empty() || testList == "all") {
			TestApp *x = firstTest;
			while (x != 0) {
				if (!runSingleTest(x, console)) retval = 1;
				x = x->previousTest;
			}
		}
		else {
			TestApp *x = firstTest;
			while (x != 0) {
				ConstStrA name = x->getTestName();
				ConstStrA::SplitIterator testIter = testList.split(',');
				while (testIter.hasItems()) {
					ConstStrA curTest = testIter.getNext();
					curTest = curTest.trim(' ');
					if (curTest == name || (curTest.tail(1) == ConstStrA('*') && curTest.crop(0,1) == name.head(curTest.length()-1))) {
						if (!runSingleTest(x, console)) retval = 1;
					}
				}
				x = x->previousTest;
			}

		}
		return retval;
	}

	StringA TestCollector::listTests() const {
		AutoArrayStream<char> out;
		if (firstTest == 0) throw ErrorMessageException(THISLOCATION, "No tests available");
		TestApp *x = firstTest;
		out.blockWrite(ConstStrA(x->getTestName()));
		x = x->previousTest;
		while (x != 0) {
			out.write(',');
			out.blockWrite(ConstStrA(x->getTestName()));
			x = x->previousTest;
		}
		return out.getArray();
	}

	void TestCollector::addTest(TestApp *test) {
		test->previousTest = firstTest;
		firstTest = test;
	}

	bool TestCollector::runSingleTest(TestApp * x, SeqFileOutput console)
	{
		SeqFileOutBuff<> bufout(console);
		SeqTextOutA textOut(bufout);
		TextOut<SeqTextOutA> print(textOut);

		ConstStrA testName(x->getTestName());
		print("Running: %1 %2 ") << testName << ConstStrA("..............................................").crop(0, testName.length());
		bufout.flush();

		try {
			x->runTest();
			print("OK\n"); bufout.flush();
			return true;
		}
		catch (const TestApp::FailedTest &e) {
			print("!!FAILED!!\nDetails:\n\tExpected output: %1\n\tProduced output: %2\n\tDescription: %3\n\n")
				<< e.getExpectedOutput()
				<< e.getProducedOutput()
				<< e.getMessageWithReason();
			bufout.flush();
			return false;
		}

	}

	static void testFn(PrintTextA &print) {
		print("OK");
	}


	static TestApp test("testSystem", "OK", testFn);

	TestApp::FailedTest::FailedTest(const ProgramLocation &loc, StringA expectedOutput, StringA producedOutput)
		:Exception(loc), expectedOutput(expectedOutput), producedOutput(producedOutput)
	{

	}

}