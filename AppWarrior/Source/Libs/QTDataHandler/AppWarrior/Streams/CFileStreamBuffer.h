/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// A local file stream
// Although the implementation is asynchronous compliant, local files are 
// synchronous by nature, and do not send notifications after read and write.
// CResourceFile relies on this behaviour so it should be changed if 
// this implementation caracteristic changes...

#ifndef _H_CFileStreamBuffer_
#define _H_CFileStreamBuffer_

#include "CAsyncStreamBuffer.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH
class CFSFileRef;

class CFileStreamBuffer : public CAsyncStreamBuffer
{
	public:
							CFileStreamBuffer();
								// throws CMemoryException
		virtual 			~CFileStreamBuffer();
								// throws nothing

		enum OpenMode
		{
			  eNONE
			, eReadOnly			// if it doesn't exist fail (InStream default)
			, eWrite 			// if it does exist it's truncated, otherwise it's created (OutStream default)
			, eAppend 			// if it doesn't exist it's created, then go to the end of the file
		};
		void 				Open(const CFSFileRef& inFileRef, OpenMode inMode);
								// throws CStreamException
		virtual bool		IsOpen();
								// throws CStreamException
		virtual void		Close();
								// throws CStreamException
		virtual UInt32 		Write(const void* inBuf, UInt32 inSize);
								// throws CStreamException
		virtual bool		CanWriteSomeMore();
								// throws CStreamException
		virtual UInt32		Read(void* inBuf, UInt32 inSize);
								// throws CStreamException
		virtual UInt32		GetAvailable();
								// throws CStreamException
		virtual void 		SetExpect(UInt32 inSize);
								// throws CStreamException
		virtual bool		IsEOF();
								// throws CStreamException
	private:
		OpenMode			mOpenMode;
		class CFileStreamBufferPS;
		CFileStreamBufferPS	&mPlatform;

}; 

HL_End_Namespace_BigRedH
#endif // _H_CFileStreamBuffer_
