/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
//
// A mix-in class that works with CListener class to implement
// dependencies. A Broadcaster sends messages to its Listeners.
//
// ** How To Use
// CBroadcaster is an abstract, mix-in class. Broadcasters have a list
// of Listeners to which they send messages via the BroadcastMessage()
// function. You attach a Listener to a Broadcaster using the
// AddListener() function.
//
// Classes derived from CBroadcaster should call the BroadcastMessage()
// function whenever they want to announce something--usually a change
// in state. For example, the destructor for CBroadcaster sends a
// msg_BroadcasterDied message to its Listeners.
//
// ** Public Interface
// * Construction:
//		CBroadcaster();
//			Default constructor. Broadcaster has no Listeners
//
// * Linking to Listeners:
//		void	AddListener(CListener *inListener);
//		void	RemoveListener(CListener *inListener);
//			Attaches/Detaches a Listener to/from a Broadcaster. These
//			functions call the appropriate Listener functions to maintain
//			the crosslinks between the Broadcaster and Listener.
//
// * Sending Messages:
//		void	BroadcastMessage(MessageT inMessage, 
//								 void *ioParam = nil) const;
//			Sends the specified message and parameter to all Listeners.
//			The meaning of the parameter depends on the message.
//
// * Broadcasting State:
//		void	StartBroadcasting();
//		void	StopBroadcasting();
//		Boolean	IsBroadcasting() const;
//			Turn off/on, Inspect broadcasting state. A Broadcaster that is
//			not broadcasting does not send messages to its Listeners (i.e.,
//			BroadcastMessage does nothing). Use Stop/Start Broadcasting
//			to temporarily alter dependencies.

#include "AW.h"
#include "CBroadcaster.h"
#include "CListener.h"
#include "CMessage.h"
#include "CMessageException.h"
#include "USyncroHelpers.h"

using namespace std;

HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//	CBroadcaster                                                [public]
// ---------------------------------------------------------------------
// Constructor

CBroadcaster::CBroadcaster()
	: mDeletionCount( 0 )
{
	mIsBroadcasting = true;
}


// ---------------------------------------------------------------------
//  CBroadcaster                                                [public]
// ---------------------------------------------------------------------
// Copy Constructor
// Makes a shallow copy; Listener links are not copied.

CBroadcaster::CBroadcaster( const CBroadcaster &inOriginal )
{
	mIsBroadcasting = inOriginal.mIsBroadcasting;
	mDeletionCount = inOriginal.mDeletionCount;
}


// ---------------------------------------------------------------------
//  CBroadcaster                                       [public][virtial]
// ---------------------------------------------------------------------
// Destructor

CBroadcaster::~CBroadcaster()
{
	try {
			// Notify Listeners that Broadcaster is going away
		CMessage theMsg( msg_BL_BroadcasterDied );
		BroadcastMessage( theMsg );

			// Tell all Listeners to remove this Broadcaster
		list<CListener*> tmpList( mListeners );
		list<CListener*>::iterator it;
		for( it = tmpList.begin(); it != tmpList.end(); it++ ){
			(*it)->RemoveBroadcaster( *this );
		}
	} catch( ... ){
	}
}


// ---------------------------------------------------------------------
//  AddListener                                                 [public]
// ---------------------------------------------------------------------
// Add a Listener to a Broadcaster
//
// Usage note: This is the public interface for associating Broadcasters
// and Listeners.
//    Right:  theBroadcaster->AddListener(theListener);
//    Wrong:  theListener->AddBroadcaster(theBroadcaster);
//
// This function takes care of notifying the Listener to update its
// list of Broadcasters.

void
CBroadcaster::AddListener( CListener &inListener )
{
	bool addBroad = false;
	try {
		{
				StMutex lock(mLLock);
				if( mDeletionSet.count( &inListener ) > 0 ){
					mDeletionSet.erase( &inListener );
					addBroad = true;
				} else if( !HasListener( inListener ) ){ // Add if not already a Listener
					mListeners.push_back( &inListener );
					addBroad = true;
				}
		}
		if( addBroad ){
			inListener.AddBroadcaster( *this );		
		}
	} catch( ... ){
		RETHROW_MESSAGE_( eAddListener );
	}
}


// ---------------------------------------------------------------------
//  RemoveListener                                              [public]
// ---------------------------------------------------------------------
// Remove a Listener from a Broadcaster
//
// Usage note: This is the public interface for dissociating
// Broadcasters and Listeners.
//      Right:  theBroadcaster->RemoveListener(theListener);
//      Wrong:  theListener->RemoveBroadcaster(theBroadcaster);
//
// This function takes care of notifying the Listener to update its
// list of Broadcasters.

void
CBroadcaster::RemoveListener( CListener &inListener )
{
	try {
		{
				StMutex lock(mLLock);
				if( mDeletionCount == 0 ){
					mListeners.remove( &inListener );
				} else {
					mDeletionSet.insert( &inListener );
				}
		}
		inListener.RemoveBroadcaster( *this );
	} catch( ... ){
		RETHROW_MESSAGE_( eRemoveListener );
	}
}


// ---------------------------------------------------------------------
//	HasListener                                                 [public]
// ---------------------------------------------------------------------
// Return whether the Broadcaster has the specified Listener

bool
CBroadcaster::HasListener( const CListener &inListener )
{
	try {
		list<CListener*>::iterator found;
		found = find( mListeners.begin(), mListeners.end(), &inListener );
		return (found != mListeners.end());
	} catch( ... ){
		return false;
	}
}


// ---------------------------------------------------------------------
//	BroadcastMessage                                            [public]
// ---------------------------------------------------------------------
// Send message to all associated Listeners
//
// Does not send message if broadcasting is turned off

void
CBroadcaster::BroadcastMessage( const CMessage &inMessage )
{
	if( mIsBroadcasting ) {
		try {
			StDeletion theDeleter( *this );
			list<CListener*> tmpList( mListeners );
			list<CListener*>::iterator it;
			for( it = tmpList.begin(); it != tmpList.end(); it++ ){
				if( mDeletionSet.count( *it ) == 0 ){
					(*it)->Listen( inMessage );
				}
			}
		} catch( ... ){
			RETHROW_MESSAGE_( eBroadcastMessage );
		}
	}
}


// ---------------------------------------------------------------------
//	DeletionClose                                              [private]
// ---------------------------------------------------------------------
// Send message to all associated Listeners
//

void
CBroadcaster::DeletionClose()
{
	mDeletionCount--;
	if( mDeletionCount == 0 ){
		try {
			set<CListener*>::iterator it;
			for( it = mDeletionSet.begin(); it != mDeletionSet.end(); it++ ){
				mListeners.remove( *it );
			}
		} catch( ... ){
			RETHROW_MESSAGE_( eRemoveListener );
		}
	}
}

HL_End_Namespace_BigRedH
