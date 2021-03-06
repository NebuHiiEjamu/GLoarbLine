/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "NewsSynch.h"
#include "CNewsDatabase.h"

extern CMyApplication *gApp;


#pragma mark CMyTask
CMyTask::CMyTask(const Uint8 *inDesc)
{
	mStage = 1;
	mIsFinished = false;
	
	gApp->AddTask(this, inDesc);
}

CMyTask::~CMyTask()
{
	gApp->RemoveTask(this);
}

void CMyTask::Process()
{
	// do nothing by default
}

void CMyTask::ShowProgress(Uint32 inVal, Uint32 inMax, const Uint8 *inDesc)
{
	gApp->SetTaskProgress(this, inVal, inMax, inDesc);
}

void CMyTask::Finish()
{
	mIsFinished = true;
	gApp->FinishTask(this);
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMyConnectTask

CMyConnectTask::CMyConnectTask(SMyServerInfo& inServerInfo)
	: CMyTask("\pConnecting...")
{
	UMemory::Copy(mServerAddr, inServerInfo.psServerAddr, inServerInfo.psServerAddr[0] + 1);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Connecting with news server �%#s�", mServerAddr);
	ShowProgress(0, 1, psBuf);

	mNntpTransact->SetMessageHandler(gApp->MessageHandler, gApp);
	mNntpTransact->StartConnect(inServerInfo.psServerAddr, inServerInfo.psUserLogin, inServerInfo.psUserPassword);
	
	mLogID = 0;
	
	mFinishTask = false;
	mFinishTimer = UTimer::New(FinishTimer, this);
	mFinishTimer->Start(40000);		// 40 seconds
}

CMyConnectTask::~CMyConnectTask()
{
	UTimer::Dispose(mFinishTimer);
}

bool CMyConnectTask::IsConnectTask(const Uint8 *inServerAddr)
{
	if (!inServerAddr || UText::CompareInsensitive(mServerAddr + 1, mServerAddr[0], inServerAddr + 1, inServerAddr[0]))
		return false;
		
	return true;
}

void CMyConnectTask::FinishTimerArmed()
{
	mFinishTimer->Start(1800000);	// 30 minutes

	if (!mFinishTask)
		mFinishTask = true;
}

void CMyConnectTask::Process()
{
	switch (mStage)
	{
		case 1:
			if (mNntpTransact->IsCommandExecuted() && mNntpTransact->IsCommandError())
			{	
				LogError();
				Finish();
			}
			break;
	};
}

void CMyConnectTask::MakeLog(const Uint8 *inLogText)
{
	mLogID = gApp->MakeLog(inLogText);
}

void CMyConnectTask::AddLog(const Uint8 *inLogText)
{
	gApp->AddLog(mLogID, inLogText);
}

void CMyConnectTask::LogRefused()
{
	Uint8 *pErrorMsg = "\pError: Connection refused";
	
	ShowProgress(100, 100, pErrorMsg);
	gApp->AddLog(mLogID, pErrorMsg);
}

void CMyConnectTask::LogClosed()
{
	Uint8 *pErrorMsg = "\pError: Connection closed";
	
	ShowProgress(100, 100, pErrorMsg);
	gApp->AddLog(mLogID, pErrorMsg);
}

void CMyConnectTask::LogError(bool inShowProgress)
{
	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: %#s", mNntpTransact->GetCommandError());

	if (inShowProgress)
		ShowProgress(100, 100, psBuf);
	
	gApp->AddLog(mLogID, psBuf);
}

void CMyConnectTask::LogStop()
{
	Uint8 *pErrorMsg = "\pError: Task stopped";
	
	ShowProgress(100, 100, pErrorMsg);
	gApp->AddLog(mLogID, pErrorMsg);
}

void CMyConnectTask::SelectLog()
{
	gApp->SelectLog(mLogID);
}

void CMyConnectTask::FinishTimer(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{	
	#pragma unused(inObject, inMsg, inData, inDataSize)

	CMyConnectTask *pConnectTask = (CMyConnectTask *)inContext;

	if (pConnectTask->mFinishTask)
	{
		pConnectTask->LogClosed();
		pConnectTask->Finish();
	}
	else if (!pConnectTask->IsConnected())
	{
		pConnectTask->LogRefused();
		pConnectTask->Finish();
	}
	else
		pConnectTask->FinishTimerArmed();
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMyListGroupsTask

CMyListGroupsTask::CMyListGroupsTask(SMyServerInfo& inServerInfo)
	: CMyConnectTask(inServerInfo)
{
	mGroupIndex = 0;

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Listing news groups from server �%#s�", mServerAddr);
	MakeLog(psBuf);
}

bool CMyListGroupsTask::IsListGroupsTask(const Uint8 *inServerAddr)
{
	return IsConnectTask(inServerAddr);
}

void CMyListGroupsTask::LogReceived()
{
	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "%lu news groups received", mGroupIndex);

	AddLog(psBuf);
}

void CMyListGroupsTask::Process()
{
	switch (mStage)
	{
		case 1:
			CMyConnectTask::Process();
 				
			if (mNntpTransact->IsConnected())
			{
				ShowProgress(0, 1, "\pListing news groups...");
			
				mNntpTransact->Command_ListGroups();
				mStage++;
			}
			break;
			
		case 2:
			if (gApp->DoListGroups1(mGroupIndex, mServerAddr, mNntpTransact))
			{
				Uint8 psBuf[256];
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Listing news groups (%lu received)...", mGroupIndex);
				ShowProgress(0, 1, psBuf);

				FinishTimerArmed();
			}

			if (mNntpTransact->IsCommandExecuted())
			{	
				if (mNntpTransact->IsCommandError())
					LogError();
				else
					LogReceived();

				Finish();
				return;
			}
			
			if (!mNntpTransact->IsConnected())
			{
				LogReceived();
				LogClosed();
				Finish();
			}
			break;
	};
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMyListArticlesTask

CMyListArticlesTask::CMyListArticlesTask(SMyServerInfo& inServerInfo, const Uint8 *inGroupName)
	: CMyConnectTask(inServerInfo)
{
	mArticleIndex = 0;
	UMemory::Copy(mGroupName, inGroupName, inGroupName[0] + 1);	

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Listing articles from news group �%#s�", mGroupName);
	MakeLog(psBuf);
}

CMyListArticlesTask::~CMyListArticlesTask()
{
	Uint32 i = 0;
	SMyArticleInfo1 *pArticleInfo;

	while (mTempArticleList.GetNext(pArticleInfo, i))
		UMemory::Dispose((TPtr)pArticleInfo);
		
	mTempArticleList.Clear();
}

bool CMyListArticlesTask::IsListArticlesTask(const Uint8 *inServerAddr, const Uint8 *inGroupName)
{
	if (!IsConnectTask(inServerAddr))
		return false;
		
	if (!inGroupName || UText::CompareInsensitive(mGroupName + 1, mGroupName[0], inGroupName + 1, inGroupName[0]))
		return false;
		
	return true;
}

void CMyListArticlesTask::LogReceived()
{
	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "%lu articles received", mArticleIndex);
	
	AddLog(psBuf);
}

void CMyListArticlesTask::Process()
{
	Uint8 psBuf[256];

	switch (mStage)
	{
		case 1:
			CMyConnectTask::Process();

			if (mNntpTransact->IsConnected())
			{
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Selecting news group �%#s�", mGroupName);
				ShowProgress(0, 1, psBuf);

				mNntpTransact->Command_SelectGroup(mGroupName);
				mStage++;
			}
			break;
			
		case 2:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}
				
				FinishTimerArmed();
				
				ShowProgress(0, 1, "\pListing articles...");
				
				mNntpTransact->Command_ListArticles();
				mStage++;
			} 
			break;

		case 3:
			bool bExecuted = mNntpTransact->IsCommandExecuted();
			if (gApp->DoListArticles1(mArticleIndex, mGroupName, mNntpTransact, mTempArticleList, bExecuted))
			{
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Listing articles (%lu received)...", mArticleIndex);
				ShowProgress(0, 1, psBuf);
				
				FinishTimerArmed();
			}

			if (bExecuted)
			{	
				if (mNntpTransact->IsCommandError())
					LogError();
				else
					LogReceived();
				
				Finish();
				return;
			}
			
			if (!mNntpTransact->IsConnected())
			{
				LogReceived();
				LogClosed();
				Finish();
			}
			break;
	};
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMyGetArticleTask

CMyGetArticleTask::CMyGetArticleTask(SMyServerInfo& inServerInfo, const Uint8 *inGroupName, const Uint8 *inArticleName, Uint32 inArticleID)
	: CMyConnectTask(inServerInfo)
{
	UMemory::Copy(mGroupName, inGroupName, inGroupName[0] + 1);
	UMemory::Copy(mArticleName, inArticleName, inArticleName[0] + 1);
	mArticleID = inArticleID;	
	mArticleSize = 0;

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Receiving article �%#s�", mArticleName);
	MakeLog(psBuf);
}

bool CMyGetArticleTask::IsGetArticleTask(const Uint8 *inServerAddr, const Uint8 *inGroupName, Uint32 inArticleID)
{
	if (!IsConnectTask(inServerAddr))
		return false;
		
	if (!inGroupName || UText::CompareInsensitive(mGroupName + 1, mGroupName[0], inGroupName + 1, inGroupName[0]))
		return false;
		
	if (mArticleID != inArticleID)
		return false;
		
	return true;
}

void CMyGetArticleTask::Process()
{
	Uint8 psBuf[256];

	switch (mStage)
	{
		case 1:
			CMyConnectTask::Process();

			if (mNntpTransact->IsConnected())
			{
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Selecting news group �%#s�", mGroupName);
				ShowProgress(1, 3, psBuf);

				mNntpTransact->Command_SelectGroup(mGroupName);
				mStage++;
			}
			break;
			
		case 2:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}

				FinishTimerArmed();
				
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Receiving article �%#s�", mArticleName);
				ShowProgress(2, 3, psBuf);
			
				mNntpTransact->Command_GetArticle(mArticleID);
				mStage++;
			}
			break;

		case 3:
			Uint32 nDataSize = mNntpTransact->GetDataSize();

			Uint8 psDataSize[32];
			psDataSize[0] = UText::SizeToText(nDataSize, psDataSize + 1, sizeof(psDataSize) - 1, kDontShowBytes);

			if (nDataSize > mArticleSize)
			{
				mArticleSize = nDataSize;
				
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Receiving article �%#s� (%#s)", mArticleName, psDataSize);
				ShowProgress(2, 3, psBuf);

				FinishTimerArmed();
			}
			
			if (mNntpTransact->IsCommandExecuted())
			{	
				if (mNntpTransact->IsCommandError())
					LogError();
				else
				{
					psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Article received (%#s)", psDataSize);
					ShowProgress(100, 100, psBuf);
					AddLog(psBuf);

					gApp->DoGetArticle1(mArticleID, mNntpTransact);
				}
				
				Finish();
				return;
			}
			
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
			}				
			break;
	};
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMySynchGroupTask

