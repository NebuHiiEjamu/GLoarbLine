/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CStringObject.h"
#include "CMessage.h"
#include "CAsyncStreamBuffer.h"

HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  CStringObject                                  				[public]
// ---------------------------------------------------------------------
// Constructor

CStringObject::CStringObject()
: mExtraUsed(false)
{}

// ---------------------------------------------------------------------
//  ~CStringObject                                 	   [public][virtual]
// ---------------------------------------------------------------------
// Destructor

CStringObject::~CStringObject()
{}

// ---------------------------------------------------------------------
//  AttachInStream                                  [protected][virtual]
// ---------------------------------------------------------------------
// This virtual function is called when a CFlattenable get's an instream
// to decode itself from. Call the base class to make the attachement
// than use this chance to read from the stream as much as it's available.

void 
CStringObject::AttachInStream(std::auto_ptr<CInStream>& inInStream)
{
	try
	{
		CFlattenable::AttachInStream(inInStream);
		TryToReadSomeMore();
	}
	catch(...)
	{
		RETHROW_MESSAGE_(eListenToMessage);
	}	
}

// ---------------------------------------------------------------------
//  ListenToMessage                                 [protected][virtual]
// ---------------------------------------------------------------------
// This virtual function is called when a CFlattenable receives a 
// notification from the stream saying that it has more data to be read.

void
CStringObject::ListenToMessage( const CMessage &inMessage )
{
	try
	{
		if (inMessage.GetID() == CAsyncStreamBuffer::eDataRcv)
			TryToReadSomeMore();
		else	
			CFlattenable::ListenToMessage(inMessage);
	}
	catch(...)
	{
		RETHROW_MESSAGE_(eListenToMessage);
	}	
}

// ---------------------------------------------------------------------
//  TryToReadSomeMore                                 		   [private]
// ---------------------------------------------------------------------
// Do the actual reading from the stream.
// Because the stream might read an odd number of bytes while the 
// CString is always even sized we must remember the ExtraByte
// between string reads

void 
CStringObject::TryToReadSomeMore()
{
	CInStream& inStream = GetInStream();
	if (inStream.IsOpen())
	{
		while (true)
		{
			UInt32 inSize = inStream.GetAvailable();
			if (inSize == 0)
				break;

			const UInt32 bufAlloc = 8192;
			UInt8 buf[bufAlloc+2]; // extra space for EOS

			UInt32 bufSz = bufAlloc;
			UInt8* bufRd = buf;
			
			// do we carry some extra chars from the previous read
			if (mExtraUsed)
			{
				*bufRd++ = mExtraByte;
				bufSz--;
				mExtraUsed = false;
			}
			
			if (inSize > bufSz)
				inSize = bufSz;
			
			UInt32 szReaded = inStream.Read(bufRd,inSize);
			szReaded += UInt32(bufRd-buf);
			
			// if an odd number of bytes was read remember 
			// the extra byte
			if ((szReaded & 1) != 0)
			{
				mExtraUsed = true;
				mExtraByte = buf[szReaded];
				--szReaded;
			}	
			// make the buffer a CString
			*(CString::value_type*)(buf+szReaded) = L'\0';
			
			// add the buffer to the current CString
			*(CString*)this += (CString::value_type*) buf;
		}
		// close the stream as soon as possible
		if (inStream.IsEOF())
			inStream.Close();
		// else wait for the next notification
	}	
}

HL_End_Namespace_BigRedH
