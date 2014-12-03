#ifndef _LIGHTSPEED_BASE_MEMORY_COMPTR_
#define _LIGHTSPEED_BASE_MEMORY_COMPTR_

#pragma once
#include "pointer.h"
namespace LightSpeed {

///Implements simplified Com+ pointer
/** It tracks references of single COM+ object. It also does automatic conversion between objects of various types using QueryInterface
	@tparam T interface name 
*/

template<typename T>
class ComPtr: public Pointer<T> {
public:
	///Construct uninitialized pointer (zeroed)
	ComPtr() {}
	///Constructs ComPtr using raw pointer
	/**@param p raw pointer. 
	 * @note Constructor expects, that pointer has already increased reference count, as most COM functions do. If you want to 
	   construct from raw pointer that has not increased reference, use importPtr() function
	   */
	ComPtr(T *p):Pointer<T>(p) {}

	///Constructs ComPtr using ComPtr of another interface
	/**
	 * @param p another ComPtr. Function handles references correctly, no extra action is need
	 */
	template<typename P>
	explicit ComPtr(ComPtr<P> p):Pointer<T>(cast(p->ptr)) {}

	///Empty pointer
	ComPtr(Null_t):Pointer<T>(nil) {}

	///Destructor releases the reference
	~ComPtr() {
		if (this->ptr) this->ptr->Release();
	}


	///Assignment operator
	ComPtr &operator=(const ComPtr &other) {
		if (this != &other && this->ptr != other.ptr) {
			other.ptr->AddRef();
			this->ptr->Release();
			this->ptr = other.ptr();
		}
		return *this;
	}

	///Exports pointer to be used in raw form
	/** 
	 * Exports pointer and increases reference count. If you returning raw pointers from library functions, they should always have increased reference count
	 * Using this method, you can achieve this requirement
	 */
	T *exportPtr() const {if (this->ptr) this->ptr->AddRef();return this->ptr;}
	
	///Imports raw pointer which was returned without increased referenced count. 
	/**
	@param ptr raw ptr;
	@return object ComPtr
	@note function increases reference count of the ptr
	*/
	static ComPtr importPtr(T *ptr) {
		if (ptr) ptr->AddRef();
		return ComPtr(ptr);
	}

	///Initialize pointer and returns pointer to it
	/** Many functions returns pointer through argument, using pointer to pointer agrument. 
	    This function prepares object to be able to receive pointer from function passed as argument. If pointer contains and object, it is released
		@return pointer to raw pointer inside of this object. You can pass return value to function which will create new object and set address to this object
		through that pointer. Function expects, that new object has increased reference count 

		*/
	void **initVoid() {
		if (this->ptr) ptr->Release();
		this->ptr = 0;
		return (void **)&ptr;
	}

	///Initialize pointer and returns pointer to it
	/** Many functions returns pointer through argument, using pointer to pointer agrument. 
	    This function prepares object to be able to receive pointer from function passed as argument. If pointer contains and object, it is released
		@return pointer to raw pointer inside of this object. You can pass return value to function which will create new object and set address to this object
		through that pointer. Function expects, that new object has increased reference count 

		*/
	T **init() {
		if (this->ptr) ptr->Release();
		this->ptr = 0;
		return &ptr;
	}

protected:
	template<typename P>
	static T *cast(P *p) {
		if (p == 0) return 0;
		void *pp;
		long hr = p->QueryInterface(__uuidof(T), &pp);
		if (hr != 0) throw ComException(THISLOCATION,hr);
		return (T *)pp;
	}
};


}

#endif