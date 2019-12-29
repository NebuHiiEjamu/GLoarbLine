/*
 * UDebug.h
 * (c)1998 Hotline Communications
 */

#pragma once
#include "typedefs.h"

class UDebug
{
	public:
		// break to current debugger
		static void Break();
		static void Break(const Int8 inMessage[], ...);
		
		// break to low-level debugger
		static void BreakAssembly();
		static void BreakAssembly(const Int8 inMessage[], ...);
		
		// break to source debugger
		static void BreakSource();
		static void BreakSource(const Int8 inMessage[], ...);

		static void LogToDebugFile(const Int8[], ...);
};

// break to current debugger only when compiled for debugging
#if DEBUG
	void DebugBreak(const Int8 inMessage[], ...);
	void DebugLog(const Int8 inMessage[], ...);
	void DebugLogFile(const Int8 inMessage[], ...);
#else
	inline void DebugBreak(const Int8[], ...) {}
	inline void DebugLog(const Int8[], ...) {}
	inline void DebugLogFile(const Int8[], ...) {}
#endif
