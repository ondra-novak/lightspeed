/*
 * TreeTest.cpp
 *
 *  Created on: 1. 12. 2014
 *      Author: ondra
 */

#include "TreeTest.h"
#include "../lightspeed/base/streams/standardIO.tcc"


namespace LightSpeedTest {

integer LightSpeedTest::TreeTest::start(const Args&) {

	TestSet tsts;
	ConsoleA console;
	srand(12345);

	for (natural i = 0; i < 1000; i++) {
		tsts.insert(i);
	}


	for (natural i = 0; i < 100000; i++) {
		bool found = true;
		natural r = rand() % 1000;
		TestSet::Iterator x = tsts.seek(r,&found);
		if (found) {
			console.print("Deleting: %1 - ") << x.peek();
			tsts.erase(x);
			if (x.hasItems()) {
				console.print("next is: %1 -") << x.peek();
				tsts.erase(x);
			}
			if (x.hasItems()) {
				natural p = x.getNext();
				console.print("%1-%2\n") << r << p;
			}
			console.flush();
		} else {
			tsts.insert(r);
		}
	}
	return 0;

}


} /* namespace LightSpeedTest */

