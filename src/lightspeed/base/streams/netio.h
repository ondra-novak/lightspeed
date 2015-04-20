/*
 * netio.h
 *
 *  Created on: 16.1.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_STREAMS_NETIO_H_
#define LIGHTSPEED_STREAMS_NETIO_H_


#include "bufferedio.h"
#include "fileio.h"
#include "standardIO.h"
#include "netio_ifc.h"

namespace LightSpeed {


	

	namespace NetworkProtocolPrefix {
		extern const wchar_t *tcp_v4;
		extern const wchar_t *tcp_v6;
		extern const wchar_t *udp_v4;
		extern const wchar_t *udp_v6;
	}


	class NetworkAddress: public ComparableEqual<NetworkAddress> {
	public:

		///Retrieves pointer to underlying object implementing this instance
		PNetworkAddress getHandle() const {return addr;}

		///Converts instance to the string representation
		StringA asString(bool resolve = false) const;

		NetworkAddress() {}
		NetworkAddress(ConstStrA address, natural port) {
			(*this) = resolve(address, port);
		}

		NetworkAddress(ConstStrA address, ConstStrA service) {
			(*this) = resolve(address, service);
		}

		///Creates object describes given network address
		/**
		 @param adr string address
		 @return pointer to object containing prepared address
		 @exception UnableToResolveAddressException cannot resolve address
		 */
		static NetworkAddress resolve(ConstStrA adr, natural port);
		static NetworkAddress resolve(ConstStrA adr, ConstStrA service);

		///Creates address useful to bind or connect to localhost
		/**
		 * @param port port number
		 * @param listen specifies listening address, useful to accept
		 *    connections. This is equivalent to specify _any_ address.
		 * if listen is false, returns address, which contains localhost 
		 *   as target address explicitly. That adress still can be used
		 *   for listening  at localhost interface
		 */
		 
		static NetworkAddress ipLocal( natural port, bool listen = true);

		static NetworkAddress ipRemote(ConstStrA address, natural port);


		bool equalTo(const NetworkAddress &other) const;

		bool isNil() const {return addr == nil;}

		NetworkAddress(PNetworkAddress addr):addr(addr) {}

		INetworkAddress *operator->() {return addr;}
		const INetworkAddress *operator->() const {return addr;}

	protected:
		PNetworkAddress addr;		


	};


	///Iterator through collector of streamed connections
	/** It virtually contains many of streams extracted from
	   underlying network layer. They can be extracted from incomming
	   connection, or can be requested for remote connection. 

	   This object act as iterator, where getNext() prepares and waits
	   for new connection and where peek() can be used to make
	   wait asynchronious, when this function returns nil, when 
	   connection is not ready yet.
	 */
	class NetworkStreamSource: public IteratorBase<PNetworkStream,
								NetworkStreamSource> {
	
	public:

		NetworkStreamSource(PNetworkStreamSource ss):handle(ss) {}
		/// PNetworkStreamSource
		/** 
		 * @param address Address specifies location, where create connection
		 * @param limitConnections How many connection can be extracted. 
				If parameter is naturalNull, iterator will able to
				extract unlimited count of connections
		 * @param limitWaiting How long will iterator wait for new connection
				If parameter is naturalNull, iterator will wait infinite
				time.
		 * @param streamTimeout default timeout for newly created streams
		 */
		NetworkStreamSource(NetworkAddress address,
				natural limitConnections = 1,
				natural limitWaiting = 30000,
				natural streamTimeout = naturalNull,
				StreamOpenMode::Type mode = StreamOpenMode::useAddress);

		///Creates stream source which listens on specific port
		/**
		 * @param port port number
		 * @param limitConnections maximum count of connection can be created by this source
		 * @param limitWaiting how long to wait on next connection in milliseconds
		 * @param streamTimeout default timeout for newly created streams
		 * @param localhost if true, port is opened for local connections only
		 */
		NetworkStreamSource(natural port,
				natural limitConnections = naturalNull,
				natural limitWaiting = naturalNull,
				natural streamTimeout = naturalNull,
				bool localhost = false);
		
		///Empty constructor
		NetworkStreamSource() {}
		NetworkStreamSource(NullType)  {}

		///Returns true, whether new connection can be prepared
		/** @retval true new connection can be prepared
		 *  @retval false reached limit, no more connections will be prepared
	     */
		bool hasItems() const;

		///Prepares or extracts new connection
		/** Function will wait for new connection time specified by
		  constructor, and if connection is not ready in time, throws
		  exception

		  @return smart pointer refers underlying object containing
		      descriptor of new connection. You have to
			  use this pointer to construct NetworkStreamOld
		  @exception NetworkTimeoutException thrown when connection is not
		      ready in time
		  */
		const PNetworkStream &getNext();


		///Retrieves local address of last extracted and taken connection
		/** 
		 For listening connections, returned object is equal to object
		 specified as address in constructor. For connecting connections,
		 returned address contains address of localhost and number of
		 used port for the connection
		 */
		NetworkAddress getLocalAddress() const;

		///Retrieves remote address of last extracted and taken connection
		/**
		 For listening connections, returned object contain address of
		 incoming connection. For connecting connections, returned
		 address contains object equal to address specified in the constructor
		 */
		NetworkAddress getRemoteAddress() const;

		PNetworkStreamSource getHandle() const {return handle;}

		bool equalTo(const NetworkStreamSource &other) const {
			return handle == other.handle;
		}

		///waits for new connection
		/**
		 * Infinite waiting
		 */
		void wait();

		///waits for new connection
		/**
		 * Waits for accepting new connection or finishing connect process.
		 * If object opened as active stream source (connecting), function
		 * causes, that connect process is being initiated now.
		 *
		 * @param timeout_ms time in miliseconds
		 * @retval true success
		 * @retval false failed due timeout
		 */
		bool wait(natural timeoutms);


	protected:

		PNetworkStreamSource handle;
		mutable PNetworkStream stream;

	};


	///Buffered bidirectional stream object.
	template<natural inputBufferSize = 0, natural outputBufferSize = inputBufferSize>
	class NetworkStream: public SeqFileInput, public SeqFileOutput {
	public:

		NetworkStream(PNetworkStream handle)
			:SeqFileInput(nil),SeqFileOutput(nil)
			,buffInput(handle.get())
			,buffOutput(handle.get())
			,handle(handle) {
			buffInput.setStaticObj();
			buffOutput.setStaticObj();
			SeqFileInput &i = *this;
			i = SeqFileInput(&buffInput);
			SeqFileOutput &o = *this;
			o = SeqFileOutput(&buffOutput);
			buffInput.setAutoflush(&buffOutput);
		}

		typedef BufferedInputStream<StreamBuffer<inputBufferSize> > BuffIn;
		typedef BufferedOutputStream<StreamBuffer<outputBufferSize> > BuffOut;

		PNetworkStream getHandle() {
			return handle;
		}

		~NetworkStream() {
			SeqFileInput &i = *this;
			i = SeqFileInput(nil);
			SeqFileOutput &o = *this;
			o = SeqFileOutput(nil);
		}
		IBufferedInputStream *getBufferedInputStream()  {
			return &buffInput;
		}
		IBufferedOutputStream *getBufferedOutputStream()  {
			return &buffOutput;
		}
		using SeqFileInput::hasItems;
		natural dataReady() const {
			return buffInput.dataReady();
		}

	protected:
		BuffIn buffInput;
		BuffOut buffOutput;
		PNetworkStream handle;
	private:
		NetworkStream(const NetworkStream &) {}
		NetworkStream &operator=(const NetworkStream &) {return *this;}

	};


	///Handles asynchronous watching state of network connections
	/**
	 * Object created using this class is able to watch state of network connections
	 * and send notifications on change of state.
	 *
	 */
	class NetworkEventListener {
	public:

		///watch for input data
		/** notifies, when some data are ready on the input */
		const static natural input = INetworkResource::waitForInput;
		///watch for output ready
		/** notifies, when there output is ready to receive some data */
		const static natural output = INetworkResource::waitForOutput;
		///watch for exception
		/** notifies about an exception on the stream */
		const static natural exception = INetworkResource::waitForException;
		///notify about timeout
		/** notifies when specified timeout ellapses */
		const static natural timeout = INetworkResource::waitForException << 1;

		NetworkEventListener():impl(INetworkServices::getNetServices().createEventListener()) {}


		class Request: protected INetworkEventListener::Request, public SharedResource {
		public:
			Request(PNetworkEventListener target,
					INetworkResource *rsrc,
					ISleepingObject *notify):target(target) {
				this->rsrc = rsrc;
				this->waitFor = rsrc->getDefaultWait();
				this->timeout_ms = naturalNull;
				this->notify = notify;
				this->reqNotify = 0;
			}

			Request &operator=(const Request &) {
				return SharedResource::makeAssign(*this);
			}

			///defines timeout for waiting
			/**
			 * @param tm_ms timeout in miliseconds. Default timeout is infinite.
			 * @return reference to request to chain additional attributes
			 */
			Request &timeout(natural tm_ms) {
				timeout_ms = tm_ms;return *this;
			}
			///specifies events
			/**
			 * @param mask combination of events to wait
			 * @return reference to request to chain additional attributes
			 */
			Request &events(natural mask) {
				waitFor = mask; return *this;
			}

			///removes all events which stops watching
			/**
			 * @return reference to request to chain additional attributes
			 */
			Request &erase() {
				waitFor = 0; return *this;
			}

			///watches for input only
			Request &forInput() {
				waitFor = input; return *this;
			}

			///watches for output only
			Request &forOutput() {
				waitFor = output; return *this;
			}

			///Sets notification object on request completion
			/** Because requests to network listener are send asynchronously, you can
			 * specify object, which will be notified after request is processed. This
			 * object MUST NOT be destroyed before notification arrives. If you want to
			 * implement timeouted waiting, you still need to keep object alive.
			 *
			 * @param ntf pointer to notificator
			 * @return reference to request allowing chaining
			 */
			Request &onCompletion(ISleepingObject *ntf) {
				this->reqNotify = ntf;
				return *this;
			}

			~Request() {
				if (!SharedResource::isShared()) {
						try {
							target->set(*this);
						} catch (...) {
							if (!std::uncaught_exception()) throw;
						}
				}
			}

			PNetworkEventListener target;

		};

		///Create registration request on stream resource
		/**
		 * @param rsrc resource to watch
		 * @param notify object notified about the event
		 * @return request object - can be modified by calling its methods
		 */
		Request operator()(INetworkResource *rsrc,ISleepingObject *notify) {
			return Request(impl,rsrc,notify);
		}
		Request operator()(NetworkStreamSource rsrc,ISleepingObject *notify) {
			return Request(impl,rsrc.getHandle().getMT(),notify);
		}

	protected:

		PNetworkEventListener impl;

	};


	class NetworkWaitingObject: public ISleepingObject,
								public IteratorBase<INetworkWaitingObject::EventInfo,NetworkWaitingObject>
	{
	public:

		typedef INetworkWaitingObject::EventInfo EventInfo;

		NetworkWaitingObject():impl(INetworkServices::getNetServices().createWaitingObject()) {}


		///Adds network resource to the object
		/**
		 * @param rsrc pointer to network resource to monitor. Pointer should be valid during monitoring
		 * @param waitFor mask of events that should be monitored
		 * @param timeout_ms how long in miliseconds will be resource monitored
		 *
		 * @note Function replaces same item identifed by rsrc.
		 */
		virtual void add(INetworkResource *rsrc, natural waitFor, natural timeout_ms) {
			impl->add(rsrc,waitFor,timeout_ms);
		}

		///Removes network resource from the monitor
		/**
		 *
		 * @param rsrc resource to remove. If resource not exists, function returns without error
		 */
		virtual void remove(INetworkResource *rsrc) {
			impl->remove(rsrc);
		}

		///Waits and retrieves event
		/**
		 * @return event information.
		 *
		 * @note function will not return until any event is recorded or another thread calls
		 *  function wakeUp().
		 */
		virtual const EventInfo &getNext() {
			return impl->getNext();
		}
		///Waits and retrieves event - doesn't removes event from the container
		/**
		 * @return event information.
		 *
		 * @note function will not return until any event is recorded or another thread calls
		 *  function wakeUp().
		 */
		virtual const EventInfo &peek() const {
			return impl->peek();
		}
		///Returns true, if object conatins any network resource
		/**
		 * @retval true there are resources
		 * @retval false no more resources
		 */
		virtual bool hasItems() const {
			return impl->hasItems();
		}


		///Perform sleeping while monitoring inserted resources
		/**
		 * @param milliseconds milliseconds to wait
		 * @retval true sleeping successful, no interruption
		 * @retval false there is untaken event. It must be took by getNext() function before
		 * sleep can be successful.
		 *
		 * @note function replaces Thread::sleep(). Thread's sleep function cannot perform monitoring
		 * selected resources. When function returns true, you have to iterate and give all resources
		 * that received event during sleeping. Otherwise, sleep will never start again.
		 *
		 * You can specify naturalNull as argument to perform infinite sleeping. Note that
		 * function getNext() also does infinite sleeping, if there is no resource signaled. If there
		 * is resource with timeout less than specified milliseconds, function sleep returns false
		 * after timeout elapses (because timeout is also interruption).
		 */
		virtual bool sleep(natural milliseconds) {
			return impl->sleep(milliseconds);
		}


		virtual void wakeUp(natural reason) throw() {
			return impl->wakeUp(reason);
		}



	protected:
		PNetworkWaitingObject impl;
	};

	//----------------------- IMPLEMENTATION -----------------------
	///Interface to access network address object


	///Converts instance to the string representation
	inline StringA NetworkAddress::asString(bool resolve) const {
		if (addr == nil) 
			throwNullPointerException(THISLOCATION);
		return addr->asString(resolve);
	}

	inline NetworkAddress NetworkAddress::resolve( ConstStrA adr, natural port_def )
	{
		return NetworkAddress(INetworkServices::
					getNetServices().createAddr(adr,port_def));
	}
	inline NetworkAddress NetworkAddress::resolve( ConstStrA adr, ConstStrA service )
	{
		return NetworkAddress(INetworkServices::
					getNetServices().createAddr(adr,service));
	}

	inline NetworkAddress NetworkAddress::ipLocal(  natural port , bool listen)
	{
		if (listen) return NetworkAddress(INetworkServices::
			getNetServices().createAddr(nil,port));
		else return NetworkAddress(INetworkServices::
			getNetServices().createAddr(INetworkAddress::getLocalhost(),port));
	}

	inline NetworkAddress NetworkAddress::ipRemote(ConstStrA address, natural port)
	{	
		return NetworkAddress(INetworkServices::
			getNetServices().createAddr(address,port));

	}

	inline bool NetworkAddress::equalTo( const NetworkAddress &other ) const
	{
		if (other.addr == nil) return addr == nil;
		return other.addr->equalTo(*addr);
	}

	inline NetworkStreamSource::NetworkStreamSource( 
		NetworkAddress address, natural limitConnections /*= 1*/, 
		natural limitWaiting /*= 30000*/ ,
		natural streamTimeout /* = naturalNull */,
		StreamOpenMode::Type mode /*= StreamOpenMode::useAddress*/)
	{
		handle = INetworkServices::getNetServices().createStreamSource(
			address.getHandle(),mode,limitConnections,limitWaiting,streamTimeout);
	}
	inline NetworkStreamSource::NetworkStreamSource(
		natural port, natural limitConnections /*= 1*/,
		natural limitWaiting /*= 30000*/ ,
		natural streamTimeout /* = naturalNull */,
		bool localhost /*=false*/)
	{
		handle = INetworkServices::getNetServices().createStreamSource(
			NetworkAddress::ipLocal(port,!localhost).getHandle(),
			StreamOpenMode::passive,
			limitConnections,
			limitWaiting,streamTimeout);
	}

	inline bool NetworkStreamSource::hasItems() const
	{
		stream = nil;
		return handle->hasItems();
	}


	inline const PNetworkStream &NetworkStreamSource::getNext()
	{
		stream = nil;
		return (stream = handle->getNext());
	}


	inline NetworkAddress NetworkStreamSource::getLocalAddress() const
	{
		stream = nil;
		return handle->getLocalAddr();
	}

	inline NetworkAddress NetworkStreamSource::getRemoteAddress() const
	{
		stream = nil;
		return handle->getPeerAddr();
	}

	inline void NetworkStreamSource::wait() {
		stream = nil;
		handle->wait();
	}

	inline bool NetworkStreamSource::wait(natural timeoutms) {
		stream = nil;
		return handle->wait(handle->getDefaultWait(),timeoutms) != 0;
	}



}






#endif /* NETIO_H_ */
