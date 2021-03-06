/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "Hotline.h"

// TO DO!!
// now that this file isn't kept open all the time, with multi-clients
// it's possible for one to update the file and a second to overwrite those updates (or perhaps even corrupt it!)
// the fix?  keep a sn at the top of the file.  each time I want to look up something, read in that sn and see if it's changed
// if it's different, reload the file's index..."the problem is" that if modified, I must merge my updates to the old
// essentially all structures in ram must be updated from disk and then disk rewritten
// each time I write, increment that sn by one.

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */

#if USE_NEWS_HISTORY

struct SMyNzAllocTableInfo
{
//	Uint32 allocTableChecksum;		// ensure it's not corrupt
	Uint32 offset;
	Uint32 size;		// in blocks
	Uint32 firstFree;	// offset of first byte with free bit - 1 is the first block, 0 indicates none free
	
};

struct SMyNzHistFileHead
{
	Uint32 sig;
	Uint32 vers;
	
	Uint32 blockSize;
	SMyNzAllocTableInfo allocTab;
	
	Uint32 catOffset;
	Uint32 catMaxCount;
	Uint32 catCount;
	
	Uint32 nextSN;		// next access SN
};

struct SMyNzHistFileCat
{
	SGUID guid;
	Uint32 offset;	// block of the list of sorted ids of read articles
	Uint32 accessSN;	// each item has an incrementing sn by date so we can purge the oldest if needbe
#if USE_ADD_SN
	Uint32 addSN;
#endif
};

#if 0
// each offset above points to a:
struct
{
	Uint32 count;
	Uint32 id[count];
};
#endif

CMyNewsReadHistory::CMyNewsReadHistory(TFSRefObj* inRef)
{
	Uint32 s;
	Require(inRef);
	mFile.ref = inRef;
	
	union
	{
		Uint8 data[512];
		SMyNzHistFileHead head;
	};
	
	ClearStruct(data);
	
	bool existing = false;
	
	try
	{
		if (mFile.ref->Exists())
		{
			mFile.ref->Open();
			existing = true;
		}
		else
		{
createNew:
			ClearStruct(data);
			const Uint32 maxCats = 500;
			const Uint32 blockSize = 32;
			const Uint32 catBlocks = (maxCats * sizeof(SMyNzHistFileCat) + blockSize - 1) / blockSize;
			const Uint32 allocTabBlocks = 2;
			
			// create the file
			head.sig = TB((Uint32)'nzht');
			head.vers = TB((Uint32)2);
			head.blockSize = TB((Uint32)blockSize);
			
			head.catOffset = TB((Uint32)1);
			head.catMaxCount = TB((Uint32)maxCats);
			head.catCount = 0;
			head.nextSN = TB((Uint32)1);
			
			// this is the block right after the catlist
			head.allocTab.offset = TB((Uint32)(catBlocks + 1));
			head.allocTab.size = TB((Uint32)allocTabBlocks);
			
			mFile.ref->CreateFileAndOpen('nzht', 'HTLC', nil);
			
			// now write the allocTable
			// must allocat catBlocks + the actual alloctable
			// we won't require more than 1 block in the alloctable to start
			Uint8 allocTab[blockSize * allocTabBlocks];
			
			Uint32 bits = (catBlocks + allocTabBlocks) & 7;
			Uint32 bytes = (catBlocks + allocTabBlocks) / 8;
			UMemory::Fill(allocTab, bytes, 0xFF);
			UMemory::Clear(allocTab + bytes, sizeof(allocTab) - bytes);
			
			head.allocTab.firstFree = TB(bytes + 1);

			bytes *= 8;
			while (bits--)
				SetBit(allocTab, bytes++);
			
			mFile.ref->Write(0, &data, sizeof(data));
			mFile.ref->Write(GetByteOffset(catBlocks + 1, blockSize), allocTab, sizeof(allocTab));
		}

		// load all the data in the file
		// read in the header		

		if (mFile.ref->Read(0, &head, sizeof(SMyNzHistFileHead)) != sizeof(SMyNzHistFileHead))
			Fail(errorType_Misc, error_Corrupt);
				
		if (head.sig != TB((Uint32)'nzht'))
			Fail(errorType_Misc, error_FormatUnknown);
				
		if (head.vers != TB((Uint32)2))
		{
			// to do - if it's 1, should we convert it?
			Fail(errorType_Misc, error_VersionUnknown);
		}	
		
		mFile.blockSize = FB(head.blockSize);
		mFile.allocTabOffset = FB(head.allocTab.offset);
		mFile.allocTableSize = FB(head.allocTab.size);
		mFile.allocTabFirstFree = FB(head.allocTab.firstFree);
		mFile.catOffset = FB(head.catOffset);
			
		mNextSN = FB(head.nextSN);
		mCatMaxCount = FB(head.catMaxCount);
		
		// load in all the categories
		{
			Uint32 catCount = FB(head.catCount);
			if (catCount)
			{
				mList.Preallocate(catCount);
				
				s = catCount * sizeof(SMyNzHistFileCat);
				
				StPtr data(s);
				if(ReadFile(FB(head.catOffset), 0, BPTR(data), s) != s)
					Fail(errorType_Misc, error_Corrupt);
				
				SMyNzHistFileCat *p = (SMyNzHistFileCat *)BPTR(data);
				
				while (catCount--)
				{
					SMyNzHistCatGUIDItm *itm = new SMyNzHistCatGUIDItm;
					
					itm->guid = p->guid;	// don't unflatten
					itm->fileOffset = FB(p->offset);
					itm->accessSN = FB(p->accessSN);
				#if USE_ADD_SN
					itm->addSN = FB(p->addSN);
				#endif
					itm->lst = nil;
					
					mList.AddItem(itm);
					p++;
				}
			}
		}		
	}
	catch(...)
	{
		mFile.blockSize = 0;
		mFile.allocTabOffset = 0;
		mFile.catOffset = 0;
		mNextSN = 0;
		mCatMaxCount = 0;
		mList.Clear();
		mFile.ref->Close();
		
		if (existing)
		{
			mFile.ref->MoveToTrash();
			existing = false;
			goto createNew;
		}
		
		throw;	
	}
	
	mFile.ref->Close();
}

