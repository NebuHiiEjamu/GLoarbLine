/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CNetTCPListener.h"
#include "CNetAddress.h"
#include "CNetTCPConnection.h"

HL_Begin_Namespace_BigRedH


// ---------------------------------------------------------------------
//  CNetTCPListener                                             [public]
// ---------------------------------------------------------------------
// Constructor

CNetTCPListener::CNetTCPListener( const CNetAddress& inListenAddress )
		: mListenAddress(inListenAddress)
{
}


// ---------------------------------------------------------------------
//  ~CNetTCPListener                                            [public]
// ---------------------------------------------------------------------
// Destructor

CNetTCPListener::~CNetTCPListener()
{
}


// ---------------------------------------------------------------------
//  GetRemoteAddress                                            [public]
// ---------------------------------------------------------------------
//

const CNetAddress&
CNetTCPListener::GetRemoteAddress()
{
	return mRemoteAddress;
}


// ---------------------------------------------------------------------
//  Listen                                                      [public]
// ---------------------------------------------------------------------
//

void
CNetTCPListener::Listen()
{
}


// ---------------------------------------------------------------------
//  Accept                                                      [public]
// ---------------------------------------------------------------------
//

void
CNetTCPListener::Accept( CNetTCPConnection &outConnection )
{
}


// ---------------------------------------------------------------------
//  Reject                                                      [public]
// ---------------------------------------------------------------------
//

void
CNetTCPListener::Reject()
{
}


HL_End_Namespace_BigRedH

