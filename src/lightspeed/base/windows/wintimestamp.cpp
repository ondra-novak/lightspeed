#include "winpch.h"
#include "..\timestamp.h"
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

	TimeStamp TimeStamp::now() {
		
		SYSTEMTIME sm;
		GetSystemTime(&sm);
		FILETIME ft;
		SystemTimeToFileTime(&sm,&ft);
		return TimeStamp::fromWindows(ft.dwLowDateTime,ft.dwHighDateTime);
	}

	TimeStamp::TimeStamp( double day )
	{
		double fr,intr;
		fr = modf(day,&intr);
		this->day = (integer)intr;
		this->time = (integer)floor(fr * dayMillis+0.5);
	}

	time_t timegm(struct tm * a_tm)
	{
		time_t ltime = mktime(a_tm);
		struct tm tm_val;
		gmtime_s(&tm_val, &ltime);
		int offset = (tm_val.tm_hour - a_tm->tm_hour);
		if (offset > 12)
		{
			offset = 24 - offset;
		}
		time_t utc = mktime(a_tm) - offset * 3600;
		return utc;
	}

	TimeStamp TimeStamp::fromYMDhms(natural Y, natural M, natural D,
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
			return TimeStamp((integer)r/daySecs,(integer)((r%daySecs)*1000));
		}
	}

	ConstStrA TimeStamp::formatTime(ArrayRef<char> targetBuffer, ConstStrA format) const {
		StringA fmthld;
		const char *cfmt = cStr(format,fmthld);

		time_t unixTime = day;
		unixTime *= TimeStamp::daySecs;
		unixTime += (time+500) / 1000;
		struct tm timeinfo;
		gmtime_s(&timeinfo,&unixTime);
		natural r = strftime(targetBuffer.data(),targetBuffer.length(),cfmt,&timeinfo);
		return targetBuffer.head(r);

	}





}
