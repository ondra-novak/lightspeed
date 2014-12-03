#ifndef _BREDYLIBS_INVOKABLE_H_
#define _BREDYLIBS_INVOKABLE_H_



 namespace LightSpeed
 {
    ///Invoker class is class which contains function invoke
    /**
     * Function Invoke can help to access member variables in
     * derived class, when the base class is template, that gets
     * name of derived class through template parameter
     *
     *
     */

    template<class  Derived>
    class Invokable
    {
    public:

        ///Returns pointer to derived class
        Derived &_invoke() {return static_cast<Derived &>(*this);}
        ///Returns pointer to derived class 
        const Derived &_invoke() const {return static_cast<const Derived &>(*this);}
        
        typedef Derived InvokeClassType;
        
        
/*        ///Makes copy of the object
        Derived *clone() const {
            return new Derived(_invoke());
        }
        
        ///Makes copy of the object at given address
        Derived *clone(void *ptr) const {
            return new(ptr) Derived(_invoke());
        }
  */
        operator Derived &() {return _invoke();}
        operator const Derived &() const {return _invoke();}


    };
    


 };


#endif /*INVOKABLE_H_*/
