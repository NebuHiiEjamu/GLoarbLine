/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#ifndef _H_CIOStream_
#define _H_CIOStream_

#include "CInStream.h"
#include "COutStream.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH


class CIOStream : public CInStream
				, public COutStream
{
	public:
						CIOStream(CStreamBuffer& inBuffer)
							// throws nothing
							: CInStream(inBuffer)
							, COutStream(inBuffer)
							{ }	
		virtual			~CIOStream()
							// throws nothing
							{}							
							
			// ** managing the buffer **
		CStreamBuffer&	GetBuffer()
							// throws nothing
							{ return CInStream::GetBuffer(); }

			// ** operations inherited from the buffer **
		bool			IsOpen()
							// throws CStreamException
							{ return CInStream::GetBuffer().IsOpen(); }
		void			Close()
							// throws CStreamException
							{ CInStream::GetBuffer().Close(); }
		UInt32			Write(const void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ return COutStream::Write(inBuf,inSize); }
		bool			CanWriteSomeMore()
							// throws CStreamException
							{ return COutStream::CanWriteSomeMore(); }
		UInt32			Read(void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ return CInStream::Read(inBuf,inSize); }
		UInt32			GetAvailable()
							// throws CStreamException
							{ return CInStream::GetAvailable(); }
		void 			SetExpect(UInt32 inSize)
							// throws CStreamException
							{ CInStream::SetExpect(inSize); }
		bool			IsEOF()
							// throws CStreamException
							{ return CInStream::IsEOF(); }

		CIOStream&		PutAll(const void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ COutStream::GetBuffer().PutAll(inBuf,inSize); return *this; }
		CIOStream&		GetAll(void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ CInStream::GetBuffer().GetAll(inBuf,inSize); return *this; }
		CIOStream&		PutWithEndianOrdering
							(const void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ COutStream::GetBuffer().PutWithEndianOrdering(inBuf,inSize); return *this; }
		CIOStream&		GetWithEndianOrdering( void* inBuf, UInt32 inSize)
							// throws CStreamException
							{ CInStream::GetBuffer().GetWithEndianOrdering(inBuf,inSize); return *this; }

		void			AddListener( CListener &inListener )
							// throws CMessageException
							{ CInStream::GetBuffer().AddListener(inListener); }

			// ** shift operations **
		CIOStream&		operator >> (UInt32& inVal)
							// throws CStreamException
							{ CInStream::GetBuffer().GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator >> (UInt16& inVal)
							// throws CStreamException
							{ CInStream::GetBuffer().GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator >> (UInt8& inVal)
							// throws CStreamException
							{ CInStream::GetBuffer().GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator >> (SInt32& inVal)
							// throws CStreamException
							{ CInStream::GetBuffer().GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator >> (SInt16& inVal)
							// throws CStreamException
							{ CInStream::GetBuffer().GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator >> (SInt8& inVal)
							// throws CStreamException
							{ CInStream::GetBuffer().GetWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }

		CIOStream&		operator << (UInt32 inVal)
							// throws CStreamException
							{ COutStream::GetBuffer().PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator << (UInt16 inVal)
							// throws CStreamException
							{ COutStream::GetBuffer().PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator << (UInt8 inVal)
							// throws CStreamException
							{ COutStream::GetBuffer().PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator << (SInt32 inVal)
							// throws CStreamException
							{ COutStream::GetBuffer().PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator << (SInt16 inVal)
							// throws CStreamException
							{ COutStream::GetBuffer().PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }
		CIOStream&		operator << (SInt8 inVal)
							// throws CStreamException
							{ COutStream::GetBuffer().PutWithEndianOrdering(&inVal,sizeof(inVal)); return *this; }

			// ** helper function **
		bool			IsSourceFor(const CMessage& inMsg)
							// throws CStreamException
							{ return CInStream::GetBuffer().IsSourceFor(inMsg); }
}; // class CIOStream

HL_End_Namespace_BigRedH
#endif // _H_CIOStream_
