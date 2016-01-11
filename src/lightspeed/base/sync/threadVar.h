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
#include "tlsalloc.h"

namespace LightSpeed {



    ///Represents one record in the TLS table
    /**Please don't use this class directly, if you don't know, what you are doing. 
    This is part of internal structures. TLSRecord is one record in the TLS table. It
    contains pointer to thread variable and pointer to function, which is called, when
    variable have to be destroyed
    */
    class TLSRecord {
    public:

    	typedef ITLSTable::DeleteFunction DeleteFunction;

        TLSRecord():varPtr(0),deleteFunct(0) {}
        bool isInited() const {
            return deleteFunct != 0 || varPtr != 0;
        }

        void set(void *varPtr, DeleteFunction del) {
            unset();
            this->varPtr = varPtr;
            this->deleteFunct = del;
        }

        void unset() {
        	//we have to first unset the item, then delete it
        	//that because there can be some logic that need to reassign item during destruction
        	//and we need also handle situation, when destructor throws an exception

        	//store pointer first
            void *v = varPtr;
            //store delete function
            DeleteFunction d = deleteFunct;
            //set pointer to 0
            varPtr = 0;
            //reset delete function
            deleteFunct = 0;

            //now, if delete function is defined, delete the object
            if (d) {
				(*d)(v);
            }
        }

        void *get() const {
            return varPtr;
        }
        
        ~TLSRecord() {
            unset();
        }

        DeleteFunction getDestructor() const {
        	return deleteFunct;
        }

    protected:

        mutable void *varPtr;
        DeleteFunction deleteFunct;
    };

    template<>
    class MoveObject<TLSRecord>: public MoveObject_Binary {};



    ///Implements TLSTable in the thread context
    /**
    Note, class is NOT thread safe. But objects created using this class will not be
        shared between the thread, so in standard usage, no synchronization is needed.
        Only when you allows thread access the TLS of another thread
    */
    template<typename Allocator = StdAlloc >
    class TLSTable: public ITLSTable {
        typedef AutoArray<TLSRecord,Allocator> DynTable;
    public:

        typedef ITLSTable::DeleteFunction DeleteFunction;

        TLSTable() {}
        TLSTable(const Allocator &alloc):dynamicTable(alloc) {}

        void clear() {
        	/* HACK: while the TLS table is destroyed, there can
        	 * be destructor, which sets new  TLS variables. Before
        	 * the destructor complete, it must ensure, whether
        	 * all entries has been release. If not, operation is
        	 * repeated.
        	 *
        	 * The TLS table is still available during destruction, so
        	 * this can easy happen.
        	 */
        	bool rep;
        	do {
        		rep = false;
        		for (natural x = 0, l = dynamicTable.length(); x <l; l++) {
        			if (dynamicTable[x].isInited()) {
        				dynamicTable(x).unset();
        				rep = true;
        			}
        		}
        	} while (rep);

        }

        ///gets record at the index
        /**
        @param index index which's record should be get
        @return Function returns pointer associated with the record. If variable
            has not been initialized yet, function returns 0;
        */
        void *getVar(natural index) const {
            if (index < dynamicTable.length()) return dynamicTable[index].get();
            else return 0;
       }

        ////sets the record at the index
        /**
        @param index index to set. Note that index have to be allocated by
            TLSAllocator. Table has structure as linear array which contains
            indexes from zero to max. If you set TLS index for example number '100' 
            with value, function will also create indexes 0...99 in unitialized state.
         */
        void setVar(natural index, void *value, DeleteFunction delFn) {
            //do nothing, if you unset the variable outside of range
            if (value == 0 && delFn == 0 && index >= dynamicTable.length())
                return;
            //allocate table, if this is necesery
            if (dynamicTable.length() <= index)
                dynamicTable.resize(index+1);
            //set the record
             dynamicTable(index).set(value,delFn);
        }


        void unsetVar(natural index) {
            //do nothing, if you unset the variable outside of range
            if (index >= dynamicTable.length()) return;
            dynamicTable(index).unset();

        }

        const Allocator &getAllocator() const {return dynamicTable.getAllocator();}

        virtual void moveTo(ITLSTable &newTable) {

        	for (natural i = 0; i < dynamicTable.length(); i++) {
				const TLSRecord &rc = dynamicTable[i];
				newTable.setVar(i,rc.get(),rc.getDestructor());
        	}
        	dynamicTable.clear();
        }

    protected:

        mutable DynTable dynamicTable;

    };



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
    	typedef ITLSTable::DeleteFunction DeleteFunction;
        ///Construction of ThreadVar, allocates the TLS index       
        ThreadVar():index(ITLSAllocator::getInstance().allocIndex()) {};

        ///Destruction of ThreadVar, releases the TLS index
        /**
        @note destructor will not delete the variable in the TLSTables. They
        must be remove manually or they will be removed with deleting TLSTables
        */
        ~ThreadVar() {
        	ITLSAllocator::getInstance().freeIndex(index);
        }


        ///Retrieves pointer to data in specified table
        /**
         *
         * @param table TLS table that contains data
         * @return pointer to data or NULL, if not set
         */
        T *operator[](const ITLSTable &table) const {
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

        void set(ITLSTable &table, T *ptr ) {
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
        void set(ITLSTable &table, T *ptr, DeleteFunction delFn) {
        	unset(table);
        	table.setVar(index,ptr,delFn);
        }


        ///Unsets value from the ThreadVar
        void unset(ITLSTable &table) {
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
        T * setValue(ITLSTable &table, const T &val) {
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
        T &operator[](ITLSTable &table) {
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
        const T &operator[](ITLSTable &table) const {
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
        void initOnce(ITLSTable &table)  {
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
        T &init(ITLSTable &table)  {
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
        T &init(ITLSTable &table)  {
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
