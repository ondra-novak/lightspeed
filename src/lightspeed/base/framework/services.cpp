
#include "services.h"
#include "../containers/map.tcc"


namespace LightSpeed {



	void Services::addService( TypeInfo nfo, IInterface *ifc )
	{
		if (ifc == 0) {removeService(nfo);return;}
		IfcStack *stack = serviceMap.find(nfo);
		if (stack == 0) {
			stack = &serviceMap.insert(nfo,IfcStack()).getNext().value;
		}
		stack->insertFirst(ifc);
	}

	void Services::removeService( TypeInfo nfo )
	{
		IfcStack *stack = serviceMap.find(nfo);
		if (stack == 0) return;
		stack->eraseFirst();
		if (stack->empty())
			serviceMap.erase(nfo);
	}

	void Services::removeService( TypeInfo nfo, IInterface *ifc )
	{
		if (ifc == 0) {removeService(nfo);return;}
		IfcStack *stack = serviceMap.find(nfo);
		if (stack == 0) return;
		IfcStack::Iterator iter = stack->getFwIter();
		while (iter.hasItems()) {
			if (iter.peek() == ifc) {
				stack->erase(iter);
				return;
			} else {
				iter.skip();
			}
		}
	}

	void * Services::proxyInterface( IInterfaceRequest &p )
	{
		IfcStack *stack = serviceMap.find(p.getType());
		if (stack == 0) return 0;
		else return p.getInterface(stack->getFirst());
	}

	const void * Services::proxyInterface( const IInterfaceRequest &p ) const
	{
		const IfcStack *stack = serviceMap.find(p.getType());
		if (stack == 0) return 0;
		else return p.getInterface(stack->getFirst());
	}
}
