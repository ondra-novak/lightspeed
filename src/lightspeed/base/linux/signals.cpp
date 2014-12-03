/*
 * signals.cpp
 *
 *  Created on: 21.3.2010
 *      Author: ondra
 */

#include "signals.h"
#include "../exceptions/systemException.h"
#include "../streams/netio_ifc.h"
#include <unistd.h>

LightSpeed::SignalMask::SignalMask(bool fill, bool state)
{
	int r = 0;
	if (fill) {
		if (state)
			r = sigfillset(&mask);
		else
			r = sigemptyset(&mask);
	}

	if (r == -1) throw ErrNoException(THISLOCATION,errno);
}



bool LightSpeed::SignalMask::isenabled(int signal)
{
	int r = sigismember(&mask,signal);

	if (r == -1) throw ErrNoException(THISLOCATION,errno);
	return r != 0;
}



void LightSpeed::SignalMask::block(SignalMask *old) {
	if (sigprocmask(SIG_BLOCK, &mask, old != 0 ? &old->mask : 0) == -1)
		 throw ErrNoException(THISLOCATION,errno);
}

void LightSpeed::SignalMask::unblock(SignalMask *old) {
	if (sigprocmask(SIG_UNBLOCK, &mask, old? &old->mask: 0) == -1)
		 throw ErrNoException(THISLOCATION,errno);
}

void LightSpeed::SignalMask::restore(SignalMask *old) {
	if (sigprocmask(SIG_SETMASK, &mask, old? &old->mask: 0) == -1)
		 throw ErrNoException(THISLOCATION,errno);
}



void LightSpeed::SignalMask::enableSignal(int signal, bool enabled)
{
	int r;
	if (enabled) r = sigaddset(&mask,signal);
	else r = sigdelset(&mask,signal);
	if (r == -1) throw ErrNoException(THISLOCATION,errno);

}



