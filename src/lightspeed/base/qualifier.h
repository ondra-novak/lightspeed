
#pragma once

namespace LightSpeed {
    ///qualifier manipulator
    /**
    * Use following class to manipulate with const in template 
    *
    * @b ConstObject<Type>::Remove removes const from the Type
    * @b ConstObject<Type>::Add adds const to the Type
    * @b ConstObject<Type>::Invert inverts const on the Type
    */

    template<class T>
    struct ConstObject
    {
        typedef T Remove;
        typedef const T Invert;
        typedef const T Add;
    };

    template<class T>
    struct ConstObject<const T>
    {
        typedef T Remove;
        typedef T Invert;
        typedef const T Add;
    };

    ///qualifier manipulator
    /**
    * Use following class to manipulate with pointer in  the template
    *
    * @b PointerObject<Type>::Remove removes star from the Type
    * @b PointerObject<Type>::Add adds star to the Type
    * @b PointerObject<Type>::Invert converts pointer to type and vice versa
    */

    template<class T>
    struct PointerObject
    {
        typedef T Remove;
        typedef T *Invert;
        typedef T *Add;
    };

    template<class T>
    struct PointerObject<T *>
    {
        typedef T Remove;
        typedef T Invert;
        typedef T *Add;
    };
    
    ///Removes reference from the type
    /**
     *  The _ field will contain type with removed const and reference
     * 
     *  if T = const X &, result is X
     *  if T = X &, result is X
     *  if T = X, result is X
     * 
     */
   
    template<class Type>
    struct OriginT {
    	typedef Type T;

		static const T &getRef(const T &x) {return x;}
    };
    
    template<class Type>
    struct OriginT<Type &> {
    	typedef Type T;

		static const T &getRef(T &x) {return x;}
	};
    
    template<class Type>
    struct OriginT<const Type &> {
		typedef typename OriginT<Type>::T T;

		static const T &getRef(const T &x) {return x;}
	};
    
	///Deprecated 
	template<class Type>
	struct DeRefType: OriginT<Type> {};
	
    
    
    ///Adds reference if it is necesery to increase performace
    /**
     * The T field will contain type best for parameter of return value
     * 
     * if T = X, result is const X &
     * if T = X &, result is X &
     * if T = const X &, result is const X &
     */ 
     template<class Type>
    struct FastParam {
        typedef const Type &T;        
    };

    template<>
    struct FastParam<void> {
        typedef void T;
    };

    template<class Type>
    struct FastParam<Type &> {
        typedef Type & T;
    };

    template<class Type>
    struct FastParam<const Type &> {
        typedef const Type &T;        
    };
    
    template<typename Type>
    struct AddReference{
    	typedef Type &T;
    };

    template<typename Type>
    struct AddReference<Type &>{
    	typedef Type &T;
    };

    template<typename Type>
    struct AddReference<const Type &>{
    	typedef const Type &T;
    };


 }
