/*
 * dispatcher.cpp
 *
 *  Created on: 30. 7. 2015
 *      Author: ondra
 */




#include "dispatcher.tcc"

namespace LightSpeed {

template Promise<typename AbstractDispatcher::DispatchHelper<Promise<int> >::RetV> AbstractDispatcher::dispatch(const Promise<int> &);


}
