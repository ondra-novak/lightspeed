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


namespace LightSpeed {


class LinuxSocketResource: public INetworkResource{

public:
	const int sock;


	LinuxSocketResource(int socket, natural defWait, bool noclose = false)
		: sock(socket),timeout(naturalNull),defWait(defWait),noclose(noclose) {}
	virtual ~LinuxSocketResource();

	using INetworkResource::wait;
	virtual void setWaitHandler(IWaitHandler *handler);
	virtual Pointer<IWaitHandler> getWaitHandler();
	virtual void setTimeout(natural time_in_ms);
	virtual natural getTimeout() const ;
	virtual natural wait(natural waitFor, natural timeout) const ;
	virtual natural getDefaultWait() const {return defWait;}

	natural userWait(natural waitFor, natural timeout) const;
protected:

	Pointer<IWaitHandler> handler;
	natural timeout;
	natural defWait;
	bool noclose;


};


class LinuxNetStream: public INetworkStream, public INetworkSocket {
public:
	LinuxNetStream(int socket, natural timeout);
	virtual ~LinuxNetStream();


	using INetworkResource::wait;
	virtual void setWaitHandler(IWaitHandler *handler);
	virtual void setTimeout(natural time_in_ms);
	virtual natural getTimeout() const ;
	virtual natural wait(natural waitFor, natural timeout) const ;
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
	LinuxSocketResource sres;
	mutable bool foundEof;
	bool outputClosed;
};

struct timeval millisecToTimeval(natural millisec);
natural safeSelect(integer socket, natural waitFor, natural timeout);

}

#endif /* LIGHTSPEED_BASE_LINUX_NETSTREAM_H_ */
