/*
 * abstractFileIOService.h
 *
 *  Created on: 23. 2. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_STREAMS_ABSTRACTFILEIOSERVICE_H_
#define LIGHTSPEED_BASE_STREAMS_ABSTRACTFILEIOSERVICE_H_

#include "../containers/linkedList.h"
#include "fileio_ifc.h"

namespace LightSpeed {

class AbstractFileIOService: public IFileIOServices {
public:

    virtual IFileIOHandler *setHandler(ConstStrW prefix, IFileIOHandler *handler);
    virtual IFileIOHandler *findHandler(ConstStrW path) const;

protected:

    struct Registration {
    	String prefix;
    	IFileIOHandler *handler;

    	Registration(String prefix,IFileIOHandler *handler):prefix(prefix),handler(handler) {}
    };

    typedef LinkedList<Registration> RegList;

    RegList regList;

};

} /* namespace coinmon */

#endif /* LIGHTSPEED_BASE_STREAMS_ABSTRACTFILEIOSERVICE_H_ */
