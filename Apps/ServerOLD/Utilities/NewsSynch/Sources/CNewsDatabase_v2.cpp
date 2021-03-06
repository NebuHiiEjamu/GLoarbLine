/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CNewsDatabase.h"


// block size must be a power of 2 and greater or equal to 256
// default is 256, although 512, 1024 also make sense for large articles

#pragma options align=packed
struct _NZ_SAllocTableInfo_v2
{
//	Uint32 allocTableChecksum;		// ensure it's not corrupt
	Uint32 offset;
	Uint32 size;		// in blocks
	Uint32 firstFree;	// offset of first byte with free bit - 1 is the first block, 0 indicates none free
	
};

struct _NZ_SDBHeader_v2
{
	// should I store a flag for endian and allow little-endian to store in native format?
	// don't need to store since type would be ZNLH - could use that
	// let's maybe add support in the future...not now.
	
	Uint32 type;	// = HLNZ
	Uint32 vers;	// 2
	//Uint32 id;	// maybe make this a GUID - 128 bits = 16 bytes
	SGUID id;
	
	// increment by 1 each time this file is updated
	Uint32 addSerial;	// incremented by 1 each time an article is added
	Uint32 delSerial;	// incremented by 1 each time an article is deleted

	Uint32 blockSize;
	
	Uint32 listOffset;	// cashed list offset
	Uint32 listSize;	// cashed list size
	
	Uint32 articleCount;
	Uint32 firstArticleOffset;
	Uint32 lastArticleOffset;
	
	Uint32 nextArticleID;

	_NZ_SAllocTableInfo_v2 allocTable;
	
	// there is a hash table that stores a sorted list of offest/id pairs
	// the size is hashMask * sizeof(_NZ_SHashInfo)
	// hashValue is the least-significant bits of the ID
	// the application has the right to rebuild the hash table if it chooses to. (to change the mask)
//	Uint32 offSetIDHashChecksum;
	Uint32 offSetIDHashOffset;	// offset to a list of _NZ_SHashInfo
	Uint32 offSetIDHashMask;	// mask to apply to ID to get the hash  

	Uint32 rsvdB[3];
	Uint8 name[32];
	Uint8 description[256];	// pstring
};

struct _NZ_SHashInfo_v2
{
	// do we need hashValue? we can find it in the list simply by using offsets.
	// ie, to get to this record we go offSetIDHashOffset + (id & hashMask) * sizeof(_NZ_SHashInfo_v2)
	
	// the offset table is sorted by ID.  Easy since all IDs are incremently added
	Uint32 offset;		// offset into file of _NZ_SIDOffset table
	Uint32 size;		// in _NZ_SIDOffset records of the data
};

struct _NZ_SIDOffset_v2
{
	Uint32 id;
	Uint32 offset;
};

#if 0
// this is the actual struct of an SNewsBlock
struct
{
	Uint32 nextFlav;
	Uint32 size;
	pstring type;
	Uint8 padding;
	Uint8 data[size];
};
#endif

struct _NZ_SGenericBlock_v2
{
	Uint32 nextFlav;
	Uint32 size;
	Uint8 type[];	// pstring
};

struct _NZ_SNewsItemBlock_v2
{
	Uint32 nextFlav;
	Uint32 size;
	Uint8 type[_NZ_DataType_ItemLen + 1];
	Uint8 padding[3/*(_NZ_DataType_ItemLen + 1 + 3) & 3*/];
	
	struct
	{
		Uint32 id;
		Uint32 prevArticleOffset;
		Uint32 nextArticleOffset;
		Uint32 parentOffset;
		Uint32 firstChildOffset;
		
		SDateTimeStamp date;
		Uint32 flags;
		Uint8 title[64];
		Uint8 poster[32];
		Uint8 externId[64];
	} body;

};
#pragma options align=reset

#if 0
// GetOldArticleListing is in the form of:
struct
{
	Uint32 groupID;	// dummy
	Uint32 count;
	pstring name;	// dummy
	pstring desc;	// dummy
	struct
	{
		Uint32 id;
		SDateTimeStamp date;
		Uint32 parentID;	// parent is the parent in the thread.  0 if I have none
		Uint32 flags;
		Uint16 flavorCount;	// 2 bytes for compatibilty

		pstring title;
		pstring poster;
		struct
		{
			pstring flavor;	// mime type
			Uint16 size;
		} flavor[flavorCount];
		
	} article[count];
} articleList;
#endif

const Uint32 nullData[] = {		// 1024 bytes
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 

	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 

	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 

	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };


class CNZArticleList_v2
{
	public:
		CNZArticleList_v2();
		CNZArticleList_v2(Uint32 inCount, bool inNewFormat = false);
		virtual ~CNZArticleList_v2();
		
		void AddArticle(const _NZ_SNewsItemBlock_v2 &inArticBlock, Uint32 inArticOffset);
		void AddNewArticle(const _NZ_SNewsItemBlock_v2 &inArticBlock, Uint32 inArticOffset);
		void AddArticleFlavor(const _NZ_SGenericBlock_v2 &inFlavBlock);
		void AddNewArticleFlavor(const _NZ_SGenericBlock_v2 &inFlavBlock);
		
		TPtr DetatchDataPtr(Uint32 &outSize);
		
	protected:
		void Reallocate(Uint32 inMinSize);
		
		Uint32 LookupID(Uint32 inOffset);
		void AddID(Uint32 inID, Uint32 inOffset);
	
		TPtr mData;
		Uint32 mDataSize;
		Uint32 mOutSize;
		
		Uint8 *mPtr;
		Uint32 mCount;
		
		Uint32 *mArticleCountPtr;
		Uint16 *mCurrentFlavCountPtr;
		
		TPtr mIDOffTable;
		Uint32 mIDOffTabSize;
		Uint32 mIDOffCount;

};

#define _NZ_GetByteOffset_v2(o, b)		(512 + (o - 1) * b)
Uint32 _NZ_AllocateBlocks_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SAllocTableInfo_v2 &ioAllocTable, Uint32 inBlockCount);
void _NZ_DeallocateBlocks_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SAllocTableInfo_v2 &ioAllocTable, Uint32 inOffset, Uint32 inBlockCount);
Uint32 _NZ_IDToOffset_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inID, const _NZ_SHashInfo_v2 &inHashInfo);
Uint32 _NZ_OffsetToID_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inOffset);
void _NZ_AddIDToHash_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SHashInfo_v2 &ioHashInfo, Uint32 inByteHashInfoOff, Uint32 inID, Uint32 inOffset, _NZ_SAllocTableInfo_v2 &ioAllocTable);
void _NZ_RmIDFromHash_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SHashInfo_v2 &ioHashInfo, Uint32 inID, _NZ_SAllocTableInfo_v2 &ioAllocTable);
Uint32 _NZ_GetLast_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inFirstOffset, Uint32 inOffsetOffset);
void _NZ_AddToList_v2(CNZArticleList_v2 *inList, TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inFirstArtic, Uint32 &ioArticCount, bool inNewFormat = false);
void _NZ_DelArticSibsChilds_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SAllocTableInfo_v2 &ioAllocTable, _NZ_SHashInfo_v2 *ioHashInfo, Uint32 inHashMask, Uint32 inArticOff, Uint32 &ioCount);
void _NZ_DelFlavs_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SAllocTableInfo_v2 &ioAllocTable, Uint32 inFirstOff);
void _NZ_ChangeParentOffset_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inArticOff, Uint32 inParentOff);
Uint32 _NZ_GetLastOffset_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inArticOff);
Uint32 _NZ_GenerateExternID(Uint8 *outExternID, Uint32 inMaxSize);


CNewsDatabase_v2::CNewsDatabase_v2(TFSRefObj* inFile)
{
	mFile = inFile->Clone();
}

CNewsDatabase_v2::~CNewsDatabase_v2()
{
	UFileSys::Dispose(mFile);
}

// throws file sys errors
void CNewsDatabase_v2::CreateNewGroup(const Uint8 inName[], const Uint8 inDesc[])
{
	const Uint32 blockSize = 256;
	const Uint32 hashMask = 31;
	const Uint32 hashSize = (((hashMask + 1) * sizeof(_NZ_SHashInfo_v2) + blockSize - 1) & ~(blockSize - 1));
	
	Uint8 buf[512 + blockSize + hashSize];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	Uint8 *allocTabPtr = buf + 512;
	Uint8 *hashTabPtr = allocTabPtr + blockSize;
	
	// set the defaults
	UMemory::Clear(buf, sizeof(buf));
	
	head->type = TB('HLNZ');
	head->vers = TB((Uint32)2);

#if CONVERT_INTS
	SGUID id;
	UGUID::Generate(id);
	UGUID::Flatten(id, &head->id);
#else
	UGUID::Generate(head->id);
#endif	

	head->addSerial = TB((Uint32)1);
	head->delSerial = TB((Uint32)1);
	head->blockSize = TB(blockSize);
	head->nextArticleID = TB((Uint32)1);
	head->allocTable.offset = TB((Uint32)1);
	head->allocTable.size = TB((Uint32)1);
	head->allocTable.firstFree = TB((Uint32)1);
	
	head->offSetIDHashOffset = TB((Uint32)2);
	head->offSetIDHashMask = TB(hashMask);
	head->name[0] = UMemory::Copy(head->name + 1, inName + 1, min((Uint32)inName[0], (Uint32)(sizeof(head->name) - 1)));
	pstrcpy(head->description, inDesc);
	
	// allocate a block for the actual allocTable
	Uint32 i = 0;
	SetBit(allocTabPtr, i++);	
	
	// allocate the hash table
	Uint32 hashBlocks = hashSize / blockSize;
	while(hashBlocks--)
		SetBit(allocTabPtr, i++);
	
	// write to disk
	mFile->CreateFile('HLNZ', 'HTLS');
	
	StFileOpener open(mFile);
	mFile->Write(0, buf, sizeof(buf));
}

