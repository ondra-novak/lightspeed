/*
 * netDgramSource.h
 *
 *  Created on: 7.7.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_WINDOWS_NETDGRAMSOURCE_H_
#define LIGHTSPEED_WINDOWS_NETDGRAMSOURCE_H_
#include "../streams/netio_ifc.h"
#include "netStream.h"
#include "../streams/memfile.h"
#include "../memory/smallAlloc.h"
#include "winsocket.h"

namespace LightSpeed {

class WindowsNetDgamSource;

class WindowsNetDatagram: public INetworkDatagram  {
public:

	MemFile<SmallAlloc<4096> > inputData;
	MemFile<SmallAlloc<4096> > outputData;
	WindowsNetDgamSource *owner;

	byte addrbuff[64];
	integer addrlen;
	PNetworkAddress assignedAddr;
	mutable natural dgrid;


	WindowsNetDatagram(RefCntPtr<WindowsNetDgamSource> owner)
		:owner(owner),addrlen(0),dgrid(naturalNull) {}


	virtual PNetworkAddress getTarget();
	virtual void send();
	virtual void sendTo(PNetworkAddress target);
	virtual void resetTarget();
	virtual void rewind();
	virtual void clear();
	virtual natural getUID() const;
    virtual natural read(void *buffer,  natural size);
    virtual natural write(const void *buffer,  natural size);
	virtual natural peek(void *buffer, natural size) const;
	virtual bool canRead() const;
	virtual bool canWrite() const;
	virtual void flush();
	virtual natural dataReady() const;
    virtual natural read(void *buffer,  natural size, FileOffset offset) const;
    virtual natural write(const void *buffer,  natural size, FileOffset offset);
    virtual void setSize(FileOffset size);
    virtual FileOffset size() const;
	virtual bool checkAddress(PNetworkAddress address);
	virtual void closeOutput();
	virtual ConstStringT<byte> peekOutputBuffer() const ;




	virtual ~WindowsNetDatagram();
protected:




};


class WindowsNetDgamSource: public WindowsSocketResource<INetworkDatagramSource>, public INetworkSocket {
public:
	WindowsNetDgamSource(natural port, natural timeout, natural startDID);

	virtual natural getDefaultWait() const {return waitForInput;}

	virtual PNetworkDatagram receive();

	virtual PNetworkDatagram create();

	virtual PNetworkDatagram create(PNetworkAddress adr);

	virtual integer getSocket(int index) const;

	virtual void setUID(natural id) {dgrId = id;}


protected:
	static UINT_PTR createDatagramSocket(natural port);
	friend class WindowsNetDatagram;

	natural dgrId;
};

}

#endif /* LIGHTSPEED_WINDOWS_NETDGRAMSOURCE_H_ */
