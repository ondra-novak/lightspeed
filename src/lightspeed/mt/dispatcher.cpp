#include "dispatcher.h"
#include "../base/actions/idispatcher.tcc"
#include "../base/containers/queue.tcc"

namespace LightSpeed {



	Dispatcher::Dispatcher() :running(Gate::stateOpen)
	{

	}

	Dispatcher::~Dispatcher()
	{
		join();
	}

	void Dispatcher::dispatch(AbstractAction *action)
	{
		Synchronized<FastLock> _(lock);
		bool firstMsg = queue.empty();
		queue.push(Constructor1<PAction, AbstractAction *>(action));
		if (firstMsg) onNewMessage();
	}

	IRuntimeAlloc & Dispatcher::getActionAllocator()
	{
		return alloc;
	}

	void Dispatcher::promiseRegistered(PPromiseControl )
	{
		
	}

	void Dispatcher::promiseResolved(PPromiseControl )
	{
		
	}

	void Dispatcher::run()
	{
		Synchronized<FastLock> _(lock);
		if (!running.isOpened()) throw ThreadBusyException(THISLOCATION);
		currentThread = getCurrentThread();
		running.close();
		needstop = false;
		try {
			natural idleCount = 0;
			while (!needstop) {
				AbstractAction *aa = getNextAction();
				SyncReleased<FastLock> _(lock);
				if (aa == 0) {					
					Thread::sleep(onIdle(idleCount));
					idleCount++;
					
				}
				else {
					idleCount = 0;
					aa->run();
					delete aa;
				}
			}
			finishRun();
		}
		catch (...) {
			finishRun();
			throw;
		}
	}

	void Dispatcher::quit()
	{
		needstop = true;
		onNewMessage();
	}

	void Dispatcher::join()
	{
		running.wait(nil);
	}

	void Dispatcher::onNewMessage() throw()
	{
		if (currentThread) currentThread->wakeUp(0);
	}

	LightSpeed::Timeout Dispatcher::onIdle(natural)
	{
		return nil;
	}

	void Dispatcher::finishRun() throw()
	{
		rejectQueue();
		running.open();
		currentThread = nil;

	}

	IDispatcher::AbstractAction * Dispatcher::getNextAction()
	{
		if (queue.empty()) return 0;
		AllocPointer<AbstractAction> &aa = queue.top();
		return aa.detach();
	}

	void Dispatcher::rejectQueue()
	{
		AllocPointer<AbstractAction> aa(getNextAction());
		while (aa != nil) {
			aa->reject();
			aa = getNextAction();
		}
	}

}
