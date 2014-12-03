/*
 * allocatedMemory.h
 *
 *  Created on: 1.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ALLOCATEDMEMORY_H_
#define LIGHTSPEED_ALLOCATEDMEMORY_H_

#include "../types.h"

namespace LightSpeed {


	template<typename T>
	class AllocatedMemory {
	public:


		AllocatedMemory() {}
		///Retrieves base address of allocated memory
		/**
		 * @return address of allocated memory
		 */
		T *getBase() const;

		///Retrieves size of allocated memory
		/**
		 * @return size of allocated memory in items
		 */
		natural getSize() const;


		///Expands allocated memory to required items
		/**
		 * @param items items required
		 * @retval true expanded.
		 * @retval false cannot expand, you have to create new block and perform copy
		 * @note if argument items is less than current size, function returns true and does nothing
		 */
		bool expand(natural items);


		///Shrinks allocated memory to required items
		/**
		 * @param items required size
		 * @retval true shrink successes
		 * @retval false shrink doesn't work, size is unchanged
		 */
		bool shrink(natural items);

		///swaps blocks
		/**
		 * @param other block
		 * @retval true swapped
		 * @retval false swap is not supported or cannot be handled fast (moving inside objects will be faster)
		 */
		bool swap(AllocatedMemory &other);

		///Moves current block to target object
		/**
		 * @param target target object. The content of object will be destroyed
		 * @retval true moved
		 * @retval false not supported or cannot be handled fast (moving inside objects will be faster)
		 *
		 * @note in compare to swap(), function move() can be optimized, that
		 *  only data from this object to target object will be moved, not
		 *  other direction. Fully swapable objects will implement this function
		 *  using swap(), but some objects (such a SmallAlloc) can
		 *  implement this function differently and may return both possible
		 *  result depend on current state.
		 */
		bool move(AllocatedMemory &target);



		///Creates new instance using parameters by other instance, but with different size
		AllocatedMemory(const AllocatedMemory &src, natural items);


	};

	class ShareableAllocatedMemory {
	public:

		enum Share {share};

		ShareableAllocatedMemory() {}

		ShareableAllocatedMemory(const ShareableAllocatedMemory &other, Share share);

		bool isShared() const;
	};

	template<typename T>
	class DetachableAllocatedMemory {
	public:


		T *detach();

		DetachableAllocatedMemory(const AllocatedMemory<T> &anotherBlock, T *bk, natural sz);

		DetachableAllocatedMemory() {}


	};


}


#endif /* LIGHTSPEED_ALLOCATEDMEMORY_H_ */
