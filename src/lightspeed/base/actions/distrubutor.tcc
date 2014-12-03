/*
 * distrubutor.tcc
 *
 *  Created on: 8.11.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ACTIONS_DISTRUBUTOR_TCC_
#define LIGHTSPEED_ACTIONS_DISTRUBUTOR_TCC_

#include "distributor.h"
#include "../exceptions/pointerException.h"

namespace LightSpeed {

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::add(Ifc ptr) {
	Sync _(this);
	listeners.add(ptr);
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::priorityAdd(Ifc ptr) {
	Sync _(this);
	listeners.insertFirst(ptr);
}

template<typename Ifc, typename StageSel, typename Lock>
natural Distributor<Ifc,StageSel,Lock>::isAdded(Ifc ptr) const {
	Sync _(this);
	natural count = 0;
	typename Listeners::Iterator iter = listeners.getFwIter();
	while (iter.hasItems()) {
		if (iter.getNext() == ptr) count++;
	}
	return count;
}

template<typename Ifc, typename StageSel, typename Lock>
bool Distributor<Ifc,StageSel,Lock>::remove(Ifc ptr) {
	Sync _(this);
	typename Listeners::Iterator iter = listeners.getFwIter();
	while (iter.hasItems()) {
		if (iter.peek() == ptr) {

			typename Stages::Iterator iter2 = stages.getFwIter();
			while(iter2.hasItems()) {
				Stage &stg = stages.getItem(iter2);
				if (stg.pos == iter) stg.pos.skip();
				iter2.skip();
			}
			if (exitMsgStage != nil) {
				Stage &stg = *exitMsgStage;
				if (stg.pos == iter) stg.pos.skip();
			}

			listeners.erase(iter);
			return true;
		}
		iter.skip();
	}
	return false;

}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::removeAll() {
	Sync _(this);
	listeners.clear();
	//if all listeners removed, clear also all stages
	stages.clear();
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::send(Ifc sender, const Msg &msg, const StageSel &sel) {
	Sync _(this);
	StageItem itm(Stage (msg,sel,listeners,&sender));
	stages.add(&itm);
	_finish(sel);
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::send(const Msg &msg, const StageSel &sel) {
	Sync _(this);
	StageItem itm(Stage(msg,sel,listeners,0));
	stages.add(&itm);
	_finish(sel);
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::send(Ifc sender, const Msg &msg) {
	send(sender,msg,StageSel());
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::send(const Msg &msg) {
	send(msg,StageSel());
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::finish(const StageSel &sel) {
	Sync _(this);
	return _finish(sel);
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::finish() {
	Sync _(this);
	return _finish(StageSel());
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::_finish(const StageSel &sel) {
	//search for stage - get iterator
	typename Stages::Iterator st = findStage(sel);
	//if stage found
	while (st.hasItems()) {
		Stage &stage = stages.getItem(st);
		Envelop env(*this,sel,stage.sender);
		if (!stage.pos.hasItems()) {
			stages.erase(st);
		} else {
			Ifc ifc = stage.pos.getNext();
			const Msg &msg = stage.msg;
			try {
				RevSync _(this);
				msg(ifc,env);
			} catch (...) {
				_stop(sel);
				throw;
			}

		}
		st = findStage(sel);
	}
}

template<typename Ifc, typename StageSel, typename Lock>
typename Distributor<Ifc,StageSel,Lock>::Stages::Iterator Distributor<Ifc,StageSel,Lock>::findStage(
						const StageSel &sel) const {

	typename Stages::Iterator iter = stages.getFwIter();
	while (iter.hasItems() && iter.peek().num != sel) {
		iter.skip();
	}
	return iter;

}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::sendExitMsg() {

	class SendCycle: public Stage {
	public:
		SendCycle(Distributor<Ifc,StageSel,Lock> &dist, const Msg &msg, const StageSel &sel, const Listeners &listeners)
			:Stage(msg,sel,listeners,0),dist(dist) {}
		void finishCycle() {
			while (this->pos.hasItems()) {
				Envelop env(dist,this->num,0);
				Ifc ifc = this->pos.getNext();
				const Msg &msg = this->msg;
				RevSync _(dist);
				msg(ifc,env);
			}
		}
		~SendCycle() {
			finishCycle();
		}
	protected:
		Distributor<Ifc,StageSel,Lock> &dist;
	};

	if (exitMsg) {
		Sync _(this);
		StageSel sel;
		SendCycle snd(*this,*exitMsg,sel,listeners);
		snd.finishCycle();
	}
}

template<typename Ifc, typename StageSel, typename Lock>
Ifc Distributor<Ifc,StageSel,Lock>::getCurrent(const StageSel &stage) const {
	try {
		Sync _(this);

		typename Stages::Iterator iter = findStage(stage);
		const Stage &st = iter.peek();
		return st.pos.getPrev();
	} catch (Exception &e) {
		throw NoListenerException(THISLOCATION) << e;
	}
}

template<typename Ifc, typename StageSel, typename Lock>
Ifc Distributor<Ifc,StageSel,Lock>::getCurrent() const {
	return getCurrent(StageSel());
}


template<typename Ifc, typename StageSel, typename Lock>
Ifc Distributor<Ifc,StageSel,Lock>::getSender(const StageSel &stage) const {
	try {
		Sync _(this);

		typename Stages::Iterator iter = findStage(stage);
		const Stage &st = iter.peek();
		if (st.sender) return *(st.sender);
		throw NullPointerException(THISLOCATION);
	} catch (Exception &e) {
		throw NoListenerException(THISLOCATION) << e;
	}

}

template<typename Ifc, typename StageSel, typename Lock>
Ifc Distributor<Ifc,StageSel,Lock>::getSender() const {
	return getSender(StageSel());
}

template<typename Ifc, typename StageSel, typename Lock>
bool Distributor<Ifc,StageSel,Lock>::empty() const {
	Sync _(this);
	return listeners.empty();
}

template<typename Ifc, typename StageSel, typename Lock>
natural Distributor<Ifc,StageSel,Lock>::size() const {
	Sync _(this);
	return listeners.size();
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::stop(const StageSel &stage) {
	Sync _(this);
	_stop(stage);
}
template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::_stop(const StageSel &stage) {
	typename Stages::Iterator iter = stages.getFwIter();
	while (iter.hasItems()) {
		const Stage &st = iter.peek();
		if (st.num == stage) {
			iter = stages.erase(iter);
		} else {
			iter.skip();
		}
	}
}

template<typename Ifc, typename StageSel, typename Lock>
void Distributor<Ifc,StageSel,Lock>::stop() {
	stop(StageSel());
}


}  // namespace LightSpeed

#endif /* LIGHTSPEED_ACTIONS_DISTRUBUTOR_TCC_ */
