#include "notifier.h"
#include "thread.h"


namespace LightSpeed {

	Notifier::Notifier():reason(0),signaled(0),forward(&Thread::current()) {}


	bool Notifier::wait(const Timeout &tm, bool wakeOnExit) {
		bool tmevent = false;
		if (wakeOnExit) {
			while (!readAcquire(&signaled) && !Thread::canFinish() && !tmevent) {
				 tmevent = Thread::sleep(tm);
			}
		} else {
			while (!readAcquire(&signaled) && !tmevent ) {
				tmevent = Thread::sleep(tm);
			}
		}
		return signaled != 0;
	}
}
