

#include "types.h"
#include "datetime.h"
#include <time.h>

namespace LightSpeed {

static const lnatural daySeconds = 3600*24;

static const integer daysPerMonth[2][12] =
	{
			{31,28,31,30,31,30,31,31,30,31,30,31},
			{31,29,31,30,31,30,31,31,30,31,30,31}
	};

static integer intDiv(integer val, natural d) {
	return val / d - (val < 0?1:0);
}

static integer intMod(integer val, natural d) {
	return val - intDiv(val,d) * d;
}

static integer intMod(integer val, natural d, integer intDivW) {
	return val - intDivW * d;
}

static integer daysToYear(integer year)
    {
        return year * 365   //every year has 365 days
		+ intDiv(year + 3, 4) // + count of leap years (year 1 is after leap year)
		- intDiv(year + 99, 100) // - count of centuries (year 1 is first century)
		+ intDiv(year + 399, 400); // + count of leap centuries (year 1 is first year after leap century)
    }

static bool isLeapYear(integer year) {
	return (intMod(year, 4) == 0 && intMod(year ,100) != 0) //leap year is every 4th excluding century year
						  || (intMod(year ,400) == 0); // but every 4 th century is leap
}

integer Date::dayNumber(integer year, integer month, integer day ) {

	bool rep;


	//month will be shifted to be in range 0 - 11 (instead 1 - 12)
	if (month == 0) {year--;month+=12;} //decrease year, if month has been 0
	month--;
	int leapRow;


	do {
		rep = false;

		//how many years to add depend on overflow of month
		integer myears = intDiv(month,12);
		year += myears;
		//fix month to be in the range 0 - 11
		month = intMod(month,12,myears);


		//is current year leap?
		bool isLeap = isLeapYear(year);

		//choose row from table
		leapRow = isLeap?1:0;

		//every year has 365 days, leap year has 366
		integer yearDays = 365 + leapRow;
		//is  day outside year range?
		if (day > yearDays) {
			//decrease by yearDays
			day-=yearDays;
			//increase year
			year++;
			//force repeat
			rep = true;
			//is day below 1 - yearDays?
		} else if (day < 1 - yearDays) {
			//shift whole year
			day+=yearDays;
			//decrease year
			year--;
			rep = true;
		} else {
			//day is in the range of year, now fix to be range in current month
			//md is count of days in current month
			integer md = daysPerMonth[leapRow][month];
			//day are above current day count
			if (day > md) {
				//increase month
				month ++;
				//decrease days
				day-= md;
				//repeat
				rep = true;
			//day is below 1
			} else if (day < 1) {
				//decrease month (will be fixed later)
				month--;
				//increase days for the month
				//we should fix leapRow because there can be
				//overflow on month. But it is not necessary,
				//because count of days in December is the same in both types of year
				day += daysPerMonth[leapRow][intMod(month,12)];
				//force repeat
				rep = true;
			}
		}
		//repeat until values are not adjusted
	} while (rep);


	//calculate day number until year/1/1
	integer days = daysToYear(year);

	//add day count from the beginning of the year
	for (integer i = 0; i < month; i++)
		days+=daysPerMonth[leapRow][i];
	//add day (-1 because 0/1/1 is day 0)
	days += day - 1;

	return days;
}

Date::PartsEx Date::dateFromDayNumber(integer dayNr) {
	//guess current year from day number
	integer year = intDiv(dayNr, 365);
	//get dayNumber for first day in that year
	integer yearNr = daysToYear(year);
	//result can be greater than dayNr, because of leapYear
	while (yearNr > dayNr) {
		//decrease year
		year--;
		//get dayNumber againg
		yearNr = daysToYear(year);
	}
	//dayOfYear is count of days from the first January
	integer dayOfYear = dayNr - yearNr;
	//get leapRow depend on if current year is leap
	integer leapRow = isLeapYear(year)?1:0;
	//prepare for calculate day
	integer day = dayOfYear;
	//prepare month
	integer month = 0;
	//find current month and day
	while (day > daysPerMonth[leapRow][month]) {
		day -= daysPerMonth[leapRow][month];
		month++;
	}

	//because month is returned in range 0 - 11, add 1 to get correct month
	//because day is returned in range 0 - 31, add 1 to get correct day
	//because dayOfYear is returned in range 0 - 364, add 1 to get correct dayOfYear
	//TODO: fix dayOfWeek
	return PartsEx(year,month+1,day+1,intMod(dayNr,7),dayOfYear + 1);
}

Date::Date(integer y, integer m, integer d):days(dayNumber(y,m,d)) {}

Date::Date(const Parts &unp):days(
		dayNumber(unp.year,unp.month,unp.day)) {}

time_t Date::getUnixTime() const
{
	return daysToUnixTime(days);
}

Date Date::today()
{
	time_t tm;
	time(&tm);
	return fromUnixTime(tm);
}

Date::PartsEx Date::getPartsEx() const
{
	return dateFromDayNumber(days);
}

Date::Parts Date::getParts() const
{
	return dateFromDayNumber(days);
}

natural Date::getMonth() const
{
	return getPartsEx().month;
}

natural Date::getDay() const
{
	return getPartsEx().day;
}

integer Date::getYear() const
{
	return getPartsEx().year;
}

Date Date::operator --(int )
{
	days--;
	return Date(days+1);
}

Date Date::operator -(const Date & other) const
{
	return Date(days - other.days);
}

Date & Date::operator --()
{
	days--;
	return *this;
}


Date Date::operator +(const Date & other) const
{
	return Date(days + other.days);
}

Date Date::operator ++(int )
{
	days++;
	return Date(days-1);
}

Date & Date::operator ++()
{
	days++;
	return *this;
}

Date & Date::operator -=(const Date & other)
{
	days-=other.days;
	return *this;
}

Date & Date::operator +=(const Date & other)
{
	days+=other.days;
	return *this;
}

integer Date::unixTimeToDays(time_t t) {
	return (integer)(((lnatural)(t) / daySeconds));
}

time_t Date::daysToUnixTime(integer d)
{
	return (time_t)(((lnatural)(d) * daySeconds));
}

Date Date::operator +(const Parts & other) const
{
	Parts cur = getParts();
	cur.day += other.day;
	cur.month +=other.month;
	cur.year += other.year;
	return Date(cur);
}

Date Date::operator -(const Parts & other) const
{
	Parts cur = getParts();
	cur.day -= other.day;
	cur.month -=other.month;
	cur.year -= other.year;
	return Date(cur);
}

Date & Date::operator +=(const Parts & other)
{
	*this = *this + other;
	return *this;
}

Date & Date::operator -=(const Parts & other)
{
	*this = *this - other;
	return *this;
}


static Timezone::ZoneInfo zones[] = {
		Timezone::ZoneInfo(Timezone::GMT,"Greenwich Mean Time",0, false),
		Timezone::ZoneInfo(Timezone::BST,"British Summer Time",1, true),
		Timezone::ZoneInfo(Timezone::IST,"Irish Summer Time",1, true),
		Timezone::ZoneInfo(Timezone::WET,"Western European Time",0, false),
		Timezone::ZoneInfo(Timezone::WEST,"Western European Summer Time",1, true),
		Timezone::ZoneInfo(Timezone::CET,"Central European Time",1, false),
		Timezone::ZoneInfo(Timezone::CEST,"Central European Summer Time",2, true),
		Timezone::ZoneInfo(Timezone::EET,"Eastern European Time",2, false),
		Timezone::ZoneInfo(Timezone::EEST,"Eastern European Summer Time",3, true),
		Timezone::ZoneInfo(Timezone::MSK,"Moscow Standard Time",3, false),
		Timezone::ZoneInfo(Timezone::MSD,"Moscow Daylight Time",4, true)
};

bool LightSpeed::Timezone::getZoneInfo(ZoneInfo & )
{
	return false;
}



}
