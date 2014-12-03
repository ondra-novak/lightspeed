 #pragma once

#include "basicTypes.h"
#include "serializer.h"
#include "../containers/optional.h"

namespace LightSpeed {


	///Interface to serialize pointer
	/**
		Interface need define type of used serializer and type of base,
		which is common to all objects that will be serialized by this interface.
		Interface itself can be bind to the another base, but there must be
		possibility to make dynamic_cast between the bases.		

	 * @tparam Serializer type of serializer used to serialize pointer
	 * @rparam BaseT common base type
	 */
	template<typename Serializer, typename BaseT>
	class IPointerSerializer: public IService {
	public:

		///Destructor
		virtual ~IPointerSerializer () {}		
		///Loads instance from the archive
		/**
		 * @param arch archive to read, must be in loading state
		 * @return pointer to created instance or NULL (it is valid value)
		 */
		virtual BaseT *load(Serializer &arch) const = 0;
		///Stores instance to the archive
		/**
		 * @param arch archive to write into. Must be in storing state
		 * @param instance instance to store
		 */
		virtual void store(Serializer &arch, const BaseT *instance) const = 0;
		///Destroyes the instance
		/** Because caller don't have information about creation and allocation
		 of the instance, it cannot call delete to destroy instance. This
		 function must handle destroying.

		 * @param instance instance to destroy
		 */
		virtual void destroy(BaseT *instance) const = 0;


		virtual TypeInfo getInterfaceType() const {
			return typeid(IPointerSerializer);
		}
	};






	///Specialization to serialize pointer
	/**
	 * When pointer is serialized, it need to solve situation, when
	 * pointer points to derived class. Then it is not possible to simply
	 * call serialize method, because it will serialize only base part 
	 * of the object, and mostly it can be impossible to serialize pointer,
	 * when it is abstract interface. 
	 *
	 * This specialization expects section that defines implementation of
	 * the IPointerSerializer interface. Implementation must implement
	 * methods load(), store() and destroy() to handle pointer serialization
	 * It also can choose best way, how to create object, especially, ho
	 * to allocate object, because serializer itself doesn't define allocator
	 * to allocate these objects.
	 *
	 * @param Type of pointer that is expected in the serialization script.	
	 *
	 * @section requires Requires
	 *
	 * It requires IPointerSerializer::AsSection instance, that contains
	 * pointer to an object implementing IPointerSerializer for given pointer type.
	 * Note that you have to declare separate section for every type of the
	 * pointer that can appear in the serialization script. To allow
	 * sharing one instance of IPointerSerializer between many base-pointer
	 * types, you can use class SrlzPtrConvert
	 * 
	 * SrlzPtrConvert<Serializer,From,To>
	 *
	 *
	 * 
	 */
	template<typename T> 
	class Serializable<T *> {
	public:

		template<typename Serializer>
		static void serialize(T *& object, Serializer &arch) {

			SrAttr<IPointerSerializer<Serializer,T> > pser(arch);
			if (arch.storing()) {
				pser->store(arch,object);
			} else if (arch.loading()) {
				T *tmp = pser->load(arch);
				std::swap(tmp,object);
				pser->destroy(tmp);
			}
		}
	};

}
