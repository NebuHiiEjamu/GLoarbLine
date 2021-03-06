// =====================================================================
//  CNetUDPConnection.h                  (C) Hotline Communications 2000
// =====================================================================
// Implements UDP connection operations for Windows & Linux
// UNIX will be tested soon :^))

#ifndef _H_CNetUDPConnection_
#define _H_CNetUDPConnection_

#if PRAGMA_ONCE
	#pragma once
#endif

#include "CNetConnection.h"

HL_Begin_Namespace_BigRedH

class CNetAddress;

class CNetUDPConnection : public CNetConnection
{
	public:
								CNetUDPConnection();
								// throws CMemoryException
								CNetUDPConnection( const CNetAddress& inLocalAddress );
								// throws CMemoryException
		virtual				~CNetUDPConnection();
		
			// ** Connection **
		virtual bool		Connect( const CNetAddress& inRemoteAddress );
								// throws CNetworkException
		virtual bool		IsConnected();
								// throws nothing
		virtual void		Disconnect();
								// throws nothing

			// ** Data Transfer **
		virtual UInt32		GetAvailable();
								// throws CNetworkException
		virtual UInt32		Read( UInt8 *inBuffer,
										UInt32 inMaxSize,
										UInt16 inTimeoutSec = 0 );
								// throws CNetworkException
		virtual UInt32		Write( UInt8 *inBuffer,
										 UInt32 inMaxSize,
										 UInt16 inTimeoutSec = 0 );
								// throws CNetworkException

	private:
		class CNetConnectionPS;
		CNetConnectionPS *mPlatform;
};

HL_End_Namespace_BigRedH

#endif // _H_CNetUDPConnection_