CMyNewsReadHistory::~CMyNewsReadHistory()
{
	PurgeToDisk();
	
	delete mFile.ref;
}

CNZReadList *CMyNewsReadHistory::GetReadList(const SGUID& inGUID)
{
	StFileOpener open(mFile.ref, perm_Rd);
	if (inGUID == (SGUID){0x00000000, 0x00000000, 0x00000000, 0x00000000})
		return nil;
	
	SMyNzHistCatGUIDItm **ptr = mList.GetArrayPtr();
	
searchAgain:
	Uint32 first = 0;
	Uint32 last = mList.GetItemCount();
	Uint32 middle;
	Uint32 count;

	if (!last--)	// if the list is empty, obviously there aren't any items
		goto createItem;
	
	while (first != last)
	{
		middle = (first + last)/2;
		
		// don't use operator< because this guid is in network byte order
		if (UMemory::Compare(&ptr[middle]->guid, &inGUID, sizeof(SGUID)) == -1)	// if ptr[middle]->guid < inGUID
		 	first = middle + 1;
		else
		 	last = middle;
	}
	
	if (ptr[first]->guid == inGUID)
	{
		if (ptr[first]->lst)
			return ptr[first]->lst;
		
		// we haven't loaded this item in yet.  read in the file, create a CNZReadList and return it
		if (ReadFile(ptr[first]->fileOffset, 0, &count, sizeof(Uint32)) != sizeof(Uint32))
			Fail(errorType_Misc, error_Corrupt);
		
		Uint32 s = FB(count) * sizeof(Uint32);
		TPtr data = nil;
		
		if (s)
		{
			try
			{
				if (s > 1048576)	// 1Mb
					Fail(errorType_Misc, error_Corrupt);
				
				data = UMemory::New(s);
				if (!data)
					Fail(errorType_Memory, memError_NotEnough);
			
				if (ReadFile(ptr[first]->fileOffset, sizeof(Uint32), BPTR(data), s) != s)
					Fail(errorType_Misc, error_Corrupt);
			}
			catch(...)
			{
				s = 0;
				if (data)
				{
					UMemory::Dispose(data);
					data = nil;
				}				
				
				throw;
			}
		}

		ptr[first]->lst = new CNZReadList(data, s);
		ptr[first]->accessSN = mNextSN++;
		return ptr[first]->lst;
	}
	else
	{
		// don't use operator< because this guid is in network byte order
		if (UMemory::Compare(&ptr[first]->guid, &inGUID, sizeof(SGUID)) == -1)	// if ptr[middle]->guid < inGUID
			first++;
	
createItem:
		// check if we've exceeded maxCount for cache and if so, purge old ones (15?)
		count = mList.GetItemCount();
		
		if (count >= mCatMaxCount)
		{
			// need to purge old articles
			// purge 15 or so
			PurgeOldest(15);
			
			// now we must find the item in the list again
			goto searchAgain;		
		}
		
		// this GUID does not yet exist.  insert it at middle.  
		SMyNzHistCatGUIDItm *list = new SMyNzHistCatGUIDItm;
		
		list->guid = inGUID;
		list->fileOffset = 0;
		list->accessSN = mNextSN++;
	#if USE_ADD_SN
		list->addSN = 0;
	#endif
		list->lst = new CNZReadList(nil, 0);
		
		mList.InsertItem(first + 1, list);
		
		return list->lst;
	}
}

