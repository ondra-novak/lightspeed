/*
 * netService.cpp
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#include "netService.h"
#include "../text/textFormat.tcc"
#include "../memory/staticAlloc.h"
#include "../exceptions/netExceptions.h"
#include "netAddress.h"
#include "networkEventListener.h"
#include "netStreamSource.h"
#include <string.h>
#include "netDgramSource.h"
#include <stdlib.h>
#include "../text/textParser.tcc"
#include "../memory/singleton.h"
#include "../debug/dbglog.h"
#include "namedPipe.h"
#include "securityattrs.h"
#include "networkEventListener2.h"

namespace LightSpeed {

#pragma comment(lib, "ws2_32.lib")

PNetworkAddress WindowsNetService::createAddr(ConstStrA remoteAddr,		
		natural Port, IPVersion ver)
{
	return WindowsNetAddress::createAddr(remoteAddr,Port,ver);
}

PNetworkAddress WindowsNetService::createAddr(ConstStrA remoteAddr,
		ConstStrA service, IPVersion ver) {
	return WindowsNetAddress::createAddr(remoteAddr,service,ver);
}

PNetworkAddress WindowsNetService::createAddr(const void *addr, natural len)
{
	return WindowsNetAddress::createAddr(addr,len);	
}

PNetworkAddress WindowsNetService::createAddr( ConstStrA remoteAddr,natural port )
{
	if (remoteAddr == INetworkAddress::getLocalhost())
		return createAddr(getLoopbackAddr(ipVerAny),port);
	else
		return WindowsNetService::createAddr(remoteAddr,port,ipVerAny);
}

PNetworkAddress WindowsNetService::createAddr( ConstStrA remoteAddr,ConstStrA service )
{
	if (remoteAddr == INetworkAddress::getLocalhost())
		return createAddr(getLoopbackAddr(ipVerAny),service);
	else
		return WindowsNetService::createAddr(remoteAddr,service,ipVerAny);

}
static INetworkServices *globServices = 0;



INetworkServices & INetworkServices::getNetServices()
{
	if (globServices) return *globServices;
	else {
		globServices = &Singleton<WindowsNetService>::getInstance();
		return *globServices;
	}
}




void INetworkServices::setIOServices(INetworkServices *newServices)
{
	globServices = newServices;
}



PNetworkEventListener WindowsNetService::createEventListener()
{
	return new WinNetworkEventListener;
}



PNetworkStreamSource WindowsNetService::createStreamSource(
	PNetworkAddress address,StreamOpenMode::Type mode,
	natural count, natural timeout, natural defTm)
{
	WindowsNetAddress *addr = dynamic_cast<WindowsNetAddress *>(address.get());
	if (addr == 0) throw NetworkInvalidAddressException(THISLOCATION,address);
	bool passive;
	switch (mode) {
		case StreamOpenMode::active: passive = false;break;
		case StreamOpenMode::passive: passive = true;break;
		default: passive = addr->getAddrInfo()->ai_flags & AI_PASSIVE;
	}
	if (passive) {
		return new WindowsNetAccept(address,count,timeout,defTm,this);
	} else {
		return new WindowsNetConnect(address,count,timeout,defTm,this);
	}
}

PNetworkDatagramSource WindowsNetService::createDatagramSource(
				natural port, natural timeout ) {

	return new WindowsNetDgamSource(port,timeout,1);

}

WindowsNetService::WindowsNetService()
{
//	LogObject lg(THISLOCATION);

	SOCKET i = socket(AF_INET6, SOCK_STREAM,0);
	if (i == INVALID_SOCKET) {
//		lg.debug("IPv6 not available");
		useIPv4 = true;
	} else {
		closesocket(i);
//		lg.debug("IPv6 is available");
		useIPv4 = false;
	}
}

WindowsNetService::IPVersion WindowsNetService::getAvailableVersions() const
{
	return useIPv4?ipVer4:ipVerAny;
}

ConstStrA WindowsNetService::getLoopbackAddr( IPVersion ver ) const
{
	switch (ver) {
		case ipVer4: return "127.0.0.1";
		case ipVer6: return "::1";
		default: return getLoopbackAddr(useIPv4?ipVer4:ipVer6);		
	}
	throw;
}

PNetworkWaitingObject WindowsNetService::createWaitingObject()
{
	throwUnsupportedFeature(THISLOCATION,this,"createWaitingObject is not supported yet under Windows Platform");
	throw;
}


PNetworkStreamSource WindowsNetService::createNamedPipe( bool server, 
	ConstStrW pipeName, 
	PipeMode mode, 
	natural maxInstances,
	ConstStrA securityDesc, 
	natural connectTimeout, natural streamDefTimeout )
{

	String tmp;
	ConstStrW prefix = L"\\\\.\\pipe\\";
	if (pipeName.head(9) != prefix) {
		tmp = prefix+pipeName;
	} else {
		tmp = pipeName;
	}

	if (server) {


		DWORD openMode = 0;
		switch(mode) {
		case pmRO: openMode = PIPE_ACCESS_INBOUND;break;
		case pmWO: openMode = PIPE_ACCESS_OUTBOUND;break;
		case pmRW: openMode = PIPE_ACCESS_DUPLEX;break;
		}

		DWORD maxInst = (DWORD)(maxInstances >= PIPE_UNLIMITED_INSTANCES? PIPE_UNLIMITED_INSTANCES:maxInstances);

		class SecAttrs: public ISecurityAttributes {
		public:
			SecAttrs(const SecurityAttributes_t &sc):sc(sc) {}

			virtual LPSECURITY_ATTRIBUTES getSecurityAttributes() const
			{
				return &const_cast<SecAttrs *>(this)->sc;
			}

			SecurityAttributes_t sc;

		};

		AllocPointer<ISecurityAttributes> secptr;
		
		if (securityDesc=="@low") secptr = new SecAttrs(SecurityAttributes_t::getLowIntegrity());
		else if (securityDesc=="@sandbox") secptr = new SecAttrs(SecurityAttributes_t::getAppCointanerIntegrity());

		return new NamedPipeServer(tmp,openMode,0,maxInst,2048,2048,connectTimeout,streamDefTimeout,secptr.detach());


	} else {

		Timeout tmw(connectTimeout);
		do {
			try {
				DWORD openMode = 0;
				switch(mode) {
				case pmRO: openMode = GENERIC_READ;break;
				case pmWO: openMode = GENERIC_WRITE;break;
				case pmRW: openMode = GENERIC_READ|GENERIC_WRITE;break;
				}

				return new NamedPipeClient(tmp,openMode,maxInstances,streamDefTimeout);
			} catch (ErrNoException &e) {
				if (e.getErrNo() == ERROR_PIPE_BUSY) {
					WaitNamedPipeW(tmp.cStr(),tmw.isInfinite()?NMPWAIT_WAIT_FOREVER:(DWORD)tmw.getRemain().msecs());
				} else {
					throw;
				}
			}
		} while (true);

	}


}

WindowsNetService::InitWinSock::InitWinSock()
{
	int res =WSAStartup(MAKEWORD(2,2),this);
	if (res != 0) throw ErrNoException(THISLOCATION,res);
}

WindowsNetService::InitWinSock::~InitWinSock()
{
	WSACleanup();
}
}

