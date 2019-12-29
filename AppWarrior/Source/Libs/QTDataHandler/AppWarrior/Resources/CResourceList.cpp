/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CResourceList.h"
#include "CResourceFile.h"
#include "CAsyncStreamBuffer.h"

HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  CResourceList                                 				[public]
// ---------------------------------------------------------------------
// Constructor

CResourceList::CResourceList()
: mResFile(nil)
{}

// ---------------------------------------------------------------------
//  ~CResourceList                                 	   [public][virtual]
// ---------------------------------------------------------------------
// Destructor

CResourceList::~CResourceList()
{
	ASSERT(mResFile != nil);
}

// ---------------------------------------------------------------------
//  AttachResFile                                 	   		   [private]
// ---------------------------------------------------------------------
// Allows the builder of this object to attach itself to it.

void
CResourceList::AttachResourceFile(CResourceFile& inResFile)
{
	mResFile = &inResFile;
	ASSERT(mResFile != nil);
}



// ---------------------------------------------------------------------
//  GetResourceObject                                  			[public]
// ---------------------------------------------------------------------

CRefObj<CFlattenable>&			
CResourceList::GetResourceObject( UInt32 inIndex ) const
{
	try
	{
		if (inIndex >= mResList.size())
			THROW_UNKNOWN_RESOURCE_(eGet);

		UInt32 theID = mResList[inIndex].mResID;
		CResourceFile::Type theType = mResList[inIndex].mResType;
		
		ASSERT(mResFile != nil);
		return mResFile->GetResourceObject(theType, theID);
	}
	catch (...)
	{
		RETHROW_RESOURCE_(eGet);
		return * (CRefObj<CFlattenable>*) nil; //stupid compiler
	}	
}

// ---------------------------------------------------------------------
//  GetSize			                                  			[public]
// ---------------------------------------------------------------------
// returns how many resources are refered by this resList

UInt32
CResourceList::GetSize( ) const
{
	try
	{
		return mResList.size();
	}
	catch (...)
	{
		return 0;
	}	
}


// ---------------------------------------------------------------------
//  GetResourceIdOf	                                  			[public]
// ---------------------------------------------------------------------
// returns the resourceId for the specified entry

UInt32
CResourceList::GetResourceIdOf(UInt32 inIndex) const
{
	UInt32 id = 0;
	try
	{
		if (inIndex >= mResList.size())
			THROW_UNKNOWN_RESOURCE_(eGet);
		id = mResList[inIndex].mResID;
	}
	catch (...)
	{
		RETHROW_RESOURCE_(eGet);
	}	
	return id;
}

// ---------------------------------------------------------------------
//  GetResourceTypeOf	                               			[public]
// ---------------------------------------------------------------------
// returns the resourceType for the specified entry

CResourceFile::Type
CResourceList::GetResourceTypeOf(UInt32 inIndex) const
{
	CResourceFile::Type theType;
	try
	{
		if (inIndex >= mResList.size())
			THROW_UNKNOWN_RESOURCE_(eGet);
		theType = mResList[inIndex].mResType;
	}
	catch (...)
	{
		RETHROW_RESOURCE_(eGet);
	}
	return theType;
}


// ---------------------------------------------------------------------
//  AttachInStream                                  [protected][virtual]
// ---------------------------------------------------------------------
// This virtual function is called when a CFlattenable get's an instream
// to decode itself from. Call the base class to make the attachement
// than use this chance to read from the stream as much as it's available.

void 
CResourceList::AttachInStream(std::auto_ptr<CInStream>& inInStream)
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
CResourceList::ListenToMessage( const CMessage &inMessage )
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

void 
CResourceList::TryToReadSomeMore()
{
	CInStream& inStream = GetInStream();
	if (inStream.IsOpen())
	{
		const UInt8 eEntrySize = 8;
		inStream.SetExpect(eEntrySize);
		// optimize the allocation
		mResList.reserve(inStream.GetAvailable()/eEntrySize);
		while (inStream.GetAvailable() >= eEntrySize)
		{
			UInt32 resType, resID;
			// stream reading does the cross platform byte ordering
			inStream >> resType >> resID;
			// create an entry
			ListEntry resListEntry;
			resListEntry.mResType	= CResourceFile::Type(resType);
			resListEntry.mResID		= resID;
			// push the entry in the table
			mResList.push_back(resListEntry);
		}
		
		// close the stream as soon as possible
		if (inStream.IsEOF())
			inStream.Close();
		// else wait for the next notification
	}	
}

HL_End_Namespace_BigRedH
