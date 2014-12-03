/*
 * ProcessTest.h
 *
 *  Created on: 25.6.2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TESTS_PROCESSTEST_H_
#define LIGHTSPEED_TESTS_PROCESSTEST_H_


#include "../lightspeed/base/framework/app.h"

namespace LightSpeedTest {


using namespace LightSpeed;

class ProcessTest: public App {
public:
	virtual integer start(const Args &args);

};

} /* namespace LightSpeedTest */
#endif /* LIGHTSPEED_TESTS_PROCESSTEST_H_ */
