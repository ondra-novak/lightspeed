/** @file
 * declaration of supporting interface for netio.h */
#pragma once
#include "../memory/refCntPtr.h"
#include "fileio_ifc.h"
#include "../../mt/sleepingobject.h"

struct sockaddr;

namespace LightSpeed {


	class INetworkAddress;
	typedef RefCntPtr<INetworkAddress> PNetworkAddress;
	class INetworkStreamSource;
	typedef RefCntPtr<INetworkStreamSource> PNetworkStreamSource;
	class INetworkStream;
	typedef RefCntPtr<INetworkStream> PNetworkStream;
	class INetworkDatagram;
	typedef RefCntPtr<INetworkDatagram> PNetworkDatagram;
	class INetworkEventListener;
	typedef RefCntPtr<INetworkEventListener> PNetworkEventListener;
	class INetworkWaitingObject;
	typedef RefCntPtr<INetworkWaitingObject> PNetworkWaitingObject;
	class INetworkResource;
	class ISleepingObject;

	///Listens network connections for events
	/**
	 * Can be configured to listen events on various streams. You can define, which
	 * events will be watches and which objects will be notified about these events
	 *
	 * @note listener starts new thread internally which handles all operations.
	 *
	 *
	 * @see NetworkEventListener
	 */
	class INetworkEventListener: public RefCntObj {
	public:

		///Request to listener
		struct Request {
			///Pointer to network resource which will be monitored for events
			/** TODO: Some implementation doesn't increase reference of the resource so caller
			  must track instance separately. In the future version, network listener must
			  keep one reference as soon as it holds this pointer */
			INetworkResource *rsrc;
			///Pointer to an ISleepingObject which will be woken up after event is recorded.
			/** event is carried on reason
			 * 			 * */
			ISleepingObject *observer;
			///Specifies mask of events to wait. See INetworkResource for events
			/** use 0 to disable monitoring */
			natural waitFor;
			///Specifies timeout.
			/** if timeout ellapses, waiting object is notified with reason = 0
			 *  use naturalNull to set infinity timeout */

			natural timeout_ms;

			///Pointer to object which receives notification when request is processed by listener
			ISleepingObject *reqNotify;

			Request () {}
			Request (INetworkResource *rsrc, ISleepingObject *observer, natural waitFor, natural timeout_ms,ISleepingObject *reqNotify)
				:rsrc(rsrc),observer(observer),waitFor(waitFor),timeout_ms(timeout_ms),reqNotify(reqNotify) {}

		};

		///sets new monitoring
		/** @param request request
		 *
		 * @note Setting monitoring on resource is not pernament. Monitoring is disabled
		 * on first recorder event and must be re-enabled by new call of method set(). This not
		 * bug, this is feature.
		 */
		virtual void set(const Request &request) = 0;

		void add(INetworkResource *rsrc, ISleepingObject *observer, natural waitFor, natural timeout_ms,ISleepingObject *reqNotify = 0) {
			set(Request(rsrc,observer,waitFor,timeout_ms,reqNotify));
		}

		void remove(INetworkResource *rsrc, ISleepingObject *observer, ISleepingObject *reqNotify = 0) {
			set(Request(rsrc,observer,0,naturalNull,reqNotify));
		}


		virtual ~INetworkEventListener() {}

	};


