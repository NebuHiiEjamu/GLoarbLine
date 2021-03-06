#if WIN32

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UTimer.h"

struct STimer {
	STimer *next;
	Uint32 winTimer;
	TMessageProc msgProc;
	void *msgProcContext;
	bool onceOnly;
};

#define REF		((STimer *)inRef)

static VOID CALLBACK _APTimerProc(HWND, UINT, UINT, DWORD);

static STimer *_APFirstTimer = nil;

/* -------------------------------------------------------------------------- */

void UTimer::Init()
{
	// nothing to do
}

TTimer UTimer::New(TMessageProc inProc, void *inContext)
{
	STimer *tm = (STimer *)UMemory::NewClear(sizeof(STimer));
	
	tm->msgProc = inProc;
	tm->msgProcContext = inContext;
	
	tm->next = _APFirstTimer;
	_APFirstTimer = tm;

	return (TTimer)tm;
}

TTimer UTimer::StartNew(TMessageProc inProc, void *inContext, Uint32 inMillisecs, Uint32 inIsRepeating)
{
	STimer *tm = (STimer *)UMemory::NewClear(sizeof(STimer));
	
	tm->msgProc = inProc;
	tm->msgProcContext = inContext;
	tm->onceOnly = (inIsRepeating == kOnceTimer);
	
	tm->winTimer = ::SetTimer(NULL, 1, inMillisecs, (TIMERPROC)_APTimerProc);
	
	if (tm->winTimer == 0)
	{
		UMemory::Dispose((TPtr)tm);
		Fail(errorType_Memory, memError_NotEnough);
	}
	
	tm->next = _APFirstTimer;
	_APFirstTimer = tm;

	return (TTimer)tm;
}

void UTimer::Dispose(TTimer inRef)
{
	if (inRef)
	{
		if (REF->winTimer) ::KillTimer(NULL, REF->winTimer);

		// remove from our queue
		STimer *tm = _APFirstTimer;
		STimer *ptm = nil;
		while (tm)
		{
			if (tm == REF)
			{
				if (ptm)
					ptm->next = tm->next;
				else
					_APFirstTimer = tm->next;
				break;
			}
			ptm = tm;
			tm = tm->next;
		}
		
		UApplication::FlushMessages(REF->msgProc, REF->msgProcContext, inRef);
		UMemory::Dispose((TPtr)inRef);
	}
}

void UTimer::Start(TTimer inRef, Uint32 inMillisecs, Uint32 inIsRepeating)
{
	Require(inRef);
	
	if (REF->winTimer) ::KillTimer(NULL, REF->winTimer);
	
	REF->onceOnly = (inIsRepeating == kOnceTimer);

	REF->winTimer = ::SetTimer(NULL, 1, inMillisecs, (TIMERPROC)_APTimerProc);

	if (REF->winTimer == 0)
		Fail(errorType_Memory, memError_NotEnough);
}

bool UTimer::WasStarted(TTimer inRef)
{
	if (REF->winTimer)
		return true;
		
	return false;
}

void UTimer::Stop(TTimer inRef)
{
	if (inRef && REF->winTimer)
	{
		::KillTimer(NULL, REF->winTimer);
		REF->winTimer = 0;
	}
}

// simulate the timer going off (does not affect time remaining etc)
void UTimer::Simulate(TTimer inRef)
{
	if (inRef) UApplication::ReplaceMessage(msg_Timer, nil, 0, priority_Timer, REF->msgProc, REF->msgProcContext, inRef);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static VOID CALLBACK _APTimerProc(HWND, UINT, UINT inID, DWORD)
{
	STimer *tm = _APFirstTimer;
	
	while (tm)
	{
		if (tm->winTimer == inID)
		{
			try {
				UApplication::ReplaceMessage(msg_Timer, nil, 0, priority_Timer, tm->msgProc, tm->msgProcContext, tm);
			} catch(...) {}
			if (tm->onceOnly)
			{
				::KillTimer(NULL, tm->winTimer);
				tm->winTimer = 0;
			}
			break;
		}
		tm = tm->next;
	}
}






#endif /* WIN32 */
