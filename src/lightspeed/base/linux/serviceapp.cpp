/*
 * serviceapp.cpp
 *
 *  Created on: 22.2.2011
 *      Author: ondra
 */

#include "../framework/serviceapp.h"
#include <unistd.h>

namespace LightSpeed {

/*
void ServiceAppBase::daemonMode() {
	int err = daemon(1,0);
	if (err == -1) throw ErrNoException(THISLOCATION,errno);
}
*/


String ServiceApp::getDefaultInstanceName(const ConstStrW arg0) const
{
	natural p = arg0.find('/');
	if (p == naturalNull) p = 0;else ++p;
	return String("/var/run/lightspeedsvc-") + arg0.offset(p);
}

}
