/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CTimerThread.h"


HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  CTimerThread                                                [public]
// ---------------------------------------------------------------------
// Constructor

CTimerThread::CTimerThread( CThread &inThread, UInt32 inWaitMilliSecs )
	: mThread(inThread), mWaitTime(inWaitMilliSecs)
{
}


// ---------------------------------------------------------------------
//  Run                                             [protected][virtual]
// ---------------------------------------------------------------------
// One pass thru the thread loop. Calls Quit to only execute once.

UInt32
CTimerThread::Run()
{
	Sleep( mWaitTime );
	mThread.Resume();
	Quit();
	return 0;
}

HL_End_Namespace_BigRedH