	struct NetworkWaitingObjectItem {
		///Identifies network resource which recorded an event.
		/** Pointer can be NULL in case that waiting has been interrupted by wakeUp function. */
		INetworkResource *rsrc;
		///Identifies event
		/**Contain mask of events that has been recorded. If value is 0, waiting timeouted.
		 * If rsrc is NULL, variable contains reason carried by wakeUp function */
		natural event;
	};
	///New alternative to INetworkEventListener - allows monitor multiple network resources
	/**
	 * Object implements ISleepingObject and can be used to waiting on any registeres network
	 * events on various network interface and waiting can be also interrupted by calling
	 * method wakeUp() on ISleepingObject
	 *
	 * Object works as iterator of items which are received events. Once event is retrieved from
	 * this object, associated network resource is removed, and must be added again
	 *
	 *
	 *
	 */
	class INetworkWaitingObject: public ISleepingObject,
							     public RefCntObj,
								 public IInterface,
								 public IteratorBase<NetworkWaitingObjectItem, INetworkWaitingObject> {
	public:

		typedef NetworkWaitingObjectItem EventInfo;

		///Adds network resource to the object
		/**
		 * @param rsrc pointer to network resource to monitor. Pointer should be valid during monitoring
		 * @param waitFor mask of events that should be monitored
		 * @param timeout_ms how long in miliseconds will be resource monitored
		 *
		 * @note Function replaces same item identifed by rsrc.
		 */
		virtual void add(INetworkResource *rsrc, natural waitFor, natural timeout_ms) = 0;

		///Removes network resource from the monitor
		/**
		 *
		 * @param rsrc resource to remove. If resource not exists, function returns without error
		 */
		virtual void remove(INetworkResource *rsrc) = 0;

		///Waits and retrieves event
		/**
		 * @return event information.
		 *
		 * @note function will not return until any event is recorded or another thread calls
		 *  function wakeUp().
		 */
		virtual const EventInfo &getNext() = 0;
		///Waits and retrieves event - doesn't removes event from the container
		/**
		 * @return event information.
		 *
		 * @note function will not return until any event is recorded or another thread calls
		 *  function wakeUp().
		 */
		virtual const EventInfo &peek() const = 0;
		///Returns true, if object conatins any network resource
		/**
		 * @retval true there are resources
		 * @retval false no more resources
		 */
		virtual bool hasItems() const = 0;


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
		virtual bool sleep(natural milliseconds) = 0;

	protected:


	};


	class INetworkAddress: public RefCntObj, public IInterface {
	public:
		virtual ~INetworkAddress() {}
		
		/// Returns text representation of network address
		virtual StringA asString(bool resolve = false) = 0;

		virtual bool equalTo(const INetworkAddress &other) const = 0;

		///Use this function to create map of address
		/**
		 * @param other other address to compare with
		 *
		 * @return true if addresses are in order, false if they are out of order.
		 *
		 * @note that ordering has no meaning, you can only use it to order item
		 * in a map container. You should not compare addresses of different type, otherwise
		 * you will gain unexpected results.
		 */
		virtual bool lessThan(const INetworkAddress &other) const = 0;

		/// Retrieves RAW representation of address
		/**
		 * In most of cases, it retrieves SOCKADDR representation of the address
		 * @param buffer pointer to buffer, which receives data. If buffer is
		 * 	NULL, function only determines required size of buffer. In this case
		 * 	parameter size is ignored
		 * @param size size of buffer. If buffer is NULL, paramater is ignored
		 * @return count of bytes copied into the buffer
		 */
		virtual natural getSockAddress(void *buffer, natural size) const = 0;

		///Contains address of localhost
		/**
		 * Always use this variable instead typing "localhost" or "127.0.0.1"
		 * because content of variable may depend on platform specification
		 */
		 
		static ConstStrA getLocalhost();
	};



	///Common interface for all network resources
	class INetworkResource: public virtual IRefCntInterface {
	public:

		///Handler called during wait
		/**
		 * @note handler is not called, if resource is inserted into INetworkEventListener
		 */
		class WaitHandler {
		public:

			///Implements user waiting
			/**
			 *
			 * @param resource Which resource is waiting - this allows to create central wait handlers
			 * @param waitFor what event to wait
			 * @param timeout how long in miliseconds
			 * @return wait event detected on the resource, or waitTimeout, if timeouted
			 */
			virtual natural wait(const INetworkResource *resource, natural waitFor, natural timeout) const {
				return resource->doWait(waitFor,timeout);
			}
		};

		///waiting timeouted
		static const natural waitTimeout = 0;
		///waiting for input (reading)
		static const natural waitForInput = 1;
		///waiting for output (writing)
		static const natural waitForOutput = 2;
		///waiting for network exception
		static const natural waitForException = 4;
		///use default waiting for resource - not return value
		static const natural waitDefault = 0;

