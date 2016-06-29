#include "netio.tcc"

namespace LightSpeed {

	//----------------------- IMPLEMENTATION -----------------------
	///Interface to access network address object


	///Converts instance to the string representation
	StringA NetworkAddress::asString(bool resolve) const {
		if (addr == nil)
			throwNullPointerException(THISLOCATION);
		return addr->asString(resolve);
	}

	NetworkAddress NetworkAddress::resolve(ConstStrA adr, natural port_def)
	{
		return NetworkAddress(INetworkServices::
			getNetServices().createAddr(adr, port_def));
	}
	NetworkAddress NetworkAddress::resolve(ConstStrA adr, ConstStrA service)
	{
		return NetworkAddress(INetworkServices::
			getNetServices().createAddr(adr, service));
	}

	NetworkAddress NetworkAddress::ipLocal(natural port, bool listen)
	{
		if (listen) return NetworkAddress(INetworkServices::
			getNetServices().createAddr(nil, port));
		else return NetworkAddress(INetworkServices::
			getNetServices().createAddr(INetworkAddress::getLocalhost(), port));
	}

	NetworkAddress NetworkAddress::ipRemote(ConstStrA address, natural port)
	{
		return NetworkAddress(INetworkServices::
			getNetServices().createAddr(address, port));

	}

	bool NetworkAddress::equalTo(const NetworkAddress &other) const
	{
		if (other.addr == nil) return addr == nil;
		return other.addr->equalTo(*addr);
	}

	NetworkStreamSource::NetworkStreamSource(
		NetworkAddress address, natural limitConnections /*= 1*/,
		natural limitWaiting /*= 30000*/,
		natural streamTimeout /* = naturalNull */,
		StreamOpenMode::Type mode /*= StreamOpenMode::useAddress*/)
	:PNetworkStreamSource(INetworkServices::getNetServices().createStreamSource(
			address.getHandle(), mode, limitConnections, limitWaiting, streamTimeout))
	{
	}
	NetworkStreamSource::NetworkStreamSource(
		natural port, natural limitConnections /*= 1*/,
		natural limitWaiting /*= 30000*/,
		natural streamTimeout /* = naturalNull */,
		bool localhost /*=false*/)
	:PNetworkStreamSource(
		INetworkServices::getNetServices().createStreamSource(
			NetworkAddress::ipLocal(port, !localhost).getHandle(),
			StreamOpenMode::passive,
			limitConnections,
			limitWaiting, streamTimeout))
	{}

	bool NetworkStreamSource::hasItems() const
	{
		stream = nil;
		return this->ptr->hasItems();
	}


	const PNetworkStream &NetworkStreamSource::getNext()
	{
		stream = nil;
		return (stream = this->ptr->getNext());
	}


	NetworkAddress NetworkStreamSource::getLocalAddress() const
	{
		stream = nil;
		return this->ptr->getLocalAddr();
	}

	NetworkAddress NetworkStreamSource::getRemoteAddress() const
	{
		stream = nil;
		return this->ptr->getPeerAddr();
	}

	void NetworkStreamSource::wait() {
		stream = nil;
		this->ptr->wait();
	}

	bool NetworkStreamSource::wait(natural timeoutms) {
		stream = nil;
		return this->ptr->wait(this->ptr->getDefaultWait(), timeoutms) != 0;
	}



}
