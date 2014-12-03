/** @file
 * File contains declaration of axioms how the basic types should be serialized
 */
#ifndef LIGHTSPEED_SERIALIZE_BASICTYPES_H_
#define LIGHTSPEED_SERIALIZE_BASICTYPES_H_

#include "../types.h"
#include "binaryType.h"

namespace LightSpeed
{

   
    ///Declaration of general type table
    /** General type table for Serializer is template class Serializable with
     * the one argument. It defines, how the type will be serialized. Using
     * the specialization feature of C++, you can anytime add entries into this
     * table and for each type specify serialization handler, which provides the
     * serialization. You can also use partial specialization to specify
     * handler for various template classes.
     * 
     * The table has one record with meaning "default" handler. If
     * there is no match for the type in  the table, default handler is
     * used. Default handler always expects, that the type is class
     * or struct with implemented method void serializer(archive). You
     * can use this default record for all newly created classes and
     * specify this method for them. There is no more work to do to 
     * handle serialization by the each class with such a method
     * 
     * Yes, it is legal to add record into the table for each type that
     * is serialized, but above way is easer
     * 
     * @note you can create own tables, complette independed or 
     * derived from Serializable. To use own tables, specify 
     * the table name as second parameter of the serializer
     */
    template<class T> 
    class Serializable {               
    public:
        
        template<typename Archive>
        static void serialize(T &object, Archive &arch) {
            object.serialize(arch);
        }
        
    };
    
    
    ///This class implements serialization for the "atom types"
    /** Atom type is the type, which cannot be more divided into the
     * sub-types. Every formatter and parser should support all atom types.
     * Atom types are by default all standard types such a int, char, bool, etc
     * 
     * If you enforce any type to be the atom type, simply create
     * specialization for the type table (Serializable) and derive this class.
     * No more code is needed.
     */
    class SerializableAtom {
    public:
        
        template<typename T, typename Archive>
        static void serialize(T &object, Archive &arch) {
            //every formatter have to implement method exchange
            //using the operator-> whe can order the formatter directly
            //without processing type table ... (because it would
            // to create infinite loop)
            arch.getFormatter().exchange(object);
        }
    };
    

    //Records in the type table using the SerializableAtom
    template<> class Serializable<bool>: public SerializableAtom {};
    template<> class Serializable<signed char>: public SerializableAtom {};
    template<> class Serializable<unsigned char>: public SerializableAtom {};
	template<> class Serializable<char>: public SerializableAtom {};
    template<> class Serializable<signed short>: public SerializableAtom {};
    template<> class Serializable<unsigned short>: public SerializableAtom {};
    template<> class Serializable<signed int>: public SerializableAtom {};
    template<> class Serializable<unsigned int>: public SerializableAtom {};
    template<> class Serializable<signed long>: public SerializableAtom {};
    template<> class Serializable<unsigned long>: public SerializableAtom {};
    template<> class Serializable<signed long long>: public SerializableAtom {};
    template<> class Serializable<unsigned long long>: public SerializableAtom {};
	template<> class Serializable<wchar_t>: public SerializableAtom {};
    template<> class Serializable<float>: public SerializableAtom {};
    template<> class Serializable<double>: public SerializableAtom {};
	template<> class Serializable<BinaryType>: public SerializableAtom {};
    
	class SerializeEnum {
	public:
		template<typename T, typename Archive>
		static void serialize(T &object, Archive &arch) {
			if (arch.storing()) {
				int k = object;
				arch(k);
			} else {
				int k;
				arch(k);
				object = (T)k;
			}
		}

	};
	

	




} // namespace LightSpeed

#endif /*BASICTYPES_H_*/