		///Retrieves default waiting operation
		/**
		 * To simplify interface, every network resource must
		 * supply default valid waiting, while this is not specified.
		 *
		 * Streams should return waitForInput, because this is most
		 * used waiting
		 *
		 * Connecting stream source should return waitForOutput|waitForException, because
		 * only this state means, that connection has been estabilished or rejected
		 *
		 * Listening stream source should return waitForInput, because only
		 * valid operation
		 *
		 *
		 *
		 * @return
		 */
		virtual natural getDefaultWait() const = 0;

		///Sets user wait handler
		/**
		 * handler is called, when blocking wait operation is requested.
		 *   Object can implement own version of waiting.
		 * @param handler pointer to handler.
		 */
		virtual void setWaitHandler(WaitHandler *handler) = 0;

		virtual WaitHandler *getWaitHandler() = 0;

		///Sets default timeout for all waiting operations
		/**
		 * Timeout is used while reading or writting while socket is not
		 * ready for specified operation.
		 *
		 * @param time_in_ms timeout in miliseconds
		 * 		- use naturalNull to wait infinitive time
		 */
		virtual void setTimeout(natural time_in_ms) = 0;

		///retrieves current timeout
		virtual natural getTimeout() const = 0;

		///Waits for data on the network resource
		/**
		 * @param waitFor specifies combination of following flags
		 *  @arg @c waitForInput operation is waiting for any input
		 *  @arg @c waitForOutput operation is waiting for finish output
		 *  @arg @c waitForException operation is waiting for exception on the network (for example OOB)
		 * @param timeout timeout to wait
		 * @retval waitTimeout wait timeouted
		 * @retval waitForInput input data are ready
		 * @retval waitForOutput output buffer has some space to write data
		 * @retval waitForException there is network exception reported
		 * @note As return value, it can be any combination of flags, when
			 multiple conditions has been met. This excludes waitTimeout, which
			 will always appear alone (and cannot be tested using &),
			 which means, that timeout elapses before any condition has been met.
		 * @exception any may throw any exception
		 */
		virtual natural wait(natural waitFor, natural timeout) const = 0;


		natural wait(natural waitFor) const {
			return wait(waitFor, getTimeout());
		}

		natural wait() const {
			return wait(0);
		}
	protected:
		///Performs wait without notifying the waithandler.
		/**
		 * Function is useful for wait handler to call orginal waiting
		 * @param waitFor
		 * @param timeout
		 * @return
		 */
		virtual natural doWait(natural waitFor, natural timeout) const = 0;

		friend class WaitHandler;


	};


	typedef RefCntPtr<INetworkAddress> PNetworkAddress;

	class INetworkStreamSource: public INetworkResource {
	public:
		virtual ~INetworkStreamSource() {}

		///true if additional stream is available
		/**
		 * @retval true there is stream available. Listening socket is able listen
		 *   more connections. Connecting socket is able to create additional connection
		 *   to the peer
		 * @retval false no more streams avaialable
		 */
		virtual bool hasItems() const = 0;
		///Retrieves next stream
		/**
		 * @return smart pointer to ISeqFileHandle interface. If connection not
		 * ready yet, function will block, until connection is ready
		 */
		virtual PNetworkStream getNext() = 0;

		///Retrieves address of peer of last extracted stream
		/**
		 * When called on listening socket, returns address of incomming connection.
		 * When called on connecting sockeg, returns address of target specified during
		 * connect
		 */
		virtual PNetworkAddress getPeerAddr() const = 0;

		///Retrieves local address
		/** 
		 * @return address of mother socket.
		 */
		virtual PNetworkAddress getLocalAddr() const = 0;

	};



	class INetworkStream: public IInOutStream, public INetworkResource {
	public:


	};

	///Network datagram - stream used to access datagram data
	/**
	 * Contains received packed and also place to store data for send
	 * When stream is read, it returns received data. When stream
	 * is written, it stores data to send.
	 *
	 * Note that interface IRndFileHandle cannot be used to read written
	 * data.
	 *
	 * Every datagram may keep address where datagram will be send
	 * before it is destroyed. It often contain address of side,
	 * which sent this datagram to this computer
	 */
	class INetworkDatagram: public IInOutStream, public IRndFileHandle {
	public:

		///Receives target for this datagram.
		/**
		 * @return pointer to object containing target address
		 */
		virtual PNetworkAddress getTarget() = 0;

