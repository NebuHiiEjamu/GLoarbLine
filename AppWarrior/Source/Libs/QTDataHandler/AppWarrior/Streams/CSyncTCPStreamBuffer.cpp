/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CDateTime.h"
#include "CNetAddress.h"
#include "CNetTCPConnection.h"
#include "CSyncStreamBuffer.h"
#include "CSyncTCPStreamBuffer.h"

HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  ~CSyncTCPStreamBuffer                              [public][virtual]
// ---------------------------------------------------------------------
// destructor

CSyncTCPStreamBuffer::~CSyncTCPStreamBuffer()
{ 
	// don't allow exceptions out of here
	try
	{
		Close();
	}
	catch(CException& e)
	{
		THROW_COPY_STREAM_(e, eClose);
	}
	catch(...)
	{
		THROW_UNKNOWN_STREAM_(eClose);
	}
}

// ---------------------------------------------------------------------
//  Open                                                        [public]
// ---------------------------------------------------------------------

void
CSyncTCPStreamBuffer::Open(const CNetAddress& inAddress)
{
	try
	{
		mConnection.Connect(inAddress);
	}
	catch(CException& e)
	{
		THROW_COPY_STREAM_(e, eOpen);
	}
	catch(...)
	{
		THROW_UNKNOWN_STREAM_(eOpen);
	}

	if(!mConnection.IsConnected())
	{
		THROW_UNKNOWN_STREAM_(eOpen);
	}
}

// ---------------------------------------------------------------------
//  Close                                                       [public]
// ---------------------------------------------------------------------

void
CSyncTCPStreamBuffer::Close()
{
	try
	{
		mConnection.Disconnect();
	}
	catch( ... )
	{
		RETHROW_STREAM_(eClose);
	}
}


// ---------------------------------------------------------------------
//  GetAvailable                                                [public]
// ---------------------------------------------------------------------
UInt32
CSyncTCPStreamBuffer::GetAvailable()
{
	UInt32 n = 0;
	try
	{
		n = mConnection.GetAvailable();
	}
	catch(CException& e)
	{
		THROW_COPY_STREAM_(e, eQuery);
	}
	catch(...)
	{
		THROW_UNKNOWN_STREAM_(eQuery);
	}
	return n;
}


// ---------------------------------------------------------------------
//  Read                                                        [public]
// ---------------------------------------------------------------------

UInt32
CSyncTCPStreamBuffer::Read(void* inBuf, UInt32 inSize)
{
	using namespace std;
	ASSERT(mConnection.IsConnected() && inBuf != nil && inSize > 0);

	CDateTime time;
	
	UInt32 available = 0;

	try
	{
		while((available = GetAvailable()) == 0)
		{
			if(CDateTime::GetCurrentDateTime().GetDateTime()-time.GetDateTime() >= mTimeout)
			{
				THROW_STREAM_(eRead, eTimeout, 0);
			}
		}

		UInt32 size  = min(inSize, available);
		UInt32 count = size;
		UInt32 read  = 0;
		UInt32 tmp   = 0;
		UInt8* ptr	 = (UInt8*)inBuf;

		while(tmp != size)
		{
			read   = mConnection.Read((UInt8*)ptr, count, mTimeout);
			tmp   += read;
			count -= read;
			ptr   += read;
			if (read>0)
				return tmp;

			if(CDateTime::GetCurrentDateTime().GetDateTime()-time.GetDateTime() >= mTimeout)
			{
				THROW_STREAM_(eRead, eTimeout, 0);
			}
		}
		
		return tmp;
	}
	catch( ... )
	{
		RETHROW_STREAM_(eRead);
	}
	
	return 0;
}


// ---------------------------------------------------------------------
//  Write                                                       [public]
// ---------------------------------------------------------------------

UInt32
CSyncTCPStreamBuffer::Write(const void* inBuf, UInt32 inSize)
{
	ASSERT(mConnection.IsConnected() && inBuf != nil && inSize > 0);

	UInt32 size    = inSize;
	UInt32 count   = size;
	UInt32 written = 0;
	UInt32 tmp     = 0;
	UInt8* ptr	   = (UInt8*)inBuf;

	CDateTime time;

	try
	{
		while(tmp != size)
		{
			written = mConnection.Write((UInt8*)ptr, count, mTimeout);
			tmp    += written;
			count  -= written;
			ptr    += written;

			if(CDateTime::GetCurrentDateTime()-time >= mTimeout)
			{
				THROW_STREAM_(eWrite, eTimeout, 0);
			}
		}

		return tmp;
	}
	catch( ... )
	{
		RETHROW_STREAM_(eWrite);
	}
	
	return 0;
}


HL_End_Namespace_BigRedH

