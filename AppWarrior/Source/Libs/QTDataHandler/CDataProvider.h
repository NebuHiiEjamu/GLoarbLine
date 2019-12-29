// =====================================================================
//  CDataProvider.h                      (C) Hotline Communications 2000
// =====================================================================
// 
//  A class that wraps syncronous getting data from any URL
// =====================================================================

#ifndef _H_DATAPROVIDER_
#define _H_DATAPROVIDER_


//-----------------------------------------------------------------------
// Includes
#include "CThread.h"
#include "CMutex.h"
#include "CNetAddress.h"
#include "CNetTCPConnection.h"
#include "CSyncTCPIOStream.h"
#include "CUnflattenFile.h"
#include "CLocalFile.h"

HL_Begin_Namespace_BigRedH


class CDataHandlerComponent;

class CDataProvider: public CThread, public CListener
{
	friend class CDataHandlerComponent;

public:
	enum ETimeOut {
		eTCPTimeOut		= 15000,	// max.time in ms to opening TCP connection
		eUnflatTimeOut	= 15000,	// max.time in ms from opening a stream to a local file
		eReadTimeOut	= 10000,	// max.time in ms to wait for data in syncronous read
		eShortSleep		= 100		// short sleep time in ms 
	};

protected:
	CDataProvider(
		CString&	inServer, 
		CString&	inFileName, 
		UInt16		inPort, 
		UInt32		inFileSize, 
		UInt32		inRefNum, 
		CString&	inTempFileName,
		CString&	inMimeType,
		UInt32		inFileType);
	virtual ~CDataProvider();
public:
	UInt32					AddRef();
	UInt32					Release(); 

	bool					IsOpen();
	bool					IsReading()
								{ return mReading > 0; }
	void					CancelReading()
								{ mCancelRead = true; }

								// get full size of the file
	UInt32					GetFullSize();

								// get file name for the original
	const CString&			GetFileName()
								{ return mFileName; }
								// get mime type for the original
	const CString&			GetMimeType()
								{ return mMimeType; }
								// get mime type for the original
	UInt32					GetFileType()
								{ return mFileType; }

								// get available size of the file
	UInt32					GetAvailableSize();

								// syncronous read 
	bool					Read(void* inPtr, UInt32 inFileOffset, UInt32& ioDataSize); 

								// stop downloading and close a TCP connection
	void					CloseConnection();

								// return the last error caught, and reset
	AW_ErrorCode			GetLastError();

protected:
	// here we download data in cycle and put it into the file
	virtual UInt32 Run();

	// respond to Unflattener's message
	virtual void ListenToMessage(const CMessage& inMessage);
private:
	UInt32					mRefCount;
	CSyncTCPIOStream		mStream;
	CString					mServer;
	CString					mFileName;
	UInt32					mPort;
	UInt32					mFileSize;
	SInt32					mRefNum;
	CString					mTempFileName;
	std::auto_ptr<CFSRef>	mTempFileRef;
	CString					mMimeType;
	UInt32					mFileType;
	UInt32					mAvailableSize;
	AW_ErrorCode			mLastError;

	CLocalFile*				mLocalFile;
	CUnflattenFile*			mUnflat;
	bool					mOpen;
	UInt16					mReading;
	bool					mCancelRead;
};


HL_End_Namespace_BigRedH

#endif //_H_DATAPROVIDER_
