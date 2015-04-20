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



///Interface to access Thread Local Storage variable table
/** Every thread has own table. You can retrie instance of ITLSTable
 * for the current thread by calling ITLSTable::getInstance
 */
class ITLSTable {
public:

	typedef void (*DeleteFunction)(void *ptr);

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
     * @param delFn pointer to a function responsible to deleting instance. If NULL,
     * no action is taken.
     */
    virtual void setVar(natural index, void *value, DeleteFunction delFn = 0) = 0;
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

template<typename T>
void deleteFunction(void *var) {
	delete reinterpret_cast<T *>(var);
}


}  // namespace LightSpeed


#endif /* LIGHTSPEED_SYNC_TLS_H_ */
