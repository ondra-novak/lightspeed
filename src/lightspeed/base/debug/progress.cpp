#include "progress.h"
#include "../sync/threadVar.h"

namespace LightSpeed{

ThreadVar<ProgressMonitor> globalProgressMonitor;



	void Progress::attachToMonitor()
	{		
		ProgressMonitor *mon = globalProgressMonitor[ITLSTable::getInstance()];
		if (mon) {
			mon->lock();
			this->callback = !mon->asyncEnabled();
			this->monitor = mon;
			this->next = monitor->head;
			monitor->head = this;
			mon->unlock();
		} else {
			this->callback = false;
			this->monitor = 0;
			this->next = 0;
		}
	}

	void Progress::detachMonitor()
	{
		if (monitor) {
			monitor->lock();
			monitor->head = next;
			monitor->unlock();
		}
	}

	void Progress::onChange()
	{
		if (monitor)
			monitor->onChange(this);
	}

	void Progress::onChangeState()
	{
		if (monitor)
			monitor->onChangeState(this);
	}

	ProgressMonitor::ProgressMonitor()
	{
		globalProgressMonitor.set(ITLSTable::getInstance(),this);
	}

	ProgressMonitor::~ProgressMonitor()
	{
		globalProgressMonitor.unset(ITLSTable::getInstance());
	}

	float ProgressMonitor::getPos( natural level /*= 0*/ ) const
	{
		lock();
		float res;
		if (head == nil) res = 0;
		else res = head->getPos(level).first;
		unlock();
		return res;
	}

	ProgressMonitor::State ProgressMonitor::getState( const Progress *p ) const
	{
		if (asyncEnabled()) {
			lock();
			const Progress *k = head;
			while (k && k != p) {
				k = k->next;
			}
			State res;
			if (k) res = State(k->getState(),k->getStateParam());
			unlock();
			return res;
		} else {
			return State(p->getState(),p->getStateParam());
		}
	}

	LightSpeed::natural ProgressMonitor::getLevel( const Progress *x )
	{
		lock();
		natural l = 0;
		const Progress *k = head;
		while (k && k != x) {
			l++;
		}
		if (k == 0) l = naturalNull;
		unlock();
		return l;
	}
}