bool CMyNewsReadHistory::IsNewAddSN(const SGUID& inGUID, Uint32 inSN)
{
#if USE_ADD_SN

	if (inGUID == (SGUID){0x00000000, 0x00000000, 0x00000000, 0x00000000})
		return false;
	
	SMyNzHistCatGUIDItm **ptr = mList.GetArrayPtr();
	
	Uint32 first = 0;
	Uint32 last = mList.GetItemCount();
	Uint32 middle;

	if (!last--)	// if the list is empty, obviously there aren't any items
		return true;
	
	while (first != last)
	{
		middle = (first + last)/2;
		
		// don't use operator< because this guid is in network byte order
		if (UMemory::Compare(&ptr[middle]->guid, &inGUID, sizeof(SGUID)) == -1)	// if ptr[middle]->guid < inGUID
		 	first = middle + 1;
		 else
		 	last = middle;
	}
	
	if (ptr[first]->guid == inGUID)
	{
		ptr[first]->servAddSN = inSN; 	// store this for later
		if (ptr[first]->lst)
			return !(ptr[first]->lst->AllItemsRead());
		else
			return ptr[first]->addSN < inSN;
	}
	else
		return true;

#else

	return true;

#endif
}

// purges cats, and readIDs to disk
void CMyNewsReadHistory::PurgeToDisk()
{
	// go thru each cat.
	StFileOpener open(mFile.ref);
	
	Uint32 count = mList.GetItemCount();
	if (!count)
		return;
	
	SMyNzHistCatGUIDItm **ptr = mList.GetArrayPtr();
	Uint32 *ids;
	Uint32 idCount;
	Uint32 s;
	Uint32 diskCount, diskBlocks, newBlocks;
	
	StPtr catOut(count * sizeof(SMyNzHistFileCat));
	SMyNzHistFileCat *catOutPtr = (SMyNzHistFileCat *)BPTR(catOut);
	
	for (Uint32 i=0; i!=count; i++)
	{
		if (ptr[i]->lst)
		{
			ids = ptr[i]->lst->GetDataPtr(s);
			idCount = s / sizeof(Uint32);

			// determine if there are any items left unread
			if (idCount < ptr[i]->lst->GetTotalCount())
				ptr[i]->addSN = 0;
			else
				ptr[i]->addSN = ptr[i]->servAddSN;

			// check if was modified
			if (ptr[i]->lst->HasChanged())
			{				
				// + 1 for the count
				newBlocks = ((idCount + 1) * sizeof(Uint32) + mFile.blockSize - 1) / mFile.blockSize;
				
				if (!ptr[i]->fileOffset)
				{
					// if there are no blocks yet allocated for this itm
					goto allocNewBlks;
				}
				
				// old blocks
				if (ReadFile(ptr[i]->fileOffset, 0, &diskCount, sizeof(Uint32)) != sizeof(Uint32))
				{
					// corrupt file.  do we complain?
					Fail(errorType_Misc, error_Corrupt);
				}
				
				diskBlocks = ((FB(diskCount) + 1) * sizeof(Uint32) + mFile.blockSize - 1) / mFile.blockSize;
				
				if (newBlocks == diskBlocks)	// just write since the same number of blocks
				{
					mFile.ref->Write(GetByteOffset(ptr[i]->fileOffset, mFile.blockSize) + sizeof(Uint32), ids, idCount * sizeof(Uint32));
				}
				else if (newBlocks < diskBlocks)	// if fewer blocks - just write and deallocate xtras
				{
					Deallocate(ptr[i]->fileOffset + newBlocks, diskBlocks - newBlocks);

					mFile.ref->Write(GetByteOffset(ptr[i]->fileOffset, mFile.blockSize) + sizeof(Uint32), ids, idCount * sizeof(Uint32));
				}
				else	// if more - deallocate old, allocate new, write, and change offset in this cat
				{
					Deallocate(ptr[i]->fileOffset, diskBlocks);
				
allocNewBlks:
					ptr[i]->fileOffset = Allocate(newBlocks);
					mFile.ref->Write(GetByteOffset(ptr[i]->fileOffset, mFile.blockSize) + sizeof(Uint32), ids, idCount * sizeof(Uint32));
				}
								
				// write the new count to this block
			#if CONVERT_INTS
				idCount = TB(idCount);
			#endif					
				mFile.ref->Write(GetByteOffset(ptr[i]->fileOffset, mFile.blockSize), &idCount, sizeof(Uint32));
			}
			
			// purge from memory
			delete ptr[i]->lst;
			ptr[i]->lst = nil;
			
			// to do - need to check if still in use
			// but a files list might still hold one of them...
			// could have an "InUse" flag meaning that another object is using it
			// and we are the ones to trash.
			// Release the obj.
			// note that this will never be a problem in Hotline since it deletes all windows prior to calling this function
			// but, to do it properly, it should check
		}
	
		catOutPtr->guid = ptr[i]->guid;
		catOutPtr->offset = TB(ptr[i]->fileOffset);
		catOutPtr->accessSN = TB(ptr[i]->accessSN);
	#if USE_ADD_SN
		catOutPtr->addSN = TB(ptr[i]->addSN);
	#endif
		catOutPtr++;
	}

	// write full cat list
	mFile.ref->Write(GetByteOffset(mFile.catOffset, mFile.blockSize), BPTR(catOut), count * sizeof(SMyNzHistFileCat));
	
	// update the header file's cat count and nextSN
	Uint32 data[2];
	data[0] = TB(count);
	data[1] = TB(mNextSN);
	
	mFile.ref->Write(OFFSET_OF(SMyNzHistFileHead, catCount), data, 2 * sizeof(Uint32));
}

