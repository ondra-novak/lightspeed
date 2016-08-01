/*
 * deque.h
 *
 *  Created on: 23.7.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_DEQUE_H_
#define LIGHTSPEED_CONTAINERS_DEQUE_H_


#include "arrayt.h"
#include "../constructor.h"


namespace LightSpeed {

class StdAlloc;

template<typename T, typename Alloc = StdAlloc>
class Deque: public ArrayTBase<T, Deque<T, Alloc> > {
public:

	typedef ArrayTBase<T, Deque<T, Alloc> > Super;
	typedef typename Super::ConstRef ConstRef;
	typedef typename Super::Ref Ref;
	typedef T Impl;

	Deque();
	Deque(const Alloc &alloc);
	~Deque();

	bool empty() const;
	bool isFull() const;
	natural length() const;

	ConstRef at(natural index) const;
	Ref mutableAt(natural index);
	Ref set(natural index, ConstRef val);

	void pushBack(ConstRef val);
	void pushFront(ConstRef val);
	void popBack();
	void popFront();
	void clear();
	ConstRef getFront() const;
	ConstRef getBack() const;
	Ref getFront();
	Ref getBack();

	template<typename I>
	void pushBack(const Constructor<T, I> &val);
	template<typename I>
	void pushFront(const Constructor<T, I> &val);

	///Reserve memory to allow deque store more items
	/** Memory is resized as needed however, you can request to preallocate space to reduce numbers
	 of resizing 
	 
	 @param count of items to preallocate
	 */
	bool reserve(natural count);
	void swap(Deque &other);

protected:

	bool internalResize(natural reqSz, natural curSz);
	void tryExpand();

	typedef typename Alloc::template AllocatedMemory<T> MemBlock;

	MemBlock memBlock;
	/*contains index of next item. 
	  it runs in range <1, bufferSize>
	  If head == 0, then container is empty, because this will not happen, when container contains items
	*/
	natural head;
	/*contains index of last item.
      it runs in range <0, bufferSize-1>
	  if head == tail, then container is full and must be expanded, unless head == 0 - then it is empty,
	*/
	natural tail;	

};


} // namespace LightSpeed





#endif /* DEQUE_H_ */
