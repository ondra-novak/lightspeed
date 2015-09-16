/*
 * dispatcher.cpp
 *
 *  Created on: 30. 7. 2015
 *      Author: ondra
 */




#include "abstractDispatcher.tcc"

namespace LightSpeed {

template Promise<IDispatcher::DispatchHelper<Promise<int> >::RetV> IDispatcher::dispatch(const Promise<int> &);

AbstractDispatcher::AbstractDispatcher()
:nextCheck(1)
{

}

void AbstractDispatcher::promiseRegistered(PPromiseControl ppromise) {
	bool willPrune;
	{
	Synchronized<FastLock> _(lock);
	dprom.add(ppromise);
	willPrune = dprom.length() >= nextCheck;
	}
	if (willPrune) {
		pruneRegPromises();
	}

}

void AbstractDispatcher::pruneRegPromises() {
	Synchronized<FastLock> _(lock);
	DispatchedPromises dprom2;
	if (dprom.length() >= nextCheck) {
		for (natural i = 0; i < dprom.length(); i++) {
			if (!dprom[i]->resolved()) {
				dprom2.add(dprom[i]);
			}
		}
	}
	dprom2.swap(dprom);
	nextCheck = dprom.length() * 2;

}

void AbstractDispatcher::cancelAllPromises() {
	Synchronized<FastLock> _(lock);
	for (natural i = 0; i < dprom.length(); i++) {
		dprom[i]->cancel();
	}
	dprom.clear();
}

}