CMySynchGroupTask::CMySynchGroupTask(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, Uint32 inPurgeDays, Uint32 inMaxArticles)
	: CMyConnectTask(inServerInfo)
{
	mArticleIndex1 = 0;
	UMemory::Copy(mGroupName1, inGroupName1, inGroupName1[0] + 1);
	
	mGroupName2[0] = 0;
	mSynchFile2 = nil;
	
	ClearStruct(mPurgeDate);
	if (inPurgeDays)
	{
		UDateTime::GetCalendarDate(calendar_Gregorian, mPurgeDate);
		mPurgeDate -= inPurgeDays * kDay_Seconds;
	}
	
	mMaxArticles = inMaxArticles;

	mArticleID1 = 0;
	mTreeIndex1 = 0;	
	mTreeIndex2 = 0;
}

CMySynchGroupTask::~CMySynchGroupTask()
{
	UFileSys::Dispose(mSynchFile2);

	Uint32 i = 0;
	SMyArticleIDsInfo1 *pArticleIDsInfo1;

	while (mArticleIDsTree1.GetNext(pArticleIDsInfo1, i))
		UMemory::Dispose((TPtr)pArticleIDsInfo1);
		
	mArticleIDsTree1.Clear();

	i = 0;
	while (mTempArticleIDsList1.GetNext(pArticleIDsInfo1, i))
		UMemory::Dispose((TPtr)pArticleIDsInfo1);
		
	mTempArticleIDsList1.Clear();
		
	i = 0;
	SMyArticleIDsInfo2 *pArticleIDsInfo2;

	while (mArticleIDsTree2.GetNext(pArticleIDsInfo2, i))
	{
		ClearDataList(pArticleIDsInfo2);
		UMemory::Dispose((TPtr)pArticleIDsInfo2);
	}
		
	mArticleIDsTree2.Clear();
}

bool CMySynchGroupTask::IsSynchGroupTask(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFile)
{
	if (!IsConnectTask(inServerAddr))
		return false;
		
	if (!inGroupName1 || UText::CompareInsensitive(mGroupName1 + 1, mGroupName1[0], inGroupName1 + 1, inGroupName1[0]))
		return false;
			
	if (!UFileSys::Equals(mSynchFile2, inGroupFile))
		return false;
			
	return true;
}

bool CMySynchGroupTask::IsSynchGroupTask(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2)
{
	if (!IsConnectTask(inServerAddr))
		return false;
		
	if (!inGroupName1 || 
		 UText::CompareInsensitive(mGroupName1 + 1, mGroupName1[0], inGroupName1 + 1, inGroupName1[0]))
		return false;
		
	TFSRefObj* pSynchFile2 = UFileSys::New(inGroupFolder, nil, 0, inGroupName2);
	scopekill(TFSRefObj, pSynchFile2);
	
	if (!UFileSys::Equals(mSynchFile2, pSynchFile2))
		return false;
			
	return true;
}
		
bool CMySynchGroupTask::SetArticleIDsTree1(bool inFinish)
{
	Uint32 nArticleIndex1 = mArticleIndex1;
	
	Uint32 i, nParentIndex;
	SNewsArticleInfo stArticleInfo;
	
	while (mNntpTransact->GetNextArticle(stArticleInfo, mArticleIndex1))
	{
		// check purge date
		if (mPurgeDate.IsValid() && mPurgeDate > stArticleInfo.stArticleDate)
			continue;
				
		nParentIndex = 0;	
		
		// search parent
		if (stArticleInfo.psParentID[0])
		{
			i = 0;
			SMyArticleIDsInfo1 *pParentIDsInfo;
			
			while (mArticleIDsTree1.GetNext(pParentIDsInfo, i))
			{
				if (!UMemory::Compare(stArticleInfo.psParentID + 1, 
									  stArticleInfo.psParentID[0], 
									  pParentIDsInfo->psArticleID + 1, 
									  pParentIDsInfo->psArticleID[0]))
				{
					nParentIndex = i;
					break;
				}
			}
		}

		SMyArticleIDsInfo1 *pArticleIDsInfo = (SMyArticleIDsInfo1 *)
				UMemory::NewClear(sizeof(SMyArticleIDsInfo1));
		
		pArticleIDsInfo->nArticleID = stArticleInfo.nArticleNumber;
		UMemory::Copy(pArticleIDsInfo->psArticleID, stArticleInfo.psArticleID, stArticleInfo.psArticleID[0] + 1);
		UMemory::Copy(pArticleIDsInfo->psParentID, stArticleInfo.psParentID, stArticleInfo.psParentID[0] + 1);
		pArticleIDsInfo->stArticleDate = stArticleInfo.stArticleDate;
		
		// if parent don't exist yet add in temporar list until finish
		if (stArticleInfo.psParentID[0] && !nParentIndex)
			mTempArticleIDsList1.AddItem(pArticleIDsInfo);
		else
			mArticleIDsTree1.AddItem(nParentIndex, pArticleIDsInfo);
	}
	
	mArticleIndex1--;	
	
	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
					"Listing articles (%lu received)...", 
					mArticleIndex1);
	ShowProgress(0, 1, psBuf);
	
	if (inFinish)
	{
		CPtrList<SMyArticleIDsInfo1> pSearchArticleIDsList;
		
		i = 0;
		SMyArticleIDsInfo1 *pArticleIDsInfo;
	
		// search parent
		while (mTempArticleIDsList1.GetNext(pArticleIDsInfo, i))
		{
			nParentIndex = 0;

			Uint32 j = 0;
			SMyArticleIDsInfo1 *pParentIDsInfo;
			
			while (mArticleIDsTree1.GetNext(pParentIDsInfo, j))
			{
				if (!UMemory::Compare(pArticleIDsInfo->psParentID + 1, pArticleIDsInfo->psParentID[0], pParentIDsInfo->psArticleID + 1, pParentIDsInfo->psArticleID[0]))
				{
					nParentIndex = j;
					break;
				}
			}
		
			if (nParentIndex)
			{
				mArticleIDsTree1.AddItem(nParentIndex, pArticleIDsInfo);
				mTempArticleIDsList1.RemoveItem(i);
				i--;
				
				pSearchArticleIDsList.AddItem(pArticleIDsInfo);
			}
		}
		
		SetArticleIDsTree1(mTempArticleIDsList1, pSearchArticleIDsList);
		pSearchArticleIDsList.Clear();
		
		i = 0; 	// if parent don't exist search brother
		while (mTempArticleIDsList1.GetNext(pArticleIDsInfo, i))
		{
			nParentIndex = 0;

			Uint32 j = 0;
			SMyArticleIDsInfo1 *pParentIDsInfo;
			
			while (mArticleIDsTree1.GetNext(pParentIDsInfo, j))
			{
				if (!UMemory::Compare(pArticleIDsInfo->psParentID + 1, pArticleIDsInfo->psParentID[0], pParentIDsInfo->psParentID + 1, pParentIDsInfo->psParentID[0]))
				{
					nParentIndex = j;
					break;
				}
			}
					
			mArticleIDsTree1.AddItem(nParentIndex, pArticleIDsInfo);
			mTempArticleIDsList1.RemoveItem(i);
			i = 0;

			pSearchArticleIDsList.AddItem(pArticleIDsInfo);
			SetArticleIDsTree1(mTempArticleIDsList1, pSearchArticleIDsList);
			pSearchArticleIDsList.Clear();
		}		

		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
						"%lu articles received from news group �%#s�", 
						mArticleIDsTree1.GetTreeCount(), mGroupName1);
						//mArticleIndex1, mGroupName1);
		AddLog(psBuf);	
	}

	return (mArticleIndex1 > nArticleIndex1);
}

