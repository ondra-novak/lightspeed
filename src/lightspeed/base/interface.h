/** @file
 * intrerface.h
 *
 *  Created on: 25.3.2010
 *      Author: ondra
 */

#pragma once
#ifndef _INTERFACE_H__lightspeed_ondra_2010_
#define _INTERFACE_H__lightspeed_ondra_2010_
#include "../base/typeinfo.h"
#include "memory/pointer.h"


namespace LightSpeed {


	///Helps to access other interface in the object
	/**
	 * Every object can implement more interfaces than one. One of interfaces
	 * are used to carry instance of the object. This interface is called main.
	 *
	 * Main interface should extends this interface IInterface, which supplies
	 * function to access other interfaces.
	 *
	 * The most common way to access other interface is to use dynamic_cast operator.
	 * But this require, that object implements every function of all exported
	 * interfaces. It is very hard to access delegated interface or make proxy
	 * object.
	 *
	 * Interface IInterface allows pass requestes through proxies using function proxyInterface.
	 * In default configuration every class which inherits the IInterface implements this
	 * function using dynamic_cast to the itself, so interface supplies
	 * dynamic_cast as expected. But proxy can redefine that function and
	 * pass request to the other objects. There is only one condition, that
	 * other objects must meet. They have to implement IInterface too.
	 *
	 * @see getIfc, getIfcPtr
	 *
	 */
	class IInterface {
	public:


		///Retrieves interface pointer
		/**
		 * @tparam Ifc use name of interface (as type) which you require. You
		 * should use interface, not name of implementating class, even if it
		 * will work.
		 * @return pointer to interface, or nil in case, that interface is not implemented
		 */
		template<typename Ifc>
		Pointer<Ifc> getIfcPtr();
		///Retrieves interface pointer
		/**
		 * @tparam Ifc use name of interface (as type) which you require. You
		 * should use interface, not name of implementating class, even if it
		 * will work.
		 * @return pointer to interface, or nil in case, that interface is not implemented
		 */
		template<typename Ifc>
		Pointer<const Ifc> getIfcPtr() const;

		///Retrieves interface reference
		/**
		 * @tparam Ifc use name of interface (as type) which you require. You
		 * should use interface, not name of implementating class, even if it
		 * will work.
		 * @return reference to the interface
		 * @exception InterfaceNotImplementedException thrown when interface is not implemented.
		 *
		 * @note use this function when you are sure, that interface must be implemented. You
		 * don't need to test return value and exception guards application against to
		 * dereferencing of NULL pointer
		 */
		template<typename Ifc>
		Ifc &getIfc();
		///Retrieves interface reference
		/**
		 * @tparam Ifc use name of interface (as type) which you require. You
		 * should use interface, not name of implementating class, even if it
		 * will work.
		 * @return reference to the interface
		 * @exception InterfaceNotImplementedException thrown when interface is not implemented.
		 *
		 * @note use this function when you are sure, that interface must be implemented. You
		 * don't need to test return value and exception guards application against to
		 * dereferencing of NULL pointer
		 */
		template<typename Ifc>
		const Ifc &getIfc() const;


		///dtor
		virtual ~IInterface() {}
	protected:


		///Interface which allows to access to the request
		class IInterfaceRequest {
		public:

			///Tries to convert pointer to required interface
			/**
			 * @param ifc pointer to object to convert
			 * @return pointer to interface, or NULL, when impossible
			 */
			virtual void *getInterface(IInterface *ifc) = 0;
			///Tries to convert pointer to required interface
			/**
			 * @param ifc pointer to object to convert
			 * @return pointer to interface, or NULL, when impossible
			 */
			virtual const void *getInterface(const IInterface *ifc) const = 0;
			///Retrieves TypeInfo object containing information about requested type
			/**
			 * @return TypeInfo object containing information about requested type
			 */
			virtual TypeInfo getType() const = 0;
		};

		template<typename Ifc>
		class IfcProxy;

	public:
		///Calls to proxy request into delegated object
		/**
		 * @param p reference to request
		 * @return pointer to interface, or NULL when impossible
		 *
		 * @note Default implementation calls IInterfaceRequest::getInterface() to this
		 */
		virtual void *proxyInterface(IInterfaceRequest &p) {
			return p.getInterface(this);
		}
		///Calls to proxy request into delegated object
		/**
		 * @param p reference to request
		 * @return pointer to interface, or NULL when impossible
		 *
		 * @note Default implementation calls IInterfaceRequest::getInterface() to this
		 */
		virtual const void *proxyInterface(const IInterfaceRequest &p) const {
			return p.getInterface(this);
		}

	};

	class InterfaceNotImplementedException;

}

#endif /* _INTERFACE_H__lightspeed_ondra_2010_ */
