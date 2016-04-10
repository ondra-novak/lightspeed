#pragma once

#include "../containers/avltreenode.h"
#include "runtimeAlloc.h"
#include "dynobject.h"
#include "../containers/autoArray.h"
#include "../exceptions/memoryleakexception.h"


namespace LightSpeed {

	///One cluster for ClusterAlloc
	/** memory is allocated cluster by cluster. Each cluster 
	 * can have up to 255 objects, with predefined size. Allocator
	 * can alloc smallest objects with size equal 1 with very little
	 * overhead.
	 */
	class AllocCluster: public IRuntimeAlloc, 
						public AvlTreeNode<char> {
	public:

		virtual natural getObjectSize() const {return owner->getObjectSize();}
		virtual IRuntimeAlloc *clone(IRuntimeAlloc &alloc) const {return owner->clone(alloc);}
		virtual IRuntimeAlloc *clone() const {return owner->clone();}


		///master object
		class IMaster: public IRuntimeAlloc {
		public:
			virtual void onDealloc(AllocCluster *cluster, bool wasFull) = 0;			
		};
		
		///Creates new cluster
		/**
		 * @param alloc reference to allocator used to allocate clusters
		 * memory area
		 * @param count maximum count of objects in this cluster
		 * @param bksize size of one object. Do not pass value returned
		 * from sizeof() operator directly. Always use objSize2bksize() 
		 * function to adjust this value to match current address alignment
		 */
		static AllocCluster *create(IRuntimeAlloc &alloc, 
			IMaster *cb, byte count, natural bksize);

		///Retrieves total count of bytes need by cluster
		/**
		 * @param count count of objects in the cluster
		 * @oaram bksize size of block (use objSize2bksize to adjust)
		 * @return count of bytes need to allocate for such a cluster
		 */
		 

		static natural getMemoryUsage( byte count, natural bksize );

		///Retrieves total count of bytes allocated by this cluster
		/**
		 * @param size of block (which is not stored with cluster
		 * @return size in bytes
		 */

		natural getMemoryUsage(natural bksize) {
			return getMemoryUsage(count,bksize);
		}

		///Allocates in the cluster
		/**
		 * @param bksize, size of block - same value used to create 
		 		cluster - function create()
		   @return pointer to allocated block, or NULL, when cluster is full
		 */
		 
		void *makeAlloc(natural bksize);

		///Deallocates the block
		/**
		 * @param pointer to block
		 * @param bksize, size of block - same value used to create
				cluster - function create()
			Function removes allocation for this block
		 */
		 
		void makeDealloc(void *ptr, natural bkSize);

		virtual void *alloc(natural objSize, IRuntimeAlloc * &owner) {
			owner = this;
			return alloc(objSize);
		}
		///Allocates object
		/** Cluster cannot provide allocation through IRuntimeAlloc,
		 * but resends this request to the master object 
		 */
		virtual void *alloc(natural objSize);
		 
		///Deallocates object
		/** Function can be called only for blocks which has been
		  * allocated in this cluster. Using function for blocks
		  * not allocated by this cluster can lead to unpredictable
		  * results. It can also throw exception InvalidPointerException
		  *
		  * @param ptr pointer to block
		  * @param sz size of block
		  */
		virtual void dealloc(void *ptr, natural sz);

		///Tests, whether pointer is allocated by this cluster
		/** @param ptr pointer to allocated block
		* @param bksize size of block - same value used to create
		cluster - function create()
		* @retval true, pointer is probably allocated by this cluster
		* @retval false, pointer is not allocated by this cluster
		*/
		bool isMyPtr(void *ptr, natural bksize) const;
		///returns true, when cluster is full
		bool isFull() const {return firstFree == 255;}
		///return true, when cluster is empty
		bool isEmpty() const {return usedCount == 0;}

		///adjusts size of the object to block size (bksize)
		/**
		 * Function adjusts size of object to match alignment
		 * objects of size 1,2,4,8,16,32.. bytes don't need to be adjusted
		 * objects of size 3 bytes are adjusted to size 4
		 * objects of size 5,6 or 7 bytes are adjusted to 8
		 * objects of size 9,10,11, are adjusted to 12 for 32 bits
		 * or  16 bytes for 64 bits.
		 * other sizes are adjusted up to first number divided by 4 or 8
		 * depend on platform.
		 * 
		 */
		static natural objSize2bksize( natural objSize);

		///destroys cluster
		void destroy(natural bksize);

		///variable is used to make linked list of free clusters
		AllocCluster *llnext;


		void reportLeaks(AutoArrayStream<MemoryLeakException::Leak> &list, natural bksize);
	protected:
		AllocCluster(IRuntimeAlloc *alloc, IMaster *master, byte count, natural bksize);
		byte * mapIndex( byte idx, natural bksize );

//		void destroy();
		byte mapAddress( void * ptr , natural bksize);

		IRuntimeAlloc *owner;
		IMaster *master;
		byte usedCount;
		byte firstFree;
		byte count;

	};


