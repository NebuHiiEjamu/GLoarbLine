/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// base class for output streams

#ifndef _H_COutStream_
#define _H_COutStream_

#include "CStreamBuffer.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

class COutStream
{
	public:
						COutStream(CStreamBuffer& inBuffer)
							// throws nothing
							: mBuffer(inBuffer)
							{ }	
		virtual			~COutStream()
							// throws nothing
							{}							
							
			// ** managing the buffer **
		CStreamBuffer&	GetBuffer()
							// throws nothing
							{ return mBuffer; }

			// ** operations inherited from the buffer **
		bool			IsOpen()
							// throws CStreamException
							{ return mBuffer.IsOpen(); }
		void			Close()
							// throws CStreamException
							{ mBuffer.Close(); }
		UInt32			Write(const void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ return mBuffer.Write(inBuf,inSize); }
		bool			CanWriteSomeMore()
							// throws CStreamException
							{ return mBuffer.CanWriteSomeMore(); }

		COutStream&		PutAll(const void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ mBuffer.PutAll(inBuf,inSize); return *this; }
		COutStream&		PutWithEndianOrdering
							(const void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ mBuffer.PutWithEndianOrdering(inBuf,inSize); return *this; }

		void			AddListener( CListener &inListener )
							// throws CMessageException
							{ mBuffer.AddListener(inListener); }

			// ** shift operations **
		COutStream&		operator << (UInt32 inVal)
							// throws CStreamException
							{ mBuffer.PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		COutStream&		operator << (UInt16 inVal)
							// throws CStreamException
							{ mBuffer.PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		COutStream&		operator << (UInt8 inVal)
							// throws CStreamException
							{ mBuffer.PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		COutStream&		operator << (SInt32 inVal)
							// throws CStreamException
							{ mBuffer.PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		COutStream&		operator << (SInt16 inVal)
							// throws CStreamException
							{ mBuffer.PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		COutStream&		operator << (SInt8 inVal)
							// throws CStreamException
							{ mBuffer.PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }

			// ** helper function **
		bool			IsSourceFor(const CMessage& inMsg)
							// throws CStreamException
							{ return mBuffer.IsSourceFor(inMsg); }
	private:
		CStreamBuffer&	mBuffer;	
};

HL_End_Namespace_BigRedH
#endif // _H_COutStream_
