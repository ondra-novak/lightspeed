/**
 * @file 
 * Contains implementation of Any class and AnyCpy class
 * 
 * @see LightSpeed::Any, LightSpeed::AnyCpy
 * 
 * $Id: any.h 240 2011-10-18 12:51:20Z ondrej.novak $
 */

#ifndef ANY_H_
#define ANY_H_

#include <typeinfo>
#include "../memory/stdAllocator.h"
#include "../types.h"
#include "../invokable.h"
#include "../exceptions/anyExcept.h"
#include "../memory/allocatorBase.h"


namespace LightSpeed
{


	template<typename Allocator>
	class IAnyTypeDesc {
	public:

		typedef typename Allocator::template BlockDecl<byte>::Block DataBlock;

        ///Retrieves size in bytes of type that object describes
        /**
         * @return size in bytes
         */
        virtual natural getTypeSize() const = 0;
        ///Retrieves reference to type_info descriptor of the type that object describes
        /**
         * @return reference to type_info descriptor
         */
        virtual const std::type_info &getTypeId() const = 0;
        
		virtual const std::type_info &getDynamicTypeId(const void *data) const = 0;

		///Allows to reinterpret cast the given pointer and throw it
        /**
         * This function is used to provide legacy runtime conversion between 
         * derived and base class. Because standard C++ doesn't contain
         * tool to make this conversion directly, using this function, 
         * Any class is able to throw pointer of Derived type and catch is as
         * Base type.
         * 
         * @param instance pointer to the variable to throw
         */
        virtual void throwPtr(const void *instance) const = 0;
        

		///Clones object
		virtual void *clone(DataBlock &alloc, const void *obj) const = 0;


		///destroy object
		virtual void destroy(DataBlock &alloc, const void *obj) const = 0;

		///Converts object
		/**
		 By default conversions are not defined, excluding some special cases.
		 You can define own conversion by specializing this function, or 
		 by specialization of AnyConvert<T>

		 @param targetType required target type
		 @param src pointer to source object
		 @param target pointer to empty space in memory where target should be constructed
		 @retval NULL conversion impossible
		 @retval ptr pointer to required object (points to target, or inside of target)
		 */
		 
		virtual void *convert(const std::type_info &targetType, const void *src, void *target) const = 0;

		virtual ~IAnyTypeDesc() {}
	};

	template<typename Allocator>
	class NullAnyTypeDesc: public IAnyTypeDesc<Allocator> {
	public:
		typedef typename IAnyTypeDesc<Allocator>::DataBlock DataBlock;

		virtual natural getTypeSize() const {return 0;}
		virtual const std::type_info &getTypeId() const {return typeid(NullType);}
		virtual const std::type_info &getDynamicTypeId(const void *data) const {return typeid(NullType);}
		virtual void throwPtr(const void *instance) const {throw nil;}
		virtual void *clone(DataBlock &alloc, const void *obj) const {return const_cast<void *>(obj);}
		virtual void destroy(DataBlock &alloc, const void *obj) const {}
		virtual void *convert(const std::type_info &targetType, const void *src, void *target) const {return 0;}
		static const NullAnyTypeDesc *getInstance() {
			static NullAnyTypeDesc desc;
			return &desc;
		}
	};

	template<typename T>
	struct AnyConvert {
		static void *conv(const std::type_info &targetType, const void *src, void *target) {
			return 0;
		}
	};

	template<typename T, typename Allocator>
	class TypedAnyTypeDesc: public IAnyTypeDesc<Allocator> {
	public:
		typedef typename IAnyTypeDesc<Allocator>::DataBlock DataBlock;

		virtual natural getTypeSize() const {return sizeof(T);}
		virtual const std::type_info &getTypeId() const {return typeid(T);}
		virtual const std::type_info &getDynamicTypeId(const void *data) const {
			const T *p = reinterpret_cast<const T *>(data);
			return typeid(*p);
		}
		virtual void throwPtr(const void *instance) const {
			throw reinterpret_cast<const T *>(instance);
		}
		virtual void *clone(DataBlock &alloc, const void *obj) const {
			alloc.alloc(sizeof(T));			
			alloc.setUsed(0,true);
			T *res = new((void *)alloc.getItem(0)) T(*reinterpret_cast<const T *>(obj));
			return res;
		}
		virtual void destroy(DataBlock &alloc, const void *obj) const {
			const_cast<T *>(reinterpret_cast<const T *>(obj))->~T();
			alloc.setUsed(0,false);
			alloc.free();
		}

