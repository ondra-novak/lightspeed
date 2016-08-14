/*
 * TCPServer.cpp
 *
 *  Created on: 8.4.2011
 *      Author: ondra
 */

#include "TCPServer.h"
#include "../../mt/atomic.h"
#include "../interface.tcc"
#include "../containers/autoArray.tcc"
#include "../containers/map.tcc"

namespace LightSpeed {


TCPServer::TCPServer(ITCPServerConnHandler &handler,natural maxThreads)
:internalExecutor(new ParallelExecutor(maxThreads))
,handler2(&handler),shutdown(1),sleeper(*this)
{
	executor = internalExecutor;
}

TCPServer::TCPServer(ITCPServerConnHandler &handler, IExecutor *connExecutor)
:executor(connExecutor)
,handler2(&handler),shutdown(1),sleeper(*this)
{


}


TCPServer::~TCPServer()
{
	stop();
//event listener first stops listening
//executor then waits to finish all unfinished tasks
}



void TCPServer::start(NetworkStreamSource tcpsource)
{

	Sync _(lock);

	if (lockCompareExchange(shutdown,1,0) != 1) return;

	eventListener = INetworkServices::getNetServices().createEventListener();

	shutdown = 0;
	mother = tcpsource;
	eventListener->add(mother,&sleeper,INetworkResource::waitForInput,naturalNull,0);
}



natural TCPServer::start(natural port, bool bindLocalOnly, natural connTimeout)
{
	Sync _(lock);
	NetworkStreamSource newsource(port,naturalNull,naturalNull,connTimeout,bindLocalOnly);
	NetworkAddress addr = newsource.getLocalAddress();
	natural retval = addr->getIfc<INetworkAddrEx>().getPortNumber();
	start(newsource);
	return retval;
}



void TCPServer::stop()
{
	//lock for shutdown
	//this also prevents to accepting new connections
	if (lockCompareExchange(shutdown,0,1) != 0) return;


	//stop all executed threads
	//this must be done first, because workers may need all others parts still available
	//shutdown flag also prevents to creating new workers
	executor->stopAll(naturalNull);

	//now, we can lock for modifications of the internal data
	Sync _(lock);

	//remove all sockets from the event listener
	for (ConnectionList::Iterator iter = connList.getFwIter(); iter.hasItems();) {
		PConnection conn = iter.getNext().getMT();
		eventListener->remove(conn->getStream(),conn);
	}

	///remove all other ports
	for (natural i = 0; i < otherPorts.length(); i++) {
		eventListener->remove(otherPorts[i]->getSocket(), otherPorts[i]);
	}

	Notifier ntf;
	//finally remove mother socket from the listener
	eventListener->remove(mother.getHandle(),&sleeper,&ntf);
	//wait for completion
	ntf.wait(naturalNull,false);

	//reset all open descriptors
	connList.clear();

	//clear other ports
	otherPorts.clear();

	///clear event listener, stop the thread
	eventListener = null;
}

void TCPServer::Sleeper::wakeUp(natural ) throw() {
	owner.acceptConn(owner.mother, 0);
	//re-register mother
	if (owner.mother.hasItems()) owner.eventListener->add(owner.mother,this,owner.mother->getDefaultWait(),naturalNull,0);
}
void TCPServer::acceptConn(NetworkStreamSource& listenSock, natural sourceId) throw() {

	//called when new connection arrived

	//if shutdown active - reject
	if (readAcquire(&shutdown)) return;

	Sync _(lock);
	//if shutdown active - reject
	if (readAcquire(&shutdown)) return;

	//receive connection
	PNetworkStream stream = listenSock.getNext();
	stream = stream.getMT();
	//retrieve remote address
	NetworkAddress remoteAddr = listenSock.getRemoteAddress();

	if (handler2) {
		//call handler
		ITCPServerContext *ctx = handler2->onIncome(remoteAddr);
		//if connection is rejected
		if (ctx == 0) return;
		//create connection object
		PConnection conn(new Connection(this,stream,ctx,sourceId));
		//register connection
		connList.insert(conn.getMT());
		//ask handler for first action
		ITCPServerConnHandler::Command cmd =  handler2->onAccept(conn,ctx);
		//carry out the action
		reuse(conn,cmd);
	}
}



class TCPServer::RunWorkerEx: public IExecutor::ExecAction::Ifc {
public:
	LIGHTSPEED_CLONEABLECLASS;

