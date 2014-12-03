#pragma once
#include "syncPt.h"
#include "..\base\containers\linkedList.h"
#include "exceptions\timeoutException.h"

namespace LightSpeed {


	///Implementation of very simple tool to exchange objects between two threads
	/**
	  Standard pipes can exchange only bytes. If you need to transfer one object to another thread
	  you will need ObjectPipe. ObjectPipe is template, so you can specify which object will be subject of
	  exchange. 

	  ObjectPipe implements one way of transfer. To implement full duplex, you will need two instances of ObjectPipe.
	  
	  ObjectPipe is MT Safe

	  To use ObjectPipe, share its instance between two threads. One thread writes objects into it using write() function.
	  Other thread reads objects from it using read() function. There can be multiple readers and multiple writers

	  There is also way to make asynchronous waiting. It means that thread can perform another actions while pipe
	  is waiting for an object. This also allows to wait for multiple pipes. For more details, read description of readAsync()
	  */
	template<typename T>
	class ObjectPipe: public SyncPt {
	public:

		///Use this class to implement asynchronous wait
		/** Class acts as SyncPt's slot and also as Optional<T> object
		   When value arrives, you can read value using Optonal methods.
		   Note that during waiting, you should not access the object. You have to remove it first using remove()

		   @see readAsync;
		   */
		class AsyncStorage: public Slot, public Optional<T> {
		public:
			void reset() {
				if (Slot::reset()) Optional<T>::unset();
			}

		};


		///Reads object from ObjectPipe. 
		/** If there is no object ready, function waits specified count of miliseconds
		 *
		 * @param tm timeout to wait on object
		 * @return object which has been written by other side
		 * @exception TimeoutException
		 */
		T read(Timeout tm = naturalNull);

		///Writes object to ObjectPipe
		/**
		  @param item reference to write object

		  @note object is copied through interface. You should transfer pointers to achieve best performance. If you
		  are using reference counting, always switch to MT safe reference counting */
		void write(const T &item);


		///Initializes asynchronous reading
		/** 
		 * @param storage reference to instance of AsyncStorage. It will receive
		   object once it is written to the queue. Instance MUST exist for whole period of waiting. If you
		   need to drop waiting, call remove().

		   See SyncPt::Slot for aditional details
	     */
		void readAsync(AsyncStorage &storage);

	protected:

		using SyncPt::add;


		typedef LinkedList<T> Pipe;
		Pipe pipe;

		FastLock lock;

		class NotifyFn {
		public:
			NotifyFn(const T &val):val(val) {}
			void operator()(SyncPt::Slot *p) {
				AsyncStorage &storage = static_cast<AsyncStorage &>(*p);
				storage.init(val);
			}

		protected:
			const T &val;
		};

	};


	template<typename T>
	void LightSpeed::ObjectPipe<T>::write( const T &item )
	{
		Synchronized<FastLock> _(lock);
		if (!SyncPt::notifyOne(NotifyFn(item)))
			pipe.add(item);
	}


	template<typename T>
	T LightSpeed::ObjectPipe<T>::read(Timeout tm) 
	{
		AsyncStorage store;
		readAsync(store);
		SyncPt::wait(store,tm);
		if (store) {
			return store.get();
		} else {
			throw TimeoutException(THISLOCATION);
		}
	}

	template<typename T>
	void LightSpeed::ObjectPipe<T>::readAsync( AsyncStorage &storage )
	{
		Synchronized<FastLock> _(lock);
		if (pipe.empty()) {
			SyncPt::add(storage);
		} else {
			storage.init(pipe.getFirst());
			pipe.eraseFirst();
			storage.notify(0);
		}
	}


}