void CMySynchGroupTask::SetArticleIDsTree1(CPtrList<SMyArticleIDsInfo1>& inTempArticleIDsList, 
										   CPtrList<SMyArticleIDsInfo1>& inSearchArticleIDsList)
{
	if (!inTempArticleIDsList.GetItemCount() || 
		!inSearchArticleIDsList.GetItemCount())
		return;
		
	Uint32 nParentIndex;
	CPtrList<SMyArticleIDsInfo1> pSearchArticleIDsList;

	Uint32 i = 0;
	SMyArticleIDsInfo1 *pArticleIDsInfo;

	// search parent
	while (inTempArticleIDsList.GetNext(pArticleIDsInfo, i))
	{
		nParentIndex = 0;

		Uint32 j = 0;
		SMyArticleIDsInfo1 *pParentIDsInfo;
			
		while (inSearchArticleIDsList.GetNext(pParentIDsInfo, j))
		{
			if (!UMemory::Compare(pArticleIDsInfo->psParentID + 1, 
								  pArticleIDsInfo->psParentID[0], 
								  pParentIDsInfo->psArticleID + 1, 
								  pParentIDsInfo->psArticleID[0]))
			{
				nParentIndex = mArticleIDsTree1.GetItemIndex(pParentIDsInfo);
				break;
			}
		}
		
		if (nParentIndex)
		{
			mArticleIDsTree1.AddItem(nParentIndex, pArticleIDsInfo);
			inTempArticleIDsList.RemoveItem(i);
			i--;
				
			pSearchArticleIDsList.AddItem(pArticleIDsInfo);
		}
	}
	
	if (pSearchArticleIDsList.GetItemCount())
	{			
		SetArticleIDsTree1(inTempArticleIDsList, pSearchArticleIDsList);
		pSearchArticleIDsList.Clear();		
	}	
}

bool CMySynchGroupTask::SetArticleIDsTree2()
{
	if (!mSynchFile2->Exists())
		return false;

	StNewsDatabase pNewsDatabase(mSynchFile2);
	
	Uint32 nListSize;
	TPtr pArticleList = pNewsDatabase->GetArticleListing(nListSize);
	if (!pArticleList)
		return false;
		
	CUnflatten unflat(pArticleList, nListSize);
	if (unflat.NotEnufData(4))
		return false;
	
	// article count
	Uint32 nCount = unflat.ReadLong();
	if (!nCount)
		return false;
		
	Uint32 nArticleCount = nCount;
		
	SMyArticleIDsInfo2 *pArticleIDsInfo = nil;
	SDateTimeStamp stArticleDate;
		
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
			
			pArticleIDsInfo = (SMyArticleIDsInfo2 *)
				UMemory::NewClear(sizeof(SMyArticleIDsInfo2));
		
			pArticleIDsInfo->nArticleID = unflat.ReadLong();
			unflat.ReadDateTimeStamp(stArticleDate);
			pArticleIDsInfo->stArticleDate = (SCalendarDate)stArticleDate;
			Uint32 nParentID = unflat.ReadLong();
			Uint32 nParentIndex = 0;
			
			if (nParentID)	// search parent begining from the end of the list
			{
				Uint32 i = mArticleIDsTree2.GetTreeCount() + 1;
				SMyArticleIDsInfo2 *pTmpArticleIDsInfo;
				
				while (mArticleIDsTree2.GetPrev(pTmpArticleIDsInfo, i))
				{
					if (pTmpArticleIDsInfo->nArticleID == nParentID)
					{
						nParentIndex = i;							
						break;
					}
				}
				
			#if DEBUG
				if (!nParentIndex)
					DebugBreak("Corrupt data - it says I have a parent, but none of that ID exists!");
			#endif
			}
			
			unflat.SkipLong();	// skip flags
			Uint16 nFlavCount = unflat.ReadWord();
			
			// title
			Uint8 pstrLen = unflat.ReadByte();	
			if (unflat.NotEnufData(pstrLen + 1))
			{
				// corrupt!
				Fail(errorType_Misc, error_Corrupt);
			}
						
			unflat.Skip(pstrLen);
			
			// poster
			pstrLen = unflat.ReadByte();
			if (unflat.NotEnufData(pstrLen))
			{
				// corrupt!
				Fail(errorType_Misc, error_Corrupt);
			}
						
			unflat.Skip(pstrLen);	
		
			// article ID
			pstrLen = unflat.ReadByte();
			if (unflat.NotEnufData(pstrLen))
			{
				// corrupt!
				Fail(errorType_Misc, error_Corrupt);
			}
			
			pArticleIDsInfo->psArticleID[0] = UMemory::Copy(
				pArticleIDsInfo->psArticleID + 1, 
			    unflat.GetPtr(), 
			    min((Uint32)pstrLen, (Uint32)(sizeof(pArticleIDsInfo->psArticleID) - 1)));
			unflat.Skip(pstrLen);	
		
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
			
				pArticleIDsInfo->nDataSize += unflat.ReadWord();

				SMyDataInfo2 *pDataInfo = (SMyDataInfo2 *)UMemory::NewClear(sizeof(SMyDataInfo2));
				
				pDataInfo->psName[0] = UMemory::Copy(pDataInfo->psName + 1, 
													 psName + 1, 
													 psName[0] > sizeof(pDataInfo->psName) - 1 ? sizeof(pDataInfo->psName) - 1 : psName[0]);
				Uint32 nFlavSize = UMemory::Copy(pDataInfo->csFlavor, 
												 psFlav + 1, 
												 psFlav[0] > sizeof(pDataInfo->csFlavor) - 1 ? sizeof(pDataInfo->csFlavor) - 1 : psFlav[0]);
				*(pDataInfo->csFlavor + nFlavSize) = 0;
				
				pArticleIDsInfo->lDataList.AddItem(pDataInfo);	
			}	
			
			try
			{
				if (!pArticleIDsInfo->psArticleID[0])	// 1.7.2 servers don't generate extern ID
					pArticleIDsInfo->psArticleID[0] = pNewsDatabase->GenerateExternID(pArticleIDsInfo->nArticleID, pArticleIDsInfo->psArticleID + 1, sizeof(pArticleIDsInfo->psArticleID) - 1);
			}
			catch(...)
			{
				ClearDataList(pArticleIDsInfo);
				UMemory::Dispose((TPtr)pArticleIDsInfo);
				pArticleIDsInfo = nil;

				continue;	// don't throw here
			}

			mArticleIDsTree2.AddItem(nParentIndex, pArticleIDsInfo);
			pArticleIDsInfo = nil;			
		}		
	}
	catch(...)
	{
		UMemory::Dispose(pArticleList);
		UMemory::Dispose((TPtr)pArticleIDsInfo);
		
		throw;
	}

	UMemory::Dispose(pArticleList);
	
	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "%lu articles in news category �%#s�", nArticleCount, mGroupName2);
	AddLog(psBuf);	
	
	PurgeArticles();
	return true;
}

bool CMySynchGroupTask::SelectNextArticle1()
{
	SMyArticleIDsInfo1 *pArticleIDsInfo1;

	while (mArticleIDsTree1.GetNext(pArticleIDsInfo1, mTreeIndex1))
	{
		if (!pArticleIDsInfo1->bSynchronized)
		{
			mArticleID1 = pArticleIDsInfo1->nArticleID;
			return true;
		}
	}
		
	return false;
}

bool CMySynchGroupTask::SelectNextArticle2()
{
	SMyArticleIDsInfo2 *pArticleIDsInfo2;

	while (mArticleIDsTree2.GetNext(pArticleIDsInfo2, mTreeIndex2))
	{
		if (!pArticleIDsInfo2->bSynchronized)
			return true;
	}
			
	return false;
}

bool CMySynchGroupTask::CreateGroup2()
{
	if (mSynchFile2->Exists())
		return true;
		
	try
	{
		StNewsDatabase pNewsDatabase(mSynchFile2, mGroupName2, "\p", gApp->UseOldNewsFormat());
	}
	catch (SError& err)
	{
		Uint8 psBuf[256];
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: Cannot create news category �%#s�", mGroupName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		return false;
	}
	
	gApp->DoListGroups2();
	return true;
}

Uint32 CMySynchGroupTask::GetParentID2(const Uint8 *inArticleID, const Uint8 *inParentID, Uint32& outParentIndex)
{
	outParentIndex = 0;
	
	if (!inParentID || !inParentID[0])
		return 0;

	// search parent
	Uint32 i = 0;
	SMyArticleIDsInfo2 *pArticleIDsInfo2;
	
	while (mArticleIDsTree2.GetNext(pArticleIDsInfo2, i))
	{
		if (!UMemory::Compare(pArticleIDsInfo2->psArticleID + 1, pArticleIDsInfo2->psArticleID[0], inParentID + 1, inParentID[0]))
		{
			outParentIndex = i;
			return pArticleIDsInfo2->nArticleID;
		}
	}
	
	// if parent don't exist search brother
	Uint8 psParentID[64];
	psParentID[0] = 0;
	
	i = 0;
	SMyArticleIDsInfo1 *pArticleIDsInfo1;

	while (mArticleIDsTree1.GetNext(pArticleIDsInfo1, i))
	{
		if (!UMemory::Compare(pArticleIDsInfo1->psArticleID + 1, pArticleIDsInfo1->psArticleID[0], inArticleID + 1, inArticleID[0]))
		{
			pArticleIDsInfo1 = mArticleIDsTree1.GetParentItem(i);
			if (pArticleIDsInfo1)
				psParentID[0] = UMemory::Copy(psParentID + 1, pArticleIDsInfo1->psArticleID + 1, pArticleIDsInfo1->psArticleID[0] > sizeof(psParentID) - 1 ? sizeof(psParentID) - 1 : pArticleIDsInfo1->psArticleID[0]);
			
			break;
		}
	}

	if (!psParentID[0])
		return 0;

	i = 0;
	while (mArticleIDsTree2.GetNext(pArticleIDsInfo2, i))
	{
		if (!UMemory::Compare(pArticleIDsInfo2->psArticleID + 1, pArticleIDsInfo2->psArticleID[0], psParentID + 1, psParentID[0]))
		{
			outParentIndex = i;
			return pArticleIDsInfo2->nArticleID;
		}
	}

	return 0;
}

