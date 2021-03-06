/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
//
//  Implementation of CDataProvider class
//  (note that it knows nothing about QuickTime)


//-----------------------------------------------------------------------
// includes

#include "aw.h"
#include "CDataProvider.h"
#include "CDataHandlerComponent.h"
#include "CFSLocalFileRef.h"
#include "UStringConverter.h"
#include "CFSException.h"
#include "CDateTime.h"
#include "StString.h"
#include "UFileUtils.h"
#include "HL_Handler.h"

//-----------------------------------------------------------------------

HL_Begin_Namespace_BigRedH



// ---------------------------------------------------------------------
//  CDataProvider											 [protected]
// ---------------------------------------------------------------------
// constructor, opens a connection according to given parameters
CDataProvider::CDataProvider(
		CString&	inServer, 
		CString&	inFileName,
		UInt16		inPort, 
		UInt32		inFileSize, 
		UInt32		inRefNum, 
		CString&	inTempFileName,
		CString&	inMimeType,
		UInt32		inFileType):
	CListener(nil),
	CThread(),
	mStream(eTCPTimeOut/1000),
	mServer(inServer),
	mFileName(inFileName),
	mPort(inPort),
	mRefNum(inRefNum),
	mFileSize(inFileSize),
	mAvailableSize(0),
	mTempFileName(inTempFileName),
	mMimeType(inMimeType),
	mFileType(inFileType),
	mLocalFile(nil),
	mUnflat(nil),
	mLastError(UAWError::eNone),
	mRefCount(0),
	mReading(0),
	mCancelRead(false)
{
	mTempFileRef.reset(nil);

	try 
	{
		CNetAddress address(mServer, mPort);  // throws exception if cannot resolve

		mStream.Open(address); // throws exception after timeout

		if (mStream.IsOpen())
		{
			// send a request 
			mStream << (UInt32)'HTXF'; 
			mStream << mRefNum;
			mStream << (UInt32)0;
			mStream << (UInt32)0;

			mUnflat = new CUnflattenFile(UFileUtils::GetTemporaryFolder(), mTempFileName); 
			mUnflat->AddListener(*this);

			// get a data using another thread
			Resume();
		}

	}
	catch ( const CException &e ) // no exceptions outside the constructor
	{
		mLastError = e.GetErrorCode();
		delete mUnflat;
		mUnflat = nil;
		StopListening();
	} 
	catch (...) // no exceptions outside the constructor
	{
		delete mUnflat;
		mUnflat = nil;
		StopListening();
	} 
	UDebug::Message("!!!Provider Created\n");
}


UInt32
CDataProvider::AddRef()
{
	return ++mRefCount;
}



UInt32
CDataProvider::Release()
{
	if (--mRefCount==0)
	{
		CDataHandlerComponent::Instance().DeleteProvider(this);
		delete this;
		return 0;
	}
	return mRefCount;
}


// ---------------------------------------------------------------------
//  ~CDataProvider										    [public]
// ---------------------------------------------------------------------
// destructor
CDataProvider::~CDataProvider()
{
	UDebug::Message("!!!Provider Deleted\n");

	// wait until Run() thread is finished, then stop the thread
	Quit();

	// Spin until thread terminates
	while( !IsDone() ){
		CThread::Sleep();
	}

	delete mUnflat,
	mUnflat = nil;

	if (mStream.IsOpen())
		mStream.Close();

	delete mLocalFile,
	mLocalFile = nil;

	if (mTempFileRef.get()!=nil)
	{
		try	
		{
			mTempFileRef->Delete();
		}
		catch (...) {}
	}
}



// ---------------------------------------------------------------------
//  IsOpen													    [public]
// ---------------------------------------------------------------------
// shows open status
// We say 'yes' even if we don't have a local file yet
bool 
CDataProvider::IsOpen()
{
	return mLocalFile!=nil || mStream.IsOpen();
}




// ---------------------------------------------------------------------
//  Run														 [protected]
// ---------------------------------------------------------------------
// threaded getting data from the TCP stream and putting it into unflattener
UInt32
CDataProvider::Run()
{
	const UInt32 kBufSize = 32768;
	static UInt8 buf[kBufSize];
	UInt32 read=0, theAvail, readSize;
	StPStyleString filename( mTempFileName );

	// read data
	try 
	{
		while( IsRunning() && ((theAvail = mStream.GetAvailable()) > 0) ){
			if( mLocalFile == nil ){
				read = mStream.Read(buf, 256);
			} else {
				if( theAvail < kBufSize ){
					readSize = theAvail;
				} else {
					readSize = kBufSize;
				}
				read = mStream.Read( buf, readSize );
			}
			if( IsRunning() ){
				mUnflat->ProcessData(buf, read);
				HL_Progress( filename, GetFullSize(), GetAvailableSize() );
				CThread::Sleep();
			}
		}
		if( IsRunning() && GetFullSize() == GetAvailableSize() ){
			mStream.Close();
			Quit();
		}
	}
	catch ( const CException &e) 
	{
		mLastError = e.GetErrorCode();
		delete mUnflat;
		mUnflat = nil;
		try {
			mStream.Close();
		} catch( ... ){}
		Quit();
	}
	catch (...) 
	{
		delete mUnflat;
		mUnflat = nil;
		try {
			mStream.Close();
		} catch( ... ){}
		Quit();
	}

	CThread::Sleep(0); // yield for Mac threads

	return 0;
}


