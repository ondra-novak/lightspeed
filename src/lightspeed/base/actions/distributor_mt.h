/*
 * distributor_mt.h
 *
 *  Created on: 12.9.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTIONS_DISTRIBUTOR_MT_H_
#define LIGHTSPEED_BASE_ACTIONS_DISTRIBUTOR_MT_H_
#include "../../mt/threadId.h"
#include "../../mt/fastlock.h"


namespace LightSpeed {


	class DistributorThreadStage {
	public:
		DistributorThreadStage():id(ThreadId::current()) {}
		DistributorThreadStage(ThreadId id):id(id) {}

		bool operator==(const DistributorThreadStage &stage) const {return id == stage.id;}
		bool operator!=(const DistributorThreadStage &stage) const {return id != stage.id;}


	protected:
		ThreadId id;
	};


	template<typename Ifc, typename AllocFactory = ClusterFactory<> >
	class MtDistributor: public Distributor<Ifc, DistributorThreadStage, FastLock, AllocFactory> {

	};

}

#endif /* LIGHTSPEED_BASE_ACTIONS_DISTRIBUTOR_MT_H_ */
