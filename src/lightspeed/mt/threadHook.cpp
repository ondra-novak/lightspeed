/*
 * threadHook.cpp
 *
 *  Created on: 6. 2. 2015
 *      Author: ondra
 */

#include "threadHook.h"

#include "../base/sync/synchronize.h"
#include "microlock.h"
namespace LightSpeed {

static MicroLock lock;
static AbstractThreadHook *chain = 0;


AbstractThreadHook::AbstractThreadHook():installed(false),next(0) {
}

bool AbstractThreadHook::isInstalled() const {
	return installed;
}

void AbstractThreadHook::install() {
	if (installed) return;
	Synchronized<MicroLock> _(lock);
	if (installed) return;
	next = chain;
	chain = this;
	installed = true;
}

void AbstractThreadHook::uninstall() {
	if (!installed) return;
	Synchronized<MicroLock> _(lock);
	if (!installed || chain == 0) return;
	if (chain == this) {
		chain = next;
		next = 0;
	} else {
		AbstractThreadHook *p = chain;
		while (p->next && p->next != this) {
			p = p->next;
		}
		p->next = next;
		next = 0;
	}
	installed = false;

}


static void callInitHooks(AbstractThreadHook *from, Thread &thread) {
	if (from) {
		try {
			callInitHooks(from->getNextHook(), thread);
		} catch (...) {
			from->onThreadException(thread);
			throw;

		}
		from->onThreadInit(thread);
	}
}

void AbstractThreadHook::callOnThreadInitHooks(Thread& thread) {
	if (chain) {
		Synchronized<MicroLock> _(lock);
		callInitHooks(chain,thread);
	}
}

void AbstractThreadHook::callOnThreadExitHooks(Thread& thread) {
	if (chain) {
		Synchronized<MicroLock> _(lock);
		AbstractThreadHook *pos = chain;
		try {
			while (pos) {
				AbstractThreadHook *x = pos;
				pos = pos->next;
				x->onThreadExit(thread);
			}
		} catch (...) {
			while (pos) {
				pos->onThreadException(thread);
				pos = pos->next;
			}
			throw;
		}
	}
}

void AbstractThreadHook::callOnThreadExceptionHooks(Thread& thread) throw() {
	if (chain) {
		Synchronized<MicroLock> _(lock);
		AbstractThreadHook *pos = chain;
		while (pos) {
			pos->onThreadException(thread);
			pos = pos->next;
		}
	}
}

AbstractThreadHook::~AbstractThreadHook() {
	if (installed) uninstall();
}


}
