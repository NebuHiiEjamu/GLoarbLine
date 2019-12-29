/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"

class UProgramCleanup
{
	public:
		static void InstallSystem(void (*inTermProc)());
		static void InstallAppl(void (*inTermProc)());
		
		static void CleanupAppl();
		
	private:
		class Cleanup;
		static UProgramCleanup::Cleanup gProgramCleanup;
};


