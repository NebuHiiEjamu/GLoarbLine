/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

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

class CNewsDatabase
{
	public:
		// returns an instance of CNewsDatabse from the file ref
		static CNewsDatabase *Construct(TFSRefObj* inFile);
		static CNewsDatabase *Create(TFSRefObj* inRef, const Uint8 inName[], const Uint8 inDesc[], bool inOldVersion = false);

		virtual ~CNewsDatabase();
		
		// returns nil if invalid
		virtual TPtr GetArticleListing(Uint32 &outSize) = 0;
		
		// returns nil if invalid (the list is in old format)
		virtual TPtr GetOldArticleListing(Uint32 &outSize) = 0;

		// returns false if corrupt
		virtual void GetGroupInfo(SMyNewsGroupInfo &outInfo) = 0;
		
		// returns false if no article with this ID, or file invalid
		virtual bool GetArticleInfo(Uint32 inArticleID, SMyNewsArticleInfo& outInfo) = 0;

		// returns nil if invalid
		virtual TPtr GetArticleFlavors(Uint32 inArticleID, Uint32 &outSize) = 0;

		// returns nil if no data of this type or of this ID, or invalid (outType must have 32 chars)
		virtual TPtr GetArticleData(Uint32 inArticleID, const Uint8 *inName, Uint32 &outSize, Int8 outType[] = nil, SMyNewsArticleInfo *outInfo = nil) = 0;
	
		// returns 0 if file invalid, or the ID of the article if successful
		virtual Uint32 AddArticle(Uint32 inParentID, const Uint8 inTitle[], const Uint8 inPoster[], Uint32 inFlags, const SDateTimeStamp *inDTS = nil, const Uint8 *inExternID = nil) = 0;
		
		// update article info
		virtual bool SetArticle(Uint32 inArticleID, const Uint8 inTitle[], const Uint8 inPoster[] = nil, const Uint32 *inFlags = nil, const SDateTimeStamp *inDTS = nil, const Uint8 *inExternID = nil) = 0;
		
		// returns false if file invalid or id invalid
		virtual bool AddArticleData(Uint32 inArticleID, const Uint8 *inName, const Int8 inType[], const Uint8 *inData, Uint32 inDataSize) = 0;
		
		// update article data (if old data don't exists will add new data)
		virtual bool SetArticleData(Uint32 inArticleID, const Uint8 *inName, const Int8 inType[], const Uint8 *inData, Uint32 inDataSize);
		
		// delete article and article data
		virtual bool DeleteArticle(Uint32 inArticleID, bool inDeleteChildren = false) = 0;
		virtual bool DeleteArticleData(Uint32 inArticleID, const Uint8 *inName) = 0;
		
		// 1.7.2 servers don't generate extern ID so we must generate ID after the article was created
		// this function will disappear soon
		virtual Uint32 GenerateExternID(Uint32 inArticleID, void *outExternID, Uint32 inMaxSize) = 0;

		// copy all articles in Dest (can be used for conversions)
		bool CopyCategory(TFSRefObj* inDest);
		bool CopyCategory(CNewsDatabase *inDest);

		// copy article from source to destination, return the ID of the article in destination
		Uint32 CopyArticle(Uint32 inArticleID, CNewsDatabase *inDest, Uint32 inParentID = 0, bool inCopyChildren = false);

		// move article from source to destination, return the ID of the article in destination
		Uint32 MoveArticle(Uint32 inArticleID, CNewsDatabase *inDest, Uint32 inParentID = 0, bool inMoveChildren = false);
};

class StNewsDatabase
{
	public:
		StNewsDatabase(TFSRefObj* inFile)			{	mDatabase = CNewsDatabase::Construct(inFile);	}
		StNewsDatabase(TFSRefObj* inRef, const Uint8 inName[], const Uint8 inDesc[], bool inOldVersion = false)	{	mDatabase = CNewsDatabase::Create(inRef, inName, inDesc, inOldVersion);	}
		~StNewsDatabase()						{	if (mDatabase) delete mDatabase;				}
		operator CNewsDatabase*()				{	return mDatabase;								}
		CNewsDatabase* operator->() const		{	return mDatabase;								}
		bool IsValid()							{	return mDatabase != nil;						}

