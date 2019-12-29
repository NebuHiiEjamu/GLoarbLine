/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
//
//  Implementation of CDataHandlerConnection class
//  (platform-independed, but uses QuickTime SDK)


//-----------------------------------------------------------------------
// includes

#include "aw.h"
#undef PRAGMA_ONCE // to avoid macro redefinition warning
#include <Components.h>
#include <QuickTimeComponents.h>
#include "CDataHandlerConnection.h"
#include "CDataHandlerComponent.h"
#include "UStringConverter.h"
//#include "CAsyncReader.h"
#include "USyncroHelpers.h"
#include "StString.h"
#include "StHandleLocker.h"
//-----------------------------------------------------------------------


HL_Begin_Namespace_BigRedH

ComponentResult AWErrToQTErr(AW_ErrorCode inError, ComponentResult inDefault = readErr );

// translates AW errors to QT errors
ComponentResult AWErrToQTErr(AW_ErrorCode inError, ComponentResult inDefault )
{
	switch (inError) {
		case UAWError::eMemoryNotEnough:	
			return memFullErr;
			
		case UAWError::eNotEnoughSpace:		
			return dskFulErr;

		case UAWError::eNetworkHostUnreachable: 
		case UAWError::eNetworkConnectionRefused: 
		case UAWError::eNetworkConnectionTimedOut: 
			return openErr;
		
		case UAWError::eNetworkConnectionAborted: 
			return readErr;
		
	default:
		return inDefault;
	}
}


void PStrCopy( Str255 inSource, Str255 outDest );

void PStrCopy( Str255 inSource, Str255 outDest )
{
	std::memcpy( outDest, inSource, inSource[0] + 1 );
}


// ---------------------------------------------------------------------
//  CDataHandlerComponent									    [public]
// ---------------------------------------------------------------------
// constructor

CDataHandlerConnection::CDataHandlerConnection():
//	mDataRef(nil),
	mProvider(nil)
{
}


// ---------------------------------------------------------------------
//  ~CDataHandlerComponent										[public]
// ---------------------------------------------------------------------
// destructor


CDataHandlerConnection::~CDataHandlerConnection()
{
//	CAsyncReader::Instance().FinishAll(this); //?? Do all reads sync

	if (mProvider)
		mProvider->Release();

//	if (mDataRef)
//		DisposeHandle(mDataRef);
}

// ---------------------------------------------------------------------
//  SetDataRef													[public]
// ---------------------------------------------------------------------
// saves dataref for future use. According to the URL, 
// we get either an existing data provider or create the new one
ComponentResult 
CDataHandlerConnection::SetDataRef( Handle inDataRef )
{
	// convert DataRef to URL here
	{
		StHandleLocker lock( inDataRef );
		mURL = CString((const char *)*inDataRef);
	}

	if (mProvider)
		mProvider->Release(),
		mProvider = nil;

	mProvider = CDataHandlerComponent::Instance().GetProvider(mURL);
	ASSERT( mProvider != nil);

	if (!mProvider->IsOpen())
		return AWErrToQTErr(mProvider->GetLastError(), openErr);

	return noErr;
}


// ---------------------------------------------------------------------
//  GetFileName													[public]
// ---------------------------------------------------------------------
// get filename with an extention. QuickTime uses it for two purposes:
// 1st - to show user in caption bar if no other name inside movie is present,
// 2nd - to make sure this is a movie (by extention). If QuickTime cannot
// determine if it is a movie or not, it uses GetMimeType method 

ComponentResult 
CDataHandlerConnection::GetFileName(StringPtr outFileName)
{
	if( mProvider != nil ){
		StPStyleString pstr( mProvider->GetFileName() );
		PStrCopy( pstr, outFileName );
	}
	
#if DEBUG
	UDebug::Message("GetFileName = %#s\n", outFileName );
#endif

	return noErr;
}


ComponentResult 
CDataHandlerConnection::GetMimeType(StringPtr outMimeType)
{
	if( mProvider != nil ){
		StPStyleString pstr( mProvider->GetMimeType() );
		PStrCopy( pstr, outMimeType );
	}

#if DEBUG
	UDebug::Message( "GetMimeType = %#s", outMimeType );
#endif

	return noErr;
}


ComponentResult 
CDataHandlerConnection::GetFileType(OSType *outFileType)
{
	if( mProvider != nil ){
		*outFileType =  mProvider->GetFileType();
	}

	return noErr;
}


