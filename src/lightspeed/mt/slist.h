#pragma once
#include "../base/exceptions/invalidParamException.h"
#include "../base/align.h"
#include "atomic.h"

namespace LightSpeed {

	///Implementation of lock-free single linked list
	/**
	 * All operations are MT safe

	 *
	 * @tparam T specifies item of the list. It has to have field 'next' public accessible or friend
	 * @tparam align specifies alignment of every item. It must be value powered by 2. For example: 4,8,16,32 etc
	 *   Choose the appropriate value depend of align of the allocator. Default value is 8 for 32-bit 
	 *   platform and 16 for 64-bit platform. SList cannot accept misaligned items. Higher values reduces
	 *   SList vulnerability to ABA problems while multiple threads removing items from the list. Bits of
	 *   the address which are not used due align are reserved to cookie. Cookie is used to detect
	 *   access of other threads at same time. For align 8, three bits will be used and SList is 
	 *   able to detect 8 changes. Every 8 change may be vulnerable. If you have less then 8 threads
	 *   accessing this object, this cannot cause problem. Align 16 reserves 4 bits and improves modification
	 *   detection to 16 changes at the same time. If you see problem in modification detection, allocate
	 *   items using higher align.
	 *   
     */

	 
	template<typename T, natural align = preferedDataAlign * 2>
	class SList {
	public:

		static const natural mask = (align - 1);
		static const natural invalid_alignment = 1 / ((align & mask) == 0?1:0);

		///constructs and initialize empty list
		SList():_top(0) {}

		SList(T *existingList):_top(ptrToCookie(existingList,0)) {}

		///insert item into the list
		/** @param itm pointer to item. Ensure, that item is not inserted into this this or another list.
		   Item is inserted at the top of the list

		   Function is lock-free multi-thread safe. If there are a lot of thread accessing the list,
		   implementation can be vulnerable to starvation. In this case, lock can do better job 
		   instead of lock-free lists
		 */
		void push(T *itm) {			
			bool rep;
			atomic ntop = _top;
			do {
				atomic ctop = ntop;
				atomic citm = ptrToCookie(itm,ctop);
				itm->next = cookieToPtr(ctop);
				ntop = lockCompareExchange(_top,ctop,citm);
				rep = ntop != ctop;
			} while (rep);
		}

		///pops item from the list (removes from the top)
		/** @return pointer to item removed from the list.

		   Function is lock-free multi-thread safe. If there are a lot of thread accessing the list,
		   implementation can be vulnerable to starvation. In this case, lock can do better job 
		   instead of lock-free lists
		 */
		T *pop() {
			bool rep;
			T *ptop;
			atomic ntop = _top;
			do {
				atomic ctop = ntop;
				ptop = cookieToPtr(ctop);
				if (ptop == 0) return 0;
				T *pnext = safeGetPointerValue(ptop->next);
				atomic chtop = ptrToCookie(pnext,ctop+1);
				ntop = lockCompareExchange(_top,ctop,chtop);
				rep = ntop != ctop;
			} while (rep);
			return ptop;
		}

		///Tests whether list is empty
		/**
		 * @retval true list is empty
		 * @retval false list is not empty
		 *
		 * @note value returned by this function can be obsolete. If you want to pop item till end,
		 * test return value of the pop().
		 */
		 
		
		bool empty() const {
			return ptrToCookie(_top) == 0;
		}

		///Insert (push) item into list in single thread access
		/** Use this function, if you sure, that no other thread will access this list.
		   Good during initialization or destruction, when list is not public for other threads.
		   Function is slightly faster, because don't need to handle concurrent access 

		   @param itm pointer to item. Ensure, that item is not inserted into this this or another list.
		   Item is inserted at the top of the list

		   */
		void push_st(T *itm) {
			itm->next = cookieToPtr(_top);
			_top = ptrToCookie(itm,_top);
		}


		///pops item from the list (removes from the top) in single thread access
		/** @return pointer to item removed from the list.

			Good during initialization or destruction, when list is not public for other threads.
			Function is slightly faster, because don't need to handle concurrent access 
		 */
		T* pop_st() {
			T *p = cookieToPtr(_top);
			if (p == 0) return 0;
			T *n = p->next;
			_top = ptrToCookie(n,_top+1);
			return p;
		}

		///Reverses list 
		 
		void reverse(SList &target) {
			T *k = pop();
			while (k) {
				target.push(k);
				k = pop();
			}
		}

		///Reverses list in single thread access
		void reverse_st(SList &target) {
			T *k = pop_st();
			while (k) {
				target.push_st(k);
				k = pop_st();
			}
		}

		///Takes whole list atomically
		/**
		 * @param source subject which list will be taken. All items are moved to 
		   this object. Other threads may finish inserting or removing items before 
		   list is taken, or will insert items into new empty list. (or starts to see
		   empty list and will not remove anything)
		 */
		 
		void take(SList &source) {
			_top = lockExchange(source._top,_top);
		}
		

		///Swaps current object with source
		/** Current object is changed atomically. source should not be accessed by multiple threads at time */
		void swap(SList &source) {
			_top = lockExchange(source._top,_top);
		}

		///Swaps content of slists, when top of current slist contain specified value
		/**
		 * @param source source list
		 * @param condition condition tested on target list. Note that pointer still require
		 * be aligned depend of align argument specified on template
		 * @retval true success - lists has been swapped
		 * @retval false failure - list has not been swapped
		 *
		 * @note function returns false when condition is equal to current top of source,
		 */
		bool swapWhenTopIs(SList &source, T *condition) {
			atomic x = ptrToCookie(condition,_top);
			atomic q = lockCompareExchange(_top,x,source._top);
			source._top = q;
			return q == x;
		}

		T *top() const {
			return cookieToPtr(_top);
		}


	protected:

		atomic _top;


		static T *cookieToPtr(atomic cookie) {
			return (T *)(cookie & ~mask);
		}

		static atomicValue ptrToCookie(T *ptr, atomic cookPart) {
			atomic cookieBase = (atomic)ptr;
			if ((cookieBase & mask) != 0) throw InvalidParamException(THISLOCATION,1,"Item must be aligned");
			return ((atomic)ptr & ~mask) | (cookPart  & mask);
		}
	private: 
		SList(const SList &other);
		SList operator=(const SList &other);


	};

	
}
