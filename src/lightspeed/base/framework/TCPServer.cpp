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

namespace LightSpeed {


const ITCPServerContext *ITCPServerConnection::reject =
	(ITCPServerContext  *)0x1;


TCPServer::TCPServer(ITCPServerConnection &handler,natural maxThreads)
:internalExecutor(new ParallelExecutor(maxThreads))
,handler(&handler),handler2(0),shutdown(1)
{
	executor = internalExecutor;
}

TCPServer::TCPServer(ITCPServerConnection &handler, TCPSharedThreadPool *connExecutor)
:executor(connExecutor)
,handler(&handler),handler2(0),shutdown(1)
{


}

TCPServer::TCPServer(ITCPServerConnHandler &handler,natural maxThreads)
:internalExecutor(new ParallelExecutor(maxThreads))
,handler(0),handler2(&handler),shutdown(1)
{
	executor = internalExecutor;
}

TCPServer::TCPServer(ITCPServerConnHandler &handler, TCPSharedThreadPool *connExecutor)
:executor(connExecutor)
,handler(),handler2(&handler),shutdown(1)
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

	shutdown = 0;
	mother = tcpsource;
	eventListener(mother,this);
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
		eventListener(conn->getStream(),conn).erase();
	}
	Notifier ntf;
	//finally remove mother socket from the listener
	eventListener(mother,this).erase().onCompletion(&ntf);
	//wait for completion
	ntf.wait(naturalNull,false);

	//reset all open descriptors
	connList.clear();

	//close all other ports
	otherPorts.clear();

	//close mother socket
	mother = NetworkStreamSource();

	//kill network event listener to cleanup any forgotten connection
	eventListener = NetworkEventListener();
	//everything is clean
	//leave shutdown flag on to prevent any future tries to repeat this

}

void TCPServer::wakeUp(natural ) throw() {
	acceptConn(mother, 0);
	//re-register mother
	if (mother.hasItems()) eventListener(mother,this);
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

	} else {
		//call handler
		ITCPServerContext *ctx = handler->onConnect(remoteAddr);

		//if connection is rejected
		if (ctx == ITCPServerConnection::reject) {
			//return from wakeUp - connection will be closed
			return;
		}

		//create connection object
		PConnection conn(new Connection(this,stream,ctx,sourceId));
		//register connection
		connList.insert(conn.getMT());

		//register connection on event listened
		eventListener(stream,conn).forInput();
	}
}

class TCPServer::RunWorker: public IExecutor::ExecAction::Ifc {
public:
	LIGHTSPEED_CLONEABLECLASS;

	RunWorker(TCPServer &server, Connection *conn):server(server),conn(conn) {}
	void operator()() const {
		server.worker(conn);
	}
protected:
	 TCPServer &server;
	mutable Connection *conn;
};


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
					owner.workerDisconnect(this);
//					owner.executor->execute(RunDisconnect(owner,this));
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
			//compatible mode
			//peak count of bytes on the stream
			bool canread  = stream->canRead();
		//zero means that connection has been closed
			if (!canread) { //connection closed

				owner.close(this);
			} else {
		//		Sync _(owner.lock); //NOTE lock here causing deadlock
				owner.executor->execute(RunWorker(owner,this));
			}
		}
	} catch (...) {
		owner.close(this);
	}
}



void TCPServer::worker(Connection *conn) {
	while (handler->onData(conn->getStream(), conn->getContext()) && !Thread::canFinish()) {
		if (!conn->getStream()->dataReady()) {
			reuse(conn);
			return;
		}

	}
	close(conn);
}



void TCPServer::close(Connection *k) {
	Sync _(lock);
	PConnection key(k);
	connList.erase(key.getMT());
}

void TCPServer::reuse(Connection *k) {
	eventListener(k->getStream(), k).forInput();
}

void TCPServer::reuse(Connection *k, ITCPServerConnHandler::Command command) {
	natural tmm;
	switch (command) {
	case ITCPServerConnHandler::cmdWaitRead:
		tmm = k->getDataReadyTimeout();
		if (tmm == naturalNull) eventListener(k->getStream(),k).forInput();
		else eventListener(k->getStream(),k).forInput().timeout(tmm);
		break;
	case ITCPServerConnHandler::cmdWaitWrite:
		tmm = k->getWriteReadyTimeout();
		if (tmm == naturalNull) eventListener(k->getStream(),k).forOutput();
		else eventListener(k->getStream(),k).forOutput().timeout(tmm);
		break;
	case ITCPServerConnHandler::cmdWaitReadOrWrite:
		tmm = std::min(k->getDataReadyTimeout(),k->getWriteReadyTimeout());
		if (tmm == naturalNull) eventListener(k->getStream(),k).forOutput().forInput();
		else eventListener(k->getStream(),k).forOutput().forInput().timeout(tmm);
		break;
	default: close(k);break;
	}

}

NetworkEventListener  TCPServer::getEventListener()
{
	return eventListener;
}

bool TCPServer::isRunning()
{
	return shutdown == 0;
}


void TCPSharedThreadPool::execute( const IExecAction &action )
{
	Sync _(lock);
	executor.execute(action);
}

bool TCPSharedThreadPool::stopAll( natural timeout /*= 0*/ )
{
	Sync _(lock);
	return executor.stopAll(timeout);
}


void TCPServer::workerEx(Connection *owner, natural eventId) {
	ITCPServerConnHandler::Command cmdout;
	switch (eventId) {
	case INetworkResource::waitForInput:
		cmdout = handler2->onDataReady(owner->getStream(),owner->getContext());break;
	case INetworkResource::waitForOutput:
		cmdout = handler2->onWriteReady(owner->getStream(),owner->getContext());break;
	case INetworkResource::waitTimeout:
		cmdout = handler2->onTimeout(owner->getStream(),owner->getContext());break;
	default:
		cmdout = ITCPServerConnHandler::cmdRemove;
	}

	reuse(owner,cmdout);

}
void TCPServer::workerDisconnect(Connection *owner) {
	handler2->onDisconnectByPeer(owner->getContext());
	close(owner);
}

void TCPSharedThreadPool::join() {
	Sync _(lock);
	return executor.join();

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
	eventListener(tcpsource,k);
	return id;
}

void TCPServer::OtherPortAccept::wakeUp(natural ) throw()  {
	owner.acceptConn(nss,sourceId);
	owner.getEventListener()(nss,this);
}

TCPServer::OtherPortAccept::OtherPortAccept(TCPServer& owner, natural sourceId,
		const NetworkStreamSource& nss)
	:owner(owner),sourceId(sourceId),nss(nss)
{
}

void TCPSharedThreadPool::finish() {
	Sync _(lock);
	executor.finish();
}

bool TCPSharedThreadPool::isRunning() const {
	Sync _(lock);
	return executor.isRunning();
}

}

