/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
/*
-- Conversions --

1 second		= 60 ticks
				= 1000 milliseconds
				= 1000000 microseconds

1 tick			= 1/60 seconds
				= 1000/60 milliseconds
				= 1000000/60 microseconds

seconds			= ticks / 60
milliseconds	= (ticks * 50) / 3
microseconds    = (ticks * 50000) / 3
*/



bool UDateTime::IsLeapYear(Uint32 /* inCalendar */, Uint32 inYear)
{
    if (inYear < 100) inYear += 1900;
	return ( ((inYear % 4) == 0) && ((inYear % 100) != 0) ) || ((inYear % 400) == 0);
}

// return 1-7 (Mon-Sun) or 0 if input is invalid
Uint8 UDateTime::CalculateWeekDay(Uint32 inYear, Uint32 inMonth, Uint32 inDay)
{
	// check date
	if (inYear <= 0 || inMonth < 1 || inMonth > 12 || inDay < 1 || inDay > 31)
		return 0;
			
	if (inDay > 30 && (inMonth == 4 || inMonth == 6 || inMonth == 9 || inMonth == 11))
		return 0;

	if (inMonth == 2)
	{
		if (UDateTime::IsLeapYear(calendar_Gregorian, inYear))
		{
			if (inDay > 29)
				return 0;
		}
		else if (inDay > 28)
			return 0;
	}

	// adjust date
	if (inMonth == 1)
	{
		inYear--;
		inMonth = 13;
	}
	else if (inMonth == 2)
	{
		inYear--;
		inMonth = 14;
	}

	// calculate day in week
	Uint8 nWeekDay = (inDay + 2 * inMonth + (Uint32)(0.6 * (inMonth + 1)) + inYear + inYear/4 - inYear/100 + inYear/400 + 1) % 7;
	if (nWeekDay == 0)
		nWeekDay = 7;
	
	return nWeekDay;
}

