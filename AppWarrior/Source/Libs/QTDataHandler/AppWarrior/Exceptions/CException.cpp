/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CException.h"
#include "UStringConverter.h"

HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  CException                                                  [public]
// ---------------------------------------------------------------------
// Constructor

CException::CException( AW_ErrorCode inErr
		 			  #if DEBUG
		 			  , const char inFname[], int inLine
		 			  #endif
					  , OS_ErrorCode inCode )
	: mErrorCode(inErr)
	 ,mErrorCodeOS(inCode)
{
	#if DEBUG
		SetSource( inFname, inLine );
	#endif

	// do the translation from the OS error code if we are 
	// required to do so
	if (inErr == UAWError::eTranslate){
		mErrorCode = UAWError::TranslateErrorCode(inCode);
//		ASSERT( mErrorCode != UAWError::eNone );
	}
}					  


// ---------------------------------------------------------------------
//  CException                                                  [public]
// ---------------------------------------------------------------------
// Copy Constructor

CException::CException( const CException& inException )
	: mErrorCode(inException.mErrorCode)
	 ,mErrorCodeOS(inException.mErrorCodeOS)
	#if DEBUG
	 ,mSourceFile(inException.mSourceFile)
	 ,mSourceLine(inException.mSourceLine)
	#endif 
{
	
}					  


// ---------------------------------------------------------------------
//  ~CException                                        [public][virtual]
// ---------------------------------------------------------------------
// destructor

CException::~CException( )
{
}
	
#if 0
	#pragma mark -
#endif

// ---------------------------------------------------------------------
//  SetSource()                                                 [public]
// ---------------------------------------------------------------------
// Only exists in debug mode
// Sets the file and the line to their appropriate values
// Since __FILE__ is not unicode we have to use   char[]  as the arguments type
#if DEBUG		

void
CException::SetSource( const char inFname[], int inLine )
{
	mSourceFile = inFname;
	mSourceLine = UInt32(inLine);
} 

#endif // DEBUG



#if 0
	#pragma mark -
#endif

// ---------------------------------------------------------------------
//  GetMessage()                                       [public][virtual]
// ---------------------------------------------------------------------
// returns a human readable message for the error that occured

CString
CException::GetMessage() const
{
	CString msg;
	switch (mErrorCode)
	{
		default: 		 
			msg = L"AppWarriorBug - Unknown failure"; 
			break;
	}	

	//?? for the moment I format the message with a constant str
	#if DEBUG
		msg += L"\nSource file ";
		msg += mSourceFile.c_str();
		// +STR("\nSource line ")+mSourceLine
	#endif

	return	msg;
}

HL_End_Namespace_BigRedH
