/*
 * interface.tcc
 *
 *  Created on: 23.9.2011
 *      Author: ondra
 */

#ifndef _INTERFACE_TCC__lightspeed_ondra_2010_
#define _INTERFACE_TCC__lightspeed_ondra_2010_

#include "interface.h"
#include "exceptions/exception.h"

namespace LightSpeed {


template<typename Ifc>
class IInterface::IfcProxy: public IInterface::IInterfaceRequest {
public:

	virtual void *getInterface(IInterface *ifc) {
		return dynamic_cast<Ifc *>(ifc);
	}
	virtual const void *getInterface(const IInterface *ifc) const {
		return dynamic_cast<const Ifc *>(ifc);
	}
	virtual TypeInfo getType() const {
		return typeid(Ifc);
	}
};



template<typename Ifc>
Pointer<Ifc> IInterface::getIfcPtr() {
	IInterface *x = this;
	if (x == 0) return 0;
	IfcProxy<Ifc> proxy;
	return reinterpret_cast<Ifc *>(proxyInterface(proxy));
}
template<typename Ifc>
Pointer<const Ifc> IInterface::getIfcPtr() const {
	const IInterface *x = this;
	if (x == 0) return 0;
	IfcProxy<Ifc> proxy;
	return reinterpret_cast<const Ifc *>(proxyInterface(proxy));
}

class InterfaceNotImplementedException: public Exception {
public:
	LIGHTSPEED_EXCEPTIONFINAL;

	InterfaceNotImplementedException(const ProgramLocation &loc, TypeInfo ifcReq, TypeInfo object)
		:Exception(loc),ifcReq(ifcReq),object(object) {}

	TypeInfo getRequestedInterface() const {return ifcReq;}
	TypeInfo getObject() const {return object;}

	static LIGHTSPEED_EXPORT const char *msgText;;
protected:
	TypeInfo ifcReq,object;

	void message(ExceptionMsg &msg) const {
		msg(msgText) << object.name() << ifcReq.name();
	}
};

template<typename Ifc>
Ifc &IInterface::getIfc() {
	Ifc *x = getIfcPtr<Ifc>();
	if (x == 0) {
		throw InterfaceNotImplementedException(THISLOCATION, TypeInfo(typeid(Ifc)), TypeInfo(typeid(*this)));
	}
	return *x;
}
template<typename Ifc>
const Ifc &IInterface::getIfc() const {
	const Ifc *x = getIfcPtr<Ifc>();
	if (x == 0) {
		throw InterfaceNotImplementedException(THISLOCATION, TypeInfo(typeid(Ifc)), TypeInfo(typeid(Ifc)));
	}
	return *x;

}


}  // namespace LightSpeed

#endif /* INTERFACE_TCC_ */
