/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// A File stream

#include "AW.h"
#include "CFileStreamBuffer.h"
#include "CFSLocalFileRefMac.h"
#include <Files.h>

HL_Begin_Namespace_BigRedH

class CFileStreamBuffer::CFileStreamBufferPS
{
	public:
		short			mFileRefNum;
		bool			mEOF;
};

// ---------------------------------------------------------------------
//  CFileStreamBuffer                                        	[public]
// ---------------------------------------------------------------------
// constructor

CFileStreamBuffer::CFileStreamBuffer()
: mOpenMode( eNONE ), mPlatform(*new CFileStreamBufferPS )
{
	mPlatform.mFileRefNum = 0;
	mPlatform.mEOF = false;
}

// ---------------------------------------------------------------------
//  ~CFileStreamBuffer                                 [public][virtual]
// ---------------------------------------------------------------------
// destructor

CFileStreamBuffer::~CFileStreamBuffer()
{
	try {
		if( IsOpen() ){
			Close();
		}
		delete &mPlatform;
	} catch( ... ){} // sorry, exceptions don't get out of here	
}

// ---------------------------------------------------------------------
//  Open			                                 			[public]
// ---------------------------------------------------------------------
// open the file

void
CFileStreamBuffer::Open( const CFSFileRef& inFileRef, OpenMode inMode )
{
	try
	{
		ASSERT(mPlatform.mFileRefNum == 0);
		ASSERT(inMode != eNONE);
		const CFSLocalFileRefMac &fileRef =
					dynamic_cast<const CFSLocalFileRefMac&>(inFileRef);
				
		mOpenMode = inMode;
		FSSpec theSpec = fileRef.GetFSpec();
		char openMode = (mOpenMode == eReadOnly) ? fsRdPerm : fsWrPerm;
		OSErr anErr = ::FSpOpenDF( &theSpec, openMode, &mPlatform.mFileRefNum );
		if( mOpenMode == eReadOnly ){
			THROW_OS_( anErr );
		} else if( anErr == fnfErr ){
			THROW_OS_( ::FSpCreate( &theSpec, fileRef.GetMacCreator(),
						fileRef.GetMacType(), smSystemScript ) );
			THROW_OS_( ::FSpOpenDF( &theSpec, openMode, &mPlatform.mFileRefNum ) );
		} else {
			THROW_OS_( anErr );
		}
		if( mOpenMode == eAppend ){	// go to the end of the file
			THROW_OS_( ::SetFPos( mPlatform.mFileRefNum, fsFromLEOF, 0 ) );
		} else if( mOpenMode == eWrite ){
			THROW_OS_( ::SetEOF( mPlatform.mFileRefNum, 0 ) );
		}	
		// send the apropriate notifications to start the listening devices
		if( GetAvailable() > 0 ){
			NotifyDataReceived();
		}
		if( mOpenMode != eReadOnly ){
			NotifyDataSent();
		}
	} catch( ... ){ 
		RETHROW_STREAM_( eOpen ); 
	}
}

// ---------------------------------------------------------------------
//  IsOpen			                             	   [public][virtual]
// ---------------------------------------------------------------------
// check if the file is open

bool
CFileStreamBuffer::IsOpen()
{
	return mPlatform.mFileRefNum != 0;
}

// ---------------------------------------------------------------------
//  Close			                             	   [public][virtual]
// ---------------------------------------------------------------------
// close the file

void
CFileStreamBuffer::Close()
{
	try {
		if( mPlatform.mFileRefNum != 0 ){
			THROW_OS_( ::FSClose( mPlatform.mFileRefNum ) );
			mPlatform.mFileRefNum = 0;
		}	
		NotifyStreamClosed( true );
	} catch( ... ){
		RETHROW_STREAM_( eClose );
	}
}

// ---------------------------------------------------------------------
//  Write			                             	   [public][virtual]
// ---------------------------------------------------------------------
// write a chunk of data to the file

UInt32
CFileStreamBuffer::Write( const void* inBuf, UInt32 inSize )
{
	SInt32 size = 0;
	try {
		ASSERT(mOpenMode != eReadOnly);
		size = inSize;
		THROW_OS_( ::FSWrite( mPlatform.mFileRefNum, &size, inBuf ) );
		ASSERT(inSize == size); // disk full ?
	} catch( ... ){
		RETHROW_STREAM_( eWrite );
	}
	return size;
}

// ---------------------------------------------------------------------
//  CanWriteSomeMore                            	   [public][virtual]
// ---------------------------------------------------------------------
// check if a future write is allowed or not

bool
CFileStreamBuffer::CanWriteSomeMore()
{
	ASSERT(mPlatform.mFileRefNum != 0);
	ASSERT(mOpenMode != eReadOnly);
	return true;
}

// ---------------------------------------------------------------------
//  Read			                            	   [public][virtual]
// ---------------------------------------------------------------------
// read a block of data from the file

UInt32
CFileStreamBuffer::Read( void* inBuf, UInt32 inSize )
{
	SInt32 size = 0;
	try {
		ASSERT(mOpenMode == eReadOnly);
		size = inSize;
		THROW_OS_( ::FSRead( mPlatform.mFileRefNum, &size, inBuf ) );
		mPlatform.mEOF = (size < inSize); // we reached EOF
	} catch( ... ){
		RETHROW_STREAM_( eRead );
	}
	return size;
}

// ---------------------------------------------------------------------
//  Read			                            	   [public][virtual]
// ---------------------------------------------------------------------
// How much more data is available in the file
// If the answer exceeds 4Gb then the return value will be 4Gb-1
// because we don't really care about the exact amount of data that is 
// available but rather if a specific amount is available or not.

UInt32
CFileStreamBuffer::GetAvailable()
{
	SInt32 size = 0;
	try {
		ASSERT(mPlatform.mFileRefNum != 0);
		SInt32 pos;
		THROW_OS_( ::GetEOF( mPlatform.mFileRefNum, &size ) );
		THROW_OS_( ::GetFPos( mPlatform.mFileRefNum, &pos ) );
		size -= pos;
		mPlatform.mEOF = (size == 0); // we reached EOF
	} catch( ... ){
		RETHROW_STREAM_( eCanRead );
	}

	return size;
}

// ---------------------------------------------------------------------
//  SetExpect		                            	   [public][virtual]
// ---------------------------------------------------------------------
// set the amount of data that has to be available before a DataReceived
// notification is sent
// Local files do not send notifications so we ignore this function call

void
CFileStreamBuffer::SetExpect(UInt32 inSize)
{
	//we don't do anything in here
}

// ---------------------------------------------------------------------
//  IsEOF			                            	   [public][virtual]
// ---------------------------------------------------------------------
// check if the file pointer has reached the end of file marker

bool
CFileStreamBuffer::IsEOF()
{
	ASSERT(mPlatform.mFileRefNum != 0);
	return mPlatform.mEOF;
}

HL_End_Namespace_BigRedH
