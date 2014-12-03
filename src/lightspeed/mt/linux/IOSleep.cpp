/*
 * IOSleep.cpp
 *
 *  Created on: 20.12.2013
 *      Author: ondra
 */

#include "IOSleep.h"
#include <errno.h>
#include "../../base/exceptions/systemException.h"
#include "../../base/containers/autoArray.tcc"
#include <poll.h>
#include "../../base/memory/smallAlloc.h"
#include <signal.h>
#include "../../base/streams/netio_ifc.h"
#include <unistd.h>
#include <fcntl.h>

namespace LightSpeed {

int pipeCloseOnExec(int *fds);

IOSleep::IOSleep() {

	int fds[2];
	if (pipeCloseOnExec(fds)) {
		int e = errno;
		throw ErrNoException(THISLOCATION,e);
	}
	fdnotify = fds[0];
	fdcancel = fds[1];
	int flags = fcntl(fdcancel, F_GETFL, 0);
	fcntl(fdcancel, F_SETFL, flags | O_NONBLOCK);

}

IOSleep::SleepResult IOSleep::sleep(INetworkResource *nres, Timeout tm) {

	if (nres == 0) throwNullPointerException(THISLOCATION);


	AutoArray<struct pollfd, SmallAlloc<100> > pfds;

	struct pollfd n;
	n.fd = fdnotify;
	n.events = POLLIN;
	n.revents = 0;

	pfds.add(n);

	int fd = nres->getIfc<INetworkSocket>().getSocket(pfds.length()-1);
	while (fd != -1) {
		struct pollfd x;
		x.fd = fd;
		x.events = POLLIN;
		x.revents = 0;
		pfds.add(x);
		fd = nres->getIfc<INetworkSocket>().getSocket(pfds.length()-1);
	}

	natural tmms = tm.getRemain().msecs();
	int res = poll(pfds.data(),pfds.length(),tmms);
	if (res == -1) {
		int e = errno;
		throw ErrNoException(THISLOCATION,e);
	}
	if (res == 0) {
		return timeout;
	}

	if (pfds[0].revents != 0) {
		while (pfds[0].revents != 0) {
			char b;
			read(fdnotify,&b,1);
			pfds(0).revents = 0;
			poll(pfds.data(),1,0);
		}
		return waken;
	} else {
		return data;
	}

}

IOSleep::~IOSleep() {
}

void IOSleep::wakeUp(natural) throw() {
	char b =1;
	write(fdcancel,&b,1);

}

} /* namespace LightSpeed */
