#pragma once
#ifndef LIGHTSPEED_BASE_TIMESTAMP
#define LIGHTSPEED_BASE_TIMESTAMP
#include <time.h>
#include "compare.h"
#include "../base/containers/string.h"
#include "exceptions/genexcept.h"

namespace LightSpeed {


	///Class is useful to store timestamps
	/**
	 * Every instance carries two values. Time in day, and day number.
	 *
	 * Time of day has resolution in milliseconds and must be in the range 0...86400000. 
	 *
	 * Day number is relative count of days from a fixed defined date. This date is not
	 * defined by this class. For general usage is recommended, to define day 0 to
	 * the date 1.1.1970 (in compatibility with Unix-timestamp).  Windows 
	 * timestamp (aka FILETIME) is re-based to 1980 subtracting 138426 days.
	 *
	 * Because you can add and subtract two timestamps, you can use this class
	 * to measure interval between to timestamps. 
	 *
	 * @note Time should be always in UTC.
	 */
	 
	class TimeStamp: public Comparable<TimeStamp> {
	public:
		static const natural daySecs = 86400;
		static const natural timeResolution = 1000;
		static const natural dayMillis = daySecs * timeResolution;
		static const natural daysFrom1601 = 134774;

		///Creates empty timestamps
		TimeStamp():day(0),time(0) {}

		static TimeStamp now();

		///Creates timestamps 
		/**
		 * @param day relative count of days to base date
		 * @param time count of milliseconds in day 
		 */
		 
		TimeStamp(integer day, natural time):day(Bin::integer32(day + time/dayMillis)),time(Bin::natural32(time % dayMillis)) {}

		///Creates timestamp
		/**
		 * @param day relative count of days to base date
		 *
		 * Useful to define relative time in days while time is set zero
		 */
		explicit TimeStamp(integer day):day(Bin::integer32(day)),time(0) {}

		explicit TimeStamp(double day);

		///Converts unix time to timestamp
		/**
		 * @param unixTime (time_t)
		 * @return timestamp
		 */
	 
		static TimeStamp fromUnix(const time_t &unixTime) {
			return TimeStamp((integer)(unixTime/daySecs),unixTime%daySecs * timeResolution);
		}

		static TimeStamp fromSeconds(natural sec) {
			return TimeStamp((integer)(sec/daySecs),sec%daySecs * timeResolution);
		}

		///Converts unix time to timestamp
		/**
		 * @param unixTime (time_t)
		 * @param millis milliseconds (from getTimeOfDay)
		 * @return timestamp 
		 */
		static TimeStamp fromUnix(const time_t &unixTime, natural millis) {
			return TimeStamp((integer)(unixTime/daySecs),unixTime%daySecs * timeResolution + millis);
		}

		///Converts Windows FILETIME to TimeStamp
		/**
		 *  @param dwLowDateTime parameter dwLowDateTime from FILETIME
		 *  @param dwHighDateTime parameter dwHighDateTime from FILETIME
		 *  @return timestamp rebased to 1.1.1980
		 */
		 
		static TimeStamp fromWindows(unsigned long dwLowDateTime, 
			unsigned long dwHighDateTime) {
				Bin::natural64 time64 = (((Bin::natural64)dwHighDateTime << 32) + dwLowDateTime)/10000;
				return TimeStamp((integer)(time64 / dayMillis - daysFrom1601), time64 % dayMillis);
		}

		///Creates timestamp from Y-M-D h:m:s format
		/**
		 * @param Y Absolute year (example: 2012). If Y is set to 0, function uses remain arguments to create time-offset
		 * @param M Month. In time-offset mode, every month has 30 days
		 * @param D Day. In time-offset mode, count of days in current offset
		 * @param h hour
		 * @param m minute
		 * @param s second
		 * @return Timestamp of time-offset
		 *
		 * Creating time offset is slightly faster, because function don't need to calculate leap years. Day
		 * is calculate using formula M * 30 + D. In standard mode, POSIX calendar is used to create this object
		 */
		static TimeStamp fromYMDhms(natural Y, natural M, natural D, natural h, natural m, natural s);

		static TimeStamp fromDBDate(ConstStrA dbdate);

		///retrieves current day number
		natural getDay() const {return day;}
		///retrieves current time in a day
		natural getTime() const {return time;}

		double getFloat() const {return double(day)+double(time)/double(dayMillis);}

