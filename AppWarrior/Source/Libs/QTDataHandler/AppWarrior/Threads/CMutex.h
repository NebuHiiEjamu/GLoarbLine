// =====================================================================
//  CMutex.h                             (C) Hotline Communications 1999
// =====================================================================
// Mutual-Exclusive Semaphore - only 1 holder at a time

#ifndef _H_CMutex_
#define _H_CMutex_

#if PRAGMA_ONCE
	#pragma once
#endif

HL_Begin_Namespace_BigRedH

class CMutex
{
	public:
							CMutex();
							// throws CMemoryException
							~CMutex();

		// ** Protocol **
		void				Acquire();
							// throws nothing
		bool				TryAcquire();
							// throws nothing
		void				Release();
							// throws nothing

	private:
		class CMutexPS;
		CMutexPS			*mPlatform;
};

HL_End_Namespace_BigRedH

#endif // _H_CMutex_