Uint32 CMySynchGroupTask::AddArticleID2(Uint32 inParentIndex, Uint32 inArticleID1, const Uint8 *inArticleID2, const SCalendarDate& inArticleDate)
{
	SMyArticleIDsInfo2 *pArticleIDsInfo = (SMyArticleIDsInfo2 *)
		UMemory::NewClear(sizeof(SMyArticleIDsInfo2));

	pArticleIDsInfo->bSynchronized = true;
	pArticleIDsInfo->nArticleID = inArticleID1;
	UMemory::Copy(pArticleIDsInfo->psArticleID, inArticleID2, inArticleID2[0] + 1);
	pArticleIDsInfo->stArticleDate = inArticleDate;

	return mArticleIDsTree2.AddItem(inParentIndex, pArticleIDsInfo);
}

bool CMySynchGroupTask::SynchArticle1(Uint32& outSynchSize)
{
	outSynchSize = 0;

	SMyArticleIDsInfo2 *pArticleIDsInfo = mArticleIDsTree2.GetItem(mTreeIndex2);
	if (!pArticleIDsInfo)
		return false;

	SMyNewsArticleInfo stArticleInfo;

	try 
	{
		StNewsDatabase pNewsDatabase(mSynchFile2);
		pNewsDatabase->GetArticleInfo(pArticleIDsInfo->nArticleID, stArticleInfo);
	}
	catch(...)
	{
		AddLog("\pError: Article corrupt");
		return false;
	}

	Uint8 psDataSize[32];
	psDataSize[0] = UText::SizeToText(pArticleIDsInfo->nDataSize, psDataSize + 1, sizeof(psDataSize) - 1, kDontShowBytes);

	Uint8 psMsg[256];
	psMsg[0] = UText::Format(psMsg + 1, sizeof(psMsg) - 1, "<--- Synchronizing article �%#s� (%#s)", stArticleInfo.title, psDataSize);
	ShowProgress(mTreeIndex2, mArticleIDsTree2.GetTreeCount(), psMsg);

	// reverse the order of parent ID's
	Uint32 nIndex = mTreeIndex2;
	CPtrList<Uint8> pParentIDsList;

	while (true)
	{
		SMyArticleIDsInfo2 *pParentIDsInfo = mArticleIDsTree2.GetParentItem(nIndex, &nIndex);
		if (!pParentIDsInfo)
			break;
	
		pParentIDsList.InsertItem(1, pParentIDsInfo->psArticleID);	
	}

	// add parent ID's
	Uint32 i = 0;
	Uint8 *pParentID;

	while (pParentIDsList.GetNext(pParentID, i))
		mNntpTransact->AddParentID(pParentID);

	pParentIDsList.Clear();

	// add misc info
	mNntpTransact->SetArticleID(stArticleInfo.externId);
	mNntpTransact->SetOrganization(gApp->GetOrganization());
	mNntpTransact->SetNewsReader(APPLICATION_NAME);
	
	SCalendarDate stPostDate = (SCalendarDate)stArticleInfo.dts;
	mNntpTransact->SetPostDate(stPostDate);
	
	mNntpTransact->SetPostInfo(stArticleInfo.title, stArticleInfo.poster, gApp->GetEmailAddress());

	StNewsDatabase pNewsDatabase(mSynchFile2);

	// add data
	i = 0;
	SMyDataInfo2 *pDataInfo;
	
	while (pArticleIDsInfo->lDataList.GetNext(pDataInfo, i))
	{
		try
		{
			Uint32 nDataSize;
			TPtr pData = pNewsDatabase->GetArticleData(pArticleIDsInfo->nArticleID, pDataInfo->psName, nDataSize);
			if (!pData)
			{
				psMsg[0] = UText::Format(psMsg + 1, sizeof(psMsg) - 1, "Error: Article �%#s� corrupt", stArticleInfo.title);
				AddLog(psMsg);

				return false;
			}
			
			mNntpTransact->AddPostData(pDataInfo->csFlavor, pDataInfo->psName, pData, nDataSize); // take ownership of pData
			outSynchSize += nDataSize;
		}
		catch(...)
		{
			psMsg[0] = UText::Format(psMsg + 1, sizeof(psMsg) - 1, "Error: Article �%#s� corrupt", stArticleInfo.title);
			AddLog(psMsg);

			return false;
		}
	}

	// add text
	Uint32 nTextSize;
	void *pAddText = gApp->GetAddArticleText(nTextSize);
	if (pAddText)
	{
		mNntpTransact->AddPostData("text/plain", "\padd text", pAddText, nTextSize); // take ownership of pAddText
		outSynchSize += nTextSize;
	}

	return true;
}

bool CMySynchGroupTask::SynchArticle2(Uint32& outSynchSize)
{
	outSynchSize = 0;

	SNewsArticleInfo stArticleInfo;
	try
	{
		if (!mNntpTransact->GetArticleInfo(stArticleInfo))
			return false;
	}
	catch(...)
	{
		return false;	// don't throw here
	}
		
//	if (!mNntpTransact->GetDataCount())
//		return false;
		
	Uint8 psDataSize[32];
	Uint32 nDataSize = mNntpTransact->GetDataSize();
	psDataSize[0] = UText::SizeToText(nDataSize, psDataSize + 1, sizeof(psDataSize) - 1, kDontShowBytes);

	Uint8 psMsg[256];
	psMsg[0] = UText::Format(psMsg + 1, sizeof(psMsg) - 1, 
					"---> Synchronizing article �%#s� (%#s)", 
					stArticleInfo.psArticleName, psDataSize);
	ShowProgress(mTreeIndex1, mArticleIDsTree1.GetTreeCount(), psMsg);

	Uint32 nParentIndex;
	Uint32 nParentID = GetParentID2(stArticleInfo.psArticleID, stArticleInfo.psParentID, nParentIndex);
	
	SDateTimeStamp stArticleDate = (SDateTimeStamp)stArticleInfo.stArticleDate;

	Uint8 *pPosterName = stArticleInfo.psPosterName;
	if (!pPosterName[0])
		pPosterName = stArticleInfo.psPosterAddr;
		
	Uint32 nArticleID;
	try
	{
		StNewsDatabase pNewsDatabase(mSynchFile2);
		nArticleID = pNewsDatabase->AddArticle(nParentID, stArticleInfo.psArticleName, pPosterName, 0, &stArticleDate, stArticleInfo.psArticleID);
		if (!nArticleID)
		{
			psMsg[0] = UText::Format(psMsg + 1, sizeof(psMsg) - 1, 
							"Error: Article �%#s� corrupt", 
							stArticleInfo.psArticleName);
			AddLog(psMsg);

			return false;
		}
	}
	catch (...)
	{
		psMsg[0] = UText::Format(psMsg + 1, sizeof(psMsg) - 1, 
						"Error: Article �%#s� corrupt", 
						stArticleInfo.psArticleName);
		AddLog(psMsg);

		return false;
	}
		
	// save article ID's
	AddArticleID2(nParentIndex, nArticleID, stArticleInfo.psArticleID, stArticleInfo.stArticleDate);

	StNewsDatabase pNewsDatabase(mSynchFile2);

	Uint32 i = 0;
	SArticleData *pArticleData;
	
	while (mNntpTransact->GetNextData(pArticleData, i))
	{	
		try
		{
			if (pNewsDatabase->AddArticleData(nArticleID, pArticleData->psName, pArticleData->csFlavor, (Uint8 *)pArticleData->pData, pArticleData->nDataSize))
				outSynchSize += pArticleData->nDataSize;
		}
		catch (...)
		{
			psMsg[0] = UText::Format(psMsg + 1, sizeof(psMsg) - 1, "Error: Article �%#s� corrupt", stArticleInfo.psArticleName);
			AddLog(psMsg);
	
			return false;
		}		
	}
		
	return true;
}

