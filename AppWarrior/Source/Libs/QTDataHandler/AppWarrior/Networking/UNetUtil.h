// =====================================================================
//  UNetUtil.h                           (C) Hotline Communications 1999
// =====================================================================
//
// Utility class to have various platform specific network calls

#ifndef _H_UNetUtil_
#define _H_UNetUtil_

#if PRAGMA_ONCE
	#pragma once
#endif

HL_Begin_Namespace_BigRedH

class UNetUtil
{
	public:
		static void		StartNetworking();
							// throws nothing
		static void		StopNetworking();
							// throws nothing
};

HL_End_Namespace_BigRedH

#endif	// _H_UNetUtil_
