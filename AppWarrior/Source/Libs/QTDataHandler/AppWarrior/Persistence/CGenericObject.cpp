/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CGenericObject.h"
#include "CMessage.h"
#include "CAsyncStreamBuffer.h"

HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  CGenericObject                                  				[public]
// ---------------------------------------------------------------------
// Constructor

CGenericObject::CGenericObject()
{}

// ---------------------------------------------------------------------
//  ~CGenericObject                                 	   [public][virtual]
// ---------------------------------------------------------------------
// Destructor

CGenericObject::~CGenericObject()
{}

// ---------------------------------------------------------------------
//  AttachInStream                                  [protected][virtual]
// ---------------------------------------------------------------------
// This virtual function is called when a CFlattenable get's an instream
// to decode itself from. Call the base class to make the attachement
// than use this chance to read from the stream as much as it's available.

void 
CGenericObject::AttachInStream(std::auto_ptr<CInStream>& inInStream)
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
CGenericObject::ListenToMessage( const CMessage &inMessage )
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

void 
CGenericObject::TryToReadSomeMore()
{
	CInStream& inStream = GetInStream();
	if (inStream.IsOpen())
	{
/* //?? !! let's not do anything with the inStream yet
	THIS NEEDS TO BE CHANGED
		while (true)
		{
			UInt32 inSize = inStream.GetAvailable();
			if (inSize == 0)
				break;
				
			UInt8 buf[8192]; 
			if (inSize > sizeof(buf))
				inSize = sizeof(buf);
			UInt32 szReaded = inStream.Read(buf,inSize);
		}
*/
		// close the stream as soon as possible
		if (inStream.IsEOF())
			inStream.Close();
		// else wait for the next notification
	}	
}

HL_End_Namespace_BigRedH
