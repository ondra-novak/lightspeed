/* * shortAlloc.h
 *
 *  Created on: 4.11.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_MEMORY_SHORTALLOC_H_
#define LIGHTSPEED_BASE_MEMORY_SHORTALLOC_H_
#include "../types.h"
#include "runtimeAlloc.h"
#include "../containers/autoArray.h"
#include "../memory/rtAlloc.h"
#include "refCntPtr.h"


namespace LightSpeed {


class ShortAllocCluster {
protected:
	///cluster size in naturals
	natural size;
	///data area
	natural *dataArea;
	///index of first hole available for allocations
	/** Every hole uses 2 addresses of empty space. First address contains index of next hole
	 * Second address is used to store size of the hole
	 */
	natural firstReadyHole;
	///index of last hole created by deallocation
	natural lastDealloc;
	///ready hole used previously for allocations
	natural firstHoleAlloc;
	///how many bytes can be allocated in the first hole
	natural holeAllocSize;
	///list of dropped holes during allocation
	natural droppedHoles;
	///total natural used for allocations
	natural usedSpace;
	///total count of holes;
	natural holeCount;

	struct Order2;
	struct Order1;
	template<typename SortFilter>
	void collectHoles(natural k, SortFilter & order1);
public:
	///construct cluster in area
	/**
	 * @param dataArea pointer to data area
	 * @param size size of are in naturals
	 */
	ShortAllocCluster(natural *dataArea, natural size);

	///construct empty cluster which can be initialized later
	ShortAllocCluster();

	///Sets allocation area
	/**
	 * @param dataArea pointer to new area
	 * @param size of area
	 *
	 * @note function resets state of object, deallocates all space and removes all holes
	 */
	void setAllocArea(natural *dataArea, natural size);


	///Allocates block in cluster
	/**
	 * @param sz required size. Size can be aligned to largest paragraph.
	 *   Size of paragraph is 2*sizeof(natural)
	 *  @return pointer to allocated space. Function returns NULL, when it
	 *   cannot find available hole to allocation. Function does not
	 *   handle hole optimization, so you should repeat allocation after
	 *   calling optimizeHoles() when first allocation fails
	 *
	 *   @note always remember allocated size
	 */



	void *allocBlock(natural sz);
	///Deallocates block
	/**
	 * @param ptr pointer to allocated block. Must be exactly the same
	 * value as value returned by allocBlock. Using different pointer causes
	 * memory corruption and application crash
	 * @param sz size of allocated block. Must be exactly the same
	 * value as value used with the allocBlock()
	 *
	 * @retval false possible invalid pointer or size, but test arguments is
	 * very simple
	 * @retval true deallocated
	 */
	bool deallocBlock(void *ptr, natural sz);
	///Optimizes unallocated space (holes)
	/** cluster doesn't optimize unallocated space by default. You have to
	 * call this function manually when allocation fails. Function is faster
	 * then hole optimization provided after each deallocation, because
	 * all holes are optimized at once. But optimization can take
	 * some time, especially when fragmentation is very heavy leading to
	 * many small unconnected holes.
	 *
	 *
	 * @retval true optimization successful
	 * @retval false nothing to optimize
	 */
	bool optimizeHoles();

	struct ClusterStats {
		///total free bytes
		natural totalFree;
		///total count of fragmens of free memory
		natural totalFragments;
		///largest free continous block
		natural largestFree;
		///total bytes unusable for allocation without optimization
		natural unusableFree;
		///total count of fragmens unusable for allocation without optimization
		natural unusableFragments;
		///average size of hole
		natural avgHoleSize;
		///rating of fragmentation 10000 - (avgHoleSize/totalFree * 10000)
		natural fragmentationRatio;

	};

	ClusterStats getStats();

	///Retrieves larges available space for next allocation
	/**
	 * @return largest available space. Function cannot see unusable memory
	 * so result can be different after optimizeHoles()
	 */
	natural getLargestFree() const;

	///Retrieves count of 'naturals' used for allocation
	natural getUsedSpace() const {return usedSpace;}

	///Returns total size of alloc area in 'naturals'
	natural getSize() const {return size;}

	///Returns free space in `naturals`
	natural getFreeSpace() const {return size - usedSpace;}

	///Calculates count of holes
	/**
	 * @return Function may return zero event if there is space available
	 * for allocations. That because current hole locked for allocations
	 * is not counted. But can appear in hole list when space in the hole
	 * cannot be used for allocations
	 */
	natural getHoleCount() const {return holeCount;}

	///Calculates average size of one hole
	/**
	 * @return
	 */
	natural getAvgHoleSize() const {
		return (size - usedSpace)/(holeCount+1);
	}

	///Returns true, if pointer points to an allocated space in this cluster
	/**
	 * @param ptr pointer to test
	 * @retval true pointer probably points to block allocated by this cluster
	 * @retval false pointer is not pointing to any block allocated by this cluster
	 *
	 * @note method is safe, you can test any pointer including NULL and invalid pointer
	 */
	bool isMyPtr(const void *ptr) const;

};


	///Simple short-term allocator for small object various size
	/**
	 * Creates one cluster with reserved space and use this cluster for
	 * carry out allocation requests. Throws exception when space is exhausted
	 */
	class SimpleShortAlloc: public IRuntimeAlloc {
	public:

		///Constructs instance of SimpleShortAlloc
		/**
		 * @param alloc allocator used to allocate whole heap
		 * @param size size of heap in 'naturals' Each natural has
		 * 4 bytes on 32 bit platform an 8 bytes on 64 bit platform.
		 *
		 * Allocator rounds size to higher even number, because every
		 * block is aligned to 2 naturals
		 */
		SimpleShortAlloc(IRuntimeAlloc &alloc, natural size);

		virtual void *alloc(natural objSize);
		virtual void dealloc(void *ptr, natural objSize);

		static void throwPointerException(const ProgramLocation &loc, const void *ptr);

	protected:
		ShortAllocCluster cluster;
		AutoArray<natural, RTAlloc> buffer;

	};

	///Short-term allocator create with static buffer
	/** can be created on stack, or inside of object. Memory is reserved
	 * inside of instance. Size is specified as template argument
	 *
	 * @tparam size size of the heap in 'naturals'. Number is
	 * rounded up to first even number
	 */
	template<natural size>
	class StaticShortAlloc: public IRuntimeAlloc {

		static const natural nsize = (size + 1) & ~(natural)2;
	public:

		StaticShortAlloc() {
			cluster.setAllocArea(buffer,nsize);
		}

		virtual void *alloc(natural objSize) {
			void *ptr = cluster.allocBlock(objSize);
			if (ptr == 0) {
				cluster.optimizeHoles();
				ptr = cluster.allocBlock(objSize);
				if (ptr == 0) throwAllocatorLimitException(THISLOCATION,objSize,cluster.getLargestFree(),typeid(*this));
			}
			return ptr;
		}

		virtual void dealloc(void *ptr, natural objSize) {
			if (ptr && objSize && !cluster.deallocBlock(ptr,objSize))
				SimpleShortAlloc::throwPointerException(THISLOCATION,ptr);
		}

	protected:
		natural buffer[nsize];
		ShortAllocCluster cluster;

	};


	///More general short-term allocator, which uses multiple cluster to satisfy allocation requests
	/**
	 * Creates additional clusters when first cluster is full. There is always
	 * active cluster, where allocation is satisfied until cluster is full. Then
	 * cluster is put into pool and cluster with some empty space is taken
	 * for next allocations. If allocation fails, cluster is put to the pool
	 * and nxt cluster is given. Only if allocation fails twice, new cluster
	 * is created
	 *
	 */

	class ShortAlloc: public IRuntimeAlloc {
	public:

		///constructs instance
		/**
		 * @param clusterAlloc pointer to allocator that will be used to allocate
		 *        clusters. Set NULL to use StdAlloc
		 * @param minClusterSize minimal cluster size
		 * 		- allocator starts to create this clusters. When each cluster
		 * 		is full, next cluster is larger about the same size. For example
		 * 		specify 1024 as minCluster creates first cluster with size 1024,
		 * 		next will have 2048, next will have 3072, etc.
		 * 		Number is in 'naturals'. Each natural has 4 or 8 bytes depend
		 * 		on platform.
		 *
		 * @param maxClusterSize maximum cluster size. Allocator will never
		 * 		create larger clusters
		 *
		 * @param sizeLimitRatio specifies how large objects will be allocated
		 * 		by the each cluster. Larger object will be allocated directly
		 * 		by parent allocator. Default value 20 means, that largest object
		 * 		to allocate in the cluster is half of its size. Limit is calculated
		 * 		using the formula : clusterSize * 10 / sizeLimitRatio. Parameter
		 * 		cannot be less than 10.
		 *
		 */
		ShortAlloc(IRuntimeAlloc *clusterAlloc = 0,
					natural minClusterSize=1024,
				    natural maxClusterSize=65536,
				    natural sizeLimitRatio=20);


		virtual void *alloc(natural objSize);
		virtual void *alloc(natural objSize, IRuntimeAlloc *& owner);
		virtual void dealloc(void *ptr, natural objSize);

		virtual ~ShortAlloc();
	protected:

		class ssh : public RefCntObj,
						  public IRuntimeAlloc,
						  public ShortAllocCluster {

		public:
			virtual void *alloc(natural objSize);
			virtual void *alloc(natural objSize, IRuntimeAlloc *&owner);
			virtual void dealloc(void *ptr, natural objSize);

			void *subAlloc(natural objSize);


			RefCntPtr<SubCluster> prev,next;
			void insertBefore(RefCntPtr<SubCluster> b);
			void moveTop();
			void moveBottom();
			void unchain();
			void removeFromList();
			void allocDataArea(IRuntimeAlloc *alloc, natural size);


			ShortAlloc &owner;
			IRuntimeAlloc *allocator;
			SubCluster(ShortAlloc *owner):owner(*owner) {}
			~SubCluster();

		};

		typedef RefCntPtr<SubCluster> PSubCluster;
		PSubCluster top;
		PSubCluster current;

		natural nextClusterSize;
		natural maxClusterSize;
		natural sizeLimitRatio;
		natural clusterExpand;
		IRuntimeAlloc *calloc;

		void clusterUpdatePos(PSubCluster cluster);
		PSubCluster addCluster();
		void blockDeallocNotify(PSubCluster cluster);
	};
}


  // namespace LightSpeed

#endif /* LIGHTSPEED_BASE_MEMORY_SHORTALLOC_H_ */
