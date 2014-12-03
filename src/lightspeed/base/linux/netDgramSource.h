/*
 * netDgramSource.h
 *
 *  Created on: 7.7.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_LINUX_NETDGRAMSOURCE_H_
#define LIGHTSPEED_LINUX_NETDGRAMSOURCE_H_
#include "../streams/netio_ifc.h"
#include "netStream.h"
#include "../streams/memfile.h"
#include "../memory/smallAlloc.h"
#include <netdb.h>
#include "../memory/poolalloc.h"

namespace LightSpeed {

class LinuxNetDgamSource;

class LinuxNetDatagram: public INetworkDatagram, public DynObject  {
public:

	MemFile<SmallAlloc<4096> > inputData;
	MemFile<SmallAlloc<4096> > outputData;
	LinuxNetDgamSource *owner;

	byte addrbuff[64];
	socklen_t addrlen;
	PNetworkAddress assignedAddr;
	mutable natural dgrid;


	LinuxNetDatagram(RefCntPtr<LinuxNetDgamSource> owner)
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
    virtual FileOffset size() const	;
	virtual bool checkAddress(PNetworkAddress address);
	virtual ConstStringT<byte> peekOutputBuffer() const;
	///Closes writes of this stream and sends the datagram to the network
	virtual void closeOutput() {send();}




	virtual ~LinuxNetDatagram();
protected:




};


class LinuxNetDgamSource: public INetworkDatagramSource, public INetworkSocket {
public:
	LinuxNetDgamSource(natural port, natural timeout, natural startDID);

	virtual natural getDefaultWait() const {return waitForInput;}

	virtual void setWaitHandler(IWaitHandler *handler);

	virtual void setTimeout(natural time_in_ms);

	virtual natural getTimeout() const;

	virtual natural wait(natural waitFor, natural timeout) const;

	virtual PNetworkDatagram receive();

	virtual PNetworkDatagram create();

	virtual PNetworkDatagram create(PNetworkAddress adr);

	virtual integer getSocket(int index) const;

	virtual void setUID(natural id) {dgrId = id;}


protected:
	LinuxSocketResource sres;
	static int createDatagramSocket(natural port);
	friend class LinuxNetDatagram;
	PoolAlloc dgmpool;

	natural dgrId;
};

}

#endif /* LIGHTSPEED_LINUX_NETDGRAMSOURCE_H_ */
