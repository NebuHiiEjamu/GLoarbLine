// =====================================================================
//  CThreadException.h                   (C) Hotline Communications 1999
// =====================================================================
// File problems , let the user try again if possible

#ifndef _H_CThreadException_
#define _H_CThreadException_

#include "CException.h"

#if PRAGMA_ONCE
	#pragma once
#endif				   

HL_Begin_Namespace_BigRedH

class CThreadException : public CException
{
	public:
		enum ETask 
		{ 
			eCreating,
			eDeleting,
			eSuspending,
			eResuming,
			eSleeping,
			eIniting,
			eQuerying,
			eCreatingMutex,
			eCreatingSemaphore
		};
		
						CThreadException( ETask inTask,
									AW_ErrorCode inAWCode _SOURCEARG_
									,OS_ErrorCode inOSCode = kNoOSError )
							: CException( inAWCode _SOURCECOPY_, inOSCode)
							, mTask( inTask )
				 				{} 
						CThreadException( const CException &inExcept, ETask inTask )
							: CException(inExcept)
							, mTask( inTask )
								{} 
						CThreadException( const CThreadException &inExcept )
							: CException(inExcept)
							, mTask( inExcept.mTask )
								{} 

		ETask			GetTask()
							{ return mTask; }

	private:
		ETask			mTask;
};

#define THROW_THREAD_( inTask, inErr, osErr )\
	throw CThreadException( (CThreadException::inTask), (UAWError::inErr) _SOURCE_, (osErr) )
#define THROW_UNKNOWN_THREAD_( inTask )\
	THROW_THREAD_( inTask, eUnknownExceptionError, kNoOSError )
#define THROW_COPY_THREAD_( inBase, inTask )\
	throw CThreadException( inBase, CThreadException::inTask )
#define THROW_OS_THREAD_( osErr, inTask )	{ OS_ErrorCode err = (osErr); \
	if(err != kNoOSError) THROW_THREAD_( inTask, eTranslate, err ); }

// This macro makes the default handling of exceptions easier
// Consider a standard Message call, that doesn't want special
// exception handling or remapping.  Do this;
//
// void MessageCall (void) {
//	try {
//		// Code goes here
//	} catch (...) {
//		RETHROW_THREAD_(whateverTask);
//	}
//}
//
// If you want special handlers, put them in before the catch (...) clause.
// Should be done with a function, but compilers aren't ANSI compliant, yet.
//
#define RETHROW_THREAD_(inTask) 			\
{											\
	try {									\
		throw;								\
	} catch (const CThreadException&) {		\
		throw;								\
	} catch (const CException& e) {			\
		THROW_COPY_THREAD_(e,inTask);		\
	} catch (...) {							\
		THROW_UNKNOWN_THREAD_(inTask);		\
	}										\
}											

HL_End_Namespace_BigRedH
#endif // _H_CThreadException_
