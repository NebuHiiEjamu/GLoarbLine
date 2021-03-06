// =====================================================================
//  CMemoryException.h                 (C) Hotline Communications 1999
// =====================================================================
// Memory exceptions are thrown when a memory allocation fails.
// CMemoryException is derived from both CException and bad_alloc so
// both our code and the standard library functions can catch them and
// deal with them correctly

#ifndef _H_CMemoryException_
#define _H_CMemoryException_

#include "CException.h"
#include <new>

#if PRAGMA_ONCE
	#pragma once
#endif

HL_Begin_Namespace_BigRedH
class CMemoryException : public CException, public std::bad_alloc
{
	public:
		enum ETask
		{
			 eAllocatingMemory
		};
		
						CMemoryException( ETask inTask
								, AW_ErrorCode inAWCode
					 			_SOURCEARG_
								,OS_ErrorCode inOSCode = kNoOSError )
							: CException( inAWCode _SOURCECOPY_, inOSCode),
							mTask( inTask )
				 				{} 
						CMemoryException( const CMemoryException &inExcept )
							: CException(inExcept),
							mTask( inExcept.mTask )
								{} 

		ETask			GetTask()
							{ return mTask; }

	private:
		ETask			mTask;
}; // class CMemoryException

#define THROW_MEMORY_( inTask, inErr, osErr )\
	throw CMemoryException( (CMemoryException::inTask), (UAWError::inErr) _SOURCE_, (osErr) )
#define THROW_UNKNOWN_MEMORY_( inTask )\
	THROW_MEMORY_( inTask, eUnknownExceptionError, kNoOSError )
#define THROW_COPY_MEMORY_( inBase, inTask )\
	throw CMemoryException( (inBase), (CMemoryException::inTask) )
#define THROW_OS_MEMORY_( osErr, inTask )	{ OS_ErrorCode err = (osErr); \
	if(err != kNoOSError) THROW_MEMORY_( inTask, eTranslate, err ); }

// This macro makes the default handling of exceptions easier
// Consider a standard Message call, that doesn't want special
// exception handling or remapping.  Do this;
//
// void MessageCall (void) {
//	try {
//		// Code goes here
//	} catch (...) {
//		RETHROW_MEMORY_(whateverTask);
//	}
//}
//
// If you want special handlers, put them in before the catch (...) clause.
// Should be done with a function, but compilers aren't ANSI compliant, yet.
//
#define RETHROW_MEMORY_(inTask) 			\
{											\
	try {									\
		throw;								\
	} catch (const CMemoryException&) {		\
		throw;								\
	} catch (const CException& e) {			\
		THROW_COPY_MEMORY_(e,inTask);		\
	} catch (...) {							\
		THROW_UNKNOWN_MEMORY_(inTask);		\
	}										\
}											

HL_End_Namespace_BigRedH
#endif // _H_CMemoryException_
