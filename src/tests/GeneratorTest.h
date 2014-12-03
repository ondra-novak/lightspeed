/*
 * GeneratorTest.h
 *
 *  Created on: 23.5.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEST_GENERATORTEST_H_
#define LIGHTSPEED_TEST_GENERATORTEST_H_

#include "../lightspeed/base/framework/app.h"

namespace LightSpeedTest {



class GeneratorTest: public LightSpeed::App {
public:

	virtual LightSpeed::integer start(const Args &args);


};

} /* namespace LightSpeedTest */
#endif /* LIGHTSPEED_TEST_GENERATORTEST_H_ */
