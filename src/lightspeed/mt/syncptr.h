/*
 * ptrlock.h
 *
 *  Created on: 8.5.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_PTRLOCK_H_
#define LIGHTSPEED_MT_PTRLOCK_H_


namespace LightSpeed {

	///Simple synchronization using pointer
	/**
	 * Volatile pointer can be set only if it is NULL on creation of this object.
	 * Destructor resets pointer back to the NULL.
	 */
	template<typename T>
	class SynchronizedPtr {
	public:

		SynchronizedPtr( T *volatile& ptr, T *value);
		SynchronizedPtr(T *volatile & ptr, T *value, bool &success);
		~SynchronizedPtr();

	protected:
		 T *volatile & lockPtr;
		T *value;
	};

}

template<typename T>
LightSpeed::SynchronizedPtr<T>::SynchronizedPtr( T *volatile & ptr, T *value)
	:lockPtr(ptr),value(0)
{
	if (lockCompareExchangePtr(&lockPtr,(T *)0,value) != 0)
		throw SynchronizedExceptionT<SynchronizedPtr<T> >(THISLOCATION);
	this->value = value;
}



template<typename T>
LightSpeed::SynchronizedPtr<T>::SynchronizedPtr(T *volatile & ptr, T *value, bool & success)
	:lockPtr(ptr),value(0)
{
	if (lockCompareExchangePtr(&lockPtr,0,value) != 0)
		success = false;
	else {
		this->value = value;
		success = true;
	}
}



template<typename T>
LightSpeed::SynchronizedPtr<T>::~SynchronizedPtr()
{
	lockCompareExchangePtr(&lockPtr,value,(T *)0);
}

  // namespace LightSpeed
#endif /* LIGHTSPEED_MT_PTRLOCK_H_ */