void CMyNewsReadHistory::PurgeOldest(Uint32 inCount)
{
	if (inCount == 0)
		inCount = 1;
	else if(inCount > 32)
		inCount = 32;
		
	struct
	{
		Uint32 off;
		Uint32 sn;
	
	} itm[32];
	
	Uint32 i;
	Uint32 s;
	
	// set up the ids
	for (i = 0; i != inCount; i++)
		itm[i].sn = max_Uint32;
	
	SMyNzHistCatGUIDItm **ptr = mList.GetArrayPtr();
	
	Uint32 count = mList.GetItemCount();
	if (inCount > count)
		inCount = count;
	
	// find the oldest inCount records
	while (count--)
	{
		if (ptr[count]->accessSN < itm[0].sn)
		{
			itm[0].sn = ptr[count]->accessSN;
			itm[0].off = count;
			
			for (i = 1; i != inCount; i++)
			{
				if (itm[i-1].sn < itm[i].sn)
					swap(itm[i-1], itm[i]);
				else
					break;
			}
		}
	}
	
	// now we must remove itm[0..inCount-1] from the list
	// and, if they have file offsets, deallocate them.
	// this is a good time to flush everything to disk as well

	// sort these in off order
	for (Uint32 j = 0; j != inCount - 1; j++)
	{
		Uint32 smallest = j;
		for (i = j + 1; i != inCount; i++)
		{
			if (itm[i].off < itm[smallest].off)
				smallest = i;
		}
		
		swap(itm[j], itm[smallest]);
	}
	
	// then loop through them and copy the block of data
	// from	(itm[i].off + 1)..(itm[i+1].off - 1)
	// to	(itm[i].off - i)..(itm[i+1].off - i)
	
	try
	{
		// I need to open the file with Write permission, but I only have read (when this func is called)
		mFile.ref->Close();
		StFileOpener open(mFile.ref);	

		inCount--;
		for (i = 0; i != inCount; i++)
		{
			s = itm[i + 1].off - itm[i].off - 1;
			
			// deallocate block at itm[i]; - remembering that we've already squeezed out i items before it
			DeallocateIDs(ptr[itm[i].off - i]->fileOffset);
			
			// squeeze if out of the list
			if (s != 0)
				UMemory::Copy(((Uint32 *)ptr) + itm[i].off - i, ((Uint32 *)ptr) + itm[i].off + 1, s);
		}
		
		count = mList.GetItemCount() - (inCount + 1);
		mList.Truncate(count);
		// deallocate block at itm[inCount]
		DeallocateIDs(ptr[itm[i].off]->fileOffset);
		
		// flush cat list to disk
		s = count * sizeof(SMyNzHistFileCat);
		StPtr outData(s);
		SMyNzHistFileCat *p = (SMyNzHistFileCat *)BPTR(outData);
		
		for (i = 0; i != count; i++)
		{
			p->guid = ptr[i]->guid;
			p->offset = TB(ptr[i]->fileOffset);
			p->accessSN = TB(ptr[i]->accessSN);
		#if USE_ADD_SN
			p->addSN = TB(ptr[i]->addSN);
		#endif	
			p++;
		}
		
		mFile.ref->Write(GetByteOffset(mFile.catOffset, mFile.blockSize), BPTR(outData), s);
		
		// write the new count at the header
		Uint32 outCount = TB(count);
		mFile.ref->Write(OFFSET_OF(SMyNzHistFileHead, catCount), &outCount, sizeof(Uint32));
	}
	catch(...)
	{
		mFile.ref->Open(perm_Rd);
		throw;
	}	
}

