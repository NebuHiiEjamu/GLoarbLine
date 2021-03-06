/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"

class UOperatingSystem
{
	public:
		static void Init();
		static bool InitQuickTime();
		
		static bool IsQuickTimeAvailable()			{	return mIsQuickTimeAvailable;		}
		static Uint8 *GetQuickTimeVersion()			{	return mQuickTimeVersion;			}
		static bool CanHandleFlash()				{	return mCanHandleFlash;				}
		
		static Uint8 *GetSystemVersion();
		
	protected:
		static bool mIsQuickTimeAvailable;
		static Str255 mQuickTimeVersion;
		static bool mCanHandleFlash;
};

typedef UOperatingSystem UOperSys;

