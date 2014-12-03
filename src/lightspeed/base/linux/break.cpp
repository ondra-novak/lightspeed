/*
 * break.cpp
 *
 *  Created on: 3.7.2010
 *      Author: ondra
 */

#include "../debug/break.h"


namespace LightSpeed {

	void debugBreak() {
		asm("int $3");
	}


}
