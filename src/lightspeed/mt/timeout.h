#ifndef LIGHTSPEED_MT_TIMEOUT
#define LIGHTSPEED_MT_TIMEOUT
#include "platform.h"


#if defined(LIGHTSPEED_PLATFORM_WINDOWS)
#include "windows/systime.h"
#elif defined(LIGHTSPEED_PLATFORM_LINUX)
#include "linux/systime.h"
#endif


namespace LightSpeed
{
    
        
    class SysTime;

    
    ///Defines timeout type
    /** Timeout type is type that specifies timeout. Variable of
     * this type can define timeout that expires specifies miliseconds
     * from now, or can expire at specified time. You can later test
     * whether timeout already expires, or how many time units remains
     * to expiration
     */
    class Timeout: public ComparableLessAndEqual<Timeout> {
        
    public:
        ///constructs infinite timeout
        /** Using default constructor (without parameters, you construct
         * inifnite timeout */
        Timeout():expireTime(nil) {}

        ///constructs infinite timeout
        Timeout(NullType x):expireTime(x) {}

        ///Constructs timeout specified by time in the future
        /** You can specify timeout in absolute time.
         * @param tm object that contains absolute time
         */
        explicit Timeout(const SysTime &tm):expireTime(tm) {}
        
        
        ///Constructs timeout specified by count of miliseconds from now
        /**
         * @param ms count of miliseconds in the future. If ms is equal to  naturalNull, timeout
         */
        Timeout(natural ms): expireTime(ms == naturalNull?SysTime(nil):SysTime::now() + SysTime(0,0,0,0,ms)) {}

        ///Constructs timeout specified by count of miliseconds from specified time
        /**
         * @param tm reference time
         * @param ms count of miliseconds in the future. If ms is equal to  naturalNull, timeout
         */
        Timeout(const SysTime &tm, natural ms): expireTime(ms == naturalNull?SysTime(nil): tm + SysTime(0,0,0,0,ms)) {}

        ///Constructs timeout specified by time from now
        /**
         * @param h hours
         * @param m minutes
         * @param s seconds
         * @param ms miliseconds
         */
        Timeout(natural h, natural m, natural s, natural ms)
        : expireTime(SysTime::now() + SysTime(0,h,m,s,ms)) {}

        ///Retrieves absolute time, when this timeout expires
        /**
         * @return expiration time
         */
        const SysTime &getExpireTime() const {return expireTime;}
        
        
        ///Returns, whether timeour expires to given time
        /**
         * @param relTo specifies a time to test expiration
         * @retval true timeour expires at given time
         * @retval false timeout doesn't expire at given time
         * @note in case of infinite timeout, function returns always false
         */
        bool expired(const SysTime &relTo) const {     
            if (relTo == nil || expireTime == nil) return false;
            return relTo > expireTime;
        }

        ///Tests, whether timeout has expired at current time
        /**
         * @retval true timeout has already expired
         * @retval false timeout did not expired yet
         */         
        bool expired() const {
            return expired(SysTime::now());
        }
        
        ///returns remaining time to expiration at given time
        /**
         * @param relTo specifies a time to calculate remain time
         * @return returns time difference between speicifed time and
         * time of expiration, If timeout already expired, it returns
         * zero time. If timeout is inifinite, it returns nil time (
         * all functions of nil time should return naturalNull
         */
        SysTime getRemain(const SysTime &relTo) const {            
            if (relTo == nil || expireTime == nil) return SysTime(nil);
            if (expired(relTo)) return SysTime();
            else return getExpireTime() - relTo;
        }

        ///returns remaining time to expiration at current time
        /**
         * @return returns time difference between current time and
         * time of expiration, If timeout already expired, it returns
         * zero time. If timeout is inifinite, it returns nil time (
         * all functions of nil time should return naturalNull
         */
         
        SysTime getRemain() const {
            return getRemain(SysTime::now());
            
        }
        
         ///Returns true, if timeout is infinite
         /**
          * @retval true timeout is infinite
          * @retval false timeout is not infinite
          */
        bool isInfinite() const {
            return expireTime == nil;
        }

        static Timeout infinite()  {
            return Timeout(nil);
        }
        
    protected:
        friend class ComparableLessAndEqual<Timeout>;
        friend class ComparableLess<Timeout>;

        bool lessThan(const Timeout &other) const {
        	if (expireTime == nil) return false;
        	if (other.expireTime == nil) return true;
        	return expireTime < other.expireTime;
        }

        bool equalTo(const Timeout &other) const {
        	if (expireTime == nil && other.expireTime == nil) return true;
        	return expireTime == other.expireTime;
        }

        SysTime expireTime;
        
    };



} // namespace LightSpeed

#endif /*TIMEOUT_H_*/
