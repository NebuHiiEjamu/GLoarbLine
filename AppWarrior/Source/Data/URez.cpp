
/****************************************

For URez::LoadItem(), allow inRef to be a special kSearchRezChain constant ?

Oh but the Release needs to be fixed first so it doesn't require inRef...
(if that's how we decide to do it)


***************************************************/




/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */


/*
Resource file format for storing data items referenced by a type code and ID
number.   Designed to be read from efficiently and quickly, written
infrequently.

-- FILE FORMAT --

// header
Uint32 format;					// always AWRZ
Uint16 version;					// currently 1
Uint8 rsvd[18];					// should all be zero
Uint32 mapSize;					// total size of map
Uint32 dataSize;				// total size of data area

// resource data
Uint8 resData[dataSize];		// data for each resource

// resource map
Uint32 checkOffset;				// offset from start of file (for corruption detection)
Uint32 typeCount;				// number of types
struct {
	Uint32 type;				// type code
	Uint32 itemCount;			// number of resources for this type
	struct {
		Uint32 id;				// ID of this resource
		Uint32 attrib;			// attributes
		Uint32 dataSize;		// actual size of data
		Uint32 dataOffset;		// offset into resData from start of file
	} itemEntry[itemCount];		// any order
} typeEntry[typeCount];			// any order

-- RESOURCE NAMES FORMAT --

Resource names are stored in '%_nm' resources.  The ID of the '%_nm' resource is
the type for which it stores the names for.

Uint32 count;					// number of items
struct {
	Uint32 id;					// begins on 4-byte boundary
	Uint16 size;
	Uint8 data[size];
} item[count];					// any order

-- ERRORS --

100		An unknown resource error has occured.
101		Specified resource does not exist.
102		Resource file is corrupt.
103		Cannot perform operation on resource because it is in use.
104		A resource with the same type and ID already exists.
*/

#include "URez.h"
#include "UMemory.h"
#include "UFileSys.h"

struct SRez {
	SRez *next;						// search chain
	
	Uint32 typeCount;
	Uint32 lastSearchType;
	Uint32 lastSearchTypeIndex;
	THdl typeList;					// SRezTypeEntry
	Uint32 idCounter;

	TIOReadProc readProc;
	TIOWriteProc writeProc;
	TIOGetSizeProc getSizeProc;
	TIOSetSizeProc setSizeProc;
	TIOControlProc controlProc;
	void *procRef;
	
	bool isChanged;
};

#pragma options align=packed
struct SRezItemEntry {
	Uint32 id;
	Uint32 attrib;
	Uint32 dataSize;				// in file
	Uint32 dataOffset;				// from start of file
	THdl hdl;
	Uint32 useCount;
};

struct SRezTypeEntry 
{
	Uint32 type;
	THdl data;						// SRezItemList
};
#pragma options align=reset

struct SRezItemList 
{
	Uint32 lastSearchID;
	Uint32 lastSearchIndex;
	Uint32 count;
	SRezItemEntry items[];
};

static SRez *_RZSearchChainHead = nil;

//#REF		((SRez *)inRef)

static bool _RZSearchTypeList(TRez inRef, Uint32 inType, Uint32& outIndex);
static bool _RZSearchItemList(THdl inHdl, Uint32 inID, Uint32& outIndex);
static Uint32 _RZDisposeItemList(THdl inHdl);
static Uint32 _RZFileGetSize(void *inRef);
static Uint32 _RZFileControl(void *inRef, Uint32 inCmd, void *inParamA, void *inParamB);
static Int32 _RZAddItem(TRez inRef, Uint32 inType, SRezItemEntry *ioNewItemEntry, bool inChangeIfExists);
static void _RZReadRezFile(TRez inRef);


static THdl _RZGetNthItemListHdl(TRez inRef, UInt32 ind)
{
  // #define _RZGetNthItemListHdl(ref, ind)		
	return (THdl)UMemory::ReadLong( ((SRez *)inRef)->typeList, 
									(ind * sizeof(SRezTypeEntry)) + sizeof(UInt32));
}

/* -------------------------------------------------------------------------- */

TRez URez::New(TIOReadProc inReadProc, TIOWriteProc inWriteProc, TIOGetSizeProc inGetSizeProc, TIOSetSizeProc inSetSizeProc, TIOControlProc inControlProc, void *inRef, bool inCreateNew)
{
	SRez *ref = (SRez *)UMemory::NewClear(sizeof(SRez));
	
	try
	{
		ref->readProc = inReadProc;
		ref->writeProc = inWriteProc;
		ref->getSizeProc = inGetSizeProc;
		ref->setSizeProc = inSetSizeProc;
		ref->controlProc = inControlProc;
		ref->procRef = inRef;
		ref->typeList = UMemory::NewHandle();
	}
	catch(...)
	{
		UMemory::Dispose((TPtr)ref);
		throw;
	}

	if (!inCreateNew)
	{
		try
		{
			_RZReadRezFile((TRez)ref);
		}
		catch(...)
		{
			URez::Dispose((TRez)ref);
			throw;
		}
	}
	
	return (TRez)ref;
}

TRez URez::NewFromFile(TFSRefObj* inFile, bool inCreateNew)
{
	return New((TIOReadProc)UFS::Read, (TIOWriteProc)UFS::Write, _RZFileGetSize, (TIOSetSizeProc)UFS::SetSize, _RZFileControl, inFile, inCreateNew);
}

void URez::Dispose(TRez inRef)
{
	THdl h;
	Uint32 i;
	SRezTypeEntry *te;
	
	if (inRef)
	{
		// remove from search chain
		SRez *tm = _RZSearchChainHead;
		SRez *ptm = nil;
		while (tm)
		{
			if (tm == ((SRez *)inRef))
			{
				if (ptm)
					ptm->next = tm->next;
				else
					_RZSearchChainHead = tm->next;
				break;
			}
			ptm = tm;
			tm = tm->next;
		}

		// kill item list handles
		h = ((SRez *)inRef)->typeList;
		if (h)
		{
			te = (SRezTypeEntry *)UMemory::Lock(h);
			
			i = ((SRez *)inRef)->typeCount;
			
			while (i--)
			{
				_RZDisposeItemList(te->data);
				UMemory::Dispose(te->data);
				te++;
			}
			
			UMemory::Unlock(h);
			UMemory::Dispose(h);
		}
		
		UMemory::Dispose((TPtr)inRef);
	}
}

void URez::SetRef(TRez inRef, void *inRefVal)
{
	Require(inRef);
	((SRez *)inRef)->procRef = inRefVal;
}

