#include "msgthread.h"
#include "exceptions/threadException.h"


namespace LightSpeed {

MsgThreadBase::MsgThreadBase():manStart(false)
{

}

MsgThread::MsgThread() {}
MsgThread::MsgThread( IRuntimeAlloc &rtAlloc )
	:MsgQueue(rtAlloc)
{

}

void MsgThreadBase::notify()
{
	if (!manStart) autoStart();
	Thread::wakeUp(0);
}

void MsgThreadBase::autoStart()
{
	if (startLock.tryLock()) {
		if (!Thread::isRunning()) {
			try {
				Thread::start(ThreadFunction::create(this,&MsgThreadBase::worker));
			} catch (...) {
				manStart = true;
				startLock.unlock();
				throw;
			}
		}
		startLock.unlock();
		manStart = true;
	}
}

void MsgThreadBase::setManualStart( bool manstart )
{
	manStart = manstart;
}

MsgThreadBase::~MsgThreadBase()
{
	Thread::stop();
}
void MsgThread::worker()
{
	while (!Thread::canFinish()) {
		while (MsgQueue::pumpMessage() && !Thread::canFinish());
		if (!Thread::canFinish()) {
			natural cntr = 0;
			natural w = onIdle(cntr++);
			while (MsgQueue::empty() && Thread::sleep(w)) {
				w = onIdle(cntr++);
			}
		}
	}

}

void MsgThread::notify()
{
	MsgThreadBase::notify();
}

natural MsgThread::onIdle(natural ) {
	return naturalNull;
}

SchedulerThread::SchedulerThread()
{

}

void SchedulerThread::notify()
{
	MsgThreadBase::notify();
}

void SchedulerThread::worker()
{
	while (!Thread::canFinish()) {
		natural wt = Scheduler::getWaitTime();
		Thread::sleep(Timeout(wt));
		while (Scheduler::pumpMessage()) {}
	}
}

natural SchedulerThread::getSystemTime() const
{
	return SysTime::now().msec();
}
natural SchedulerThread::getCurTime() const {return getSystemTime();}

SchedulerThread::SchedulerThread( IRuntimeAlloc &rtAlloc ):
	Scheduler(rtAlloc)
{

}

MsgThread& MsgThread::current() {
	MsgThread *x = currentPtr();
	if (x == 0) throw NoCurrentThreadException(THISLOCATION,ThreadId::current());
	return *x;
}

MsgThread* MsgThread::currentPtr() {
	Thread *x = Thread::currentPtr();
	return dynamic_cast<MsgThread *>(x);
}

SchedulerThread& SchedulerThread::current() {
	SchedulerThread *x = currentPtr();
	if (x == 0) throw NoCurrentThreadException(THISLOCATION,ThreadId::current());
	return *x;
}

SchedulerThread* SchedulerThread::currentPtr() {
	Thread *x = Thread::currentPtr();
	return dynamic_cast<SchedulerThread *>(x);
}
}
