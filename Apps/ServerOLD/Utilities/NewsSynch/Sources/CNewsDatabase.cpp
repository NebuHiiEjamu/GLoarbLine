/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CNewsDatabase.h"

bool _NZ_CopyCategory(CNewsDatabase *inSource, CNewsDatabase *inDest);
Uint32 _NZ_CopyArticle(CNewsDatabase *inSource, Uint32 inArticleID, CNewsDatabase *inDest, Uint32 inParentID, bool inCopyChildren, bool inCopyNext = false);
Uint32 _NZ_GenerateExternID(Uint8 *outExternID, Uint32 inMaxSize);


CNewsDatabase *CNewsDatabase::Construct(TFSRefObj* inFile)
{
	ASSERT(inFile);

	bool bIsFolder;
	if (!inFile->Exists(&bIsFolder) || bIsFolder)
		Fail(errorType_FileSys, fsError_NoSuchFile);

	StFileOpener open(inFile, perm_Read);
	
	Uint32 type;
	if(inFile->Read(0, &type, sizeof(Uint32)) != sizeof(Uint32))
		Fail(errorType_Misc, error_Corrupt);
	
	type = TB(type);
	if(type != 'HLNZ')
		Fail(errorType_Misc, error_FormatUnknown);
	
	Uint32 vers;
	if(inFile->Read(4, &vers, sizeof(Uint32)) != sizeof(Uint32))
		Fail(errorType_Misc, error_Corrupt);

	vers = TB(vers);
	CNewsDatabase *db = nil;

	if(vers == 2)
		db = new CNewsDatabase_v2(inFile);
	else if(vers == 3)
		db = new CNewsDatabase_v3(inFile);
	else
		Fail(errorType_Misc, error_VersionUnknown);

	return db;
}

CNewsDatabase *CNewsDatabase::Create(TFSRefObj* inRef, const Uint8 inName[], const Uint8 inDesc[], bool inOldVersion)
{
	ASSERT(inRef);

	if (inRef->Exists())
		Fail(errorType_FileSys, fsError_ItemAlreadyExists);
	
	CNewsDatabase *db = nil;

	if (inOldVersion)
	{
		db = new CNewsDatabase_v2(inRef);
		((CNewsDatabase_v2 *)db)->CreateNewGroup(inName, inDesc);
	}
	else
	{
		db = new CNewsDatabase_v3(inRef);
		((CNewsDatabase_v3 *)db)->CreateNewGroup(inName, inDesc);
	}
	
	return db;
}

// without this ~CNewsDatabase_v2 and ~CNewsDatabase_v3 are not called
CNewsDatabase::~CNewsDatabase()
{
}

bool CNewsDatabase::SetArticleData(Uint32 inArticleID, const Uint8 *inName, const Int8 inType[], const Uint8 *inData, Uint32 inDataSize)
{
	DeleteArticleData(inArticleID, inName);

	return AddArticleData(inArticleID, inName, inType, inData, inDataSize);
}

bool CNewsDatabase::CopyCategory(TFSRefObj* inDest)
{
	Require(inDest);

	bool bIsFolder;
	CNewsDatabase *pDest;
	
	if (inDest->Exists(&bIsFolder))
	{
		if (bIsFolder)
			return false;
	
		pDest = CNewsDatabase::Construct(inDest);
	}
	else
	{
		SMyNewsGroupInfo stNewsGroupInfo;
		this->GetGroupInfo(stNewsGroupInfo);
		
		pDest = CNewsDatabase::Create(inDest, stNewsGroupInfo.name, stNewsGroupInfo.desc);
	}
		
	bool bRet = _NZ_CopyCategory(this, pDest);
	delete pDest;
	
	return bRet;
}

bool CNewsDatabase::CopyCategory(CNewsDatabase *inDest)
{
	return _NZ_CopyCategory(this, inDest);
}

Uint32 CNewsDatabase::CopyArticle(Uint32 inArticleID, CNewsDatabase *inDest, Uint32 inParentID, bool inCopyChildren)
{
	return _NZ_CopyArticle(this, inArticleID, inDest, inParentID, inCopyChildren);
}

Uint32 CNewsDatabase::MoveArticle(Uint32 inArticleID, CNewsDatabase *inDest, Uint32 inParentID, bool inMoveChildren)
{
	Uint32 nArticleID = _NZ_CopyArticle(this, inArticleID, inDest, inParentID, inMoveChildren);
	
	if (nArticleID)
		DeleteArticle(inArticleID, inMoveChildren);
		
	return nArticleID;
}

#pragma mark -

struct SArticleIDsInfo {
	Uint32 nOldArticleID;
	Uint32 nNewArticleID;
};

