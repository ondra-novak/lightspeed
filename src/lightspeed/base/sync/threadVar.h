#ifndef LIGHTSPEED_MT_TLS    
#define LIGHTSPEED_MT_TLS
#pragma once



#include <set>
#include <vector>
#include <cstddef>
#include "../exceptions/rangeException.h"
#include "../exceptions/pointerException.h"
#include "../sync/synchronize.h"
#include "../iter/sortFilter.h"
#include "tls.h"

namespace LightSpeed {

	

    ///Declaration of thread variable
    /** ThreadVar is actually pointer to a value. ThreadVar can contain different pointer
     * on a different thread. Each thread can see different content (different pointer).
     *
     * Because you can specify also way, how to destroy the object where variable points, ThreadVar
     * can be also used to store whole values, not just pointers
     *
     * When thread starts, every ThreadVar contains NULL, it does mean they don't have values assigned. You
     * can use etheir set() to set pointer or pointer with a "destroy function" or setValue() to set
     * value directly without bothering with allocation and need to define a destroy function.
     *
     * To access value of ThreadVar, you will need an instance of ITLSTable class. Instance valid
     * for current thread can be obtained by function ITLSTable::getInstance() anywhere in the
     * program. If you have instance of another thread, you can also access it (and change
     * the variable in a different thread remotely) but you will need to perform proper synchronization.
     *
     * You cannot access TLSTable of not-running thread
     */
    template<class T>
    class ThreadVar {
    public:   
    	typedef TLSTable::Destructor DeleteFunction;
        ///Construction of ThreadVar, allocates the TLS index       
        ThreadVar():index(TLSAlloc::getInstance().allocIndex()) {};

        ///Destruction of ThreadVar, releases the TLS index
        /**
        @note destructor will not delete the variable in the TLSTables. They
        must be remove manually or they will be removed with deleting TLSTables
        */
        ~ThreadVar() {
        	TLSAlloc::getInstance().freeIndex(index);
        }


        ///Retrieves pointer to data in specified table
        /**
         *
         * @param table TLS table that contains data
         * @return pointer to data or NULL, if not set
         */
        T *operator[](const TLSTable &table) const {
            return reinterpret_cast<T *>(table.getVar(index));
        }

        ///Sets pointer to the ThreadVar
        /** Setting pointer means, that thread variable will
         * remember the pointer, but will not track destruction of
         * this variable. You have to manage pointer and refered instance
         * manually
         *
         * @param table TLS table
         * @param ptr pointer to store
         */

        void set(TLSTable &table, T *ptr ) {
        	unset(table);
            table.setVar(index,ptr,0);
        }

        ///Sets pointer to the ThreadVar
        /**
         * @param table reference to the TLS table
         * @param ptr pointer to set
         * @param delFn pointer to interface registered with variable which
         * 		will handle variable destruction
         *
         * @note before set is called unset() to remove old variable.
         */
        void set(TLSTable &table, T *ptr, DeleteFunction delFn) {
        	unset(table);
        	table.setVar(index,ptr,delFn);
        }


        ///Unsets value from the ThreadVar
        void unset(TLSTable &table) {
        	table.unsetVar(this->index);
        }

        ///Allocates and sets new value
        /**
         * @param table tls table where to set a value
         * @param val new value
         * @return pointer to newly allocated variable inside TLS
         *
         * Function sets a new value of the ThreadVar. It uses operator new to allocate
         * copy of the value and also registers "delete" function, so when
         * the variable or the thread is destroyed, delete is used to
         * destroy the value automatically
         */
        T * setValue(TLSTable &table, const T &val) {
        	T * ret = new T(val);
        	set(table,ret, &deleteFunction<T>);
        	return ret;
        }


    protected:
        natural index;

    private:
        ThreadVar &operator=(const ThreadVar &var);
        ThreadVar(const ThreadVar &other);
    };


    ///Helper class for ThreadVarInit and ThreadVarInitDefault
    template<typename T, typename Impl>
    class ThreadVarInitBase: public ThreadVar<T> {
    public:
    	///Receives value of the variable for specified TLS
    	/**
    	 * @param table specify TLS from which this variable is received. You can use
    	 *  ITLSTable::getInstance() to receive TLS for current thread. Because receiving
    	 *  TLS can cost some time, it is recommended to store TLS reference to a
    	 *  local variable while accessing multiple TLS variables in a single oblock.
    	 *
    	 * @return reference to variable. On the first access, variable is initialized by
    	 *  value passed to the constructor.
    	 *
    	 * @note this function can be used to modify value
    	 *
    	 */
        T &operator[](TLSTable &table) {
        	T *p = ThreadVar<T>::operator[](table);
        	if (p == 0) {
        		return static_cast<Impl *>(this)->init(table);
        	} else {
        		return *p;
        	}
        }

