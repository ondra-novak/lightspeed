/*
 * deque.tcc
 *
 *  Created on: 17.9.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_DEQUE_TCC_
#define LIGHTSPEED_CONTAINERS_DEQUE_TCC_

#pragma once

#include "deque.h"
#include "move.h"
#include "../exceptions/container.h"


namespace LightSpeed {

	template<typename T, typename Alloc /*= StdAlloc*/>
	Deque<T, Alloc>::Deque()
		:memBlock(Alloc(),0), head(0), tail(0)
	{

	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	Deque<T, Alloc>::Deque(const Alloc &alloc)
		:memBlock(alloc,0), head(0), tail(0)
	{

	}


	template<typename T, typename Alloc /*= StdAlloc*/>
	typename Deque<T, Alloc>::Ref Deque<T, Alloc>::set(natural index, ConstRef val)
	{
		if (index >= length()) throwRangeException_To<integer>(THISLOCATION, length() - 1, index);
		return mutableAt(index) = val;
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	typename Deque<T, Alloc>::Ref Deque<T, Alloc>::mutableAt(natural index)
	{
		if (index >= length()) throwRangeException_To<integer>(THISLOCATION, length() - 1, index);
		index += tail;
		return memBlock.getBase()[index % memBlock.getSize()];
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	typename Deque<T, Alloc>::ConstRef Deque<T, Alloc>::at(natural index) const
	{		
		if (index >= length()) throwRangeException_To<integer>(THISLOCATION, length() - 1, index);
		index += tail;
		return memBlock.getBase()[index % memBlock.getSize()];
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	natural Deque<T, Alloc>::length() const
	{
		
		if (head && head <= tail) {
			return head + memBlock.getSize() -tail;
		}
		else return head - tail;
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	template<typename I>
	void Deque < T, Alloc > ::pushFront(const Constructor<T, I> &val)
	{
		if (isFull())  tryExpand(); 
		natural memSz = memBlock.getSize();
		natural at = head % memSz;
		val.construct(memBlock.getBase() + at);
		head = at + 1;
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	template<typename I>
	void Deque<T, Alloc>::pushBack(const Constructor<T, I> &val)
	{
		if (isFull()) tryExpand();
		natural memSz = memBlock.getSize();
		if (head == 0) head = memSz;
		natural at = (tail + memSz - 1) % memSz;
		val.construct(memBlock.getBase() + at);
		tail = at;

	}


	template<typename T, typename Alloc /*= StdAlloc*/>
	bool Deque<T, Alloc>::isFull() const
	{
		return length() == memBlock.getSize();
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	bool Deque<T, Alloc>::empty() const
	{
		return head == 0;
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	typename Deque<T, Alloc>::ConstRef Deque<T, Alloc>::getBack() const
	{
		if (empty()) throw ContainerIsEmptyException(THISLOCATION);
		return memBlock.getBase()[tail];
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	typename Deque<T, Alloc>::ConstRef Deque<T, Alloc>::getFront() const
	{
		if (empty()) throw ContainerIsEmptyException(THISLOCATION);
		return memBlock.getBase()[head-1];

	}
	template<typename T, typename Alloc /*= StdAlloc*/>
	typename Deque<T, Alloc>::Ref Deque<T, Alloc>::getBack() 
	{
		if (empty()) throw ContainerIsEmptyException(THISLOCATION);
		return memBlock.getBase()[tail];
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	typename Deque<T, Alloc>::Ref Deque<T, Alloc>::getFront()
	{
		if (empty()) throw ContainerIsEmptyException(THISLOCATION);
		return memBlock.getBase()[head - 1];
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	void Deque<T, Alloc>::popFront()
	{
	
		if (empty()) throw ContainerIsEmptyException(THISLOCATION);
		natural memSz = memBlock.getSize();
		head--;
		T &itm = memBlock.getBase()[head];		
		if (head == tail) head = tail = 0; //< set empty (both zero)
		else head = (head + memSz - 1) % memSz + 1;
		itm.~T();
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	void Deque<T, Alloc>::popBack()
	{
		if (empty()) throw ContainerIsEmptyException(THISLOCATION);
		natural memSz = memBlock.getSize();
		T &itm = memBlock.getBase()[tail];
		tail ++;
		if (head == tail) head = tail = 0; //<set empty (both zero)
		else tail %= memSz;
		itm.~T();
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	void Deque<T, Alloc>::pushFront(ConstRef val)
	{
		pushFront(Constructor1<T,T>(val));
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	void Deque<T, Alloc>::pushBack(ConstRef val)
	{
		pushBack(Constructor1<T,T>(val));
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	bool Deque<T, Alloc>::internalResize(natural reqSz, natural curSz)
	{
		//class allows to rollback item moving when exception is thrown
		/* If there is exception during moving, the failed items is considered as not moved.
		  So we need to move all items back before exception can be throw out
		  The destructor if this class can handle it. Because rollback is performed during
		  stack unwinding, additional exceptions are considered as double throw with appropriate
		  behavior. Move operator should throw exception only if there is none in flight.
		  */
		class MoveOp {
		public:			
			MoveOp(T *oldPos, T *newPos, natural count, natural wrap, natural offset)
				:oldPos(oldPos), newPos(newPos), count(count), wrap(wrap), offset(offset), i(0) {}
			void doMove() {
				while (i < count) {
					natural srcIndex = (offset + i) % wrap;
					MoveObject<T>::doMove(oldPos + srcIndex, newPos + i);
					i++;
				}
			}
			~MoveOp() {
				if (i < count) for (natural j = 0; j < i; i++) {
					natural srcIndex = (offset + j) % wrap;
					MoveObject<T>::doMove(newPos + j, oldPos + srcIndex);
				}
			}

		protected:
			T *oldPos;
			T *newPos;
			natural count;
			natural wrap;
			natural offset;
			natural i;
		};

		//allocate temporary block
		MemBlock tmp(memBlock, reqSz);
		//prepare empty block
		MemBlock newblk(memBlock, 0);
		//test, whether we are able to move block from temp to new
		if (tmp.move(newblk)) {

			MoveOp(memBlock.getBase(), newblk.getBase(), curSz, memBlock.getSize(), tail).doMove();
			//finally, move new block to old instance - oldblock will be deleted
			newblk.move(memBlock);
			head = curSz;
			tail = 0;
			return true;
		}
		else {
			//when we are unable to move newly created block
			return false;
		}
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	bool LightSpeed::Deque<T, Alloc>::reserve(natural count)
	{
		natural sz = length();
		if (count > sz) {
			return internalResize(count, sz);
		}
		else {
			return false;
		}
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	void Deque<T, Alloc>::tryExpand()
	{
		natural curLen = length();		
		natural newSz = curLen ? curLen + (curLen + 1) / 2 : 1;
//		natural curBlock = memBlock.getSize();
		if (!reserve(newSz)) {
			throwAllocatorLimitException(THISLOCATION, newSz, memBlock.getSize(), typeid(MemBlock));
		}
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	void Deque<T, Alloc>::clear()
	{
		while (!empty()) {
			popFront();
		}
	}

	template<typename T, typename Alloc /*= StdAlloc*/>
	Deque<T, Alloc>::~Deque()
	{
		class Deleter {
		public:
			Deleter(T *pole, natural offset, natural len, natural wrap) :pole(pole), offset(offset), len(len), wrap(wrap), i(0) {}
			void run() {
				while (i < len) {
					pole[(offset + i++) % wrap].~T();
				}
			}
			~Deleter() {
				run();
			}

		protected:
			T *pole;
			natural offset;
			natural len;
			natural wrap;
			natural i;
		};

		Deleter(memBlock.getBase(), tail, length(), memBlock.getSize()).run();
	}


	template<typename T, typename Alloc>
	void Deque<T,Alloc>::swap(Deque<T,Alloc> &other) {
		 memBlock.swap(other.memBlock);
		 natural h = head; head = other.head; other.head = h;
		 natural t = tail; tail = other.tail; other.tail = t;
	}

}
#endif /* LIGHTSPEED_CONTAINERS_DEQUE_TCC_ */



