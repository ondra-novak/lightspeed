/*
 * move.h
 *
 *  Created on: 23.7.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_MOVE_H_
#define LIGHTSPEED_CONTAINERS_MOVE_H_

#include <utility>

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
	template<typename T>
	class MoveObject {
	public:

		static T *doMove(T *oldAddr, void *newAddr) {
	        T *res = new(newAddr) T(*oldAddr);
	        oldAddr->~T();
	        return res;
		}
	};

	///Moving using binary copying
	/** Binary copying can be a lot of faster, but no all objects can be moved by this method.
	 * All POD data can be moved binary. Simple layout object could be also moved binary.
	 */
	class MoveObject_Binary {
	public:
		template<typename T>
		static T *doMove(T *oldAddr, void *newAddr) {
			natural cnt = sizeof(T)/sizeof(natural);
			for (natural i = 0; i < cnt; i++) {
				reinterpret_cast<natural *>(newAddr)[i] =
						reinterpret_cast<natural *>(oldAddr)[i];
			}
			natural cnt2 = sizeof(T)%sizeof(natural);
			for (natural i = 0; i < cnt2; i++) {
				reinterpret_cast<byte *>(newAddr)[cnt*sizeof(natural)+i] =
						reinterpret_cast<byte *>(oldAddr)[cnt*sizeof(natural)+i];
			}
			return reinterpret_cast<T *>(oldAddr);
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
