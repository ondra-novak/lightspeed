/*
 * abstractFileIOService.cpp
 *
 *  Created on: 23. 2. 2015
 *      Author: ondra
 */

#include "abstractFileIOService.h"

namespace LightSpeed {


IFileIOHandler* LightSpeed::AbstractFileIOService::setHandler(ConstStrW prefix, IFileIOHandler* handler) {

	RegList::Iterator iter = regList.getFwIter();

	while (iter.hasItems()) {
		const Registration &r = iter.peek();
		if (r.prefix == prefix) {
			IFileIOHandler *prev = r.handler;
			if (handler != 0) {
				regList.insertBefore(iter,Registration(r.prefix,handler));
			}
			regList.erase(iter);
			return prev;
		} else if (r.prefix.length() < prefix.length() || (r.prefix.length() == prefix.length() && r.prefix < prefix))  {
			regList.insertBefore(iter,Registration(prefix,handler));
			return NULL;
		} else  {
			iter.skip();
		}
	}
	regList.add(Registration(prefix,handler));
	return 0;

}

IFileIOHandler* LightSpeed::AbstractFileIOService::findHandler(ConstStrW path) const {
	RegList::Iterator iter = regList.getFwIter();
	while (iter.hasItems()) {
		const Registration &r = iter.getNext();
		if (path.head(r.prefix.length()) == r.prefix) return r.handler;
	}
	return NULL;

}



} /* namespace coinmon */