		virtual void *convert(const std::type_info &targetType, const void *src, void *target) const {
			return AnyConvert<T>::conv(targetType,src,target);
		}

		virtual void *load(DataBlock &alloc, const T &value) const {
			return clone(alloc,&value);
		}

		static const TypedAnyTypeDesc *getInstance() {
			static TypedAnyTypeDesc desc;
			return &desc;
		}

	};
/*
	template<typename T, int n, typename Allocator>
	class TypedAnyTypeDesc<T[n],Allocator>: public IAnyTypeDesc<Allocator> {
	public:
		typedef typename IAnyTypeDesc<Allocator>::DataBlock DataBlock;


		virtual natural getTypeSize() const {return sizeof(T[n]);}
		virtual const std::type_info &getTypeId() const {return typeid(T[n]);}
		virtual const std::type_info &getDynamicTypeId(const void *data) const 
			{return typeid(T[n]);}			
		virtual void throwPtr(const void *instance) const {
			throw nil;
		}
		virtual void *clone(Allocator &alloc, const void *obj) const {
			return const_cast<void *>(obj);
		}
		virtual void destroy(Allocator &alloc, const void *obj) const {

		}

		virtual void *convert(const std::type_info &targetType, const void *src, void *target) const {
			return AnyConvert<T>::conv(targetType,src,target);
		}


		virtual void *load(Allocator &alloc, T value[n]) const {
			return const_cast<void *>((const void *)value);
		}

		static const TypedAnyTypeDesc *getInstance() {
			static TypedAnyTypeDesc desc;
			return &desc;
		}

	};


*/
	template<typename Allocator>
	class AnyT {
	
	public:


		~AnyT() {
			clear();
		}

		AnyT():typeDesc(NullAnyTypeDesc<Allocator>::getInstance())
			,dataBlock(Allocator().template create<byte>()) {}

		AnyT(NullType x)
			:typeDesc(NullAnyTypeDesc<Allocator>::getInstance())
			,dataBlock(Allocator().template create<byte>()) {}



		AnyT(const AnyT &other):typeDesc(other.typeDesc),dataBlock(other.dataBlock) {
			typeDesc->clone(dataBlock,other.getData());
		}

		template<typename T>
		AnyT(const T &object)
			:typeDesc(TypedAnyTypeDesc<T,Allocator>::getInstance())
			,dataBlock(Allocator().template create<byte>()) 
		{
			try {
				static_cast<const TypedAnyTypeDesc<T,Allocator> *>(
					typeDesc)->load(dataBlock,object);
			} catch (...) {
				dataBlock.free();
				Exception::rethrow(THISLOCATION);
			}
		}

		template<typename T>
		AnyT(const T &object, const Allocator &alloc)
			:typeDesc(TypedAnyTypeDesc<T,Allocator>::getInstance())
			,dataBlock(alloc.template create<byte>()) 
		{	
			data = static_cast<const TypedAnyTypeDesc<T,Allocator> *>(
				typeDesc)->load(dataBlock,object);
		}

		const std::type_info &getTypeId() const {
			return typeDesc->getTypeId();
		}
		const std::type_info &getDynamicTypeId() const {
			return typeDesc->getDynamicTypeId(getData());
		}
	
		AnyT &operator=(const AnyT &other) {
			if (getData() != other.getData()) {
				clear();
				typeDesc = other.typeDesc;
				typeDesc->clone(dataBlock,other.getData());
			}
			return *this;
		}
	

		bool operator==(NullType nl) const 
			{return typeDesc == NullAnyTypeDesc<Allocator>::getInstance();}
		bool operator!=(NullType nl) const 
			{return typeDesc != NullAnyTypeDesc<Allocator>::getInstance();}

		///Retrieves value as pointer
		/**
		 @param convBuff contains space to store T. If buffer is
			NULL, conversion is not applied. Otherwise, function
			tries to convert value into the buffer. If
			conversion is successful, returned pointer into
			the buffer
		 @return pointer to required value, or NULL, if conversion
			cannot be performed.
		*/
		template<typename T>
		const T *get(void *convBuff = 0) const {
			
			const std::type_info &myId = typeid(T);
			if (getTypeId() == myId) {
				return reinterpret_cast<const T *>(getData());
			}

			if (convBuff) {
				void *res = typeDesc->convert(myId,getData(),convBuff);
				if (res) {
					return reinterpret_cast<const T *>(res);
				}
			}

			try {
				typeDesc->throwPtr(getData());
			} catch (const T *x) {
				return x;
			} catch (...) {
				return 0;
			}
			return 0;
		}

