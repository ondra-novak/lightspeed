/*
 * datetime.h
 *
 *  Created on: 16.8.2010
 *      Author: ondra
 */

#ifndef _LIGHTSPEED_DATETIME_H_
#define _LIGHTSPEED_DATETIME_H_

#include <time.h>

namespace LightSpeed {


	class Date {
	public:

		struct Parts {
			integer year;
			integer month;
			integer day;
			Parts() {}
			Parts(integer year, integer month, integer day)
				:year(year),month(month),day(day) {}
		};

		struct PartsEx: public Parts {
			integer dayOfWeek;
			integer dayOfYear;
			PartsEx() {}
			PartsEx(integer year, integer month, integer day,
					integer dayOfWeek, integer dayOfYear)
				:Parts(day,month,year),dayOfWeek(dayOfWeek),dayOfYear(dayOfYear) {}
			PartsEx(const Parts &b,integer dayOfWeek, integer dayOfYear)
				:Parts(b),dayOfWeek(dayOfWeek),dayOfYear(dayOfYear) {}
		};

		Date(): days(0) {}
		Date(integer y, integer m, integer d);
		explicit Date(const Parts &unp);
		explicit Date(integer days):days(days) {}

		static Date today();

		static Date fromUnixTime(time_t t) {
			return Date(unixTimeToDays(t));
		}

		time_t getUnixTime() const;

		integer getYear() const;

		natural getMonth() const;

		natural getDay() const;

		integer getDays() const {return days;}

		Parts getParts() const;

		PartsEx getPartsEx() const;

		operator Parts() const {return getParts();}
		operator PartsEx() const {return getPartsEx();}


		///Adds day interval to date
		/** Second argument is used as count of days from the beginning
		 *  ( getDays() ) and value is added to the current date,
		 *
		 *  To add months or years, use Parts structure as second argument
		 *
		 * @param other
		 * @return
		 */
		Date operator+(const Date &other) const;
		Date operator-(const Date &other) const;
		Date &operator+=(const Date &other);
		Date &operator-=(const Date &other);
		Date &operator++();
		Date &operator--();
		Date operator++(int);
		Date operator--(int);
		Date operator+(const Parts &other) const;
		Date operator-(const Parts &other) const;
		Date &operator+=(const Parts &other);
		Date &operator-=(const Parts &other);

		/// Calculates "dayNumber" it is count of days from 0/1/1
		/**
		 *
		 * @param year year in range 0 - xxxx
		 * @param month month in range 1 - 12. Outside values are adjusted
		 * @param day day in range 1 - 31. Outside values are adjusted
		 * @return
		 */
		static integer dayNumber(integer year, integer month, integer day );

		static PartsEx dateFromDayNumber(integer dayNr);

	protected:
		integer days;

		static integer unixTimeToDays(time_t t);
		static time_t daysToUnixTime(integer d);

	};


	///Time storage
	/** this class can store time value to maximal precision
	 * to miliseconds. Because it doesn't store the date
	 * it can be used to measure time up to 24 hours.
	 *
	 * It contains carry flag to connect with date object
	 */
	class Time {
	public:

		struct Parts {
			natural hour;
			natural minute;
			natural second;
			natural mili;
			Parts();
			Parts(natural hour, natural mnt, natural sec, natural mili = 0)
				:hour(hour)
				,minute(mnt)
				,second(sec)
				,mili(mili) {}
		};

		explicit Time(Bin::natural32 miliseconds):value(miliseconds) {}
		Time():value(0) {}
		Time(natural hour, natural mnt, natural sec, natural mili = 0)
			:value(wrap(Bin::natural32(((hour*60+mnt)*60+sec)*1000+mili))) {}
		Time(const Parts &parts)
			:value(wrap(Bin::natural32(((parts.hour*60+parts.minute)*60+parts.second)*1000+parts.mili))) {}

		Parts getParts() const {
			return Parts(getHour(),getMinute(),getSecond(),getMilisecond());
		}

