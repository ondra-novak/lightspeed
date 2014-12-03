#pragma once

#ifndef LIGHTSPEED_MEMORY_STACKFACTORY_H_
#define LIGHTSPEED_MEMORY_STACKFACTORY_H_

#include "factory.h"

namespace LightSpeed {


	class StackFactoryCorruptedException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		StackFactoryCorruptedException(const ProgramLocation &loc)
			:Exception(loc) {}
	protected:
		void message(ExceptionMsg &msg) const {
			msg.cString("Stack factory - corrupted heap");
		}
	};

	class StackFactoryMemoryLeakException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		StackFactoryMemoryLeakException(const ProgramLocation &loc)
			:Exception(loc) {}
	protected:
		void message(ExceptionMsg &msg) const {
			msg.cString("Stack factory - memory leak detected");
		}
	};

	template<typename baseType = natural, baseType objSize, baseType objMax>
	class StackFactoryBlock {
	
		static const baseType bkWCount = (objSize+sizeof(baseType)-1)/objSize;
		static const baseType totalWSpace = bkWCount * objMax;
		static const baseType totalSpace = totalWSpace * sizeof(baseType);

		struct Block {
			baseType data[bkWCount];
		};

		Block space[totalWSpace];
		baseType firstFree;
		baseType currentEnd;
		static const baseType endMark = ~((baseType)0);
	public:

		static const baseType maxObjSize = bkWCount*sizeof(baseType);

		StackFactoryBlock():firstFree(endMark),currentEnd(0) {}
		
		void *alloc() {
			void *res = 0;
			if (firstFree == endMark) {
				if (currentEnd >= objMax) return res;
				res = space[currentEnd].data;
				++currentEnd;
			} else {
				res = space[firstFree].data;
				firstFree = space[firstFree].data[0];
			}
		}

		bool free(void *ptr) {
			Block *x = reinterpret_cast<Block *>(ptr);
			baseType pos = x - space;
			if (x + 1 == currentEnd) {
				currentEnd--;
			}
			else {
				x->data[0] = firstFree;
				firstFree = pos;
			}
			return true;
		}

		void detectLeak() {
			baseType countEmpty;
			baseType x = firstFree;
			while (x != endMark && countEmpty < currentEnd) {
				countEmpty++;
			}
			if (x == endMark && countEmpty != currentEnd)
				throw StackFactoryCorruptedException(THISLOCATION);
			if (x != endMark && countEmpty == currentEnd)
				throw StackFactoryMemoryLeakException(THISLOCATION);
		}
		
	};

	template<natural objSize, natural maxObj>
	class StackFactory {
	public:

		StackFactory() {}
		StackFactory(const StackFactory &) {}
		~StackFactory() {
			if (!std::uncaught_exception())
				block.detectLeak();
		}

		template<typename Obj>
		class Factory: public FactoryBase<Factory<Obj> > {
			friend class FactoryBase<Factory<Obj> >;

			void *alloc(natural sz) {
				return owner.alloc(sz);
			}

			void delloc(void *ptr, natural sz) {
				return owner.dealloc(ptr,sz);
			}

			StackFactory &owner;

		public:
			Factory(StackFactory &owner):owner(owner) {}
		};


	protected:
		StackFactoryBlock<objSize,maxObj> block;
		
		void *alloc(natural sz) {
			if (sz>block.maxObjSize) 
				throwAllocatorLimitException(THISLOCATION,sz,block.maxObjSize,typeid(StackFactory));
			void *ptr =block.alloc();
			if (ptr == 0)
				throwAllocatorLimitException(THISLOCATION,sz,0,typeid(StackFactory));
			return ptr;
		}

		void dealloc(void *ptr, natural sz) {
			if (block.free(ptr) == false)
				throw InvalidPointerException(THISLOCATION);
		}



	};


}
