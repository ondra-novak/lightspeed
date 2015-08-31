/*
 * ReadLineTest.h
 *
 *  Created on: 31. 8. 2015
 *      Author: ondra
 */

#ifndef TESTS_READLINETEST_H_
#define TESTS_READLINETEST_H_
#include "../lightspeed/base/framework/app.h"


namespace LightSpeedTest {

using namespace LightSpeed;


class ReadLineTest: public App {
public:

	ReadLineTest() {}
	ReadLineTest(integer priority):App(priority) {}
	virtual integer start(const Args &args);
};

} /* namespace LightSpeedTest */

#endif /* TESTS_READLINETEST_H_ */
