#include <typeinfo>
#include "exception.h"

#ifndef LIGHTSPEED_UNSUPORTEDFEATURE_H_
#define LIGHTSPEED_UNSUPORTEDFEATURE_H_

#include "../containers/string.h"

namespace LightSpeed
{
    
    ///Common exception thrown when unsupported feature is requested.    
    class UnsupportedFeature: public Exception {
    public:
        UnsupportedFeature(const ProgramLocation &loc)
            :Exception(loc) {}     
        virtual String getFeatureName() const = 0;
        virtual String getClassName() const = 0;
        virtual ~UnsupportedFeature() throw() {};

        static LIGHTSPEED_EXPORT const char *msgText;;

    };
    
    ///Template to help to create unsupported feature for each class 
    template<class T>
    class UnsupportedFeatureOnClass: public UnsupportedFeature {      
    public:
        LIGHTSPEED_EXCEPTIONFINAL;

        ///Deprecated
        UnsupportedFeatureOnClass(const ConstStrC &feature,
                                  const ProgramLocation &loc)
            : UnsupportedFeature(loc),feature(feature) {}
        ///Using ansi characters
        UnsupportedFeatureOnClass(const ProgramLocation &loc,
                                  const ConstStrC &feature
                                  )
            : UnsupportedFeature(loc),feature(feature) {}
        ///Recomended usage
        UnsupportedFeatureOnClass(const ProgramLocation &loc,
                                  const ConstStrW &feature
                                  )
            : UnsupportedFeature(loc),feature(feature) {}
        
        virtual String getFeatureName() const {return feature;}
        virtual String getClassName() const {
            return String(typeid(*this).name());
        }
        virtual ~UnsupportedFeatureOnClass() throw() {};
    public:
        String feature;
        
        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText) << getClassName() << getFeatureName();
        }
                
    };
    
    class NotImplementedExcption: public Exception {
    public:
    	LIGHTSPEED_EXCEPTIONFINAL;
    	NotImplementedExcption(const ProgramLocation &loc)
    		:Exception(loc) {}
    	NotImplementedExcption(const ProgramLocation &loc, const String &feature)
    		:Exception(loc),feature(feature) {}

    	const String &getFeature() const {return feature;}

    	  static LIGHTSPEED_EXPORT const char *msgText;;

    	virtual ~NotImplementedExcption() throw() {}

    protected:
    	String feature;

    	virtual void message(ExceptionMsg &msg) const {
    	        	msg(msgText) << feature;
    	}
    };
        


} // namespace LightSpeed


#endif /*UNSUPORTEDFEATURE_H_*/
