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
#include "../../mt/fastlock.h"
#include "../containers/queue.h"
#include "../containers/autoArray.h"
namespace LightSpeed {

///writting there brand new TLS table implementation

class FastTLSAlloc {
public:
	FastTLSAlloc() :counter(0) {}

	natural allocIndex();
	void freeIndex(natural index);
	natural getCounter() const { return counter; }


	static FastTLSAlloc &getInstance();

protected:
	FastLock lock;
	natural counter;
	Queue<natural> freeList;
};




class FastTLSTable {
public:

	FastTLSTable() {}
	typedef void(*Destructor)(void *ptr);

	void *getVar(natural index) const {
		if (index >= table.length()) return 0;
		return table.data()[index].ptr;
	}

	void setVar(natural index, void *value, Destructor dtor) {
		if (index >= table.length()) {
			table.resize(index + 1);
		}
		table.data()[index].set(value, dtor);
	}

	void unsetVar(natural index) {
		if (index >= table.length()) return;
		table.data()[index].set(0, 0);
	}
	void clear();


	static FastTLSTable &getInstance();

	FastTLSTable &operator=(FastTLSTable &other) {
		for (natural i = 0; i < other.table.length(); i++) {
			setVar(i, other.table[i].ptr, other.table[i].dtor);
			other.table(i).dtor = 0;
		}
		return *this;
	}


	~FastTLSTable() { clear(); }
private:
	FastTLSTable(const FastTLSTable &);

protected:
	struct TLSItem {
		void *ptr;
		Destructor dtor;

		TLSItem() :ptr(0), dtor(0) {}
		void set(void *newptr, Destructor newdtor) {
			void *oldPtr = ptr;
			Destructor oldDtor = dtor;
			ptr = newptr;
			dtor = newdtor;
			if (oldDtor) oldDtor(ptr);
		}

		bool used() const { return ptr != 0; }		
	};

	AutoArray<TLSItem> table;
};


typedef FastTLSTable TLSTable;
//declared for backward compatibility
typedef FastTLSTable ITLSTable;
typedef FastTLSAlloc TLSAlloc;


template<typename T>
void deleteFunction(void *var) {
	delete reinterpret_cast<T *>(var);
}


}  // namespace LightSpeed


#endif /* LIGHTSPEED_SYNC_TLS_H_ */
