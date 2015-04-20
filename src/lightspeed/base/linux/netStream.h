/*
 * netStream.h
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_LINUX_NETSTREAM_H_
#define LIGHTSPEED_BASE_LINUX_NETSTREAM_H_
#include "../streams/netio_ifc.h"
#include <sys/time.h>
#include "linsocket.h"

namespace LightSpeed {



class LinuxNetStream: public LinuxSocketResource<INetworkStream>, public INetworkSocket {
public:
	LinuxNetStream(int socket, natural timeout);
	virtual ~LinuxNetStream();


    virtual natural read(void *buffer,  natural size);
    virtual natural write(const void *buffer,  natural size) ;
	virtual natural peek(void *buffer, natural size) const ;
	virtual bool canRead() const;
	virtual bool canWrite() const;
	virtual integer getSocket(int idx) const;
	virtual natural getDefaultWait() const;
	virtual void flush() {}
	virtual natural dataReady() const;
	virtual void closeOutput();


protected:
	mutable bool foundEof;
	mutable natural countReady;
	bool outputClosed;

};

struct timeval millisecToTimeval(natural millisec);
natural safeSelect(integer socket, natural waitFor, natural timeout);

}

#endif /* LIGHTSPEED_BASE_LINUX_NETSTREAM_H_ */
