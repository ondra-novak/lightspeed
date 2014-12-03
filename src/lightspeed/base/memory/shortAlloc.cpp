/*
 * shortAlloc.cpp
 *
 *  Created on: 4.11.2011
 *      Author: ondra
 */



#include "shortAlloc.h"
#include "runtimeAlloc.h"
#include "../types.h"
#include "../iter/sortFilter.tcc"
#include "../memory/rtAlloc.h"
#include "../memory/smallAlloc.h"
#include "../exceptions/pointerException.h"
namespace LightSpeed {




struct ShortAllocCluster::Order2 {
	bool operator()(natural *a, natural *b) const {
		return a[1] < b[1];
	}
};

struct ShortAllocCluster::Order1 {
	bool operator()(natural a, natural b) const {
		return a < b;
	}
};

template<typename SortFilter>
void ShortAllocCluster::collectHoles(natural k, SortFilter & order1)
{
	while(k != naturalNull){
		order1.input(k);
		k = dataArea[k];
	}
}


ShortAllocCluster::ShortAllocCluster(natural *dataArea, natural size) {
	setAllocArea(dataArea,size);
}

bool ShortAllocCluster::isMyPtr(const void *ptr) const {
	const natural *k = reinterpret_cast<const natural *>(ptr);
	return k >= dataArea && k < (dataArea + size);
}

void *ShortAllocCluster::allocBlock(natural sz) {
	natural rsz = (2*sizeof(natural) + sz - 1)/(2*sizeof(natural)) *2;

	usedSpace += rsz;
	//if there is no space, try to allocate in largest hole
	if (rsz <= holeAllocSize) {
		void *out = dataArea + firstHoleAlloc - rsz;
		holeAllocSize -= rsz;
		return out;
	}

	//if there is no space in hole, but is space in firstReadyHole
	if (firstReadyHole != naturalNull && rsz <= dataArea[firstReadyHole+1]) {

		///often can next block match exactly to requested block
		if (rsz ==  dataArea[firstReadyHole+1]) {
			//take whole block and don't drop remain of current firstHoleAlloc
			void *out = dataArea+ firstReadyHole;
			firstReadyHole = dataArea[firstReadyHole];
			holeCount--; //<this takes one hole
			return out;
		} else {

			if (holeAllocSize) {
				//move current hole into droppedHoles
				dataArea[firstHoleAlloc] = droppedHoles;
				dataArea[firstHoleAlloc+1] = holeAllocSize;
				droppedHoles = firstHoleAlloc;
				++holeCount; //<this adds one hole
			}
			//make firstReadyHole allocHole
			firstHoleAlloc = firstReadyHole;
			holeAllocSize = dataArea[firstReadyHole+1];
			firstReadyHole = dataArea[firstReadyHole];
			--holeCount; //this creates new hole

			//allocate in it
			void *out = dataArea + firstHoleAlloc - rsz;
			holeAllocSize -= rsz;
			return out;
		}
	}

	//if there is no space in ready holes but space in lastDealloc
	if (lastDealloc != naturalNull && rsz <= dataArea[lastDealloc+1]) {
		//allocate in it, but on other side.
		void *out = dataArea + lastDealloc;

		//short hole
		natural lastDeallocSz = dataArea[lastDealloc+1] - rsz;
		//if remaining space in the hole
		if (lastDeallocSz > 0) {
			//short space in the hole
			dataArea[lastDealloc+rsz] =dataArea[lastDealloc];
			lastDealloc+=rsz;
			dataArea[lastDealloc+1] = lastDeallocSz;
		} else {
			//if no remaining space in the hole
			//remove hole complete
			--holeCount;
			lastDealloc = dataArea[lastDealloc];
		}
		return out;

	}

	//in that case, we cannot find available space fast.
	//it can be due requested block is large, or there are many holes in lastDelloc
	//or at droppedHoles.
	//Function now returns NULL, to continue allocation, try to reorderHoles() and  allocate
	//again. If doesn't help, you will need to create new cluster for allocation
	usedSpace -= rsz;

	return 0;
}

ShortAllocCluster::ShortAllocCluster()
{
	setAllocArea(0,0);
}

void ShortAllocCluster::setAllocArea(natural *dataArea, natural size)
{
	this->size = (size + 1) & ~(natural)(2);
        this->dataArea = dataArea;
        this->firstReadyHole = naturalNull;
        this->lastDealloc = naturalNull;
        this->firstHoleAlloc = 0;
        this->holeAllocSize = size;
        this->droppedHoles = naturalNull;
        this->usedSpace = 0;
        this->holeCount = 0;
    }

