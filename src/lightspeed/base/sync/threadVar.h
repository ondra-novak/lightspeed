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


        TLSRecord():varPtr(0),deleteFunct(0) {}
        bool isInited() const {
            return deleteFunct != 0 || varPtr != 0;
        }

        void set(void *varPtr, IThreadVarDelete *del) {
            unset();
            this->varPtr = varPtr;
            this->deleteFunct = del;
        }

        void unset() {
            if (deleteFunct) {
				(*deleteFunct)(varPtr);
            }
            varPtr =0;
			deleteFunct = 0;
        }

        void *get() const {
            return varPtr;
        }
        
        ~TLSRecord() {
            unset();
        }

        IThreadVarDelete *getDestructor() const {
        	return deleteFunct;
        }

    protected:

        mutable void *varPtr;
        IThreadVarDelete *deleteFunct;
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

        TLSTable() {}
        TLSTable(const Allocator &alloc):dynamicTable(alloc) {}

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
        void setVar(natural index, void *value, IThreadVarDelete *delFn) {
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
    template<class T>
    class ThreadVar {
    public:   
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
        void set(ITLSTable &table, T *ptr, IThreadVarDelete *delFn) {
        	unset(table);
        	table.setVar(index,ptr,delFn);
        }


        ///Unsets value from the ThreadVar
        void unset(ITLSTable &table) {
        	table.unsetVar(this->index);
        }

		///Sets value if object in current thread
		/**
		 * @param ptr new value
		 *
		 * @note function can be slow, because retrieveing current context is slow. It is better set variable through ITLSTable
		 */
#if 0		 
		void set(T *ptr) {set(ITLSTable::getInstance(),ptr);}
		///Sets value if object in current thread
		/**
		 * @param ptr new value
		 * @param delFn object notified about destroying the variable
		 * @note function can be slow, because retrieveing current context is slow. It is better set variable through ITLSTable
		 */
		void set(T *ptr, IThreadVarDelete *delFn) {set(ITLSTable::getInstance(),ptr,delFn);}

		void unset() {
			ITLSTable::getInstance().unsetVar(this->index);
		}

		operator T *() const {return operator[](ITLSTable::getInstance());}
		T *operator->() const {return operator[](ITLSTable::getInstance());}
#endif

    protected:
        natural index;

    private:
        ThreadVar &operator=(const ThreadVar &var);
        ThreadVar(const ThreadVar &other);

    };

}


#endif
