/*
 * ITCPServer.h
 *
 *  Created on: Jan 26, 2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ITCPSERVER_H_
#define LIGHTSPEED_BASE_ITCPSERVER_H_
#include "../streams/netio.h"

namespace LightSpeed {

class ISleepingObject;
class IExecutor;


///Context of connection.
/** Interface doesn't define any functions, only virtual destructor, that allows to destroy context from the server
 * without knowing implementation.
 */
class ITCPServerContext: virtual public IInterface {
public:
	virtual ~ITCPServerContext() {}
};


class TCPServer;

///Interface to control single connection
/**
 * Pointer to interface is recived by function ITCPServerConnHandler::onConnect() and can be stored into
 * context. You can modify parameters of connection in handler. You can access owner server or modify
 * timeouts.
 *
 * @note MT Safety - all functions are safe when they called from handler. Otherwise functions should to
 * be considered as MT unsafe. Settings timeout outside of handler while connection is waiting
 * in connection-pool has no effect, until handler is called, then new values are applied.
 */
class ITCPServerConnControl: public IInterface {
public:
	///retrieves reference to owner server
	/** In projects where multiple servers there can be useful to know, which connection belongs to which server */
	virtual TCPServer &getOwner() = 0;
	///retrieves reference to owner server
	/** In projects where multiple servers there can be useful to know, which connection belongs to which server */
	virtual const TCPServer &getOwner() const = 0;
	///Sets timeout applied while connection is waiting to receive data
	/** When handler puts connection into dataReady-waiting, this timeout can limit how long connection
	 * will wait, until new data arrives. When timeout elapses, onTimeout() event is called.
	 *
	 * @param timeout time in milliseconds always measured from begining of the waiting. Default value is
	 * infinite
	 *
	 * @note in case, that handler puts connection into both waiting states, shorter timeout is applied
	 */
	virtual void setDataReadyTimeout(natural timeout) = 0;
	///Sets timeout applied while connection is waiting to empty output buffers
	/** When handler puts connection into writeReady-waiting, this timeout can limit how long connection
	 * will wait, until output buffer is emptied. When timeout elapses, onTimeout() event is called.
	 *
	 * @param timeout time in milliseconds always measured from beginning of the waiting. Default value is
	 * infinite
	 *
	 * @note in case, that handler puts connection into both waiting states, shorter timeout is applied
	 */
	virtual void setWriteReadyTimeout(natural timeout) = 0;
	virtual natural getDataReadyTimeout() const = 0;
	virtual natural getWriteReadyTimeout() const = 0;
	///retrieves ID of connection source
	/**
	 * Function is useful when multiple-port server is running. In this case, function returns
	 * ID of port, through which connection has arrived. Master port has always sourceId zero, other
	 * ports are numbered as they were added
	 *
	 * @return
	 */
	virtual natural getSourceId() const = 0;

	///Retrieves pointer to the server's executor
	/** Anytime handler need to spawn a asynchronous job, it can use
	 * server's internal executor. Executor uses server's thread pool, so
	 * you should use this wisely especially when handler creates a lot of
	 * threads.
	 *
	 * Best use of this function is to request server thread for request
	 * waiting for external event. Because postponed request
	 * lost its thread, this can be used to allocate a new tread to finish
	 * the request.
	 *
	 * @return pointer to server's executor
	 */
	virtual IExecutor *getServerExecutor() const = 0;

	///Returns object that can be used to wake connection while it has been put into inactive state
	/**
	 * When the handler returns cmdWaitUserWakeup, then connection is put into inactive state, where
	 * it no longer receives events from the network event listener. It just sleeping without
	 * occupying any thread.
	 *
	 * If you want to wakeup such connection, you have to call wakeUp() on userSleeper. This
	 * function returns pointer to such object bound to the current connection.
	 *
	 * You can call the wakeUp() function even if connection is active. The event is recorded
	 *  and prevents it to sleep in the future.
	 *
	 */

	virtual ISleepingObject *getUserSleeper() throw() = 0;
	

};

///User handler to define actions on various events happened in TCP Server
class ITCPServerConnHandler: virtual public IInterface {
public:

	///Defines various commands for
	/** When the handler finish all operations on the connection, 
	* it can specify, what it should the TCP server
	 * do with the finished connection
	 */
	enum Command {
		/** Removes the connection from the pool. The TCP server stops monitor this connection 
		 * and if it is not shared, the connection is disconnected. Because you can keep pointer 
		 * to the connection outside of the TCP server (it is RefCntPtr), the connection can remain 
		 * opened until the last reference is destroyed. This rule is not valid for the ITCPServerContext, 
		 * which is always destroyed despite on how many objects refers this connection
		 *
		 */
		cmdRemove = 0,
		/** commands the TCP server to monitor the connection for an incoming data. Once
		 * there is at least one byte to read, the function onDataReady() is called
		 */
		cmdWaitRead = 1,
		/** commands the TCP Server to monitor the connection for a space in the output buffer.
		 * If there is a lot of writing because the output connection is slow, handler can request
		 * the TCP Server to monitor output buffers. When there is a space in the ouptut buffer,
		 * the function onWriteReady() is called
		 */
		cmdWaitWrite = 2,
		/** commands the TCP Server to monitor both the input and the output. 
		* If one of events is caught,the appropriate function
		 * is called. The TCP Server always trigger only one event at once and disables monitoring until
		 * the handler returns
		 */
		cmdWaitReadOrWrite = 3,
		/**
		 * Stops monitoring and enables the "user wakeup". In this state, connection's handler can
		 * be activated through the "User-wakeup" feature. This can be achieved through an object
		 * returned by the function getUserSleeper() on the ITCPServerConnControl. Invoking
		 * the function wakeUp() brings connection back to the live through the function onUserWakeup.
		 *
		 * Note that this feature is available only when connection is in this state. Invoking the
		 * function wakeUp other time will not wake the connection. However the request is recorded
		 * and once the connection is later put into this state, it is immediately woken back. The
		 * same behavior happen, when somebody calls wakeUp during handler is executed.
		 *
		 *
		*/

