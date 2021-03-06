// =====================================================================
//  CMessageWrangler.h                   (C) Hotline Communications 1999
// =====================================================================
//
// A mix-in class that works with CListener class to implement message
// queuing and dispatching. A listener is assigned a CMessageWrangler
// (usually a thread or the application) at creation. When the listener
// gets sent a message, it asks the wrangler (if its wrangler is not
// nil) to queue the message for it. If the wrangler is nil, it handles
// the message immediately. The wrangler is responsible for dispatching
// the message back to the listener at the appropriate time
// (with a call to HandleNextMessage or HandleAllMessages). This should
// be called in the message loop of the application, or in the looping
// code of the thread. Both CApplication and CMessagingThread have been
// setup to do this already. This insures that messages received by a
// listener are received in the proper thread context.

#ifndef _H_CMessageWrangler_
#define _H_CMessageWrangler_

#if PRAGMA_ONCE
	#pragma once
#endif					    

#include "CMessage.h"
#include "CMutex.h"

HL_Begin_Namespace_BigRedH

class CListener;


class CMessageWrangler
{
	friend class CListener;
	
	public:
								CMessageWrangler();
									// throws CMemoryException
		virtual					~CMessageWrangler();
									// throws nothing
		
			// ** Message Handling **
		bool					HandleNextMessage();
									// throws CMessageException
		void					HandleAllMessages();
									// throws CMessageException

	private:
		std::list<CListener*>	mListeners;
			// Q of Listener,Message pairs
		typedef std::pair<CListener*,CMessage*>
								ListenerMessagePair;
		std::list<ListenerMessagePair>
								mQueue;

		CMutex					mLLocker; // Lock for Listener Q
		CMutex					mQLocker; // Lock for ListenerMessagePair Q

			// ** Message Q'ing **
		void					QueueMessage( CListener &inListener,
											CMessage *inMessage );
									// throws CMessageException
							
			// ** Listener Management **
		void					RegisterListener( CListener &inListener );
									// throws CMessageException
		void					UnregisterListener( CListener &inListener );
									// throws CMessageException
		
			// ** Copy Constructor **
								CMessageWrangler( const CMessageWrangler &inOriginal );
									// throws nothing


		// A functor to find a listener in a pair, used by the remove_if
		struct FindListener : std::unary_function<ListenerMessagePair,bool>
		{
								FindListener( CListener *inListener )
									// throws nothing
									: mListener( inListener ) {};

			bool				operator()( const ListenerMessagePair &a ) const
									// throws nothing
									{ return a.first == mListener; }

			CListener*			mListener;
		};

};

HL_End_Namespace_BigRedH
#endif // _H_CMessageWrangler_
