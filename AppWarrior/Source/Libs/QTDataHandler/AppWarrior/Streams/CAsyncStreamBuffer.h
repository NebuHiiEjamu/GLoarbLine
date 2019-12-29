/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#ifndef _H_CAsyncStreamBuffer_
#define _H_CAsyncStreamBuffer_

#include "CStreamBuffer.h"
#include "CBroadcaster.h"
#include "CMessage.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

class CAsyncStreamBuffer : public CStreamBuffer
						 , public CBroadcaster
{
	public:
						CAsyncStreamBuffer()
							// throws ???
							: mReceivedRecursion(0)
							, mSentRecursion(0)
							{}
			// ** the way notification messages look like **
		enum 			NotifyID
							{ 
								  eDataRcv = 'Drcv'
								, eDataSnt = 'Dsnt' 
								, eClosed  = 'DCls' 
							};
		typedef TObjectMessageS<bool, CAsyncStreamBuffer&> Message;

		virtual void	AddListener( CListener &inListener )
							// throws CMessageException
							{
								CBroadcaster::AddListener(inListener);
							}
	protected:					
		void 			NotifyDataReceived()
							// throws CMessageException
							{ 
								if (++mReceivedRecursion == 1)
									BroadcastMessage(Message(eDataRcv,true,*this));	
								--mReceivedRecursion;
							}
		void			NotifyDataSent()
							// throws CMessageException
							{ 
								if (++mSentRecursion == 1)
									BroadcastMessage(Message(eDataSnt,true,*this));	
								--mSentRecursion;
							}
		void 			NotifyStreamClosed(bool inSuccess)
							// throws CMessageException
							{ 
								BroadcastMessage(Message(eClosed,inSuccess,*this));	
							}
	private:
		UInt8 			mReceivedRecursion;
		UInt8 			mSentRecursion;
}; // class CAsyncStreamBuffer

HL_End_Namespace_BigRedH
#endif // _H_CAsyncStreamBuffer_
