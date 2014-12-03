/*
 * job.cpp
 *
 *  Created on: 19. 9. 2014
 *      Author: ondra
 */

#include "job.h"
#include "../../mt/atomic.h"
#include "ijobcontrol.h"

namespace LightSpeed {

Job::Job():state(not_running)
{
}

Job::Job(ISleepingObject* jobWakeup):state(not_running),ntf_so(jobWakeup) {
}

Job::Job(IJobControl* jobWakeup):state(not_running),ntf_exec(jobWakeup) {
}

Job& Job::operator =(const Job& source) {
	if (&source != this) {
		stop();
		jobFn = source.jobFn;
		ntf_so  = source.ntf_so;
		ntf_exec = source.ntf_exec;
		state = source.state;
	}
	return *this;
}

void Job::bindTo(ISleepingObject* jobWakeup) {
	ntf_so = jobWakeup;
	ntf_exec = nil;
}

void Job::bindTo(IJobControl* jobWakeup) {
	ntf_so = nil;
	ntf_exec = jobWakeup;
}

void Job::unbind() {
	ntf_so = nil;
	ntf_exec = nil;
}

void Job::execute(const IJobFunction& action) {
	State v = setState(not_running, ready);
	if (v == not_running) {
		jobFn = JobFunction(action);
		run();
	} else {
		throw UnexpectedJobState(THISLOCATION, v);
	}
}

void Job::sleep(const IJobFunction& action) {
	State v = setState(running, sleeping, running_event, ready);
	if (v != running && v != running_event) throw UnexpectedJobState(THISLOCATION, v);
	jobFn = JobFunction(action);
}

void Job::stop() {
	state = not_running;
}

bool Job::run() {
	State v = setState(ready,running);
	bool ran = false;
	while (v != ready) {
		jobFn();
		v = setState(ready,running);
	}
	return ran;
}

void Job::wakeUp(natural reason) throw() {
	this->reason = reason;
	State v = setState(sleeping,ready,running,running_event);
	if (v == sleeping) {
		if (ntf_so) ntf_so->wakeUp(reason);
		else if (ntf_exec) ntf_exec->execute(JobFunction::create(this,&Job::run));
		else run();
	}
}


void Job::join() {
}

void Job::finish() {
}

Job::State Job::getState() const {
	return (Job::State)state;
}

Job::State Job::setState(State oldState, State newState) {
	return (State)lockCompareExchange(state,oldState,newState);
}

Job::State Job::setState(State oldState1, State newState1, State oldState2, State newState2) {
	atomicValue a,b;
	do {
		a = state;
		if (a == oldState1) b = lockCompareExchange(state,a,newState1);
		else if (a == oldState2) b = lockCompareExchange(state,a,newState2);
		else return (State)a;
	} while (b != a);
	return (State)a;
}

} /* namespace LightSpeed */