TPtr CNewsDatabase_v2::GetArticleListing(Uint32 &outSize)
{
	outSize = 0;
	StFileOpener open(mFile);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);
	Uint32 articCount = FB(head->articleCount);

	CNZArticleList_v2 list(articCount, true);
	_NZ_AddToList_v2(&list, mFile, blockSize, head->firstArticleOffset, articCount, true);

	return list.DetatchDataPtr(outSize);
}

TPtr CNewsDatabase_v2::GetOldArticleListing(Uint32 &outSize)
{
	outSize = 0;
	StFileOpener open(mFile);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);

	TPtr listData = nil;
	Uint32 listSize;

	// read cashed article list
	if(head->listOffset)
	{
		listSize = FB(head->listSize);
	
		listData = UMemory::New(listSize);
		if (!listData)
			Fail(errorType_Memory, memError_NotEnough);
		
		if(mFile->Read(_NZ_GetByteOffset_v2(FB(head->listOffset), blockSize), listData, listSize) != listSize)
			Fail(errorType_Misc, error_Corrupt);

		outSize = listSize;
		return listData;
	}
		
	Uint32 articCount = FB(head->articleCount);

	CNZArticleList_v2 list(articCount);
	_NZ_AddToList_v2(&list, mFile, blockSize, head->firstArticleOffset, articCount);

	listData = list.DetatchDataPtr(listSize);
	outSize = listSize;
	
	// cash article list
	Uint32 listOffset = _NZ_AllocateBlocks_v2(mFile, blockSize, head->allocTable, listSize / blockSize + 1);
	mFile->Write(_NZ_GetByteOffset_v2(listOffset, blockSize), listData, listSize);
		
	listOffset = FB(listOffset);
	mFile->Write(OFFSET_OF(_NZ_SDBHeader_v2, listOffset), &listOffset, sizeof(listOffset));

	listSize = FB(listSize);
	mFile->Write(OFFSET_OF(_NZ_SDBHeader_v2, listSize), &listSize, sizeof(listSize));
	
	return listData;
}

void CNewsDatabase_v2::GetGroupInfo(SMyNewsGroupInfo &outInfo)
{
	StFileOpener open(mFile, perm_Read);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	UGUID::Unflatten(outInfo.id, &head->id);
	outInfo.articleCount = FB(head->articleCount);
	outInfo.addSN = FB(head->addSerial);
	outInfo.deleteSN = FB(head->delSerial);
	
	outInfo.nameSize = UMemory::Copy(outInfo.name, head->name + 1, min((Uint32)31, (Uint32)head->name[0]));
	UMemory::Clear(outInfo.name + outInfo.nameSize, sizeof(outInfo.name) - outInfo.nameSize);
	
	outInfo.descSize = UMemory::Copy(outInfo.desc, head->description + 1, head->description[0]);
	UMemory::Clear(outInfo.desc + outInfo.descSize, sizeof(outInfo.desc) - outInfo.descSize);
}