		///Checks, whether address of packed is equal to specified address
		/**
		 * @param address address to check
		 * @retval false not equal
		 * @retval true is equal
		 *
		 * @note checking address can be slightly faster in compare to
		 * calling getTarget to receive current address and making
		 * comparsion with other address. Function getTarget() may
		 * be delayed while it creates object. Sometime it may
		 * need to call kernel, This function only compares
		 * two memory places without need to create anything.
		 */
		virtual bool checkAddress(PNetworkAddress address) = 0;

		///Immediatelly sends datagram to the target
		/**
		 * Function sends datagram to the target and resets target to
		 * prevent packet duplication.
		 *
		 * Note that you have to flush any cache in the chain before
		 * function is called;
		 */
		virtual void send() = 0;

		///Immediatelly sends datagram to the specified target
		/**
		 *
		 * @param target address of datagram target
		 *
		 * Function sends datagram to the target and resets target to
		 * prevent packet duplication.
		 *
		 * Note that you have to flush any cache in the chain before
		 * function is called;
		 */
		virtual void sendTo(PNetworkAddress target) = 0;
		///Removes any target address from the datagram
		/**
		 * Once datagram doesn't have target specified, it wouldn't be
		 * send automatically with destructor
		 */
		virtual void resetTarget() = 0;
		///Rewinds sequentional stream to allow read data again
		virtual void rewind() = 0;
		///Removes written data
		/** If packet is emptied, it is not send */
		virtual void clear() = 0;

		///Receives datagram number
		/**
		 * Function helps to implement acknowledge scheme. Every datagram can
		 * have unique ID, which can be later used to acknowledge delivered
		 * datagrams. This ID is not send, you have to include it manually. But
		 * everytime new datagram is created, new UID is generated
		 *
		 * @return datagram UID
		 *
		 * @note UIDs don't need to be generated continuously. There can
		 * be holes between numbers. To reduce holes, function generates UID
		 * only if asked for first time for every datagram, and for every
		 * sent datagram when UID has not been asked. Function is not MT safe.
		 * It always generates increasing sequence, until UID is overflowed
		 */
		virtual natural getUID() const = 0;

		using IInOutStream::read;
		using IInOutStream::write;
		using IRndFileHandle::read;
		using IRndFileHandle::write;

		///Retrieves reference to current written data
		/** Reference is read only. It is tent to be used for various checksum calculators before
		 * final checksum is appended and datagram deparded.
		 *
		 * @return const reference to written data
		 */
		virtual ConstStringT<byte> peekOutputBuffer() const = 0;

		virtual ~INetworkDatagram() {}
	};

	///Object represents opened datagram connection associated with specifed port
	/**
	 * To create object which implements this interface, call
	 * 	INetworkServices::createDatagramSource
	 */
	class INetworkDatagramSource: public INetworkResource {
	public:

		///Waits and receives datagram from the datagram source
		/**
		 * @return received packet. Packet contains received data and
		 * also can be used to write reply which is back to the client
		 */
		virtual PNetworkDatagram receive() = 0;
		///Creates empty datagram with unbound target address
		/**
		 * @return empty packet
		 */
		virtual PNetworkDatagram create() = 0;


		///Creates empty datagram with bound target address
		/**
		 * @param adr target address
		 * @return empty packet
		 */
		virtual PNetworkDatagram create(PNetworkAddress adr) = 0;


		///Sets UID for next datagram
		/**
		 * @param id new id of next datagram
		 *
		 * @note datagram id is incremented only if it is used. This
		 * situation is defined by calling INetworkDatagram::getUID().
		 * Otherwise, UID is not changed
		 *
		 */
		virtual void setUID(natural id) = 0;


	};


	typedef RefCntPtr<INetworkStreamSource> PNetworkStreamSource;
	typedef RefCntPtr<INetworkDatagramSource> PNetworkDatagramSource;


	class ISleepingObject;

	typedef RefCntPtr<INetworkEventListener> PNetworkEventListener;

	namespace StreamOpenMode {
		enum Type {
			///Mode depends on address definition
			/** if address contains name of foreign machine
			  active mode is used, otherwise, passive mode is used
			  */
			useAddress,