#if 0
Uint32 UDateTime::DateToText(const SCalendarDate& inInfo, void *outText, Uint32 inMaxSize, Uint32 inOptions)
{
	static const Int8 *day_name[] = {
		"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
	};
	static const Int8 *abbrev_day_name[] = {
		"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
	};
	static const Int8 *month_name[] = {
		"January", "February", "March", "April", "May", "June",	"July", "August",
		"September", "October", "November", "December"
	};
	static const Int8 *abbrev_month_name[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	Uint32 s;
	
	Require(inInfo.weekDay >= 1 && inInfo.weekDay <= 7 && inInfo.month >= 1 && inInfo.month <= 12);
	
	if (inOptions & kShortDateText)						// eg "1/31/92"
		s = UText::Format(outText, inMaxSize, "%hu/%hu/%2.2hu", inInfo.month, inInfo.day, inInfo.year % 100);
	else if (inOptions & kShortDateFullYearText)		// eg "1/31/1992"
		s = UText::Format(outText, inMaxSize, "%hu/%hu/%hu", inInfo.month, inInfo.day, inInfo.year);
	else if (inOptions & kAbbrevDateText)				// eg "Fri, Jan 31, 1992"
		s = UText::Format(outText, inMaxSize, "%s, %s %hu, %hu", abbrev_day_name[inInfo.weekDay-1], abbrev_month_name[inInfo.month-1], inInfo.day, inInfo.year);
	else if (inOptions & kLongDateText)					// eg "Friday, January 31, 1992"
		s = UText::Format(outText, inMaxSize, "%s, %s %hu, %hu", day_name[inInfo.weekDay-1], month_name[inInfo.month-1], inInfo.day, inInfo.year);
	else
		s = 0;
	
	if (inOptions & (kTimeText | kTimeWithSecsText))	// eg "10:41 AM" or "10:41:18 AM"
	{
		outText = BPTR(outText) + s;
		inMaxSize -= s;
		
		if (inMaxSize >= 6)	// don't bother outputting time if not enough space
		{
			if (s)	// if had date
			{
				if (inOptions & (kShortDateText|kShortDateFullYearText))
				{
					*((Uint8 *)outText)++ = ' ';
					inMaxSize--;
					s++;
				}
				else
				{
					*((Uint8 *)outText)++ = ',';
					*((Uint8 *)outText)++ = ' ';
					inMaxSize -= 2;
					s += 2;
				}
			}
			
			Int16 hour = inInfo.hour;
			const Int8 *am;
			
			if (hour < 12)
			{
				am = "AM";
				hour++;
			}
			else
			{
				am = "PM";
				hour++;
				hour -= 12;
			}
			
			if (inOptions & kTimeWithSecsText)
				s += UText::Format(outText, inMaxSize, "%hu:%2.2hu:%2.2hu %s", hour, inInfo.minute, inInfo.second, am);
			else
				s += UText::Format(outText, inMaxSize, "%hu:%2.2hu %s", hour, inInfo.minute, am);
		}
	}

	return s;
}
#endif

/* -------------------------------------------------------------------------- */

bool SCalendarDate::operator==(const SCalendarDate& cd) const
{
	return (this->year == cd.year && this->month == cd.month && this->day == cd.day && 
			this->hour == cd.hour && this->minute == cd.minute && this->second == cd.second);
}

bool SCalendarDate::operator!=(const SCalendarDate& cd) const
{
	return (this->year != cd.year || this->month != cd.month || this->day != cd.day || 
			this->hour != cd.hour || this->minute != cd.minute || this->second != cd.second);
}

bool SCalendarDate::operator>(const SCalendarDate& cd) const
{
	if (this->year > cd.year)
		return true;
	
	if (this->year == cd.year)
	{
		if (this->month > cd.month)
			return true;
		
		if (this->month == cd.month)
		{
			if (this->day > cd.day)
				return true;

			if (this->day == cd.day)
			{
				if (this->hour > cd.hour)
					return true;
				
				if (this->hour == cd.hour)
				{
					if (this->minute > cd.minute)
						return true;
					
					if (this->minute == cd.minute)
					{
						if (this->second > cd.second)
							return true;
					}
				}
			}
		}
	}
	
	return false;
}

bool SCalendarDate::operator<(const SCalendarDate& cd) const
{
	if (this->year < cd.year)
		return true;
	
	if (this->year == cd.year)
	{
		if (this->month < cd.month)
			return true;
		
		if (this->month == cd.month)
		{
			if (this->day < cd.day)
				return true;

			if (this->day == cd.day)
			{
				if (this->hour < cd.hour)
					return true;
				
				if (this->hour == cd.hour)
				{
					if (this->minute < cd.minute)
						return true;
					
					if (this->minute == cd.minute)
					{
						if (this->second < cd.second)
							return true;
					}
				}
			}
		}
	}
	
	return false;
}

bool SCalendarDate::operator>=(const SCalendarDate& cd) const
{
	if (this->year > cd.year)
		return true;
	
	if (this->year == cd.year)
	{
		if (this->month > cd.month)
			return true;
		
		if (this->month == cd.month)
		{
			if (this->day > cd.day)
				return true;

			if (this->day == cd.day)
			{
				if (this->hour > cd.hour)
					return true;
				
				if (this->hour == cd.hour)
				{
					if (this->minute > cd.minute)
						return true;
					
					if (this->minute == cd.minute)
					{
						if (this->second >= cd.second)
							return true;
					}
				}
			}
		}
	}
	
	return false;
}

bool SCalendarDate::operator<=(const SCalendarDate& cd) const
{
	if (this->year < cd.year)
		return true;
	
	if (this->year == cd.year)
	{
		if (this->month < cd.month)
			return true;
		
		if (this->month == cd.month)
		{
			if (this->day < cd.day)
				return true;

			if (this->day == cd.day)
			{
				if (this->hour < cd.hour)
					return true;
				
				if (this->hour == cd.hour)
				{
					if (this->minute < cd.minute)
						return true;
					
					if (this->minute == cd.minute)
					{
						if (this->second <= cd.second)
							return true;
					}
				}
			}
		}
	}
	
	return false;
}

SCalendarDate& SCalendarDate::operator+=(Int32 inSeconds)
{
	SDateTimeStamp dts = (SDateTimeStamp)*this;
	dts += inSeconds;
	*this = (SCalendarDate)dts;
	
	return *this;
}

SCalendarDate& SCalendarDate::operator-=(Int32 inSeconds)
{
	SDateTimeStamp dts = (SDateTimeStamp)*this;
	dts -= inSeconds;
	*this = (SCalendarDate)dts;
	
	return *this;
}

SCalendarDate SCalendarDate::operator+(Int32 inSeconds) const
{
	SCalendarDate cd = *this;
	cd += inSeconds;
	
	return cd;
}

SCalendarDate SCalendarDate::operator-(Int32 inSeconds) const
{
	SCalendarDate cd = *this;
	cd -= inSeconds;
	
	return cd;
}

// return seconds
Int32 SCalendarDate::operator-(const SCalendarDate& cd) const
{
	SDateTimeStamp dts1 = (SDateTimeStamp)*this;
	SDateTimeStamp dts2 = (SDateTimeStamp)cd;

	return (dts1 - dts2);
}

SCalendarDate::operator SDateTimeStamp() const
{
	SDateTimeStamp dts;
	ClearStruct(dts);
	
	if (!this->IsValid())
		return dts;
	
	dts.year = this->year;
	
	Uint32 bufSeconds[12] = {0, 2678400, 5097600, 7776000, 10368000, 13046400, 15638400, 18316800, 20995200, 23587200, 26265600, 28857600 };
	if (UDateTime::IsLeapYear(calendar_Gregorian, this->year))
		bufSeconds[2] += kDay_Seconds;
	
	dts.seconds = bufSeconds[this->month - 1] + (this->second + (60 * (this->minute + 60 * ((this->hour + 24 * (this->day - 1))))));
	
	return dts;
}

bool SCalendarDate::IsValid() const
{
	if (this->year <= 0 || this->month < 1 || this->month > 12 || this->day < 1 || this->day > 31 || this->weekDay < 1 || this->weekDay > 7)
		return false;
			
	if (this->day > 30 && (this->month == 4 || this->month == 6 || this->month == 9 || this->month == 11))
		return false;

	if (this->month == 2)
	{
		if (UDateTime::IsLeapYear(calendar_Gregorian, this->year))
		{
			if (this->day > 29)
				return false;
		}
		else if (this->day > 28)
			return false;
	}
	
	if (this->hour < 0 || this->hour > 23 || this->minute < 0 || this->minute > 59 || this->second < 0 || this->second > 59)
		return false;
	
	return true;
}

/* -------------------------------------------------------------------------- */

SDateTimeStamp& SDateTimeStamp::operator+=(Int32 inSeconds)
{
	if (!inSeconds)
		return *this;
	
	if (inSeconds < 0)
	{
		*this -= -inSeconds;
		return *this;
	}
		
	while (this->seconds + inSeconds < inSeconds)
	{
		Uint32 nSeconds;
		if (UDateTime::IsLeapYear(calendar_Gregorian, this->year))
			nSeconds = kLeapYear_Seconds;
		else
			nSeconds = kYear_Seconds;
				
		if (this->seconds < nSeconds)	// the operation cannot be completed
			return *this;
		
		this->year++;
		this->seconds -= nSeconds;
	}
	
	this->seconds += inSeconds;
	return *this;
}

SDateTimeStamp& SDateTimeStamp::operator-=(Int32 inSeconds)
{
	if (!inSeconds)
		return *this;
	
	if (inSeconds < 0)
	{
		*this += -inSeconds;
		return *this;
	}
		
	while (this->seconds < inSeconds)
	{
		if (this->year <= 1)		// the operation cannot be completed
			return *this;

		Uint32 nSeconds;
		if (UDateTime::IsLeapYear(calendar_Gregorian, this->year - 1))
			nSeconds = kLeapYear_Seconds;
		else
			nSeconds = kYear_Seconds;
						
		this->year--;
		this->seconds += nSeconds;
	}
	
	this->seconds -= inSeconds;
	return *this;
}

SDateTimeStamp SDateTimeStamp::operator+(Int32 inSeconds) const
{
	SDateTimeStamp dts = *this;
	dts += inSeconds;
	
	return dts;
}

SDateTimeStamp SDateTimeStamp::operator-(Int32 inSeconds) const
{
	SDateTimeStamp dts = *this;
	dts -= inSeconds;
	
	return dts;
}

// return seconds
Int32 SDateTimeStamp::operator-(const SDateTimeStamp& dts) const
{
	SDateTimeStamp dts1 = *this;
	SDateTimeStamp dts2 = dts;

	while (dts1.year > dts2.year)
	{
		if (UDateTime::IsLeapYear(calendar_Gregorian, dts1.year - 1))
			dts1.seconds += kLeapYear_Seconds;
		else
			dts1.seconds += kYear_Seconds;

		dts1.year--;
	}
	
	while (dts1.year < dts2.year)
	{
		if (UDateTime::IsLeapYear(calendar_Gregorian, dts2.year - 1))
			dts2.seconds += kLeapYear_Seconds;
		else
			dts2.seconds += kYear_Seconds;

		dts2.year--;
	}

	// lose some milliseconds
	return (dts1.seconds + dts1.msecs/kSecond_Milliseconds - dts2.seconds - dts2.msecs/kSecond_Milliseconds);
}

SDateTimeStamp::operator SCalendarDate() const
{
	SCalendarDate cd;
	ClearStruct(cd);

	if (!this->IsValid())
		return cd;

	// year
	cd.year = this->year;
	Uint32 nSecondsCount = this->seconds + this->msecs/kSecond_Milliseconds;	// lose some milliseconds
	
	while (true)
	{
		Uint32 nSeconds;
	 	if (UDateTime::IsLeapYear(calendar_Gregorian, cd.year))
			nSeconds = kLeapYear_Seconds;
		else
			nSeconds = kYear_Seconds;
	
		if (nSecondsCount < nSeconds)
			break;
			
		cd.year++;
		nSecondsCount -= nSeconds;
	}

	// month
	Uint32 bufSeconds[12] = {0, 2678400, 5097600, 7776000, 10368000, 13046400, 15638400, 18316800, 20995200, 23587200, 26265600, 28857600 };
	if (UDateTime::IsLeapYear(calendar_Gregorian, cd.year))
		bufSeconds[2] += kDay_Seconds;
	
	cd.month = 1;
	while (nSecondsCount > bufSeconds[cd.month - 1] && cd.month < kYear_Months)
		cd.month++;
	
	if (nSecondsCount < bufSeconds[cd.month - 1])
		cd.month--;
	
	nSecondsCount -= bufSeconds[cd.month - 1];
	
	// day
	cd.day = nSecondsCount/kDay_Seconds + 1;
	nSecondsCount -= (cd.day - 1) * kDay_Seconds;
	
	// hour
	cd.hour = nSecondsCount/kHour_Seconds;
	nSecondsCount -= cd.hour * kHour_Seconds;
	
	// minute
	cd.minute = nSecondsCount/kMinute_Seconds;
	nSecondsCount -= cd.minute * kMinute_Seconds;

	// second
	cd.second = nSecondsCount;
	
	// weekDay
	cd.weekDay = UDateTime::CalculateWeekDay(cd.year, cd.month, cd.day);
	
	return cd;
}

bool SDateTimeStamp::IsValid() const
{
	if (!this->year)
		return false;

	return true;
}

bool SDateTimeStamp::ConvertYear(Uint16 inYear)
{
	if (!inYear)
		return false;
		
	Uint32 nSeconds;
	while (this->year != inYear)
	{
		if (this->year > inYear)
		{
			if (UDateTime::IsLeapYear(calendar_Gregorian, this->year - 1))
				nSeconds = kLeapYear_Seconds;
			else
				nSeconds = kYear_Seconds;
						
			this->year--;
			this->seconds += nSeconds;
		}
		else
		{
			if (UDateTime::IsLeapYear(calendar_Gregorian, this->year))
				nSeconds = kLeapYear_Seconds;
			else
				nSeconds = kYear_Seconds;
				
			if (this->seconds < nSeconds)	// the operation cannot be completed
				return false;
		
			this->year++;
			this->seconds -= nSeconds;
		}
	}
	
	return true;
}

/***********************************************************

CALENDAR NOTES

Gregorian, number of days in each month:

Jan 31
Feb 28 - 29 days in a leap year
Mar 31
Apr 30
May 31
Jun 30
Jul 31
Aug 31
Sep 30
Oct 31
Nov 30
Dec 31

In the gregorian calendar, years that are divisible by 400
(to produce a whole number) are leap years.



***********************************************************/




#if 0

static char *GMCsingledaynames[] =
{
	"S",
	"M",
	"T",
	"W",
	"T",
	"F",
	"S"
};

static char *GMCshortdaynames[] =
{
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

static char *GMCdaynames[] =
{
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};

static char *GMCshortmonthnames[] =
{"",
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"};

static char *GMCmonthnames[] =
{"",
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"};

static int GMCmonthlengths[] = {31,28,31,30,31,30,31,31,30,31,30,31};

static AStringLabel* GDayTextArray[32];


static inline long GMCdaysInMonth(int m, int y)
{
    if ((m==2)&& GMCisleap(y))
        return 29;
    else
        return GMCmonthlengths[m-1];
}


#endif