bool CNewsDatabase_v2::GetArticleInfo(Uint32 inArticleID, SMyNewsArticleInfo& outInfo)
{
	StFileOpener open(mFile, perm_Read);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);
	
	// read in the appropriate hash record
	_NZ_SHashInfo_v2 hash;
	if(mFile->Read(_NZ_GetByteOffset_v2(FB(head->offSetIDHashOffset), blockSize) + (inArticleID & FB(head->offSetIDHashMask)) * sizeof(_NZ_SHashInfo_v2), &hash, sizeof(hash)) != sizeof(_NZ_SHashInfo_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	Uint32 off = _NZ_IDToOffset_v2(mFile, blockSize, inArticleID, hash);
	
	if(!off)
		Fail(errorType_Misc, error_NoSuchItem);
	
	_NZ_SNewsItemBlock_v2 item;
	if(mFile->Read(_NZ_GetByteOffset_v2(off, blockSize), &item, sizeof(item)) != sizeof(item))
		Fail(errorType_Misc, error_Corrupt);
	
	outInfo.id = FB(item.body.id);
	outInfo.prevID = _NZ_OffsetToID_v2(mFile, blockSize, FB(item.body.prevArticleOffset));
	outInfo.nextID = _NZ_OffsetToID_v2(mFile, blockSize, FB(item.body.nextArticleOffset));		
	outInfo.parentID = _NZ_OffsetToID_v2(mFile, blockSize, FB(item.body.parentOffset));
	outInfo.firstChildID = _NZ_OffsetToID_v2(mFile, blockSize, FB(item.body.firstChildOffset));
	outInfo.flags = FB(item.body.flags);
	outInfo.dts.Unflatten(&item.body.date);
	outInfo.title[0] = UMemory::Copy(outInfo.title + 1, item.body.title + 1, min((Uint32)item.body.title[0], (Uint32)63));
	outInfo.poster[0] = UMemory::Copy(outInfo.poster + 1, item.body.poster + 1, min((Uint32)item.body.poster[0], (Uint32)31));
	outInfo.externId[0] = UMemory::Copy(outInfo.externId + 1, item.body.externId + 1, min((Uint32)item.body.externId[0], (Uint32)63));
	
	return true;
}

TPtr CNewsDatabase_v2::GetArticleFlavors(Uint32 inArticleID, Uint32 &outSize)
{
	outSize = 0;
	StFileOpener open(mFile);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);

	// read in the appropriate hash record
	_NZ_SHashInfo_v2 hash;
	if(mFile->Read(_NZ_GetByteOffset_v2(FB(head->offSetIDHashOffset), blockSize) + (inArticleID & FB(head->offSetIDHashMask)) * sizeof(_NZ_SHashInfo_v2), &hash, sizeof(hash)) != sizeof(_NZ_SHashInfo_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	Uint32 off = _NZ_IDToOffset_v2(mFile, blockSize, inArticleID, hash);
	
	if(!off)
		Fail(errorType_Misc, error_NoSuchItem);
	
	_NZ_SNewsItemBlock_v2 item;
	if(mFile->Read(_NZ_GetByteOffset_v2(off, blockSize), &item, sizeof(item)) != sizeof(item))
		Fail(errorType_Misc, error_Corrupt);
	
	// loop thru flavs 'till we get the one we want
	_NZ_SGenericBlock_v2 *blk = (_NZ_SGenericBlock_v2 *)buf;
	
	Uint32 maxFlavs = 30;
	Uint32 readSize = min(blockSize, (Uint32)sizeof(buf), (Uint32)(sizeof(_NZ_SGenericBlock_v2) + 64));
	blk->nextFlav = item.nextFlav;
	
	CNZArticleList_v2 list;
	while(maxFlavs-- && blk->nextFlav)
	{
		off = _NZ_GetByteOffset_v2(FB(blk->nextFlav), blockSize);
		
		if(mFile->Read(off, buf, readSize) != readSize)
			Fail(errorType_Misc, error_Corrupt);
	
		list.AddNewArticleFlavor(*blk);
	}

	return list.DetatchDataPtr(outSize);
}

TPtr CNewsDatabase_v2::GetArticleData(Uint32 inArticleID, const Uint8 *inName, Uint32 &outSize, Int8 outType[], SMyNewsArticleInfo *outInfo)
{
	if (UMemory::Compare(inName + 1, inName[0], ".body", 5))
		return nil;

	StFileOpener open(mFile, perm_Read);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);
	
	// read in the appropriate hash record
	_NZ_SHashInfo_v2 hash;
	if(mFile->Read(_NZ_GetByteOffset_v2(FB(head->offSetIDHashOffset), blockSize) + (inArticleID & FB(head->offSetIDHashMask)) * sizeof(_NZ_SHashInfo_v2), &hash, sizeof(hash)) != sizeof(_NZ_SHashInfo_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	Uint32 off = _NZ_IDToOffset_v2(mFile, blockSize, inArticleID, hash);
	
	if(!off)
		Fail(errorType_Misc, error_NoSuchItem);
	
	_NZ_SNewsItemBlock_v2 item;
	
	if(mFile->Read(_NZ_GetByteOffset_v2(off, blockSize), &item, sizeof(item)) != sizeof(item))
		Fail(errorType_Misc, error_Corrupt);
	
	// loop thru flavs 'till we get the one we want
	_NZ_SGenericBlock_v2 *blk = (_NZ_SGenericBlock_v2 *)buf;
	
	Uint32 maxFlavs = 30;
	
	Int8 *pDataType = "text/plain";
	Uint32 typeLen = strlen(pDataType);
	Uint32 readSize = min(blockSize, (Uint32)sizeof(buf), (Uint32)(sizeof(_NZ_SGenericBlock_v2) + 1 + typeLen));
	TPtr outData = nil;
	
	blk->nextFlav = item.nextFlav;
	
	while(maxFlavs-- && blk->nextFlav)
	{
		off = _NZ_GetByteOffset_v2(FB(blk->nextFlav), blockSize);
		
		if(mFile->Read(off, buf, readSize) != readSize)
			Fail(errorType_Misc, error_Corrupt);
		
		if(blk->type[0] == typeLen && UMemory::Equal(blk->type + 1, (Uint8 *)pDataType, typeLen))
		{
			Uint32 s = FB(blk->size);
			
			try
			{
				outData = UMemory::New(s);
				if(outData)
				{
					if(mFile->Read(off + sizeof(_NZ_SGenericBlock_v2) + RoundUp4(typeLen + 1), BPTR(outData), s) != s)
						Fail(errorType_Misc, error_Corrupt);
					
					if (outType)
						UMemory::Copy(outType, pDataType, strlen(pDataType) + 1);

					if(outInfo)
					{
						outInfo->id = FB(item.body.id);
						outInfo->prevID = _NZ_OffsetToID_v2(mFile, blockSize, FB(item.body.prevArticleOffset));
						outInfo->nextID = _NZ_OffsetToID_v2(mFile, blockSize, FB(item.body.nextArticleOffset));		
						outInfo->parentID = _NZ_OffsetToID_v2(mFile, blockSize, FB(item.body.parentOffset));
						outInfo->firstChildID = _NZ_OffsetToID_v2(mFile, blockSize, FB(item.body.firstChildOffset));
						outInfo->flags = FB(item.body.flags);
						outInfo->dts.Unflatten(&item.body.date);
						outInfo->title[0] = UMemory::Copy(outInfo->title + 1, item.body.title + 1, min((Uint32)item.body.title[0], (Uint32)63));
						outInfo->poster[0] = UMemory::Copy(outInfo->poster + 1, item.body.poster + 1, min((Uint32)item.body.poster[0], (Uint32)31));
						outInfo->externId[0] = UMemory::Copy(outInfo->externId + 1, item.body.externId + 1, min((Uint32)item.body.externId[0], (Uint32)63));
					}
					
					outSize = s;	
					return outData;
				}
				else
					Fail(errorType_Memory, memError_NotEnough);
			}
			catch(...)
			{
				if(outData)
				{
					delete outData;
					outData = nil;	
				}
				
				throw;
			}
		}
	}
	
	Fail(errorType_Misc, error_NoSuchItem);
	return nil;
}

Uint32 CNewsDatabase_v2::AddArticle(Uint32 inParentID, const Uint8 inTitle[], const Uint8 inPoster[], Uint32 inFlags, const SDateTimeStamp *inDTS, const Uint8 *inExternID)
{
	StFileOpener open(mFile);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);
	
	Uint32 hashMask = FB(head->offSetIDHashMask);
	Uint32 hashSize = (hashMask + 1) * sizeof(_NZ_SHashInfo_v2);
	Uint32 hashFileOffset = _NZ_GetByteOffset_v2(FB(head->offSetIDHashOffset), blockSize);
	StPtr idHashBuf(hashSize);
	mFile->Read(hashFileOffset, BPTR(idHashBuf), hashSize);
	_NZ_SHashInfo_v2 *idHash = (_NZ_SHashInfo_v2 *)BPTR(idHashBuf);
	
	Uint32 parentOffset = 0;
	if(inParentID)
	{
		// find it in the hash table
		parentOffset = _NZ_IDToOffset_v2(mFile, blockSize, inParentID, idHash[inParentID & hashMask]);

#if DEBUG
		if(!parentOffset)
			DebugBreak("Cannot find this parent!");
#endif
	}
	
	// off will be the offset pointed to by firstFree...
	// this must write the header to inFile as well as set that in the allocTable
	Uint32 off = _NZ_AllocateBlocks_v2(mFile, blockSize, head->allocTable, 1);
	Uint32 netByteOrderOff = TB(off);	// if windows has to suffer for swaping byte order, macos can suffer copying these temps
	Uint32 id = FB(head->nextArticleID);
	
	try
	{
		// add this article info to offset off
		// then update the parents and siblings' references
		
		StPtr block(blockSize);
		
		UMemory::Clear(BPTR(block), blockSize);
		
		_NZ_SNewsItemBlock_v2 *itm = (_NZ_SNewsItemBlock_v2 *)BPTR(block);
		
		// header stuff
		itm->size = TB((Uint32)sizeof(itm->body));
		pstrcpy(itm->type, _NZ_DataType_Item);
	
		// body stuff
		itm->body.id = head->nextArticleID;	// already in network byte order
		itm->body.parentOffset = TB(parentOffset);
		
		if(inDTS)
			itm->body.date = *inDTS;
		else
			UDateTime::GetDateTimeStamp(itm->body.date);
		
#if CONVERT_INTS
		itm->body.date.year = TB(itm->body.date.year);
		itm->body.date.msecs = TB(itm->body.date.msecs);
		itm->body.date.seconds = TB(itm->body.date.seconds);
#endif
		
		itm->body.flags = TB(inFlags);
		itm->body.title[0] = UMemory::Copy(itm->body.title + 1, inTitle + 1, min((Uint32)inTitle[0], (Uint32)(sizeof(itm->body.title) - 1)));
		itm->body.poster[0] = UMemory::Copy(itm->body.poster + 1, inPoster + 1, min((Uint32)inPoster[0], (Uint32)(sizeof(itm->body.poster) - 1)));
		
		if (inExternID && inExternID[0])
			itm->body.externId[0] = UMemory::Copy(itm->body.externId + 1, inExternID + 1, min((Uint32)inExternID[0], (Uint32)(sizeof(itm->body.externId) - 1)));
		else
			itm->body.externId[0] = _NZ_GenerateExternID(itm->body.externId + 1, sizeof(itm->body.externId) - 1);
					
		Uint32 prevArticleOffset = 0;
		
		// now see where this fits in the file based on inParentID
		if(parentOffset)
		{
			itm->body.parentOffset = TB(parentOffset);
			// load the parent and find my prevArticleOffset
			Uint32 firstChildOffset;
			if(mFile->Read(_NZ_GetByteOffset_v2(parentOffset, blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.firstChildOffset), &firstChildOffset, sizeof(firstChildOffset)) != sizeof(firstChildOffset))
				Fail(errorType_Misc, error_Corrupt);

			prevArticleOffset = _NZ_GetLast_v2(mFile, blockSize, FB(firstChildOffset), OFFSET_OF(_NZ_SNewsItemBlock_v2, body.nextArticleOffset));
			itm->body.prevArticleOffset = TB(prevArticleOffset);
			
			// next is 0
			// first child is 0
		}
		else
		{
			// this just gets slapped to the end
			itm->body.prevArticleOffset = head->lastArticleOffset;	// already in network byte order
			prevArticleOffset = FB(head->lastArticleOffset);
			head->lastArticleOffset = netByteOrderOff;
		}
		

// --- bug - if we fail after this block of code, then the category is corrupt as we don't change these values back to their previous state...
		// update prevArticle to store a ref to me
		if(prevArticleOffset)
		{
			mFile->Write(_NZ_GetByteOffset_v2(prevArticleOffset, blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.nextArticleOffset), &netByteOrderOff, sizeof(netByteOrderOff));
		}
		else
		{
			// the prev is 0 meaning this is the first article in its heirarchy...
			// ie, the first child of its parent
			if(parentOffset)
			{
				mFile->Write(_NZ_GetByteOffset_v2(parentOffset, blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.firstChildOffset), &netByteOrderOff, sizeof(netByteOrderOff));
			}
			else
			{
				// if there is no parent, this is the first in root
				head->firstArticleOffset = netByteOrderOff;
			}
		}
// ---		
		
		// add the ID+offset to the ID hash
			// if the id already exists, fail
		_NZ_AddIDToHash_v2(mFile, blockSize, idHash[id & hashMask], hashFileOffset + (id & hashMask) * sizeof(_NZ_SHashInfo_v2), id, off, head->allocTable);
	
		// write this article
		mFile->Write(_NZ_GetByteOffset_v2(off, blockSize), BPTR(block), blockSize);
		
		head->addSerial = TB(FB(head->addSerial) + 1);
		head->nextArticleID = TB(id + 1);
		head->articleCount = TB(FB(head->articleCount) + 1);
	
		// delete cached article list
		if(head->listOffset)
		{
			_NZ_DeallocateBlocks_v2(mFile, blockSize, head->allocTable, TB(head->listOffset), TB(head->listSize) / blockSize + 1);

			head->listOffset = 0;
			head->listSize = 0;
		}

		// update the header
		mFile->Write(0, buf, sizeof(buf));
	}
	catch(...)
	{
		// something failed! - deallocate the block at off
		_NZ_DeallocateBlocks_v2(mFile, blockSize, head->allocTable, off, 1);
		// TO DO remove the ID from the off/id hash
		
		
		throw;
	}
	
	return id;
}

bool CNewsDatabase_v2::SetArticle(Uint32 inArticleID, const Uint8 inTitle[], const Uint8 inPoster[], const Uint32 *inFlags, const SDateTimeStamp *inDTS, const Uint8 *inExternID)
{
	StFileOpener open(mFile);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);
	
	Uint32 hashMask = FB(head->offSetIDHashMask);
	Uint32 hashSize = (hashMask + 1) * sizeof(_NZ_SHashInfo_v2);
	Uint32 hashFileOffset = _NZ_GetByteOffset_v2(FB(head->offSetIDHashOffset), blockSize);
	StPtr idHashBuf(hashSize);
	mFile->Read(hashFileOffset, BPTR(idHashBuf), hashSize);
	_NZ_SHashInfo_v2 *idHash = (_NZ_SHashInfo_v2 *)BPTR(idHashBuf);
	
	// find it in the hash table
	Uint32 articOff = _NZ_IDToOffset_v2(mFile, blockSize, inArticleID, idHash[inArticleID & hashMask]);
	
	if(!articOff)
		Fail(errorType_Misc, error_NoSuchItem);

	Uint32 off = _NZ_GetByteOffset_v2(articOff, blockSize);
	_NZ_SNewsItemBlock_v2 itm;

	if (inTitle)
	{
		itm.body.title[0] = UMemory::Copy(itm.body.title + 1, inTitle + 1, min((Uint32)inTitle[0], (Uint32)(sizeof(itm.body.title) - 1)));
		mFile->Write(off + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.title), itm.body.title, itm.body.title[0] + 1);
	}

	if (inPoster)
	{
		itm.body.poster[0] = UMemory::Copy(itm.body.poster + 1, inPoster + 1, min((Uint32)inPoster[0], (Uint32)(sizeof(itm.body.poster) - 1)));
		mFile->Write(off + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.poster), itm.body.poster, itm.body.poster[0] + 1);
	}

	if (inFlags)
	{
		itm.body.flags = TB(*inFlags);
		mFile->Write(off + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.flags), &itm.body.flags, sizeof(itm.body.flags));
	}
	
	if (inDTS)
	{
		itm.body.date = *inDTS;
		
#if CONVERT_INTS
		itm.body.date.year = TB(itm.body.date.year);
		itm.body.date.msecs = TB(itm.body.date.msecs);
		itm.body.date.seconds = TB(itm.body.date.seconds);
#endif

		mFile->Write(off + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.date), &itm.body.date, sizeof(itm.body.date));
	}
	
	if (inExternID)
	{
		itm.body.externId[0] = UMemory::Copy(itm.body.externId + 1, inExternID + 1, min((Uint32)inExternID[0], (Uint32)(sizeof(itm.body.externId) - 1)));
		mFile->Write(off + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.externId), itm.body.externId, itm.body.externId[0] + 1);
	}

	return true;
}