	RunWorkerEx(TCPServer &server, Connection *conn, natural eventId):server(server),conn(conn),eventId(eventId) {}
	void operator()() const {
		server.workerEx(conn,eventId);
	}
protected:
	TCPServer &server;
	mutable Connection *conn;
	natural eventId;
};

class TCPServer::RunWorkerCompletion: public IExecutor::ExecAction::Ifc {
public:
	LIGHTSPEED_CLONEABLECLASS;

	RunWorkerCompletion(TCPServer &server, Connection *conn):server(server),conn(conn) {}
	void operator()() const {
		server.workerEx(conn,eventUserWakeup);
	}
protected:
	TCPServer &server;
	mutable Connection *conn;
};


class TCPServer::RunDisconnect: public IExecutor::ExecAction::Ifc {
public:
	LIGHTSPEED_CLONEABLECLASS;

	RunDisconnect(TCPServer &server, Connection *conn):server(server),conn(conn) {}
	void operator()() const {
		server.workerDisconnect(conn);
	}
protected:
	TCPServer &server;
	mutable Connection *conn;
};

void TCPServer::Connection::wakeUp(natural reason) throw() {
	//called when some data arrived to the opened connection

	//if shutdown - reject request
	if (owner.shutdown) return;
		try {
			if (owner.handler2 != 0) {

				if (reason & INetworkResource::waitForInput) {
					bool canread  = stream->canRead();
					if (!canread) { //connection closed
						//interface states, that disconnect is called in the control thread
						//so let it be
						//disconnect it now
						owner.workerDisconnect(this);
					} else {
						owner.executor->execute(RunWorkerEx(owner,this,reason));
					}
				} else if (reason & INetworkResource::waitForOutput){
					owner.executor->execute(RunWorkerEx(owner,this,INetworkResource::waitForOutput));
				} else if (reason == INetworkResource::waitTimeout){
					owner.executor->execute(RunWorkerEx(owner,this,INetworkResource::waitTimeout));
				} else {
					owner.close(this);
				}


			} else {
				owner.close(this);
			}

		} catch (...) {
			owner.close(this);
		} 
}



void TCPServer::Connection::userWakeup(  )
{
	owner.executor->execute(RunWorkerCompletion(owner,this));
}



void TCPServer::close(Connection *k) {
	Sync _(lock);
	PConnection key(k);
	connList.erase(key.getMT());
}

void TCPServer::reuse(Connection *k) {
	eventListener->add(k->getStream(), k, INetworkResource::waitForInput);
}

void TCPServer::reuse(Connection *k, ITCPServerConnHandler::Command command) {
	natural tmm;
	switch (command) {
	case ITCPServerConnHandler::cmdWaitRead:
		tmm = k->getDataReadyTimeout();
		eventListener->add(k->getStream(),k,INetworkResource::waitForInput,tmm);
		break;
	case ITCPServerConnHandler::cmdWaitWrite:
		tmm = k->getWriteReadyTimeout();
		eventListener->add(k->getStream(),k,INetworkResource::waitForOutput,tmm);
		break;
	case ITCPServerConnHandler::cmdWaitReadOrWrite:
		tmm = std::min(k->getDataReadyTimeout(),k->getWriteReadyTimeout());
		eventListener->add(k->getStream(),k,INetworkResource::waitForInput | INetworkResource::waitForOutput,tmm);
		break;
	case ITCPServerConnHandler::cmdWaitUserWakeup:
		break;

	default: close(k);break;
	}

}

PNetworkEventListener  TCPServer::getEventListener()
{
	return eventListener;
}

bool TCPServer::isRunning()
{
	return shutdown == 0;
}




void TCPServer::workerEx(Connection *owner, natural eventId) {

	ITCPServerConnHandler::Command cmdout;
	try {
		switch (eventId) {
		case INetworkResource::waitForInput:
			cmdout = handler2->onDataReady(owner->getStream(),owner->getContext());break;
		case INetworkResource::waitForOutput:
			cmdout = handler2->onWriteReady(owner->getStream(),owner->getContext());break;
		case INetworkResource::waitTimeout:
			cmdout = handler2->onTimeout(owner->getStream(),owner->getContext());break;
		case eventUserWakeup:
			cmdout = handler2->onUserWakeup(owner->getStream(),owner->getContext(), owner->userWakeupReason);break;
		default:
			cmdout = ITCPServerConnHandler::cmdRemove;
		}

		//if cmdout is set to cmdWaitUserWakeup, then let connection goes sleep
		while (cmdout == ITCPServerConnHandler::cmdWaitUserWakeup) {
			//push userWakeupEnabled to the state
			atomicValue v = lockCompareExchange(owner->userWakeupState, 0, owner->userWakeupSleepingState);
			//test whether it successful,
			//this can fail only when event has been recorded
			if (v & owner->userWakeupEvent) {
				//reset the state
				if (lockCompareExchange(owner->userWakeupState, v, 0) == v) {
					//call the handler again
					cmdout = handler2->onUserWakeup(owner->getStream(),owner->getContext(), owner->userWakeupReason);
					//depend on result, we can repeat or return to normal processing
				} else {
					//this is uncommon situation. Failing to reset means, that there are two threads
					//running the same connection. This should not happen
					//the only solution is not allow this thread to continue
					return;
				}
			} else {
				//we successfuly set the state, leave connection now
				return;
			}
		}

	} catch (...) {
		cmdout = ITCPServerConnHandler::cmdRemove;
	}

	//reuse connection depend on status
	reuse(owner,cmdout);
}

void TCPServer::workerDisconnect(Connection *owner) {
	handler2->onDisconnectByPeer(owner->getContext());
	close(owner);
}


natural TCPServer::addPort(natural port, bool bindLocalOnly,
		natural connTimeout) {

	NetworkStreamSource newsource(port,naturalNull,naturalNull,connTimeout,bindLocalOnly);
	NetworkAddress addr = newsource.getLocalAddress();
//	natural retval = addr->getIfc<INetworkAddrEx>().getPortNumber();
	return addPort(newsource);

}

natural TCPServer::addPort(NetworkStreamSource tcpsource) {
	natural id = otherPorts.length()+1;
	OtherPortAccept *k = new OtherPortAccept(*this,id,tcpsource);
	otherPorts.add(k);
	eventListener->add(tcpsource,k,INetworkResource::waitForInput);
	return id;
}

void TCPServer::OtherPortAccept::wakeUp(natural ) throw()  {
	owner.acceptConn(getSocket(),sourceId);
	owner.getEventListener()->add(getSocket(),this);
}

TCPServer::OtherPortAccept::OtherPortAccept(TCPServer& owner, natural sourceId,
		const NetworkStreamSource& nss)
	:owner(owner),sourceId(sourceId),nss(nss)
{
}


void TCPServer::Connection::CompletionWakeUp::wakeUp( natural reason /*= 0 */ ) throw()
{
	owner.userWakeupReason = reason;
	//try to record even when waiting is not expected
	atomicValue value = lockCompareExchange(owner.userWakeupState,0,userWakeupEvent);
	//this fails, when userWakeupEnabled is enabled, so connection is already sleeping
	if (value & userWakeupSleepingState) {

		//clear internal state, because we are going to wakeUp the connection
		if (lockCompareExchange(owner.userWakeupState, value, 0) == value) {
			//wakeup the connection
			owner.userWakeup();
		} else {
			//note setting to zero can fail. This can happen, when two threads wakening
			//same connection at the same time (twice)
			//first resets the state, second fails.
			//in this case, repeat this function
			this->wakeUp(reason);
		}
	} else {
		//state has been remembered, we can leave connection now because it is still
		//in handler. It will execute userWakeup immediately when it leaves its thread
	}
}

}