    	///Receives value of the variable for specified TLS
    	/**
    	 * @param table specify TLS from which this variable is received. You can use
    	 *  ITLSTable::getInstance() to receive TLS for current thread. Because receiving
    	 *  TLS can cost some time, it is recommended to store TLS reference to a
    	 *  local variable while accessing multiple TLS variables in a single oblock.
    	 *
    	 * @return reference to variable. On the first access, variable is initialized by
    	 *  value passed to the constructor.
    	 *
    	 * @note this is const function, returned value is const and cannot be modified.
    	 *
    	 */
        const T &operator[](TLSTable &table) const {
        	T *p = ThreadVar<T>::operator[](table);
        	if (p == 0) {
        		return static_cast<Impl *>(const_cast<ThreadVarInitBase *>(this))->init(table);
        	} else {
        		return *p;
        	}
        }

        ///Performs manual-initialization when it is necessary
        /**
         * Function checks or initializes variable for specified TLS. If variable is
         * already initialized, function does nothing.
         *
         * @param table reference to the TLS table for the current or an other thread
         *
         * @note accessing the variable without picking return value performs the same
         * action.
         */
        void initOnce(TLSTable &table)  {
        	this->operator[](table);
        }

    };

    ///Thread variable with automatic initialization
    /** Thread variable can be constructed with any object which is used to its
     * initialization. There is no limit in compare to platform or C++11 implementation
     * which can limit initialization to standard types.
     *
     * Note that initialization is performed during first access of the variable. This
     * can be source of issues, when one expect independent behavior while accessing the
     * variable. First access can also throw an exception when initialization fails
     *
     * You can also enforce initialization by calling function init(). You must call this
     * function, when you passing variable as ThreadVar<T>. Without this, callee can
     * receive NULL through ThreadVar without initialization (because class is not polymorphic)
     *
     */
    template<typename T>
    class ThreadVarInit: public ThreadVarInitBase<T,ThreadVarInit<T> > {
    public:
    	///Initializes thread variable
    	/**
    	 * Allocates slot in the TLS allocation table. Also stores value. It makes
    	 * copy of that value
    	 *
    	 * @param val value used to initialize.
    	 *
    	 * @note function doesn't initialize variable in the current thread. You have to
    	 * call init() to perform extra initialization
    	 *
    	 * @note variable can be inaccessible in threads active during creation time. It
    	 * is strongly recommended to create instance during program initialization or
    	 * before threads are created
    	 */
    	ThreadVarInit(const T &val):val(val) {}



        ///Performs manual-initialization
        /**
         * @param table reference to the TLS table for the current or an other thread
         * @return function returns reference to the variable.
         *
         * @note Function always perform new initialization despite on current variable
         * state. Also note that initialization causes that any reference to the
         * previous variable becomes invalid. This is only valid way how to reset
         * variable to the initial state on type which has no assignment operator (it performs
         * destruction and new construction)
         *
         * @see initOnce
         *
         */
        T &init(TLSTable &table)  {
        	T *x = new T(val);
        	try {
        		//function can throw exception
        		ThreadVar<T>::set(table, x, &deleteFunction<T>);
        	} catch (...) {
        		//so delete created object
        		delete x;
        		//rethrow
        		throw;
        	}
         	return *x;
        }

    protected:
    	T val;
    };

    ///Thread variable with automatic initialization
     /** Thread variable can be constructed with any object which is used to its
      * initialization. There is no limit in compare to platform or C++11 implementation
      * which can limit initialization to standard types.
      *
      * Variable is initialized using default constructor
      *
      * Note that initialization is performed during first access of the variable. This
      * can be source of issues, when one expect independent behavior while accessing the
      * variable. First access can also throw an exception when initialization fails
      *
      * You can also enforce initialization by calling function init(). You must call this
      * function, when you passing variable as ThreadVar<T>. Without this, callee can
      * receive NULL through ThreadVar without initialization (because class is not polymorphic)
      *
      */
   template<typename T>
    class ThreadVarInitDefault:  public ThreadVarInitBase<T,ThreadVarInitDefault<T> > {
    public:
       ///Performs manual-initialization
       /**
        * @param table reference to the TLS table for the current or an other thread
        * @return function returns reference to the variable.
        *
        * @note Function always perform new initialization despite on current variable
        * state. Also note that initialization causes that any reference to the
        * previous variable becomes invalid. This is only valid way how to reset
        * variable to the initial state on type which has no assignment operator (it performs
        * destruction and new construction)
        *
        * @see initOnce
        *
        */
        T &init(TLSTable &table)  {
        	T *x = new T;
        	try {
        		ThreadVar<T>::set(table, x, &deleteFunction<T>);
        	} catch (...) {
        		delete x;
        		throw;
        	}
         	return *x;
         }
  };

}


#endif
