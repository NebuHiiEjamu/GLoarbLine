// =====================================================================
//  CThreadManager.h                     (C) Hotline Communications 1999
// =====================================================================
//
// Singleton to track and manage all threads
//

#ifndef _H_CThreadManager_
#define _H_CThreadManager_

#if PRAGMA_ONCE
	#pragma once
#endif

#include <map>
#include <memory>
#include "CThread.h"
#include "CMutex.h"


HL_Begin_Namespace_BigRedH


class CThreadManager
{
	public:
		static CThreadManager&	Instance();
		static void				ClearInstance();

								~CThreadManager();

		void					Preallocate( UInt32 inNum );

		CThread*				GetCurrentThread();

			// ** Maintain **
		void					AddThread( CThread *inThread );
		void					RemoveThread( CThread *inThread );

			// ** Termination **
		void					TerminateAll();
		void					KillAll();

			// ** Cleanup **
		void					RunCleanup();

	private:
		static std::auto_ptr<CThreadManager>	sInstance; // Singleton

		typedef	std::map<TThreadID,CThread*>	TContainer;
		TContainer				mThreads;
		CMutex					mTLock;
		
								CThreadManager();
};

HL_End_Namespace_BigRedH
#endif	// _H_CThreadManager_