		template<typename T>
		operator T() const {

			char buff[sizeof(T)];
			const T *res = get<T>(buff);
			if (res == 0) {
				throw AnyTypeConversionImpossibleException(THISLOCATION,
					typeid(T),getTypeId());
			} else if (reinterpret_cast<const char *>(res) >= buff && 
				reinterpret_cast<const char *>(res) < buff + sizeof(T)) {
					T tmp = *res;
					res->~T();
					return tmp;
			} else {
				return *res;
			}
		}

		void clear() {
			if (dataBlock.isAllocated())
				typeDesc->destroy(dataBlock,getData());			
			typeDesc = NullAnyTypeDesc<Allocator>::getInstance();
		}

	protected:

		typedef typename Allocator::template BlockDecl<byte>::Block DataBlock;

		const IAnyTypeDesc<Allocator> *typeDesc;
		DataBlock dataBlock;

		void *getData() {
			if (!dataBlock.isAllocated()) return 0;
			return (void *)dataBlock.getItem(0);
		}
		const void *getData() const {
			if (!dataBlock.isAllocated()) return 0;
			return (const void *)dataBlock.getItem(0);
		}

	};


	typedef AnyT<StdAlloc> Any;
	template class AnyT<StdAlloc>;
#if 0


    ///Internal interface for Any class
    /**
     * Using this interface, Any class can identify referred variable type and
     * provide some actions without need to know all informations about the type
     */
    class IAnyType {
    protected:
        ///Implementation of getInterface()
        virtual const void *getInterfacePtr(const std::type_info &ifcDesc) const {
            return 0;
        }
        
    public:
        
        ///Retrieves additional interface
        /**
         * You have to specify type of interface as template argument. 
         * @return !=NULL return pointer to requested interface
         * @return NULL Interface is no available for this instance
         */
        template<class T>
        const T *getInterface() const {
            return reinterpret_cast<const T *>(getInterfacePtr(typeid(T)));
        }
        
        
        ///Retrieves size in bytes of type that object describes
        /**
         * @return size in bytes
         */
        virtual natural getTypeSize() const = 0;
        
        ///Retrieves reference to type_info descriptor of the type that object describes
        /**
         * @return reference to type_info descriptor
         */
        virtual const std::type_info &getTypeId() const = 0;
        
        ///Allows to reinterpret cast the given pointer and throw it
        /**
         * This function is used to provide legacy runtime conversion between 
         * derived and base class. Because standard C++ doesn't contain
         * tool to make this conversion directly, using this function, 
         * Any class is able to throw pointer of Derived type and catch is as
         * Base type.
         * 
         * @param instance pointer to the variable to throw
         */
        virtual void throwPtr(const void *instance) const = 0;
        
        ///Notifies descriptor, that given variable should be destroyed
        /**
         * Function is called in Any's destructor. It is used to notify,
         * descriptor, that reference will be release. If descriptor uses
         * variable allocated in heap memory, this is the best place
         * to release the memory. Standard Any class has this function empty,
         * but AnyCpy can reimplement this function
         * 
         * @param instance pointer to instance to release
         */
        virtual void releaseVar(const void *instance) const = 0;
        
        ///Retrieves pointer to singleton object implements this interface
        /**
         * For given interface, there can be only one derived class declared
         * as template. Parameter of template is the type, which the class
         * describes. Function will retrieve singleton instance for given
         * type. 
         * 
         * For extending the interface, extension should not be written
         * by extending this class. Just create new interface and let
         * implementation class make extension using the function
         * getInterfacePtr
         * 
         * @return pointer to singleton object
         */
        
        virtual IAnyType *getSingleton() const = 0;
    };
    
