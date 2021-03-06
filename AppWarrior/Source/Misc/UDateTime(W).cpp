#if WIN32
 
/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UDateTime.h"
void _DTSMinimizeDTS(SDateTimeStamp &ioDTS);
void _DTSSysTimeToDTS(SYSTEMTIME st, SDateTimeStamp &dts);

/* -------------------------------------------------------------------------- */

bool SDateTimeStamp::operator==(const SDateTimeStamp& dts) const
{
	SDateTimeStamp thisDTS = *this;
	SDateTimeStamp inDts = dts;
	
	_DTSMinimizeDTS(thisDTS);
	_DTSMinimizeDTS(inDts);
	
	return (thisDTS.year == inDts.year && thisDTS.seconds == inDts.seconds && thisDTS.msecs == inDts.msecs);
}

bool SDateTimeStamp::operator!=(const SDateTimeStamp& dts) const
{
	SDateTimeStamp thisDTS = *this;
	SDateTimeStamp inDts = dts;
	
	_DTSMinimizeDTS(thisDTS);
	_DTSMinimizeDTS(inDts);
	
	return (thisDTS.year != inDts.year || thisDTS.seconds != inDts.seconds || thisDTS.msecs != inDts.msecs);
}

bool SDateTimeStamp::operator>(const SDateTimeStamp& dts) const
{
	SDateTimeStamp thisDTS = *this;
	SDateTimeStamp inDts = dts;
	
	_DTSMinimizeDTS(thisDTS);
	_DTSMinimizeDTS(inDts);
	
	if(thisDTS.year > inDts.year)
		return true;
	
	if(thisDTS.year == inDts.year)
	{
		if(thisDTS.seconds > inDts.seconds)
			return true;
		
		if(thisDTS.seconds == inDts.seconds)
		{
			if(thisDTS.msecs > inDts.msecs)
				return true;
		}
	}
	return false;
}

bool SDateTimeStamp::operator<(const SDateTimeStamp& dts) const
{
	SDateTimeStamp thisDTS = *this;
	SDateTimeStamp inDts = dts;
	
	_DTSMinimizeDTS(thisDTS);
	_DTSMinimizeDTS(inDts);
	
	if(thisDTS.year < inDts.year)
		return true;
	
	if(thisDTS.year == inDts.year)
	{
		if(thisDTS.seconds < inDts.seconds)
			return true;
		
		if(thisDTS.seconds == inDts.seconds)
		{
			if(thisDTS.msecs < inDts.msecs)
				return true;
		}
	}
	return false;
}


bool SDateTimeStamp::operator>=(const SDateTimeStamp& dts) const
{
	SDateTimeStamp thisDTS = *this;
	SDateTimeStamp inDts = dts;
	
	_DTSMinimizeDTS(thisDTS);
	_DTSMinimizeDTS(inDts);
	
	if(thisDTS.year > inDts.year)
		return true;
	
	if(thisDTS.year == inDts.year)
	{
		if(thisDTS.seconds > inDts.seconds)
			return true;
		
		if(thisDTS.seconds == inDts.seconds)
		{
			if(thisDTS.msecs >= inDts.msecs)
				return true;
		}
	}
	return false;
}


bool SDateTimeStamp::operator<=(const SDateTimeStamp& dts) const
{
	SDateTimeStamp thisDTS = *this;
	SDateTimeStamp inDts = dts;
	
	_DTSMinimizeDTS(thisDTS);
	_DTSMinimizeDTS(inDts);
	
	if(thisDTS.year < inDts.year)
		return true;
	
	if(thisDTS.year == inDts.year)
	{
		if(thisDTS.seconds < inDts.seconds)
			return true;
		
		if(thisDTS.seconds == inDts.seconds)
		{
			if(thisDTS.msecs <= inDts.msecs)
				return true;
		}
	}
	return false;
}


Uint32 SDateTimeStamp::Flatten(void *outData)
{
	*((Uint16*)outData) = TB(year);
	*((Uint16*)(BPTR(outData) + sizeof(Uint16))) = TB(msecs);
	*((Uint32*)(BPTR(outData) + 2 * sizeof(Uint16))) = TB(seconds);
	return sizeof(Uint16) + sizeof(Uint16) + sizeof(Uint32);
}

