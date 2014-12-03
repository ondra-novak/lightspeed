#pragma once
#include "iservices.h"
#include "../containers/linkedList.h"
#include "../containers/map.h"

namespace LightSpeed {

	class Services: public IServices {
	public:
	
		using IServices::addService;
		using IServices::removeService;
		void addService(TypeInfo nfo, IInterface *ifc);
		void removeService(TypeInfo nfo);
		void removeService(TypeInfo nfo, IInterface *ifc);


	protected:
		typedef LinkedList<IInterface *> IfcStack;
		typedef Map<TypeInfo, IfcStack> ServiceMap;

		ServiceMap serviceMap;

		virtual void *proxyInterface(IInterfaceRequest &p);
		virtual const void *proxyInterface(const IInterfaceRequest &p) const;



	};

}
