/*
 * tls.h
 *
 *  Created on: 22.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_SYNC_TLS_H_
#define LIGHTSPEED_SYNC_TLS_H_


#include "../types.h"
#include "../memory/factory.h"
namespace LightSpeed {

///Interface to delete object registered in Thred Local Storage
class IThreadVarDelete {
public:
	///Called when object registered under pointer must be destroyed
	/** note - function should not throw exceptions */
	virtual void operator()(void *ptr)  = 0;
};

///Factory, that is extended by IThreadVarDelete interface
/**
 * @tparam T type allocated and destroyed by this factory
 * @tparam Factory name of class used to carry responsibility of allocations and destroyings
 *
 * You can use this class as IThreadVarDelete. Object created using this
 * factory will be also destroyed by this factory when threads exits
 */
template<typename T, typename Factory>
class ThreadVarDeleteFactory: public IThreadVarDelete,
			public Factory::template Factory<T> {
	typedef typename Factory::template Factory<T> Super;
public:
	ThreadVarDeleteFactory(const Factory &fact)
		:Super(Factory::template Factory<T>(fact)) {}
	ThreadVarDeleteFactory() {}

	virtual void operator()(void *ptr) {
		Super::destroyInstance(reinterpret_cast<T *>(ptr));
	}

};

///Interface to access Thread Local Storage variable table
/** Every thread has own table. You can retrie instance of ITLSTable
 * for the current thread by calling ITLSTable::getInstance
 */
class ITLSTable {
public:

	///Retrieves pointer to variable at given index
	/**
	 * @param index index into TLS table. Index can be out of range
	 * @return pointer to registered variable, 0 if no variable is registered
	 */
    virtual void *getVar(natural index) const = 0;
    ///Sets new variable to the index
    /**
     * @param index index into TLS table.
     * @param value new pointer to be registered
     * @param delFn pointer to object responsible to deleting instance. If NULL,
     * no action is taken.
     */
    virtual void setVar(natural index, void *value, IThreadVarDelete *delFn) = 0;
    virtual void unsetVar(natural index) = 0;
    virtual ~ITLSTable() {}


    typedef ITLSTable &(*fn_GetTLS)();

	///Retrieves instance of TLS in current thread
    static ITLSTable &getInstance();

	static void setTLSFunction(fn_GetTLS fn);

	static fn_GetTLS getTLSFunction();

};



class ITLSAllocator {
public:
    virtual natural allocIndex() = 0;
    virtual void freeIndex(natural index) = 0;
    virtual ~ITLSAllocator() {}

    typedef ITLSAllocator &(*fn_GetTLSAllocator)();

	static ITLSAllocator &getInstance();

	static void setTLSFunction(fn_GetTLSAllocator fn);

	static fn_GetTLSAllocator getTLSFunction();


    ///Retrieves id of highest index
    /** Needed when ITLSAllocator table is preallocated */
    virtual natural getMaxIndex() const = 0;
};

ITLSTable &_stGetTLS();
ITLSAllocator &_stGetTLSAllocator();


}  // namespace LightSpeed


#endif /* LIGHTSPEED_SYNC_TLS_H_ */
