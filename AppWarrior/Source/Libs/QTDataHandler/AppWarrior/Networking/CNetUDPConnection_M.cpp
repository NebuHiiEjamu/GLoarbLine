/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CNetUDPConnection.h"
#include "UNetUtilMac.h"
#include <OpenTransport.h>
#include <OpenTptInternet.h>

HL_Begin_Namespace_BigRedH

class CNetUDPConnection::CNetConnectionPS
{
	public:
		TEndpoint*	mSocket;

		CNetConnectionPS() : mSocket( kOTInvalidEndpointRef ) {}
};

// ---------------------------------------------------------------------
// CNetTCPConnection                                            [public]
// ---------------------------------------------------------------------
// Creates TCP connection class with default local address

CNetUDPConnection::CNetUDPConnection()
	: mPlatform( new CNetUDPConnection::CNetConnectionPS )
{
}


// ---------------------------------------------------------------------
// CNetTCPConnection                                            [public]
// ---------------------------------------------------------------------
// Creates TCP connection class and sets local address

CNetUDPConnection::CNetUDPConnection( const CNetAddress& inLocalAddress )
	: CNetConnection( inLocalAddress ), mPlatform( new CNetUDPConnection::CNetConnectionPS )
{
}


// ---------------------------------------------------------------------
//  ~CNetUDPConnection                                          [public]
// ---------------------------------------------------------------------
// Destructor

CNetUDPConnection::~CNetUDPConnection()
{
	Disconnect();
	delete mPlatform;
}


// ---------------------------------------------------------------------
//  Connect                                                     [public]
// ---------------------------------------------------------------------
//

bool
CNetUDPConnection::Connect( const CNetAddress& inRemoteAddress )
{
	bool connected = false;
	
	return connected;
}


// ---------------------------------------------------------------------
//  Disconnect                                                  [public]
// ---------------------------------------------------------------------
//

void
CNetUDPConnection::Disconnect()
{
	if( mPlatform->mSocket != kOTInvalidEndpointRef ){ // Connected
		mPlatform->mSocket->Unbind();
		mPlatform->mSocket->RemoveNotifier();
		mPlatform->mSocket->Close();
		mPlatform->mSocket = kOTInvalidEndpointRef;
	}
}


// ---------------------------------------------------------------------
//  Read                                                        [public]
// ---------------------------------------------------------------------
//

UInt32
CNetUDPConnection::Read( UInt8 *inBuffer, UInt32 inMaxSize,
						UInt16 inTimeoutSec )
{
	UInt32 bytesRead = 0;
	
	return bytesRead;
}


// ---------------------------------------------------------------------
//  Read                                                        [public]
// ---------------------------------------------------------------------
//

UInt32
CNetUDPConnection::Write( UInt8 *inBuffer, UInt32 inMaxSize,
						UInt16 inTimeoutSec )
{
	UInt32 bytesWritten = 0;
	
	return bytesWritten;
}

HL_End_Namespace_BigRedH

