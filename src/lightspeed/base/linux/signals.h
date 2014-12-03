/*
 * signals.h
 *
 *  Created on: 21.3.2010
 *      Author: ondra
 */

#include <signal.h>
#include "wait.h"
#include "../memory/sharedResource.h"

#pragma once

namespace LightSpeed {


	class SignalMask {

	public:

		SignalMask(bool fill = true, bool state = false);
		void enableSignal(int signal, bool enabled);
		bool isenabled(int signal);
		void block(SignalMask *old = 0);
		void unblock(SignalMask *old = 0);
		void restore(SignalMask *old = 0);

		const sigset_t *get_sigset_t() const {return &mask;}

	protected:
		sigset_t mask;

	};


}