bool _NZ_CopyCategory(CNewsDatabase *inSource, CNewsDatabase *inDest)
{
	Uint32 nSize;
	TPtr pList = inSource->GetArticleListing(nSize);
	if (!pList)
		return false;
	
	CUnflatten unflat(pList, nSize);
	if (unflat.NotEnufData(4))
	{
		UMemory::Dispose(pList);
		return false;
	}
	
	// article count
	Uint32 nCount = unflat.ReadLong();
	if (!nCount)
	{
		UMemory::Dispose(pList);
		return false;
	}
				
	SArticleIDsInfo *pArticleIDsInfo;
	CPtrList<SArticleIDsInfo> lArticleIDsList;
		
	try
	{
		// now for the individual articles
		while (nCount--)
		{
			if (unflat.NotEnufData(4 + 8 + 4 + 4 + 2 + 3))
			{
				// corrupt
				Fail(errorType_Misc, error_Corrupt);
			}
			
			Uint32 nOldArticleID = unflat.ReadLong();
						
			SDateTimeStamp stDateTime;
			unflat.ReadDateTimeStamp(stDateTime);
			
			Uint32 nNewParentID = 0;
			Uint32 nOldParentID = unflat.ReadLong();
			
			if (nOldParentID)
			{
				Uint32 i = 0;
				while (lArticleIDsList.GetNext(pArticleIDsInfo, i))
				{
					if (pArticleIDsInfo->nOldArticleID == nOldParentID)
					{
						nNewParentID = pArticleIDsInfo->nNewArticleID;
						break;
					}
				}
				
			#if DEBUG
				if (!nNewParentID)
					DebugBreak("Corrupt data - it says I have a parent, but none of that ID exists!");
			#endif
			}
			
			Uint32 nFlags = unflat.ReadLong();
			Uint16 nFlavCount = unflat.ReadWord();
			
			// title
			Uint8 pstrLen = unflat.ReadByte();
			
			if (unflat.NotEnufData(pstrLen + 1))
			{
				// corrupt!
				Fail(errorType_Misc, error_Corrupt);
			}
			
			Uint8 psArticleName[64];
			psArticleName[0] = UMemory::Copy(psArticleName + 1, unflat.GetPtr(), min((Uint32)pstrLen, (Uint32)(sizeof(psArticleName) - 1)));
			unflat.Skip(pstrLen);
			
			// poster
			pstrLen = unflat.ReadByte();
			if (unflat.NotEnufData(pstrLen))
			{
				// corrupt!
				Fail(errorType_Misc, error_Corrupt);
			}
			
			Uint8 psArticlePoster[32];
			psArticlePoster[0] = UMemory::Copy(psArticlePoster + 1, unflat.GetPtr(), min((Uint32)pstrLen, (Uint32)(sizeof(psArticlePoster) - 1)));
			unflat.Skip(pstrLen);			
						
			// article ID
			pstrLen = unflat.ReadByte();
			if (unflat.NotEnufData(pstrLen))
			{
				// corrupt!
				Fail(errorType_Misc, error_Corrupt);
			}
			
			Uint8 psArticleID[64];
			psArticleID[0] = UMemory::Copy(psArticleID + 1, unflat.GetPtr(), min((Uint32)pstrLen, (Uint32)(sizeof(psArticleID) - 1)));
			unflat.Skip(pstrLen);	

			// generate ID
			if (!psArticleID[0])
				psArticleID[0] = _NZ_GenerateExternID(psArticleID + 1, sizeof(psArticleID) - 1);
	
			// create new article
			Uint32 nNewArticleID = inDest->AddArticle(nNewParentID, psArticleName, psArticlePoster, nFlags, &stDateTime, psArticleID);
			if (!nNewArticleID)
				Fail(errorType_Misc, error_Corrupt);

			// data
			while (nFlavCount--)
			{
				if (unflat.NotEnufData(sizeof(Uint8)))
				{
					// corrupt
					Fail(errorType_Misc, error_Corrupt);
				}

				Uint8 *psName = unflat.ReadPString();
				if (!psName || unflat.NotEnufData(sizeof(Uint8)))
				{
					// corrupt
					Fail(errorType_Misc, error_Corrupt);
				}

				Uint8 *psFlav = unflat.ReadPString();
				if (!psFlav || unflat.NotEnufData(sizeof(Uint16)))
				{
					// corrupt
					Fail(errorType_Misc, error_Corrupt);
				}
				
				unflat.SkipWord();

				Int8 csFlavor[32];
				Uint32 nFlavSize = UMemory::Copy(csFlavor, psFlav + 1, psFlav[0] > sizeof(csFlavor) - 1 ? sizeof(csFlavor) - 1 : psFlav[0]);
				*(csFlavor + nFlavSize) = 0;

				// read data
				Uint32 nDataSize;
				TPtr pData = inSource->GetArticleData(nOldArticleID, psName, nDataSize);
				if (!pData)
					Fail(errorType_Misc, error_Corrupt);

				try
				{
					// write data
					if (!inDest->AddArticleData(nNewArticleID, psName, csFlavor, (Uint8 *)pData, nDataSize))
						Fail(errorType_Misc, error_Corrupt);
				}
				catch(...)
				{
					UMemory::Dispose(pData);
					throw;
				}

				UMemory::Dispose(pData);
			}
			
			// add ID's in list
			SArticleIDsInfo *pArticleIDsInfo = (SArticleIDsInfo *)UMemory::New(sizeof(SArticleIDsInfo));
			
			pArticleIDsInfo->nOldArticleID = nOldArticleID;
			pArticleIDsInfo->nNewArticleID = nNewArticleID;
			lArticleIDsList.AddItem(pArticleIDsInfo);
		}		
	}
	catch(...)
	{
		Uint32 i = 0;
		while (lArticleIDsList.GetNext(pArticleIDsInfo, i))
			UMemory::Dispose((TPtr)pArticleIDsInfo);

		lArticleIDsList.Clear();
		UMemory::Dispose(pList);

		throw;
	}

	Uint32 i = 0;
	while (lArticleIDsList.GetNext(pArticleIDsInfo, i))
		UMemory::Dispose((TPtr)pArticleIDsInfo);

	lArticleIDsList.Clear();
	UMemory::Dispose(pList);
	
	return true;
}

