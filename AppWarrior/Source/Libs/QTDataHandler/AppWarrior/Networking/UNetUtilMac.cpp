/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "UNetUtilMac.h" 
#include "StString.h"


HL_Begin_Namespace_BigRedH

#if TARGET_API_MAC_CARBON
OTNotifyUPP UNetUtilMac::sYieldingNotifierUPP = NewOTNotifyUPP(UNetUtilMac::YieldingNotifier);
#endif

// ---------------------------------------------------------------------
//  YieldingNotifier                                    [public][static]
// ---------------------------------------------------------------------
// Allows for threading yields from sync OT calls

pascal void
UNetUtilMac::YieldingNotifier( void *contextPtr, OTEventCode code, 
				OTResult result, void *cookie )
{
#pragma unused(contextPtr,result,cookie)

	OSStatus junk;
	
	switch( code ){
		case kOTSyncIdleEvent:
			junk = ::YieldToAnyThread();
			break;
		default:
			// do nothing
			break;
	}
}


// ---------------------------------------------------------------------
//  OTToNetAddress                                      [public][static]
// ---------------------------------------------------------------------
// Converts OT to CNetAddress

CNetAddress
UNetUtilMac::OTToNetAddress( InetAddress *inAddress )
{
	switch( inAddress->fAddressType ){
		case AF_INET:
			{
				InetAddress *inetAddr = (InetAddress*)inAddress;
				return CNetAddress( inetAddr->fHost, inetAddr->fPort );
			}

		case AF_DNS:
			{
				DNSAddress *dnsAddr = (DNSAddress*)inAddress;
				return CNetAddress( CString(dnsAddr->fName), 0 );
			}
	}
	return CNetAddress(0);
}


// ---------------------------------------------------------------------
//  NetAddressToOT                                      [public][static]
// ---------------------------------------------------------------------
// Converts OT to CNetAddress

InetAddress*
UNetUtilMac::NetAddressToOT( const CNetAddress &inAddress )
{
	InetAddress* inAddr = nil;
	inAddr = (InetAddress*)::NewPtrClear(sizeof (InetAddress));

	inAddr->fAddressType = AF_INET;
	inAddr->fPort = inAddress.GetPort();
	inAddr->fHost = inAddress.GetIP();
	
	return inAddr;
}


#if 0
	#pragma mark -
#endif
TimerUPP CNetTimeoutMac::sNetTimerUPP = NewTimerUPP(CNetTimeoutMac::Execute);

// ---------------------------------------------------------------------
//  CNetTimeoutMac                                              [public]
// ---------------------------------------------------------------------
// Constructor

CNetTimeoutMac::CNetTimeoutMac( TEndpoint *inEndpoint,
								UInt32 inMilliSecs )
	: mEndpoint( inEndpoint ), mFired( false )
{
	mTask.mTimer = this;

	mTask.mTask.qLink = nil;
	mTask.mTask.qType = 0;
	mTask.mTask.tmAddr = sNetTimerUPP;
	mTask.mTask.tmCount = 0;
	mTask.mTask.tmWakeUp = 0;
	mTask.mTask.tmReserved = 0;

	mTask.mSemQ.qFlags = 0;
	mTask.mSemQ.qHead = nil;
	mTask.mSemQ.qTail = nil;

	mTask.mSemEl.qLink = nil;
	mTask.mSemEl.qType = 0;

	InsertTimeTask();
	PrimeTimeTask( inMilliSecs );
}


// ---------------------------------------------------------------------
//  ~CThreadTimer                                               [public]
// ---------------------------------------------------------------------
// Destructor

CNetTimeoutMac::~CNetTimeoutMac()
{
	RemoveTimeTask();
}


// ---------------------------------------------------------------------
//  Execute                                            [private][static]
// ---------------------------------------------------------------------
// Time Manager completion routine.  Converts the given Time Manager 
// task into a SNetTimeoutTMTask, then kill the give call.

pascal void
CNetTimeoutMac::Execute( TMTaskPtr inTask )
{
	SNetTimeoutTMTask *tpb = GetTimeMgrPtr(inTask);
	CNetTimeoutMac *theTimer = tpb->mTimer;
	theTimer->mFired = true;
	theTimer->mEndpoint->CancelSynchronousCalls( kOTCanceledErr );//kETIMEDOUTErr );
}


// ---------------------------------------------------------------------
//	GetTimeMgrPtr                                      [private][static]
// ---------------------------------------------------------------------

inline CNetTimeoutMac::SNetTimeoutTMTask*
CNetTimeoutMac::GetTimeMgrPtr( TMTaskPtr inTaskPtr )
{
	using namespace std;

	return (reinterpret_cast<SNetTimeoutTMTask*>(
			reinterpret_cast<char *>(inTaskPtr) - 
			OFFSET_OF(SNetTimeoutTMTask, mTask)));
}


// ---------------------------------------------------------------------
//  InsertTimeTask                                             [private]
// ---------------------------------------------------------------------
// Insert the task into the Time Manager's queue.

inline void
CNetTimeoutMac::InsertTimeTask()
{
	// signal timer "semaphore"
	::Enqueue( &mTask.mSemEl, &mTask.mSemQ );
	
	// add element to timer queue
#if TARGET_API_MAC_CARBON
	::InstallTimeTask(reinterpret_cast<QElemPtr>(&mTask.mTask));
#else
	::InsTime(reinterpret_cast<QElemPtr>(&mTask.mTask));
#endif
}


// ---------------------------------------------------------------------
//  PrimeTimeTask                                              [private]
// ---------------------------------------------------------------------
// Activate the task in the Time Manager's queue.

inline void
CNetTimeoutMac::PrimeTimeTask( UInt32 inMilliSecs )
{
	::PrimeTime( reinterpret_cast<QElemPtr>(&mTask.mTask),
						inMilliSecs );
}


// ---------------------------------------------------------------------
//  RemoveTimeTask                                             [private]
// ---------------------------------------------------------------------
// Remove the task from the Time Manager's queue.

inline void
CNetTimeoutMac::RemoveTimeTask()
{
	// attempt to grab timer "semaphore"
	if( ::Dequeue( &mTask.mSemEl, &mTask.mSemQ ) == noErr ){
		::RmvTime( reinterpret_cast<QElemPtr>(&mTask.mTask) );
	}
}

HL_End_Namespace_BigRedH
