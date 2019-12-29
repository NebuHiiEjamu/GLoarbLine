// =====================================================================
//  CGraphicsException.h                 (C) Hotline Communications 1999
// =====================================================================
// Graphics problems , let the user try again if possible

#ifndef _H_CGraphicsException_
#define _H_CGraphicsException_

#include "CException.h"

HL_Begin_Namespace_BigRedH

class CGraphicsException : public CException
{
	public:
		enum ETask 
		{ 
			eNoTask
			,eDrawImage
			,eAlphaMaskImage
			,eAlphaDrawImage
			,eColorKeyedDrawImage
			,eSetClipping
			,eGetClipping
			,eImageCreate
			,eGraphicsPortCreate
			,eGetAlphaMask
			,eSolidFill
			,eGetPixel
			,eSetPixel
			,eDrawLine
			,eDrawRect
			,eFillRect
			,eDrawOval
			,eFillOval
			,eTileDrawImage
			,eTileAlphaMaskImage
			,eTileAlphaDrawImage
			,eTileColorKeyedDrawImage
			,eApplyImage
			,eFillPolygon
			,eDrawText
			,eFontConstructor
			,eFontGetMetrics
			,eFontMeasureText
			,eFontRealization 			// Converting a CFont into OS structures...
		};
		
						CGraphicsException( 
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

						CGraphicsException( 
									const CException &inExcept,
									ETask inTask)
							: CException(inExcept)
							, mTask (inTask)
								{}									
						CGraphicsException( const CGraphicsException &inExcept )
							: CException(inExcept)
							, mTask( inExcept.mTask )
								{} 

		ETask			GetTask()
							{ return mTask; }

	private:
		ETask			mTask;
};

#define THROW_GRAPHICS_( inTask, inErr, osErr )\
	throw CGraphicsException( (CGraphicsException::inTask), (UAWError::inErr) _SOURCE_, (osErr) )
#define THROW_UNKNOWN_GRAPHICS_( inTask )\
	THROW_GRAPHICS_( inTask, eUnknownExceptionError, kNoOSError )
#define THROW_COPY_GRAPHICS_( inBase, inTask )\
	throw CGraphicsException( (inBase), (CGraphicsException::inTask) )
#define THROW_OS_GRAPHICS_( osErr, inTask )	{ OS_ErrorCode err = (osErr); \
	if(err != kNoOSError) THROW_GRAPHICS_( inTask, eTranslate, err ); }

// This macro makes the default handling of exceptions easier
// Consider a standard graphics call, that doesn't want special
// exception handling or remapping.  Do this;
//
// void GraphicsCall (void) {
//	try {
//		// Code goes here
//	} catch (...) {
//		RETHROW_GRAPHICS_(whateverTask);
//	}
//}
//
// If you want special handlers, put them in before the catch (...) clause.
// Should be done with a function, but compilers aren't ANSI compliant, yet.
//
#define RETHROW_GRAPHICS_(inTask) 			\
{											\
	try {									\
		throw;								\
	} catch (const CGraphicsException&) {	\
		throw;								\
	} catch (const CException& e) {			\
		THROW_COPY_GRAPHICS_(e,inTask);		\
	} catch (...) {							\
		THROW_UNKNOWN_GRAPHICS_(inTask);	\
	}										\
}											

HL_End_Namespace_BigRedH
#endif // _H_CGraphicsException_
