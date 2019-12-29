/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// base class for CImage CSound and any other flattenable object

#include "AW.h"
#include "CFlattenable.h"
#include "CPersistenceException.h"
#include "CMessage.h"
#include "CAsyncStreamBuffer.h"
#include "TObjectFactory.h"
#include "CStreamException.h"

#include "CGenericObject.h"
//#include "CImagePNG.h"
#include "CStringObject.h"
#include "CResourceList.h"

HL_Begin_Namespace_BigRedH


struct MimeToObject
{
	CString::value_type* mimeType;
	CFlattenable* (*CreateFunc) ();
};

// ---------------------------------------------------------------------
//  _sMimeToObjectTable		                         	  		[static]
// ---------------------------------------------------------------------

static MimeToObject  _sMimeToObjectTable [] =
{
  	{ L"resource/list", 	TObjectFunctor<CResourceList,CFlattenable>::Create }
  , { L"text/plain", 		TObjectFunctor<CStringObject,CFlattenable>::Create }
//  , { L"image/png", 		TObjectFunctor<CImagePNG,CFlattenable>::Create }
	// WHEN UPDATING this table don't forget to update 
	// Resources/CResourceFile.cpp _sTypeToMimeTable too
};

// ---------------------------------------------------------------------
//  GetObjectFromType		                         	[public][static]
// ---------------------------------------------------------------------
// Create a flattenable object like a CImage or CSound out of a stream
// given the mime type.
// Parse the table of known mime types and create the appropriate object.

std::auto_ptr<CFlattenable> 
CFlattenable::GetObjectFromType( const CString& inType )
{
	try
	{
		UInt32 tableLength = sizeof(_sMimeToObjectTable) / sizeof(*_sMimeToObjectTable);
		UInt32 foundAt;
		for(foundAt = 0; foundAt < tableLength; foundAt++)
		{
			if (0 == inType.compare(_sMimeToObjectTable[foundAt].mimeType))
				break;
		}
		if (foundAt >= tableLength)
		{
			// don't fail but rather return the CGenericObject for unknown types
			return std::auto_ptr<CFlattenable>(TObjectFunctor<CGenericObject,CFlattenable>::Create());
			//ASSERT(false);
			//THROW_UNKNOWN_PERSISTENCE_(eCreateObject);
		}
		return std::auto_ptr<CFlattenable>((_sMimeToObjectTable[foundAt].CreateFunc) ());
	}
	catch (...) { RETHROW_PERSISTENCE_(eCreateObject); }
	return std::auto_ptr<CFlattenable>(nil);
}									  	



// ---------------------------------------------------------------------
//  CFlattenable                                       			[public]
// ---------------------------------------------------------------------
// constructor

CFlattenable::CFlattenable()
{
}


// ---------------------------------------------------------------------
//  ~CFlattenable                                      [public][virtual]
// ---------------------------------------------------------------------
// destructor

CFlattenable::~CFlattenable()
{}

// ---------------------------------------------------------------------
//  GetInStream		                                    	 [protected]
// ---------------------------------------------------------------------
// Helper function for derived classes, allows you to acces the inStream
// when DataReceived notifications arrive. 

CInStream&	
CFlattenable::GetInStream()
{
	if (mInStream.get() == nil)
		THROW_UNKNOWN_STREAM_(eGet);
	return *mInStream.get();
}

// ---------------------------------------------------------------------
//  AttachInStream                                     [public][virtual]
// ---------------------------------------------------------------------
// Gives the derived classes a chance to read the initial data from 
// the stream.
// Any function that overrides this one should call the base function first.

void 
CFlattenable::AttachInStream(std::auto_ptr<CInStream>& inInStream)
{
	(mInStream = inInStream)->AddListener(*this);
	// make shure the object gets the chance to read the data
	// by calling the initial reads here
	// if (mInStream->IsOpen())
	//		TryToReadSomeMoreData();
}

// ---------------------------------------------------------------------
//  ListenToMessage                                 [protected][virtual]
// ---------------------------------------------------------------------
// This virtual function is called when a CFlattenable receives a 
// notification from the stream saying that it has closed.
// Use this chance to delete the stream.

void
CFlattenable::ListenToMessage( const CMessage &inMessage )
{
	try
	{
		switch (inMessage.GetID())
		{
		case CAsyncStreamBuffer::eClosed:
			{	
				const CAsyncStreamBuffer::Message* msgS =
					 static_cast <const CAsyncStreamBuffer::Message*> (&inMessage);
				if (mInStream.get() != nil && mInStream->IsSourceFor(inMessage))
				{	// remove the stream as soon as it's closed
					mInStream = std::auto_ptr<CInStream> (nil);
				}
			}	
			break;
		} // switch	
	}
	catch (...) { RETHROW_MESSAGE_(eListenToMessage); }
}


HL_End_Namespace_BigRedH
