// =====================================================================
//	CMessage.h                           (C) Hotline Communications 1999
// =====================================================================
//
// Base message class. These objects are send by Broadcasters and
// received by Listeners

#ifndef _H_CMessage_
#define _H_CMessage_

#include "CMessageException.h"

#if PRAGMA_ONCE
	#pragma once
#endif

				 
HL_Begin_Namespace_BigRedH

class CMessage {
	public:
							CMessage( UInt32 inID );
								// throws nothing
		virtual				~CMessage();
								// throws nothing

		virtual CMessage*	Clone() const
								// throws nothing
								{ return new CMessage( *this ); }

				// ** Accessors **
		UInt32				GetID() const
								// throws nothing
								{ return mID; }

							// Comparison op
		bool				operator == ( const CMessage &inRHS ) const
								// throws nothing
								{ return mID == inRHS.mID; }

	protected:
							CMessage( const CMessage &inOriginal );

		UInt32				mID;

	private:
						CMessage();
};



template <class T>
class TObjectMessage : public CMessage 
{
	public:
							TObjectMessage( UInt32 inID, T inObject)
								: CMessage( inID )
								, mObject( inObject )
								{}

		virtual CMessage*	Clone() const
								{ return new TObjectMessage( *this ); }

				// ** Accessors **
		T					GetMsg() const
								{ return mObject; }
	protected:
							TObjectMessage( const TObjectMessage &inOriginal )
								: CMessage(inOriginal)
								, mObject( inOriginal.mObject)
								{}
	private:
		T					mObject;
							TObjectMessage();
}; // class TObjectMessage<T,void>

template <class T, class S>
class TObjectMessageS : public CMessage 
{
	public:
							TObjectMessageS( UInt32 inID, T inObject, S inSource )
								: CMessage( inID )
								, mObject( inObject )
								, mSource( inSource )
								{}

		virtual CMessage*	Clone() const
								{ return new TObjectMessageS( *this ); }

				// ** Accessors **
		T					GetMsg() const
								{ return mObject; }
		S					GetSource() const
								{ return mSource; }

	protected:
							TObjectMessageS( const TObjectMessageS &inOriginal )
								: CMessage(inOriginal)
								, mObject( inOriginal.mObject)
								, mSource( inOriginal.mSource)
								{}

		T					mObject;
		S					mSource;
	private:
							TObjectMessageS();
}; // class TObjectMessage<T,S>

template <class S>
class TObjectMessageSNoT : public CMessage 
{
	public:
							TObjectMessageSNoT( UInt32 inID, S inSource )
								: CMessage( inID )
								, mSource( inSource )
								{}

		virtual CMessage*	Clone() const
								{ return new TObjectMessageSNoT( *this ); }

				// ** Accessors **
		S					GetSource() const
								{ return mSource; }

	protected:
							TObjectMessageSNoT( const TObjectMessageSNoT &inOriginal )
								: CMessage(inOriginal)
								, mSource( inOriginal.mSource)
								{}

		S					mSource;
	private:
							TObjectMessageSNoT();
}; // class TObjectMessage<void,S>


HL_End_Namespace_BigRedH
#endif // _H_CMessage_
