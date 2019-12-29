/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "HotlineFolderDownload.h"


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */

CMyDLItem::CMyDLItem(SFSListItem *inItem, CMyDLFldr *inParent, bool inDBAccess)
{
	if (inItem->typeCode == TB('fldr'))
	{
		mFile = nil;
		TFSRefObj* pFldr = UFS::New(inParent->GetFolder(), nil, inItem->name, fsOption_ResolveAliases);

		if (!pFldr->Exists())
		{
			mFldr = nil;
			delete pFldr;
				
			return;
		}
		
		// don't add this item if it's already in the list
		// there's a lot of overhead in these calls...perhaps have an option in the server to never ever ever never resolve aliases?
		CMyDLFldr *pParent = inParent;
		while (pParent)
		{
			if (pParent->HasItem(pFldr))
			{
				mFldr = nil;
				delete pFldr;
				
				return;
			}

			pParent = pParent->GetParent();
		}
		
		mFldr = new CMyDLFldr(pFldr, inParent, inDBAccess);
	}
	else
	{
		mFldr = nil;
		mFile = UFS::New(inParent->GetFolder(), nil, inItem->name, fsOption_ResolveAliases);

		if (!mFile->Exists())
		{
			delete mFile;
			mFile = nil;
				
			return;
		}

		CMyDLFldr *pParent = inParent;
		while (pParent)
		{
			if (pParent->HasItem(mFile))
			{
				delete mFile;
				mFile = nil;
				
				return;
			}

			pParent = pParent->GetParent();
		}
	}
}

CMyDLItem::~CMyDLItem()
{
	if (mFile)
		delete mFile;
	else if (mFldr)
		delete mFldr;
}

TFSRefObj* CMyDLItem::GetNextItem(Uint8*& ioPathStart, Int16& ioMaxPathSize, Uint16& ioCount, bool *outIsFolder)
{
	if (mFile)
	{
		if (mIsDone)
			return nil;
			
		*outIsFolder = false;
		
		Uint8 name[256];
		mFile->GetName(name);
		Int16 myPathSize = name[0] + 1 + 2;
				
		if (ioMaxPathSize < myPathSize)
			ioMaxPathSize = -1;
		else
		{
			ioMaxPathSize -= myPathSize;
			
			ioPathStart -= name[0] + 1;
			pstrcpy(ioPathStart, name);
			ioPathStart -= 2;
			(*((Uint16 *)ioPathStart)) = 0;
			ioCount++;
		}

		mIsDone = true;
		return mFile;
	}
	else if (mFldr)
		return mFldr->GetNextItem(ioPathStart, ioMaxPathSize, ioCount, outIsFolder);

	return nil;
}

bool CMyDLItem::HasItem(TFSRefObj* inRef)
{
	if (mFile)
		return mFile->Equals(inRef);
	else if (mFldr)
		return mFldr->HasItem(inRef);	

	return false;
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

CMyDLFldr::CMyDLFldr(TFSRefObj* inRef, CMyDLFldr *inParent, bool inDBAccess)
{
	mRef = inRef;
	mParent = inParent;
	
	mCurrentItem = 0;
	static Uint8 lower[256];
	
	THdl h = UFS::GetListing(inRef);
	scopekill(THdlObj, h);
	if (h)
	{	
		void *p;
		StHandleLocker lock(h, p);
		Uint8 *endPtr = BPTR(p) + UMemory::GetSize(h);
		Uint32 count = *(Uint32 *)p;
		
		SFSListItem *item = (SFSListItem *)(BPTR(p) + 4);
		mItems.Preallocate(count);
				
		while (count--)
		{
			if ((BPTR(item) + sizeof(SFSListItem)) >= endPtr) break;
						
			pstrcpy(lower, item->name);
			UText::MakeLowercase(lower + 1, lower[0]);

			if (inDBAccess || !UMemory::Search("drop box", 8, lower + 1, lower[0]))
				mItems.AddItem(new CMyDLItem(item, this, inDBAccess));
			
			BPTR(item) += (sizeof(SFSListItem) + item->name[0] + 4) & 0xFFFFFFFC;
		}
	}
}

CMyDLFldr::~CMyDLFldr()
{
	CMyDLItem *itm;
	Uint32 index = 0;

	while (mItems.GetNext(itm, index))
		delete itm;
	
	mItems.Clear();
			
	if (mRef)
		delete mRef;
}

Uint32 CMyDLFldr::GetTotalItems()
{
	Uint32 count = 1;
	
	CMyDLItem *itm;
	Uint32 index = 0;

	while (mItems.GetNext(itm, index))
		count += itm->GetTotalItems();

	return count;
}

Uint32 CMyDLFldr::GetTotalSize()
{
	Uint32 size = 0;
	
	CMyDLItem *itm;
	Uint32 index = 0;

	while (mItems.GetNext(itm, index))
		size += itm->GetTotalSize();

	return size;
}

TFSRefObj* CMyDLFldr::GetNextItem(Uint8*& ioPathStart, Int16& ioMaxPathSize, Uint16& ioCount, bool *outIsFolder)
{
	Uint8 name[256];
	Int16 myPathSize;
	
	if (mCurrentItem == 0)
	{
		mCurrentItem++;
		*outIsFolder = true;
		
		mRef->GetName(name);
		myPathSize = name[0] + 1 + 2;
		
		if (ioMaxPathSize < myPathSize)
			ioMaxPathSize = -1;
		else
		{
			ioMaxPathSize -= myPathSize;
			
			ioPathStart -= name[0] + 1;
			pstrcpy(ioPathStart, name);
			ioPathStart -= 2;
			(*((Uint16 *)ioPathStart)) = 0;
			ioCount++;
		}
		
		return mRef;
	}
	
	// if the item is null, but we still have items, let's get the next item (ie, skip blanks)
	Uint32 count = mItems.GetItemCount();
	
	while (mCurrentItem != count + 1)
	{
		TFSRefObj* itm = mItems.GetItem(mCurrentItem)->GetNextItem(ioPathStart, ioMaxPathSize, ioCount, outIsFolder);
		if (itm)
		{
			mRef->GetName(name);
			myPathSize = name[0] + 1 + 2;
			
			if (ioMaxPathSize < myPathSize)
				ioMaxPathSize = -1;
			else
			{
				ioMaxPathSize -= myPathSize;
				
				ioPathStart -= name[0] + 1;
				pstrcpy(ioPathStart, name);
				ioPathStart -= 2;
				(*((Uint16 *)ioPathStart)) = 0;
				ioCount++;
			}
	
			return itm;
		}

		mCurrentItem++;
	}
	
	return nil;
}

bool CMyDLFldr::HasItem(TFSRefObj* inRef)
{
	if (mRef->Equals(inRef))
		return true;
	
	CMyDLItem *itm;
	Uint32 index = 0;

	while (mItems.GetNext(itm, index))
		if (itm->HasItem(inRef))
			return true;
	
	return false;
}