    ///Any class ... referes to any variable and remembers its type
    /**
     * Any class is const refernce to a variable of any type. Object of this
     * class holds pointer to the variable and pointer to the typeDescriptor
     * implemented by IAnyType interface. 
     * 
     * Remember, that Any is reference, so you have to keep original variable
     * valid, until reference is released. You can also make copy of 
     * refered variable, this feature is implemented by class AnyCpy
     * 
     * Usage of any class is as parameter of the function. Caller can
     * pass variable of any type. Instance of Any class will remember the
     * type and is able to transfer the type into called function. Called function
     * can try to fetch content of the instance, inspect the stored type or
     * provide multiple functions depends on variable type
     * 
     * Type Check and conversions are done in Run-Time. Conversions can be limited,
     * because run-time has less informations about types. Errors are reported
     * through exceptions
     * 
     * Instance of Any class can also contains 'nil' value. This value is
     * equal to "no value" or "empty". nil is enumeration symbol declared in
     * LightSpeed::NullType. To detect empty instance, make comparsion with 'nil'
     * 
     * Instance of Any can be compared with another instance of Any. Comparsion
     * is equal to comparsion of the pointers. Two instances are equal, if
     * referes the same variable. Comparsion of content is not available
     * in Any class... but can be provided by future extension
     */ 
    class Any {
    public:
        
        ///Constructs empty instance
        /** Constructs instance that contains 'nil' */
        Any():typeDesc(0),dataRef(0) {}
        
        ///Constructs copy of instance
        /**Constructor will create new reference. It will not copy the
         * content. 
         * @param other reference of other instance
         */
        Any(const Any &other)
            :typeDesc(other.typeDesc?other.typeDesc->getSingleton():0)
            ,dataRef(other.dataRef) {}
        
        ///Constructs instance using internal informations
        /**This allows to create instance, if there are standalone (void) pointer to
         * variable and standalone pointer to type descriptor. Constructor
         * is used internally, but need to be available for extensions
         * 
         * @param typeDesc pointer to type descriptor that describes type of variable
         * that refers second parameter
         * @param dataRef void pointer to variable which will be refered by this instance
         */ 
        Any(const IAnyType *typeDesc, const void *dataRef)
                :typeDesc(typeDesc),dataRef(dataRef) {}
        
        ///Constructs empty instance
        /** Constructs instance that contains 'nil' 
         * 
         * @param x place to specify 'nil'. This allows to implicit conversion 'nil' to empty Any instance
         * */
        Any(NullType x):typeDesc(0),dataRef(0) {}

        ///Constructs instance using the reference to any variable
        /** 
         * This is most used constructor. It converts variable to Any instance creating
         * reference on it and remebering its type. It is template constructor, so
         * it can use class information to retrieve and create type descriptor.
         * 
         * @param var variable used to create reference. Should be variable, not direct
         * value. Value or constructor can be used, if instance is created as
         * parameter of function. C++ garantees, that temporary variable will
         * valid, until function returns.
         * class
         * @note Detecting and describing type can be configured by specialization
         * of the template class AnyPacker<T>. For special behaviour, 
         * create new specialization. 
         * 
         * @see AnyPacker
         */
        template<class T> Any(const T &var);
        
        
        ///Destructor
        /**
         * Destroyes Any instance and optionally destroy variable owned
         * by this instance. In most of cases, variable is not owner by
         * the Any instance, but extension can request this feature
         */ 
        ~Any() {
            if (typeDesc) typeDesc->releaseVar(dataRef);
        }
        
        
        ///Makes assignment to another Any variable
        /**
         * Assignment always creates new reference to the variable. Overwrites
         * previous reference. Optionally destroyes the owned variable
         * 
         * @param other right part of assignment
         * @retun reference to this, as standard C++ recomends
         */
        Any &operator=(const Any &other) {
            if (typeDesc) typeDesc->releaseVar(dataRef);
            dataRef = other.dataRef;
            typeDesc = other.typeDesc->getSingleton();
            return *this;
        }
        
        ///Compares two Any instances
        /**
         * @param other right part of operator
         * @retval true instances are refer the same variable
         * @retval false instances are not refer the same variable
         */
        bool operator==(const Any &other) const {
            return other.dataRef == dataRef;
        }
        ///Compares two Any instances
        /**
         * @param other right part of operator
         * @retval true instances are not refer the same variable
         * @retval false instances are refer the same variable
         */
        bool operator!=(const Any &other) const {
            return other.dataRef != dataRef;
        }
        
        
        ///Sets the given pointer to the address of refered variable
        /**
         * Function can be used to retrieve pointer without need to
         * catch exception. Function will not throw exception in case, that
         * conversion is not possible
         * 
         * @param ref reference to pointer. Variable can be unintialized. After
         * successfully return, pointer will contain address to refered variable
         * If pointer is set to 0, instance contains 'nil'. This is reported as success.
         * @retval true success
         * @retval false conversion is impossible
         * 
         * @note you can use operator() to retrieve reference without need 
         * to declare pointer and check success
         */
        
                
        template<class T>
        bool getRef(const T * &ref) const;
        
        
        ///Converts Any instance to const reference.
        /**
         * Template operator will try to convert instance to a const
         * reference to the type detected by the compiler through operator().
         * 
         * Operator will always return some reference, or throw an exception
         * @return reference to the contained variable, if variable is the same type
         * or can be converted to the requested type
         * @exception AnyTypeEmptyException Any instance contains 'nil'
         * @exception AnyTypeConversionImpossibleException Conversion is not possible,
         */ 
        template<class T>
        operator const T &() const;
        