		///Retrieves current hour
		natural getHour() const {
			return getHours() % 24;
		}
		///Retrieves current minute
		natural getMinute() const {
			return getMinutes() % 60;
		}
		///Retrieves current second
		natural getSecond() const {
			return getSeconds() % 60;
		}
		///Retrieves current milisecond
		natural getMilisecond() const {
			return getMiliseconds() % 1000;
		}

		///Retrieves total hours from the beginning
		natural getHours() const {
			return value/(1000L*60L*60L);
		}
		///Retrieves total minutes from the beginning
		natural getMinutes() const {
			return value/(1000L*60L);
		}
		///Retrieves total seconds from the beginning
		natural getSeconds() const {
			return value/(1000L);
		}
		///Retrieves total mili-seconds from the beginning
		/** @note if there is overflow, naturalNull is returned instead
		 */
		natural getMiliseconds() const {
			return wrap(value);
		}

		///Adds time interval
		Time operator+(const Time &t) const {
			Bin::natural32 x = wrap(value) + wrap(t.value);
			return Time(x);
		}

		///Subtract time interval
		Time operator-(const Time &t) const {
			Bin::natural32 x = wrap(value) - wrap(t.value);
			x += 2 * dayMilis * (x >> 31);
			return Time(x);
		}

		Time &operator+=(const Time &r) {
			*this = *this + r;
			return *this;
		}
		Time &operator-=(const Time &r) {
			*this = *this - r;
			return *this;
		}

		///retrieves carry status
		/**
		 * Carry flag is set every time when calculation overflows the
		 * storage space. If it set after addition, you have
		 * to increase date by one day. If it set after subtraction,
		 * you have to decrease date by one day
		 *
		 * @retval true value contains overflow.
		 * @return false no overflow
		 */
		bool carry() const {return value > dayMilis;}


		///Clears carry, returns previous carry state
		/**
		 * Carry flag is set every time when calculation overflows the
		 * storage space. If it set after addition, you have
		 * to increase date by one day. If it set after subtraction,
		 * you have to decrease date by one day
		 *
		 * Use this function to handle overflow. You are able to
		 * retrieve and reset overflow flag
		 *
		 *
		 * @retval true there was a carry, cleared
		 * @return false no carry was and not cleared.
		 */
		bool carryClear() {
			bool c = carry();
			value = wrap(value);
			return c;
		}

		time_t getUnixTime() const {return getSeconds();}

		static Time now();

		static Time fromUnixTime(time_t t) {
			return Time(t % (24*60*60) + 1000);
		}

	protected:
		Bin::natural32 value;

		static const Bin::natural32 dayMilis = 24*60*60*1000;

		static inline Bin::natural32 wrap(Bin::natural32 t) {
			return t % dayMilis;
		}


	};


	class DateTime {
	public:

		struct Parts: public Date::Parts, public Time::Parts {
			Parts() {}
			Parts(const Date::Parts &d, const Time::Parts &t)
				:Date::Parts(d),Time::Parts(t) {}
		};
		struct PartsEx: public Date::PartsEx, public Time::Parts {
			PartsEx() {}
			PartsEx(const Date::PartsEx &d, const Time::Parts &t)
				:Date::PartsEx(d),Time::Parts(t) {}
		};

		DateTime(const Date &d, const Time &t):d(d),t(t) {}
		DateTime() {}
		DateTime(natural year, natural month, natural day,
				natural hour, natural minute, natural second, natural ms = 0)
			:d(year,month,day), t(hour,minute,second,ms) {}

		DateTime(const Date::Parts &d, const Time::Parts &p)
			:d(d),t(p) {}

		DateTime(const Parts &p):d(p),t(p) {}

		static DateTime today();

		static DateTime now() {return today();}

		static DateTime fromUnixTime(time_t t) {
			return DateTime(Date::fromUnixTime(t), Time::fromUnixTime(t));
		}

		time_t getUnixTime() const {return d.getUnixTime() + t.getUnixTime();}

		natural getYear() const {return d.getYear();}

		natural getMonth() const {return d.getMonth();}