// wants an open file
void CMyNewsReadHistory::DeallocateIDs(Uint32 inOffset)
{
	if (inOffset == 0)
		return;		// this item has no data on disk
		
	// determine how many blocks to deallocate
	Uint32 count;
	if (ReadFile(inOffset, 0, &count, sizeof(Uint32)) != sizeof(Uint32))
		Fail(errorType_Misc, error_Corrupt);
	
	count = FB(count) + 1;
	
	// deallocate count blocks at inOffset
	Deallocate(inOffset, count);
}

// wants an open file
void CMyNewsReadHistory::Deallocate(Uint32 inOffset, Uint32 inCount)
{
	Uint8 buf[1024];

	Uint32 fileOff = GetByteOffset(mFile.allocTabOffset, mFile.blockSize);
	Uint32 startByte = (inOffset - 1) / 8 + fileOff;
	Uint32 size = (inOffset + inCount + 8 - 1) / 8 + fileOff - startByte;
	
	if (size > sizeof(buf))
		Fail(errorType_Misc, error_OutOfRange);
		
	mFile.ref->Read(startByte, buf, size);

#if DEBUG
	Uint32 count = inCount;
	Uint32 start = (inOffset - 1) & 7;
	while (count--)
		if (!GetBit(buf, start++))
			DebugBreak("DeallocateBlocks: block not allocated!");
#endif
	
	Uint32 startBit = (inOffset - 1) & 7;
	while (inCount--)
		ClearBit(buf, startBit++);
	  
	mFile.ref->Write(startByte, buf, size);
	
	// this func should also check for the last allocated block and SetSize() of this file to only include the last one
}

