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

	ConstStrA TimeStamp::formatTime(ArrayRef<char> targetBuffer, ConstStrA format) const {
		StringA fmthld;
		const char *cfmt = cStr(format,fmthld);

		time_t unixTime = day;
		unixTime *= TimeStamp::daySecs;
		unixTime += (time+500) / 1000;
		struct tm timeinfo;
		gmtime_r(&unixTime,&timeinfo);
		natural r = strftime(targetBuffer.data(),targetBuffer.length(),cfmt,&timeinfo);
		return targetBuffer.head(r);

	}



}

