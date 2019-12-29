/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#ifndef _H_AWTypes_
#define _H_AWTypes_

#if PRAGMA_ONCE
	#pragma once
#endif

#include "CString.h"
		 		 
HL_Begin_Namespace_BigRedH

typedef unsigned long 		UInt32;
typedef signed long 		SInt32;
typedef unsigned short 		UInt16;
typedef signed short 		SInt16;
typedef unsigned char 		UInt8;
typedef signed char			SInt8;

#if defined(_MSC_VER) && !defined(__MWERKS__) && defined(_M_IX86) && defined(__cplusplus) && (_MSC_VER >= 1100)
	// MSVC knows 64 bit as __int64
	typedef unsigned __int64	UInt64;
	typedef signed __int64		SInt64;
#elif defined(TYPE_LONGLONG)
	typedef unsigned long long	UInt64;
	typedef signed long long	SInt64;
#else
	// others might not know
	typedef unsigned long 		UInt64[2];
	typedef signed long			SInt64[2];
#endif

#ifndef TYPE_BOOL
	typedef SInt8 			bool;
	#ifndef false
		#define false		bool(0!=0)
	#endif
	#ifndef true	
		#define true		bool(0==0)
	#endif	
#endif

// Errors
#if TARGET_OS_MAC
	typedef SInt32			OS_ErrorCode;	// Mac OS specific error code
	const OS_ErrorCode		kNoOSError = noErr;
#elif TARGET_OS_WIN32
	typedef DWORD			OS_ErrorCode;	// Win32 OS specific error code
	const OS_ErrorCode		kNoOSError = ERROR_SUCCESS;
#elif TARGET_OS_UNIX
	typedef UInt32			OS_ErrorCode;	// Un*x OS specific error code
	const OS_ErrorCode		kNoOSError = 0;
#endif	

// Strings
typedef	UInt32				FourCharCode;

#ifndef nil
	#define nil 0
#endif

#ifndef null
	#define null nil
#endif

// Menus
typedef SInt32 CommandT;
typedef UInt16 ModifierKeys;
typedef SInt16 MenuID;
typedef SInt16 MenuGroupID;

#ifndef OFFSET_OF
#define OFFSET_OF(T, member)		((UInt32)&(((T *)0)->member))
#endif

#undef abs
template <class T>
inline T abs (T i)
{
	return (i < 0) ? -i : i;
}

HL_End_Namespace_BigRedH
#endif //_H_AWTypes_
