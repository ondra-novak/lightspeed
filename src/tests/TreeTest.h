/*
 * TreeTest.h
 *
 *  Created on: 1. 12. 2014
 *      Author: ondra
 */

#ifndef LIGHTSPEEDTEST_TREETEST_H_
#define LIGHTSPEEDTEST_TREETEST_H_


#include "../lightspeed/base/framework/app.h"
#include "../lightspeed/base/containers/set.h"

namespace LightSpeedTest {


using namespace LightSpeed;

class TreeTest: public LightSpeed::App {
public:
	TreeTest() {}

	virtual integer start(const Args &args);

	typedef Set<natural> TestSet;

};

} /* namespace LightSpeedTest */

#endif /* LIGHTSPEEDTEST_TREETEST_H_ */