bool CNewsDatabase_v2::AddArticleData(Uint32 inArticleID, const Uint8 *inName, const Int8 inType[], const Uint8 *inData, Uint32 inDataSize)
{
	if (UMemory::Compare(inName + 1, inName[0], ".body", 5))
		return false;

	StFileOpener open(mFile);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);
	
	Uint32 hashMask = FB(head->offSetIDHashMask);
	Uint32 hashSize = (hashMask + 1) * sizeof(_NZ_SHashInfo_v2);
	Uint32 hashFileOffset = _NZ_GetByteOffset_v2(FB(head->offSetIDHashOffset), blockSize);
	StPtr idHashBuf(hashSize);
	mFile->Read(hashFileOffset, BPTR(idHashBuf), hashSize);
	_NZ_SHashInfo_v2 *idHash = (_NZ_SHashInfo_v2 *)BPTR(idHashBuf);
	
	// find it in the hash table
	Uint32 articOff = _NZ_IDToOffset_v2(mFile, blockSize, inArticleID, idHash[inArticleID & hashMask]);
	
	if(!articOff)
		Fail(errorType_Misc, error_NoSuchItem);

	// now we can add. 
	
	// TO DO..check if we already have this flav
	
	Uint32 typeLen = min((Uint32)strlen(inType), (Uint32)31);
	Uint32 headSize = sizeof(_NZ_SGenericBlock_v2) + RoundUp4(1 + typeLen);
	
	Uint32 flav = _NZ_GetLast_v2(mFile, blockSize, articOff, OFFSET_OF(_NZ_SNewsItemBlock_v2, nextFlav));
	
	if(!flav)
	{
		Fail(errorType_Misc, error_Corrupt);
	}
	
	// allocate space for new flav
	Uint32 flavOff = _NZ_AllocateBlocks_v2(mFile, blockSize, head->allocTable, (headSize + inDataSize + blockSize - 1) / blockSize);
	
	Uint8 data[256];
	*((Uint32 *)(data + headSize - 4)) = 0;	// set the padding first
	
	_NZ_SGenericBlock_v2 *blk = (_NZ_SGenericBlock_v2 *)data;
	
	blk->nextFlav = 0;
	blk->size = TB(inDataSize);
	blk->type[0] = UMemory::Copy(blk->type + 1, inType, typeLen);
	
	Uint32 flavByteOff = _NZ_GetByteOffset_v2(flavOff, blockSize);
	
	flavByteOff += mFile->Write(flavByteOff, data, headSize);
	flavByteOff += mFile->Write(flavByteOff, inData, inDataSize);
	
	Uint32 remainingSize = blockSize - (flavByteOff % blockSize);
	ASSERT(remainingSize <= sizeof(nullData) * sizeof(Uint32));
	if(remainingSize)
		flavByteOff += mFile->Write(flavByteOff, nullData, remainingSize);
	
	// now write to the last flav this flavor
	Uint32 netFlavOff = TB(flavOff);
	mFile->Write(_NZ_GetByteOffset_v2(flav, blockSize) + OFFSET_OF(_NZ_SGenericBlock_v2, nextFlav),  &netFlavOff, sizeof(Uint32));
		
	return true;
}

// should not return anything
bool CNewsDatabase_v2::DeleteArticle(Uint32 inArticleID, bool inDeleteChildren)
{
	// find the ID
	// if delete children, it's easy. - loop thru all children and call the delete func
	// else, replace this article with his first child
	
	// with each delete, deallocate the space for all flavors

	StFileOpener open(mFile);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);
	Uint32 articCount = FB(head->articleCount);
	
	Uint32 hashMask = FB(head->offSetIDHashMask);
	Uint32 hashSize = (hashMask + 1) * sizeof(_NZ_SHashInfo_v2);
	Uint32 hashFileOffset = _NZ_GetByteOffset_v2(FB(head->offSetIDHashOffset), blockSize);
	StPtr idHashBuf(hashSize);
	mFile->Read(hashFileOffset, BPTR(idHashBuf), hashSize);
	_NZ_SHashInfo_v2 *idHash = (_NZ_SHashInfo_v2 *)BPTR(idHashBuf);
	
	// find it in the hash table
	Uint32 articOff = _NZ_IDToOffset_v2(mFile, blockSize, inArticleID, idHash[inArticleID & hashMask]);
	
	if(!articOff)
		Fail(errorType_Misc, error_NoSuchItem);

	_NZ_SNewsItemBlock_v2 itm;
	if(mFile->Read(_NZ_GetByteOffset_v2(articOff, blockSize), &itm, sizeof(_NZ_SNewsItemBlock_v2)) != sizeof(_NZ_SNewsItemBlock_v2))
		Fail(errorType_Misc, error_Corrupt);

	// delete all child's
	if(inDeleteChildren)
		_NZ_DelArticSibsChilds_v2(mFile, blockSize, head->allocTable, idHash, hashMask, FB(itm.body.firstChildOffset), articCount);
		
	// remove flavors
	_NZ_DelFlavs_v2(mFile, blockSize, head->allocTable, articOff);

	// remove this id from the table
	Uint32 id = FB(itm.body.id);
	_NZ_RmIDFromHash_v2(mFile, blockSize, idHash[id & hashMask], id, head->allocTable);
	articCount--;

	if(inDeleteChildren || !itm.body.firstChildOffset)
	{
		// take care of siblings
		if(itm.body.prevArticleOffset)
		{
			// write next to the prev's next
			mFile->Write(_NZ_GetByteOffset_v2(FB(itm.body.prevArticleOffset), blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.nextArticleOffset), &itm.body.nextArticleOffset, sizeof(Uint32));
		}
		else
		{
			// must update parent's firstChildOffset
				// if no parent, then I'm the first artic, and must update head.
			
			if(itm.body.parentOffset)
				mFile->Write(_NZ_GetByteOffset_v2(FB(itm.body.parentOffset), blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.firstChildOffset), &itm.body.nextArticleOffset, sizeof(Uint32));
			else
				head->firstArticleOffset = itm.body.nextArticleOffset;
		}

		if(itm.body.nextArticleOffset)
		{
			// write prev to the next's prev
			mFile->Write(_NZ_GetByteOffset_v2(FB(itm.body.nextArticleOffset), blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.prevArticleOffset), &itm.body.prevArticleOffset, sizeof(Uint32));
		}
		else
		{
			// may need to udpate head's last artic (if no parent)
			if(!itm.body.parentOffset)
				head->lastArticleOffset = itm.body.prevArticleOffset;
		}				
	}
	else	// replace this article with his first child
	{
		// read first child
		_NZ_SNewsItemBlock_v2 firstChild;
		if(mFile->Read(_NZ_GetByteOffset_v2(FB(itm.body.firstChildOffset), blockSize), &firstChild, sizeof(_NZ_SNewsItemBlock_v2)) != sizeof(_NZ_SNewsItemBlock_v2))
			Fail(errorType_Misc, error_Corrupt);

		// take care of siblings
		if(itm.body.prevArticleOffset)
		{
			// write first child to the prev's next
			mFile->Write(_NZ_GetByteOffset_v2(FB(itm.body.prevArticleOffset), blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.nextArticleOffset), &itm.body.firstChildOffset, sizeof(Uint32));
		}
		else
		{
			// must update parent's firstChildOffset
				// if no parent, then I'm the first artic, and must update head.
			
			if(itm.body.parentOffset)
				mFile->Write(_NZ_GetByteOffset_v2(FB(itm.body.parentOffset), blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.firstChildOffset), &itm.body.firstChildOffset, sizeof(Uint32));
			else
				head->firstArticleOffset = itm.body.firstChildOffset;
		}

		if(itm.body.nextArticleOffset)
		{
			// write first child to the next's prev
			mFile->Write(_NZ_GetByteOffset_v2(FB(itm.body.nextArticleOffset), blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.prevArticleOffset), &itm.body.firstChildOffset, sizeof(Uint32));
		}
		else
		{
			// may need to udpate head's last artic (if no parent)
			if(!itm.body.parentOffset)
				head->lastArticleOffset = itm.body.firstChildOffset;
		}				

		// replace parent offset for siblings
		_NZ_ChangeParentOffset_v2(mFile, blockSize, FB(firstChild.body.nextArticleOffset), itm.body.firstChildOffset);

		if (firstChild.body.firstChildOffset)
		{
			if (firstChild.body.nextArticleOffset)
			{
				// return offset for last sibling
				Uint32 lastOffset = _NZ_GetLastOffset_v2(mFile, blockSize, FB(firstChild.body.firstChildOffset));
				mFile->Write(_NZ_GetByteOffset_v2(lastOffset, blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.nextArticleOffset), &firstChild.body.nextArticleOffset, sizeof(Uint32));

				lastOffset = FB(lastOffset);
				mFile->Write(_NZ_GetByteOffset_v2(FB(firstChild.body.nextArticleOffset), blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.prevArticleOffset), &lastOffset, sizeof(Uint32));
			}	
		}
		else
		{
			firstChild.body.firstChildOffset = firstChild.body.nextArticleOffset;
		
			if (firstChild.body.firstChildOffset)
			{
				Uint32 nullOffset = 0;
				mFile->Write(_NZ_GetByteOffset_v2(FB(firstChild.body.firstChildOffset), blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.prevArticleOffset), &nullOffset, sizeof(Uint32));
			}
		}

		firstChild.body.prevArticleOffset = itm.body.prevArticleOffset;
		firstChild.body.nextArticleOffset = itm.body.nextArticleOffset;
		firstChild.body.parentOffset = itm.body.parentOffset;

		// write in file first child
		mFile->Write(_NZ_GetByteOffset_v2(FB(itm.body.firstChildOffset), blockSize), &firstChild, sizeof(_NZ_SNewsItemBlock_v2));
	}
		
	// write the hash table back to disk
	mFile->Write(hashFileOffset, BPTR(idHashBuf), hashSize);

	// delete cached article list
	if(head->listOffset)
	{
		_NZ_DeallocateBlocks_v2(mFile, blockSize, head->allocTable, TB(head->listOffset), TB(head->listSize) / blockSize + 1);

		head->listOffset = 0;
		head->listSize = 0;
	}

	head->articleCount = TB(articCount);
	// write the header back to disk
	mFile->Write(0, buf, sizeof(_NZ_SDBHeader_v2));

	return true;
}

