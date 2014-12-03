/*
 * netStream.h
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_WINDOWS_NETSTREAM_H_
#define LIGHTSPEED_BASE_WINDOWS_NETSTREAM_H_
#include "../streams/netio_ifc.h"



namespace LightSpeed {


class WindowsSocketResource: public LightSpeed::INetworkResource{

public:
	const UINT_PTR sock;


	WindowsSocketResource(UINT_PTR socket, natural defWait, bool noclose = false)
		: sock(socket),timeout(naturalNull),defWait(defWait),noclose(noclose) {}
	virtual ~WindowsSocketResource();

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


class WindowsNetStream: public LightSpeed::INetworkStream, public INetworkSocket {
public:
	WindowsNetStream(UINT_PTR socket, natural timeout);
	virtual ~WindowsNetStream();


	using INetworkResource::wait;
	virtual void setWaitHandler(IWaitHandler *handler);
	virtual void setTimeout(natural time_in_ms);
	virtual natural getTimeout() const ;
	virtual natural wait(natural waitFor, natural timeout) const ;
    virtual natural read(void *buffer,  natural size);
    virtual natural write(const void *buffer,  natural size) ;
	virtual natural peek(void *buffer, natural size) const ;

	natural getReadyBytes() const;
	virtual bool canRead() const;
	virtual bool canWrite() const;
	virtual integer getSocket(int idx) const;
	virtual natural getDefaultWait() const;
	virtual void flush() {}
	virtual natural dataReady() const;
	virtual void closeOutput();




protected:
	WindowsSocketResource sres;
	bool outputClosed;
};

struct timeval millisecToTimeval(natural millisec);

}

#endif /* LIGHTSPEED_BASE_WINDOWS_NETSTREAM_H_ */
