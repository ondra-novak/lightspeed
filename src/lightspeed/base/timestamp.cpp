#include "timestamp.h"
#include "text/textParser.tcc"
#include "exceptions/systemException.h"
#include <math.h>

namespace LightSpeed {



	TimeStamp TimeStamp::fromDBTime(ConstStrA dbdate) {
		using namespace LightSpeed;
		TextParser<char, StaticAlloc<256> > parser;
		if (parser(" %u1-%u2-%u3 %u4:%u5:%u6 ",dbdate)) {
			natural yr = parser[1],
				mh = parser[2],
				dy = parser[3],
				hh = parser[4],
				mn = parser[5],
				sc = parser[6];
			return TimeStamp::fromYMDhms(yr,mh,dy,hh,mn,sc);
		} else if (parser(" %u1-%u2-%u3 " ,dbdate)) {
			natural yr = parser[1],
				mh = parser[2],
				dy = parser[3];
			return TimeStamp::fromYMDhms(yr,mh,dy,0,0,0);
		} else if (parser(" %u1:%u2:%u3 ",dbdate)) {
			natural hh = parser[1],
				mn = parser[2],
				sc = parser[3];
			return TimeStamp::fromYMDhms(0,0,0,hh,mn,sc);
		} else {
			throw InvalidDateTimeFormat(THISLOCATION, dbdate);
		}

	}

	static natural fixShortYear(natural year) {
		if (year <= 50) year+=2000;else year+=1900;
		return year;
	}