bool CNewsDatabase_v2::DeleteArticleData(Uint32 inArticleID, const Uint8 *inName)
{
	if (UMemory::Compare(inName + 1, inName[0], ".body", 5))
		return false;

	StFileOpener open(mFile);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);
	
	Uint32 hashMask = FB(head->offSetIDHashMask);
	Uint32 hashSize = (hashMask + 1) * sizeof(_NZ_SHashInfo_v2);
	Uint32 hashFileOffset = _NZ_GetByteOffset_v2(FB(head->offSetIDHashOffset), blockSize);
	StPtr idHashBuf(hashSize);
	mFile->Read(hashFileOffset, BPTR(idHashBuf), hashSize);
	_NZ_SHashInfo_v2 *idHash = (_NZ_SHashInfo_v2 *)BPTR(idHashBuf);
	
	// find it in the hash table
	Uint32 articOff = _NZ_IDToOffset_v2(mFile, blockSize, inArticleID, idHash[inArticleID & hashMask]);
	
	if(!articOff)
		Fail(errorType_Misc, error_NoSuchItem);

	// now we can delete
	Int8 *pType = "text/plain";
	Uint32 typeLen = strlen(pType);
	Uint32 readSize = min(blockSize, (Uint32)sizeof(buf), (Uint32)(sizeof(_NZ_SGenericBlock_v2) + 1 + typeLen));
	
	// search
	Uint8 data[256];
	_NZ_SGenericBlock_v2 *blk = (_NZ_SGenericBlock_v2 *)data;

	if(mFile->Read(_NZ_GetByteOffset_v2(articOff, blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, nextFlav), &blk->nextFlav, sizeof(blk->nextFlav)) != sizeof(blk->nextFlav))
		Fail(errorType_Misc, error_Corrupt);

	Uint32 currentFlav;
	Uint32 prevFlav = articOff;
	Uint32 maxFlavs = 30;
	
	// loop thru flavs
	while(maxFlavs-- && blk->nextFlav)
	{
		currentFlav = FB(blk->nextFlav);
		
		if(mFile->Read(_NZ_GetByteOffset_v2(currentFlav, blockSize), data, readSize) != readSize)
			Fail(errorType_Misc, error_Corrupt);
		
		if(blk->type[0] == typeLen && UMemory::Equal(blk->type + 1, pType, typeLen))
		{
			mFile->Write(_NZ_GetByteOffset_v2(prevFlav, blockSize) + OFFSET_OF(_NZ_SGenericBlock_v2, nextFlav),  &blk->nextFlav, sizeof(Uint32));
			_NZ_DeallocateBlocks_v2(mFile, blockSize, head->allocTable, currentFlav, (FB(blk->size) + blockSize - 1) / blockSize);
			return true;
		}
		
		prevFlav = FB(blk->nextFlav);
	}
	
	return false;
}

// 1.7.2 servers don't generate extern ID so we must generate ID after the article was created
Uint32 CNewsDatabase_v2::GenerateExternID(Uint32 inArticleID, void *outExternID, Uint32 inMaxSize)
{
	StFileOpener open(mFile);
	
	Uint8 buf[512];
	_NZ_SDBHeader_v2 *head = (_NZ_SDBHeader_v2 *)buf;
	
	if(mFile->Read(0, buf, sizeof(_NZ_SDBHeader_v2)) != sizeof(_NZ_SDBHeader_v2))
		Fail(errorType_Misc, error_Corrupt);
	
	if(head->type != TB('HLNZ'))
		Fail(errorType_Misc, error_FormatUnknown);
	
	if(head->vers != TB((Uint32)2))
		Fail(errorType_Misc, error_VersionUnknown);
	
	Uint32 blockSize = FB(head->blockSize);
	
	Uint32 hashMask = FB(head->offSetIDHashMask);
	Uint32 hashSize = (hashMask + 1) * sizeof(_NZ_SHashInfo_v2);
	Uint32 hashFileOffset = _NZ_GetByteOffset_v2(FB(head->offSetIDHashOffset), blockSize);
	StPtr idHashBuf(hashSize);
	mFile->Read(hashFileOffset, BPTR(idHashBuf), hashSize);
	_NZ_SHashInfo_v2 *idHash = (_NZ_SHashInfo_v2 *)BPTR(idHashBuf);
	
	// find it in the hash table
	Uint32 articOff = _NZ_IDToOffset_v2(mFile, blockSize, inArticleID, idHash[inArticleID & hashMask]);
	
	if(!articOff)
		Fail(errorType_Misc, error_NoSuchItem);
	
	Uint8 psExternID[64];
	psExternID[0] = _NZ_GenerateExternID(psExternID + 1, sizeof(psExternID) - 1);

	// write extern ID in file
	mFile->Write(_NZ_GetByteOffset_v2(articOff, blockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.externId),  psExternID, psExternID[0] + 1);

	return UMemory::Copy(outExternID, psExternID + 1, psExternID[0] > inMaxSize ? inMaxSize : psExternID[0]);
}

#pragma mark -