bool URez::IsChanged(TRez inRef)
{
	Require(inRef);
	return ((SRez *)inRef)->isChanged;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// takes ownership of inHdl
Int32 URez::AddItem(TRez inRef, Uint32 inType, Int32 inID, THdl inHdl, const Uint8 *inName, Uint32 inAttrib, Uint32 inOptions)
{
	SRezItemEntry info;
	bool changeIfExists = (inOptions & kChangeIfExists) != 0;
	
	info.id = inID;
	info.attrib = inAttrib | resAttr_Changed;
	info.dataSize = info.dataOffset = info.useCount = 0;
	info.hdl = inHdl;
	
	_RZAddItem(inRef, inType, &info, changeIfExists);
	inID = info.id;
	
	try
	{
		if (inName)
			URez::SetItemName(inRef, inType, inID, inName+1, inName[0]);
		else if (changeIfExists)
			URez::SetItemName(inRef, inType, inID, nil, 0);
	}
	catch(...)
	{
		URez::RemoveItem(inRef, inType, inID);
		throw;
	}
	
	return inID;
}

void URez::RemoveItem(TRez inRef, Uint32 inType, Int32 inID)
{
	Uint32 i;

	if (inRef && _RZSearchTypeList(inRef, inType, i))
	{
		THdl itemListHdl = _RZGetNthItemListHdl(inRef, i);
				
		if (_RZSearchItemList(itemListHdl, inID, i))
		{
			SRezItemList *itemList = (SRezItemList *)UMemory::Lock(itemListHdl);
			SRezItemEntry *item = itemList->items + i;
			THdl h = item->hdl;
			Uint32 useCount = item->useCount;
			UMemory::Unlock(itemListHdl);
			
			if (useCount)
			{
				DebugBreak("URez::RemoveItem() - can't remove because resource is loaded");
				Fail(errorType_Rez, rezError_ResourceInUse);
			}
			
			UMemory::Remove(itemListHdl, sizeof(SRezItemList) + (i * sizeof(SRezItemEntry)), sizeof(SRezItemEntry));
			
			itemList = (SRezItemList *)UMemory::Lock(itemListHdl);
			itemList->count--;
			itemList->lastSearchID = itemList->lastSearchIndex = 0;
			UMemory::Unlock(itemListHdl);

			//((SRez *)inRef)->totalItemCount--;
			((SRez *)inRef)->isChanged = true;
			
			UMemory::Dispose(h);
			
			// remove name
			URez::SetItemName(inRef, inType, inID, nil, 0);
		}
	}
}

void URez::RemoveAllItems(TRez inRef, Uint32 inType)
{
	Uint32 i;
	THdl itemListHdl;
	
	if (inRef && _RZSearchTypeList(inRef, inType, i))
	{
		itemListHdl = _RZGetNthItemListHdl(inRef, i);
	
		((SRez *)inRef)->isChanged = true;
		_RZDisposeItemList(itemListHdl);
		UMemory::ReallocateClear(itemListHdl, sizeof(SRezItemList));

		// remove all resource names for this type
		RemoveItem(inRef, '%_nm', inType);
	}
}

bool URez::ItemExists(TRez inRef, Uint32 inType, Int32 inID)
{
	Require(inRef);
	Uint32 i;
	
	if (_RZSearchTypeList(inRef, inType, i))
	{
		return _RZSearchItemList(_RZGetNthItemListHdl(inRef, i), inID, i);
	}
	
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

THdl URez::LoadItem(TRez inRef, Uint32 inType, Int32 inID, Uint32 inNilIfNotFound)
{
	Uint32 i;
	
	if (inRef == nil)
	{
		if (inNilIfNotFound) return nil;
		Fail(errorType_Misc, error_Param);
	}
	
	if (_RZSearchTypeList(inRef, inType, i))
	{
		THdl h = _RZGetNthItemListHdl(inRef, i);
		
		if (_RZSearchItemList(h, inID, i))
		{
			SRezItemList *itemList;
			StHandleLocker lock(h, (void*&)itemList);
			SRezItemEntry *item = itemList->items + i;
			
			// if handle got discarded, kill it
			h = item->hdl;
			if (h && UMemory::ClearDiscardable(h))
			{
				UMemory::Dispose(h);
				item->hdl = h = nil;
			}

			if (h == nil)
			{
				item->useCount = 0;
				
				if (item->dataSize && item->dataOffset)
				{
					// read resource into memory (into a new handle)
					TIOReadProc readProc = ((SRez *)inRef)->readProc;
					if (readProc == nil) Fail(errorType_Rez, rezError_Unknown);
					
					h = UMemory::NewHandle(item->dataSize);
					try
					{
						void *p;
						StHandleLocker lock(h, p);
						readProc(((SRez *)inRef)->procRef, item->dataOffset, p, item->dataSize);
					}
					catch(...)
					{
						UMemory::Dispose(h);
						throw;
					}
					item->hdl = h;
				}
				else
					item->hdl = h = UMemory::NewHandle();
			}
			
			// keep a count of how many loads/releases
			item->useCount++;
			
			return h;
		}
	}
	
	if (!inNilIfNotFound) Fail(errorType_Rez, rezError_NoSuchResource);
	return nil;
}

void URez::ReleaseItem(TRez inRef, Uint32 inType, Int32 inID)
{
	if (inRef)
	{
		Uint32 i;
		
		if (_RZSearchTypeList(inRef, inType, i))
		{
			THdl h = _RZGetNthItemListHdl(inRef, i);
			
			if (_RZSearchItemList(h, inID, i))
			{
				SRezItemList *itemList;
				StHandleLocker lock(h, (void*&)itemList);
				SRezItemEntry *item = itemList->items + i;
				
				if (item->useCount == 0)
				{
					DebugBreak("URez - ReleaseItem() called too many times");
				}
				else
				{
					item->useCount--;
					
					// if not being used and not changed, make handle purgeable
					if ((item->useCount == 0) && ((item->attrib & resAttr_Changed) == 0))
						UMemory::SetDiscardable(item->hdl);
				}
			}
		}
	}
}

// if you change the handle, must call ChangedItem() BEFORE ReleaseItem()
void URez::ChangedItem(TRez inRef, Uint32 inType, Int32 inID)
{
	Require(inRef);
	Uint32 i;
	
	if (_RZSearchTypeList(inRef, inType, i))
	{
		THdl h = _RZGetNthItemListHdl(inRef, i);
		
		if (_RZSearchItemList(h, inID, i))
		{
			SRezItemList *itemList;
			StHandleLocker lock(h, (void*&)itemList);
			SRezItemEntry *item = itemList->items + i;
			
			if (item->useCount == 0)
				DebugBreak("URez::ChangedItem() - resource is not loaded");
			else
			{
				item->attrib |= resAttr_Changed;
				((SRez *)inRef)->isChanged = true;
			}
		}
	}
}

void URez::SetItemHandle(TRez inRef, Uint32 inType, Int32 inID, THdl inHdl)
{
	Require(inRef && inHdl);
	Uint32 i;
	
	if (_RZSearchTypeList(inRef, inType, i))
	{
		THdl h = _RZGetNthItemListHdl(inRef, i);
		
		if (_RZSearchItemList(h, inID, i))
		{
			SRezItemList *itemList;
			StHandleLocker lock(h, (void*&)itemList);
			SRezItemEntry *item = itemList->items + i;
			
			if (item->useCount == 0)
			{
				UMemory::Dispose(item->hdl);
				item->hdl = inHdl;
				item->attrib |= resAttr_Changed;
				((SRez *)inRef)->isChanged = true;
			}
			else
			{
				DebugBreak("URez::SetItemHandle() - resource is loaded");
				Fail(errorType_Rez, rezError_ResourceInUse);
			}
		}
		else
			Fail(errorType_Rez, rezError_NoSuchResource);
	}
	else
		Fail(errorType_Rez, rezError_NoSuchResource);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

Uint32 URez::ReadItem(TRez inRef, Uint32 inType, Int32 inID, Uint32 inOffset, void *outData, Uint32 inMaxSize)
{
	Require(inRef);
	Uint32 i, s;
	
	if (_RZSearchTypeList(inRef, inType, i))
	{
		THdl h = _RZGetNthItemListHdl(inRef, i);
		
		if (_RZSearchItemList(h, inID, i))
		{
			SRezItemList *itemList;
			StHandleLocker lock(h, (void*&)itemList);
			SRezItemEntry *item = itemList->items + i;
			
			// if resource is in memory, just copy from that. Otherwise read directly from file
			h = item->hdl;
			void *p = h ? UMemory::Lock(h) : nil;
			if (p)	// can also be nil if discarded
			{
				s = UMemory::GetSize(h);
				
				if (inOffset >= s) return 0;
				if (inOffset + inMaxSize > s) inMaxSize = s - inOffset;
				
				UMemory::Copy(outData, BPTR(p)+inOffset, inMaxSize);
				UMemory::Unlock(h);
			}
			else
			{
				s = item->dataSize;
				
				if (inOffset >= s) return 0;
				if (inOffset + inMaxSize > s) inMaxSize = s - inOffset;
				
				TIOReadProc readProc = ((SRez *)inRef)->readProc;
				if (readProc == nil) Fail(errorType_Rez, rezError_Unknown);
				
				readProc(((SRez *)inRef)->procRef, item->dataOffset + inOffset, outData, inMaxSize);
			}
			
			return inMaxSize;
		}
	}
	
	Fail(errorType_Rez, rezError_NoSuchResource);
	return 0;
}

Uint32 URez::GetItemSize(TRez inRef, Uint32 inType, Int32 inID)
{
	Require(inRef);
	Uint32 i;
	
	if (_RZSearchTypeList(inRef, inType, i))
	{
		THdl h = _RZGetNthItemListHdl(inRef, i);
		
		if (_RZSearchItemList(h, inID, i))
		{
			SRezItemList *itemList;
			StHandleLocker lock(h, (void*&)itemList);
			SRezItemEntry *item = itemList->items + i;
			
			h = item->hdl;
			if (h)
			{
				i = UMemory::GetSize(h);
				if (i) return i;	// size is 0 if discarded
			}
			
			return item->dataSize;
		}
	}
	
	Fail(errorType_Rez, rezError_NoSuchResource);
	return 0;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

Uint32 URez::GetItemName(TRez inRef, Uint32 inType, Int32 inID, void *outData, Uint32 inMaxSize)
{
	THdl h;
	Uint8 *p, *ep;
	Uint32 s = 0;
	
	h = URez::LoadItem(inRef, '%_nm', inType, true);
	
	if (h)
	{
		StRezReleaser release(inRef, '%_nm', inType);
		
		{
			StHandleLocker lock(h, (void*&)p);
			ep = p + UMemory::GetSize(h);
			
			p += 4;		// skip count (we'll use end-of-data instead)
			
			while (p < ep)
			{
				if (*(Int32 *)p == inID)
				{
					s = *(Uint16 *)(p+4);
					if (p+6+s > ep) Fail(errorType_Rez, rezError_Corrupt);

					if (s > inMaxSize) s = inMaxSize;
					UMemory::Copy(outData, p+6, s);
					break;
				}
				
				// next item (begins on 4-byte boundary)
				p += ((Uint32)*(Uint16 *)(p+4) + 6 + 3) & ~3;
			}
		}
	}
	
	return s;
}

void URez::SetItemName(TRez inRef, Uint32 inType, Int32 inID, const void *inData, Uint32 inDataSize)
{
	THdl h;
	Uint8 *p, *ep, *sp;
	Uint32 offset, s;
	
	Require(inType && inID);
	if (inType == '%_nm') return;
	
#if DEBUG
	if (inDataSize > max_Uint16)
		DebugBreak("URez - maximum name size is 65535 bytes");
#endif
	
	h = URez::LoadItem(inRef, '%_nm', inType, true);
	
	if (h == nil)
	{
		if (inDataSize)
		{
			h = UMemory::NewHandle((inDataSize + 4 + 6 + 3) & ~3);
			
			try
			{
				p = UMemory::Lock(h);
				
				*((Uint32 *)p)++ = 1;
				*((Int32 *)p)++ = inID;
				*((Uint16 *)p)++ = inDataSize;
				UMemory::Copy(p, inData, inDataSize);
				
				UMemory::Unlock(h);
				URez::AddItem(inRef, '%_nm', inType, h);
			}
			catch(...)
			{
				UMemory::Dispose(h);
				throw;
			}
		}
	}
	else
	{
		StRezReleaser release(inRef, '%_nm', inType);
		
		// search for existing name
		{
			StHandleLocker lock(h, (void*&)p);
			sp = p;
			ep = sp + UMemory::GetSize(h);

			offset = 0;
			p += 4;

			while (p < ep)
			{
				if (*(Int32 *)p == inID)
				{
					offset = p - sp;
					s = *(Uint16 *)(p+4);
					if (p+6+s > ep) Fail(errorType_Rez, rezError_Corrupt);
					break;
				}
				
				// next item (begins on 4-byte boundary)
				p += ((Uint32)*(Uint16 *)(p+4) + 6 + 3) & ~3;
			}
		}
		
		// remove, update, or add name
		if (inDataSize == 0)				// if remove
		{
			if (offset)						// if found
			{
				// remove
				UMemory::Remove(h, offset, (s + 6 + 3) & ~3);
				(*(Uint32 *)UMemory::Lock(h))--;
				UMemory::Unlock(h);
				URez::ChangedItem(inRef, '%_nm', inType);
			}
		}
		else								// update or add
		{
			if (offset == 0)				// if not found
			{
				// add
				offset = UMemory::GetSize(h);
				UMemory::SetSize(h, offset + ((inDataSize + 6 + 3) & ~3));
				
				StHandleLocker lock(h, (void*&)p);
				(*(Uint32 *)p)++;
				
				p += offset;
				*((Int32 *)p)++ = inID;
				*((Uint16 *)p)++ = inDataSize;
				UMemory::Copy(p, inData, inDataSize);
			}
			else							// found
			{
				// update
				UMemory::Replace(h, offset, (s + 6 + 3) & ~3, nil, (inDataSize + 6 + 3) & ~3);
				
				StHandleLocker lock(h, (void*&)p);
				p += offset;
				*((Int32 *)p)++ = inID;
				*((Uint16 *)p)++ = inDataSize;
				UMemory::Copy(p, inData, inDataSize);
			}
			
			URez::ChangedItem(inRef, '%_nm', inType);
		}
	}
}

Uint32 URez::GetItemAttributes(TRez inRef, Uint32 inType, Int32 inID)
{
	Require(inRef);
	Uint32 i;
	
	if (_RZSearchTypeList(inRef, inType, i))
	{
		THdl h = _RZGetNthItemListHdl(inRef, i);
		
		if (_RZSearchItemList(h, inID, i))
		{
			SRezItemList *itemList;
			StHandleLocker lock(h, (void*&)itemList);
			SRezItemEntry *item = itemList->items + i;
			
			return item->attrib;
		}
	}
	
	Fail(errorType_Rez, rezError_NoSuchResource);
	return 0;
}

void URez::SetItemAttributes(TRez inRef, Uint32 inType, Int32 inID, Uint32 inAttrib)
{
	Require(inRef);
	Uint32 i;
	
	if (_RZSearchTypeList(inRef, inType, i))
	{
		THdl h = _RZGetNthItemListHdl(inRef, i);
		
		if (_RZSearchItemList(h, inID, i))
		{
			SRezItemList *itemList;
			StHandleLocker lock(h, (void*&)itemList);
			SRezItemEntry *item = itemList->items + i;
			
			// preserve changed flag (that's only if DATA changed)
			inAttrib &= ~resAttr_Changed;
			if (item->attrib & resAttr_Changed)
				inAttrib |= resAttr_Changed;
			
			item->attrib = inAttrib;
			
			((SRez *)inRef)->isChanged = true;
		}
	}
	
	Fail(errorType_Rez, rezError_NoSuchResource);
}

void URez::ChangeItemAttributes(TRez inRef, Uint32 inType, Int32 inID, Uint32 inWhichAttr, Uint32 inNewAttr)
{
	Require(inRef);
	Uint32 i, n;
	
	if (_RZSearchTypeList(inRef, inType, i))
	{
		THdl h = _RZGetNthItemListHdl(inRef, i);
		
		if (_RZSearchItemList(h, inID, i))
		{
			SRezItemList *itemList;
			StHandleLocker lock(h, (void*&)itemList);
			SRezItemEntry *item = itemList->items + i;
			
			// don't allow changing of "changed" flag
			inWhichAttr &= ~resAttr_Changed;
			
			n = item->attrib;
			n &= ~inWhichAttr;				// clear inWhichAttr bits
			n |= inNewAttr & inWhichAttr;	// turn on inNewAttr bits (but only those set in inWhichAttr as well)
			
			item->attrib = n;
			
			((SRez *)inRef)->isChanged = true;
		}
	}
	
	Fail(errorType_Rez, rezError_NoSuchResource);
}

void URez::SetItemID(TRez inRef, Uint32 inType, Int32 inID, Uint32 inNewType, Int32 inNewID)
{
	#pragma unused(inRef, inType, inID, inNewType, inNewID)
	
	Fail(errorType_Misc, error_Unimplemented);
	
	// need to update name too!  remove old and add new?
}

void URez::SetItemInfo(TRez inRef, Uint32 inType, Int32 inID, Uint32 inNewType, Int32 inNewID, Uint32 inAttrib, const Uint8 *inName)
{
	#pragma unused(inRef, inType, inID, inNewType, inNewID, inAttrib, inName)
	
	Fail(errorType_Misc, error_Unimplemented);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
Format of the handle returned by the GetTypeListing() function:

Uint32 count;
struct {
	Uint32 type;
	Uint32 itemCount;
} entry[count];
*/
THdl URez::GetTypeListing(TRez inRef, Uint32 *outCount, Uint32 /* inOptions */)
{
	SRezTypeEntry *typeEntry;
	Uint32 n;
	THdl h;
	Uint32 *lp, *sp;
	
	Require(inRef);
	
	n = ((SRez *)inRef)->typeCount;
	StHandleLocker lockTL(((SRez *)inRef)->typeList, (void*&)typeEntry);
	
	h = UMemory::NewHandle(4 + (n * 8));
	
	sp = lp = (Uint32 *)UMemory::Lock(h);
	lp++;	// skip count
	
	while (n--)
	{
		lp[0] = typeEntry->type;
		lp[1] = UMemory::ReadLong(typeEntry->data, (UInt32)&((SRezItemList*)0)->count);
				
		if (lp[1]) lp += 2;		// don't save this one if no items (empty type)
		
		typeEntry++;
	}
	
	n = ((lp - sp) - 1) / 2;	// calculate number of types stored in handle
	*sp = n;					// store count at start of handle
	if (outCount) *outCount = n;
	
	UMemory::Unlock(h);
	UMemory::SetSize(h, 4 + (n * 8));
	
	return h;
}

/*
 * Returns nil if no items.  Format of handle is:
 *
 *		Uint32 count;
 *		SRezListItem item[count];	// each item begins on four-byte boundary
 */
THdl URez::GetItemListing(TRez inRef, Uint32 inType, Uint32 *outCount, Uint32 /* inOptions */)
{
	Uint32 i, n;
	THdl h, listHdl;
	Uint8 buf[4096];
	SRezListItem& info = *(SRezListItem *)buf;
	
	Require(inRef);

	if (!_RZSearchTypeList(inRef, inType, i))
	{
		if (outCount) *outCount = 0;
		return nil;
	}
	
	THdl itemListHdl = _RZGetNthItemListHdl(inRef, i);
	
	SRezItemList *itemList;
	StHandleLocker lock(itemListHdl, (void*&)itemList);
	SRezItemEntry *item = itemList->items;
	n = itemList->count;
	if (outCount) *outCount = n;
	
	listHdl = UMemory::NewHandle(&n, sizeof(n));
	
	try
	{
		while (n--)
		{
			info.id = item->id;
			info.attrib = item->attrib;

			h = item->hdl;
			if (h)
			{
				info.size = UMemory::GetSize(h);
				if (info.size == 0) info.size = item->dataSize;
			}
			else
				info.size = item->dataSize;
			
			info.nameSize = URez::GetItemName(inRef, inType, info.id, info.nameData, sizeof(buf) - sizeof(SRezListItem) - 4);
			
			i = (sizeof(SRezListItem) + info.nameSize + 3) & ~3;
			UMemory::Append(listHdl, buf, i);
			
			item++;
		}
	}
	catch(...)
	{
		UMemory::Dispose(listHdl);
		throw;
	}
	
	return listHdl;
}

bool URez::GetItemListNext(THdl inItemList, Uint32& ioOffset, Int32 *outID, Uint32 *outSize, Uint32 *outAttrib, Uint8 *outName)
{
	Uint8 *p;
	Uint32 maxSize;
	SRezListItem *item;
	
	if (inItemList == nil) return false;
	StHandleLocker lock(inItemList, (void*&)p);
	maxSize = UMemory::GetSize(inItemList);
	
	if (ioOffset == 0) ioOffset = 4;		// skip count
	if ((ioOffset + sizeof(SRezListItem)) > maxSize) return false;
	
	item = (SRezListItem *)(p + ioOffset);
	
	if (outName)
	{
		maxSize = item->nameSize;
		if (maxSize > 255) maxSize = 255;
		UMemory::Copy(outName+1, item->nameData, maxSize);
		outName[0] = maxSize;
	}
	
	if (outID)				*outID = item->id;
	if (outAttrib)			*outAttrib = item->attrib;
	if (outSize)			*outSize = item->size;
	
	ioOffset += (sizeof(SRezListItem) + item->nameSize + 3) & ~3;
	
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void URez::AddSearchChainStart(TRez inRef, Uint32 /* inOptions */)
{
	Require(inRef);
	
	// remove from search chain
	SRez *curLink = _RZSearchChainHead;
	SRez *prevLink = nil;
	while (curLink)
	{
		if (curLink == ((SRez *)inRef))
		{
			if (prevLink)
				prevLink->next = curLink->next;
			else
				_RZSearchChainHead = curLink->next;
			break;
		}
		prevLink = curLink;
		curLink = curLink->next;
	}
	
	// insert at start of chain
	((SRez *)inRef)->next = _RZSearchChainHead;
	_RZSearchChainHead = ((SRez *)inRef);
}

void URez::AddSearchChainEnd(TRez inRef, Uint32 /* inOptions */)
{
	Require(inRef);
	
	// remove from search chain and find end
	SRez *curLink = _RZSearchChainHead;
	SRez *prevLink = nil;
	while (curLink)
	{
		if (curLink == ((SRez *)inRef))
		{
			curLink = curLink->next;
			
			if (prevLink)
				prevLink->next = curLink;
			else
				_RZSearchChainHead = curLink;
		}
		else
		{
			prevLink = curLink;
			curLink = curLink->next;
		}
	}
	
	// insert at end of chain
	((SRez *)inRef)->next = nil;
	if (prevLink)
		prevLink->next = ((SRez *)inRef);
	else
		_RZSearchChainHead = ((SRez *)inRef);
}

// returns nil if not found
TRez URez::SearchChain(Uint32 inType, Int32 inID, Uint32 /* inOptions */)
{
	SRez *rez = _RZSearchChainHead;
	
	while (rez)
	{
		if (ItemExists((TRez)rez, inType, inID))
			return (TRez)rez;
		
		rez = rez->next;
	}
	
	return nil;
}

void URez::AddProgramFileToSearchChain(const Uint8 *inName)
{
	TFSRefObj* fp = nil;
	TRez rz = nil;
	
	ASSERT(inName);
	
	try
	{
		fp = UFileSys::New(kProgramFolder, nil, inName, fsOption_RequireExistingFile);
		fp->Open(perm_Read);
	
		rz = URez::NewFromFile(fp, false);
		URez::AddSearchChainEnd(rz);
	}
	catch(...)
	{
		delete fp;
		delete rz;
		throw;
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
This version of Save() simply rebuilds the entire file to a new file, and then
deletes the old.  This is easier, completely safe in the event of a crash,
and may actually be faster if there are many changes.

A future version of URez could offer the *option* of updating the existing file,
which might be faster for large files with few changes.  To do this it could
generate a list of offsets and actions such as maintain, remove, resize.
A utility function could then call the I/O procs to change the data according
to the list of offsets and actions.
*/
void URez::Save(TRez inRef, Uint32 /* inOptions */)
{
	enum {
		headerSize			= 32,
		mapHeaderSize		= 8,
		typeHeaderSize		= 8,
		resEntrySize		= 16
	};

	TIOControlProc controlProc;
	TIOSetSizeProc setSizeProc;
	TIOReadProc readProc;
	TIOWriteProc writeProc;
	void *procRef, *tempRef;
	SRezTypeEntry *typeList;
	SRezItemEntry *itemList, *item;
	SRezItemList *itemListInfo;
	Uint32 typeCount, typeIndex, itemCount, itemIndex, dataAreaSize, mapSize, writeOffset, s, resBufSize;
	THdl itemListHdl, h, resBuf;
	TPtr mapBuf;
	Uint32 *lp;
	Uint8 headerBuf[headerSize];
	void *tempp;

	// if not changed, don't do anything
	Require(inRef);
	if (!((SRez *)inRef)->isChanged) return;
	
	// get procs
	controlProc = ((SRez *)inRef)->controlProc;
	if (controlProc == nil)
	{
		DebugBreak("URez::Save() - need control proc");
		Fail(errorType_Misc, error_Protocol);
	}
	setSizeProc = ((SRez *)inRef)->setSizeProc;
	readProc = ((SRez *)inRef)->readProc;
	writeProc = ((SRez *)inRef)->writeProc;
	if (setSizeProc == nil || readProc == nil || writeProc == nil)
	{
		DebugBreak("URez::Save() - need Read, Write, and SetSize procs");
		Fail(errorType_Misc, error_Protocol);
	}
	procRef = ((SRez *)inRef)->procRef;
	
	// calc size of data section and map
	dataAreaSize = 0;
	mapSize = mapHeaderSize;
	typeCount = ((SRez *)inRef)->typeCount;
	StHandleLocker lockTypeList(((SRez *)inRef)->typeList, (void*&)typeList);
	for (typeIndex = 0; typeIndex != typeCount; typeIndex++)			// loop thru types
	{
		itemListHdl = typeList[typeIndex].data;
		StHandleLocker lockItemList(itemListHdl, (void*&)itemListInfo);
		itemList = itemListInfo->items;
		itemCount = itemListInfo->count;
		
		if (itemCount)		// ignore type if no items
		{
			mapSize += typeHeaderSize + (itemCount * resEntrySize);
			
			for (itemIndex = 0; itemIndex != itemCount; itemIndex++)	// loop thru items
			{
				item = itemList + itemIndex;
				
				if ((item->attrib & resAttr_Changed) && item->hdl)
					dataAreaSize += UMemory::GetSize(item->hdl);
				else
					dataAreaSize += item->dataSize;
			}
		}
	}
	dataAreaSize += 3;				// round to 4 byte boundary
	dataAreaSize &= ~3;
	
	// build file header
	lp = (Uint32 *)headerBuf;
	lp[0] = 'AWRZ';
	lp[1] = TB((Uint32)0x00010000);
	lp[2] = 0;
	lp[3] = 0;
	lp[4] = 0;
	lp[5] = 0;
	lp[6] = TB(mapSize);
	lp[7] = TB(dataAreaSize);
	lp += 8;
	
	// write out the new file
	mapBuf = nil;
	tempRef = nil;
	resBuf = nil;
	try
	{
		// start creating map
		mapBuf = UMemory::New(mapSize);
		lp = (Uint32 *)mapBuf;
		lp[0] = headerSize + dataAreaSize;
		lp[1] = typeCount;
		lp += 2;
		
		// allocate resource buffer
		resBufSize = 1024;
		resBuf = UMemory::NewHandle(resBufSize);
		
		// create temp file
		tempRef = (void *)controlProc(procRef, 1, nil, nil);
		
		// expand temp file to the correct size and write header
		setSizeProc(tempRef, headerSize + dataAreaSize + mapSize);
		writeProc(tempRef, 0, headerBuf, headerSize);
		writeOffset = headerSize;
		
		// write resource data
		for (typeIndex = 0; typeIndex != typeCount; typeIndex++)
		{
			itemListHdl = typeList[typeIndex].data;
			StHandleLocker lockItemList(itemListHdl, (void*&)itemListInfo);
			itemList = itemListInfo->items;
			itemCount = itemListInfo->count;
			
			if (itemCount)		// ignore type if no items
			{
				// add to map
				lp[0] = TB(typeList[typeIndex].type);
				lp[1] = TB(itemCount);
				lp += 2;
				
				// write data for each item of this type
				for (itemIndex = 0; itemIndex != itemCount; itemIndex++)
				{
					item = itemList + itemIndex;
					
					// write data to file
					if ((item->attrib & resAttr_Changed) && item->hdl)
					{
						h = item->hdl;
						s = UMemory::GetSize(h);
						
						// write handle out to temp file
						StHandleLocker lockHdl(h, tempp);
						writeProc(tempRef, writeOffset, tempp, s);
					}
					else
					{
						// expand buffer if necessary
						s = item->dataSize;
						if (s > resBufSize)
						{
							resBufSize = s + 1024;
							UMemory::Reallocate(resBuf, resBufSize);
						}
						
						// read resource into memory, write out to temp file
						StHandleLocker lockResBuf(resBuf, tempp);
						readProc(procRef, item->dataOffset, tempp, s);
						writeProc(tempRef, writeOffset, tempp, s);
					}
					
					// add to map
					lp[0] = TB(item->id);
					lp[1] = TB(item->attrib & ~resAttr_Changed);
					lp[2] = TB(s);
					lp[3] = TB(writeOffset);
					lp += 4;
					
					// advance write offset past data for this resource
					writeOffset += s;
				}
			}
		}
		
		// don't need resource buffer anymore
		UMemory::Dispose(resBuf);
		resBuf = nil;
		
		// write map
		writeProc(tempRef, headerSize + dataAreaSize, mapBuf, mapSize);
		UMemory::Dispose(mapBuf);
		mapBuf = nil;
		
		// switch to temp file
		controlProc(procRef, 3, tempRef, nil);
	}
	catch(...)
	{
		// delete temp file and release memory
		if (tempRef)
		{
			try { controlProc(procRef, 2, tempRef, nil); } catch(...) {}
		}
		UMemory::Dispose(mapBuf);
		UMemory::Dispose(resBuf);
		throw;
	}
	
	// update offsets to match the new file
	writeOffset = headerSize;
	for (typeIndex = 0; typeIndex != typeCount; typeIndex++)
	{
		itemListHdl = typeList[typeIndex].data;
		StHandleLocker lockItemList(itemListHdl, (void*&)itemListInfo);
		itemList = itemListInfo->items;
		itemCount = itemListInfo->count;
		
		for (itemIndex = 0; itemIndex != itemCount; itemIndex++)	// loop thru items
		{
			item = itemList + itemIndex;
			
			// update offset
			item->dataOffset = writeOffset;
			
			if ((item->attrib & resAttr_Changed) && item->hdl)
			{
				s = UMemory::GetSize(item->hdl);
				
				item->dataSize = s;
				writeOffset += s;
				
				// clear changed bit
				item->attrib &= ~resAttr_Changed;
				if (item->useCount == 0) UMemory::SetDiscardable(item->hdl);
			}
			else
				writeOffset += item->dataSize;
		}
	}
	
	// file is no longer changed
	((SRez *)inRef)->isChanged = false;
}

Int32 URez::GetLowestUnusedID(TRez inRef, Uint32 inType, Int32 inStartFrom)
{
	Require(inRef);
	Uint32 i;
	
	if (!_RZSearchTypeList(inRef, inType, i))
		return (inStartFrom == 0) ? 1 : inStartFrom;
		
	THdl h = _RZGetNthItemListHdl(inRef, i);
	
	for(;;)
	{
		if (inStartFrom == 0) inStartFrom = 1;
		
		if (!_RZSearchItemList(h, inStartFrom, i))
			break; 
		
		inStartFrom++;
	}
	
	return inStartFrom;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

THdl URez::Reload(SRezStub& ioStub, TRez inRef, Uint32 inType, Int32 inID, Uint32 inNilIfNotFound)
{
	THdl h = URez::LoadItem(inRef, inType, inID, inNilIfNotFound);
	
	URez::ReleaseItem(ioStub.rez, ioStub.type, ioStub.id);
	
	if (h)
	{
		ioStub.rez = inRef;
		ioStub.hdl = h;
		ioStub.type = inType;
		ioStub.id = inID;
	}
	else
	{
		ioStub.rez = nil;
		ioStub.hdl = nil;
		ioStub.type = 0;
		ioStub.id = 0;
	}
	
	return h;
}

THdl URez::Reload(SRezStub& ioStub, Uint32 inType, Int32 inID)
{
	TRez r = URez::SearchChain(inType, inID);
	
	THdl h = URez::LoadItem(r, inType, inID, true);
	
	URez::ReleaseItem(ioStub.rez, ioStub.type, ioStub.id);
	
	if (h)
	{
		ioStub.rez = r;
		ioStub.hdl = h;
		ioStub.type = inType;
		ioStub.id = inID;
	}
	else
	{
		ioStub.rez = nil;
		ioStub.hdl = nil;
		ioStub.type = 0;
		ioStub.id = 0;
	}
	
	return h;
}

void URez::SetStubHdl(SRezStub& ioStub, THdl inHdl)
{
	URez::ReleaseItem(ioStub.rez, ioStub.type, ioStub.id);

	ioStub.rez = nil;
	ioStub.hdl = inHdl;
	ioStub.type = 0;
	ioStub.id = 0;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// outIndex is 0-based
static bool _RZSearchTypeList(TRez inRef, Uint32 inType, Uint32& outIndex)
{
	if (inType == 0)
	{
		outIndex = 0;
		return false;
	}

	if (((SRez *)inRef)->lastSearchType == inType)
	{
		outIndex = ((SRez *)inRef)->lastSearchTypeIndex;
		return true;
	}
	
	if (((SRez *)inRef)->typeCount == 0)
	{
		outIndex = 0;
		return false;
	}

	SRezTypeEntry *lookupTab;
	StHandleLocker lock(((SRez *)inRef)->typeList, (void*&)lookupTab);
	
	Uint32 type;
	Uint32 l, r, i;
	
	type = lookupTab[0].type;
	
	if (inType == type)
	{
		outIndex = 0;
		goto found;
	}
	else if (inType < type)
	{
		outIndex = 0;
		return false;
	}
	
	l = 1;
	r = ((SRez *)inRef)->typeCount - 1;
	
	if (l > r)
	{
		outIndex = 1;
		return false;
	}
	
	while (l <= r)
	{
		i = (l + r) >> 1;
		
		type = lookupTab[i].type;

		if (inType == type)
		{
			outIndex = i;
			goto found;
		}
		else if (inType < type)
			r = i - 1;
		else
			l = i + 1;
	}
	
	if (inType > type)
		i++;
	
	outIndex = i;
	return false;
	
found:
	((SRez *)inRef)->lastSearchType = inType;
	((SRez *)inRef)->lastSearchTypeIndex = outIndex;
	return true;
}

// outIndex is 0-based
static bool _RZSearchItemList(THdl inHdl, Uint32 inID, Uint32& outIndex)
{
	SRezItemList *info;
	StHandleLocker lock(inHdl, (void*&)info);
	
	if (inID == 0)
	{
		outIndex = 0;
		return false;
	}

	if (info->lastSearchID == inID)
	{
		outIndex = info->lastSearchIndex;
		return true;
	}
	
	if (info->count == 0)
	{
		outIndex = 0;
		return false;
	}

	SRezItemEntry *lookupTab = info->items;
	Uint32 id;
	Uint32 l, r, i;
	
	id = lookupTab[0].id;
	
	if (inID == id)
	{
		outIndex = 0;
		goto found;
	}
	else if (inID < id)
	{
		outIndex = 0;
		return false;
	}
	
	l = 1;
	r = info->count - 1;
	
	if (l > r)
	{
		outIndex = 1;
		return false;
	}
	
	while (l <= r)
	{
		i = (l + r) >> 1;
		
		id = lookupTab[i].id;

		if (inID == id)
		{
			outIndex = i;
			goto found;
		}
		else if (inID < id)
			r = i - 1;
		else
			l = i + 1;
	}
	
	if (inID > id)
		i++;
	
	outIndex = i;
	return false;
	
found:
	info->lastSearchID = inID;
	info->lastSearchIndex = outIndex;
	return true;
}

// inHdl should be a SRezItemList, does NOT dispose inHdl
static Uint32 _RZDisposeItemList(THdl inHdl)
{
	SRezItemList *itemList;
	SRezItemEntry *item;
	Uint32 i, c;
	
	if (inHdl == nil) return 0;
	
	StHandleLocker lock(inHdl, (void*&)itemList);
	item = itemList->items;
	i = c = itemList->count;
	
	itemList->lastSearchID = itemList->lastSearchIndex = itemList->count = 0;

	while (i--)
	{
		UMemory::Dispose(item->hdl);
		item++;
	}
	
	return c;
}

static Uint32 _RZFileGetSize(void *inRef)
{
	return inRef ? UFS::GetSize((TFSRefObj*)inRef) : 0;
}

static Uint32 _RZFileControl(void *inRef, Uint32 inCmd, void *inParamA, void */* inParamB */)
{
	TFSRefObj* fp;
	
	if (inCmd == 1)			// create temp file
	{
		fp = UFS::NewTemp();
		try
		{
			fp->CreateFile('BINA', 'hDmp');
			fp->Open(perm_ReadWrite);
		}
		catch(...)
		{
			delete fp;
			throw;
		}
		return (Uint32)fp;
	}
	else if (inCmd == 2)	// delete temp file
	{
		fp = (TFSRefObj*)inParamA;
		try {
			fp->DeleteFile();
		} catch(...) {}
		delete fp;
	}
	else if (inCmd == 3)	// switch to temp file
	{
		TFSRefObj* origFile = (TFSRefObj*)inRef;
		fp = (TFSRefObj*)inParamA;
		
		if (origFile->Exists())
		{
			UFS::SwapData(fp, origFile);
			UFS::Close(fp);
			UFS::DeleteFile(fp);
		}
		else
		{
			fp->MoveAndRename(origFile, nil);
		}
		
		delete fp;
	}
	
	return 0;
}

static Int32 _RZAddItem(TRez inRef, Uint32 inType, SRezItemEntry *ioNewItemEntry, bool inChangeIfExists)
{
	SRezItemList *itemList;
	SRezItemEntry *item;
	Uint32 i, id;
	THdl itemListHdl;
	SRezTypeEntry newTypeEntry;
		
	// get item list handle
	if (_RZSearchTypeList(inRef, inType, i))
	{
		// type exists, grab handle
		itemListHdl = _RZGetNthItemListHdl(inRef, i);
	}
	else
	{
		// type doesn't exist, need to add it
		itemListHdl = UMemory::NewHandleClear(sizeof(SRezItemList));
		try
		{
			newTypeEntry.type = inType;
			newTypeEntry.data = itemListHdl;
						
			UMemory::Insert(((SRez *)inRef)->typeList, i * sizeof(SRezTypeEntry), &newTypeEntry, sizeof(newTypeEntry));
			
			((SRez *)inRef)->typeCount++;
			((SRez *)inRef)->lastSearchType = inType;
			((SRez *)inRef)->lastSearchTypeIndex = i;
		}
		catch(...)
		{
			UMemory::Dispose(itemListHdl);
			throw;
		}
	}
	
	// determine index at which to insert new item
	id = ioNewItemEntry->id;
	if (id == 0)
	{
		// generate unique ID
		id = ((SRez *)inRef)->idCounter;
		for(;;)
		{
			id++;
			if (id == 0) id = 1;
			
			if (!_RZSearchItemList(itemListHdl, id, i)) break;
		}
		((SRez *)inRef)->idCounter = ioNewItemEntry->id = id;
	}
	else
	{
		// check if existing item
		if (_RZSearchItemList(itemListHdl, id, i))
		{
			if (inChangeIfExists)
			{
				StHandleLocker lock(itemListHdl, (void*&)itemList);
				item = itemList->items + i;
				
				if (item->useCount)
				{
					DebugBreak("URez - cannot change because resource is loaded");
					Fail(errorType_Rez, rezError_ResourceInUse);
				}
				UMemory::Dispose(item->hdl);
				
				*item = *ioNewItemEntry;
				
				((SRez *)inRef)->isChanged = true;
								
				return id;
			}
			Fail(errorType_Rez, rezError_ResourceAlreadyExists);
		}
	}
	
	// insert the new item entry
	UMemory::Insert(itemListHdl, sizeof(SRezItemList) + (i * sizeof(SRezItemEntry)), ioNewItemEntry, sizeof(SRezItemEntry));

	itemList = (SRezItemList *)UMemory::Lock(itemListHdl);
	itemList->lastSearchID = id;
	itemList->lastSearchIndex = i;
	itemList->count++;
	//((SRez *)inRef)->totalItemCount++;
	((SRez *)inRef)->isChanged = true;
	UMemory::Unlock(itemListHdl);
	
	// return ID of newly created item
	return id;
}

static void _RZReadRezFile(TRez inRef)
{
	/*****************************************************************
	
	A better way to do this might be to first dump all existing lists,
	then build from scratch based on info in file.  Allocate item
	list handle to the correct size, fill it in based on map, then
	sort it.  This is likely to be much faster.
	
	******************************************************************/
	
	enum {
		headerSize			= 32,
		mapHeaderSize		= 8,
		typeHeaderSize		= 8,
		resEntrySize		= 16,
		smallestFileSize	= headerSize + mapHeaderSize
	};
	
	TIOReadProc readProc;
	TIOGetSizeProc getSizeProc;
	void *procRef;
	Uint32 fileSize, mapSize, resDataSize, resDataEndOffset, typeCount, itemCount, type;
	Uint8 *p, *ep;
	TPtr heapBuf = nil;
	Uint8 buf[256];
	SRezItemEntry itemEntry;
	
	try
	{
		// get procs
		readProc = ((SRez *)inRef)->readProc;
		getSizeProc = ((SRez *)inRef)->getSizeProc;
		procRef = ((SRez *)inRef)->procRef;
		
		// check if new file
		if (getSizeProc == nil || readProc == nil)
		{
			newFile:
			((SRez *)inRef)->isChanged = true;
			return;
		}
		
		// determine total size of file
		fileSize = getSizeProc(procRef);
		if (fileSize == 0) goto newFile;
		
		// read header
		if (fileSize < smallestFileSize) goto corrupt;
		if (readProc(procRef, 0, buf, headerSize) < headerSize) goto corrupt;

		// check format and version
		p = buf;
		if (*((Uint32 *)p)++ != TB((Uint32)'AWRZ')) Fail(errorType_Misc, error_FormatUnknown);
		if (*((Uint16 *)p)++ != TB((Uint16)1)) Fail(errorType_Misc, error_VersionUnknown);
		
		// extract info from header
		p += 18;
		mapSize = FB(*((Uint32 *)p)++);
		resDataSize = FB(*((Uint32 *)p)++);
		if (fileSize < (headerSize + resDataSize + mapSize)) goto corrupt;
		if (mapSize < mapHeaderSize) goto corrupt;
		resDataEndOffset = headerSize + resDataSize;
		
		// read entire resource map into memory
		heapBuf = UMemory::New(mapSize);
		if (readProc(procRef, resDataEndOffset, heapBuf, mapSize) < mapSize) goto corrupt;
		
		// extract and validate map header
		p = (Uint8 *)heapBuf;
		ep = p + mapSize;
		if (FB(*((Uint32 *)p)++) != resDataEndOffset) goto corrupt;
		typeCount = FB(*((Uint32 *)p)++);
		if ((mapSize - mapHeaderSize) < (typeCount * typeHeaderSize)) goto corrupt;
		
		// add entry to lists for each resource
		itemEntry.hdl = nil;
		itemEntry.useCount = 0;
		while (typeCount--)
		{
			// extract info from type header
			if (ep - p < typeHeaderSize) goto corrupt;
			type = FB(*((Uint32 *)p)++);
			itemCount = FB(*((Uint32 *)p)++);
			if ((ep - p) < (itemCount * resEntrySize)) goto corrupt;
			if (type == 0) goto corrupt;
			
			// add entry for each resource of this type
			while (itemCount--)
			{
				// extract resource info
				itemEntry.id = FB(((Uint32 *)p)[0]);
				itemEntry.attrib = FB(((Uint32 *)p)[1]);
				itemEntry.dataSize = FB(((Uint32 *)p)[2]);
				itemEntry.dataOffset = FB(((Uint32 *)p)[3]);
				p += 16;
				
				// validate resource info
				if (itemEntry.id == 0) goto corrupt;
				itemEntry.attrib &= ~(resAttr_Changed|resAttr_Validated);
				if (itemEntry.dataOffset >= resDataEndOffset) goto corrupt;
				if (itemEntry.dataOffset + itemEntry.dataSize > resDataEndOffset) goto corrupt;

				// add the resource
				_RZAddItem(inRef, type, &itemEntry, true);
			}
		}

		// all done
		UMemory::Dispose(heapBuf);
		heapBuf = nil;
		((SRez *)inRef)->isChanged = false;
		return;
		
		// file is corrupt
		corrupt:
		Fail(errorType_Rez, rezError_Corrupt);
	}
	catch(...)
	{
		// clean up
		UMemory::Dispose(heapBuf);
		throw;
	}
}