    bool ShortAllocCluster::deallocBlock(void *ptr, natural sz)
    {
        natural rsz = (2 * sizeof (natural) + sz - 1) / (2 * sizeof (natural)) * 2;
        natural *ppos = reinterpret_cast<natural*>(ptr);
        natural pos = ppos - dataArea;
        if(pos >= size || dataArea + pos != ppos || (pos & 1) == 1)
            return false;

        usedSpace -= rsz;
        //returning memory into the hole
        if(firstHoleAlloc + holeAllocSize == pos){
            holeAllocSize += rsz;
            return true;
        }
        //memory lies near to previous dealloc
        if(pos + rsz == lastDealloc){
            dataArea[pos] = dataArea[lastDealloc];
            dataArea[pos + 1] = dataArea[lastDealloc + 1] + rsz;
            lastDealloc = pos;
            return true;
        }
        //memory lies near to previous dealloc
        if(lastDealloc != naturalNull && dataArea[lastDealloc + 1] + lastDealloc == pos){
            dataArea[lastDealloc + 1] += rsz;
            return true;
        }
        //cannot find suitable hole to merge
        //create new hole
        dataArea[pos] = lastDealloc;
        dataArea[pos + 1] = rsz;
        lastDealloc = pos;
        ++holeCount; //this creates new hole
        return true;
    }

    bool ShortAllocCluster::optimizeHoles()
    {
        SmallAlloc<1024> smlal;
        //orders holes by address ascended
        SortFilter<natural,Order1,SmallAlloc<1024> > order1(Order1(), smlal);
        //orders holes by size ascended
        SortFilter<natural*,Order2,SmallAlloc<1024> > order2(Order2(), smlal);
        //scan all deallocated holes
        collectHoles(lastDealloc, order1);
        //scan all dropped holes
        collectHoles(droppedHoles, order1);
        //if no holes collected, exit now - no work to do
        if(!order1.hasItems())
            return false;

        //scan all ready holes - can be merged
        collectHoles(firstReadyHole, order1);
        //remove ready holes
        firstReadyHole = naturalNull;
        //remove dealloc holes
        lastDealloc = naturalNull;
        //remove drop holes
        droppedHoles = naturalNull;
        holeCount = 0;
        //take first hole
        natural k = order1.output();
        //take its size
        natural l = dataArea[k + 1];
        //while there are some holes
        while(order1.hasItems()){
            //take next
            natural p = order1.output();
            //can be merged with previous?
            if(k + l == p){
                //if yes, merge it
                l = l + dataArea[p + 1];
            }else{
                //if not, finalize current hole updating its size
                dataArea[k + 1] = l;
                //and add for ordering by size
                order2.input(dataArea + k);
                //this hole becomes first
                k = p;
                //and take its size
                l = dataArea[p + 1];
            }
        }

        //now we have merged holes ordered by size ascended
        //take each holde
        while(order2.hasItems()){
            natural *k = order2.output();
            //create chain in reverse order (largest will be first)
            *k = firstReadyHole;
            firstReadyHole = k - dataArea;
            ++holeCount;
        }
        //done, retrn status
        return true;
    }

    SimpleShortAlloc::SimpleShortAlloc(IRuntimeAlloc & alloc, natural size)
    :buffer(RTAlloc(alloc))
    {
        natural nsize = (size + 2 * sizeof (natural) - 1) / 2 * sizeof (natural);
        buffer.resize(nsize);
        cluster.setAllocArea(buffer.data(), nsize);
    }

