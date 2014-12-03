/*
 * syncobj.h
 *
 *  Created on: 9.11.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_SYNC_SYNCOBJ_H_
#define LIGHTSPEED_SYNC_SYNCOBJ_H_

#include "syncobj.h"
#include "synchronize.h"

namespace LightSpeed {

	template<typename Lock>
	class SyncObj: private Lock {
	public:

		SyncObj() {}
		SyncObj(const Lock &lk):Lock(lk) {}


		class Sync: public Synchronized<Lock> {
		public:
			Sync(const SyncObj &obj):Synchronized<Lock>(obj.getLock()) {}
			Sync(const SyncObj *obj):Synchronized<Lock>(obj->getLock()) {}
		};

		class RevSync: public SyncReleased<Lock> {
		public:
			RevSync(const SyncObj &obj):SyncReleased<Lock>(obj.getLock()) {}
			RevSync(const SyncObj *obj):SyncReleased<Lock>(obj->getLock()) {}
		};


		Lock &getLock() const {return const_cast<SyncObj &>(*this);}
	};

}  // namespace LightSpeed

#endif /* LIGHTSPEED_SYNC_SYNCOBJ_H_ */
