/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CThread.h"
#include "CThreadManager.h"

//#define __C_RUNTIME_THREAD__

#ifdef __C_RUNTIME_THREAD__
#	include <Process.h>
#endif


HL_Begin_Namespace_BigRedH

class CThread::CThreadLow {
	public:
		static UInt32 WINAPI	Start( void *inArg );
};

// ---------------------------------------------------------------------
//  Start                                               [public][static]
// ---------------------------------------------------------------------

UInt32 WINAPI
CThread::CThreadLow::Start( void *inArg )
{
	UInt32 retValue = 0;

	if(inArg == nil)
	{
		return static_cast<UInt32>(-1);
	}

	CThread* volatile theThread	= reinterpret_cast<CThread*>(inArg);

	// Initialize
	try
 	{
//		theThread->mDone = true;
		theThread->ThreadEnter();
	}
	catch(...)
	{
		theThread->mDone = true;
		return static_cast<UInt32>(-1);
	}

	// Run it
	try
	{
		retValue = theThread->Execute();
	}
	catch(...)
	{
		theThread->mDone = true;
		return static_cast<UInt32>(-1);
	}

	// Exit
	try
	{
		if( !theThread->IsTerminated() )
		{
			theThread->ThreadExit( retValue );
		}
	}
	catch( ... )
	{
	}

	theThread->mDone = true;
	return retValue;
}


// ---------------------------------------------------------------------
//  CThread                                                     [public]
// ---------------------------------------------------------------------
// Constructor

CThread::CThread(EPriority inPriority) : mID(0), mRunning(false), mTerminated(false), mDone(false)
{
	// phase I(create)
#ifdef __C_RUNTIME_THREAD__
	typedef unsigned (__stdcall* _start)(void*);
	//
	unsigned id;
 	mID = (HANDLE)(_beginthreadex(0, 0, (_start)(&CThreadLow::Start), static_cast<void*>(this), CREATE_SUSPENDED, (unsigned*)(&id)));
#else
	DWORD id;
 	mID = ::CreateThread(0, 0, (LPTHREAD_START_ROUTINE)(&CThreadLow::Start), static_cast<void*>(this), CREATE_SUSPENDED, &id);
#endif

#ifdef __C_RUNTIME_THREAD__
 	if(mID == -1)
#else
 	if(mID == 0)
#endif
	{
		THROW_THREAD_(eCreating, eCreateThread, ::GetLastError());
	}

	// phase II(set priority)
#ifdef _DEBUG
	DWORD rc = ::GetLastError();
#endif

#ifdef __C_RUNTIME_THREAD__
	if((int)(mID) != -1)
#else
	if(mID != 0)
#endif
	{
		switch(inPriority)
		{
			case CThread::ePriorityLow:
				::SetThreadPriority(mID, THREAD_PRIORITY_BELOW_NORMAL);
				break;
			case CThread::ePriorityMedium:
				::SetThreadPriority(mID, THREAD_PRIORITY_NORMAL);
				break;
			case CThread::ePriorityHigh:
				::SetThreadPriority(mID, THREAD_PRIORITY_ABOVE_NORMAL);
				break;
		}
		
		mRunning = true;
		CThreadManager::Instance().AddThread(this);
	}
}


// ---------------------------------------------------------------------
//  ~CThread                                           [public][virtual]
// ---------------------------------------------------------------------
// Destructor

CThread::~CThread()
{
	if(mID != 0)
	{
	  ::CloseHandle(mID), mID = 0;
		CThreadManager::Instance().RemoveThread(this);
	}
}


// ---------------------------------------------------------------------
//  Sleep                                                       [public]
// ---------------------------------------------------------------------

void
CThread::Sleep( UInt32 inSleepMilliSecs )
{
	::Sleep(inSleepMilliSecs);
}


// ---------------------------------------------------------------------
//  Suspend                                                     [public]
// ---------------------------------------------------------------------

void
CThread::Suspend()
{
	ASSERT(mID != 0);
	if(mID != 0)
	{
		DWORD rc = ::SuspendThread(mID);

		if(rc == DWORD(-1))
		{
			THROW_OS_THREAD_(::GetLastError(), eResuming);
		}
	}
}


// ---------------------------------------------------------------------
//  Resume                                                      [public]
// ---------------------------------------------------------------------

void
CThread::Resume()
{
	ASSERT(mID != 0);
	if(mID != 0)
	{
		DWORD rc = ::ResumeThread(mID);

		if(rc == DWORD(-1))
		{
			THROW_OS_THREAD_(::GetLastError(), eResuming);
		}
	}
}


// ---------------------------------------------------------------------
//  Execute                                         [protected][virtual]
// ---------------------------------------------------------------------
// Execute the thread. Handle the looping here

UInt32
CThread::Execute()
{
	UInt32 retValue = 0;
	while( IsRunning() && !IsTerminated() ){
		retValue = Run();
	}
	return retValue;
}


// ---------------------------------------------------------------------
//  Terminate                                                   [public]
// ---------------------------------------------------------------------

void
CThread::Terminate()
{
	mTerminated = true;
}

// ---------------------------------------------------------------------
//  Initialize                                                 [private]
// ---------------------------------------------------------------------

void
CThread::Initialize()
{
	// not relevant
}

// ---------------------------------------------------------------------
//  Preallocate                                                [private]
// ---------------------------------------------------------------------

void
CThread::Preallocate( UInt32 inNumPrealloc )
{
	// not relevant
}

// ---------------------------------------------------------------------
//  Kill                                                       [private]
// ---------------------------------------------------------------------

void
CThread::Kill()
{
	ASSERT(mID != 0);
	if(mID != 0)
	{
		::TerminateThread(mID,  static_cast<UInt32>(-1));
	}
}


// ---------------------------------------------------------------------
//  IsDone                                                     [private]
// ---------------------------------------------------------------------

bool
CThread::IsDone()
{
	return mDone;
	/*
	if(mDone && mID != 0)
	{
		DWORD rc;
		return ::GetExitCodeThread(mID, &rc) != STILL_ACTIVE;
	}
	else
	{
		return false;
	}
	*/
}


// ---------------------------------------------------------------------
//  GetCurrentID                                       [private][static]
// ---------------------------------------------------------------------
// Get the currenttly executing thread

TThreadID
CThread::GetCurrentID()
{
	return ::GetCurrentThread();
}


HL_End_Namespace_BigRedH

