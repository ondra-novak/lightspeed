/*
 * netStream.h
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_WINDOWS_NETSTREAM_H_
#define LIGHTSPEED_BASE_WINDOWS_NETSTREAM_H_
#include "../streams/netio_ifc.h"
#include "winsocket.h"


namespace LightSpeed {



class WindowsNetStream: public WindowsSocketResource<INetworkStream>, public INetworkSocket {
public:
	WindowsNetStream(UINT_PTR socket, natural timeout);
	virtual ~WindowsNetStream();


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

	mutable bool foundEof;
	mutable natural countReady;
	bool outputClosed;
};


}

#endif /* LIGHTSPEED_BASE_WINDOWS_NETSTREAM_H_ */