Uint32 _NZ_CopyArticle(CNewsDatabase *inSource, Uint32 inArticleID, CNewsDatabase *inDest, Uint32 inParentID, bool inCopyChildren, bool inCopyNext)
{
	if (!inArticleID)
		return 0;
	
	SMyNewsArticleInfo stArticleInfo;
	if (!inSource->GetArticleInfo(inArticleID, stArticleInfo))
		return 0;

	Uint32 nFlavSize;
	TPtr pFlavors = inSource->GetArticleFlavors(inArticleID, nFlavSize);
	if (!pFlavors)
		return 0;

	Uint32 nDataSize;
	TPtr pData = nil;
	Uint32 nArticleID;
	
	try
	{
		CUnflatten unflat(pFlavors, nFlavSize);
		if (unflat.NotEnufData(10))
			Fail(errorType_Misc, error_Corrupt);

		unflat.SkipLong();	// skip count (=1)

		Uint16 nFlavCount = unflat.ReadWord();
		nArticleID = inDest->AddArticle(inParentID, stArticleInfo.title, stArticleInfo.poster, stArticleInfo.flags, &stArticleInfo.dts, stArticleInfo.externId);

		while (nFlavCount--)
		{
			if (unflat.NotEnufData(sizeof(Uint8)))
				Fail(errorType_Misc, error_Corrupt);

			Uint8 *pName = unflat.ReadPString();
			if (!pName || unflat.NotEnufData(sizeof(Uint8)))
				Fail(errorType_Misc, error_Corrupt);

			Uint8 *pFlav = unflat.ReadPString();
			if (!pFlav || unflat.NotEnufData(sizeof(Uint16)))
				Fail(errorType_Misc, error_Corrupt);
								
			unflat.SkipWord();

			Int8 csFlavor[32];
			nFlavSize = UMemory::Copy(csFlavor, pFlav + 1, pFlav[0] > sizeof(csFlavor) - 1 ? sizeof(csFlavor) - 1 : pFlav[0]);
			*(csFlavor + nFlavSize) = 0;

			pData = inSource->GetArticleData(inArticleID, pName, nDataSize);
			if (pData)
			{
				inDest->AddArticleData(nArticleID, pName, csFlavor, (Uint8 *)pData, nDataSize);
								
				UMemory::Dispose(pData);
				pData = nil;
			}
		}
	}
	catch(...)
	{
		UMemory::Dispose(pFlavors);
		UMemory::Dispose(pData);
		throw;
	}

	UMemory::Dispose(pFlavors);
	
	if (inCopyChildren)
	{
		_NZ_CopyArticle(inSource, stArticleInfo.firstChildID, inDest, nArticleID, inCopyChildren, true);
	
		if (inCopyNext)
			_NZ_CopyArticle(inSource, stArticleInfo.nextID, inDest, inParentID, inCopyChildren, true);
	}
	
	return nArticleID;
}

Uint32 _NZ_GenerateExternID(Uint8 *outExternID, Uint32 inMaxSize)
{
	SGUID stGuid;
	UGUID::Generate(stGuid);

	Uint8 psGuid[64];
	psGuid[0] = UGUID::ToText(stGuid, psGuid + 1, sizeof(psGuid) - 1);

	return UText::Format(outExternID, inMaxSize, "%#s@bigredh.com", psGuid);
}