// check max articles
void CMySynchGroupTask::MakeMaxArticles()
{
	if (!mMaxArticles || mMaxArticles >= mArticleIDsTree1.GetTreeCount() + mArticleIDsTree2.GetTreeCount())
		return;

	Uint32 nPurgeCount = 0;
	ShowProgress(0, 1, "\pPurging old articles...");

	Uint32 nArticleCount = 0;

	Uint32 i = 0;
	SMyArticleIDsInfo1 *pArticleIDsInfo1;
	SMyArticleIDsInfo2 *pArticleIDsInfo2;

	// counting unsynchronized articles from mArticleIDsTree1
	while (mArticleIDsTree1.GetNext(pArticleIDsInfo1, i))
	{
		if (!pArticleIDsInfo1->bSynchronized)
			nArticleCount++;
	}

	// counting all articles from mArticleIDsTree2
	nArticleCount += mArticleIDsTree2.GetTreeCount();

	if (nArticleCount <= mMaxArticles)
		return;
	
	// make purge list
	SPurgeInfo *pPurgeInfo;
	CPtrList<SPurgeInfo> lPurgeList;
	
	while (nArticleCount > mMaxArticles)
	{
		pPurgeInfo = (SPurgeInfo *)UMemory::NewClear(sizeof(SPurgeInfo));
		pPurgeInfo->stArticleDate.year = max_Int16;
		
		lPurgeList.AddItem(pPurgeInfo);
		nArticleCount--;
	}
		
	SCalendarDate stPurgeDate;
	Uint32 nPurgeIndex = GetPurgeDate(lPurgeList, stPurgeDate);
	
	i = 0;	// search old unsynchronized articles in mArticleIDsTree1
	while (mArticleIDsTree1.GetNext(pArticleIDsInfo1, i))
	{
		if (pArticleIDsInfo1->bSynchronized)
			continue;
		
		if (pArticleIDsInfo1->stArticleDate < stPurgeDate)
		{
			pPurgeInfo = lPurgeList.GetItem(nPurgeIndex);
			if (pPurgeInfo)
			{
				pPurgeInfo->bTreeFlag = true;
				pPurgeInfo->nArticleID = pArticleIDsInfo1->nArticleID;
				pPurgeInfo->stArticleDate = pArticleIDsInfo1->stArticleDate;
			}
			
			nPurgeIndex = GetPurgeDate(lPurgeList, stPurgeDate);
		}
	}

	i = 0;	// search all old articles in mArticleIDsTree2
	while (mArticleIDsTree2.GetNext(pArticleIDsInfo2, i))
	{
		if (pArticleIDsInfo2->stArticleDate < stPurgeDate)
		{
			pPurgeInfo = lPurgeList.GetItem(nPurgeIndex);
			if (pPurgeInfo)
			{
				pPurgeInfo->bTreeFlag = false;
				pPurgeInfo->nArticleID = pArticleIDsInfo2->nArticleID;
				pPurgeInfo->stArticleDate = pArticleIDsInfo2->stArticleDate;
			}
			
			nPurgeIndex = GetPurgeDate(lPurgeList, stPurgeDate);
		}
	}

	i = 0;	// purging old articles and clear purge list
	while (lPurgeList.GetNext(pPurgeInfo, i))
	{
		if (pPurgeInfo->nArticleID)
		{
			Uint32 j = 0;
			
			if (pPurgeInfo->bTreeFlag)
			{
				// purge old articles in mArticleIDsTree1
				while (mArticleIDsTree1.GetNext(pArticleIDsInfo1, j))
				{
					if (pArticleIDsInfo1->nArticleID == pPurgeInfo->nArticleID)
					{
						mArticleIDsTree1.RemoveItem(j, false);
						
						UMemory::Dispose((TPtr)pArticleIDsInfo1);
						break;
					}
				}
			}
			else
			{
				StNewsDatabase pNewsDatabase(mSynchFile2);

				// purge old articles in news category and mArticleIDsTree1
				while (mArticleIDsTree2.GetNext(pArticleIDsInfo2, j))
				{
					if (pArticleIDsInfo2->nArticleID == pPurgeInfo->nArticleID)
					{
						nPurgeCount++;
						
						try
						{
							pNewsDatabase->DeleteArticle(pArticleIDsInfo2->nArticleID, false);
						}
						catch (...)
						{
							AddLog("\pError: Purging article failed");
						}
						
						mArticleIDsTree2.RemoveItem(j, false);
						
						ClearDataList(pArticleIDsInfo2);
						UMemory::Dispose((TPtr)pArticleIDsInfo2);
						break;
					}
				}
			}
		}
		
		UMemory::Dispose((TPtr)pPurgeInfo);
	}

	lPurgeList.Clear();

	if (nPurgeCount)
	{
		Uint8 psBuf[256];
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "%lu articles purged", nPurgeCount);
		AddLog(psBuf);
	}
}

// check purge date
void CMySynchGroupTask::PurgeArticles()
{
	if (!mPurgeDate.IsValid())
		return;

	Uint32 nPurgeCount = 0;
	ShowProgress(0, 1, "\pPurging old articles...");
	
	StNewsDatabase pNewsDatabase(mSynchFile2);

	Uint32 i = 0;
	SMyArticleIDsInfo2 *pArticleIDsInfo2;

	while (mArticleIDsTree2.GetNext(pArticleIDsInfo2, i))
	{
		if (mPurgeDate > pArticleIDsInfo2->stArticleDate)
		{
			nPurgeCount++;

			try
			{
				pNewsDatabase->DeleteArticle(pArticleIDsInfo2->nArticleID, false);
			}
			catch (...)
			{
				AddLog("\pError: Purging article failed");
			}
			
			mArticleIDsTree2.RemoveItem(i, false);
			i--;
			
			ClearDataList(pArticleIDsInfo2);	
			UMemory::Dispose((TPtr)pArticleIDsInfo2);
		}
	}
	
	if (nPurgeCount)
	{
		Uint8 psBuf[256];
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "%lu articles purged", nPurgeCount);
		AddLog(psBuf);
	}
}

void CMySynchGroupTask::MarkSynchronized()
{
	Uint32 nSynchronizedCount = 0;
	ShowProgress(0, 1, "\pSearching articles already synchronized...");

	Uint32 i = 0;
	SMyArticleIDsInfo2 *pArticleIDsInfo2;

	while (mArticleIDsTree2.GetNext(pArticleIDsInfo2, i))
	{
		if (pArticleIDsInfo2->bSynchronized)
			continue;
			
		Uint32 j = 0;
		SMyArticleIDsInfo1 *pArticleIDsInfo1;
		while (mArticleIDsTree1.GetNext(pArticleIDsInfo1, j))
		{
			if (pArticleIDsInfo1->bSynchronized)
				continue;
			if (!UMemory::Compare(pArticleIDsInfo1->psArticleID + 1, 
								  pArticleIDsInfo1->psArticleID[0], 
								  pArticleIDsInfo2->psArticleID + 1, 
								  pArticleIDsInfo2->psArticleID[0]))
			{
				nSynchronizedCount++;
				pArticleIDsInfo1->bSynchronized = true;
				pArticleIDsInfo2->bSynchronized = true;
				break;
			}
		}	
	}

	if (nSynchronizedCount)
	{
		Uint8 psBuf[256];
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "%lu articles already synchronized", nSynchronizedCount);
		AddLog(psBuf);
	}
}

Uint32 CMySynchGroupTask::GetPurgeDate(CPtrList<SPurgeInfo>& inPurgeList, SCalendarDate& outPurgeDate)
{
	ClearStruct(outPurgeDate);
	Uint32 nPurgeIndex = 0;
	
	Uint32 i = 0;
	SPurgeInfo *pPurgeInfo;
	
	while (inPurgeList.GetNext(pPurgeInfo, i))
	{
		if (pPurgeInfo->stArticleDate > outPurgeDate)
		{
			nPurgeIndex = i;
			outPurgeDate = pPurgeInfo->stArticleDate;
		}
	}

	return nPurgeIndex;
}

void CMySynchGroupTask::ClearDataList(SMyArticleIDsInfo2 *inArticleIDsInfo)
{
	if (!inArticleIDsInfo)
		return;

	Uint32 i = 0;
	SMyDataInfo2 *pDataInfo;
		
	while (inArticleIDsInfo->lDataList.GetNext(pDataInfo, i))
		UMemory::Dispose((TPtr)pDataInfo);
		
	inArticleIDsInfo->lDataList.Clear();
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMySynchGroupTask1

CMySynchGroupTask1::CMySynchGroupTask1(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, Uint32 inPurgeDays, Uint32 inMaxArticles)
	: CMySynchGroupTask(inServerInfo, inGroupName1, inPurgeDays, inMaxArticles)
{
	mSynchFile2 = inGroupFile->Clone();
	mSynchFile2->GetName(mGroupName2);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "<--- Synchronizing news groups: �%#s� <--- �%#s�", mGroupName1, mGroupName2);
	MakeLog(psBuf);

	bool bIsFolder;
	if (!mSynchFile2->Exists(&bIsFolder) || bIsFolder)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: �%#s� is invalid", mGroupName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		Finish();
		return;
	}	
	
	mSynchCount1 = 0;
	mSynchSize1 = 0;
	mTempSize1 = 0;
		
	try
	{
		SetArticleIDsTree2();
	}
	catch(...)
	{
		// don't throw here
	}
}

CMySynchGroupTask1::CMySynchGroupTask1(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inPurgeDays, Uint32 inMaxArticles)
	: CMySynchGroupTask(inServerInfo, inGroupName1, inPurgeDays, inMaxArticles)
{
	UMemory::Copy(mGroupName2, inGroupName2, inGroupName2[0] + 1);
	mSynchFile2 = UFileSys::New(inGroupFolder, nil, 0, mGroupName2);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "<--- Synchronizing news groups: �%#s� <--- �%#s�", mGroupName1, mGroupName2);
	MakeLog(psBuf);

	bool bIsFolder;
	if (!mSynchFile2->Exists(&bIsFolder) || bIsFolder)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: �%#s� is invalid", mGroupName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		Finish();
		return;
	}	
	
	mSynchCount1 = 0;
	mSynchSize1 = 0;
	mTempSize1 = 0;
	
	try
	{
		SetArticleIDsTree2();
	}
	catch(...)
	{
		// don't throw here
	}
}

void CMySynchGroupTask1::LogSynchronized()
{
	Uint8 psSynchSize[32];
	psSynchSize[0] = UText::SizeToText(mSynchSize1, psSynchSize + 1, sizeof(psSynchSize) - 1, kDontShowBytes);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "<--- %lu articles synchronized (%#s)", mSynchCount1, psSynchSize);
	
	ShowProgress(100, 100, psBuf);
	AddLog(psBuf);
}

