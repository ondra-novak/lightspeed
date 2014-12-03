#include "winpch.h"
#include "..\timestamp.h"
#include <math.h>
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
}