/*
 * multilock.h
 *
 *  Created on: Jun 6, 2014
 *      Author: ondra
 */

#ifndef LIGHTSPEED_SYNC_MULTILOCK_H_
#define LIGHTSPEED_SYNC_MULTILOCK_H_


namespace LightSpeed {

class MultiLock {
protected:
	template<typename A, typename B>
	static bool compareAddr(const A &a, const B &b) {
		return reinterpret_cast<const byte *>(&a) < reinterpret_cast<const byter *>(&b);
	}
};

template<typename Lk1, typename Lk2>
class MultiLock2: public MultiLock
{
public:
	MultiLock2(Lk1 &lk1, Lk2 lk2);
    void lock();
    void unlock();
    bool tryLock();

protected:
    Lk1 &lk1;
    Lk2 &lk2;

};

template<typename Lk1, typename Lk2, typename Lk3>
class MultiLock3: public MultiLock2<Lk1,Lk2> {
public:
	MultiLock3(Lk1 &lk1, Lk2 lk2, Lk3 lk3):MultiLock2(lk1,lk2),lk3(lk3) {}
    void lock();
    void unlock();
    bool tryLock();

protected:
	Lk3 &lk3;
};

template<typename Lk1, typename Lk2>
inline MultiLock2<Lk1, Lk2>::MultiLock2(Lk1& lk1, Lk2 lk2):lk1(lk1),lk2(lk2) {
}

template<typename Lk1, typename Lk2>
inline void MultiLock2<Lk1, Lk2>::lock() {
	if (compareAddr(lk1,lk2)) {
		lk1.lock();
		lk2.lock();
	} else {
		lk2.lock();
		lk1.lock();
	}
}

template<typename Lk1, typename Lk2>
inline void MultiLock2<Lk1, Lk2>::unlock() {
	lk1.unlock();
	lk2.unlock();
}

template<typename Lk1, typename Lk2>
inline bool MultiLock2<Lk1, Lk2>::tryLock() {
	if (reinterpret_cast<byte *>(&lk1) < reinterpret_cast<byte *>(&lk2)) {
		if (!lk1.tryLock()) return false;
		if (!lk2.tryLock()) { lk1.unlock();return false;}
	} else {
		if (!lk2.tryLock()) return false;
		if (!lk1.tryLock()) {lk2.unlock();return false;}
	}
}

template<typename Lk1, typename Lk2, typename Lk3>
inline void MultiLock3<Lk1, Lk2, Lk3>::lock() {
	if (compareAddr(this->lk1,this->lk2)) {
		if (compareAddr(this->lk1,lk3)) {
			lk1.lock();
			if (compareAddr(this->lk2,lk3)) {
				this->lk2.lock();
				lk3.lock();
			} else {
				lk3.lock();
				this->lk2.lock();
			}
		} else {
			lk3.lock();
			if (compareAddr(this->lk1,this->lk2)) {
				this->lk1.lock();
				this->lk2.lock();
			} else {
				this->lk2.lock();
				this->lk1.lock();
			}
		}
	} else {
		if (compareAddr(this->lk2,lk3)){
			this->lk2.lock();
			if (compareAddr(this->lk1,lk3)) {
				this->lk1.lock();
				lk3.lock();
			} else {
				lk3.lock();
				this->lk1.lock();
			}
		} else {
			lk3.lock();
			this->lk2.lock();
			this->lk1.lock();
		}


	}



}

template<typename Lk1, typename Lk2, typename Lk3>
inline void MultiLock3<Lk1, Lk2, Lk3>::unlock() {

}

template<typename Lk1, typename Lk2, typename Lk3>
inline bool MultiLock3<Lk1, Lk2, Lk3>::tryLock() {

}

}


#endif /* LIGHTSPEED_SYNC_MULTILOCK_H_ */
