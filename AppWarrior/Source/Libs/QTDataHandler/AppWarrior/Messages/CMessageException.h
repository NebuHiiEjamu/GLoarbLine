// =====================================================================
//  CMessageException.h                  (C) Hotline Communications 1999
// =====================================================================
// Message problems, let the user try again if possible

#ifndef _H_CMessageException_
#define _H_CMessageException_

#include "CException.h"

HL_Begin_Namespace_BigRedH

class CMessageException : public CException
{
	public:
		enum ETask 
		{ 
			eCreateListener,
			eBroadcastMessage,
			eListenToMessage,
			eQueueMessage,
			eAddListener,
			eAddBroadcaster,
			eRemoveListener,
			eRemoveBroadcaster
		};
		
						CMessageException( 
									ETask inTask,
									AW_ErrorCode inAWCode _SOURCEARG_
									,OS_ErrorCode inOSCode = kNoOSError
					 				 )
							: CException( 
								inAWCode
					 			_SOURCECOPY_
								, inOSCode)
								, mTask( inTask )
				 				{} 

						CMessageException( 
									const CException &inExcept,
									ETask inTask)
							: CException(inExcept)
							, mTask (inTask)
								{}									
						CMessageException( const CMessageException &inExcept )
							: CException(inExcept)
							, mTask( inExcept.mTask )
								{} 

		ETask			GetTask()
							{ return mTask; }

	private:
		ETask			mTask;
};

#define THROW_MESSAGE_( inTask, inErr, osErr )\
	throw CMessageException( (CMessageException::inTask), (UAWError::inErr) _SOURCE_, (osErr) )
#define THROW_UNKNOWN_MESSAGE_( inTask )\
	THROW_MESSAGE_( inTask, eUnknownExceptionError, kNoOSError )
#define THROW_COPY_MESSAGE_( inBase, inTask )\
	throw CMessageException( (inBase), (CMessageException::inTask) )
#define THROW_OS_MESSAGE_( osErr, inTask )	{ OS_ErrorCode err = (osErr); \
	if(err != kNoOSError) THROW_MESSAGE_( inTask, eTranslate, err ); }

// This macro makes the default handling of exceptions easier
// Consider a standard Message call, that doesn't want special
// exception handling or remapping.  Do this;
//
// void MessageCall (void) {
//	try {
//		// Code goes here
//	} catch (...) {
//		RETHROW_MESSAGE_(whateverTask);
//	}
//}
//
// If you want special handlers, put them in before the catch (...) clause.
// Should be done with a function, but compilers aren't ANSI compliant, yet.
//
#define RETHROW_MESSAGE_(inTask) 			\
{											\
	try {									\
		throw;								\
	} catch (const CMessageException&) {	\
		throw;								\
	} catch (const CException& e) {			\
		THROW_COPY_MESSAGE_(e,inTask);		\
	} catch (...) {							\
		THROW_UNKNOWN_MESSAGE_(inTask);	\
	}										\
}											

HL_End_Namespace_BigRedH
#endif // _H_CMessageException_