        ///Converts Any instance to value
        /**
         * Template operator will try to convert instance to the value
         *  of the type detected by the compiler through operator().
         * 
         * @note that T must define copy constructor.
         * 
         * @return Returns value made as copy of variable which Any type refers. Copy
         * is not depend on the reference, so instance of Any can be
         * later destroyed without affecting the copy. 
         * @exception AnyTypeEmptyException Any instance contains 'nil'
         * @exception AnyTypeConversionImpossibleException Conversion is not possible,
         */ 
        template<class T>
        operator T () const {
            const T &res = (*this);
            return res;
        }
        
        ///Retrieves size of variable in bytes        
        /**
         * @retval >0 size of variable
         * @retval 0 instance of Any is empty (nil)
         */
        natural getSize() const {
            if (typeDesc)
                return typeDesc->getTypeSize();
            else return 0;
        }
        
        ///Retrieves type_info descriptor of refered variable
        /**
         * return type_info of contained variablee. Function will work
         * with empty instance. In this case, return type_info of NullType
         */
        const std::type_info &getType() const {
            if (typeDesc)
                return typeDesc->getTypeId();
            else return typeid(nil);
        }
        
        
        ///Swaps content of two variables
        /**
         * Swapping is very useful, if you want to store variable 
         * into another Any instance without loosing variable's extension. 
         * You can use for example store the copied variable into another
         * variable. Default copying will create reference,
         * 
         * @param a first object
         * @param b second object
         * result is swapped variables
         */
        static void swap(Any &a, Any &b) {
            std::swap(a.typeDesc,b.typeDesc);
            std::swap(a.dataRef,b.dataRef);            
        }
        
        ///Swaps content of two variables 
        /* Swapping is very useful, if you want to store variable 
        * into another Any instance without loosing variable's extension. 
        * You can use for example store the copied variable into another
        * variable. Default copying will create reference,
        * 
        * @param other second object
        * result is swapped variables
        */
        void swap(Any &other) {
            swap(*this,other);
        }
        
    protected:
        
        const IAnyType *typeDesc;
        const void *dataRef;
        
        
    };

    
    ///Implementation of the IAnyType interface
    /**Template class allow to create implementation for each type used
     * with Any class. This instance describes the type and offers 
     * functions to work with the type
     */
    template<class T>
    class AnyRefType: public IAnyType {
    public:
        
        virtual natural getTypeSize() const {
            return sizeof(T);
        }
        virtual const std::type_info &getTypeId() const {
            return typeid(T);
        }
        virtual void throwPtr(const void *instance) const {
            const T *ptr = reinterpret_cast<const T *>(instance);
            throw (const_cast<T *>(ptr));
        }
        virtual void releaseVar(const void *instance) const {
            
        }
        virtual IAnyType *getSingleton() const {
            static AnyRefType x;
            return &x;
        }
        
        
    };
    
    ///Template class that implements packing variable into instance of Any type
    template<class T>
    class AnyPacker {
    public:
        ///Packs the variable into any type
        /**
         * @param other reference to variable to pack
         * @return instance of Any class
         */
        static Any pack(const T &other) {
            AnyRefType<T> typeDesc;
            return Any(typeDesc->getSingleton(),&other);                        
        }
    };
    
    template<class T> 
    Any::Any(const T &var) {
        Any tmp = AnyPacker<T>::pack(var);
        typeDesc = tmp.typeDesc;
        dataRef = tmp.dataRef;
    }
    
    
    template<class T>
    Any::operator const T &() const {
        
        if (*this == nil)
            throw AnyTypeEmptyException(THISLOCATION, typeid(T)); 
        
        const T *ptr = 0;
        if (!getRef(ptr))  {
            throw AnyTypeConversionImpossibleException(THISLOCATION,
                    typeid(T),typeDesc->getTypeId());
        }
        return *ptr;
            
    }

