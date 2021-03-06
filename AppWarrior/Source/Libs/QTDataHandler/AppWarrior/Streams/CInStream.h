/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#ifndef _H_CInStream_
#define _H_CInStream_

#include "CStreamBuffer.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

class CInStream
{
	public:
						CInStream(CStreamBuffer& inBuffer)
							// throws nothing
							: mBuffer(inBuffer)
							{ }
		virtual			~CInStream()
							// throws nothing
							{}							
			// ** managing the buffer **
		CStreamBuffer&	GetBuffer()
							// throws nothing
							{ return mBuffer; }

			// ** operations inherited from the buffer **
		bool			IsOpen()
							// throws nothing
							{ return mBuffer.IsOpen(); }
		void			Close()
							// throws CStreamException
							{ mBuffer.Close(); }
		UInt32			Read(void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ return mBuffer.Read(inBuf,inSize); }
		UInt32			GetAvailable()
							// throws CStreamException
							{ return mBuffer.GetAvailable(); }
		void 			SetExpect(UInt32 inSize)
							// throws CStreamException
							{ mBuffer.SetExpect(inSize); }
		bool			IsEOF()
							// throws CStreamException
							{ return mBuffer.IsEOF(); }

		CInStream&		GetAll(void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ mBuffer.GetAll(inBuf,inSize); return *this; }
		CInStream&		GetWithEndianOrdering( void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ mBuffer.GetWithEndianOrdering(inBuf,inSize); return *this; }

		void			AddListener( CListener &inListener )
							// throws CMessageException
							{ mBuffer.AddListener(inListener); }

			// ** specific shift operations **
		CInStream&		operator >> (UInt32& inVal)
							// throws CStreamException
							{ mBuffer.GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CInStream&		operator >> (UInt16& inVal)
							// throws CStreamException
							{ mBuffer.GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CInStream&		operator >> (UInt8& inVal)
							// throws CStreamException
							{ mBuffer.GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CInStream&		operator >> (SInt32& inVal)
							// throws CStreamException
							{ mBuffer.GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CInStream&		operator >> (SInt16& inVal)
							// throws CStreamException
							{ mBuffer.GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CInStream&		operator >> (SInt8& inVal)
							// throws CStreamException
							{ mBuffer.GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }

			// ** helper function **
		bool			IsSourceFor(const CMessage& inMsg)
							// throws CStreamException
							{ return mBuffer.IsSourceFor(inMsg); }
	private:
		CStreamBuffer&	mBuffer;	
};

HL_End_Namespace_BigRedH
#endif // _H_CInStream_