void CMySynchGroupTask1::Process()
{
	switch (mStage)
	{
		case 1:
			CMyConnectTask::Process();

			if (mNntpTransact->IsConnected())
			{
				Uint8 psBuf[256];
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Selecting news group �%#s�", mGroupName1);
				ShowProgress(0, 1, psBuf);

				mNntpTransact->Command_SelectGroup(mGroupName1);
				mStage++;
			}
			break;
			
		case 2:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{	
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}
				
				FinishTimerArmed();
				
				ShowProgress(0, 1, "\pListing articles...");

				mNntpTransact->Command_ListArticles();
				mStage++;
			} 
			break;

		case 3:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			bool bExecuted = mNntpTransact->IsCommandExecuted();
			if (SetArticleIDsTree1(bExecuted))
				FinishTimerArmed();

			if (bExecuted)
			{	
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}			
			
				MarkSynchronized();
				
				ShowProgress(0, 1, "\p<--- Starting synchronization...");

				if (!SelectNextArticle2())
				{
					LogSynchronized();
					Finish();
					return;
				}
				
				mNntpTransact->Command_PostArticleStart();						
				mStage++;
			}			
			break;

		case 4:
			if (!mNntpTransact->IsConnected())
			{
				LogSynchronized();
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{
				FinishTimerArmed();
				
				if (mNntpTransact->IsCommandError() || !SynchArticle1(mTempSize1))
				{
					if (mNntpTransact->IsCommandError())
						LogError(false);

					if (!SelectNextArticle2())
					{
						LogSynchronized();
						Finish();
						return;
					}
									
					mNntpTransact->Command_PostArticleStart();
					return;
				}
				
				mNntpTransact->Command_PostArticleFinish();
				mStage++;
			}
			break;
			
		case 5:
			if (!mNntpTransact->IsConnected())
			{
				LogSynchronized();
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{	
				FinishTimerArmed();
				
				if (mNntpTransact->IsCommandError())
					LogError(false);
				else
				{
					mSynchCount1++;
					mSynchSize1 += mTempSize1;
				}

				if (!SelectNextArticle2())
				{
					LogSynchronized();
					Finish();
					return;
				}
				
				mNntpTransact->Command_PostArticleStart();
				mStage = 4;
			}				
			break;
	};
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMySynchGroupTask2

CMySynchGroupTask2::CMySynchGroupTask2(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, Uint32 inPurgeDays, Uint32 inMaxArticles)
	: CMySynchGroupTask(inServerInfo, inGroupName1, inPurgeDays, inMaxArticles)
{
	mSynchFile2 = inGroupFile->Clone();
	mSynchFile2->GetName(mGroupName2);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
					"---> Synchronizing news groups: �%#s� ---> �%#s�", 
					mGroupName1, mGroupName2);
	MakeLog(psBuf);

	bool bIsFolder;
	if (mSynchFile2->Exists(&bIsFolder) && bIsFolder)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: �%#s� is invalid", mGroupName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		Finish();
		return;		
	}
	
	mSynchCount2 = 0;
	mSynchSize2 = 0;
	
	try
	{
		SetArticleIDsTree2();
	}
	catch(...)
	{
		// don't throw here
	}
}

CMySynchGroupTask2::CMySynchGroupTask2(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inPurgeDays, Uint32 inMaxArticles)
	: CMySynchGroupTask(inServerInfo, inGroupName1, inPurgeDays, inMaxArticles)
{
	if (inGroupName2)
	{
		UMemory::Copy(mGroupName2, inGroupName2, inGroupName2[0] + 1);
	}
	else
	{
		UMemory::Copy(mGroupName2, mGroupName1, mGroupName1[0] + 1);
	#if WIN32
		// add .hnz extension on PC
		mGroupName2[0] += UMemory::Copy(mGroupName2 + mGroupName2[0] + 1, ".hnz", 4);
	#else
		// max 31 chars for file name on MAC
		if (mGroupName2[0] > 31) mGroupName2[0] = 31;
	#endif
	}

	mSynchFile2 = UFileSys::New(inGroupFolder, nil, 0, mGroupName2);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
					"---> Synchronizing news groups: �%#s� ---> �%#s�", 
					mGroupName1, mGroupName2);
	MakeLog(psBuf);

	bool bIsFolder;
	if (mSynchFile2->Exists(&bIsFolder) && bIsFolder)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: �%#s� is invalid", mGroupName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		Finish();
		return;
	}
	
	mSynchCount2 = 0;
	mSynchSize2 = 0;
	
	try
	{
		SetArticleIDsTree2();
	}
	catch(...)
	{
		// don't throw here
	}
}

void CMySynchGroupTask2::LogSynchronized()
{
	Uint8 psSynchSize[32];
	psSynchSize[0] = UText::SizeToText(mSynchSize2, psSynchSize + 1, sizeof(psSynchSize) - 1, kDontShowBytes);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
					"---> %lu articles synchronized (%#s)", 
					mSynchCount2, psSynchSize);
	
	ShowProgress(100, 100, psBuf);
	AddLog(psBuf);
}

void CMySynchGroupTask2::Process()
{
	switch (mStage)
	{
		case 1:
			CMyConnectTask::Process();

			if (mNntpTransact->IsConnected())
			{
				Uint8 psBuf[256];
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Selecting news group �%#s�", mGroupName1);
				ShowProgress(0, 1, psBuf);

				mNntpTransact->Command_SelectGroup(mGroupName1);
				mStage++;
			}
			break;
			
		case 2:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{	
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}

				FinishTimerArmed();
			
				ShowProgress(0, 1, "\pListing articles...");

				mNntpTransact->Command_ListArticles();
				mStage++;
			} 
			break;

		case 3:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			bool bExecuted = mNntpTransact->IsCommandExecuted();
			if (SetArticleIDsTree1(bExecuted))
				FinishTimerArmed();

			if (bExecuted)
			{	
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}			
			
				MarkSynchronized();
				MakeMaxArticles();

				ShowProgress(0, 1, "\p---> Starting synchronization...");

				if (!SelectNextArticle1() || !CreateGroup2())
				{
					LogSynchronized();
					Finish();
					return;
				}
				
				mNntpTransact->Command_GetArticle(mArticleID1);
				mStage++;
			}			
			break;

		case 4:
			if (mNntpTransact->IsCommandExecuted())
			{	
				FinishTimerArmed();
				
				Uint32 nTempSize2;
				if (mNntpTransact->IsCommandError())
					LogError(false);
				else if (SynchArticle2(nTempSize2))
				{
					mSynchCount2++;
					mSynchSize2 += nTempSize2;
				}
				
				if (!mNntpTransact->IsConnected())
				{
					LogSynchronized();
					LogClosed();
					Finish();
					return;
				}

				if (!SelectNextArticle1())
				{
					LogSynchronized();
					Finish();
					return;
				}
					
				mNntpTransact->Command_GetArticle(mArticleID1);
			}
			
			if (!mNntpTransact->IsConnected())
			{
				LogSynchronized();
				LogClosed();
				Finish();
			}	
			break;
	};
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMySynchGroupTask21

CMySynchGroupTask21::CMySynchGroupTask21(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, Uint32 inPurgeDays, Uint32 inMaxArticles)
	: CMySynchGroupTask(inServerInfo, inGroupName1, inPurgeDays, inMaxArticles)
{
	mSynchFile2 = inGroupFile->Clone();
	mSynchFile2->GetName(mGroupName2);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
					"<--> Synchronizing news groups: �%#s� <--> �%#s�", 
					mGroupName1, mGroupName2);
	MakeLog(psBuf);

	bool bIsFolder;
	if (mSynchFile2->Exists(&bIsFolder) && bIsFolder)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: �%#s� is invalid", mGroupName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		Finish();
		return;
	}	
	
	mSynchCount1 = 0;
	mSynchSize1 = 0;
	mTempSize1 = 0;
	
	mSynchCount2 = 0;
	mSynchSize2 = 0;
	
	try
	{
		SetArticleIDsTree2();
	}
	catch(...)
	{
		// don't throw here
	}
}

CMySynchGroupTask21::CMySynchGroupTask21(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inPurgeDays, Uint32 inMaxArticles)
	: CMySynchGroupTask(inServerInfo, inGroupName1, inPurgeDays, inMaxArticles)
{
	if (inGroupName2)
		UMemory::Copy(mGroupName2, inGroupName2, inGroupName2[0] + 1);
	else
	{
		UMemory::Copy(mGroupName2, mGroupName1, mGroupName1[0] + 1);
	#if WIN32
		// add .hnz extension on PC
		mGroupName2[0] += UMemory::Copy(mGroupName2 + mGroupName2[0] + 1, ".hnz", 4);
	#else
		// max 31 chars for file name on MAC
		if (mGroupName2[0] > 31) mGroupName2[0] = 31;
	#endif
	}
	
	mSynchFile2 = UFileSys::New(inGroupFolder, nil, 0, mGroupName2);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
						"<--> Synchronizing news groups: �%#s� <--> �%#s�", 
						mGroupName1, mGroupName2);
	MakeLog(psBuf);

	bool bIsFolder;
	if (mSynchFile2->Exists(&bIsFolder) && bIsFolder)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: �%#s� is invalid", mGroupName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		Finish();
		return;
	}	
	
	mSynchCount1 = 0;
	mSynchSize1 = 0;
	mTempSize1 = 0;
	
	mSynchCount2 = 0;
	mSynchSize2 = 0;
	
	try
	{
		SetArticleIDsTree2();
	}
	catch(...)
	{
		// don't throw here
	}
}

