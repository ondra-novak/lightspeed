/*
 * TCPServer.h
 *
 *  Created on: 8.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_TCPSERVER_H_
#define LIGHTSPEED_BASE_TCPSERVER_H_

#include "app.h"
#include "../streams/netio.h"
#include "../actions/parallelExecutor.h"
#include "../sync/synchronize.h"
#include "../containers/set.h"
#include <functional>
#include "../memory/clusterAllocFactory.h"


#include "ITCPServer.h"
#include "../../mt/fastlock.h"
#include "../../mt/mutex.h"
#include "../memory/sharedPtr.h"

namespace LightSpeed {



class TCPServer {
public:



	///Constructs the server
	/**
	 * @param handler reference to object, which handles incoming data and connections
	 * @param maxThreads Specifies maximum count of threads will be prepared
	 *              to process connections. This doesn't limit count of connections
	 *				because idle connections are not assigned to the threads.
	 *				There will be always +1 thread created to accept new connections,
	 *				so specify 32 causes, that there will be 33 threads at all.	 
	 */
	 
	TCPServer(ITCPServerConnHandler &handler, natural maxThreads = 32);
	///Constructs the server sharing thread pool with another server
	/**
	 * @param handler reference to object, which handles incoming data and connections
	 * @param connExecutor executor object containing thread pool. It
	 * must go through TCPSharedTheadPool object to ensure MT safety
	 * (executors cannot )
	 */
	TCPServer(ITCPServerConnHandler &handler, IExecutor *connExecutor);

	virtual ~TCPServer();

	///starts service on given port
	/**
	 *
	 * @param port number specifies port where service is opened. 
	 *				Default value 0 starts server on random unused port
	 * @param bindLocalOnly if true, only local connections are allowed
	 * @param connTimeout defines default timeout for blocking reading and writting for the connection
	 * @return returns port number where server has been opened. If port has
	 *              been specified, function returns same value.
	 */
	natural start(natural port = 0, 
		  	      bool bindLocalOnly = true, 
			      natural connTimeout = 30000);

	///starts service using specified stream source
	void start(NetworkStreamSource tcpsource);

	///stops service
	/**
	 * Stop service
	 * @param doneNotify pointer to interface notified, that server is stopped
	 */
	void stop();

	NetworkEventListener getEventListener();

	bool isRunning();

	natural addPort(natural port, bool bindLocalOnly = true, natural connTimeout = 30000);

	natural addPort(NetworkStreamSource tcpsource);

	///retrieves internal executor
	/** You can use executor to submit short time jobs without need to allocate additional threads
	 * @return Function returns pointer to executor. Do not release the pointer. If server doesn't
	 * use internal executor, function returns NULL. Then you must use getExecutor() method instead,
	 * but this don't need to be ParallelExecutor
	 */
	ParallelExecutor *getInternalExecutor() const {return internalExecutor;}

	///retrieves executor
	/** You can use executor to submit short time jobs without need to allocate additional threads. Note
	 * that server don't need to use ParallerExecutor. It can be synchronous executor blocking any
	 * server requests until job is finished.
	 * @return Function returns pointer to executor. Do not release the pointer.
	 */
	IExecutor *getExecutor() const {return executor;}


	///Retrieves count of opened connections
	natural getConnectionCount() const {return connList.size();}

protected:


	class Connection: public RefCntObj, public ISleepingObject, public ITCPServerConnControl {
	public:

		Connection(TCPServer *owner, PNetworkStream stream, ITCPServerContext *ctx, natural sourceId)
			:owner(*owner),stream(stream),ctx(ctx),
			 dataReadyTimeout(naturalNull),
			 writeReadyTimeout(naturalNull),
			sourceId(sourceId),
			userWakeupState(0),
			complWakeUp(*this) {
		}

		///wake up when data
		void wakeUp(natural reason) throw();



		const PNetworkStream &getStream() const {return stream;}
		ITCPServerContext *getContext() const {return ctx;}

		virtual TCPServer &getOwner() {return owner;}
		virtual const TCPServer &getOwner() const {return owner;}
		virtual void setDataReadyTimeout(natural timeout) {dataReadyTimeout = timeout;}
		virtual void setWriteReadyTimeout(natural timeout) {writeReadyTimeout = timeout;}
		virtual natural getDataReadyTimeout() const {return dataReadyTimeout;}
		virtual natural getWriteReadyTimeout() const {return writeReadyTimeout;}
		virtual natural getSourceId() const {return sourceId;}
		IExecutor *getServerExecutor() const {return owner.getExecutor();}
		ISleepingObject *getUserSleeper() throw() {return &complWakeUp;}


	protected:
		friend class TCPServer;

		TCPServer &owner;
		PNetworkStream stream;
		AllocPointer<ITCPServerContext> ctx;
		natural dataReadyTimeout;
		natural writeReadyTimeout;
		natural sourceId;
		atomic userWakeupState;

		static const atomicValue userWakeupEnabled = 1;
		static const atomicValue userWakeupEvent = 2;
		static const atomicValue userWakeupEnabledEvent = userWakeupEnabled | userWakeupEvent;


		class CompletionWakeUp: public ISleepingObject {
		public:
			Connection &owner;
			CompletionWakeUp(Connection &owner):owner(owner) {}

			virtual void wakeUp( natural reason = 0 ) throw();

		};

		CompletionWakeUp complWakeUp;
		void userWakeup(natural reason);
		atomicValue setUserWakeupState(atomicValue flag);


	};

	class OtherPortAccept: public ISleepingObject {
	public:
		virtual void wakeUp(natural reason = 0) throw();
		OtherPortAccept(TCPServer &owner, natural sourceId, const NetworkStreamSource &nss);

		NetworkStreamSource getSocket() const { return nss; }		
	protected:
		natural sourceId;
		NetworkStreamSource nss;

		TCPServer &owner;
	};

	typedef RefCntPtr<Connection> PConnection;
	typedef Set<PConnection,std::less<PConnection> > ConnectionList;
	typedef Synchronized<Mutex> Sync;

	IExecutor *executor;
	AllocPointer<ParallelExecutor> internalExecutor;
	NetworkEventListener eventListener;
	NetworkStreamSource mother;
	AutoArray<SharedPtr<OtherPortAccept> > otherPorts;
	ConnectionList connList;
	Mutex lock;
	ITCPServerConnHandler *handler2;
	atomic shutdown;

	natural readTimeout, writeTimeout;


	class Sleeper: public ISleepingObject {
	public:
		virtual void wakeUp(natural reason = 0) throw();
		TCPServer &owner;
		Sleeper(TCPServer &owner):owner(owner) {}
	};
	Sleeper sleeper;


	void acceptConn(NetworkStreamSource& listenSock, natural sourceId) throw();
	void worker(Connection *owner);
	void workerEx(Connection *owner, natural eventId);
	void workerDisconnect(Connection *owner);
	void checkUserWakeup(Connection *k);
	void close(Connection *k);
	void reuse(Connection *k);
	void reuse(Connection *k, ITCPServerConnHandler::Command command);
	bool running;

	class RunWorkerEx;
	class RunWorkerCompletion;
	class RunDisconnect;
};


}

#endif /* LIGHTSPEED_BASE_TCPSERVER_H_ */
