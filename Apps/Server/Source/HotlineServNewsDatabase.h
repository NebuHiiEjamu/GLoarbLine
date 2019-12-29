/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

// block size must be a power of 2 and greater or equal to 256
// default is 256, although 512, 1024 also make sense for large articles

#pragma options align=packed
struct _NZ_SAllocTableInfo
{
//	Uint32 allocTableChecksum;		// ensure it's not corrupt
	Uint32 offset;
	Uint32 size;		// in blocks
	Uint32 firstFree;	// offset of first byte with free bit - 1 is the first block, 0 indicates none free
	
};

struct _NZ_SDBHeader
{
	// should I store a flag for endian and allow little-endian to store in native format?
	// don't need to store since type would be ZNLH - could use that
	// let's maybe add support in the future...not now.
	
	Uint32 type;	// = HLNZ
	Uint32 vers;	// 2
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

	_NZ_SAllocTableInfo allocTable;
	
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

struct _NZ_SHashInfo
{
	// do we need hashValue? we can find it in the list simply by using offsets.
	// ie, to get to this record we go offSetIDHashOffset + (id & hashMask) * sizeof(_NZ_SHashInfo)
	
	// the offset table is sorted by ID.  Easy since all IDs are incremently added
	Uint32 offset;		// offset into file of _NZ_SIDOffset table
	Uint32 size;		// in _NZ_SIDOffset records of the data
};

struct _NZ_SIDOffset
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

struct _NZ_SGenericBlock
{
	Uint32 nextFlav;
	Uint32 size;
	Uint8 type[];	// pstring

};

const Uint8 _NZ_DataType_Item[] = "\papplication/x-hlnewsitem";
const Uint8 _NZ_DataType_ItemLen = 24;	// the length of the above pstring

struct _NZ_SNewsItemBlock
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
// GetArticleListing is in the form of:
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
		Uint16 flavorCount;		// 2 bytes for compatibilty

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

struct SMyNewsGroupInfo
{
	SGUID id;
	Uint32 articleCount;
	Uint32 addSN;		// incremented by one each time an article is added
	Uint32 deleteSN;	// incremented by one each time an article is deleted
	Uint8 nameSize;
	Uint8 name[31];
	Uint8 descSize;
	Uint8 desc[255];
};

struct SMyNewsArticleInfo
{
	Uint32 id;
	Uint32 prevID;
	Uint32 nextID;
	Uint32 parentID;
	Uint32 firstChildID;
	Uint32 flags;
	SDateTimeStamp dts;
	Uint8 title[64];
	Uint8 poster[32];
	Uint8 externId[64];
};

class UMyNewsDatabase
{
	public:
		// creates a new news database file
		static void CreateNewGroup(TFSRefObj* inRef, Uint32 inID, const Uint8 inName[], const Uint8 inDesc[]);
		
		// returns nil if file invalid
		static TPtr GetArticleListing(TFSRefObj* inFile, Uint32 &outSize, bool inGetExternID = false);
		
		// returns false if corrupt
		static void GetGroupInfo(TFSRefObj* inFile, SMyNewsGroupInfo &outInfo);
		
		// returns nil if no data of this type or of this ID, or file invalid
		static TPtr GetArticleData(TFSRefObj* inFile, Uint32 inArticleID, const Int8 inDataType[], Uint32 &outSize, SMyNewsArticleInfo *outInfo = nil);
		
		// returns 0 if file invalid, or the ID of the article if successful
		static Uint32 AddArticle(TFSRefObj* inFile, Uint32 inParentID, const Uint8 inTitle[], const Uint8 inPoster[], Uint32 inFlags, const SDateTimeStamp *inDTS = nil, const Uint8 *inExternID = nil);
		
		// returns false if file invalid or id invalid
		static bool AddData(TFSRefObj* inFile, Uint32 inArticleID, const Int8 inType[], const Uint8 *inData, Uint32 inDataSize);
		
		static bool DeleteArticle(TFSRefObj* inFile, Uint32 inID, bool inDeleteChildren = false);

		// 1.7.2 servers don't generate extern ID so we must generate ID after the article was created
		static Uint32 GenerateExternID(TFSRefObj* inFile, Uint32 inArticleID, void *outExternID, Uint32 inMaxSize);
};

class CNZArticleList
{
	public:
		CNZArticleList(Uint32 inCount);
		virtual ~CNZArticleList();
		
		void AddArticle(const _NZ_SNewsItemBlock &inArticBlock, Uint32 inArticOffset, bool inGetExternID);
		void AddArticleFlavor(const _NZ_SGenericBlock &inFlavBlock);
		
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
		
		Uint16 *mCurrentFlavCountPtr;
		
		TPtr mIDOffTable;
		Uint32 mIDOffTabSize;
		Uint32 mIDOffCount;

};

