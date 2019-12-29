/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"

// calendar types
enum {
	calendar_Gregorian		= 1,

	kYear_Months			= 12,
	
	kYear_Days				= 365,
	kYear_Hours				= 8760,
	kYear_Minutes			= 525600,
	kYear_Seconds			= 31536000,

	kLeapYear_Days			= 366,
	kLeapYear_Hours			= 8784,
	kLeapYear_Minutes		= 527040,
	kLeapYear_Seconds		= 31622400,
		
	kWeek_Days				= 7,
	kWeek_Hours				= 168,
	kWeek_Minutes			= 10080,
	kWeek_Seconds			= 604800,
		
	kDay_Hours				= 24,
	kDay_Minutes			= 1440,
	kDay_Seconds			= 86400,
	
	kHour_Minutes			= 60,
	kHour_Seconds			= 3600,

	kMinute_Seconds			= 60,
	kMinute_Milliseconds 	= 60000,
	
	kSecond_Milliseconds 	= 1000
};

// date-to-text options
enum {
	kShortDateText					= 0x01,		// 1/31/92
	kShortDateFullYearText			= 0x02,		// 1/31/1992
	kAbbrevDateText					= 0x04,		// Fri, Jan 31, 1992
	kLongDateText					= 0x08,		// Friday, January 31, 1992

	kTimeText						= 0x10,		// 10:41 PM
	kTimeWithSecsText				= 0x20		// 10:41:18 PM
};

struct SDateTimeStamp;

#pragma options align=packed
struct SCalendarDate {
	Int16 year;			// eg, 1996, 2060
	Int16 month;		// 1-12 (Jan-Dec)
	Int16 day;			// 1-31
	Int16 hour;			// 0-23
	Int16 minute;		// 0-59
	Int16 second;		// 0-59
	Int16 weekDay;		// 1-7 (Mon-Sun)
	Int16 val;			// set to 0

	bool operator==(const SCalendarDate& cd) const;
	bool operator!=(const SCalendarDate& cd) const;
	bool operator>(const SCalendarDate& cd) const;
	bool operator<(const SCalendarDate& cd) const;
	bool operator>=(const SCalendarDate& cd) const;
	bool operator<=(const SCalendarDate& cd) const;
	SCalendarDate& operator+=(Int32 inSeconds);
	SCalendarDate& operator-=(Int32 inSeconds);
	SCalendarDate operator+(Int32 inSeconds) const;
	SCalendarDate operator-(Int32 inSeconds) const;
	Int32 operator-(const SCalendarDate& cd) const;		// return seconds
	operator SDateTimeStamp() const;					// cast operator
	
	bool IsValid() const;
};
 
struct SDateTimeStamp {
	Uint16 year;		// start off at midnight 1/1 of this year
	Uint16 msecs;		// add this many milliseconds
	Uint32 seconds;		// add this many seconds
	
	bool operator==(const SDateTimeStamp& dts) const;
	bool operator!=(const SDateTimeStamp& dts) const;
	bool operator>(const SDateTimeStamp& dts) const;
	bool operator<(const SDateTimeStamp& dts) const;
	bool operator>=(const SDateTimeStamp& dts) const;
	bool operator<=(const SDateTimeStamp& dts) const;
	SDateTimeStamp& operator+=(Int32 inSeconds);
	SDateTimeStamp& operator-=(Int32 inSeconds);
	SDateTimeStamp operator+(Int32 inSeconds) const;
	SDateTimeStamp operator-(Int32 inSeconds) const;
	Int32 operator-(const SDateTimeStamp& dts) const;	// return seconds
	operator SCalendarDate() const;						// cast operator
	
	bool IsValid() const;
	bool ConvertYear(Uint16 inYear);
	
	// returns 8 always
	Uint32 Flatten(void *outData);
	Uint32 Unflatten(const void *inData);		// requires minimum 8 bytes and returns 8
};
#pragma options align=reset

class UDateTime
{
	public:
		static Uint32 GetSeconds();
		static Uint32 GetMilliseconds();
		static void GetCalendarDate(Uint32 inCalendar, SCalendarDate& outInfo);
		static void GetDateTimeStamp(SDateTimeStamp& outInfo);
		static Uint32 DateToText(const SCalendarDate& inInfo, void *outText, Uint32 inMaxSize, Uint32 inOptions = kAbbrevDateText);
		static Uint32 DateToText(const SDateTimeStamp& inInfo, void *outText, Uint32 inMaxSize, Uint32 inOptions = kAbbrevDateText);
		static Uint32 DateToText(void *outText, Uint32 inMaxSize, Uint32 inOptions = kAbbrevDateText);
		static bool IsLeapYear(Uint32 inCalendar, Uint32 inYear);
		static Uint8 CalculateWeekDay(Uint32 inYear, Uint32 inMonth, Uint32 inDay);	// return 1-7 (Mon-Sun)
		static Int32 GetGMTDelta(); 		// return the difference, in seconds, between local time and GMT
};

