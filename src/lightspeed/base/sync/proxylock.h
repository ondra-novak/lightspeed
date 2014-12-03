/*
 * proxylock.h
 *
 *  Created on: 20.8.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_MT_PROXYLOCK_H_
#define LIGHTSPEED_BASE_MT_PROXYLOCK_H_

namespace LightSpeed {

    ///Proxy lock that delegates locking to the another lock
    /**
    Proxy lock also allows to change delegate, or unset delegate and complette disable
    its function
    */
    template<class LockT>
    class ProxyLock {
        LockT *lk;
    public:
        ProxyLock(LockT *lk):lk(lk) {}
        ProxyLock():lk(0) {}

        void lock() const {
            if (lk) lk->lock();
        }
        void unlock() const {
            if (lk) lk->unlock();
        }
        bool tryLock() const {
            if (lk) return lk->tryLock();
            else return false;
        }
        void set(LockT *lk) {this->lk = lk;}
        void unset() {lk = 0;}
        LockT get() const {return lk;}
    };



} // namespace LightSpeed

#endif /* PROXYLOCK_H_ */