	private:
		CNewsDatabase *mDatabase;
};

class CNewsDatabase_v2 : public CNewsDatabase
{
	public:
		CNewsDatabase_v2(TFSRefObj* inFile);
		virtual ~CNewsDatabase_v2();

		// creates a new news database file
		void CreateNewGroup(const Uint8 inName[], const Uint8 inDesc[]);

		virtual TPtr GetArticleListing(Uint32 &outSize);
		virtual TPtr GetOldArticleListing(Uint32 &outSize);
		virtual void GetGroupInfo(SMyNewsGroupInfo &outInfo);
		virtual bool GetArticleInfo(Uint32 inArticleID, SMyNewsArticleInfo& outInfo);
		virtual TPtr GetArticleFlavors(Uint32 inArticleID, Uint32 &outSize);
		virtual TPtr GetArticleData(Uint32 inArticleID, const Uint8 *inName, Uint32 &outSize, Int8 outType[] = nil, SMyNewsArticleInfo *outInfo = nil);
		virtual Uint32 AddArticle(Uint32 inParentID, const Uint8 inTitle[], const Uint8 inPoster[], Uint32 inFlags, const SDateTimeStamp *inDTS = nil, const Uint8 *inExternID = nil);
		virtual bool SetArticle(Uint32 inArticleID, const Uint8 inTitle[], const Uint8 inPoster[] = nil, const Uint32 *inFlags = nil, const SDateTimeStamp *inDTS = nil, const Uint8 *inExternID = nil);
		virtual bool AddArticleData(Uint32 inArticleID, const Uint8 *inName, const Int8 inType[], const Uint8 *inData, Uint32 inDataSize);
		virtual bool DeleteArticle(Uint32 inArticleID, bool inDeleteChildren = false);
		virtual bool DeleteArticleData(Uint32 inArticleID, const Uint8 *inName);
		virtual Uint32 GenerateExternID(Uint32 inArticleID, void *outExternID, Uint32 inMaxSize);

	protected:
		TFSRefObj* mFile;
};

class CNewsDatabase_v3 : public CNewsDatabase
{
	public:
		CNewsDatabase_v3(TFSRefObj* inFile);
		virtual ~CNewsDatabase_v3();

		// creates a new news database file
		void CreateNewGroup(const Uint8 inName[], const Uint8 inDesc[]);

		virtual TPtr GetArticleListing(Uint32 &outSize);
		virtual TPtr GetOldArticleListing(Uint32 &outSize);
		virtual void GetGroupInfo(SMyNewsGroupInfo &outInfo);
		virtual bool GetArticleInfo(Uint32 inArticleID, SMyNewsArticleInfo& outInfo);
		virtual TPtr GetArticleFlavors(Uint32 inArticleID, Uint32 &outSize);
		virtual TPtr GetArticleData(Uint32 inArticleID, const Uint8 *inName, Uint32 &outSize, Int8 outType[] = nil, SMyNewsArticleInfo *outInfo = nil);
		virtual Uint32 AddArticle(Uint32 inParentID, const Uint8 inTitle[], const Uint8 inPoster[], Uint32 inFlags, const SDateTimeStamp *inDTS = nil, const Uint8 *inExternID = nil);
		virtual bool SetArticle(Uint32 inArticleID, const Uint8 inTitle[], const Uint8 inPoster[] = nil, const Uint32 *inFlags = nil, const SDateTimeStamp *inDTS = nil, const Uint8 *inExternID = nil);
		virtual bool AddArticleData(Uint32 inArticleID, const Uint8 *inName, const Int8 inType[], const Uint8 *inData, Uint32 inDataSize);
		virtual bool DeleteArticle(Uint32 inID, bool inDeleteChildren = false);
		virtual bool DeleteArticleData(Uint32 inArticleID, const Uint8 *inName);
		
		// nothing to do here
		virtual Uint32 GenerateExternID(Uint32 /*inArticleID*/, void */*outExternID*/, Uint32 /*inMaxSize*/){return 0;};

	protected:
		TFSRefObj* mFile;
};

const Uint8 _NZ_DataType_Item[] = "\papplication/x-hlnewsitem";
const Uint8 _NZ_DataType_ItemLen = 24;	// the length of the above pstring