    template<class T>
    bool Any::getRef(const T * &ref) const {

        if (*this == nil) {
            ref = 0;
            return true;
        }
        if (typeid(T) == typeDesc->getTypeId()) {
            ref = reinterpret_cast<const T *>(dataRef);
            return true;
        }
        try {
            typeDesc->throwPtr(dataRef);
        } catch (T *catched) {
            ref = catched;
            return true;
        } catch (...) {
            ref = 0;
            return false;
        }
        //never should be here
        ref = 0; 
        return false;      
    }
    
    ///Interface that extends IAnyType to allow make copies of any instance
    /** Extension is made by derived class AnyTypeCopyRef. If caller wants
     * to access this interface, it must call IAnyType::getInterface and
     * specify IAnyTypeCopy as template parameter. Subject can return NULL,
     * so object is not copyable
     */
    class IAnyTypeCopy {
    public:
        ///Makes copy of specified instance
        /** @param ptr pointer to  object. Object must have type equal
         * to type of this descriptor. Copy is created through copy constructor
         * @retval pointer to copy. 
         */
        virtual void *copy(const void *ptr) const = 0;
        ///Retrieves pointer to interface for instances stored as reference
        virtual IAnyType *getSingletonRef() const = 0;
        ///Retrieves pointer to interface for instances stored as copy
        virtual IAnyType *getSingletonCpy() const = 0;
    };
    
    ///Any type with copy capabilities
    /** Implements Any type that can hold object of any type,
     * allows to object be copied and stored independ to
     * originalobject and without need to know the object's type
     * 
     * Caller must supply object with public copy constructor. If type
     * is used as parameter of a function, it also informs, that
     * called function will make a copy of the object. So if you
     * pass 'not-copyable' object, function will probably throw and exception
     */
    class AnyCpy: public Any {
    public:
        
        friend class AnyPacker<AnyCpy>;
       
        ///Creates empty instance
        AnyCpy() {}
        ///Creates copy of instance
        /**@param other original instance
         * @note There is no difference in copy constructors used on Any
         * and AnyCpy class. Both will create instance that keeps
         * reference to the original variable. If you need to create
         * copy of original variable, you have to do explicitly, by calling
         * function AnyCpy::copy()
         */
        AnyCpy(const AnyCpy &other) 
            :Any(other.typeDesc?other.typeDesc->getSingleton():0,other.dataRef) {}
        ///Create AnyCpy instance directly specifying type and data pointer
        /**This allows to create instance, if there are standalone (void) pointer to
         * variable and standalone pointer to type descriptor. Constructor
         * is used internally, but need to be available for extensions
         * 
         * @param typeDesc pointer to type descriptor that describes type of variable
         * that refers second parameter
         * @param dataRef void pointer to variable which will be refered by this instance
         */ 
        
        AnyCpy(const IAnyType *typeDesc, const void *dataRef):Any(typeDesc,dataRef) {}      

        AnyCpy(NullType x):Any(x) {}

        ///Constructs instance using the reference to any variable
        /** 
         * This is most used constructor. It converts variable to Any instance creating
         * reference on it and remebering its type. It is template constructor, so
         * it can use class information to retrieve and create type descriptor.
         * 
         * @param var variable used to create reference. Should be variable, not direct
         * value. Value or constructor can be used, if instance is created as
         * parameter of function. C++ garantees, that temporary variable will
         * valid, until function returns.
         * class
         * @note Detecting and describing type can be configured by specialization
         * of the template class AnyPacker<T>. For special behaviour, 
         * create new specialization.
         * 
         * @note parameter must specify object with public copy constructor 
         * 
         * @see AnyPacker
         */

        template<class T> AnyCpy(const T &var);
        
        ///Assigns instance new value
        /** @param other original instance
         * @note Even if the target object holds copy of an object, it will
         * return the reference to the object kept by other object. Previous
         * object is destroyed and released
         *
         */
        AnyCpy &operator=(const AnyCpy &other) {
            Any::operator=(other);
            const IAnyTypeCopy *ifc = typeDesc->getInterface<IAnyTypeCopy>();
            if (ifc != 0)
                typeDesc = ifc->getSingletonRef();
            return *this;
                
        }
        

