/*
 * runtimeAlloc.h
 *
 *  Created on: 2.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_RUNTIMEALLOC_H_
#define LIGHTSPEED_MEMORY_RUNTIMEALLOC_H_

#pragma once

#include <typeinfo>
#include "../types.h"
#include "../exceptions/throws.h"
#include "../meta/isDynamic.h"
#include "../../base/memory/cloneable.h"

namespace LightSpeed {

	class IRuntimeAlloc: public ICloneable {
	public:

		LIGHTSPEED_CLONEABLEDECL(IRuntimeAlloc)

		///allocates space for the object
		/**
		 * @param objSize size of object in the bytes
		 * @return pointer to the newly allocated space
		 */
		virtual void *alloc(natural objSize)  = 0;
		///releases allocated space
		/**
		 * Function is called, when construction of copy fails with an
		 * exception.
		 * @param ptr pointer to memory allocated by allocSpace()
		 * @param objSize size of object, the same value as used in allocSpace()
		 */
		virtual void dealloc(void *ptr, natural objSize)  = 0;


		///Allocates space for the object and associates instance of the allocator
		/**
		 * Function can be used to faster search allocator which is responsible
		 * for this allocation in case, that current instance represents
		 * group of allocators. Searching allocator by address only
		 * can be slower, so you can remember owner of allocated block.
		 *
		 * Class PolyAlloc uses this allocator to retrieve pointer for remembering
		 * with the allocated block instead remembering pointer to current instance
		 *
		 * @param objSize size to allocate
		 * @param owner variable which receives pointer of instance
		 *
		 * @note default implementation sets owner to this. Some implementations
		 * doesn't support basic alloc() and requires this extended version.
		 * They can throw exception.
		 */
		virtual void *alloc(natural objSize, IRuntimeAlloc * &owner) = 0;

		///Allocates object using IRuntimeAlloc
		/**
		 * @param obj source object.
		 * @return pointer to copy allocated by this allocator
		 *
		 * @note Do not use to create object of polymorphic classes unless you plan to destroy them on the same
		 * level of class hierarchy. To use allocator for polymorphic classes, see DynObject class.
		 *
		 * @note this function is intended to be used to create instance of simple class or struct, for example
		 * POD objects or objects without virtual destructor. In all other cases, use DynObject, which can handle
		 * allocation better. Objects created by createInstance MUST be destroyed by destroyInstance. Do not use
		 * operator delete
		 *
		 * @see DynObject
		 *
		 *
		 */
		template<typename T>
		T *createInstance(const T &obj)  {
			natural s = sizeof(T);
			void *p = alloc(s);
			try {
				return new(p) T(obj);
			} catch (...) {
				try {
					dealloc(p,s);
				} catch (...) {

				}
				throw;
			}
		}

		template<typename T, typename U>
		T *createInstanceUsing(const U &obj)  {
			natural s = sizeof(T);
			void *p = alloc(s);
			try {
				return new(p) T(obj);
			} catch (...) {
				try {
					dealloc(p,s);
				} catch (...) {

				}
				throw;
			}
		}

		///Deletes instance using IRuntimeAlloc
		/**
		 * @param inst pointer to instance created by createInstance.
		 *
		 */
		template<typename T>
		void destroyInstance(T *inst)  {
			class Deleter {
			public:
				void *addr;
				IRuntimeAlloc *owner;
				~Deleter() {
					owner->dealloc(addr,sizeof(T));
				}
			};

			if (inst == 0) return;
			Deleter del;
			del.addr = inst;
			del.owner = this;
			inst->~T();
		}

		virtual ~IRuntimeAlloc() {}
	};



	class AllocInBuffer: public IRuntimeAlloc {
	public:
		AllocInBuffer(void *buffer,natural maxSize):buffer(buffer),used(0),maxSize(maxSize) {}
		virtual void *alloc(natural objSize)  {
			if (used + objSize > maxSize)
				throwAllocatorLimitException(THISLOCATION,objSize,maxSize-used,typeid(AllocInBuffer));
			natural pos = used;
			used+=objSize;
			return reinterpret_cast<byte *>(buffer)+pos;
		}
		virtual void *alloc(natural objSize, IRuntimeAlloc * &owner) {
			owner = this;
			return alloc(objSize);
		}

		virtual void dealloc(void *ptr, natural objSize)  {
			natural offs = reinterpret_cast<byte *>(ptr)
					- reinterpret_cast<byte *>(buffer);

			if (offs+objSize >= used && offs < used) {
				used = offs;
			}
		}

		virtual natural getObjectSize() const {return sizeof(AllocInBuffer);}
		virtual AllocInBuffer *clone(IRuntimeAlloc &) const {throwUnsupportedFeature(THISLOCATION,this,"clone");return 0;}
		virtual AllocInBuffer *clone() const {throwUnsupportedFeature(THISLOCATION,this,"clone");return 0;}

	protected:
		void *buffer;
		mutable natural used;
		natural maxSize;
	};

	template<natural size>
	class AllocAtStack: public AllocInBuffer {
	public:
		AllocAtStack():AllocInBuffer(buffer,size) {}

	protected:
		byte buffer[size];
	};


	///Ad-hoc allocator allows to allocate in AllocatedMemory<byte> instance
	/**
	 * Similar to AllocInBuffer, but buffer is defined by memory kept in instance of AllocatedMemory
	 *
	 * class must be specialized to bytes.
	 */
	template<template<class> class _AllocatedMemory>
	class AllocInAllocatedMemory: public IRuntimeAlloc {
	public:
		AllocInAllocatedMemory(_AllocatedMemory<byte> &trg):mem(trg) {}
		virtual void *alloc(natural objSize) const {
			if (objSize > mem.getSize()) {
				throwAllocatorLimitException(THISLOCATION,objSize,mem.getSize(),typeid(AllocInBuffer));
			}
			return mem.getBase();
		}
		virtual void dealloc(void *, natural ) const {}

		virtual natural getObjectSize() const {return sizeof(AllocInBuffer);}
		virtual AllocInAllocatedMemory *clone(IRuntimeAlloc &) const {throwUnsupportedFeature(THISLOCATION,this,"clone");return 0;}
		virtual AllocInAllocatedMemory *clone() const {throwUnsupportedFeature(THISLOCATION,this,"clone");return 0;}

	protected:
		_AllocatedMemory<byte> mem;
	};


	template<class Alloc>
	class RuntimeAllocInAllocator: public IRuntimeAlloc {
		typedef typename Alloc::template AllocatedMemory<byte> Block;
	public:
		RuntimeAllocInAllocator(const Alloc &alloc):bk(alloc),occupied(false) {}
		RuntimeAllocInAllocator():bk(Alloc()),occupied(false) {}
		virtual void *alloc(natural objSize) const {
			if (occupied) {
				throwAllocatorLimitException(THISLOCATION,objSize,0,typeid(AllocInBuffer));
			} else {
				Block nwbk(bk,objSize);
				nwbk.swap(bk);
				occupied = true;
				return bk.getBase();
			}

		}
		virtual void dealloc(void *, natural ) const {
			Block nwbk(bk);
			nwbk.swap(bk);
			occupied = false;
		}


	protected:

		mutable Block bk;
		mutable bool occupied;

	};





}

#endif /* LIGHTSPEED_MEMORY_RUNTIMEALLOC_H_ */
