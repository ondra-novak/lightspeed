#pragma once
#include "runtimeAlloc.h"
#include "../../mt/slist.h"

namespace LightSpeed {


	class PoolAlloc: public IRuntimeAlloc {

		struct BlockBase {
			natural size;
		};

		struct Block: public BlockBase {
			Block *next;
		};


		PoolAlloc &operator=(const PoolAlloc &);

	public:
		LIGHTSPEED_CLONEABLECLASS;


		virtual void *alloc(natural objSize, IRuntimeAlloc * &owner) {
			owner = this;
			return alloc(objSize);
		}

		virtual void *alloc(natural objSize) {

			lockInc(refs);
			//adjust size to have space to store pointer
			if  (objSize < sizeof(Block) - sizeof(BlockBase)) objSize = sizeof(Block) - sizeof(BlockBase);
			//take first empty block
			Block *x = blockList.pop();
		/*	if (x != 0 && (natural)x < 0xFF) {
				abort();
			}*/
			//is there?
			while (x) {
				//finally we safety isolate first block
				if (x->size < objSize) {
					//to small blocks discard
					operator delete(x);					
				} else {
					//block found, return it
					return &x->next;
				}
				x = blockList.pop();
			}
			//no block isolated
			if (objSize > largestAlloc) largestAlloc = objSize;
			else if (objSize < largestAlloc) objSize = largestAlloc;
			//allocate new one

			x = (Block *)operator new(sizeof(BlockBase) + objSize);
			//set its size
			x->size = objSize;
			//return pointer to it
			return &x->next;
		}
		virtual void dealloc(void *ptr, natural )  {

			//convert pointer to block
			Block *k = static_cast<Block *>(reinterpret_cast<BlockBase *>(ptr)-1);

			if (destroyed != 0) {
				operator delete(k);
			} else
				blockList.push(k);

			release();
		}


		PoolAlloc():largestAlloc(0),destroyed(0),refs(1) {}
		PoolAlloc(const PoolAlloc &):largestAlloc(0),destroyed(0),refs(1) {}
		void freeExtra() {
			//cleanup blocks
			while (Block *k = blockList.pop()) {
				operator delete(k);
			}

		}
		~PoolAlloc() {
			freeExtra();
			destroyed = 0x12345678;
		}

		virtual void release() {
			if (lockDec(refs) == 0) {
				freeExtra();
				destroyed = 0x12345678;
			}
		}

	protected:
		SList<Block> blockList;
		natural largestAlloc;
		natural destroyed;
		atomic refs;
	};



	class MultiPoolAlloc: public IRuntimeAlloc {
	public:
		LIGHTSPEED_CLONEABLECLASS;


		virtual void *alloc(natural objSize, IRuntimeAlloc * &owner) {
			owner = getOwner(objSize);
			return owner->alloc(objSize, owner);
		}

		virtual void *alloc(natural objSize) {
			return getOwner(objSize)->alloc(objSize);
		}
		virtual void dealloc(void *ptr, natural sz)  {
			getOwner(sz)->dealloc(ptr,sz);
		}

		virtual void release() {
			all16.release();
			all32.release();
			all64.release();
			all128.release();
			all256.release();
			all512.release();
			all1024.release();
		}

	protected:

		IRuntimeAlloc *getOwner(natural objSize) {
			IRuntimeAlloc * owner;
			if (objSize > 1024) {
				owner = &StdAlloc::getInstance();
			} else if (objSize <= 16) {
				owner = &all16;
			} else if (objSize <= 32) {
				owner = &all32;
			} else if (objSize <= 64) {
				owner = &all64;
			} else if (objSize <= 128) {
				owner = &all128;
			} else if (objSize <= 256) {
				owner = &all256;
			} else if (objSize <= 512) {
				owner = &all512;
			} else  {
				owner = &all1024;
			}
			return owner;
		}

		PoolAlloc all16,all32,all64,all128,all256,all512,all1024;

	};
}
