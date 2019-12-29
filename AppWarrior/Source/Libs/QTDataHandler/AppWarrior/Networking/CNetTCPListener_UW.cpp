/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#if TARGET_OS_WIN32
	#define socklen_t int
#elif TARGET_OS_UNIX
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <errno.h>
	#define INVALID_SOCKET (-1)
	#define SOCKET int
	#define closesocket close
	#define WSAGetLastError() errno
#endif
#include "CNetTCPListener.h"
#include "CNetAddress.h"
#include "CNetTCPConnection.h"
#include "CNetTCPConnection_UW.h"
#include "CNetworkException.h"

HL_Begin_Namespace_BigRedH

class CNetTCPListener::CNetConnectionPS
{
	public:
		SOCKET	mSocket;

		CNetConnectionPS() : mSocket( INVALID_SOCKET ) {}
};

// ---------------------------------------------------------------------
// CNetTCPListener                                              [public]
// ---------------------------------------------------------------------
// Initialize default values for a object

CNetTCPListener::CNetTCPListener( const CNetAddress& inListenAddress )
	: mListenAddress( inListenAddress ), mPlatform( new CNetTCPListener::CNetConnectionPS )
{
}


// ---------------------------------------------------------------------
// ~CNetTCPListener                                             [public]
// ---------------------------------------------------------------------
// Destructor checks if socket is open and closes it

CNetTCPListener::~CNetTCPListener()
{
	if ( mPlatform->mSocket != INVALID_SOCKET )
//??	shutdown( mPlatform->mSocket, 2 );
		closesocket( mPlatform->mSocket );
	delete mPlatform;
}


// ---------------------------------------------------------------------
// GetRemoteAddress                                             [public]
// ---------------------------------------------------------------------
// Returns remote address

const CNetAddress&
CNetTCPListener::GetRemoteAddress()
{
	return mRemoteAddress;
}


// ---------------------------------------------------------------------
//  Listen                                                      [public]
// ---------------------------------------------------------------------
// Create listening connection.

void
CNetTCPListener::Listen()
{
	try
	{
		mPlatform->mSocket = ::socket(AF_INET, SOCK_STREAM, 0);
		if(mPlatform->mSocket == INVALID_SOCKET)
		{
			CNetAddress address;
			THROW_NET_( eConnecting, eNetworkCannotCreate, address, OS_ErrorCode(::WSAGetLastError()) );
		}

		sockaddr_in addr;
		addr.sin_family		 = AF_INET;
		addr.sin_port		 = mListenAddress.GetPort();
		addr.sin_addr.s_addr = mListenAddress.GetIP();

		if(::bind(mPlatform->mSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
		{
			CNetAddress address;
			THROW_NET_( eConnecting, eNetworkCannotBind, address, OS_ErrorCode(::WSAGetLastError()) );
		}

		if(::listen(mPlatform->mSocket, 8) != 0)
		{
			CNetAddress address;
			THROW_NET_( eConnecting, eNetworkCannotBind, address, OS_ErrorCode(::WSAGetLastError()) );
		}
	}
	catch (...)
	{
		RETHROW_NET_( eConnecting, mListenAddress );
	}
}


// ---------------------------------------------------------------------
// Accept                                                       [public]
// ---------------------------------------------------------------------
// Accept connection and set remote address. This can block!

void
CNetTCPListener::Accept( CNetTCPConnection &outConnection )
{
	try
	{
		socklen_t size;
		sockaddr_in addr;
		SOCKET s = ::accept(mPlatform->mSocket, reinterpret_cast<sockaddr*>(&addr), &size);
		if(s == INVALID_SOCKET)
		{
			CNetAddress address;
			THROW_NET_( eConnecting, eNetworkCannotCreate, address, OS_ErrorCode(::WSAGetLastError()) );
		}

		CNetAddress address(static_cast<UInt32>(addr.sin_addr.s_addr), addr.sin_port);
		mRemoteAddress = address;
		outConnection.mPlatform->mSocket = s;
		outConnection.SetRemoteAddress(address);
	}
	catch (...)
	{
		RETHROW_NET_( eConnecting, mListenAddress );
	}
}


// ---------------------------------------------------------------------
// Reject                                                       [public]
// ---------------------------------------------------------------------
// Reject connection. Revisit!

void
CNetTCPListener::Reject()
{
}


HL_End_Namespace_BigRedH