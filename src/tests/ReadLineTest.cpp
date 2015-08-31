/*
 * ReadLineTest.cpp
 *
 *  Created on: 31. 8. 2015
 *      Author: ondra
 */

#include "ReadLineTest.h"

#include "../lightspeed/base/streams/standardIO.h"
namespace LightSpeedTest {

static const char *example="This is example multiline file\r\n"
		"There is new line\r\n"
		"There is an other line\r\n"
		"Next line is empty\r\n"
		"\r\n"
		"Next two lines are empty\r\n"
		"\r\n"
		"\r\n"
		"Next line contains single character\r\n"
		".\r\n"
		"This line ends by incomplete sequence \n"
		"This line ends by strange sequence \r\r\n"
		"This is mac style ending \r"
		"There is end of text with new line\r\n";

static const char *example2="This is example multiline file2|"
		"There is next line|"
		"Next line is empty|"
		"|"
		"Text ends without new line";

static const char *example3="This is example multiline file212341234"
		"There is next line12341234"
		"Next line is empty12341234"
		"12341234"
		"Where is the end of line? 123412312345433321213212341234"
		"Last line";

integer LightSpeedTest::ReadLineTest::start(const Args&) {

	ConsoleA console;
	console.print("\nTest1:\n");
	{
		ConstStrA text(example);
		TextLineReader<ConstStrA::Iterator> iter(text.getFwIter(),ConstStrA("\r\n"));

		while(iter.hasItems()) {
			ConstStrA line = iter.getNext();
			console.print(">> %1 <<\n") << line;
		}
	}
	console.print("\nTest2:\n");
	{
		ConstStrA text(example2);
		TextLineReader<ConstStrA::Iterator> iter(text.getFwIter(),ConstStrA("|"));

		while(iter.hasItems()) {
			ConstStrA line = iter.getNext();
			console.print(">> %1 <<\n") << line;
		}
	}
	console.print("\nTest3:\n");
	{
		ConstStrA text(example3);
		TextLineReader<ConstStrA::Iterator> iter(text.getFwIter(),ConstStrA("12341234"));

		while(iter.hasItems()) {
			ConstStrA line = iter.getNext();
			console.print(">> %1 <<\n") << line;
		}
	}
	return 0;

}


} /* namespace LightSpeedTest */

/*RESULT

Test1:
>> This is example multiline file <<
>> There is new line <<
>> There is an other line <<
>> Next line is empty <<
>>  <<
>> Next two lines are empty <<
>>  <<
>>  <<
>> Next line contains single character <<
>> . <<
>> This line ends by incomplete sequence
This line ends by strange sequence
 <<
>> This is mac style ending
There is end of text with new line
 <<

Test2:
>> This is example multiline file2 <<
>> There is next line <<
>> Next line is empty <<
>>  <<
>> Text ends without new line <<

Test3:
>> This is example multiline file2 <<
>> There is next line <<
>> Next line is empty <<
>>  <<
>> Where is the end of line? 1234123123454333212132 <<
>> Last line <<


 */
