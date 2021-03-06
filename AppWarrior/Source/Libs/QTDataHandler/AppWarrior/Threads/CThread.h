// =====================================================================
//	CThread.h                            (C) Hotline Communications 1999
// =====================================================================
//
// Light weight process (thread) class

#ifndef _H_CThread_
#define _H_CThread_

#if PRAGMA_ONCE
	#pragma once
#endif

#if TARGET_OS_MAC
	#include <Threads.h>
	typedef ThreadID		TThreadID;
#elif TARGET_OS_WIN32
	typedef HANDLE			TThreadID;
#elif TARGET_OS_UNIX
	#include <pthread.h>
	typedef pthread_t		TThreadID;
#endif
#include "CThreadException.h"


HL_Begin_Namespace_BigRedH

class CThread
{
	public:
		enum EPriority {
			ePriorityLow,
			ePriorityMedium,
			ePriorityHigh
		};
		
							CThread( EPriority inPriority = ePriorityMedium );
		virtual void		Suspend();
		virtual void		Resume();

		virtual void		Terminate();

		static void			Sleep( UInt32 inSleepMilliSecs = 0 );

	protected:
		virtual 			~CThread();

		bool				IsTerminated()
								{ return mTerminated; }
		bool				IsRunning()
								{ return mRunning; }

		virtual UInt32		Execute();
		void				Quit()
								{ mRunning = false; }

			// Methods to override
		virtual void		ThreadEnter()
								{}
		virtual UInt32		Run() = 0; // 1 spin through the loop
		virtual void		ThreadExit( UInt32 inExitCode )
								{}

		bool				IsDone();

	private:
		friend class CThreadManager;

		static void			Initialize();
		static void			Preallocate( UInt32 inNumPrealloc );
		static TThreadID	GetCurrentID();
								// throws CThreadException

		void				Kill();

		TThreadID			mID;
		volatile bool		mRunning, mTerminated, mDone;
		
		#if TARGET_OS_MAC
			class CThreadTimer;
			CThreadTimer*	mTimer;
		#endif

		class CThreadLow;
		friend class CThreadLow;
};

HL_End_Namespace_BigRedH

#endif	// _H_CThread_