    void *SimpleShortAlloc::alloc(natural objSize)
    {
        void *p = cluster.allocBlock(objSize);
        if (p == 0) {
		cluster.optimizeHoles();
		p = cluster.allocBlock(objSize);
		if (p == 0)
			throwAllocatorLimitException(THISLOCATION,objSize,cluster.getLargestFree(),typeid(SimpleShortAlloc));
	}
        return p;
    }

    void SimpleShortAlloc::dealloc(void *ptr, natural objSize)
    {
        cluster.deallocBlock(ptr, objSize);
    }

    ShortAllocCluster::ClusterStats ShortAllocCluster::getStats()
    {
        ClusterStats res;
        res.totalFree = holeAllocSize;
        res.largestFree = getLargestFree();
        res.unusableFree = 0;
        res.unusableFragments = 0;
        res.totalFragments = 0;
        natural k = firstReadyHole;
        while(k != naturalNull){
            res.totalFree += dataArea[k + 1];
            ++res.totalFragments;
            k = dataArea[k];
        }
        k = lastDealloc;
        while(k != naturalNull){
            res.unusableFree += dataArea[k + 1];
            ++res.unusableFragments;
            k = dataArea[k];
        }
        k = droppedHoles;
        while(k != naturalNull){
            res.unusableFree += dataArea[k + 1];
            ++res.unusableFragments;
            k = dataArea[k];
        }
        res.totalFree += res.unusableFree;
        res.totalFragments += res.unusableFragments;
        res.fragmentationRatio = res.totalFragments * 10000 / (res.totalFree / 2);
        res.totalFree *= sizeof (natural);
        res.unusableFree *= sizeof (natural);
        if(res.totalFragments)
            res.avgHoleSize = res.totalFree / res.totalFragments;

        else
            res.avgHoleSize = 0;

        return res;
    }
    natural ShortAllocCluster::getLargestFree() const
    {
        natural hsz = holeAllocSize;
        if(firstReadyHole != naturalNull && dataArea[firstReadyHole + 1] > hsz)
            hsz = dataArea[firstReadyHole + 1];

        return hsz * sizeof (natural);
    }
    void SimpleShortAlloc::throwPointerException(const ProgramLocation & loc, const void *ptr)
    {
        throw InvalidPointerException(loc, ptr);
    }

    ShortAlloc::ShortAlloc(IRuntimeAlloc *clusterAlloc,
    					   natural minClusterSize, natural maxClusterSize,
    					   natural sizeLimitRatio)
    	:nextClusterSize(minClusterSize)
    	,maxClusterSize(maxClusterSize)
    	,sizeLimitRatio(sizeLimitRatio)
    	,clusterExpand(minClusterSize)
    	,calloc(clusterAlloc)
    {
    	if (calloc == 0) calloc = &StdAlloc::getInstance();
    }

    void *ShortAlloc::alloc(natural objSize)
    {
        IRuntimeAlloc *owner;
        return alloc(objSize, owner);
    }

    void *ShortAlloc::alloc(natural objSize, IRuntimeAlloc *& owner)
    {
    	void *ptr = 0;
    	if (objSize > nextClusterSize * 10 / sizeLimitRatio) {
    		return calloc->alloc(objSize,owner);
    	}
    	if (current == nil) {
    		if (top != nil) {
    			current = top;
    			top = current->next;
    			current->removeFromList();
    		} else {
    			current = addCluster();
    		}
    	}
   		ptr = current->alloc(objSize);
   		if (ptr) {
   			owner = current;
   			return ptr;
   		}

    	current->insertBefore(top);
    	top = current;
    	clusterUpdatePos(top);


    	PSubCluster x = addCluster();
    	ptr = x->alloc(objSize);
    	if (ptr == 0)
    		throwOutOfMemoryException(THISLOCATION,objSize);
    	current = x;
    	return ptr;
    }

