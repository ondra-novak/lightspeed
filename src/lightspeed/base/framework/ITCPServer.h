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
	 * you shuld use this wisely especially when handler creates a lot of
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

};

///Brand new handler which can be used instead old ITCPServerConnection.
/** Server supports both handlers, but only one at same time. You can initialize
 * server using new handler, then events of this handler will be calleded
 */
class ITCPServerConnHandler: virtual public IInterface {
public:

	///Defines various commands for Socket pool
	/** When handler finish all operations on connection, it can specify, what it should TCP server
	 * do with finished connection
	 */
	enum Command {
		/** Removes connection from the pool. TCP server stops monitor this connection and if it
		 * is not shared, connection is disconnected. Because you can keep pointer to the connection
		 * outside of TCP server (it is RefCntPtr), connection can remain opened until last reference
		 * is destroyed. This rule is not applied on ITCPServerContext, which is destroyed regardless
		 * how many objects refers this connection
		 *
		 */
		cmdRemove = 0,
		/** commands TCP server to monitor connection for incoming data. Once
		 * there is at least one byte to read, onDataReady() is called
		 */
		cmdWaitRead = 1,
		/** commands TCP Server to monitor connection for space in output buffer.
		 * While there is a lot of writing because output connection is slow, handler can request
		 * TCP Server to monitor output buffers. When there is space in the ouput buffer,
		 * onWriteReady() is called
		 */
		cmdWaitWrite = 2,
		/** commands TCP Server to monitor both. If one of events is caught, appropriate function
		 * is called. TCP Server always trigger only one event at once and disables monitoring until
		 * handler returns
		 */
		cmdWaitReadOrWrite = 3
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
	///Called when peer connection has been closed
	/** This is difference from ITCPServerConnection, when disconnected streams are reported
	 * through onData while read() will return zero. If disconnection is detected during waiting,
	 * function onDisconnectByPeer is called. Function is not called, when disconnection is detected
	 * by handler while reading. When function read() returns 0, handler should return cmdRemove in reaction.
	 *
	 * @param context associated context. Because stream has been disconnected, you cannot access it.
	 *
	 * @note This function is called in control thread. You should not perform long task, because it blocks
	 * processing other connections. If you need to execute a long task, use server's parallel executor to
	 * execute task in one of server's thread.
	 *
	 */
	virtual void onDisconnectByPeer(ITCPServerContext *context) throw () = 0;

	///Called, when connection has incomed
	/**
	 * Function prepares handler to new connection. It can create new context, which will be associated with
	 * the new connection or it can reject connection.
	 *
	 * @param addr address of incomming connection.
	 * @return pointer to newly created context. Server expects, that object will be destroyable by delete operator,
	 *   	so it should be allocated on heap (but it can have overloaded delete operator to perform another cleanup
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
	 * processing other connections. Function should only configure connection and return command to TCPServer
	 * insert connection into connection-pool. At this point, stream is not available. To send welcome
	 * message, return cmdWaitWrite and function onWriteReady() will be called immediately with the stream.
	 *
	 */
	virtual Command onAccept(ITCPServerConnControl *controlObject, ITCPServerContext *context) = 0;
	virtual ~ITCPServerConnHandler() {}
};

class AbstractTCPServerContext: public ITCPServerContext {
public:
	const NetworkAddress addr;
	ITCPServerConnControl *concontrol;

	AbstractTCPServerContext(const NetworkAddress &addr,ITCPServerConnControl *concontrol)
		:addr(addr),concontrol(concontrol) {}
};

///Abstract TCP Server connection - you need implement onDataReady only
class AbstractTCPServerConnHandler: public ITCPServerConnHandler {
public:


	virtual Command onWriteReady(const PNetworkStream &, ITCPServerContext *) throw() {return cmdWaitRead;}
	virtual Command onTimeout(const PNetworkStream &, ITCPServerContext *) throw() {return cmdRemove;}
	virtual void onDisconnectByPeer(ITCPServerContext *) throw() {}
	virtual ITCPServerContext *onConnect(const NetworkAddress &addr, ITCPServerConnControl *controlObject) throw() {
		return new AbstractTCPServerContext(addr,controlObject);
	}


};
class ITCPServerConnection: public IInterface {
public:
	///called, when data available on connection
	/**
	 * @param stream network stream containing the arrived data
	 * @param context object associated with the connection
	 * @retval true continue waiting for new data
	 * @retval false close the connection instantly
	 */
	virtual bool onData(const PNetworkStream &stream, ITCPServerContext *context)  throw() = 0;
	///called on new connection
	/**
	 * @param addr address from connection arrived
	 * @return function can return pointer to user object which is associated with
	 * the connection. This object is later passed in function onData to identify
	 * the connection. When connection is closed, object is deleted (using
	 * delete operator). Function can return NULL then no context is associated with
	 * the connection. Function can return value of "reject" const if it don't
	 * want to accept connection from the specified address
	 *
	 */
	virtual ITCPServerContext *onConnect(const NetworkAddress &addr) throw() = 0;
	virtual ~ITCPServerConnection() {}


	class ConnectionAddress: public ITCPServerContext {
	public:
		ConnectionAddress(const NetworkAddress addr):clientAddr(addr) {}

		NetworkAddress clientAddr;
	};

	///use as return value of onConnect() to reject this connection;
	static const ITCPServerContext *reject;
};

}


#endif /* ITCPSERVER_H_ */
