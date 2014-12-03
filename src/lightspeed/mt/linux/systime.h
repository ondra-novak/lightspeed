#ifndef LIGHTSPEED_INTERNAL_LINUX_SYSTIME_H_
#define LIGHTSPEED_INTERNAL_LINUX_SYSTIME_H_

#include <sys/time.h>
#include <time.h>
#include "../../base/types.h"
#include "../../base/compare.h"

namespace LightSpeed
{

    
        ///Systime is class designed to measure time difference
        /** Instance of this class contains timestamp from current system. 
         * You can calculate difference of two timestamps. Timestamp itself
         * is not useful, because it can be calculated from the system timer,
         * which is not stored when computer is turned off
         * 
         * To use this class, make difference of two snapshots. You will retrieve
         * instance, that contains time relative to zero time. You 
         * can use function secs(), mssecs(), mins(), hours() etc to retrieve
         * calculate difference
         */          
        class SysTime: public ComparableLess<SysTime> {
            friend class ComparableLess<SysTime>;
        public:
            
            ///creates zero initialized system time
            /** This object doesn't specify time, but defines zero time span */
            SysTime() {
                curTime.tv_sec = 0;
                curTime.tv_usec = 0;
            }
            
            ///creates unitialized time
            /** This object specifies neither time nor zero time span.
             * Using this object will cause, that all calculations will
             * return unitialized object. Requesting particular values
             * may lead to undefined results.
             * 
             * To detect unitialized object, use object==nil condition.
             * @param x specify 'nil' to create the unitialized object
             */
            SysTime(NullType) {
                curTime.tv_sec = -1;
            }
            
            ///creates instance using the timeval
            /** @note this constructor is available only in linux envirnment
             */
            SysTime(struct timeval tm):curTime(tm) {}
            ///creates instance specifying seconds and microseconds
            /** @note this constructor is available only in linux envirnment
             */
            SysTime(time_t sec, suseconds_t us) {
                curTime.tv_sec = sec;
                curTime.tv_usec = us;
            }

            SysTime(natural days, natural hours, natural minutes, 
                    natural seconds, natural miliseconds) {                
                curTime.tv_sec = (((days * 24 + hours) * 60 + minutes)
                         * 60 + seconds) + miliseconds/1000;
                curTime.tv_usec = (miliseconds % 1000) * 1000;                
            }   
            
            ///Returns current system time
            /** Makes snapshot and returns it as SysTime
             */
            static SysTime now() {
                struct timeval tm;
                gettimeofday(&tm,0);
                return tm;
            }
            
                        
            SysTime operator+(const SysTime &other) const {
                if ((*this) == nil || other == nil) return SysTime(nil);
                return SysTime(curTime.tv_sec + other.curTime.tv_sec 
                            + normalizeDiv(curTime.tv_usec + other.curTime.tv_usec,1000000),
                            normalizeMod(curTime.tv_usec + other.curTime.tv_usec,1000000));
            }

            SysTime operator-(const SysTime &other) const {
                if ((*this) == nil || other == nil) return SysTime(nil);
                return SysTime(curTime.tv_sec - other.curTime.tv_sec 
                            + normalizeDiv(curTime.tv_usec - other.curTime.tv_usec,1000000),
                            normalizeMod(curTime.tv_usec - other.curTime.tv_usec,1000000));
            }

            
            ///retrieves total time in miliseconds
            /** @return total time in miliseconds
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural msecs() const {
                if ((*this) == nil) return naturalNull;
                return curTime.tv_sec * 1000 + curTime.tv_usec / 1000;
            }

            ///retrieves total time in seconds
            /** @return total time in seconds
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural secs() const {
                if ((*this) == nil) return naturalNull;
                return curTime.tv_sec ;
            }

            ///retrieves total time in minutes
            /** @return total time in minutes
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural mins() const {
                if ((*this) == nil) return naturalNull;
                return curTime.tv_sec/60 ;
            }

            ///retrieves total time in hours
            /** @return total time in hours
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural hours() const {
                if ((*this) == nil) return naturalNull;
                return curTime.tv_sec/3600 ;
            }
            ///retrieves total time in days
            /** @return total time in days
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural days() const {
                if ((*this) == nil) return naturalNull;
                return curTime.tv_sec/(3600*24) ;
            }
            
            ///retrieves current milisecons
            /**
             * @return current milisecond 0 - 999
             */
            natural msec() const {
                if ((*this) == nil) return naturalNull;
                return curTime.tv_usec / 1000;
            }

            ///retrieves current second
            /**
             * @return current second 0 - 59
             */
            natural sec() const {
                if ((*this) == nil) return naturalNull;
                return curTime.tv_sec % 60;
            }
            
            ///retrieves current minute
            /**
             * @return current minute 0 - 59
             */
            natural min() const {
                if ((*this) == nil) return naturalNull;
                return (curTime.tv_sec /60)%60;
            }
            
            ///retrieves current hour
            /**
             * @return current hour 0 - 23
             */
            natural hour() const {
                if ((*this) == nil) return naturalNull;
                return (curTime.tv_sec /3600)%24;
            }

            
            const struct timeval &getTimeVal() const {
                return curTime;
            }

            const struct timespec getTimeSpec() const {
            	struct timespec res;
            	res.tv_sec = curTime.tv_sec;
            	res.tv_nsec = curTime.tv_usec * 1000;
            	return res;
            }

            template<typename Arch>
            void serialize(Arch &arch) {
            	arch(curTime.tv_sec);
            	arch(curTime.tv_usec);
            }

        protected:
            
            struct timeval curTime; 
            
            static integer normalizeDiv(integer val, integer norm) {
                if (val < 0) return (val - norm + 1)/norm;
                else return val/norm;
            }
            
            static integer normalizeMod(integer val, integer norm) {
                if (val < 0) return norm + (val%norm);
                else return val%norm;
            }
            
            bool isNil() const {
                return curTime.tv_sec == -1;
            }
            
            bool lessThan(const SysTime &other) const {
                if (isNil() || other.isNil()) return false;
                return curTime.tv_sec < other.curTime.tv_sec
                || (curTime.tv_sec == other.curTime.tv_sec
                        && curTime.tv_usec < other.curTime.tv_usec);
            }
            
            bool equalTo(const SysTime &other) const {
                if (isNil() || other.isNil()) return false;
                return curTime.tv_sec == other.curTime.tv_sec
                && curTime.tv_usec == other.curTime.tv_usec;
            }

        };
    



} // namespace LightSpeed


#endif /*SYSTIME_H_*/
