#include "exception.h"

#ifndef LIGHTSPEED_EXCEPTIONS_RANGEEXCEPTION_H_
#define LIGHTSPEED_EXCEPTIONS_RANGEEXCEPTION_H_


namespace LightSpeed {



	class IRangeException: public virtual Exception {
	public:
	
		IRangeException(const ProgramLocation &loc):Exception(loc) {}
		IRangeException():Exception(THISLOCATION) {}

		static LIGHTSPEED_EXPORT const char *msgText;;
        static LIGHTSPEED_EXPORT const char *msgTextNA;
        static LIGHTSPEED_EXPORT const char *msgTextFrom;
        static LIGHTSPEED_EXPORT const char *msgTextTo;
        static LIGHTSPEED_EXPORT const char *msgTextFromTo;

	};

	namespace RangeExceptMode {
	    enum RangeFromTo {rangeFromTo};
	    enum RangeFrom {rangeFrom};
	    enum RangeTo {rangeTo};
	    enum NoRange {noRange};
	}
	
	template<class T>
	class RangeException: public IRangeException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
	
        RangeException(const ProgramLocation &loc,
                RangeExceptMode::RangeFromTo mode,
                    T rangeFrom , T rangeTo, T val);
        RangeException(const ProgramLocation &loc,
                RangeExceptMode::RangeFrom mode,
                    T rangeFrom , T val);
        RangeException(const ProgramLocation &loc,
                RangeExceptMode::RangeTo mode,
                    T rangeTo , T val);

        RangeException(const ProgramLocation &loc,
                RangeExceptMode::NoRange mode, T val);

        RangeException(const ProgramLocation &loc);

	
		bool isRangeFromDefined() const {return bfrom;}
		bool isRangeToDefined() const {return bto;}
		const T &getRangeFrom() const {return rfrom;}
		const T &getRangeTo() const {return rto;}
	


	protected:
		T rfrom,rto,val;
		bool bfrom,bto;

        virtual void message(ExceptionMsg &msg) const {

        	msg(msgText);
        	const char *x;
        	if (bfrom && bto) x = msgTextFromTo;
        	else if (bfrom) x = msgTextFrom;
        	else if (bto) x = msgTextTo;
        	else x = msgTextNA;

        	msg(x) << rfrom << rto << val;
        }
	

	
		const char *getStaticDesc() const {
			return "Exception is thrown, when value used in operation has been "
				"out of allowed range. Exception's description should show the "
				"value and the allowed range.";
		}




	};

// ----------- IMPL ---------------------



template<class T> inline RangeException<T>::RangeException(
        const ProgramLocation & loc, RangeExceptMode::RangeFromTo,
        T rangeFrom, T rangeTo, T val)
        :Exception(loc), rfrom(rangeFrom),rto(rangeTo),val(val),
        bfrom(true),bto(true) {}



template<class T> inline RangeException<T>::RangeException(
        const ProgramLocation & loc, RangeExceptMode::RangeFrom,
        T rangeFrom, T val)
        :Exception(loc), rfrom(rangeFrom),val(val),
        bfrom(true),bto(false) {}



template<class T> inline RangeException<T>::RangeException(
        const ProgramLocation & loc, RangeExceptMode::RangeTo,
        T rangeTo, T val)
        :Exception(loc), rto(rangeTo),val(val),bfrom(false),bto(true) {}



template<class T> inline RangeException<T>::RangeException(
        const ProgramLocation & loc, RangeExceptMode::NoRange , T val)
        :Exception(loc), val(val),bfrom(false),bto(false) {}




template<class T> inline RangeException<T>::RangeException(
        const ProgramLocation & loc)
        :Exception(loc), bfrom(false),bto(false) {}

}
 // namespace LightSpeed


#endif /*RANGEEXCEPTION_H_*/