		///returns miliseconds
		natural getMS() const {return time % timeResolution;}

		///compares two timestamps - use standard relation operators
		CompareResult compare(const TimeStamp &other) const {
			if (day == other.day) {
				if (time == other.time) return cmpResultEqual;
				return time < other.time?cmpResultLess:cmpResultGreater;
			} else 
				return day < other.day?cmpResultLess:cmpResultGreater;
		}

		///adds two timestamps. 
		/**
		 * It has meaning, when one of timestamps are result from subtraction                                                                     
		 */
		 
		TimeStamp operator + (const TimeStamp &other) const {
			Bin::natural32 newtime = time + other.time;
			integer newdate = day + other.day + fixOverflow(newtime);
			return TimeStamp(newdate, newtime);
		}
		
		///subtstracts two timestamps
		/**
		 * useful to calculate interval
		 */
		 
		TimeStamp operator - (const TimeStamp &other) const {
			Bin::natural32 newtime = time - other.time;
			integer newdate = day - other.day - fixUnderflow(newtime);
			return TimeStamp(newdate, newtime);
		}

		///Formats time using strftime posix function
		/**
		 * @param format specifies strftime format
		 * @param hintBuffer allows to specify buffer used to format result. Default value is will cause
		 *        determining buffer size dynamically from the format.
		 * @return formatted string
		 */
		StringA formatTime(ConstStrA format, natural hintBuffer = 0) const;
		
		StringA asDBTime() const;

		static TimeStamp intervalMSec(natural msec) {
			return TimeStamp(0,msec);
		}
		static TimeStamp intervalSec(natural sec) {
			return TimeStamp(0,sec*timeResolution);
		}
		static TimeStamp intervalMin(natural min)  {
			return TimeStamp(0,min*timeResolution*60);
		}
		static TimeStamp intervalHour(natural hour){
			return TimeStamp(0,hour*timeResolution*60*60);
		}

		time_t asUnix() const {return ((time_t)day * daySecs) + (time / timeResolution);}


		natural getMilis() const {
			natural k = getDay();
			k*= dayMillis;
			return k + getTime();
		}

		natural getSecs() const {
			natural k = getDay();
			k*= (dayMillis)/1000;
			return k + (getTime()/1000);
		}

		natural getMins() const {
			natural k = getDay();
			k*= (dayMillis)/60000;
			return k + (getTime()/60000);
		}

		natural getHours() const {
			natural k = getDay();
			k*= (dayMillis)/3600000;
			return k + (getTime()/3600000);
		}


		bool isNull() const {return day == 0 && time == 0 ;}
	protected:

		static integer fixOverflow( Bin::natural32 &time )
		{
			natural adds = time / dayMillis;
			time = time % dayMillis;
			return adds;
		}

		static integer fixUnderflow( Bin::natural32 &time )
		{
			integer adds = 0;
			while (time > 0x80000000) {
				adds++;
				time+=dayMillis;
			}
			return adds;
		}

#if 0
		/* http://alcor.concordia.ca/~gpkatch/gdate-c.html */

		static natural gday(natural y,natural m,natural d) {
			natural mm = (m + 9) % 12;
			natural yy = y - m/10;
			natural dd = d - 1;
			return 365*yy + yy/4 - yy/100 + yy/400 + (mm*306 + 5)/10 + dd;
		}
		

		struct Date dtf(long d) { /* convert day number to y,m,d format */
			struct sdate pd;
			long y, ddd, mm, dd, mi;

			y = (10000*d + 14780)/3652425;
			ddd = d - (y*365 + y/4 - y/100 + y/400);
			if (ddd < 0) {
				y--;
				ddd = d - (y*365 + y/4 - y/100 + y/400);
			}
			mi = (52 + 100*ddd)/3060;
			pd.y = y + (mi + 2)/12;
			pd.m = (mi + 2)%12 + 1;
			pd.d = ddd - (mi*306 + 5)/10 + 1;
			return pd;
		}
#endif


		static const char *message_InvalidDateTimeFormat;

		typedef LightSpeed::GenException1<message_InvalidDateTimeFormat, ConstStrA> InvalidDateTimeFormat;

	protected:
		///day relative time_t base
		Bin::integer32 day;
		///time (in a day) in milliseconds, UTC
		Bin::natural32 time;






	};



}




#endif
