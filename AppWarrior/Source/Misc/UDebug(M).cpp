#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UDebug.h"
#include "UText.h"
#include "stdarg.h"

#include <Types.h>

#if __POWERPC__ || powerc

#define	BreakToLowLevelDebugger()		SysBreak()
#define BreakStrToLowLevelDebugger(s)	SysBreakStr(s)
#define	BreakToSourceDebugger()			Debugger()
#define	BreakStrToSourceDebugger(s)		DebugStr(s)
#define	BreakToCurrentDebugger()		Debugger()
#define	BreakStrToCurrentDebugger(s)	DebugStr(s)

#else // 68K

#define	BreakToLowLevelDebugger()		Debugger()
#define	BreakStrToLowLevelDebugger(s)	DebugStr(s)
#define	BreakToSourceDebugger()			SysBreak()
#define	BreakStrToSourceDebugger(s)		SysBreakStr(s)
#define	BreakToCurrentDebugger()		SysBreak()
#define	BreakStrToCurrentDebugger(s)	SysBreakStr(s)

#endif

/* -------------------------------------------------------------------------- */

void UDebug::Break()
{
	BreakToCurrentDebugger();
}

void UDebug::Break(const Int8 msg[], ...)
{
	Uint8 str[256];

	va_list va;
	va_start(va, msg);
	str[0] = UText::FormatArg(str+1, sizeof(str)-1, msg, va);
	va_end(va);
	
	BreakStrToCurrentDebugger(str);
}

void UDebug::BreakAssembly()
{
	BreakToLowLevelDebugger();
}

void UDebug::BreakAssembly(const Int8 msg[], ...)
{
	Uint8 str[256];

	va_list va;
	va_start(va, msg);
	str[0] = UText::FormatArg(str+1, sizeof(str)-1, msg, va);
	va_end(va);
	
	BreakStrToLowLevelDebugger(str);
}

void UDebug::BreakSource()
{
	BreakToSourceDebugger();
}

void UDebug::BreakSource(const Int8 msg[], ...)
{
	Uint8 str[256];

	va_list va;
	va_start(va, msg);
	str[0] = UText::FormatArg(str+1, sizeof(str)-1, msg, va);
	va_end(va);
	
	BreakStrToSourceDebugger(str);
}

void UDebug::LogToDebugFile(const Int8[], ...)
{
	DebugBreak("UDebug::LogToDebugFile unimplemented");
}


#if DEBUG
void DebugBreak(const Int8 msg[], ...)
{
	Uint8 str[256];

	va_list va;
	va_start(va, msg);
	str[0] = UText::FormatArg(str+1, sizeof(str)-1, msg, va);
	va_end(va);
	
	BreakStrToCurrentDebugger(str);
}

void DebugLog(const Int8 msg[], ...)
{
	Uint8 str[256];

	va_list va;
	va_start(va, msg);
	str[0] = UText::FormatArg(str+1, sizeof(str)-1, msg, va);
	va_end(va);
	
	BreakStrToSourceDebugger(str);
}
void DebugLogFile(const Int8 inMsg[], ...)
{
	#pragma unused(inMsg)
	BreakStrToCurrentDebugger("\pDebugLogFile not implemented");
}
#endif




#endif /* MACINTOSH */