// inFirstArtic is in net byte order
// allocates inBlockCount blocks into the allocTable on inFile.  Writes the changes to inFile and updates ioAllocTable.
// ioAllocTable's members are in network byte order  (Huh..Huh...you said "member")
// inFile is assumed to be open writable
// returns the offset of the block allocated.
// ioAllocTable may be modified - either just the firstFree or if there is no room in the allocTable, it might be grown and reallocated.  (reallocate the alloc table!)
Uint32 _NZ_AllocateBlocks_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SAllocTableInfo_v2 &ioAllocTable, Uint32 inBlockCount)
{
	Uint8 buf[2048];	// this will be nice cached.
	Uint32 b;	
	// if inBlockCount = 1, we don't have to jump through too many hoops.  the firstFree is the one we allocate, and then set it to the next free
		// how big a block do we read 5
	if(!ioAllocTable.firstFree)
	{
allocNewBlock:
		// nothing free!  need to allocate alloctable.
		// there's room at block (size * inBlockSize * 8) + 1 to allocate the alloc table.  Write it to the file 
		Uint32 s = FB(ioAllocTable.size);
		Uint32 oldAllocSize = s * inBlockSize;
		Uint32 newAllocSize = oldAllocSize + inBlockSize;
		Uint32 oldOffset = FB(ioAllocTable.offset);
		Uint32 newOffset = s * inBlockSize * 8 + 1;
		StPtr allocTab(newAllocSize);
		inFile->Read(_NZ_GetByteOffset_v2(oldOffset, inBlockSize), BPTR(allocTab), oldAllocSize);
		
		UMemory::Clear(BPTR(allocTab) + oldAllocSize, newAllocSize - oldAllocSize);

		//in the new data, allocate the new alloc table. deallocate the old.
		Uint32 n = s;
		oldOffset--;
		Uint32 tmpNewOff = newOffset - 1;
		while(n--)
		{
			ClearBit(BPTR(allocTab), oldOffset++);
			SetBit(BPTR(allocTab), tmpNewOff++);
		}
		
		SetBit(BPTR(allocTab), tmpNewOff++);
		
		inFile->Write(_NZ_GetByteOffset_v2(newOffset, inBlockSize), BPTR(allocTab), newAllocSize);
		
		// update header's info
		ioAllocTable.offset = TB(newOffset);
		ioAllocTable.size = TB(s + 1);
		
		if(FB(ioAllocTable.firstFree) > (oldOffset + 7) / 8)
			ioAllocTable.firstFree = TB((oldOffset + 7) / 8);

		inFile->Write(OFFSET_OF(_NZ_SDBHeader_v2, allocTable), &ioAllocTable, sizeof(_NZ_SAllocTableInfo_v2));
	
		// check the old location of the alloc table to see if it will hold what we wish to allocate.
			// else, set the next block to the next item in the alloc table.  ie, s bits into the table
			
	}
	
	
	Uint32 firstFree = RoundDown4(FB(ioAllocTable.firstFree) - 1);
	Uint32 allocOffset = _NZ_GetByteOffset_v2(FB(ioAllocTable.offset), inBlockSize);
	
tryNextRead:
	
	Uint32 size = min((Uint32)sizeof(buf), FB(ioAllocTable.size) * inBlockSize - firstFree);
	
	inFile->Read(allocOffset + firstFree, buf, size);
	
	Uint32 *p = (Uint32 *)buf;
	Int32 fourByteCount = size / sizeof(Uint32);
	Uint32 bit;
	
keepSearching:	
	while(fourByteCount--)
	{
		if(*p != 0xFFFFFFFF)
		{
			// now search the bits for a free one.
			for(bit = 0; bit != 32; bit++)
			{
				if(!GetBit(p, bit))
				{
					// if we haven't read in enough data to check the full range we need
					if((fourByteCount + 1) * 4 * 8 - bit < inBlockCount)
					{
						// update vars and read in the next chunk
							// make sure this block is re-read
						// but first, check to see that additional space is present.
							// if not, allocate new space

						goto outOfBlocks;
					}
					
					
					// check the next x bits
					for(b = 1; b != inBlockCount; b++)
					{
						if(GetBit(p, bit + b))
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

	if(sizeof(buf) > FB(ioAllocTable.size) * inBlockSize - firstFree + (inBlockCount + 7) / 8)
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
	
	while(inBlockCount--)
		SetBit(p, b++);
		
	inFile->Write(allocOffset + firstFree + (BPTR(p) - buf), BPTR(p), (b + 7) / 8);
	
	return (firstFree + (BPTR(p) - buf)) * 8 + bit + 1;
	


	
	
	// what I want is to find the first open space. 
		// when I find a free bit beginning in this space, loop inBlockCount times and if I hit a bit, jump forward the number of bits I looped until my start is in the next block
		// if we hit this stage, go back to reading in Uint32s above.
			// once the Uint32s run out, we read in another chunk of the allocTable and repeat.
				// if this all runs out, we need to reallocate the alloctable.  Once this is done we can just go ahead and allocate becase we know that the newly-allocated block has heaps of space.
		
		
	// if we do not have space, we must allocate an additional block for the alloc table.  
}

// inFile is assumed to be open writable
// ioAllocTable is the alloc table info whose members are in network byte order
// inOffset is the offset of the first block to deallocate
// inBlockCount is the number of blocks to deallocate
void _NZ_DeallocateBlocks_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SAllocTableInfo_v2 &ioAllocTable, Uint32 inOffset, Uint32 inBlockCount)
{
	Uint8 buf[1024];

	Uint32 fileOff = _NZ_GetByteOffset_v2(FB(ioAllocTable.offset), inBlockSize);
	
	Uint32 startByte = (inOffset - 1) / 8 + fileOff;
	Uint32 size = (inOffset + inBlockCount + 8 - 1) / 8 + fileOff - startByte;
	
	if(size > sizeof(buf))
		Fail(errorType_Misc, error_OutOfRange);
		
	inFile->Read(startByte, buf, size);

#if DEBUG
	Uint32 count = inBlockCount;
	Uint32 start = (inOffset - 1) & 7;
	while(count--)
		if(!GetBit(buf, start++))
			DebugBreak("DeallocateBlocks: block not allocated!");
#endif

	
	Uint32 startBit = (inOffset - 1) & 7;
	while(inBlockCount--)
		ClearBit(buf, startBit++);
	  
	inFile->Write(startByte, buf, size);
	
	if(FB(ioAllocTable.firstFree) > inOffset)
	{
		ioAllocTable.firstFree = TB(inOffset);
		// write this to disk.
		inFile->Write(OFFSET_OF(_NZ_SDBHeader_v2, allocTable), &ioAllocTable, sizeof(_NZ_SAllocTableInfo_v2));
	}

	// this func should also check for the last allocated block and SetSize() of this file to only include the last one

}

// inFile open
// inHashInfo's members in NetByteOrder
Uint32 _NZ_IDToOffset_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inID, const _NZ_SHashInfo_v2 &inHashInfo)
{
	Uint32 recordCount = FB(inHashInfo.size);
	Uint32 recordOffset = FB(inHashInfo.offset);

	if(!recordCount || !recordOffset)
		return nil;
		
	Uint32 dataSize = recordCount * sizeof(_NZ_SIDOffset_v2);
	StPtr data(dataSize);
	
	if(inFile->Read(_NZ_GetByteOffset_v2(recordOffset, inBlockSize), BPTR(data), dataSize) != dataSize)
		Fail(errorType_Misc, error_OutOfRange);
	
	_NZ_SIDOffset_v2 *offID = (_NZ_SIDOffset_v2 *)BPTR(data);
	
	// do binary search on this item.
	
	Uint32 first = 0;
	Uint32 last = recordCount - 1;
	Uint32 middle;
	
	while(first != last)
	{
		middle = (first + last)/2;
	
		if(FB(offID[middle].id) < inID)
		 	first = middle + 1;
		 else
		 	last = middle;
	}
	
	if(FB(offID[first].id) == inID)
		return FB(offID[first].offset);
	
	return 0;
}



Uint32 _NZ_OffsetToID_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inOffset)
{
	Uint32 id;
	if(inOffset)
	{
		if(inFile->Read(_NZ_GetByteOffset_v2(inOffset, inBlockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.id), &id, sizeof(Uint32)) != sizeof(Uint32))
			return 0;
	
		return FB(id);
	}

	return 0;
}


// ioHashInfo is in NetByteOrder
// generally ioHashInfo's size is modified by +8. as well, this is updated in inFile.
// inByteHashInfoOff is the offset in bytes into the file of this ioHashInfo record (in case we need to modify, which we will likely do)
// ioAllocTable in net byte order
	// only used if the ID/hash stuff need to be moved around because of a full block
		// in which case _NZ_DeallocateBlocks_v2 and _NZ_AllocateBlocks_v2 would be called.
void _NZ_AddIDToHash_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SHashInfo_v2 &ioHashInfo, Uint32 inByteHashInfoOff, Uint32 inID, Uint32 inOffset, _NZ_SAllocTableInfo_v2 &ioAllocTable)
{
	Uint32 recordCount = FB(ioHashInfo.size);
	Uint32 recordOffset = FB(ioHashInfo.offset);
	if(!((recordCount * sizeof(_NZ_SIDOffset_v2)) & (inBlockSize - 1)))
	{
		// we've filled up all the space in this id/offset block...
		// allocate a new block, copy over the old info, deallocate the old block, and update the file with the new offset to that block
		
		Uint32 oldBlockSize = recordCount * sizeof(_NZ_SIDOffset_v2);		
		Uint32 newBlock = _NZ_AllocateBlocks_v2(inFile, inBlockSize, ioAllocTable, oldBlockSize / inBlockSize + 1);
		
		if(!newBlock)
			Fail(errorType_Misc, error_Unknown);
		
		// copy over the data to the new block if there was an old block
		StPtr data(oldBlockSize + inBlockSize);
		if(oldBlockSize)
		{
			inFile->Read(_NZ_GetByteOffset_v2(recordOffset, inBlockSize), BPTR(data), oldBlockSize);
			_NZ_DeallocateBlocks_v2(inFile, inBlockSize, ioAllocTable, recordOffset, oldBlockSize / inBlockSize);
		}
		
		UMemory::Clear(BPTR(data) + oldBlockSize, inBlockSize);
		inFile->Write(_NZ_GetByteOffset_v2(newBlock, inBlockSize), BPTR(data), oldBlockSize + inBlockSize);

		//bug - if something below fails, this new offset won't be written
		// essentially a memory leak in the file
		ioHashInfo.offset = TB(newBlock);
		
		recordCount = FB(ioHashInfo.size);
		recordOffset = FB(ioHashInfo.offset);
	}
	
	
	_NZ_SIDOffset_v2 record;
	
	// 99.9% of the time, this ID will be > than the last ID and all we have to do is append it to the end.
	// read in previous record to ensure this is the case and fail if not.
	
	if(recordCount)	// do some error checking to make sure not corrupt
	{
	
		if(inFile->Read(_NZ_GetByteOffset_v2(recordOffset, inBlockSize) + (recordCount - 1) * sizeof(_NZ_SIDOffset_v2), &record, sizeof(record)) != sizeof(record))
			Fail(errorType_Misc, error_Corrupt);
		
		if(FB(record.id) >= inID)
			Fail(errorType_Misc, error_Corrupt);
	}
	record.id = TB(inID);
	record.offset = TB(inOffset);
	
	// write this to the table
	if(inFile->Write(_NZ_GetByteOffset_v2(recordOffset, inBlockSize) + recordCount * sizeof(_NZ_SIDOffset_v2), &record, sizeof(record)) != sizeof(record))
		Fail(errorType_Misc, error_Corrupt);
	
	ioHashInfo.size = TB(recordCount + 1);
	
	inFile->Write(inByteHashInfoOff, &ioHashInfo, sizeof(_NZ_SHashInfo_v2));
}
	

void _NZ_RmIDFromHash_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SHashInfo_v2 &ioHashInfo, Uint32 inID, _NZ_SAllocTableInfo_v2 &ioAllocTable)
{
	Uint32 recordCount = FB(ioHashInfo.size);
	Uint32 recordOffset = FB(ioHashInfo.offset);

	if(!recordCount || !recordOffset)
		return;
		
	Uint32 dataSize = recordCount * sizeof(_NZ_SIDOffset_v2);
	StPtr data(dataSize);
	
	Uint32 hashTableOffset = _NZ_GetByteOffset_v2(recordOffset, inBlockSize);
	if(inFile->Read(hashTableOffset, BPTR(data), dataSize) != dataSize)
		Fail(errorType_Misc, error_OutOfRange);
	
	_NZ_SIDOffset_v2 *offID = (_NZ_SIDOffset_v2 *)BPTR(data);
	
	// do binary search on this item.
	
	Uint32 first = 0;
	Uint32 last = recordCount - 1;
	Uint32 middle;
	
	while(first != last)
	{
		middle = (first + last)/2;
	
		if(FB(offID[middle].id) < inID)
		 	first = middle + 1;
		 else
		 	last = middle;
	}

	if(FB(offID[first].id) == inID)
	{
		// we've found the record
		// shift over all following records back one
		
		for(last = last + 1; last != recordCount; last++)
			offID[last - 1] = offID[last];
		
		// decrease size by 1
		// if the size makes an extra block free, deallocate it
		
		Uint32 oldLastBlock =  (dataSize + inBlockSize - 1) / inBlockSize;
		ioHashInfo.size = TB(--recordCount);
		if((recordCount * sizeof(_NZ_SIDOffset_v2) + inBlockSize - 1) / inBlockSize != oldLastBlock)
		{
			// if we need to deallocate a block
			_NZ_DeallocateBlocks_v2(inFile, inBlockSize, ioAllocTable, recordOffset + oldLastBlock - 1, 1);
			// if we have 0 blocks, set the head of this to 0
			if(recordCount == 0)
				ioHashInfo.offset = 0;

		}
	}
	
	// write the updated hash table (only the parts needed)
	inFile->Write(hashTableOffset + first * sizeof(_NZ_SIDOffset_v2), BPTR(data) + first * sizeof(_NZ_SIDOffset_v2), (recordCount - first) * sizeof(_NZ_SIDOffset_v2));
}


