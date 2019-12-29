/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#ifndef _H_AWMacros_
#define _H_AWMacros_

#if   defined(__MWERKS__)
	#define COMPILER_CW 1
#elif defined(_MSC_VER)
	#define COMPILER_MSVC 1
#elif defined(__GNUC__)
	#define COMPILER_GCC 1
#else	
	#pragma message("Unknown compiler, please define!")
#endif	


#if 0
	#pragma mark
#endif	


// to work properly 3rd party headers need 
// the definitions undefined instead of zeroed

#if COMPILER_CW && macintosh
	#include <ConditionalMacros.h>
#elif COMPILER_CW && __INTEL__
	//#define TARGET_OS_MAC				0
	#define TARGET_OS_WIN32				1
	//#define TARGET_OS_UNIX				0
	//#define TARGET_CPU_PPC  			0
	//#define TARGET_CPU_68K  			0
	#define TARGET_CPU_X86  			1
	//#define TARGET_CPU_MIPS 			0
	//#define TARGET_CPU_SPARC			0		
	//#define TARGET_CPU_ALPHA			0
	#define TARGET_RT_LITTLE_ENDIAN		1
	//#define TARGET_RT_BIG_ENDIAN		0
	#define PRAGMA_ONCE					1
	#define TYPE_LONGLONG				1
	#define TYPE_BOOL					1
	//#define PRAGMA_IMPORT				0
	//#define PRAGMA_STRUCT_ALIGN			0
	#define PRAGMA_STRUCT_PACK			1
	#define PRAGMA_STRUCT_PACKPUSH		1
	//#define PRAGMA_ENUM_PACK			0
	#define PRAGMA_ENUM_ALWAYSINT		1
	//#define PRAGMA_ENUM_OPTIONS			0
#elif COMPILER_MSVC && defined(_WIN32)
	//#define TARGET_OS_MAC				0
	#define TARGET_OS_WIN32				1
	//#define TARGET_OS_UNIX				0
	//#define TARGET_CPU_PPC  			0
	//#define TARGET_CPU_68K  			0
	#define TARGET_CPU_X86  			1
	//#define TARGET_CPU_MIPS 			0
	//#define TARGET_CPU_SPARC			0		
	//#define TARGET_CPU_ALPHA			0
	#define TARGET_RT_LITTLE_ENDIAN		1
	//#define TARGET_RT_BIG_ENDIAN		0
	#if _MSC_VER > 1000
		#define PRAGMA_ONCE				1
	#else	
		//#define PRAGMA_ONCE				0
	#endif	
	//#define TYPE_LONGLONG				0
	#define TYPE_BOOL					1
	//#define PRAGMA_IMPORT				0
	//#define PRAGMA_STRUCT_ALIGN			0
	#define PRAGMA_STRUCT_PACK			1
	#define PRAGMA_STRUCT_PACKPUSH		1
	//#define PRAGMA_ENUM_PACK			0
	//#define PRAGMA_ENUM_ALWAYSINT		0
	//#define PRAGMA_ENUM_OPTIONS			0
#elif COMPILER_MSVC && defined(_M_ALPHA)
	//#define TARGET_OS_MAC				0
	#define TARGET_OS_WIN32				1
	//#define TARGET_OS_UNIX				0
	//#define TARGET_CPU_PPC  			0
	//#define TARGET_CPU_68K  			0
	//#define TARGET_CPU_X86  			0
	//#define TARGET_CPU_MIPS 			0
	//#define TARGET_CPU_SPARC			0
	#define TARGET_CPU_ALPHA			1
	#define TARGET_RT_LITTLE_ENDIAN		1
	//#define TARGET_RT_BIG_ENDIAN		0
	//#define TARGET_RT_MAC_CFM			0
	//#define TARGET_RT_MAC_68881			0
	#if _MSC_VER > 1000
		#define PRAGMA_ONCE				1
	#else	
		//#define PRAGMA_ONCE				0
	#endif	
	//#define TYPE_LONGLONG				0
	#define TYPE_BOOL					1
	//#define PRAGMA_IMPORT				0
	//#define PRAGMA_STRUCT_ALIGN			0
	#define PRAGMA_STRUCT_PACK			1
	#define PRAGMA_STRUCT_PACKPUSH		1
	//#define PRAGMA_ENUM_PACK			0
	//#define PRAGMA_ENUM_ALWAYSINT		0
	//#define PRAGMA_ENUM_OPTIONS			0
#elif COMPILER_GCC && defined(__linux__)
	//#define TARGET_OS_MAC				0
	//#define TARGET_OS_WIN32				0
	#define TARGET_OS_UNIX				1
	//#define TARGET_CPU_PPC  			0
	//#define TARGET_CPU_68K  			0
	#define TARGET_CPU_X86  			1
	//#define TARGET_CPU_MIPS 			0
	//#define TARGET_CPU_SPARC			0		
	//#define TARGET_CPU_ALPHA			0
	#define TARGET_RT_LITTLE_ENDIAN		1
	//#define TARGET_RT_BIG_ENDIAN		0
	//#define PRAGMA_ONCE					0
	#define TYPE_LONGLONG				1
	#define TYPE_BOOL					1
	//#define PRAGMA_IMPORT				0
	//#define PRAGMA_STRUCT_ALIGN			0
	#define PRAGMA_STRUCT_PACK			1
	//#define PRAGMA_STRUCT_PACKPUSH		0
	//#define PRAGMA_ENUM_PACK			0
	//#define PRAGMA_ENUM_ALWAYSINT		0
	//#define PRAGMA_ENUM_OPTIONS			0
#endif	


#if PRAGMA_ONCE
	#pragma once
#endif		 			 

#ifndef BUILDING_APP_
	// Default to ON
	//?? hack allow using HotSpring in non-apps
	#define BUILDING_APP_ 1
#endif

#ifndef DEBUG
	// Default to OFF
	#define DEBUG 0
	#pragma message("DEBUG not defined setting to 0")
#endif

#if 0
	#pragma mark NAMESPACE
#endif

#ifndef HL_Uses_BigRedH_Namespace
	// Default to OFF
	#define HL_Uses_BigRedH_Namespace 0  
	#pragma message("HL_Uses_BigRedH_Namespace not defined, setting to 0")
#endif

#if HL_Uses_BigRedH_Namespace
	#define HL_Begin_Namespace_BigRedH namespace BigRedH { 
	#define HL_End_Namespace_BigRedH  }
	#define HL_Using_Namespace_BigRedH using namespace BigRedH;
	#define HL_BigRedH     BigRedH::
#else
	#define HL_Begin_Namespace_BigRedH
	#define HL_End_Namespace_BigRedH
	#define HL_Using_Namespace_BigRedH
	#define HL_BigRedH ::
#endif

#if COMPILER_CW
	#define __asm asm
#endif
		// the size of the parameter in bits (ie bytes*8) 
#define BITSIZE(x) (sizeof(x)<<3)

#if DEBUG
	#define ASSERT(X)	do {if (!(X)) { UDebug::Message( "Assert(" #X "): File - '%s', Line - %d", __FILE__, __LINE__ ); UDebug::Breakpoint();}} while(false)
#else
	#define ASSERT(X)	0
#endif

#endif // _H_AWMacros_