		cmdWaitUserWakeup = 4 
	};


	///Called when there are data ready
	/**
	 * @param stream connected stream
	 * @param context user context created during onConnect()
	 * @return command to TCP server
	 */
	virtual Command onDataReady(const PNetworkStream &stream, ITCPServerContext *context) throw() = 0;
	///Called when there are data ready
	/**
	 * @param stream connected stream
	 * @param context user context created during onConnect()
	 * @return command to TCP server
	 *
	 */
	virtual Command onWriteReady(const PNetworkStream &stream, ITCPServerContext *context) throw() = 0;
	///Called when there is no activity at connection for specified time
	/**
	 * @param stream connected stream
	 * @param context user context created during onConnect
	 * @return command to TCP server
	 *
	 * Most common action for this event is cmdRemove causing disconnecting timeouted connections
	 */
	virtual Command onTimeout(const PNetworkStream &stream, ITCPServerContext *context) throw () = 0;
	///Called when user sleeper is waken up using ITCPServerConnControl::getSleeperObject when connection is in apropriate state
	/**
	 * Function will executed when something calls wakeUp on the UserSleeper and connection is
	 * in the state waitUserWakeup.
	 *
	 * @param stream connected stream
	 * @param context user context created during onConnect()
	 * @param reason reason passed through user wakeup
	 * @return command to TCP server. To continue waiting to userWakeup, you can return waitUserWakeup command
	 *
	 * otherwise return correct command.
	 *
	 *
	 */
	virtual Command onUserWakeup(const PNetworkStream &stream, ITCPServerContext *context, natural reason = 0) throw() = 0;
	///Called when the peer connection has been closed
	/**If the disconnection is detected during a waiting,
	 * function onDisconnectByPeer is called. Function is not called, when disconnection is detected
	 * by the handler while reading. When the function read() returns 0, handler should return cmdRemove as reaction.
	 *
	 * @param context associated context.
	 *
	 * @note This function is called in control thread. You should not perform long task, because it blocks
	 * processing other connections. If you need to execute a long task, use server's parallel executor to
	 * execute task in one of server's thread.
	 *
	 */
	virtual void onDisconnectByPeer(ITCPServerContext *context) throw () = 0;

	///Called, when connection income
	/**
	 * The function prepares the handler to new connection. It can create new context, which will be associated with
	 * the new connection or it can reject connection.
	 *
	 * @param addr address of the incoming connection.
	 * @return pointer to newly created context. Server expects, that object will be destroyable by delete operator,
	 *   	so it should be allocated on the heap (but it can have overloaded delete operator to perform another cleanup
	 *   	action). Context is destroyed when connection is disconnected or removed from the server. Function
	 *   	can also return 0 to reject connection. This contradicts with old interface, where returned 0
	 *   	caused, that conection has been created without context. This is not permitted here, you should
	 *   	always create a context. Returning ITCPServerConnection::reject is also accepted as long as
	 *   	TCPServer supports old interface,
	 *
	 */
	virtual ITCPServerContext *onIncome(const NetworkAddress &addr) throw() = 0;

	 ///Called after connection has been accepted.
	 /** @param controlObject pointer to control object. It can be used to modify timeouts or other parameters of this
	 * 		object. Note that this pointer is not available in other event functions, but is still valid
	 * 		until connection is disconnected (i.e. pointer is not valid in destructor). You have to store
	 * 		pointer in the context, if you want to access this object other time.
	 *
	 *  @param context context associated with connection
	 *
	 * @return command to TCP server
	 *
	 * @note function is called in control thread. You should not perform long task, because this blocks
	 * processing of other connections. Function should only configure connection and return command to allow TCPServer to
	 * insert connection into the connection-pool. At this point, stream is not available. To send welcome
	 * message, return cmdWaitWrite and function onWriteReady() will be called immediately with the stream.
	 *
	 */
	virtual Command onAccept(ITCPServerConnControl *controlObject, ITCPServerContext *context) = 0;
	virtual ~ITCPServerConnHandler() {}
};

///helper class to build connection contexts
class AbstractTCPServerContext: public ITCPServerContext {
public:
	const NetworkAddress addr;
	ITCPServerConnControl *concontrol;

	AbstractTCPServerContext(const NetworkAddress &addr,ITCPServerConnControl *concontrol)
		:addr(addr),concontrol(concontrol) {}
};
}


#endif /* ITCPSERVER_H_ */
