// =====================================================================
//  CNetConnection.h                     (C) Hotline Communications 1999
// =====================================================================
//

#ifndef _H_CNetConnection_
#define _H_CNetConnection_

#if PRAGMA_ONCE
	#pragma once
#endif
#include "CNetAddress.h"

HL_Begin_Namespace_BigRedH


class CNetConnection
{
	public:
			// ** Construction **
							CNetConnection()
								{}
		explicit			CNetConnection( const CNetAddress& inLocalAddress )
								: mLocalAddress( inLocalAddress )
								{}
		virtual				~CNetConnection()
								{}

			// ** Addresses **
		const CNetAddress&	GetLocalAddress()	const
								{ return mLocalAddress; }
		const CNetAddress&	GetRemoteAddress()	const
								{ return mRemoteAddress; }

			// ** Connection **
		virtual bool		Connect( const CNetAddress& inRemoteAddress ) = 0;
		virtual bool		IsConnected() = 0;
		virtual void		Disconnect() = 0;

			// ** Data Transfer **
		virtual UInt32		GetAvailable() = 0;
		virtual UInt32		Read( UInt8 *inBuffer,
									UInt32 inMaxSize,
									UInt16 inTimeoutSec = 0 ) = 0;
		virtual UInt32		Write( UInt8 *inBuffer,
									UInt32 inMaxSize,
									UInt16 inTimeoutSec = 0 ) = 0;

	protected:
		void				SetLocalAddress( const CNetAddress &inLocalAddress )
								{ mLocalAddress = inLocalAddress; }
		void				SetRemoteAddress( const CNetAddress &inRemoteAddress )
								{ mRemoteAddress = inRemoteAddress; }
	
	private:
		CNetAddress			mLocalAddress;
		CNetAddress			mRemoteAddress;
};

HL_End_Namespace_BigRedH

#endif // _H_CNetConnection_