void CMySynchGroupTask21::LogSynchronized1()
{
	Uint8 psSynchSize[32];
	psSynchSize[0] = UText::SizeToText(mSynchSize1, psSynchSize + 1, sizeof(psSynchSize) - 1, kDontShowBytes);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "<--- %lu articles synchronized (%#s)", mSynchCount1, psSynchSize);
	
	ShowProgress(100, 100, psBuf);
	AddLog(psBuf);
}

void CMySynchGroupTask21::LogSynchronized2()
{
	Uint8 psSynchSize[32];
	psSynchSize[0] = UText::SizeToText(mSynchSize2, psSynchSize + 1, sizeof(psSynchSize) - 1, kDontShowBytes);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
						"---> %lu articles synchronized (%#s)", 
						mSynchCount2, psSynchSize);
	
	ShowProgress(100, 100, psBuf);
	AddLog(psBuf);
}

void CMySynchGroupTask21::Process()
{
goFromStart:

	switch (mStage)
	{
		case 1:
			CMyConnectTask::Process();

			if (mNntpTransact->IsConnected())
			{
				Uint8 psBuf[256];
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Selecting news group �%#s�", mGroupName1);
				ShowProgress(0, 1, psBuf);

				mNntpTransact->Command_SelectGroup(mGroupName1);
				mStage++;
			}
			break;
			
		case 2:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{	
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}

				FinishTimerArmed();
				
				ShowProgress(0, 1, "\pListing articles...");

				mNntpTransact->Command_ListArticles();
				mStage++;
			} 
			break;

		case 3:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			bool bExecuted = mNntpTransact->IsCommandExecuted();
			if (SetArticleIDsTree1(bExecuted))
				FinishTimerArmed();

			if (bExecuted)
			{	
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}			
			
				MarkSynchronized();
				MakeMaxArticles();

				ShowProgress(0, 1, "\p---> Starting synchronization...");

				if (!CreateGroup2())
				{
					LogSynchronized1();
					LogSynchronized2();
					Finish();
					return;
				}

				if (!SelectNextArticle1())
				{
					LogSynchronized2();
					
					mStage = 5;
					goto goFromStart;
				}
				
				mNntpTransact->Command_GetArticle(mArticleID1);
				mStage++;
			}			
			break;

		case 4:
			if (mNntpTransact->IsCommandExecuted())
			{	
				FinishTimerArmed();
				
				Uint32 nTempSize2;
				if (mNntpTransact->IsCommandError())
					LogError(false);
				else if (SynchArticle2(nTempSize2))
				{
					mSynchCount2++;
					mSynchSize2 += nTempSize2;
				}
				
				if (!mNntpTransact->IsConnected())
				{
					LogSynchronized1();
					LogSynchronized2();
					LogClosed();
					Finish();
					return;
				}
				
				if (!SelectNextArticle1())
				{
					LogSynchronized2();
					
					mStage = 5;
					goto goFromStart;
				}
					
				mNntpTransact->Command_GetArticle(mArticleID1);
			}
			
			if (!mNntpTransact->IsConnected())
			{
				LogSynchronized1();
				LogSynchronized2();
				LogClosed();
				Finish();
			}	
			break;

		case 5:
			if (!mNntpTransact->IsConnected())
			{
				LogSynchronized1();
				LogClosed();
				Finish();
				return;
			}

			FinishTimerArmed();
			
			ShowProgress(0, 1, "\p<--- Starting synchronization...");

			if (!SelectNextArticle2())
			{
				LogSynchronized1();
				Finish();
				return;
			}
				
			mNntpTransact->Command_PostArticleStart();						
			mStage++;			
			break;

		case 6:
			if (!mNntpTransact->IsConnected())
			{
				LogSynchronized1();
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{
				FinishTimerArmed();
				
				if (mNntpTransact->IsCommandError() || !SynchArticle1(mTempSize1))
				{
					if (mNntpTransact->IsCommandError())
						LogError(false);

					if (!SelectNextArticle2())
					{
						LogSynchronized1();
						Finish();
						return;
					}
									
					mNntpTransact->Command_PostArticleStart();
					return;
				}
				
				mNntpTransact->Command_PostArticleFinish();
				mStage++;
			}
			break;
			
		case 7:
			if (!mNntpTransact->IsConnected())
			{
				LogSynchronized1();
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{	
				FinishTimerArmed();
				
				if (mNntpTransact->IsCommandError())
					LogError(false);
				else
				{
					mSynchCount1++;
					mSynchSize1 += mTempSize1;
				}

				if (!SelectNextArticle2())
				{
					LogSynchronized1();
					Finish();
					return;
				}
				
				mNntpTransact->Command_PostArticleStart();
				mStage = 6;
			}				
			break;
	};
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMySynchArticleTask1

CMySynchArticleTask1::CMySynchArticleTask1(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, CPtrList<Uint8>& inParentIDs1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inArticleID2, const Uint8 *inArticleName2, CPtrList<SMyDataInfo2>& inDataList2)
	: CMyConnectTask(inServerInfo)
{
	UMemory::Copy(mGroupName1, inGroupName1, inGroupName1[0] + 1);
	mSynchFile2 = UFileSys::New(inGroupFolder, nil, 0, inGroupName2);

	mArticleID2 = inArticleID2;
	UMemory::Copy(mArticleName2, inArticleName2, inArticleName2[0] + 1);	

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "<--- Synchronizing article �%#s�", mArticleName2);
	MakeLog(psBuf);

	bool bIsFolder;
	if (!mSynchFile2->Exists(&bIsFolder) || bIsFolder)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: �%#s� is invalid", inGroupName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		Finish();
		return;
	}	
	
	Uint32 i = 0;
	Uint8 *pParentID;
	
	while (inParentIDs1.GetNext(pParentID, i))
		mParentIDs1.AddItem(pParentID);	
		
	i = 0;
	SMyDataInfo2 *pDataInfo;
	
	while (inDataList2.GetNext(pDataInfo, i))
	{
		SMyDataInfo2 *pNewDataInfo = (SMyDataInfo2 *)UMemory::NewClear(sizeof(SMyDataInfo2));
		
		UMemory::Copy(pNewDataInfo->psName, pDataInfo->psName, pDataInfo->psName[0] + 1);
		UMemory::Copy(pNewDataInfo->csFlavor, pDataInfo->csFlavor, sizeof(pDataInfo->csFlavor) + 1);
		
		mDataList2.AddItem(pNewDataInfo);
	}
	
	mSynchSize1 = 0;
}

CMySynchArticleTask1::~CMySynchArticleTask1()
{
	UFileSys::Dispose(mSynchFile2);

	Uint32 i = 0;
	Uint8 *pParentID;
	
	while (mParentIDs1.GetNext(pParentID, i))
		UMemory::Dispose((TPtr)pParentID);
		
	mParentIDs1.Clear();

	i = 0;
	SMyDataInfo2 *pDataInfo;
	
	while (mDataList2.GetNext(pDataInfo, i))
		UMemory::Dispose((TPtr)pDataInfo);

	mDataList2.Clear();	
}

bool CMySynchArticleTask1::IsSynchArticleTask1(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inArticleID2)
{
	if (!IsConnectTask(inServerAddr))
		return false;
		
	if (!inGroupName1 || UText::CompareInsensitive(mGroupName1 + 1, mGroupName1[0], inGroupName1 + 1, inGroupName1[0]))
		return false;
		
	TFSRefObj* pSynchFile2 = UFileSys::New(inGroupFolder, nil, 0, inGroupName2);
	scopekill(TFSRefObj, pSynchFile2);
	
	if (!UFileSys::Equals(mSynchFile2, pSynchFile2))
		return false;
	
	if (mArticleID2 != inArticleID2)
		return false;
		
	return true;
}

void CMySynchArticleTask1::LogSynchronized()
{
	Uint8 psSynchSize[32];
	psSynchSize[0] = UText::SizeToText(mSynchSize1, psSynchSize + 1, sizeof(psSynchSize) - 1, kDontShowBytes);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Article synchronized (%#s)", psSynchSize);
	
	ShowProgress(100, 100, psBuf);
	AddLog(psBuf);
}

void CMySynchArticleTask1::Process()
{
	Uint8 psBuf[256];

	switch (mStage)
	{
		case 1:
			if (mNntpTransact->IsConnected())
			{
				CMyConnectTask::Process();

				if (!mNntpTransact->IsPostingAllowed())
				{
					Uint8 *pErrorMsg = "\pError: Posting not allowed";
					ShowProgress(100, 100, pErrorMsg);
					AddLog(pErrorMsg);
					
					Finish();
					return;
				}

				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Selecting news group �%#s�", mGroupName1);
				ShowProgress(1, 4, psBuf);

				mNntpTransact->Command_SelectGroup(mGroupName1);
				mStage++;
			}
			break;
			
		case 2:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{					
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}
			
				if (!mNntpTransact->IsGroupPostingAllowed())
				{
					Uint8 *pErrorMsg = "\pError: Posting not allowed";
					ShowProgress(100, 100, pErrorMsg);
					AddLog(pErrorMsg);
					
					Finish();
					return;
				}
					
				FinishTimerArmed();
				
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "<--- Start posting article �%#s�", mArticleName2);
				ShowProgress(2, 4, psBuf);

				mNntpTransact->Command_PostArticleStart();
				mStage++;
			} 
			break;

		case 3:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{	
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}
				
				if (!SynchArticle1())
				{
					Finish();
					return;
				}

				FinishTimerArmed();
				
				mNntpTransact->Command_PostArticleFinish();
				mStage++;
			}
			break;

		case 4:
			if (mNntpTransact->IsCommandExecuted())
			{	
				if (mNntpTransact->IsCommandError())
					LogError();
				else
					LogSynchronized();
					
				Finish();
				return;
			}

			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
			}
			break;
	};
}