			///Stream is created by actively connecting foreign machine
			active,
			
			///Stream is created after another machine connect tho this machine			
			passive, //passive		

		};
	}

	///Contains network services
	class INetworkServices: public IInterface{
	public:
		virtual ~INetworkServices() {}



		///Creates network stream source
		/**
		 * Stream source object is responsible to create network stream. Stream source can be
		 * active - everytime it is requested to create stream, it makes connection to the specified
		 * address. Stream source can be passive - will listen and wait for new incoming connection.
		 * Once connection is estabilished, new stream object is created and can be used to communicate
		 * with other side.
		 *
		 * @param address address associated with the stream
		 * @param mode specifies whether streams connecting or listening
		 * @param count count of streams available in returned object. It can be 1 for
		 *   connecting streams or naturalNull for listening streams. But you can use another values
		 *   depend on what do you need
		 * @param timeout How long, in milliseconds, object will wait for connection. This
		 *   can be naturalNull to make infinite waiting
		 * @param streamDefTimeout Default timeout for newly created streams
		 * @return object useful to create new streams.
		 */
		virtual PNetworkStreamSource createStreamSource(
				PNetworkAddress address,
				StreamOpenMode::Type  mode = StreamOpenMode::useAddress,
				natural count = 1,
				natural timeout = naturalNull,
				natural streamDefTimeout = naturalNull) = 0;


		///Creates network datagram source
		/** Datagram-source is object that creates datagrams for specified
		 * network port. You can use datagram to send piece of information to
		 * the other datagram-source located on another computer somewhere in the
		 * network. You can also receive sent datagrams and reads informations
		 * stored insided.
		 *
		 * @param port specifies port where open the datagram source. If
		 * 	  zero sepcified (default value) function picks first unused port.
		 * @param timeout specifies timeout to wait for datagram by function
		 * 	 INetworkDatagramSource::receive()
		 * @return reference to object implementing datagram source
		 */
		virtual PNetworkDatagramSource createDatagramSource(
						natural port = 0, natural timeout = naturalNull) = 0;
	
		 
		virtual PNetworkAddress createAddr(ConstStrA remoteAddr, natural Port) = 0;
		virtual PNetworkAddress createAddr(ConstStrA remoteAddr,ConstStrA service) = 0;

		///Creates address from serialized form placed in memory
		/**
		 * @param sock_addr pointer to serialized network address. It can correspond
		 *   with sockaddr structure
		 * @param len length of the address
		 * @return address
		 *
		 * @see INetworkAddress::getSockAddress
		 */
		virtual PNetworkAddress createAddr(const void *sock_addr, natural len) = 0;

		///Creates object that handles waiting for events on network resources (async)
		/**This object uses standalone thread to monitor resources
		 *
		 * @return pointer to listener
		 */
		virtual PNetworkEventListener createEventListener() = 0;

		///Creates object that handles waiting for events on network resources (sync)
		/**
		 * New alternative doesn't uses thread, but acts as ISleepingObject. This allows to
		 * wait on network events and other events generated by other threads. Object can be
		 * used instead of Thread::sleep() function while it waiting for network event.
		 * @return
		 */
		virtual PNetworkWaitingObject createWaitingObject() = 0;


		///Gets global singleton for network services
		static INetworkServices &getNetServices();

		///Sets new global singleton for newtwork services
        static void setIOServices(INetworkServices *newServices);

	};

	///retrieves socket from the stream
	/** use dynamic_cast on INetworkStream to access this interface */
	class INetworkSocket {
	public:

		///Retrieves socket
		/**
		 *
		 * @param index index of socket - need for multisocket resources
		 * @return socket ID, -1 in case where socket is not set (in singlesocket
		 * resource, function returns -1 if index is not zero
		 */
		virtual integer getSocket(int index) const = 0;

		virtual ~INetworkSocket() {}
	};

	///Address extensions 
	/** works for some platforms. Interface can be retrieves asking
	  object which implementing INetworkAddress.
	
	*/
	class INetworkAddrEx {
	public:

		///Enables SO_REUSEADDR
		/**
		 * @param bool enable enables the flag
		 * @retval true succeed
		 * @retval false probably not supported by the object
		 */
		virtual bool enableReuseAddr(bool enable) = 0;

