#ifndef LIGHTSPEED_EMPTYCLASS_H_
#define LIGHTSPEED_EMPTYCLASS_H_

namespace LightSpeed
{

    ///Declaration of the empty  class
    /** Empty class has various usage. It can act as user context, when
     * you don't require context, or it can be used as base class if you
     * want to create template class with a base class as parameter,
     * but without specific base class defined.
     * 
     * Empty class is empty, has no methods, and no fields.
     */
    class Empty {
  
	public:
		template<typename Arch>
		void serialize(Arch &) {

		}

    };


    ///Tests, whether class is empty
    /**@param T tested class
     * @return field value contains true, if T doesn't contain any member.
     *
     * You can use this condition to determine, whether it is need to
     * allocate instance or not. Default test to sizeof() will never
     * return zero, but test with probe class will return size
     * of probe class if the base class is empty. Base class is tested class
     */
    template<typename T>
    struct IsEmpty {


        ///Probe class, whill use T as base class
        template<typename X>
        class Probe: public X {
            int dummy;
        };

        ///result of test
        static const bool value = sizeof(Probe<T>) == sizeof(Probe<Empty>);

    };

    ///For easier identification when used in templates when Void is used instead void as value
    typedef Empty Void;


} // namespace LightSpeed


#endif /*EMPTYCLASS_H_*/
