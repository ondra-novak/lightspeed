/*
 * tls.cpp
 *
 *  Created on: 22.10.2010
 *      Author: ondra
 */

#include "tls.h"
#include "threadVar.h"
#include "..\containers\queue.tcc"
#include "..\..\mt\thread.h"

namespace LightSpeed {

	natural FastTLSAlloc::allocIndex()
	{
		Synchronized<FastLock> _(lock);
		if (freeList.empty()) {
			return counter++;
		}
		else {
			natural out = freeList.top();
			freeList.pop();
			return out;
		}
	}

	void FastTLSAlloc::freeIndex(natural index)
	{
		Synchronized<FastLock> _(lock);
		freeList.push(index);

	}

	FastTLSAlloc & FastTLSAlloc::getInstance()
	{
		return Singleton<FastTLSAlloc>::getInstance();
	}

	void FastTLSTable::clear()
	{
		class Deleter {
		public:
			TLSItem *data;
			natural count;
			Deleter(TLSItem *data, natural count) :data(data), count(count) {}
			bool run() {
				bool found = false;
				while (count > 0) {
					count--;
					if (data[count].ptr && data[count].dtor) {
						data[count].dtor(data[count].ptr);
						found = true;
					}
					data[count].ptr = 0;
				}
				return found;
			}
			~Deleter() {
				run();
			}
		};
		bool rep;
		do {
			rep = Deleter(table.data(), table.length()).run();
		} while (rep);
	}

	FastTLSTable & FastTLSTable::getInstance()
	{
		Thread *x = Thread::currentPtr();
		if (x == 0) {
			static TLSTable global;
			return global;
		}
		else {
			return x->getTls();
		}
	}

}
