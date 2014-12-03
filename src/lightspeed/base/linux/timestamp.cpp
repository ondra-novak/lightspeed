#include <math.h>
#include <time.h>
#include <errno.h>
#include "../debug/programlocation.h"
#include "../exceptions/systemException.h"
#include <string.h>
#include "../debug/dbglog.h"
#include "../timestamp.h"

#include "../../base/text/textParser.tcc"

namespace LightSpeed {


	TimeStamp::TimeStamp( double day )
	{
		double fr,intr;
		fr = modf(day,&intr);
		this->day = (integer)intr;
		this->time = (integer)round(fr * dayMillis);
	}


	class TZOffset {
	public:
		integer offset;

		TZOffset() {
			time_t t,t2;
			time(&t);
			struct tm tmp;
			gmtime_r(&t,&tmp);
			t2 = mktime(&tmp);
			offset = t - t2;
		}
	};

	//static TZOffset tszoffset;
/*	double TimeStamp::getFloat() const
	{
		return day + (double)time / (double)dayMillis;
	}*/

	TimeStamp LightSpeed::TimeStamp::fromYMDhms(natural Y, natural M, natural D,
			natural h, natural m, natural s) {
		if (Y == 0) {
			natural daysec = ((((h * 60) + m) * 60) + s);
			natural ofs = daysec / daySecs;
			daysec %= daySecs;

			return TimeStamp(M*30+D + ofs,daysec * 1000);
		} else {
			struct tm tminfo;
			tminfo.tm_year = Y - 1900;
			tminfo.tm_mon = M - 1;
			tminfo.tm_mday = D ;
			tminfo.tm_hour = h;
			tminfo.tm_min = m;
			tminfo.tm_sec = s;
			tminfo.tm_isdst = 0;
			tminfo.tm_wday = 0;
			tminfo.tm_yday = 0;

			time_t r = timegm(&tminfo);
			if (r == -1) throw ErrNoWithDescException(THISLOCATION, errno, "Failed to create time");
//			r-=tminfo.tm_gmtoff;
			return TimeStamp(r/daySecs,(r%daySecs)*1000);
		}
	}

	LightSpeed::StringA internalFormatTime(natural day, natural time, ConstStrA format, natural buffSize) {

		time_t unixTime = day;
		unixTime *= TimeStamp::daySecs;
		unixTime += (time+500) / 1000;
		struct tm timeinfo;
		gmtime_r(&unixTime,&timeinfo);
		char *buffer = (char * )alloca(buffSize);
		buffer[0] = 'x';
		natural r = strftime(buffer,buffSize,format.data(),&timeinfo);
		if (r == 0 && buffer[0] != 0) {
			buffSize = buffSize * 3 / 2;
			return internalFormatTime(day,time,format,buffSize);
		} else {
			return buffer;
		}
	}

	LightSpeed::StringA LightSpeed::TimeStamp::formatTime(ConstStrA format,natural hintBuffer) const {
		if (hintBuffer == 0) hintBuffer = format.length() * 3 + 10;
		if (format.data()[format.length()] != 0) {
			//HACK: watch out, pointer can be unaccessible
			char *buff = (char *)alloca(format.length()+1);
			strncpy(buff,format.data(),format.length());
			buff[format.length()] = 0;
			return formatTime(buff,hintBuffer);
		}
		return internalFormatTime(this->day,this->time,format,hintBuffer);
	}

	StringA TimeStamp::asDBTime() const {
		return formatTime("%Y-%m-%d %H:%M:%S",20);
	}

	const char *TimeStamp::message_InvalidDateTimeFormat = "Invalid date/time: %1";


	TimeStamp TimeStamp::fromDBDate(ConstStrA dbdate) {
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

}