// ---------------------------------------------------------------------
//  GetFullSize													[public]
// ---------------------------------------------------------------------
// Get full size of the movie
ComponentResult 
CDataHandlerConnection::GetFullSize(long* outSize)
{
	*outSize = (long)mProvider->GetFullSize();
	return (*outSize==0)?badComponentSelector:noErr;
}


// ---------------------------------------------------------------------
//  GetFullSize													[public]
// ---------------------------------------------------------------------
// get available size of the movie
ComponentResult 
CDataHandlerConnection::GetAvailableSize(long* outSize)
{
	*outSize = (long)mProvider->GetAvailableSize();
	return noErr;
}

// ---------------------------------------------------------------------
//  OpenForRead													[public]
// ---------------------------------------------------------------------
// open current dataref for read
ComponentResult 
CDataHandlerConnection::OpenForRead()
{
	if (!mProvider->IsOpen())
		return AWErrToQTErr(mProvider->GetLastError(), openErr);
	return noErr;
}

// ---------------------------------------------------------------------
//  CloseForRead												[public]
// ---------------------------------------------------------------------
// close current dataref
ComponentResult 
CDataHandlerConnection::CloseForRead()
{
	return noErr;
}


// ---------------------------------------------------------------------
//  CompareDataRef												[public]
// ---------------------------------------------------------------------
// compare given dataref with current
bool
CDataHandlerConnection::CompareDataRef(Handle inDataRef)
{
	StHandleLocker lock( inDataRef );
	CString inURL( (const char *)*inDataRef );
	return (mURL == inURL);
}


// ---------------------------------------------------------------------
//  ScheduleData												[public]
// ---------------------------------------------------------------------
// read data sync ar async
ComponentResult 
CDataHandlerConnection::ScheduleData(
		Ptr inPlaceToPutData,
		UInt32 inFileOffset,
		UInt32 inDataSize,
		UInt32 inRefCon,
		DataHSchedulePtr inScheduleRec,
		DataHCompletionUPP inCompletionRtn)
{
	if (!mProvider->IsOpen())
		return AWErrToQTErr(mProvider->GetLastError(), openErr); 

//	if (inScheduleRec==nil) //?? Do all reads sync
    {
		UDebug::Message("Sync, from=%d datasize=%d\n",inFileOffset, inDataSize);

		// synchronous request, using the main thread
		ComponentResult result = noErr;
		if ( !mProvider->Read(inPlaceToPutData, inFileOffset, inDataSize) )
			result = AWErrToQTErr(mProvider->GetLastError(), readErr); 

		if (inCompletionRtn)
		#if TARGET_API_MAC_CARBON
			InvokeDataHCompletionUPP(inPlaceToPutData, inRefCon, result, inCompletionRtn);
		#else
			CallDataHCompletionProc(inCompletionRtn, inPlaceToPutData, inRefCon, result);
		#endif
			
		return result;
    }
/*
	else
    {
		UDebug::Message("Async (%x) started\n",inPlaceToPutData);
		CAsyncReader::Instance().Read( 
				this, mProvider,
				inPlaceToPutData, inFileOffset, inDataSize,
				inRefCon, inScheduleRec, inCompletionRtn);
	}
*/
	return noErr;
}


// ---------------------------------------------------------------------
//  FinishData													[public]
// ---------------------------------------------------------------------
// finish reading data
ComponentResult  
CDataHandlerConnection::FinishData (
		Ptr inPlaceToPutData, 
		Boolean inCancel)
{
    return badComponentSelector; //?? Do all reads sync
/*
	bool result;
	if (inCancel)
	{
		if (inPlaceToPutData)
			result = CAsyncReader::Instance().FinishData(inPlaceToPutData);
		else
			result = CAsyncReader::Instance().FinishAll(this);
	}
	else
	{
		if (inPlaceToPutData)
			result = CAsyncReader::Instance().SpeedUpData(inPlaceToPutData);
		else
			result = false;
	}
	
	return result? noErr: readErr;
*/
}


// ---------------------------------------------------------------------
//  DataTask													[public]
// ---------------------------------------------------------------------
// spend time. A caller periodically calls that function to let us do 
// our internal time-based work
ComponentResult 
CDataHandlerConnection::DataTask()
{
	CThread::Sleep();
	return noErr;
}


// stop downloading and close a TCP stream
ComponentResult 
CDataHandlerConnection::FlushData()
{
	ASSERT(mProvider != nil);
	mProvider->CloseConnection();
	return noErr;
}



HL_End_Namespace_BigRedH
