// ====================================================================
//  CAsyncReader.h						(C) Hotline Communications 2000
// ====================================================================
// 
//  A class that wraps getting data asyncronously
//  for several connections 
//
//  Internally it uses CDataProvider's synchronous Read function 
// ====================================================================

#ifndef _H_ASYNCREADER_
#define _H_ASYNCREADER_


//-----------------------------------------------------------------------
// Includes
#include <Components.h>
#include <QuickTimeComponents.h>
#include "CThread.h"
#include "CMutex.h"
#include "CDataProvider.h"
#include "CDataHandlerConnection.h"

HL_Begin_Namespace_BigRedH

class CAsyncReader: public CThread
{
public:
	static CAsyncReader& Instance();
	virtual ~CAsyncReader();
public:
	// asyncronous read 
	bool Read(
		CDataHandlerConnection* inConnection,
		CDataProvider*		inProvider,
		Ptr					inPtr, 
		UInt32				inFileOffset, 
		UInt32				inDataSize,
        UInt32				inRefCon,
        DataHSchedulePtr	inScheduleRec,
        DataHCompletionUPP	inCompletionRtn);

	// SpeedUp asynchronous requst for a given pointer
	bool SpeedUpData (Ptr inPtr);

	// remove previous request
	bool FinishData(Ptr inPtr);

	// remove all requests
	bool FinishAll(CDataHandlerConnection* inConnection);

protected:
	// constructor
	CAsyncReader();

	// here we download data in cycle and put it into the file
	virtual UInt32 Run();
private:

	CMutex mListLock, mThreadLock;

	// Asyncronous read request handling class
	class CDataRequest
	{
	public:
		// init constructor
		CDataRequest(
			CDataHandlerConnection* inConnection,
			CDataProvider*		inDataProvider,
	        Ptr					inPlaceToPutData,
		    UInt32				inFileOffset,
			UInt32				inDataSize,
			UInt32				inRefCon,
			DataHSchedulePtr	inScheduleRec,
			DataHCompletionUPP	inCompletionRtn);
		~CDataRequest();
		// data
		bool				mSuccess;
        Ptr					mPlaceToPutData;
        UInt32				mFileOffset;
        UInt32				mDataSize;
        UInt32				mRefCon;
        DataHScheduleRecord	mScheduleRec;
        DataHCompletionUPP	mCompletionRtn;
		CDataProvider*		mDataProvider;
		CDataHandlerConnection* mConnection;
	};

	CDataRequest* mCurrentRequest;
	std::list<CDataRequest*> mRequests;
	
	static std::auto_ptr<CAsyncReader>	sInstance; // Singleton
};


HL_End_Namespace_BigRedH

#endif //_H_ASYNCREADER_
