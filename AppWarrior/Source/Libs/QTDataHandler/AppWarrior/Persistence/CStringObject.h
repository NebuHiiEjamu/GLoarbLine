// =====================================================================
//	CStringObject.h                      (C) Hotline Communications 2000
// =====================================================================
// This is a CString and a CFlattenable at the same time. It is used
// to get a string from resources in the same way as Images are. 

#ifndef _H_CStringObject_
#define _H_CStringObject_

#include "CFlattenable.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

class CStringObject : public CFlattenable
					, public CString
{
	public:
							CStringObject();
								// ???
		virtual				~CStringObject();
								// throws nothing

	protected:
			//** CFlattenable overrides **
		virtual void 		AttachInStream(std::auto_ptr<CInStream>& inInStream);
								// throws CMessageException
		virtual void		ListenToMessage( const CMessage &inMessage );
								// throws CMessageException
	private:
		void				TryToReadSomeMore();
								// throws ???

		UInt8				mExtraByte;
		bool				mExtraUsed;
};

HL_End_Namespace_BigRedH
#endif // _H_CStringObject_