	static TimeStamp parse8601date(ConstStrA datePart) {		
		TextParser<char, StaticAlloc<50> > parse;
		if (datePart.length() == 4) {
			if (!parse("%u1",datePart)) throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, datePart);
			return TimeStamp::fromYMDhms(parse[1],1,1,0,0,0);
		} else if (datePart.length() == 6) {
			if (!parse("%(2,2)u1%%(2,2)u2%%(2,2)u3",datePart)) throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, datePart);
			return TimeStamp::fromYMDhms(fixShortYear(parse[1]),parse[2],parse[3],0,0,0);	
		} else if (datePart.length() == 7) {
			//YYYYWww
			if (parse("%u1W%u2",datePart))  {
				//TODO better implementation of week
				return TimeStamp::fromYMDhms(parse[1],1,(natural)parse[2]*7-6,0,0,0);						
			//YYYYDDD
			} else if (parse("%(4,4)u1%%u2",datePart))  {
				return TimeStamp::fromYMDhms(parse[1],1,parse[2],0,0,0);						
			//YYYY-MM
			} else if (parse("%u1-%u2",datePart))  {
				return TimeStamp::fromYMDhms(parse[1],parse[2],1,0,0,0);						
			} else {
				throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, datePart);
			}			
		} else if (datePart.length() == 8) {
			//YY-MM-DD
			if (parse("%u1-%u2-%u3",datePart))  {				
				return TimeStamp::fromYMDhms(fixShortYear(parse[1]),parse[2],parse[3],0,0,0);	
			//YYYY-Www
			} else if (parse("%u1-W%u2",datePart))  {				
				return TimeStamp::fromYMDhms(parse[1],1,(natural)parse[2]*7-6,0,0,0);	
			//YYYY-DDD
			} else if (parse("%u1-%u2",datePart))  {				
				return TimeStamp::fromYMDhms(parse[1],1,parse[2],0,0,0);	
			//YYYYMMDD
			} else if (parse("%(4,4)u1%%(2,2)u2%%(2,2)u3",datePart))  {				
				return TimeStamp::fromYMDhms(parse[1],parse[2],parse[3],0,0,0);	
			} else {
				throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, datePart);
			}
		} else if (datePart.length() == 10) {
			//YYYY-MM-DD
			if (parse("%u1-%u2-%u3",datePart))  {				
				return TimeStamp::fromYMDhms(parse[1],parse[2],parse[3],0,0,0);	
			} else {
				throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, datePart);
			}			
		} else {
			throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, datePart);
		}
	}



	static TimeStamp parse8601time(ConstStrA timePart) {		
		TextParser<char, StaticAlloc<50> > parse;
	
		natural zonepos = timePart.findOneOf(ConstStrA("Z+-"));
		ConstStrA zonespec;
		if (zonepos != naturalNull) { 
			zonespec  = timePart.offset(zonepos);
			timePart = timePart.head(zonepos);
		}

		natural dot = timePart.find('.');
		natural millis = 0;
		if (dot != naturalNull) {
			ConstStrA fract = timePart.offset(dot);
			timePart = timePart.head(dot);
			if (parse("%f1",fract)) {
				float f = parse[1];
				millis = (natural)floor(f*TimeStamp::timeResolution);				
			}
		}
		TimeStamp res;
		if (timePart.length() == 2) {
			if (parse("%u1",timePart))
				res = TimeStamp::fromYMDhms(0,0,0,parse[1],0,0);
			else
				throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, timePart);
		} else if (timePart.length() == 4) {
			if (parse("%u(2,2)u1%%(2,2)u2",timePart))
				res = TimeStamp::fromYMDhms(0,0,0,parse[1],parse[2],0);
			else
				throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, timePart);

		} else if (timePart.length() == 5) {
			if (parse("%u1:%u2",timePart))
				res = TimeStamp::fromYMDhms(0,0,0,parse[1],parse[2],0);
			else
				throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, timePart);
		} else if (timePart.length() == 6) {
			if (parse("%u(2,2)u1%%(2,2)u2%%(2,2)u3",timePart))
				res = TimeStamp::fromYMDhms(0,0,0,parse[1],parse[2],parse[3]);
			else
				throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, timePart);

		} else if (timePart.length() == 8) {
			if (parse("%u1:%u2:%u3",timePart))
				res = TimeStamp::fromYMDhms(0,0,0,parse[1],parse[2],parse[3]);
			else
				throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, timePart);
		}

		res = res + TimeStamp::intervalMSec(millis);
		if (!zonespec.empty()) {
			if (zonespec != "Z") {
				integer zoneofsmin;
				char zonedir = zonespec[0];
				zonespec = zonespec.offset(1);
				if (parse("%u1",zonespec)) {
					zoneofsmin = (natural)parse[1] * 60;
				} else if (parse("%(2,2)u1%%(2,2)u2",zonespec)) {
					zoneofsmin = (natural)parse[1] * 60+(natural)parse[2];
				} else if (parse("%u1%:%u2",zonespec)) {
					zoneofsmin = (natural)parse[1] * 60+(natural)parse[2];
				} else  {
					throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, zonespec);				
				}
				TimeStamp ofs = TimeStamp::intervalMin(zoneofsmin);
				if (zonedir == '-') {
					res = res + ofs;
				} else if (zonedir != '+') {
					res = res - ofs;
				} else {
					throw TimeStamp::InvalidDateTimeFormat(THISLOCATION, zonespec);				
				}
			}
		}
		return res;
		

	}


	LightSpeed::TimeStamp TimeStamp::fromISO8601Time( ConstStrA isotime )
	{
		natural sep = isotime.findOneOf(ConstStrA("T "));
		if (sep == naturalNull) sep = isotime.length();
		ConstStrA datePart = isotime.head(sep);
		ConstStrA timePart = isotime.offset(sep+1);

		try {
		TimeStamp date = parse8601date(datePart);
		TimeStamp time = parse8601time(timePart);
		return date + time;
		} catch (Exception &e) {
			throw InvalidDateTimeFormat(THISLOCATION, isotime) << e;
		}
/*

		ConstStrA::Iterator iter = isotime;
		natural year = 0;
		natural month = 0;
		natural day = 0;
		natural hour = 0;
		natural minute = 0;
		natural second = 0;
		natural houroffset = 0;
		natural minuteoffset = 0;
		char sign;

		char c;
		for (natural i = 0; i < 4; i++) {
			c = iter.getNext();
			if (isdigit(c)) year = year*10 + (c - '0'); 
			else throw InvalidDateTimeFormat(THISLOCATION,isotime);
		}
		c = iter.peek();
		if (c == '-') iter.skip();
		for (natural i = 0; i < 2; i++) {
			c = iter.getNext();
			if (isdigit(c)) month = month * 10 +(c - '0');
			else throw InvalidDateTimeFormat(THISLOCATION,isotime);
		}
		c = iter.peek();
		if (c == '-') iter.skip();
		for (natural i = 0; i < 2; i++) {
			c = iter.getNext();
			if (isdigit(c)) day = day * 10 +(c - '0');
			else throw InvalidDateTimeFormat(THISLOCATION,isotime);
		}
		if (iter.hasItems()) {
			c = iter.getNext();
			if (c != 'T') throw InvalidDateTimeFormat(THISLOCATION,isotime);
			for (natural i = 0; i < 4; i++) {
				c = iter.getNext();
				if (isdigit(c)) hour = hour*10 + (c - '0'); 
				else throw InvalidDateTimeFormat(THISLOCATION,isotime);
			}			
			c = iter.peek();
			if (c == ':') iter.skip();
			c = iter.peek();
			if (isdigit(c)) {
				for (natural i = 0; i < 2; i++) {
					c = iter.getNext();
					if (isdigit(c)) minute = minute * 10 +(c - '0');
					else throw InvalidDateTimeFormat(THISLOCATION,isotime);
				}
				c = iter.peek();
				if (c == ':') iter.skip();
				c = iter.peek();
				if (isdigit(c)) {
					for (natural i = 0; i < 2; i++) {
						c = iter.getNext();
						if (isdigit(c)) second = second * 10 +(c - '0');
						else throw InvalidDateTimeFormat(THISLOCATION,isotime);
					}
				}
			}
		}
*/		
	}

	LightSpeed::StringA LightSpeed::TimeStamp::formatTime(ConstStrA format,natural hintBuffer) const {

		if (hintBuffer == 0) hintBuffer = format.length() * 3;
		AutoArray<char> buffer;
		buffer.resize(hintBuffer);
		ConstStrA res = formatTime(buffer,format);
		if (res.empty()) {
			buffer.resize(hintBuffer * 2);
			res = formatTime(buffer,format);
			if (res.empty()) {
				buffer.resize(hintBuffer * 4);
				res = formatTime(buffer,format);
				if (res.empty()) {
					throw ErrNoWithDescException(THISLOCATION,EFAULT,"Failed to create buffer for formatted date");
				}
			}
		}
		return res;

	}


	const char *TimeStamp::message_InvalidDateTimeFormat = "Invalid date/time: %1";


	static const char *DAY_NAMES[] =
	{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static const char *MONTH_NAMES[] =
	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };



	TimeStamp::RFC1123Time TimeStamp::asRFC1123Time() const {

		CArray<char,30> buf;
		formatTime(buf, "%w--, %d %m- %Y %H:%M:%S GMT");
		memcpy(buf.data(), DAY_NAMES[buf[0] - '0'], 3);
		memcpy(buf.data()+8, MONTH_NAMES[(buf[8] - '0') * 10 + (buf[9] - '0') -1 ], 3);

		return buf;

	}


	TimeStamp::DBTime TimeStamp::asDBTime() const {
		CArray<char,20> buf;
		return formatTime(buf,"%Y-%m-%d %H:%M:%S");
	}


	TimeStamp::ISO8601Time TimeStamp::asISO8601Time() const
	{
		CArray<char,21> buf;
		return formatTime(buf,"%Y-%m-%dT%H:%M:%SZ");		
	}



}
