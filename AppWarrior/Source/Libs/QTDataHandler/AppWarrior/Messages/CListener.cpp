/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
//
// An abstract mix-in class that works with CBroadcaster class to
// implement dependencies. A Listener receives messages from its 
// Broadcasters.
//
// ** Public Interface
// * Construction:
//		CListener();
//			Default constructor. Listener has no Broadcasters
//
// * Linking to Broadcasters:
//		Link a Listener to a Broadcaster by sending an AddListener message
//		to a Broadcaster. [see CBroadcaster]
//
// * Responding to Messages:
//		void	ListenToMessage(MessageT inMessage, void *ioParam);
//			Derived classes *must* override this function (it is a
//			pure virtual function in CListener).
//			LCBroadcaster::BroadcastMessage calls this function.
//
//		Example Implementation:
//
//		{
//			switch (inMessage) {
//
//				case msg_NameChanged:	// ioParam is a StringPtr
//					DoNameChanged((StringPtr) ioParam);
//					break;
//
//				case msg_NewValue:		// ioParam is a long*
//					DoNewValue(*(long *) ioParam);
//					break;
//			}
//		}
//
//		The programmer can define message constants and associated
//		meanings for the ioParam parameter.
//
//		A Broadcaster always sends an msg_BroadcasterDied message, with
//		a pointer to itself as the parameter, before it is deleted.
//
// * Listening State:
//		void	StopListening();
//		void	StartListening();
//		Boolean	IsListening();
//			Turn off/on, Inspect listening state. A Listener that is
//			not listening does not receive messages from its Broadcasters.
//			Use Stop/Start Listening to temporarily alter dependencies.

#include "AW.h"
#include "CListener.h"
#include "CBroadcaster.h"
#include "CMessage.h"
#include "CMessageWrangler.h"
#include "CThreadManager.h"
#if BUILDING_APP_
	#include "CApplication.h"
#endif
#include "CMessageException.h"

using namespace std;


HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  CListener                                                   [public]
// ---------------------------------------------------------------------
// Constructor

CListener::CListener()
	: mWrangler( nil ), mIsListening( true )
{
	try {
		CThread *currThread = CThreadManager::Instance().GetCurrentThread();
		if( currThread == nil ){
			//??? a hack for non applications, review when time available
			#if BUILDING_APP_
				mWrangler = &CApplication::Instance();
			#else
				mWrangler = nil;
			#endif
		} else {
			mWrangler = dynamic_cast<CMessageWrangler*>(currThread);
		}
		if( mWrangler != nil ){
			mWrangler->RegisterListener( *this );
		}	
	} catch( ... ){
		RETHROW_MESSAGE_( eCreateListener );
	}
}


// ---------------------------------------------------------------------
//  CListener                                                   [public]
// ---------------------------------------------------------------------
// Constructor

CListener::CListener( CMessageWrangler *inWrangler )
	: mWrangler( inWrangler ), mIsListening( true )
{
	try {
		if( mWrangler != nil ){
			mWrangler->RegisterListener( *this );
		}	
	} catch( ... ){
		RETHROW_MESSAGE_( eCreateListener );
	}
}


// ---------------------------------------------------------------------
//  CListener                                                   [public]
// ---------------------------------------------------------------------
// Copy Constructor
// Makes a shallow copy; Broadcaster links are not copied.

CListener::CListener( const CListener &inOriginal )
	: mWrangler( inOriginal.mWrangler ),
		mIsListening( inOriginal.mIsListening )
{
	try {
		if( mWrangler != nil ){
			mWrangler->RegisterListener( *this );
		}
	} catch( ... ){
		RETHROW_MESSAGE_( eCreateListener );
	}

}


// ---------------------------------------------------------------------
//  ~CListener                                         [public][virtual]
// ---------------------------------------------------------------------
// Destructor. remove all the listeners

CListener::~CListener()
{
	try {
		list<CBroadcaster*> tmpList( mBroadcasters );
		list<CBroadcaster*>::iterator it;
		for( it = tmpList.begin(); it != tmpList.end(); it++ ){
			(*it)->RemoveListener( *this );
		}
		if( mWrangler != nil ){
			mWrangler->UnregisterListener( *this );
		}
	} catch( ... ){
	}
}


// ---------------------------------------------------------------------
//  HasBroadcaster                                              [public]
// ---------------------------------------------------------------------
// Return whether a Listener has the specified Broadcaster

bool
CListener::HasBroadcaster( const CBroadcaster &inBroadcaster )
{
	try {
		list<CBroadcaster*>::iterator found;
		found = find( mBroadcasters.begin(), mBroadcasters.end(), &inBroadcaster );
		return (found != mBroadcasters.end());
	} catch( ... ){
		return false;
	}
}


// ---------------------------------------------------------------------
//  AddBroadcaster                                           [protected]
// ---------------------------------------------------------------------
// Add a Broadcaster to a Listener
//
// You should not call this function directly.
// CBroadcaster::AddListener will call this function
//		Right:	theBroadcaster->AddListener(theListener);
//		Wrong:	theListener->AddBroadcaster(theBroadcaster);

void
CListener::AddBroadcaster( CBroadcaster &inBroadcaster )
{
	try {
		if( !HasBroadcaster( inBroadcaster ) ){
			mBroadcasters.push_back( &inBroadcaster );
		}
	} catch( ... ){
		RETHROW_MESSAGE_( eAddBroadcaster );
	}
}


// ---------------------------------------------------------------------
//  RemoveBroadcaster                                        [protected]
// ---------------------------------------------------------------------
// Remove a Broadcaster from a Listener
//
// You should not call this function directly. 
// CBroadcaster::RemoveListener will call this function
//		Right:	theBroadcaster->RemoveListener(theListener);
//		Wrong:	theListener->RemoveBroadcaster(theBroadcaster);

void
CListener::RemoveBroadcaster( CBroadcaster &inBroadcaster )
{
	try {
		mBroadcasters.remove( &inBroadcaster );
	} catch( ... ){
		RETHROW_MESSAGE_( eRemoveBroadcaster );
	}
}


// ---------------------------------------------------------------------
//  Listen                                                      [public]
// ---------------------------------------------------------------------
// Pass the message off to the Wrangler if one exists, else handle it
// in the ListenToMessage function

void
CListener::Listen( const CMessage &inMessage )
{
	try {
		if( mWrangler == nil ){ // No wrangler
			ListenToMessage( inMessage );
		} else {
			mWrangler->QueueMessage( *this, inMessage.Clone() );
		}
	} catch( ... ){
		RETHROW_MESSAGE_( eListenToMessage );
	}
}

HL_End_Namespace_BigRedH