		///Retrieves port number. 
		/** You need this function to receive port number for locally created servers */
		virtual natural getPortNumber() const = 0;

		virtual ~INetworkAddrEx() {}

	};


	///Allows to create address for TCP/IP in specified version
	/** Ask INetworkServices for this interface */
	class INetworkServicesIP {
	public:

		///Defines IP version
		enum IPVersion {
			///use any available version (default)
			/** Passive connections are opened on both protocols */
			ipVerAny,
			///use IPv4 only
			ipVer4,
			///use IPv6 only
			ipVer6
		};

		///
		virtual PNetworkAddress createAddr(ConstStrA remoteAddr, 
			natural Port, IPVersion version) = 0;
		virtual PNetworkAddress createAddr(ConstStrA remoteAddr,
			ConstStrA service, IPVersion version) = 0;

		///Retrieves IP version availability
		/**
		 * @retval ipVerAny both IPv4 and IPv6 are available
		 * @retval ipVer4 only IPv4 is available
		 * @retval ipVer6 only IPv6 is available
		 */
		 
		virtual IPVersion getAvailableVersions() const = 0;

		///Retrieves name of localhost for specified version
		/**
		 * @param version of network. If you use ipVerAny, result
		 *   depends on IPv6 support on current computer. For
		 *   support both version, IPv6 has precedence
		 */
		 
		virtual ConstStrA getLoopbackAddr(IPVersion ver = ipVerAny) const = 0;

		virtual ~INetworkServicesIP() {}
	};

	///Allows to create named pipe that works similar as network stream
	/** named pipes can be created localy or remotely. Working with named is similar to working with
        network streams. Note that not all implementations can support all features with 
        named pipes. To receive instance of this interface, use getIfc on INetworkService object */
	class INamedPipeServices {
	public:

	   enum PipeMode {
			///open pipe for read only
			pmRO,
			///open pipe for write only
            pmWO,
			///open pipe for both read and write mode
			pmRW
	   };

		
	   ///Creates named pipe stream source
	   /**
        @param server set true to work as server, false to work as client
        @param pipeName name of pipe. Use without path to achieve platform independence
        @param mode specify one of pipe mode
		@param specify number of instances. Default value means unlimited count
		@param securityDesc string is used to specify security description for that pipe. This argument
              is complete under control by active implementation. Default value - empty string - means
			  to apply default security description. Under windows, you can use following keywords 
                  '@normal' - reserved for hight level process to allow open pipe to communicate with
	                          processes on normal level.
                  '@low'    - reserved for high or normal level process to allow open pipe to communicate with
                              processes on low level
                  '@sandbox' - create pipe to the sandbox level. Only sandboxed processes can connect this pipe
        @param connectTimeout how long to wait for connection before exception is thrown
        @param streamDefTimeout specified timeout value for newly created streams
	    @return returns pointer to object that can be used to connect named pipe or create new named pipe server
		*/
        
	   virtual PNetworkStreamSource createNamedPipe(
			bool server, 
			ConstStrW pipeName,
			PipeMode mode, 
			natural maxInstances = naturalNull,
			ConstStrA securityDesc = ConstStrA(),
			natural connectTimeout = naturalNull,
			natural streamDefTimeout = naturalNull
		) = 0;

	};


	template<typename Base>
	class NetworkResourceCommon: public Base {
	public:

		NetworkResourceCommon():defTimeout(naturalNull) {}
		virtual void setWaitHandler(INetworkResource::WaitHandler *handler) {
			waitHandler = handler;
		}
		virtual INetworkResource::WaitHandler *getWaitHandler() {
			return waitHandler;
		}
		virtual void setTimeout(natural time_in_ms) {
			defTimeout = time_in_ms;
		}
		virtual natural getTimeout() const {
			return defTimeout;
		}
		virtual natural wait(natural waitFor, natural timeout) const {
			if (waitHandler == nil) return this->doWait(waitFor,timeout);
			else return waitHandler->wait(this,waitFor,timeout);
		}
		
		using Base::wait;

	protected:
		Pointer<INetworkResource::WaitHandler> waitHandler;
		natural defTimeout;
	};


}