// ---------------------------------------------------------------------
//  Read													    [public]
// ---------------------------------------------------------------------
// clients use this function to get data synchronously.
// If a connection requires an asynchronous mode, it should use
// CAsyncReader instead, which internally uses CDataProvider::Read
bool 
CDataProvider::Read(void* inPtr, UInt32 inFileOffset, UInt32& ioDataSize)
{
	bool result = false;
	
	if( mCancelRead )
		return false;
	
	AddRef();
	mReading++;

	// if the local file is not open yet, probably
	// the unflatten object haven't got enough data to create it.
	// We'll wait a little, then return if it is still unavailable
	if ( !mLocalFile && IsRunning())
	{
		CDateTime endTime, currTime;
		endTime.AddSeconds( eUnflatTimeOut / 1000 );
		while (!mLocalFile && IsRunning())
		{
			CThread::Sleep(eShortSleep);
			HL_Event();
			if( mCancelRead  )
				goto finish;
			currTime.SetupRealTime();
			if ( currTime > endTime )
			{
				UDebug::Message(">>> Unflat timeout!!!\n");
				goto finish;
			}
		}
	}

	// if we still don't have the local file, just return with error
	if (!mLocalFile)
		goto finish;

	// check if we have available data in local file
	if (IsRunning() && inFileOffset+ioDataSize>GetAvailableSize() )
	{
		CDateTime endTime, currTime;
		endTime.AddSeconds( eReadTimeOut / 1000 );
		while ( IsRunning() && inFileOffset+ioDataSize>GetAvailableSize() )
		{
			CThread::Sleep(eShortSleep);
			HL_Event();
			if( mCancelRead  )
				goto finish;
			currTime.SetupRealTime();
			if ( currTime > endTime )
			{
				UDebug::Message(">>> Read timeout!!!\n");
				goto finish;
			}
		}
	}

	// if we still don't have enough data, just return with error
	if( inFileOffset+ioDataSize > GetAvailableSize() )
		goto finish;
		
	// read from the local file
	try {
		UInt64 ofs64 = inFileOffset;
		ioDataSize = mLocalFile->ReadFrom( (UInt8*)inPtr, (UInt64)ioDataSize, ofs64);
		result = ioDataSize>0;
	}
	catch ( const CException &e ) {
		mLastError = e.GetErrorCode();
	}
	catch (...)	{
	}
finish:
	mReading--;
	Release();
	return result;
}

// ---------------------------------------------------------------------
//  GetFullSize												    [public]
// ---------------------------------------------------------------------
// return the full size of a file
UInt32 
CDataProvider::GetFullSize()
{
	return mFileSize; 
}

// ---------------------------------------------------------------------
//  GetAvailableSize										    [public]
// ---------------------------------------------------------------------
// return the available size of a file for this moment
UInt32 
CDataProvider::GetAvailableSize()
{
	return mLocalFile? mLocalFile->GetSize(): 0;
}

// ---------------------------------------------------------------------
//  GetLastError    										    [public]
// ---------------------------------------------------------------------
// return the last error that was caught and reset it
AW_ErrorCode 
CDataProvider::GetLastError()
{
	AW_ErrorCode err = mLastError;
	mLastError = UAWError::eNone;
	return err;
}


// ---------------------------------------------------------------------
//  ListenToMessage											 [protected]
// ---------------------------------------------------------------------
// Listen to CUnflattenFile. When it has created a local file,
// it sends "eDataCreated" message, so we can open that file for reading
void 
CDataProvider::ListenToMessage(const CMessage& inMessage)
{
	if (inMessage.GetID()==CUnflattenFile::eDataCreated)
	{
		try
		{
			mFileSize = mUnflat->GetDataForkSize();
			UDebug::Message("Full size=%d\n", mFileSize);
			mTempFileRef = mUnflat->GetFileRef().Clone();
			mLocalFile = new CLocalFile( *(CFSLocalFileRef*)(mTempFileRef.get()) );
			mLocalFile->Open(
				CLocalFile::eMode_ReadOnly, 
				CLocalFile::eShare_ReadWrite);
			mUnflat->RemoveListener( *this );
		}
		catch ( const CException &e )
		{
			mLastError = e.GetErrorCode();
			delete mLocalFile;
			mLocalFile = nil;
		}
		catch (...)
		{
			delete mLocalFile;
			mLocalFile = nil;
		}
	}
}


void
CDataProvider::CloseConnection()
{
	if (mStream.IsOpen())
	{
		// stop "Run()" thread
		Quit();
		// Spin until thread terminates
		while( !IsDone() ){
			CThread::Sleep();
		}
		mStream.Close();
	}
}

HL_End_Namespace_BigRedH
