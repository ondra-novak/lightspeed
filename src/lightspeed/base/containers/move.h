/*
 * move.h
 *
 *  Created on: 23.7.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_MOVE_H_
#define LIGHTSPEED_CONTAINERS_MOVE_H_

#include <utility>
#include <string.h>



namespace LightSpeed {

enum ShareConstruct {
	_shareConstruct
};

enum MoveConstruct {
	_moveConstruct
};

	///Default moving traits
	/** It performs moving using standard constructor. It first creates
	 * copy on new address and the destroys object on old address
	 */

	class MoveObject_Copy {
	public:
		template<typename T>
		static T *doMove(T *oldAddr, void *newAddr) {
			T *res = new(newAddr) T(*oldAddr);
			oldAddr->~T();
			return res;
		}
	};


	template<typename T>
	class MoveObject: public MoveObject_Copy {
	public:

#ifdef LIGHTSPEED_ENABLE_CPP11
		static T *doMove(T *oldAddr, void *newAddr) {
	        T *res = new(newAddr) T(std::move(*oldAddr));
	        oldAddr->~T();
	        return res;
		}
#endif
	};



	///Moving using binary copying
	/** Binary copying can be a lot of faster, but no all objects can be moved by this method.
	 * All POD data can be moved binary. Simple layout object could be also moved binary.
	 */
	class MoveObject_Binary {
	public:
		template<typename T>
		static T *doMove(T *oldAddr, void *newAddr) {
			memmove(newAddr, oldAddr, sizeof(T));
			return reinterpret_cast<T *>(newAddr);
		}
	};

	///Moves using method declared on object
	/**
	 * Method is named move. It has one parameter .. address of new location.
	 * Result is pointer on the new location. Note that move method
	 * should leave object destroyed
	 */
	class MoveObject_Method {
	public:
		template<typename T>
		static T *doMove(T *oldAddr, void *newAddr) {
			return oldAddr->move(newAddr);
		}
	};

	///Moves using std::swap
	/**
	 * It creates object on new location and then uses std::swap to change
	 * it with object on old address. After swapping, it destroyes the temporary
	 * object
	 */
	class MoveObject_Swap {
	public:
		template<typename T>
		static T *doMove(T *oldAddr, void *newAddr) {
			T *x = new(newAddr) T();
			std::swap(*oldAddr,*x);
			oldAddr->~T();
			return x;
		}
	};


	class MoveObject_Construct {
	public:
		template<typename T>
		static T *doMove(T *oldAddr, void *newAddr) {
	        T *res = new(newAddr) T(_moveConstruct, *oldAddr);
	        oldAddr->~T();
	        return res;
		}

	};

	template<typename T>
	inline T *moveObject(T *from, void *to) {
		return MoveObject<T>::doMove(from,to);
	}


}




#endif /* MOVE_H_ */