Uint32 SDateTimeStamp::Unflatten(const void *inData)
{
	year = FB(*((Uint16*)inData));
	msecs = FB(*((Uint16*)(BPTR(inData) + sizeof(Uint16))));
	seconds = FB(*((Uint32*)(BPTR(inData) + sizeof(Uint16) + sizeof(Uint16))));
	return sizeof(Uint16) + sizeof(Uint16) + sizeof(Uint32);
}


void _DTSMinimizeDTS(SDateTimeStamp &ioDTS)
{
	SYSTEMTIME st = { ioDTS.year, 1, 0, 1, 0, 0, 0, 0 };
	FILETIME ft;
	SHuge huge;
	
	if (!::SystemTimeToFileTime(&st, &ft)) goto failed;
	
	UMath::Mul64U(ioDTS.seconds, 10000000/*00*/, huge);
	
	UMath::Add64(huge, *(SHuge *)&ft, *(SHuge *)&ft);
	
	huge.hi = 0;
	huge.lo = ioDTS.msecs * 10000/*00*/;

	UMath::Add64(huge, *(SHuge *)&ft, *(SHuge *)&ft);
	
	if (!::FileTimeToSystemTime(&ft, &st)) goto failed;
	
	_DTSSysTimeToDTS(st, ioDTS);
	return;
	
failed:
	return;
}

#pragma mark -

/*
 * GetSeconds() returns the number of seconds that have elapsed since a certain
 * time.  It's useful for elapsed time measurements in the minutes or hours.
 */
Uint32 UDateTime::GetSeconds()
{
	return ::GetTickCount() / 1000;
}

/*
 * GetMilliseconds() returns the number of milliseconds (1 sec = 1000 ms) that
 * have elapsed since a certain time (typically since the computer started up,
 * but don't rely on it).  It's useful for elapsed time measurements over a
 * short period - don't use GetMilliseconds() over a long time period (more
 * than a minute).
 */
Uint32 UDateTime::GetMilliseconds()
{
	return ::GetTickCount();
}

/*
 * GetCalendarDate() sets <outInfo> to the current date and time in the
 * calendar format specified by <inCalendar>.
 */
void UDateTime::GetCalendarDate(Uint32 /* inCalendar */, SCalendarDate& outInfo)
{
	SYSTEMTIME st;
	
//	::GetSystemTime(&st); // the system time is expressed in Coordinated Universal Time (UTC)
	::GetLocalTime(&st);
	
	outInfo.year = st.wYear;
	outInfo.month = st.wMonth;
	outInfo.day = st.wDay;
	outInfo.hour = st.wHour;
	outInfo.minute = st.wMinute;
	outInfo.second = st.wSecond;
	outInfo.weekDay = (st.wDayOfWeek == 0) ? 7 : st.wDayOfWeek;
	outInfo.val = 0;
}

void UDateTime::GetDateTimeStamp(SDateTimeStamp& outInfo)
{
	SYSTEMTIME st;
	
//	::GetSystemTime(&st); // the system time is expressed in Coordinated Universal Time (UTC)
	::GetLocalTime(&st);
	
	_DTSSysTimeToDTS(st, outInfo);
}


Uint32 UDateTime::DateToText(const SCalendarDate& inInfo, void *outText, Uint32 inMaxSize, Uint32 inOptions)
{
	SYSTEMTIME st;
	
	st.wYear = inInfo.year; 
	st.wMonth = inInfo.month; 
	st.wDayOfWeek = (inInfo.weekDay == 7) ? 0 : inInfo.weekDay; 
	st.wDay = inInfo.day;
	st.wHour = inInfo.hour; 
	st.wMinute = inInfo.minute; 
	st.wSecond = inInfo.second; 
	st.wMilliseconds = 0; 
	
	Uint32 s;

	if (inMaxSize == 0) return 0;
	
	if (inOptions & (kShortDateText|kShortDateFullYearText|kAbbrevDateText|kLongDateText))
	{
		if (inOptions & kShortDateFullYearText)
			s = ::GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, "MM'/'dd'/'yyyy", (LPTSTR)outText, inMaxSize);
		else if (inOptions & kAbbrevDateText)
			s = ::GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, "ddd',' MMM d',' yyyy", (LPTSTR)outText, inMaxSize);
		else
			s = ::GetDateFormat(LOCALE_USER_DEFAULT, (inOptions & kShortDateText) ? DATE_SHORTDATE : DATE_LONGDATE, &st, NULL, (LPTSTR)outText, inMaxSize);

		if (s) s--;		// don't include null char
	}
	else
		s = 0;
	
	if (inOptions & (kTimeText|kTimeWithSecsText))
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
			
			Uint32 ts = ::GetTimeFormat(LOCALE_USER_DEFAULT, (inOptions & kTimeWithSecsText) ? 0 : TIME_NOSECONDS, &st, NULL, (LPTSTR)outText, inMaxSize);
			if (ts) ts--;		// don't include null char
			
			s += ts;
		}
	}
	
	return s;
}

