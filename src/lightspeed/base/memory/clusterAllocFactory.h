#pragma once
#include "factory.h"
#include "clusterAlloc.h"
#include "stdFactory.h"

namespace LightSpeed {


	class ClusterAllocFactory_Base: public SharedResource {
	public:

		void *alloc(natural objSize) {
			ClusterAlloc *aa = getAlloc();
			return aa->alloc(objSize);
		}
		void dealloc(void *ptr, natural objSize) {
			a->dealloc(ptr,objSize);
		}

		ClusterAllocFactory_Base(IRuntimeAlloc &alloc) {
			a = new(alloc) ClusterAlloc(alloc);
		}
		
		ClusterAllocFactory_Base() {}

		~ClusterAllocFactory_Base() {
			if (!isShared()) delete a.get();
		}

		void setAllocator(IRuntimeAlloc &alloc) {
			if (a != nil) delete a.get();
			a = new(alloc) ClusterAlloc(alloc);
			SharedResource::forEachInstance(&updatePtr);
		}

		IRuntimeAlloc *getRTAlloc() {return getAlloc();}

	protected:
		Pointer<ClusterAlloc> a;

		ClusterAlloc * getAlloc() 
		{
			if (a == nil) {
				a = new ClusterAlloc(StdAlloc::getInstance());
				SharedResource::forEachInstance(&updatePtr);
			}
			return a;
		}

		static bool updatePtr(SharedResource *a, SharedResource *b) {
			ClusterAllocFactory_Base *ca  = static_cast<ClusterAllocFactory_Base *>(a);
			ClusterAllocFactory_Base *cb  = static_cast<ClusterAllocFactory_Base *>(b);
			ca->a = cb->a;
			return true;
		}


	};

	class ClusterAllocFactory: public ClusterAllocFactory_Base {
	public:

		ClusterAllocFactory(IRuntimeAlloc &alloc):ClusterAllocFactory_Base(alloc) {
		}
		ClusterAllocFactory() {}

		template<typename T>
		class Factory: public FactoryBase<T,Factory<T> > {
		public:

			Factory(const ClusterAllocFactory_Base &master):master(master) {}

			void *alloc(natural objSize) {
				return master.alloc(objSize);
			}

			void dealloc(void *ptr, natural objSize) {
				master.dealloc(ptr,objSize);
			}

		protected:
			ClusterAllocFactory_Base master;
		};
	};

}