Uint32 CMyNewsReadHistory::Allocate(Uint32 inCount)
{
	Uint8 buf[3072];	// this will be nice cached.
	Uint32 b;
	SMyNzAllocTableInfo fileAllocTab;
	
	// if inBlockCount = 1, we don't have to jump through too many hoops.  the firstFree is the one we allocate, and then set it to the next free
	// how big a block do we read 5
	if (!mFile.allocTabFirstFree)
	{
allocNewBlock:
		// nothing free!  need to allocate alloctable.
		// there's room at block (size * inBlockSize * 8) + 1 to allocate the alloc table.  Write it to the file 
		Uint32 oldAllocSize = mFile.allocTableSize * mFile.blockSize;
		Uint32 newAllocSize = oldAllocSize + mFile.blockSize;
		Uint32 oldOffset = mFile.allocTabOffset;
		Uint32 newOffset = mFile.allocTableSize * mFile.blockSize * 8 + 1;
		StPtr allocTab(newAllocSize);
		mFile.ref->Read(GetByteOffset(oldOffset, mFile.blockSize), BPTR(allocTab), oldAllocSize);
		
		UMemory::Clear(BPTR(allocTab) + oldAllocSize, newAllocSize - oldAllocSize);
		
		//in the new data, allocate the new alloc table. deallocate the old.
		Uint32 n = mFile.allocTableSize;
		oldOffset--;
		Uint32 tmpNewOff = newOffset - 1;
		while (n--)
		{
			ClearBit(BPTR(allocTab), oldOffset++);
			SetBit(BPTR(allocTab), tmpNewOff++);
		}
		
		SetBit(BPTR(allocTab), tmpNewOff++);
		
		mFile.ref->Write(GetByteOffset(newOffset, mFile.blockSize), BPTR(allocTab), newAllocSize);
		
		// update header's info
		mFile.allocTabOffset = newOffset;
		mFile.allocTableSize++;
		
		if (mFile.allocTabFirstFree > (oldOffset + 7) / 8)
			mFile.allocTabFirstFree = (oldOffset + 7) / 8;

		fileAllocTab.offset = TB(mFile.allocTabOffset);
		fileAllocTab.size = TB(mFile.allocTableSize);
		fileAllocTab.firstFree = TB(mFile.allocTabFirstFree);
		
		mFile.ref->Write(OFFSET_OF(SMyNzHistFileHead, allocTab), &fileAllocTab, sizeof(SMyNzAllocTableInfo));
	
		// check the old location of the alloc table to see if it will hold what we wish to allocate.
		// else, set the next block to the next item in the alloc table.  ie, s bits into the table
	}
	
	Uint32 firstFree = RoundDown4(mFile.allocTabFirstFree - 1);
	Uint32 allocOffset = GetByteOffset(mFile.allocTabOffset, mFile.blockSize);
	
tryNextRead:
	
	Uint32 size = min((Uint32)sizeof(buf), mFile.allocTableSize * mFile.blockSize - firstFree);
	
	mFile.ref->Read(allocOffset + firstFree, buf, size);
	
	Uint32 *p = (Uint32 *)buf;
	Int32 fourByteCount = size / sizeof(Uint32);
	Uint32 bit;
	
keepSearching:	
	while (fourByteCount--)
	{
		if (*p != 0xFFFFFFFF)
		{
			// now search the bits for a free one.
			for (bit = 0; bit != 32; bit++)
			{
				if (!GetBit(p, bit))
				{
					// if we haven't read in enough data to check the full range we need
					if ((fourByteCount + 1) * 4 * 8 - bit < inCount)
					{
						// update vars and read in the next chunk
						// make sure this block is re-read
						// but first, check to see that additional space is present.
						// if not, allocate new space

						goto outOfBlocks;
					}
					
					// check the next x bits
					for (b = 1; b != inCount; b++)
					{
						if (GetBit(p, bit + b))
						{
							goto notFound;
						}				
					}
					goto foundSpace;
					
notFound:			;		
				}
			
			}		
		}

		p++;
	}
	
outOfBlocks:

	if (sizeof(buf) > mFile.allocTableSize * mFile.allocTabOffset - firstFree + (mFile.allocTabOffset + 7) / 8)
	{
		// need to allocate a new block
		goto allocNewBlock;
	}
	else
	{
		firstFree += size - (fourByteCount + 1) * 4;
		goto tryNextRead;
	}
	
foundSpace:

	// allocate the space
	// save the alloc table
	// find the next free block - may require additional reading.
	// write the header info with this.
	
	b = bit;
	while (inCount--)
		SetBit(p, b++);
		
	mFile.ref->Write(allocOffset + firstFree + (BPTR(p) - buf), BPTR(p), (b + 7) / 8);
	
	return (firstFree + (BPTR(p) - buf)) * 8 + bit + 1;
	
	// what I want is to find the first open space. 
	// when I find a free bit beginning in this space, loop inBlockCount times and if I hit a bit, jump forward the number of bits I looped until my start is in the next block
	// if we hit this stage, go back to reading in Uint32s above.
	// once the Uint32s run out, we read in another chunk of the allocTable and repeat.
	// if this all runs out, we need to reallocate the alloctable.  Once this is done we can just go ahead and allocate becase we know that the newly-allocated block has heaps of space.
			
	// if we do not have space, we must allocate an additional block for the alloc table.  
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

CNZReadList::CNZReadList(TPtr inData, Uint32 inDataSize)
{
	mRefs = 1;
	mHasChanged = false;
	mIDCount = inDataSize / sizeof(Uint32);
	mIDDataSize = inDataSize;
	mID = (Uint32 *)inData;
	
	mTotalCount = 0;
	
	mCheckedList.InsertItems(1, mIDCount, false);
}

CNZReadList::~CNZReadList()
{
	if (mID)
		UMemory::Dispose((TPtr)mID);
}

void CNZReadList::SetRead(Uint32 inID, bool inRead)
{
/*
	if (!mID)
	{
		if(!inRead)
			return;
			
		mID = (Uint32 *)UMemory::New(16 * sizeof(Uint32));
		
		if(!mID)
			Fail(errorType_Memory, memError_NotEnough);
		
		mIDDataSize = 16 * sizeof(Uint32);
		mID[0] = 0;
	
	}
*/	
	Uint32 first = 0;
	if (!mID || !mIDCount)
		goto unread;
	
	Uint32 last = mIDCount - 1;
	Uint32 middle;
	Uint32 count;
	
	while (first != last)
	{
		middle = (first + last)/2;
	
		if (mID[middle] < inID)
		 	first = middle + 1;
		 else
		 	last = middle;
	}
	
	if (mID[first] == inID)
	{
		if (inRead)	// we've already read this
			return;
		
		// this id exists and we want to remove it
		last++;
		count = mIDCount - first;
		
		mCheckedList.RemoveItem(first + 1);

		while (count--)
			mID[first++] = mID[last++];
		
		mIDCount--;
	}
	else
	{
		if (mID[first] < inID)
		{
			first++;
		}

unread:
		if (!inRead)	// it's not read and we want to set it to unread
			return;
	
		// this id doesn't exist and we wish to insert it
		// check if we need to resize
		if (mIDDataSize < (mIDCount + 1) * sizeof(Uint32))
		{
			Uint32 s = (mIDCount + 32) * sizeof(Uint32);
			TPtr p = UMemory::Reallocate((TPtr)mID, s);
			
			if (!p)
				Fail(errorType_Memory, memError_NotEnough);
				
			mID = (Uint32 *)p;
			mIDDataSize = s;
		}
		
		count = mIDCount - first;
		middle = first;
		first = mIDCount;
		last = first - 1;
		
		mIDCount++;

		while (count--)
			mID[first--] = mID[last--];
		
		mID[middle] = inID;
		mCheckedList.InsertItem(middle + 1, true);
	}

	mHasChanged = true;
}

bool CNZReadList::CheckRead(Uint32 inID)
{
	if (!mID || !mIDCount)
		return false;
		
	Uint32 first = 0;
	Uint32 last = mIDCount - 1;
	Uint32 middle;
	
	while (first != last)
	{
		middle = (first + last)/2;
	
		if (mID[middle] < inID)
		 	first = middle + 1;
		 else
		 	last = middle;
	}
	
	if (mID[first] == inID)
	{
		mCheckedList.SetItem(first + 1, true);
		return true;
	}
	
	return false;
}

void CNZReadList::PurgeUnchecked()
{
	// I don't like the way this does what it does...want to think it over again when I have time
	
	Uint32 count = mIDCount + 1;
	Uint8 *p = BPTR(mID);
	
	if (!mIDCount)
		return;
		
	Uint32 i = 1;
	Uint32 firstSet;
	
	for (;;)
	{
		if (i == count) break;
		// skip over all unset
		while (!mCheckedList.GetItem(i))
		{
			if (i++ == count) goto done;
		}
		
		firstSet = i - 1;
		// count up number of set
		while (mCheckedList.GetItem(i))
		{
			if (i++ == count) break;
		}
		
		// write the set blocks
		p += UMemory::Copy(p, BPTR(mID) + firstSet * sizeof(Uint32), (i - firstSet - 1) * sizeof(Uint32));
	}
	
done:
	mIDCount = (p - BPTR(mID)) / sizeof(Uint32);
	mCheckedList.Clear();
	mCheckedList.InsertItems(1, mIDCount, false);
	
	if (count - 1 != mIDCount)
		mHasChanged = true;
}

void CNZReadList::ResetChecked()
{
	mCheckedList.SetItems(1, mIDCount, false);
}

#endif