bool CMySynchArticleTask1::SynchArticle1()
{
	Uint8 psBuf[256];
	
	SMyNewsArticleInfo stArticleInfo;
	try
	{
		StNewsDatabase pNewsDatabase(mSynchFile2);
		pNewsDatabase->GetArticleInfo(mArticleID2, stArticleInfo);

		if (!stArticleInfo.externId[0])	// 1.7.2 servers don't generate extern ID
			stArticleInfo.externId[0] = pNewsDatabase->GenerateExternID(mArticleID2, stArticleInfo.externId + 1, sizeof(stArticleInfo.externId) - 1);
	}
	catch(...)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: Article �%#s� corrupt", mArticleName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		return false;
	}

	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "<--- Posting article �%#s�", mArticleName2);
	ShowProgress(3, 4, psBuf);

	// add parent ID's
	Uint32 i = 0;
	Uint8 *pParentID;
		
	while (mParentIDs1.GetNext(pParentID, i))
		mNntpTransact->AddParentID(pParentID);

	// add misc info
	mNntpTransact->SetArticleID(stArticleInfo.externId);
	mNntpTransact->SetOrganization(gApp->GetOrganization());
	mNntpTransact->SetNewsReader(APPLICATION_NAME);
	
	SCalendarDate stPostDate = (SCalendarDate)stArticleInfo.dts;
	mNntpTransact->SetPostDate(stPostDate);

	mNntpTransact->SetPostInfo(stArticleInfo.title, stArticleInfo.poster, gApp->GetEmailAddress());

	StNewsDatabase pNewsDatabase(mSynchFile2);
		
	// add data
	i = 0;
	SMyDataInfo2 *pDataInfo;
	
	while (mDataList2.GetNext(pDataInfo, i))
	{
		try
		{
			Uint32 nDataSize;
			TPtr pData = pNewsDatabase->GetArticleData(mArticleID2, pDataInfo->psName, nDataSize);
			if (!pData)
			{
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: Article �%#s� corrupt", mArticleName2);
				ShowProgress(100, 100, psBuf);
				AddLog(psBuf);

				return false;
			}
	
			mNntpTransact->AddPostData(pDataInfo->csFlavor, pDataInfo->psName, pData, nDataSize); // take ownership of pData
			mSynchSize1 += nDataSize;
		}
		catch(...)
		{
			psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: Article �%#s� corrupt", mArticleName2);
			ShowProgress(100, 100, psBuf);
			AddLog(psBuf);

			return false;
		}
	}
	
	// add text
	Uint32 nTextSize;
	void *pAddText = gApp->GetAddArticleText(nTextSize);
	if (pAddText)
	{
		mNntpTransact->AddPostData("text/plain", "\padd text", pAddText, nTextSize); // take ownership of pAddText
		mSynchSize1 += nTextSize;
	}

	return true;
}

/* ������������������������������������������������������������������������� */
#pragma mark -
#pragma mark CMySynchArticleTask2

CMySynchArticleTask2::CMySynchArticleTask2(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, Uint32 inArticleID1, const Uint8 *inArticleName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inParentID2)
	: CMyConnectTask(inServerInfo)
{
	UMemory::Copy(mGroupName1, inGroupName1, inGroupName1[0] + 1);
	mSynchFile2 = UFileSys::New(inGroupFolder, nil, 0, inGroupName2);

	mArticleID1 = inArticleID1;
	UMemory::Copy(mArticleName1, inArticleName1, inArticleName1[0] + 1);
	mParentID2 = inParentID2;	

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
						"---> Synchronizing article �%#s�", 
						mArticleName1);
	MakeLog(psBuf);

	bool bIsFolder;
	if (!mSynchFile2->Exists(&bIsFolder) || bIsFolder)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: �%#s� is invalid", inGroupName2);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);

		Finish();
		return;
	}	

	mSynchSize2 = 0;
}

CMySynchArticleTask2::~CMySynchArticleTask2()
{
	UFileSys::Dispose(mSynchFile2);
}

bool CMySynchArticleTask2::IsSynchArticleTask2(const Uint8 *inServerAddr, const Uint8 *inGroupName1, Uint32 inArticleID1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2)
{
	if (!IsConnectTask(inServerAddr))
		return false;
		
	if (!inGroupName1 || UText::CompareInsensitive(mGroupName1 + 1, mGroupName1[0], inGroupName1 + 1, inGroupName1[0]))
		return false;
		
	if (mArticleID1 != inArticleID1)
		return false;

	TFSRefObj* pSynchFile2 = UFileSys::New(inGroupFolder, nil, 0, inGroupName2);
	scopekill(TFSRefObj, pSynchFile2);
	
	if (!UFileSys::Equals(mSynchFile2, pSynchFile2))
		return false;
			
	return true;
}

void CMySynchArticleTask2::LogSynchronized()
{
	Uint8 psSynchSize[32];
	psSynchSize[0] = UText::SizeToText(mSynchSize2, psSynchSize + 1, sizeof(psSynchSize) - 1, kDontShowBytes);

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Article synchronized (%#s)", psSynchSize);
	
	ShowProgress(100, 100, psBuf);
	AddLog(psBuf);
}

void CMySynchArticleTask2::Process()
{
	Uint8 psBuf[256];

	switch (mStage)
	{
		case 1:
			CMyConnectTask::Process();

			if (mNntpTransact->IsConnected())
			{
				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Selecting news group �%#s�", mGroupName1);
				ShowProgress(1, 3, psBuf);

				mNntpTransact->Command_SelectGroup(mGroupName1);
				mStage++;
			}
			break;
			
		case 2:
			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
				return;
			}

			if (mNntpTransact->IsCommandExecuted())
			{					
				if (mNntpTransact->IsCommandError())
				{
					LogError();
					Finish();
					return;
				}
				
				FinishTimerArmed();

				psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
									"---> Start posting article �%#s�", 
									mArticleName1);
				ShowProgress(2, 3, psBuf);

				mNntpTransact->Command_GetArticle(mArticleID1);
				mStage++;
			} 
			break;

		case 3:
			if (mNntpTransact->IsCommandExecuted())
			{	
				FinishTimerArmed();
				
				if (mNntpTransact->IsCommandError())
					LogError();
				else if (SynchArticle2())
					LogSynchronized();
				else
				{
					Uint8 *pErrorMsg = "\pError: Article corrupt";
					ShowProgress(100, 100, pErrorMsg);
					AddLog(pErrorMsg);
				}
					
				Finish();
				return;
			}

			if (!mNntpTransact->IsConnected())
			{
				LogClosed();
				Finish();
			}
			break;
	};
}

bool CMySynchArticleTask2::SynchArticle2()
{
	SNewsArticleInfo stArticleInfo;
	try
	{
		if (!mNntpTransact->GetArticleInfo(stArticleInfo))
			return false;
	}
	catch(...)
	{
		return false;	// don't throw here
	}

	if (!mNntpTransact->GetDataCount())
		return false;

	Uint8 psBuf[256];
	psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, 
						"---> Posting article �%#s�", 
						mArticleName1);
	ShowProgress(3, 4, psBuf);

	SDateTimeStamp stArticleDate = (SDateTimeStamp)stArticleInfo.stArticleDate;
	
	Uint8 *pPosterName = stArticleInfo.psPosterName;
	if (!pPosterName[0])
		pPosterName = stArticleInfo.psPosterAddr;

	Uint32 nArticleID;
	try
	{
		StNewsDatabase pNewsDatabase(mSynchFile2);
		nArticleID = pNewsDatabase->AddArticle(mParentID2, stArticleInfo.psArticleName, pPosterName, 0, &stArticleDate, stArticleInfo.psArticleID);
		if (!nArticleID)
		{
			psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: Article �%#s� corrupt", stArticleInfo.psArticleName);
			ShowProgress(100, 100, psBuf);
			AddLog(psBuf);

			return false;
		}
	}
	catch (...)
	{
		psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: Article �%#s� corrupt", stArticleInfo.psArticleName);
		ShowProgress(100, 100, psBuf);
		AddLog(psBuf);
	
		return false;
	}

	StNewsDatabase pNewsDatabase(mSynchFile2);
	
	Uint32 i = 0;
	SArticleData *pArticleData;
	
	while (mNntpTransact->GetNextData(pArticleData, i))
	{	
		try
		{
			if (pNewsDatabase->AddArticleData(nArticleID, pArticleData->psName, pArticleData->csFlavor, (Uint8 *)pArticleData->pData, pArticleData->nDataSize))
				mSynchSize2 += pArticleData->nDataSize;
		}
		catch (...)
		{
			psBuf[0] = UText::Format(psBuf + 1, sizeof(psBuf) - 1, "Error: Article �%#s� corrupt", stArticleInfo.psArticleName);
			ShowProgress(100, 100, psBuf);
			AddLog(psBuf);
		
			return false;
		}
	}
	
	gApp->DoListArticles2();
	return true;
}