Uint32 UDateTime::DateToText(const SDateTimeStamp& inInfo, void *outText, Uint32 inMaxSize, Uint32 inOptions)
{
	SYSTEMTIME st = { inInfo.year, 1, 0, 1, 0, 0, 0, 0 };
	SCalendarDate cd;
	FILETIME ft;
	SHuge huge;
	
	if (!::SystemTimeToFileTime(&st, &ft)) return 0;
	
	UMath::Mul64U(inInfo.seconds, 10000000/*00*/, huge);
	
	UMath::Add64(huge, *(SHuge *)&ft, *(SHuge *)&ft);
	
	huge.hi = 0;
	huge.lo = inInfo.msecs * 10000/*00*/;

	UMath::Add64(huge, *(SHuge *)&ft, *(SHuge *)&ft);
	
	if (!::FileTimeToSystemTime(&ft, &st)) return 0;
	
	cd.year = st.wYear;
	cd.month = st.wMonth;
	cd.day = st.wDay;
	cd.hour = st.wHour;
	cd.minute = st.wMinute;
	cd.second = st.wSecond;
	st.wDayOfWeek -= 1;
	cd.weekDay = (st.wDayOfWeek == 0) ? 7 : st.wDayOfWeek;
	cd.val = 0;
	
	return DateToText(cd, outText, inMaxSize, inOptions);
}

Uint32 UDateTime::DateToText(void *outText, Uint32 inMaxSize, Uint32 inOptions)
{
	SCalendarDate date;
	GetCalendarDate(calendar_Gregorian, date);
	return DateToText(date, outText, inMaxSize, inOptions);
}

// return the difference, in seconds, between local time and GMT
Int32 UDateTime::GetGMTDelta()
{
/*	_TIME_ZONE_INFORMATION stTimeZoneInfo;
	ClearStruct(stTimeZoneInfo);
	
	if (GetTimeZoneInformation(&stTimeZoneInfo) == TIME_ZONE_ID_INVALID)
		return 0;
		
	return -((stTimeZoneInfo.Bias + stTimeZoneInfo.DaylightBias) * 60); OR
	return -((stTimeZoneInfo.Bias + stTimeZoneInfo.StandardBias) * 60);
*/

	SYSTEMTIME st1;	
	::GetLocalTime(&st1);
		
	SCalendarDate cd1;	
	cd1.year = st1.wYear;
	cd1.month = st1.wMonth;
	cd1.day = st1.wDay;
	cd1.hour = st1.wHour;
	cd1.minute = st1.wMinute;
	cd1.second = st1.wSecond;
	cd1.weekDay = (st1.wDayOfWeek == 0) ? 7 : st1.wDayOfWeek;
	cd1.val = 0;

	SYSTEMTIME st2;	
	::GetSystemTime(&st2);

	SCalendarDate cd2;	
	cd2.year = st2.wYear;
	cd2.month = st2.wMonth;
	cd2.day = st2.wDay;
	cd2.hour = st2.wHour;
	cd2.minute = st2.wMinute;
	cd2.second = st2.wSecond;
	cd2.weekDay = (st2.wDayOfWeek == 0) ? 7 : st2.wDayOfWeek;
	cd2.val = 0;
	
	return (cd1 - cd2);
}

void _DTSSysTimeToDTS(SYSTEMTIME st, SDateTimeStamp &dts)
{
	dts.year = st.wYear;
	
	const Uint32 secs[] = {	0, 2678400, 5097600, 7776000, 10368000, 13046400, 15638400, 18316800, 20995200, 23587200, 26265600, 28857600 };
	
	Uint16 month = min((Uint32)st.wMonth, (Uint32)12);
	
	dts.seconds = secs[month - 1]
		+ (month > 2 && UDateTime::IsLeapYear(calendar_Gregorian, st.wYear) ? 24 * 60 * 60 : 0)
		+ (st.wSecond + (60 * (st.wMinute + 60 * ((st.wHour + 24 * (st.wDay - 1))))));
	
	dts.msecs = st.wMilliseconds;
}


#endif /* WIN32 */
