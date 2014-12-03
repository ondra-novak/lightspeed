#include "clusterAlloc.h"
#include "../exceptions/pointerException.h"
#include "../exceptions/invalidParamException.h"
#include "staticAlloc.h"
#include "../containers/autoArray.tcc"

namespace LightSpeed {

static const natural allocClusterSize 
	= (sizeof(AllocCluster) + sizeof(natural) - 1) & (~(sizeof(natural) -1));


AllocCluster::AllocCluster(IRuntimeAlloc *alloc, IMaster *master, byte count, natural bksize)
:llnext(0),owner(alloc),master(master),usedCount(0),firstFree(0),count(count)
{
	for (byte i = 1; i < count; i++) {
		byte *b = mapIndex(i-1, bksize);
		*b = i;
	}
	*mapIndex(count - 1, bksize) = 255;
}


AllocCluster * AllocCluster::create( IRuntimeAlloc &alloc, IMaster *cb, byte count, natural bksize )
{
	natural totalsz = getMemoryUsage(count, bksize);
	IRuntimeAlloc *owner;
	void *p = alloc.alloc(totalsz,owner);
	AllocCluster *k = new(p) AllocCluster(owner,cb,count,bksize);
	return k;	
}

natural AllocCluster::objSize2bksize( natural objSize )
{
	natural fs = sizeof(void *);
	while (objSize < fs) fs>>=1;
	fs--;
	return (objSize + fs) & (~fs);
}

byte * AllocCluster::mapIndex( byte idx, natural bksize )
{
	byte *k = reinterpret_cast<byte *>(this);
	k += allocClusterSize;
	k += idx * bksize;
	return k;
}

void * AllocCluster::alloc( natural objSize )
{
	IRuntimeAlloc *owner;
	return master->alloc(objSize,owner);

}

void AllocCluster::dealloc( void *ptr, natural sz )
{
	natural bksize = objSize2bksize(sz);
	if (!isMyPtr(ptr,bksize)) throw InvalidPointerException(THISLOCATION,ptr);
	bool wasFull = isFull();
	makeDealloc(ptr,sz);
	master->onDealloc(this,wasFull);
}

byte AllocCluster::mapAddress( void * ptr , natural bksize )
{
	byte *a = reinterpret_cast<byte *>(ptr);
	byte *zs = reinterpret_cast<byte *>(this) + allocClusterSize;
	return byte((a - zs) / bksize);
}

bool AllocCluster::isMyPtr( void *ptr, natural bksize ) const
{
	const byte *a = reinterpret_cast<const byte *>(ptr);
	const byte *zs = reinterpret_cast<const byte *>(this) + allocClusterSize;
	return a >= zs && a < zs + count * bksize;
	
}

void * AllocCluster::makeAlloc( natural bksize )
{
	if (firstFree == 255) return 0;
	byte *z = mapIndex(firstFree,bksize);
	firstFree = *z;
	usedCount++;
	return z;

}

void AllocCluster::makeDealloc( void *ptr, natural bksize )
{
	byte i = mapAddress(ptr,bksize);
	byte *k = reinterpret_cast<byte *>(ptr);
	*k = firstFree;
	firstFree = i;
	usedCount--;

}

void AllocCluster::destroy(natural bksize)
{
	natural s = getMemoryUsage(bksize);
	IRuntimeAlloc &alloc = *owner;
	this->~AllocCluster();
	alloc.dealloc(this,s);	
}

LightSpeed::natural AllocCluster::getMemoryUsage( byte count, natural bksize )
{
	return allocClusterSize + count * bksize;
}

void AllocCluster::reportLeaks( AutoArrayStream<MemoryLeakException::Leak> &list, natural bksize)
{
	AutoArray<bool, StaticAlloc<256> > useMap;
	useMap.resize(count,true);
	byte z= firstFree;
	while (z != 255) {
		if (useMap(z) == false) break;
		useMap(z) = false;
		z = *mapIndex(z,bksize);
	}
	for (natural i = 0; i < useMap.length(); i++) {
		if (useMap[i]) list.write(MemoryLeakException::Leak(mapIndex(byte(i),bksize),bksize,i));
	}
}

void * AllocMulticluster::alloc(IRuntimeAlloc *&owner)
{	
	if (freeClusters == nil) {
		AllocCluster *k = AllocCluster::create(rtalloc,this,nextCnt,bkSize);
		if (nextCnt < 255) nextCnt++;
		allClusters.insert(k);
		freeClusters = k;
	}
	void *res = freeClusters->makeAlloc(bkSize);
	owner = freeClusters;
	checkEmpty(freeClusters);
	if (freeClusters->isFull()) {
		freeClusters = freeClusters->llnext;
	}

	return res;
}

void * AllocMulticluster::alloc( natural objSize, IRuntimeAlloc * &owner )
{
	natural objbksize = AllocCluster::objSize2bksize(objSize);
	if (objbksize == bkSize) return alloc(owner);
	else return topMaster.alloc(objSize,owner);
}

void * AllocMulticluster::alloc( natural size )
{
	IRuntimeAlloc *owner;
	return alloc(size,owner);
}
void AllocMulticluster::dealloc( void *ptr )
{
	if (freeClusters != nil && freeClusters->isMyPtr(ptr,bkSize)) {		
		freeClusters->makeDealloc(ptr,bkSize);
		onDealloc(freeClusters,false);
		
	} else {
		const AvlTreeNode<> *fakeNd = reinterpret_cast<const AvlTreeNode<> *>(ptr);
		AvlTreeBasic<PtrNodeCmp>::Iterator iter = allClusters.seek(*fakeNd,Direction::backward,0);
		if (iter.hasItems()) {
			AllocCluster::PNode nd =  iter.getNext();
			AllocCluster *k = static_cast<AllocCluster *>(nd);
			if (!k->isMyPtr(ptr,bkSize)) throw InvalidPointerException(THISLOCATION,ptr);
			bool wasFull = k->isFull();
			k->makeDealloc(ptr,bkSize);
			onDealloc(k,wasFull);
		}
	}
}

void AllocMulticluster::dealloc( void *ptr, natural size )
{
	natural objbksize = AllocCluster::objSize2bksize(size);
	if (objbksize == bkSize) return dealloc(ptr);
	else return topMaster.dealloc(ptr,size);
}

void AllocMulticluster::onDealloc( AllocCluster *cluster, bool wasFull )
{
	if (wasFull) {
		cluster->llnext = freeClusters;
		freeClusters = cluster;
	}
	checkEmpty(cluster);
}

void AllocMulticluster::checkEmpty(AllocCluster *k)
{
	while (k->llnext != 0 && k->isEmpty() && k->llnext->isEmpty()) {
		
			AllocCluster *l = k->llnext;
			k->llnext = l->llnext;
			allClusters.remove(*l);
			l->destroy(bkSize);
	}
}

void AllocMulticluster::reportLeaks( AutoArrayStream<MemoryLeakException::Leak> &list )
{
	AvlTreeBasic<PtrNodeCmp>::Iterator iter = allClusters.getFwIter();
	while (iter.hasItems()) {
		AvlTreeNode<> *nd = iter.getNext();
		AllocCluster *clust = static_cast<AllocCluster *>(nd);
		clust->reportLeaks(list,bkSize);
	}
}

class AllocMulticluster::TreeEraser {
public:
	TreeEraser(natural bksize):bksize(bksize) {}
	void operator()(AvlTreeNode<> *nd) const {
		AllocCluster *l = static_cast<AllocCluster *>(nd);
		l->destroy(bksize);
	}
protected:
	natural bksize;
	
};

AllocMulticluster::~AllocMulticluster()
{
	TreeEraser ers(bkSize);
	allClusters.clear(ers);
	freeClusters = nil;
}


bool AllocMulticluster::PtrNodeCmp::operator()( const AvlTreeNode<> *a, const AvlTreeNode<> *b ) const
{
	return a < b;
}

bool ClusterAlloc::McCmp::operator()( const AvlTreeNode<> *a, const AvlTreeNode<> *b ) const
{
	const AllocMulticlusterNode *ca = static_cast<const AllocMulticlusterNode *>(a);
	const AllocMulticlusterNode *cb = static_cast<const AllocMulticlusterNode *>(b);
	return ca->getBlockSize() < cb->getBlockSize();
}

void * ClusterAlloc::alloc( natural objSize, IRuntimeAlloc * &owner )
{
	natural bksize = AllocCluster::objSize2bksize(objSize);
	AllocMulticlusterNode ndsearch(bksize);
	AvlTreeNode<> *found = tree.find(ndsearch);
	if (found == 0) {
		AllocMulticluster *m = new(rtalloc) AllocMulticluster(*this,rtalloc,bksize);
		found = tree.insert(m).getNext();
	}
	AllocMulticluster *mc = static_cast<AllocMulticluster *>(found);
	return mc->alloc(owner);
}

void * ClusterAlloc::alloc( natural objSize )
{
	IRuntimeAlloc * owner;
	return alloc(objSize,owner);
}

void ClusterAlloc::dealloc( void *ptr, natural objSize )
{
	natural bksize = AllocCluster::objSize2bksize(objSize);
	AllocMulticlusterNode ndsearch(bksize);
	AvlTreeNode<> *found = tree.find(ndsearch);
	if (found == 0) throw InvalidParamException(THISLOCATION,2,
					String("There are no clusters for such allocation"));
	AllocMulticluster *mc = static_cast<AllocMulticluster *>(found);
	return mc->dealloc(ptr);
}

ClusterAlloc::~ClusterAlloc() {
	try {
	 clear();
	} catch (...) {
	/*	if (!std::uncaught_exception())
			throw;*/
	}
}

ClusterAlloc::ClusterAlloc( const ClusterAlloc &alloc )
:rtalloc(alloc.rtalloc)
{

}

ClusterAlloc::ClusterAlloc()
:rtalloc(StdAlloc::getInstance())
{
	
}

ClusterAlloc::ClusterAlloc( IRuntimeAlloc &rtalloc ):rtalloc(rtalloc)
{
	
}
void ClusterAlloc::treeEraser( AvlTreeNode<> *nd )
{
	delete static_cast<AllocMulticluster *>(nd);
}

void ClusterAlloc::clear()
{
	if (!std::uncaught_exception()) {
		AutoArrayStream<MemoryLeakException::Leak> lklist;
		for (AvlTreeBasic<McCmp>::Iterator iter = tree.getFwIter(); iter.hasItems();) {
			AvlTreeNode<> *nd = iter.getNext();
			AllocMulticluster *k = static_cast<AllocMulticluster *>(nd);
			k->reportLeaks(lklist);
		}
		tree.clear(&treeEraser);
		if (!lklist.empty())
			throw MemoryLeakException(THISLOCATION,MemoryLeakException::LeakList(lklist.getArray()));
	} else {
		tree.clear(&treeEraser);

	}
}

/*void AllocCluster::destroy() {
}*/

AllocMulticluster::AllocMulticluster(const AllocMulticluster& other)
:AllocMulticlusterNode(other.bkSize)
,rtalloc(other.rtalloc)
,topMaster(other.topMaster)
,nextCnt(byte(1 + (sizeof(AllocCluster) + bkSize/2)/bkSize)) {}

		}

