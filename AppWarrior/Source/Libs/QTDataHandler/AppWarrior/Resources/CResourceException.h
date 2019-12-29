// =====================================================================
//  CResourceException.h                 (C) Hotline Communications 2000
// =====================================================================
// File problems , let the user try again if possible

#ifndef _H_CResourceException_
#define _H_CResourceException_

#include "CException.h"

#if PRAGMA_ONCE
	#pragma once
#endif				   

HL_Begin_Namespace_BigRedH

class CResourceException : public CException
{
	public:
		enum ETask 
		{ 
			  eOpen, eClose
			, eGet
		};
		
						CResourceException( ETask inTask
										, AW_ErrorCode inAWCode
							 			  _SOURCEARG_
										, OS_ErrorCode inOSCode = kNoOSError )
							// throws ???
							: CException( inAWCode _SOURCECOPY_, inOSCode)
							, mTask( inTask )
			 				{} 
						CResourceException( const CException &inExcept
										, ETask inTask )
							// throws ???
							: CException(inExcept)
							, mTask( inTask )
							{} 
						CResourceException( const CResourceException &inExcept )
							// throws ???
							: CException(inExcept)
							, mTask( inExcept.mTask )
							{} 

		ETask			GetTask()
							// throws nothing
							{ return mTask; }

	private:
		ETask			mTask;
};

#define THROW_RESOURCE_( inTask, inErr, osErr )\
	throw CResourceException( (CResourceException::inTask), (UAWError::inErr) _SOURCE_, (osErr) )
#define THROW_UNKNOWN_RESOURCE_( inTask )\
	THROW_RESOURCE_( inTask, eUnknownExceptionError, kNoOSError )
#define THROW_COPY_RESOURCE_( inBase, inTask )\
	throw CResourceException( inBase, CResourceException::inTask )
#define THROW_OS_RESOURCE_( osErr, inTask )	{ OS_ErrorCode err = (osErr); \
	if(err != kNoOSError) THROW_RESOURCE_( inTask, eTranslate, err ); }

#define RETHROW_RESOURCE_(inTask) 			\
{											\
	try {									\
		throw;								\
	} catch (const CResourceException&) {	\
		throw;								\
	} catch (const CException& e) {			\
		THROW_COPY_RESOURCE_(e,inTask);		\
	} catch (...) {							\
		THROW_UNKNOWN_RESOURCE_(inTask);	\
	}										\
}											

HL_End_Namespace_BigRedH
#endif // _H_CResourceException_
