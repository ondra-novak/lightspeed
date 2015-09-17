#pragma once
#include "../../base/windows/winpch.h"
#include "../../base/types.h"
#include "../../base/compare.h"

#ifndef _WINSOCK2API_
struct timeval {
	long    tv_sec;         /* seconds */
	long    tv_usec;        /* and microseconds */
};
#endif

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
                curTime = 0;
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
            SysTime(NullType x) {
                curTime = (DWORD)(-1);
            }
            
            ///creates instance using the timeval
            /** @note this constructor is available only in linux envirnment
             */
            SysTime(DWORD ticks):curTime(ticks) {}

            SysTime(natural days, natural hours, natural minutes, 
                    natural seconds, natural miliseconds) {                
                curTime = DWORD(((((days * 24 + hours) * 60 + minutes)
                         * 60 + seconds) * 1000)+ miliseconds);
            }   
            
            ///Returns current system time
            /** Makes snapshot and returns it as SysTime
             */
            static SysTime now() {
                return SysTime(GetTickCount());
            }
            
                        
            SysTime operator+(const SysTime &other) const {
                if ((*this) == nil || other == nil) return SysTime(nil);
                return SysTime(curTime + other.curTime);
            }
            
            SysTime operator-(const SysTime &other) const {
                if ((*this) == nil || other == nil) return SysTime(nil);
                return SysTime(curTime - other.curTime);
            }

            
            ///retrieves total time in miliseconds
            /** @return total time in miliseconds
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural msecs() const {
                if ((*this) == nil) return naturalNull;
                return curTime;
            }



            ///retrieves total time in seconds
            /** @return total time in seconds
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural secs() const {
                if ((*this) == nil) return naturalNull;
                return curTime/1000;
            }

            ///retrieves total time in minutes
            /** @return total time in minutes
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural mins() const {
                if ((*this) == nil) return naturalNull;
                return curTime/(60*1000);
            }

            ///retrieves total time in hours
            /** @return total time in hours
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural hours() const {
                if ((*this) == nil) return naturalNull;
                return curTime/(3600*1000);
            }
            ///retrieves total time in days
            /** @return total time in days
             * @note Function is designed to be used in time difference. 
             * You can get overflow, if you will use to global time
             */
            natural days() const {
                if ((*this) == nil) return naturalNull;
                return curTime/(3600*1000*24);
            }
            
            ///retrieves current milisecons
            /**
             * @return current milisecond 0 - 999
             */
            natural msec() const {
                if ((*this) == nil) return naturalNull;
                return curTime % 1000;
            }

            ///retrieves current second
            /**
             * @return current second 0 - 59
             */
            natural sec() const {
                if ((*this) == nil) return naturalNull;
                return  secs() % 60;
            }
            
            ///retrieves current minute
            /**
             * @return current minute 0 - 59
             */
            natural min() const {
                if ((*this) == nil) return naturalNull;
                return  mins() % 60;
            }
            
            ///retrieves current hour
            /**
             * @return current hour 0 - 23
             */
            natural hour() const {
                if ((*this) == nil) return naturalNull;
                return  hours() % 24;
            }
            

			struct timeval getTimeVal() const {
				struct timeval r;
				r.tv_sec = curTime / 1000;
				r.tv_usec = (curTime % 1000) * 1000;
				return r;
			}

        protected:
            
            DWORD curTime;
            
            
            bool isNil() const {
                return curTime == (DWORD)-1;
            }
            
            bool lessThan(const SysTime &other) const {
                if (isNil() || other.isNil()) return false;
                return curTime < other.curTime;
            }
            
            bool equalTo(const SysTime &other) const {
                if (isNil() || other.isNil()) return false;
                return curTime == other.curTime;
            }

        };
    



} // namespace LightSpeed



