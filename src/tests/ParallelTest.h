/*
 * ParallelTest.h
 *
 *  Created on: 14.5.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEST_PARALLELTEST_H_
#define LIGHTSPEED_TEST_PARALLELTEST_H_
#include "../lightspeed/base/framework/app.h"

namespace LightSpeedTest {

using namespace LightSpeed;

class ParallelTest: public App {
public:

	virtual integer start(const Args &args);


protected:

	void worker(natural id);
};

} /* namespace LightSpeedTest */
#endif /* LIGHTSPEED_TEST_PARALLELTEST_H_ */
