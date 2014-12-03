#pragma once
#include "../typeinfo.h"
#include "../interface.h"


namespace LightSpeed {


	///Extended interface to simply registration of the service
	/**
	 * Services are objects which implements IInterface. If you want
	 * to register service, you need supply pointer to object and 
	 * type description of the interface which object implements. You
	 * can register one object with more interface. Unfortunately, you
	 * need to carry two variables for single action: Pointer and type.
	 * 
	 * To simplify registration, you can extend IService and define
	 * set of interfaces implemented by the object. The object itself knows
	 * which interfaces are implemented and this information is carried
	 * with object itself.
	 *
	 * Object must implement method getInterfaceType() with or without 
	 * argument. With argument allows object to implement multiple 
	 * of interfaces
	 *
	 */
	 
	class IService: public IInterface {
	public:
		virtual TypeInfo getInterfaceType() const = 0;

		virtual TypeInfo getInterfaceType(natural index) const  {
			if (index == 0) return getInterfaceType();
			else return TypeInfo();
		}
	};


	class IServices: public IInterface {
	public:
		 ///Registers service to the application
		 /**  
		  * Service is implemented interface, which can be retrieved through
		  * IInterface object. This function allows to dynamically register
		  * additional objects to the application. 
		  *
		  * @param ifctype TypeInfo object describes interface
		  * @param ifc pointer to object, which implements the interface. 
		  *   Object must also implement IInterface interface
		  *
		  * @note You can register more objects with the interface, but the
		  * last registered is active. Objects are ordeded in LIFO structure
		  * so you can temporary replace implementation of interface and
		  * later restore old implementation.
		  */
		 virtual void addService(TypeInfo ifctype, IInterface *ifc) = 0;
		 ///Removes registered service
		 /**
		  * @param ifctype type description of interface, which implementation
		  * will be removed. Function also restores object which has been active
		  * before last one replaced it.
		  */		  
		 virtual void removeService(TypeInfo ifctype) = 0;

		 ///Removes register service
		 /**
		  * Removes service identified by pointer to implementation
		  * used during addService
		  *
          * @param nfo type of interface which implementation will be removed
		  * @param ifc pointer to object. If there is more objects registered
		  *  to the interface, function leaves other objects untouched. Function
		  *  can be used for object, which is not currently active (top of the stack)
		  *  If this parameter is zero, function removes currently active object
		  */
		  
		 virtual void removeService(TypeInfo nfo, IInterface *ifc) = 0;

		 ///Adds service which extends IService class
		 /** Objects implementing IService carries description of implemented interfaces.
		  *
		  * @param svc reference to object to register		  
		  *
		  * @note function registers all interfaces that object supports
		  */
		 void addService(IService &svc) {
			 natural idx = 0;
			 TypeInfo n;
			 TypeInfo t = svc.getInterfaceType(0);
			 while (t != n) {
				 addService(t,&svc);
				 idx++;
				 t = svc.getInterfaceType(idx);

			 }
		 }
		 ///Removes service which extends IService class
		 /** Objects implementing IService carries description of implemented interfaces.
		  *
		  * @param svc reference to object to remove
		  *
		  * @note function removes all interfaces that object supports
		  */
		 void removeService(IService &svc) {
			 natural idx = 0;
			 TypeInfo nul;
			 TypeInfo t = svc.getInterfaceType(0);
			 while (t != nul) {
				 removeService(t,&svc);
				 idx++;
				 t = svc.getInterfaceType(idx);

			 }
		 }
		 ///Registers one interface with specified object
		 /**
		  * @param svc reference to object implementing some interface
		  * @param index index of interface. Object should order implemented
		  *  interfaces by 
		  */
		  
		 void addService(IService &svc, natural index) {
			 addService(svc.getInterfaceType(index),&svc);
		 }
		 void removeService(IService &svc, natural index) {
			 removeService(svc.getInterfaceType(index),&svc);
		 }

		 virtual ~IServices() {}
	};



}