    void ShortAlloc::dealloc(void *ptr, natural objSize)
    {
    	if (objSize > nextClusterSize * 10 / sizeLimitRatio) {
    		return calloc->dealloc(ptr,objSize);
    	}
    	//implementation can be slow.
    	//It is better, when caller use alloc with two parameters whether
    	//pointer to subcluster is returned

    	SubCluster *t = top;
    	while (t != 0) {
    		if (t -> isMyPtr(ptr)) {
    			t->dealloc(ptr,objSize);
    			return;
    		}
    		t = t->next;
    	}
    	//did not found, probably allocated by
    	calloc->dealloc(ptr,objSize);
    }

    ShortAlloc::~ShortAlloc()
    {
    	if (top != nil) {
    		top->unchain();
    	}
    }


void *ShortAlloc::SubCluster::subAlloc(natural objSize)
{
	void *ptr = this->allocBlock(objSize);
	if (ptr == 0) {
		this->optimizeHoles();
		ptr = this->allocBlock(objSize);
	}
	return 0;
}

void *ShortAlloc::SubCluster::alloc(natural objSize, IRuntimeAlloc *&owner) {

	return this->owner.alloc(objSize,owner);

}

void *ShortAlloc::SubCluster::alloc(natural objSize) {
	return this->owner.alloc(objSize);
}


void ShortAlloc::SubCluster::dealloc(void *ptr, natural objSize)
{
	if (isMyPtr(ptr)) {
		deallocBlock(ptr,objSize);
		owner.blockDeallocNotify(this);
	} else {
		return owner.dealloc(ptr,objSize);
	}

}



void ShortAlloc::SubCluster::insertBefore(RefCntPtr<SubCluster> b)
{
	if (b == nil) {
		next = prev = nil;
	} else {
		next = b;
		prev = b->prev;
		b->prev = this;
		if (prev != nil) prev->next = b;
	}
}



void ShortAlloc::SubCluster::moveTop()
{
	if (prev != nil) {
		PSubCluster x = prev;
		removeFromList();
		insertBefore(x);
	}
}



void ShortAlloc::SubCluster::moveBottom()
{
	if (next != nil) {
		PSubCluster x = next;
		x->removeFromList();
		x->insertBefore(this);

	}
}



void ShortAlloc::SubCluster::unchain()
{
	if (next) next->unchain();
	if (prev) prev->unchain();
	next = nil;
	prev = nil;
}

void ShortAlloc::SubCluster::removeFromList() {
	if (next) next->prev = prev;
	if (prev) prev->next = next;
	next = nil;
	prev = nil;
}


void ShortAlloc::clusterUpdatePos(PSubCluster cluster) {

	while (cluster->prev != nil
			&& cluster->getFreeSpace() > cluster->prev->getFreeSpace()) {
		cluster->moveTop();
	}
	while (cluster->next != nil
			&& cluster->getFreeSpace() < cluster->next->getFreeSpace()) {
		cluster->moveBottom();
	}
	if (cluster->prev == nil) {
		top = cluster;
	}
}

ShortAlloc::PSubCluster ShortAlloc::addCluster() {
	PSubCluster cluster = new(*calloc) PolyAlloc<IRuntimeAlloc, SubCluster>(this);
	cluster->allocDataArea(calloc,nextClusterSize);
	nextClusterSize += clusterExpand;
	if (nextClusterSize > maxClusterSize) nextClusterSize = maxClusterSize;
	return cluster;
}


void ShortAlloc::SubCluster::allocDataArea(IRuntimeAlloc *allocator, natural size) {
	if (this->dataArea) {
		this->allocator->dealloc(this->dataArea,this->size * sizeof(natural));
	}
	this->allocator = allocator;
	natural *data = (natural *)this->allocator->alloc(size * sizeof(natural));
	setAllocArea(data,size);
}

void ShortAlloc::blockDeallocNotify(PSubCluster cluster) {
	if (cluster != current) {
		if (cluster->getUsedSpace() == 0) {
			if (cluster == top) cluster = cluster->next;
			cluster->removeFromList();
		} else {
			clusterUpdatePos(cluster);
		}
	}
}

ShortAlloc::SubCluster::~SubCluster() {
	if (this->dataArea)
		this->allocator->dealloc(this->dataArea,this->size * sizeof(natural));
}

// namespace LightSpeed
}
