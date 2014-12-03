/*
 * deque.h
 *
 *  Created on: 23.7.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_DEQUE_H_
#define LIGHTSPEED_CONTAINERS_DEQUE_H_

#include "arrayt.h"
#include "../memory/staticAlloc.h"
#include "linkedList.h"
#include "../memory/nodeAlloc.h"


namespace LightSpeed {


template<typename T>
struct DequeClustSize {
	static const natural n = (256+sizeof(T)-1)/sizeof(T);
};

///Representing container, where items can be appended on both sided
/**
 * This is core class to implement Stack and Queue. You can push data
 * at front or at back, and you can retrieve and delete data also on
 * both ends. Object simulates ArrayT to access item using index. 
 *
 * @tparam T type stored in container
 * @tparam Alloc Factory to allocate internal objects
 * @tparam citems count of items allocated per cluster. Default value
 *   causes, that objects smaller than 256 bytes will be stored in clusters
 *   to reduce memory fragmentation
 */
 
template<typename T, natural citems = DequeClustSize<T>::n >
class Deque: public ArrayTBase<T,Deque<T,citems> > {
	typedef typename StaticAlloc<citems>::template AllocatedMemory<T> Cluster;
	typedef LinkedList<Cluster> List;
public:
	typedef ArrayTBase<T,Deque<T,citems> > Super;

	Deque():head(0),tail(citems) {}	
	explicit Deque(const NodeAlloc &alloc):head(0),tail(citems),list(alloc) {}
	Deque(const Deque &other);

	template<typename Impl>
	explicit Deque(IIterator<T,Impl> &iter);
	template<typename Impl>
	explicit Deque(const IIterator<T,Impl> &iter);
	template<typename Impl>
	explicit Deque(IIterator<T,Impl> &iter, NodeAlloc alloc);
	template<typename Impl>
	explicit Deque(const IIterator<T,Impl> &iter, NodeAlloc alloc);

	~Deque() {empty();}
	
	Deque &operator=(const Deque &other);

	void pushFront(const T &item);
	void pushBack(const T &item);
	T &pushFront();
	T &pushBack();
	const T &front() const;
	const T &back() const;
	T &front();
	T &back();
	void popFront();
	void popBack();
	bool empty() const;
	natural length() const;
	void clear();

	typedef T ItemT;
	typedef typename Super::OrgItemT OrgItemT;
	typedef typename Super::ConstItemT ConstItemT;
	typedef typename Super::ConstRef ConstRef;
	typedef typename Super::Ref Ref;

	ConstRef at(natural index) const;
	Ref mutableAt(natural index) ;
	Deque &set(natural index, ConstRef val);
	
	template<typename Fn>
	bool forEach(Fn functor) const {
		return forEach(functor,0,length());
	}
	template<typename Fn>
	bool forEach(Fn functor, natural from, natural count) const;


	class Writter: public WriteIteratorBase<T,Writter> {
	public:
		
		Writter(Deque &owner, bool dir)
			:owner(owner),dir(dir) {}
		bool hasItems() const {return true;}
		void write(const T &x) {
			if (dir) owner.pushBack(x);else owner.pushFront(x);
		}
	protected:
		Deque &owner;
		bool dir;
	};

	class Reader: public IteratorBase<T, Reader> {
	public:

		Reader(Deque &owner, bool dir)
			:owner(owner),dir(dir),discardLast(false) {
			
		}	
		bool hasItems() const {
			return (!discardLast && !owner.empty())
					|| owner.length() > 1;
		}
		const T &getNext() {
			if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION, typeid(T));
			if (dir) {
				if (discardLast) owner.popBack();
				return owner.back();
			} else {
				if (discardLast) owner.popFront();
				return owner.front();
			}
		}

		const T &peek() const {
			if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION, typeid(T));
			if (dir) {
				if (discardLast) {
					owner.popBack();
					discardLast = false;
				}
				return owner.back();
			} else {
				if (discardLast) {
					owner.popFront();
					discardLast = false;
				}
				return owner.front();
			}
		}
		
		natural getRemain() const {
			return owner.length() - (discardLast?1:0);
		}

		~Reader() {
			if (dir) {
				if (discardLast) {
					owner.popBack();
				}
			} else {
				if (discardLast) {
					owner.popFront();
				}
			}			
		}

	protected:
		Deque &owner;
		bool dir;
		mutable bool discardLast;
	};

	Writter writeBack() {
		return Writter(*this,true);
	}
	Writter writeFront() {
		return Writter(*this,false);
	}

	Reader readBack() {
		return Reader(*this,true);
	}
	Reader readFront() {
		return Reader(*this,false);
	}

	NodeAlloc getAllocator() const {
		return list.getAllocator();
	}

	class Loader {
	public:
		Loader(Deque &owner):wr(owner.writeBack()) {}
		bool operator()(const T &x) const {
			wr.write(x);
			return false;
		}
	protected:
		mutable Writter wr;
	};

	class Iterator: public IteratorBase<T, Iterator> {
	public:
		Iterator(const typename List::Iterator &iter, natural pos, natural count)
			:iter(iter),pos(pos),count(count) {}
		bool hasItems() const {return count > 0;}
		const T &getNext() {
			const Cluster &k = iter.peek();
			const T &out = k.getBase()[pos];
			pos++;count--;
			if (pos >= citems) {
				pos-=citems;
				iter.skip();
			}
			return out;
		}
		const T &peek() const {
			const Cluster &k = iter.peek();
			return k.getBase()[pos];
		}

		natural getRemain() const {
			return count;
		}
		
	protected:
		typename List::Iterator iter;
		natural pos;
		natural count;
	};

	Iterator getFwIter() const {
		return Iterator(list.getFwIter(),head,length());
	}

	template<typename Serializer>
	void serialize(Serializer &arch) {
		typename Serializer::Array arr(arch,length());
		if (arch.storing()) {
			Iterator iter = getFwIter();
			while (iter.hasItems() && arr.next())  {
				arch << iter.getNext();
			}
		} else {
			while (arr.next()) {
				arch(pushFront());
			}
		}
	}

protected:

	natural head, tail;
	List list;
	
	void addClusterFront();
	void addClusterBack();
	Cluster &seek(natural &idx);
	const Cluster &seek(natural &idx) const ;
	typename List::Iterator seekIter(natural &idx) const;

};


} // namespace LightSpeed





#endif /* DEQUE_H_ */