// returns offset of last item
// offsetOffset is the offset into the struct of the offset we read in to find the next block.
Uint32 _NZ_GetLast_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inFirstOffset, Uint32 inOffsetOffset)
{
	// this could have better error checking

	Uint32 itm = TB(inFirstOffset);
	
	
	Uint32 last = itm;
	
	Uint32 max = 100;	// in case corrupt and it's endless
	
	while(itm && max--)
	{
		last = itm;
	
		if(inFile->Read(_NZ_GetByteOffset_v2(FB(itm), inBlockSize) + inOffsetOffset, &itm, sizeof(itm)) != sizeof(itm))
			Fail(errorType_Misc, error_Corrupt);
	}

	return FB(last);
}

void _NZ_AddToList_v2(CNZArticleList_v2 *inList, TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inFirstArtic, Uint32 &ioArticCount, bool inNewFormat)
{
	union
	{
		Uint32 buf[256];
		_NZ_SNewsItemBlock_v2 blk;
		_NZ_SGenericBlock_v2 flav;
	};
	
	//loop until there is no other artic.
	for(;;)
	{
		if(!inFirstArtic || !ioArticCount)
			break;
		
		ioArticCount--;
		
		if(inFile->Read(_NZ_GetByteOffset_v2(FB(inFirstArtic), inBlockSize), &blk, sizeof(blk)) != sizeof(blk))
			Fail(errorType_Misc, error_Corrupt);
		
		// add this article
		if (inNewFormat)
			inList->AddNewArticle(blk, inFirstArtic);
		else
			inList->AddArticle(blk, inFirstArtic);
		
		// set up the next block - store these now because reading in the flavs will overwrite this.
		inFirstArtic = blk.body.nextArticleOffset;
		Uint32 firstChildOff = blk.body.firstChildOffset;

		// add any flavors it may have
		Uint32 maxFlavs = 32;
		while(maxFlavs-- && flav.nextFlav)
		{
			if(inFile->Read(_NZ_GetByteOffset_v2(FB(flav.nextFlav), inBlockSize), &flav, inBlockSize) == 0)
				break;	// if we can't find this flav, let's just show the rest of the articles...
			
			// add this flavor
			if (inNewFormat)
				inList->AddNewArticleFlavor(flav);
			else
				inList->AddArticleFlavor(flav);
		}

		// if it has children, call this func on them.
		if(firstChildOff)
			_NZ_AddToList_v2(inList, inFile, inBlockSize, firstChildOff, ioArticCount, inNewFormat);
	}
}

// this needs a max itteration count - in case the database is corrupt
void _NZ_DelArticSibsChilds_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SAllocTableInfo_v2 &ioAllocTable, _NZ_SHashInfo_v2 *ioHashInfo, Uint32 inHashMask, Uint32 inArticOff, Uint32 &ioCount)
{
	if(!inArticOff)
		return;
		
	_NZ_SNewsItemBlock_v2 itm;
	
	if(inFile->Read(_NZ_GetByteOffset_v2(inArticOff, inBlockSize), &itm, sizeof(_NZ_SNewsItemBlock_v2)) != sizeof(_NZ_SNewsItemBlock_v2))
		Fail(errorType_Misc, error_Corrupt);
		
	// delete all my children
	_NZ_DelArticSibsChilds_v2(inFile, inBlockSize, ioAllocTable, ioHashInfo, inHashMask, FB(itm.body.firstChildOffset), ioCount);

	// delete all my siblings
	_NZ_DelArticSibsChilds_v2(inFile, inBlockSize, ioAllocTable, ioHashInfo, inHashMask, FB(itm.body.nextArticleOffset), ioCount);
	
	// deallocate all flavors
	_NZ_DelFlavs_v2(inFile, inBlockSize, ioAllocTable, inArticOff);

	
	// dereference me from offset/ID table
	Uint32 id = _NZ_OffsetToID_v2(inFile, inBlockSize, inArticOff);
	_NZ_RmIDFromHash_v2(inFile, inBlockSize, ioHashInfo[id & inHashMask], id, ioAllocTable);

	ioCount--;	
}

void _NZ_DelFlavs_v2(TFSRefObj* inFile, Uint32 inBlockSize, _NZ_SAllocTableInfo_v2 &ioAllocTable, Uint32 inFirstOff)
{
	_NZ_SGenericBlock_v2 blk;
	
	Uint32 maxFlavs = 64;	// don't wanna loop forever if corrupt
	
	while(inFirstOff && maxFlavs--)
	{
		if(inFile->Read(_NZ_GetByteOffset_v2(inFirstOff, inBlockSize), &blk, sizeof(_NZ_SGenericBlock_v2)) != sizeof(_NZ_SGenericBlock_v2))
			Fail(errorType_Misc, error_Corrupt);
		
		_NZ_DeallocateBlocks_v2(inFile, inBlockSize, ioAllocTable, inFirstOff, (FB(blk.size) + inBlockSize - 1) / inBlockSize);
		
		inFirstOff = FB(blk.nextFlav);
	}
}

void _NZ_ChangeParentOffset_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inArticOff, Uint32 inParentOff)
{
	if(!inArticOff)
		return;

	inFile->Write(_NZ_GetByteOffset_v2(inArticOff, inBlockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.parentOffset), &inParentOff, sizeof(Uint32));

	Uint32 nextOffset;
	if(inFile->Read(_NZ_GetByteOffset_v2(inArticOff, inBlockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.nextArticleOffset), &nextOffset, sizeof(Uint32)) != sizeof(Uint32))
		Fail(errorType_Misc, error_Corrupt);

	_NZ_ChangeParentOffset_v2(inFile, inBlockSize, FB(nextOffset), inParentOff);
}

Uint32 _NZ_GetLastOffset_v2(TFSRefObj* inFile, Uint32 inBlockSize, Uint32 inArticOff)
{
	if(!inArticOff)
		return 0;

	Uint32 nextOffset;
	if(inFile->Read(_NZ_GetByteOffset_v2(inArticOff, inBlockSize) + OFFSET_OF(_NZ_SNewsItemBlock_v2, body.nextArticleOffset), &nextOffset, sizeof(Uint32)) != sizeof(Uint32))
		Fail(errorType_Misc, error_Corrupt);

	if(!nextOffset)
		return inArticOff;
	
	return _NZ_GetLastOffset_v2(inFile, inBlockSize, FB(nextOffset));
}

#pragma mark -

// for flavor listing
CNZArticleList_v2::CNZArticleList_v2()
{
	mDataSize = 50;
	mData = UMemory::New(mDataSize);
	mOutSize = 0;
	
	mPtr = BPTR(mData);
	
	mArticleCountPtr = ((Uint32 *)mPtr)++;
	*mArticleCountPtr = 0;		// skip over the count for now (this will be set later)

	mCurrentFlavCountPtr = ((Uint16 *)mPtr)++;	// flav count
	*mCurrentFlavCountPtr = 0;
		
	mCount = 1;
	
	mIDOffCount = 0;
	mIDOffTabSize = 0;
	mIDOffTable = nil;
}

// for article listing
CNZArticleList_v2::CNZArticleList_v2(Uint32 inCount, bool inNewFormat)
{
	mDataSize = 3 * sizeof(Uint32) + inCount * 50;
	mData = UMemory::New(mDataSize);
	mCurrentFlavCountPtr = nil;
	mOutSize = 0;
	
	mPtr = BPTR(mData);
	
	if (!inNewFormat)
		*((Uint32*)mPtr)++ = 0;	// don't need the id, but add the field for backward compat.
	
	mArticleCountPtr = ((Uint32 *)mPtr)++;
	*mArticleCountPtr = 0;		// skip over the count for now (this will be set later)
	
	if (!inNewFormat)
	{
		*mPtr++ = 0;			// don't need name
		*mPtr++ = 0;			// don't need desc
	}
	
	// don't use inCount as the count since we want to make sure this is accurate
	mCount = 0;
	
	mIDOffCount = 0;
	mIDOffTabSize = 0;
	mIDOffTable = nil;
}

CNZArticleList_v2::~CNZArticleList_v2()
{
	if(mData)
		UMemory::Dispose(mData);
	
	if(mIDOffTable)
		delete mIDOffTable;
}