	 class AllocMulticlusterNode: public AvlTreeNode<char> {
	 public:
		 AllocMulticlusterNode(natural bkSize):AvlTreeNode<char>(0), bkSize(bkSize) {}
		 natural getBlockSize() const {return bkSize;}
	 protected:
		natural bkSize;

	 };

	 ///Class which handles allocation of objects of the same size 
	/**
	 * Class is collection of multiple clusters. It initializes with specified
	 * size and only object with this size can be allocated. 
	 * 
	 */
	 
	class AllocMulticluster: public AllocCluster::IMaster, 
							public AllocMulticlusterNode,
							public DynObject {
	public:
		


		AllocMulticluster(IRuntimeAlloc &topMaster, IRuntimeAlloc &alloc, natural bkSize)
			:AllocMulticlusterNode(bkSize)
			,rtalloc(alloc)
			,topMaster(topMaster)
			,nextCnt(byte(1 + (sizeof(AllocCluster) + bkSize/2)/bkSize)) {}
		
		void *alloc(IRuntimeAlloc *&owner);
		void dealloc(void *ptr);

		AllocMulticluster(const AllocMulticluster &other);

		~AllocMulticluster();
		

		void reportLeaks(AutoArrayStream<MemoryLeakException::Leak> &list);

		LIGHTSPEED_CLONEABLECLASS;
	protected:
		struct PtrNodeCmp {
			bool operator()(const AvlTreeNode<char> *a, const AvlTreeNode<char> *b) const;
		};

		virtual void onDealloc(AllocCluster *cluster, bool wasFull) ;
		virtual void *alloc(natural objSize, IRuntimeAlloc * &owner) ;
		void *alloc(natural size);
		void dealloc(void *ptr, natural size);
		void checkEmpty(AllocCluster *k);

			///tree of all clusters ordered by its address
		AvlTreeBasic<PtrNodeCmp, char> allClusters;
			///linked list of cluster where is at least one empty space
		Pointer<AllocCluster> freeClusters;

		///size of every block
		IRuntimeAlloc &rtalloc;
		IRuntimeAlloc &topMaster;
		byte nextCnt;
		class TreeEraser;

	};

	///Allocates objects in clusters
	/**
	 * Complete allocator, which can make allocations using clusters. This
	 * is very effecient, when there is a lot of small objects of the same size.
	 * It is also able to allocate 1-byte or 2-byte objects without any extra overhead
	 * 
	 * ClusterAlloc extends interface IRuntimeAlloc, so you can use it with
	 * DynObject. It is also DynObject, so it can be allocated by another IRuntimeAlloc
	 * 
	 * To use ClusterAlloc as factory, see ClusterAllocFactory
	 */
	 
	class ClusterAlloc: public IRuntimeAlloc, public DynObject  {
	public:
		LIGHTSPEED_CLONEABLECLASS
		///Constructs allocator
		/**
		 * @param allocator to allocate clusters		 
		 */
		 
		ClusterAlloc(IRuntimeAlloc &rtalloc);
		ClusterAlloc();

		void *alloc(natural objSize, IRuntimeAlloc * &owner);
		void *alloc(natural objSize);
		void dealloc(void *ptr, natural objSize);
		///Deletes all clusters
		/**
		 * You can call this function after all blocks are removed, otherwise
		 * MemoryLeakException is thrown
		 */
		 
		void clear();

		~ClusterAlloc();

		ClusterAlloc(const ClusterAlloc &alloc);

	protected:

		struct McCmp {
			bool operator()(const AvlTreeNode<char> *a, const AvlTreeNode<char> *b) const;
		};

		IRuntimeAlloc &rtalloc;
		AvlTreeBasic<McCmp,char> tree;

	private:
		ClusterAlloc &operator=(ClusterAlloc &other);
		static void treeEraser(AvlTreeNode<char> *nd);
	};


}
