/*
 * dispatcher.cpp
 *
 *  Created on: 30. 7. 2015
 *      Author: ondra
 */




#include "idispatcher.tcc"
#include "message.h"

namespace LightSpeed {
	
	typedef Message<int> TestFn;

	template void IDispatcher::dispatch(const TestFn &);

	void IDispatcher::dispatch(const Promise<void> &promise)
	{
		class A : public AbstractAction {
		public:

			A(const Promise<void> &promise) :promise(promise) {}
			virtual void run() throw() 
			{
				promise.resolve();
			}

			virtual void reject() throw() 
			{
				promise.reject(CanceledException(THISLOCATION));
			}
			~A() throw() {}
		protected:
			Promise<void> promise;
		};
		AbstractAction *aa = new(getActionAllocator()) A(promise);
		dispatch(aa);
	}

	template void IDispatcher::dispatchFuture(const Future<int> &, const Promise<int> &);
	template void IDispatcher::dispatch<TestFn, int>(const TestFn &, const Promise<int> &);

}