		natural getDay() const {return d.getDay();}

		natural getDays() const {return d.getDays();}

		Parts getParts() const {return Parts(d.getParts(),t.getParts());}

		PartsEx getDateEx() const {return PartsEx(d.getPartsEx(),t.getParts());}

		natural getHour() const {return t.getHour();}

		natural getMinute() const {return t.getMinute();}

		natural getSecond() const {return t.getSecond();}

		natural getMilisecond() const {return t.getMilisecond();}

		const Date &getDate() const {return d;}

		const Time &getTime() const {return t;}

		natural getHours() const {return t.getHours() + 24*d.getDays();}
		natural getMinutes() const {return t.getMinutes() + 24*60*d.getDays();}
		natural getSeconds() const {return t.getSeconds() + 24*60*60*d.getDays();}
		natural getMiliseconds() const {return t.getSeconds() + 24*60*60*1000*d.getDays();}

		DateTime operator+(const DateTime &dt) const {
			Date dx = d + dt.d;
			Time tx = t + dt.t;
			if (tx.carryClear()) ++dx;
			return DateTime(dx,tx);
		}

		DateTime operator-(const DateTime &dt) const {
			Date dx = d - dt.d;
			Time tx = t - dt.t;
			if (tx.carryClear()) --dx;
			return DateTime(dx,tx);
		}

		DateTime &operator += (const DateTime &dt)  {
			d += dt.d;
			t += dt.t;
			if (t.carryClear()) ++d;
			return *this;
		}

		DateTime &operator -= (const DateTime &dt) {
			d -= dt.d;
			t -= dt.t;
			if (t.carryClear()) --d;
			return *this;
		}

	protected:

		Date d;
		Time t;
	};

	class Timezone {
	public:

		enum Zone {
			//Europe
			GMT, BST, IST, WET, WEST, CET, CEST, EET, EEST, MSK, MSD,
			//North america
			NST, NDT, AST, ADT, EST, EDT, CST, CDT, MST, MDT, PST,PDT,AKST,AKDT,HAST,HADT,
			//UTC
			UTC,
			//unknown
			Unknown
		};

		struct ZoneInfo {
			Zone zone;
			const char *name;
			short offsetMin;
			bool dst;

			ZoneInfo() {}
			ZoneInfo(Zone zone):zone(zone),name(0),offsetMin(0),dst(false) {}
			ZoneInfo(const char *name):zone(Unknown),name(name),offsetMin(0),dst(false) {}
			ZoneInfo(Zone zone, const char *name, short offsetMin, bool dst):zone(zone),name(name),offsetMin(offsetMin),dst(dst) {}
		};

		///Retrieves information about zone
		/**
		 * @param search contains partially filed structure, that specifies zone to find. To find zone by name,
		 * 		you have to specify name member variable, and set zone to the "unknown". As result, function will
		 * 		overwrite the stucture with valid values
		 * @retval true success.
		 * @retval false failed, zone not found
		 */
		static bool getZoneInfo(ZoneInfo &search);

	    bool getDst() const
	    {
	        return dst;
	    }

	    short getOffsetMin() const
	    {
	        return offsetMin;
	    }

	    Zone getZone() const
	    {
	        return zone;
	    }

	    Timezone(Zone zn) {init(zn);}
		Timezone(short offsetMin, bool dst) {init(ZoneInfo(Unknown,NULL,offsetMin,dst));}
		Timezone(const char *zoneName) {init(zoneName);}
		Timezone(short offsetMin, bool dst, Zone zn):zone(zn),offsetMin(offsetMin),dst(dst) {}

	protected:

		/// current time zone
		Zone zone;
		/// current offset to UTC
		short offsetMin;
		/// dst in effect
		bool dst;

		void init(const ZoneInfo &zinfo) {
			ZoneInfo zf = zinfo;
			getZoneInfo(zf);
			zone = zf.zone;
			offsetMin = zf.offsetMin;
			dst = zf.dst;
		}


	};
}


#endif /* DATETIME_H_ */