// assume inArticBlock is in network byte order
void CNZArticleList_v2::AddArticle(const _NZ_SNewsItemBlock_v2 &inArticBlock, Uint32 inArticOffset)
{
	Uint32 titleSize = min((Uint32)inArticBlock.body.title[0], (Uint32)63);
	Uint32 posterSize = min((Uint32)inArticBlock.body.poster[0], (Uint32)31);
	
	Uint32 outSize =
		sizeof(Uint32) +			// ID
		sizeof(Uint32) +			// ParentID
		sizeof(SDateTimeStamp) +	// date
		sizeof(Uint32) +			// flags
		1 + titleSize +				// title
		1 + posterSize +			// poster
		sizeof(Uint16);				// flavor count
	
	if(mDataSize - (mPtr - BPTR(mData)) < outSize)	// if we don't have enough room
		Reallocate(outSize);
	
	*((Uint32*)mPtr)++ = inArticBlock.body.id;								// id
	*((SDateTimeStamp*)mPtr)++ = inArticBlock.body.date;					// date
	*((Uint32*)mPtr)++ = LookupID(inArticBlock.body.parentOffset);			// parent id
	*((Uint32*)mPtr)++ = inArticBlock.body.flags;							// flags

	mCurrentFlavCountPtr = ((Uint16 *)mPtr)++;								// flav count
	*mCurrentFlavCountPtr = 0;

	*mPtr++ = titleSize;
	mPtr += UMemory::Copy(mPtr, inArticBlock.body.title + 1, titleSize);	// title
	*mPtr++ = posterSize;
	mPtr += UMemory::Copy(mPtr, inArticBlock.body.poster + 1, posterSize);	// poster

	// don't bother adding if it has no children since we won't ever have to look it up
	if(inArticBlock.body.firstChildOffset)
		AddID(inArticBlock.body.id, inArticOffset);
		
	mCount++;
}

// assume inArticBlock is in network byte order
void CNZArticleList_v2::AddNewArticle(const _NZ_SNewsItemBlock_v2 &inArticBlock, Uint32 inArticOffset)
{
	Uint32 titleSize = min((Uint32)inArticBlock.body.title[0], (Uint32)63);
	Uint32 posterSize = min((Uint32)inArticBlock.body.poster[0], (Uint32)31);
	
	Uint32 outSize =
		sizeof(Uint32) +			// ID
		sizeof(Uint32) +			// ParentID
		sizeof(SDateTimeStamp) +	// date
		sizeof(Uint32) +			// flags
		1 + titleSize +				// title
		1 + posterSize +			// poster
		sizeof(Uint16);				// flavor count
		
	Uint32 externIdSize = min((Uint32)inArticBlock.body.externId[0], (Uint32)63);
	outSize += 1 + externIdSize;
	
	if(mDataSize - (mPtr - BPTR(mData)) < outSize)	// if we don't have enough room
		Reallocate(outSize);
	
	*((Uint32*)mPtr)++ = inArticBlock.body.id;								// id
	*((SDateTimeStamp*)mPtr)++ = inArticBlock.body.date;					// date
	*((Uint32*)mPtr)++ = LookupID(inArticBlock.body.parentOffset);			// parent id
	*((Uint32*)mPtr)++ = inArticBlock.body.flags;							// flags

	mCurrentFlavCountPtr = ((Uint16 *)mPtr)++;								// flav count
	*mCurrentFlavCountPtr = 0;

	*mPtr++ = titleSize;
	mPtr += UMemory::Copy(mPtr, inArticBlock.body.title + 1, titleSize);	// title
	*mPtr++ = posterSize;
	mPtr += UMemory::Copy(mPtr, inArticBlock.body.poster + 1, posterSize);	// poster
	*mPtr++ = externIdSize;
	mPtr += UMemory::Copy(mPtr, inArticBlock.body.externId + 1, externIdSize);	// extern ID

	// don't bother adding if it has no children since we won't ever have to look it up
	if(inArticBlock.body.firstChildOffset)
		AddID(inArticBlock.body.id, inArticOffset);
		
	mCount++;
}

void CNZArticleList_v2::AddArticleFlavor(const _NZ_SGenericBlock_v2 &inFlavBlock)
{
	if((mPtr - BPTR(mData)) > mDataSize)
		DebugBreak("ptr advanced too far!");
		
	Require(mCurrentFlavCountPtr);
	
	Uint32 flavSize = min((Uint32)inFlavBlock.type[0], (Uint32)63);
	
	Uint32 s =
		1 + flavSize +			// flavor
		sizeof(Uint16); 		// size
	
	if(mDataSize - (mPtr - BPTR(mData)) < s)
		Reallocate(s);

	// write the type
	*mPtr++ = flavSize;
	mPtr+= UMemory::Copy(mPtr, inFlavBlock.type + 1, flavSize);
	
	// write the size of the data
	*((Uint16*)mPtr)++ = TB((Uint16)(FB(inFlavBlock.size)));
	
	*mCurrentFlavCountPtr = TB((Uint16)(FB(*mCurrentFlavCountPtr) + 1));
}

void CNZArticleList_v2::AddNewArticleFlavor(const _NZ_SGenericBlock_v2 &inFlavBlock)
{
	if((mPtr - BPTR(mData)) > mDataSize)
		DebugBreak("ptr advanced too far!");
		
	Require(mCurrentFlavCountPtr);
	
	Uint8 name[8];
	name[0] = UMemory::Copy(name + 1, ".body", 5);
	
	Uint32 nameSize = min((Uint32)name[0], (Uint32)63);					// max 63 chars for name
	Uint32 flavSize = min((Uint32)inFlavBlock.type[0], (Uint32)31);		// max 32 chars for type
	
	Uint32 s =
		1 + nameSize + 			// name
		1 + flavSize +			// flavor
		sizeof(Uint16); 		// size
	
	if(mDataSize - (mPtr - BPTR(mData)) < s)
		Reallocate(s);

	// write the name
	*mPtr++ = nameSize;
	mPtr+= UMemory::Copy(mPtr, name + 1, nameSize);

	// write the type
	*mPtr++ = flavSize;
	mPtr+= UMemory::Copy(mPtr, inFlavBlock.type + 1, flavSize);
	
	// write the size of the data
	*((Uint16*)mPtr)++ = TB((Uint16)(FB(inFlavBlock.size)));
	
	*mCurrentFlavCountPtr = TB((Uint16)(FB(*mCurrentFlavCountPtr) + 1));
}

TPtr CNZArticleList_v2::DetatchDataPtr(Uint32 &outSize)
{
	if(mData)
	{
		// write the count
		*mArticleCountPtr = TB(mCount);

		// "truncate" the data to the size we need
		Uint32 s = mPtr - BPTR(mData);
		TPtr outData = UMemory::Reallocate(mData, s);
		
		// clear all our internal members
		mDataSize = nil;
		mData = nil;

		mArticleCountPtr = nil;
		mCurrentFlavCountPtr = nil;
		
		// return the data
		outSize = s;
		return outData;
	}
	
	return nil;
}

void CNZArticleList_v2::Reallocate(Uint32 inMinSize)
{
	Uint32 s = mPtr - BPTR(mData);
	Uint32 curArticleCountOff = BPTR(mArticleCountPtr) - BPTR(mData);
	Uint32 curFlavCountOff = BPTR(mCurrentFlavCountPtr) - BPTR(mData);
	Uint32 outSize = s + inMinSize + s / 10;	// Increase by size needed + 10%
	
	TPtr outData = UMemory::Reallocate(mData, outSize);
	
	if(!outData)
		Fail(errorType_Memory, memError_NotEnough);
	
	mData = outData;
	mPtr = BPTR(mData) + s;
	
	mCurrentFlavCountPtr = (Uint16 *)(BPTR(mData) + curFlavCountOff);
	mArticleCountPtr = (Uint32 *)(BPTR(mData) + curArticleCountOff);
	
	mDataSize = outSize;
}

Uint32 CNZArticleList_v2::LookupID(Uint32 inOffset)
{
	if(!inOffset || !mIDOffTable)
		return 0;
		
	_NZ_SIDOffset_v2 *itm = (_NZ_SIDOffset_v2*)mIDOffTable;

	// do binary search
	Uint32 first = 0;
	Uint32 last = mIDOffCount - 1;
	Uint32 middle;
	
	while(first != last)
	{
		middle = (first + last)/2;
	
		if(itm[middle].offset < inOffset)
		 	first = middle + 1;
		 else
		 	last = middle;
	}
	
	if(itm[first].offset == inOffset)
		return itm[first].id;
	
	return 0;
}


void CNZArticleList_v2::AddID(Uint32 inID, Uint32 inOffset)
{
	_NZ_SIDOffset_v2 *itm = (_NZ_SIDOffset_v2*)mIDOffTable;

	// do binary search
	Uint32 first = 0;
	if(!mIDOffCount || !mIDOffTable)
		goto allocBlock;

	Uint32 last = mIDOffCount;
	Uint32 middle;
	
	while(first != last)
	{
		middle = (first + last)/2;
	
		if(itm[middle].offset < inOffset)
		 	first = middle + 1;
		 else
		 	last = middle;
	}
	
	// allocate more mem if needed, then copy
	if((mIDOffCount + 1) * sizeof(_NZ_SIDOffset_v2) > mIDOffTabSize)
	{
allocBlock:
		// allocate more mem
		Uint32 s = (mIDOffCount + 10) * sizeof(_NZ_SIDOffset_v2);
		
		TPtr tmpPtr = mIDOffTable;
		mIDOffTable = nil;			// in case this fails, the deconstructor is called
		
		mIDOffTable = UMemory::Reallocate(tmpPtr, s);
		if(!mIDOffTable)
			Fail(errorType_Memory, memError_NotEnough);

		mIDOffTabSize = s;
		itm = (_NZ_SIDOffset_v2*)mIDOffTable;
	}
	
//	if(itm[first].offset == inOffset)
//		return;	// this offset already exists!
	
	_NZ_SIDOffset_v2 *oldPtr = itm + first;
	_NZ_SIDOffset_v2 *newPtr = oldPtr + 1;
	
	Uint32 count = mIDOffCount - first;
	while(count--)
		*newPtr++ = *oldPtr++;
	
	itm[first].id = inID;
	itm[first].offset = inOffset;
	mIDOffCount++;
}
