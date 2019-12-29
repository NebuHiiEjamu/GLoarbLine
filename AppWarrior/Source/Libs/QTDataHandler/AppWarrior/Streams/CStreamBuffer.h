/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// the interface for all stream buffers
// How to write:		whenever you want to write check for
//						CanWriteSomeMore(), if it says yes just write
//						if it says no, forget it, you will get a message
//						saying "DataSent" later
// How to read:			if GetAvailable() says there is data just read
//						if it does not than don't do anything because you 
//						will get a message saying "DataReceived" later
// note:	Local file systems will just notify you that there is data
//			available to read (or space to write data) at the begining
//			to initiate the cicle, later on no notification will be sent
//			but you will always get CanWriteSomeMore()==true 
// more:	If you choose to ignore that CanWriteSomeMore()==false the Write
//			will not fail but rather the internal cache will be required to 
//			grow, no problem as long as you don't exagerate ...
//			On the other hand if you choose to ignore GetAvailable()==0
//			the subsequent Read will just read 0 bytes without failing

#ifndef _H_CStreamBuffer_
#define _H_CStreamBuffer_

#include "CStreamException.h"
#include "CEndianOrder.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

class CListener;
class CMessage;

class CStreamBuffer : public CEndianOrder
{
	public:
		virtual				~CStreamBuffer()	
								// throws nothing
								{}
		virtual bool		IsOpen()=0;
								// throws CStreamException
		virtual void		Close()=0;
								// throws CStreamException
		virtual UInt32 		Write(const void* inBuf, UInt32 inSize)=0;
								// throws CStreamException
		virtual bool		CanWriteSomeMore()=0;
								// throws CStreamException
		virtual UInt32		Read(void* inBuf, UInt32 inSize)=0;
								// throws CStreamException
		virtual UInt32		GetAvailable()=0;
								// throws CStreamException
		virtual void 		SetExpect(UInt32 inSize)=0;
								// throws CStreamException
		virtual bool		IsEOF()=0;
								// throws CStreamException
		virtual void		AddListener( CListener &inListener )=0;
								// throws CMessageException

			// ** Support for shift operations **
			// These methods are synchronous by nature
			// and they provide for byte reordering.
		void 				PutAll(const void* inBuf, UInt32 inSize);
								// throws CStreamException
		void 				GetAll(void* inBuf, UInt32 inSize);
								// throws CStreamException
		void 				PutWithEndianOrdering
								(const void* inBuf, UInt32 inSize);
								// throws CStreamException
		void 				GetWithEndianOrdering
								(void* inBuf, UInt32 inSize);
								// throws CStreamException
			// helper for finding out if this stream is the source of the notification
		bool				IsSourceFor(const CMessage& inMsg);
								// throws CStreamException
};


HL_End_Namespace_BigRedH
#endif // _H_CStreamBuffer_