        ///converts reference to copy
        /** Use this function to detach object from its original value. AnyCpy
         * holds reference until copy() called. After function return, you have
         * garnteed, that value is not depending on original variable. 
         * 
         * You will not see difference between copy and reference. Class Any
         * or AnyCpy works the same way. Any future copying contained value into
         * anothed instance of Any or AnyCpy will create reference to this
         * value. You have to keep this instance valid, because it acts
         * same way as original variable.
         * 
         * @retval true successfully copied
         * @retval false no action taken. Instance contains 'nil' or 
         * already owns the value (contains copy)
         * @exception AnyNotCopyableException Cannot create copy, because instance
         * contains reference to variable declared as not copyable
         * @exception AnyTypeUnableCopyException Cannot create copy, because
         * copying failed (returns 0)
         */
        bool copy() {
            if (dataRef && typeDesc) {
                const IAnyTypeCopy *ifc = typeDesc->getInterface<IAnyTypeCopy>();
                if (ifc == 0)
                    throw AnyNotCopyableException(THISLOCATION,typeDesc->getTypeId());               
                IAnyType *newifc = ifc->getSingletonCpy();
                if (newifc != typeDesc) {
                    const void *newdata =  ifc->copy(dataRef);
                    if (newdata == 0)
                        throw AnyTypeUnableCopyException(THISLOCATION,typeDesc->getTypeId());
                    dataRef = newdata;
                    typeDesc = newifc;
                    return true;
                }
            }
            return false;                       
        }
        
        
        
        ///Test, whether instance is valid AnyCpy
        /**
         * @retval true success
         * @retval false failed. Instance is not copyable, or is 'nil'
         */
        bool isValid() const {
            const IAnyTypeCopy *ifc = typeDesc->getInterface<IAnyTypeCopy>();
            return ifc != 0;            
        }
        
    };
    
    ///Base class for copy classes
    template<class T>
    class AnyTypeCopyBase: public IAnyTypeCopy {
    public:
        virtual void *copy(const void *ptr) const {
            const T *ref = reinterpret_cast<const T *>(ptr);
            return new T(*ref);
        }
        virtual IAnyType *getSingletonRef() const;
        virtual IAnyType *getSingletonCpy() const ;
    };
    
    ///Implementation of AnyCpy interface for reference
    template<class T>
    class AnyTypeCopyRef: public AnyRefType<T>, public AnyTypeCopyBase<T> {
    public:
        virtual const void *getInterfacePtr(const std::type_info &type) const {
            if (type == typeid(IAnyTypeCopy))
                return static_cast<const IAnyTypeCopy *>(this);
                else return AnyRefType<T>::getInterfacePtr(type);
        }
    };

    ///Implementation of AnyCpy interface for copy
    template<class T>
    class AnyTypeCopyCpy: public AnyTypeCopyRef<T> {
    public:
        virtual void releaseVar(const void *instance) const {
            T *ptr = reinterpret_cast<T *>(const_cast<void *>(instance));
            delete ptr;
        }
    };

    template<class T>
    IAnyType *AnyTypeCopyBase<T>::getSingletonRef() const {
        static AnyTypeCopyRef<T> x;
        return &x;
    }
    template<class T>
    IAnyType *AnyTypeCopyBase<T>::getSingletonCpy() const {
        static AnyTypeCopyCpy<T> x;
        return &x;
        
    }

    template<>
    class AnyPacker<AnyCpy> {
    public:
        Any pack(const AnyCpy &other) {
            return Any(other.typeDesc?
                    other.typeDesc->getSingleton():0,other.dataRef);
        }
    };

    template<class T>
    class AnyCpyPacker {
    public:
        static AnyCpy pack(const T &other) {
            AnyTypeCopyRef<T> typeDesc;
            return AnyCpy(typeDesc->getSingletonRef(),&other);                        
        }
    };

    template<> class AnyCpyPacker<Any> {
    public:
        static AnyCpy pack(const Any &other) {
            if (other == nil) return nil;
            const AnyCpy &tmpd = static_cast<const AnyCpy &>(other);
            if (tmpd.isValid())
                return tmpd;
            else
                throw AnyNotCopyableException(THISLOCATION,tmpd.getType()); 
        }
    };

    
    template<class T> 
    AnyCpy::AnyCpy(const T &var) {
        AnyCpy tmp = AnyCpyPacker<T>::pack(var);
        typeDesc = tmp.typeDesc;
        dataRef = tmp.dataRef;
    }


#endif
}

#endif /*ANY_H_*/
