// =====================================================================
//  CPersistenceException.h              (C) Hotline Communications 1999
// =====================================================================
// File problems , let the user try again if possible

#ifndef _H_CPersistenceException_
#define _H_CPersistenceException_

#include "CException.h"

#if PRAGMA_ONCE
	#pragma once
#endif				   

HL_Begin_Namespace_BigRedH

class CPersistenceException : public CException
{
	public:
		enum 			ETask 
							{ 
								  eCreateObject, eGetObject
								, eCreateFactory, eDeleteFactory
								, eRegister, eUnregister
								, eCreateCache, eDeleteCache
							};
		
						CPersistenceException( ETask inTask
									  		 , AW_ErrorCode inAWCode
							 		  		 _SOURCEARG_
									  		 , OS_ErrorCode inOSCode = kNoOSError )
							// throws ???
							: CException( inAWCode _SOURCECOPY_, inOSCode)
							, mTask( inTask )
			 				{} 
						CPersistenceException( const CException &inExcept
									  		 , ETask inTask )
							// throws ???
							: CException(inExcept)
							, mTask( inTask )
							{} 
						CPersistenceException( const CPersistenceException &inExcept )
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

#define THROW_PERSISTENCE_( inTask, inErr, osErr )\
	throw CPersistenceException( (CPersistenceException::inTask), (UAWError::inErr) _SOURCE_, (osErr) )
#define THROW_UNKNOWN_PERSISTENCE_( inTask )\
	THROW_PERSISTENCE_( inTask, eUnknownExceptionError, kNoOSError )
#define THROW_COPY_PERSISTENCE_( inBase, inTask )\
	throw CPersistenceException( inBase, CPersistenceException::inTask )
#define THROW_OS_PERSISTENCE_( osErr, inTask )	{ OS_ErrorCode err = (osErr); \
	if(err != kNoOSError) THROW_PERSISTENCE_( inTask, eTranslate, err ); }

#define RETHROW_PERSISTENCE_(inTask) 			\
	{try {	throw;								\
	} catch (const CPersistenceException&) {	\
		throw;									\
	} catch (const CException& e) {				\
		THROW_COPY_PERSISTENCE_(e,inTask);		\
	} catch (...) {								\
		THROW_UNKNOWN_PERSISTENCE_(inTask);		\
	}}

HL_End_Namespace_BigRedH
#endif // _H_CPersistenceException_
