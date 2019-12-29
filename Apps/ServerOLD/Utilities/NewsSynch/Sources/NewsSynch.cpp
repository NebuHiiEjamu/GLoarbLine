/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "NewsSynch.h"
#include "CNewsDatabase.h"

CMyApplication *gApp = nil;

void main()
{
	UOperatingSystem::Init();
				
	try
	{		
		UTransport::Init();
		
		if (!UTransport::HasTCP())
			Fail(errorType_Transport, transError_NeedTCP);

		gApp = new CMyApplication;
		gApp->StartUp();
		gApp->Run();
		delete gApp;
	}
	catch(SError& err)
	{
		UError::Alert(err);
	}
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyApplication

CMyApplication::CMyApplication()
{
	mSynchWin = nil;
	
	ClearStruct(mServerInfo);
	ClearStruct(mGroupInfo);
	mBundleRef = nil;
		
	mSynchTimer = NewTimer();
	mFinishTaskTimer = NewTimer();
}

CMyApplication::~CMyApplication()
{
	UFileSys::Dispose(mBundleRef);
	
	UTimer::Dispose(mSynchTimer);
	UTimer::Dispose(mFinishTaskTimer);
}

void CMyApplication::StartUp()
{
#if WIN32
	try { URez::AddProgramFileToSearchChain("\pNewsSynch.dat"); } catch(...){ }
#endif

	mSynchWin = new CMySynchWin();
	UWindow::SetMainWindow(*mSynchWin);

	Uint8 nTabPercent[17];
	SRect stWinRect, stViewsRect[14];
	
	if (ReadPrefs(stWinRect, stViewsRect, nTabPercent))
	{
		mSynchWin->SetBounds(stWinRect);
		mSynchWin->SetInternalBounds(stViewsRect, nTabPercent);
		mSynchWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
	}

	mSynchWin->UpdateSortBtn1(mOptions.nOpts & myOpt_SortDescending1);
	mSynchWin->UpdateSortBtn2(mOptions.nOpts & myOpt_SortDescending2);
	mSynchWin->Show();

	ReadSynchList();
	MakeAutoSynch();
}

void CMyApplication::WindowHit(CWindow *inWindow, const SHitMsgData& inInfo)
{
	#pragma unused(inWindow)

	switch (inInfo.id)
	{
		case viewID_Refresh:
			DoRefresh();
			break;
			
		case viewID_AddSynch:
			DoAddSynch();
			break;

		case viewID_Synch:
			DoSynch();
			break;

		case viewID_Options:
			DoOptions();
			break;
		
		case viewID_StopTask:
			DoStopTask();
			break;
		
		case viewID_SelectServer1:
			DoSelectServer1();
			break;

		case viewID_SelectServer2:
			DoSelectServer2();
			break;
			
		case viewID_ServerAddr1:
			DoListGroups1();
			break;
		
		case viewID_SelectBundle2:
			DoSelectBundle2();
			break;
				
		case viewID_GroupFilter1:
			DoGroupFilter1(inInfo);
			break;

		case viewID_GroupFilter2:
			DoGroupFilter2(inInfo);
			break;

		case viewID_SortArticles1:
			SortArticles1();
			break;

		case viewID_SortArticles2:
			SortArticles2();
			break;
		
		case viewID_Attachment1:
			SaveAttachment1();
			break;	
			
		case viewID_Attachment2:
			SaveAttachment2();
			break;

		// close box
		case CWindow::ClassID:
			if (inWindow == mSynchWin)
				UserQuit();
			break;
	};
}

void CMyApplication::HandleMessage(void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	switch (inMsg)
	{					
		case msg_DataTimeOut:
		case msg_ConnectionClosed:
		case msg_ConnectionEstablished:
		case msg_ConnectionRefused:

		case msg_NntpError:
		case msg_NntpListGroups:
		case msg_NntpSelectGroup:
		case msg_NntpListArticles:
		case msg_NntpSelectArticle:
		case msg_NntpGetArticle:
		case msg_NntpGetArticleHeader:
		case msg_NntpGetArticleData:
		case msg_NntpExistsArticle:
		case msg_NntpPostArticle:
			ProcessTasks();
	
		default:
			CApplication::HandleMessage(inObject, inMsg, inData, inDataSize);
			break;
	};
}

void CMyApplication::Timer(TTimer inTimer)
{
	if (inTimer == mFinishTaskTimer)
		KillFinishedTasks();
	else if (inTimer == mSynchTimer)
		MakeAutoSynch();
}

void CMyApplication::UserQuit()
{	
	WritePrefs();
	UMemory::Dispose((TPtr)mOptions.pAddArticleText);
	
	KillAllTasks();
	ClearSynchList();
	ClearServerList1();
	ClearServerList2();

	CApplication::UserQuit();
}

void CMyApplication::ProcessTasks()
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
		
	while (mTaskList.GetNext(pTask, i))
	{
		if (!pTask->IsFinished())
		{
			try
			{
				pTask->Process();				
			}
			catch(...)
			{
				delete pTask;					
				throw;
			}
		}
	}
}

#pragma mark -

void CMyApplication::DoRefresh()
{
	if (mSynchWin->GetGroupListView1()->IsFocus())
		DoListArticles1();
	else if(mSynchWin->GetArticleTreeView1()->IsFocus())
		DoGetArticle1();
	else if (mSynchWin->GetGroupListView2()->IsFocus())
		DoListArticles2();
	else if (mSynchWin->GetArticleTreeView2()->IsFocus())
		DoOpenArticle2();
}

void CMyApplication::DoAddSynch()
{
	SMySynchInfo stSynchInfo;
	ClearStruct(stSynchInfo);

	stSynchInfo.stServerInfo.psServerAddr[0] = mSynchWin->GetServerAddr(stSynchInfo.stServerInfo.psServerAddr + 1, sizeof(stSynchInfo.stServerInfo.psServerAddr) - 1);
	stSynchInfo.stServerInfo.psUserLogin[0] = mSynchWin->GetUserLogin(stSynchInfo.stServerInfo.psUserLogin + 1, sizeof(stSynchInfo.stServerInfo.psUserLogin) - 1);
	stSynchInfo.stServerInfo.psUserPassword[0] = mSynchWin->GetUserPassword(stSynchInfo.stServerInfo.psUserPassword + 1, sizeof(stSynchInfo.stServerInfo.psUserPassword) - 1);

	SMyGroupInfo1 *pGroupInfo1;
	if (mSynchWin->GetGroupListView1()->GetSelectedItem(&pGroupInfo1))
		UMemory::Copy(stSynchInfo.psGroupName, pGroupInfo1->stGroupInfo.psGroupName, pGroupInfo1->stGroupInfo.psGroupName[0] + 1);
	
	if (mBundleRef)
	{
		SMyGroupInfo2 *pGroupInfo2;
		if (mSynchWin->GetGroupListView2()->GetSelectedItem(&pGroupInfo2))
			stSynchInfo.pHotlineFile = UFileSys::New(mBundleRef, nil, pGroupInfo2->psGroupName, fsOption_PreferExistingFile);
		else if (stSynchInfo.psGroupName[0])
		{
			Uint8 psGroupName[256];
			UMemory::Copy(psGroupName, stSynchInfo.psGroupName, stSynchInfo.psGroupName[0] + 1);
		#if WIN32
			// add .hnz extension on PC
			psGroupName[0] += UMemory::Copy(psGroupName + psGroupName[0] + 1, ".hnz", 4);
		#else
			// max 31 chars for file name on MAC
			if (psGroupName[0] > 31) psGroupName[0] = 31;
		#endif

			stSynchInfo.pHotlineFile = UFileSys::New(mBundleRef, nil, psGroupName);
		}
	}
	
	if (stSynchInfo.pHotlineFile)
		stSynchInfo.psHotlineServer[0] = GetServerNameGroup(stSynchInfo.pHotlineFile, stSynchInfo.psHotlineServer + 1, sizeof(stSynchInfo.psHotlineServer) - 1);

	DoOptions(&stSynchInfo);
	
	if (stSynchInfo.pHotlineFile)
		UFileSys::Dispose(stSynchInfo.pHotlineFile);
}

void CMyApplication::DoSynch()
{
	if (!mServerInfo.psServerAddr[0] || !mBundleRef)
		return;

	SMyGroupInfo1 *pGroupInfo1;
	if (!mSynchWin->GetGroupListView1()->GetSelectedItem(&pGroupInfo1))
		return;
	
	SMyGroupInfo2 *pGroupInfo2;
	if (!mSynchWin->GetGroupListView2()->GetSelectedItem(&pGroupInfo2))
		return;

	if (IsSynchGroupTask21(mServerInfo.psServerAddr, pGroupInfo1->stGroupInfo.psGroupName, mBundleRef, pGroupInfo2->psGroupName))
		return;
	
	new CMySynchGroupTask21(mServerInfo, pGroupInfo1->stGroupInfo.psGroupName, mBundleRef, pGroupInfo2->psGroupName, mOptions.nPurgeDays, mOptions.nMaxArticles);
}

bool CMyApplication::DoOptions(const SMySynchInfo *inSynchInfo)
{
	Uint32 id;
	CMyOptionsWin *win = new CMyOptionsWin();
	
	CPtrList<SMySynchInfo> stSynchList;
	ReadSynchInfo(&stSynchList);

	Uint8 psNewsServer[64];
	psNewsServer[0] = mSynchWin->GetServerAddr(psNewsServer + 1, sizeof(psNewsServer) - 1);
	
	Uint8 psHotlineServer[32];
	psHotlineServer[0] = mSynchWin->GetServerNameView()->GetText(psHotlineServer + 1, sizeof(psHotlineServer) - 1);
	
	win->SetInfo(mOptions, &stSynchList, psNewsServer, psHotlineServer);
	win->Show();
	if (inSynchInfo)
		win->AddSynch(inSynchInfo);
	
	// go into modal loop to process modal options window
	for(;;)
	{
		win->ProcessModal();
		id = win->GetHitID();
		
		if (id == 1)		// save
		{
			win->GetInfo(mOptions);
			delete win;
													
			WritePrefs();
			WriteSynchInfo(&stSynchList);
			
			SetServerName2();
			MakeAutoSynch();
			
			return true;
		}
		else if (id == 2)	// cancel
		{
			delete win;
			ClearSynchList(&stSynchList);
			
			return false;
		}
		else if(id == 3)
			win->AddSynch();
		else if(id == 4)
			win->EditSynch();
		else if(id == 5)
			win->DeleteSynch();
		else if(id == 6)
			win->MakeSynch();
		else if(id == 7)
			win->FilterList();
		else if(id == 8 || id == 9)
			win->RefreshList();
	}
}

void CMyApplication::DoStopTask()
{
	CMyTask *pTask = mSynchWin->GetTaskListView()->GetSelectedTask();
	
	if (pTask && !pTask->IsFinished())
	{
		((CMyConnectTask *)pTask)->LogStop();
		delete pTask;
	}
}

bool CMyApplication::UseOldNewsFormat()
{
#if NEW_NEWS_FORMAT
	return (mOptions.nOpts & myOpt_OldNewsFormat);
#else
	return true;
#endif
}

Uint8 *CMyApplication::GetEmailAddress()
{
	return mOptions.psEmailAddress;
}

Uint8 *CMyApplication::GetOrganization()
{
	return mOptions.psOrganization;
}

void *CMyApplication::GetAddArticleText(Uint32& outSize)
{
	outSize = mOptions.nArticleTextSize;
	if (!outSize)
		return nil;
	
	return UMemory::New(mOptions.pAddArticleText, mOptions.nArticleTextSize);
}

Uint32 CMyApplication::GetServerNameBundle(TFSRefObj* inBundleRef, Uint8 *outServerName, Uint32 inMaxSize)
{
	if (!inBundleRef || !outServerName || !inMaxSize)
		return 0;

	Uint8 psServerName[32];
	psServerName[0] = SearchServerList2(inBundleRef, psServerName + 1, sizeof(psServerName) - 1);
	
	if (psServerName[0])
		return UMemory::Copy(outServerName, psServerName + 1, psServerName[0] > inMaxSize ? inMaxSize : psServerName[0]);
	
	return mSynchWin->GetServerNameView()->GetText(outServerName, inMaxSize);
}

Uint32 CMyApplication::GetServerNameGroup(TFSRefObj* inGroupFile, Uint8 *outServerName, Uint32 inMaxSize)
{
	if (!inGroupFile || !outServerName || !inMaxSize)
		return 0;

	TFSRefObj* pBundleRef = inGroupFile->GetFolder();
	if (!pBundleRef)
		return 0;
	
	scopekill(TFSRefObj, pBundleRef);

	return GetServerNameBundle(pBundleRef, outServerName, inMaxSize);
}

#pragma mark -

void CMyApplication::DoSelectServer1()
{
	Uint32 id;
	CMySelectServerWin1 *win = new CMySelectServerWin1();

	win->GetServerListView()->SetItemsFromList(&mServerList1);
	win->Show();
	
	// go into modal loop to process modal this window
	for(;;)
	{
		win->ProcessModal();
		id = win->GetHitID();
		
		if (id == 1)		// select
		{
			SMyServerInfo *pServerInfo;
			if (win->GetServerListView()->GetSelectedItem(&pServerInfo))
			{
				mSynchWin->SetServerInfo(pServerInfo);
				DoListGroups1();
			}
			
			delete win;			
			break;
		}
		else if (id == 2)	// cancel
		{
			delete win;
			break;
		}
	}
}

void CMyApplication::DoListGroups1()
{
	ClearStruct(mServerInfo);
	
	mServerInfo.psServerAddr[0] = mSynchWin->GetServerAddr(mServerInfo.psServerAddr + 1, sizeof(mServerInfo.psServerAddr) - 1);
	if (!mServerInfo.psServerAddr[0])
		return;

	mServerInfo.psUserLogin[0] = mSynchWin->GetUserLogin(mServerInfo.psUserLogin + 1, sizeof(mServerInfo.psUserLogin) - 1);
	mServerInfo.psUserPassword[0] = mSynchWin->GetUserPassword(mServerInfo.psUserPassword + 1, sizeof(mServerInfo.psUserPassword) - 1);

	AddServerList1(mServerInfo);
	StopListGroupsTask(mServerInfo.psServerAddr);
		
	mSynchWin->DeleteAllGroup1();
	mSynchWin->DeleteAllArticle1();
	mSynchWin->DeleteArticle1();
			
	new CMyListGroupsTask(mServerInfo);
}

bool CMyApplication::DoListGroups1(Uint32& inGroupIndex, const Uint8 *inServerAddr, TNntpTransact inNntpTransact)
{
	if (UMemory::Compare(mServerInfo.psServerAddr + 1, mServerInfo.psServerAddr[0], inServerAddr + 1, inServerAddr[0]))
		return false;

	Uint32 nGroupIndex = inGroupIndex;

	Uint8 psFilterText[256];
	psFilterText[0] = mSynchWin->GetFilterText1(psFilterText + 1, sizeof(psFilterText) - 1);

	SMyGroupInfo1 *pGroupInfo = (SMyGroupInfo1 *)UMemory::NewClear(sizeof(SMyGroupInfo1));
	while (inNntpTransact->GetNextGroup(pGroupInfo->stGroupInfo, inGroupIndex))
	{		
		mSynchWin->AddGroup1(pGroupInfo, psFilterText);
		pGroupInfo = (SMyGroupInfo1 *)UMemory::NewClear(sizeof(SMyGroupInfo1));
	}

	inGroupIndex--;
	UMemory::Dispose((TPtr)pGroupInfo);

	return (inGroupIndex > nGroupIndex);
}

void CMyApplication::DoListArticles1()
{
	SMyGroupInfo1 *pGroupInfo;
	if (!mSynchWin->GetGroupListView1()->GetSelectedItem(&pGroupInfo))
		return;
	
	StopListArticlesTask(mServerInfo.psServerAddr, pGroupInfo->stGroupInfo.psGroupName);
		
	mSynchWin->DeleteAllArticle1();
	mSynchWin->DeleteArticle1();

	mGroupInfo = pGroupInfo->stGroupInfo;
	new CMyListArticlesTask(mServerInfo, mGroupInfo.psGroupName);
}

bool CMyApplication::DoListArticles1(Uint32& inArticleIndex, const Uint8 *inGroupName, TNntpTransact inNntpTransact, CPtrList<SMyArticleInfo1>& inTempArticleList, bool inFinish)
{
	CMyGroupListView1 *pGroupListView = mSynchWin->GetGroupListView1();

	SMyGroupInfo1 *pGroupInfo;
	Uint32 nSelectedGroup = pGroupListView->GetSelectedItem(&pGroupInfo);
	if (!nSelectedGroup)
		return false;
	
	if (UMemory::Compare(pGroupInfo->stGroupInfo.psGroupName + 1, pGroupInfo->stGroupInfo.psGroupName[0], inGroupName + 1, inGroupName[0]))
		return false;
	
	Uint32 nArticleIndex = inArticleIndex;
	
	Uint32 i, nParentIndex;
	CMyArticleTreeView1 *pArticleTreeView = mSynchWin->GetArticleTreeView1();
	SMyArticleInfo1 *pArticleInfo = (SMyArticleInfo1 *)UMemory::NewClear(sizeof(SMyArticleInfo1));
	
	while (inNntpTransact->GetNextArticle(pArticleInfo->stArticleInfo, inArticleIndex))
	{
		nParentIndex = 0;
		
		// search parent
		if (pArticleInfo->stArticleInfo.psParentID[0])
		{
			i = 0;
			SMyArticleInfo1 *pParentInfo;
			
			while (pArticleTreeView->GetNextTreeItem(pParentInfo, i))
			{
				if (!UMemory::Compare(pArticleInfo->stArticleInfo.psParentID + 1, pArticleInfo->stArticleInfo.psParentID[0], pParentInfo->stArticleInfo.psArticleID + 1, pParentInfo->stArticleInfo.psArticleID[0]))
				{
					nParentIndex = i;
					break;
				}
			}			
			
			if (nParentIndex)
			{
				if (pArticleTreeView->GetTreeItemLevel(nParentIndex) == 1)
					pArticleTreeView->SetDisclosure(nParentIndex, optTree_Collapsed);
				else
					pArticleTreeView->SetDisclosure(nParentIndex, optTree_Disclosed);
			}
		}
	
		// if parent don't exist yet add in temporar list until finish
		if (pArticleInfo->stArticleInfo.psParentID[0] && !nParentIndex)
			inTempArticleList.AddItem(pArticleInfo);
		else
			pArticleTreeView->AddTreeItem(nParentIndex, pArticleInfo, false);
		
		pArticleInfo = (SMyArticleInfo1 *)UMemory::NewClear(sizeof(SMyArticleInfo1));
	}
	
	inArticleIndex--;
	UMemory::Dispose((TPtr)pArticleInfo);
	
	if (inFinish)
	{
		CPtrList<SMyArticleInfo1> pSearchArticleList;
				
		i = 0;	// search parent
		while (inTempArticleList.GetNext(pArticleInfo, i))
		{
			nParentIndex = 0;

			Uint32 j = 0;
			SMyArticleInfo1 *pParentInfo;
			
			while (pArticleTreeView->GetNextTreeItem(pParentInfo, j))
			{
				if (!UMemory::Compare(pArticleInfo->stArticleInfo.psParentID + 1, pArticleInfo->stArticleInfo.psParentID[0], pParentInfo->stArticleInfo.psArticleID + 1, pParentInfo->stArticleInfo.psArticleID[0]))
				{
					nParentIndex = j;
					break;
				}
			}
		
			if (nParentIndex)
			{
				if (pArticleTreeView->GetTreeItemLevel(nParentIndex) == 1)
					pArticleTreeView->SetDisclosure(nParentIndex, optTree_Collapsed);
				else
					pArticleTreeView->SetDisclosure(nParentIndex, optTree_Disclosed);

				pArticleTreeView->AddTreeItem(nParentIndex, pArticleInfo, false);
				inTempArticleList.RemoveItem(i);
				i--;
				
				pSearchArticleList.AddItem(pArticleInfo);
			}
		}
				
		DoListArticles1(inTempArticleList, pSearchArticleList);
		pSearchArticleList.Clear();
				
		i = 0; 	// if parent don't exist search brother
		while (inTempArticleList.GetNext(pArticleInfo, i))
		{
			nParentIndex = 0;

			Uint32 j = 0;
			SMyArticleInfo1 *pParentInfo;
			
			while (pArticleTreeView->GetNextTreeItem(pParentInfo, j))
			{
				if (!UMemory::Compare(pArticleInfo->stArticleInfo.psParentID + 1, pArticleInfo->stArticleInfo.psParentID[0], pParentInfo->stArticleInfo.psParentID + 1, pParentInfo->stArticleInfo.psParentID[0]))
				{
					nParentIndex = j;
					break;
				}
			}
					
			if (nParentIndex)
			{
				if (pArticleTreeView->GetTreeItemLevel(nParentIndex) == 1)
					pArticleTreeView->SetDisclosure(nParentIndex, optTree_Collapsed);
				else
					pArticleTreeView->SetDisclosure(nParentIndex, optTree_Disclosed);
			}

			pArticleTreeView->AddTreeItem(nParentIndex, pArticleInfo, false);
			inTempArticleList.RemoveItem(i);
			i = 0;

			pSearchArticleList.AddItem(pArticleInfo);
			DoListArticles1(inTempArticleList, pSearchArticleList);
			pSearchArticleList.Clear();
		}		
	
		// sort articles
		pArticleTreeView->SortArticles(mOptions.nOpts & myOpt_SortDescending1);
	}

	pGroupInfo->stGroupInfo.nArticleCount = pArticleTreeView->GetTreeCount();
	pGroupListView->RefreshItem(nSelectedGroup);

	return (inArticleIndex > nArticleIndex);
}

void CMyApplication::DoListArticles1(CPtrList<SMyArticleInfo1>& inTempArticleList, CPtrList<SMyArticleInfo1>& inSearchArticleList)
{
	if (!inTempArticleList.GetItemCount() || !inSearchArticleList.GetItemCount())
		return;
		
	Uint32 nParentIndex;
	CPtrList<SMyArticleInfo1> pSearchArticleList;
	CMyArticleTreeView1 *pArticleTreeView = mSynchWin->GetArticleTreeView1();

	Uint32 i = 0;
	SMyArticleInfo1 *pArticleInfo;

	// search parent
	while (inTempArticleList.GetNext(pArticleInfo, i))
	{
		nParentIndex = 0;

		Uint32 j = 0;
		SMyArticleInfo1 *pParentInfo;
			
		while (inSearchArticleList.GetNext(pParentInfo, j))
		{
			if (!UMemory::Compare(pArticleInfo->stArticleInfo.psParentID + 1, pArticleInfo->stArticleInfo.psParentID[0], pParentInfo->stArticleInfo.psArticleID + 1, pParentInfo->stArticleInfo.psArticleID[0]))
			{
				nParentIndex = pArticleTreeView->GetTreeItemIndex(pParentInfo);
				break;
			}
		}
		
		if (nParentIndex)
		{
			if (pArticleTreeView->GetTreeItemLevel(nParentIndex) == 1)
				pArticleTreeView->SetDisclosure(nParentIndex, optTree_Collapsed);
			else
				pArticleTreeView->SetDisclosure(nParentIndex, optTree_Disclosed);

			pArticleTreeView->AddTreeItem(nParentIndex, pArticleInfo, false);
			inTempArticleList.RemoveItem(i);
			i--;
				
			pSearchArticleList.AddItem(pArticleInfo);
		}
	}
	
	if (pSearchArticleList.GetItemCount())
	{			
		DoListArticles1(inTempArticleList, pSearchArticleList);
		pSearchArticleList.Clear();		
	}
}

void CMyApplication::DoGetArticle1()
{
	SMyArticleInfo1 *pArticleInfo;
	if (!mSynchWin->GetArticleTreeView1()->GetFirstSelectedTreeItem(&pArticleInfo))
		return;
	
	if (IsGetArticleTask(mServerInfo.psServerAddr, mGroupInfo.psGroupName, pArticleInfo->stArticleInfo.nArticleNumber))
		return;
	
	mSynchWin->DeleteArticle1();

	new CMyGetArticleTask(mServerInfo, mGroupInfo.psGroupName, pArticleInfo->stArticleInfo.psArticleName, pArticleInfo->stArticleInfo.nArticleNumber);
}

void CMyApplication::DoGetArticle1(Uint32 inArticleNumber, TNntpTransact inNntpTransact)
{
	SMyArticleInfo1 *pArticleInfo;
	if (!mSynchWin->GetArticleTreeView1()->GetFirstSelectedTreeItem(&pArticleInfo))
		return;

	if (pArticleInfo->stArticleInfo.nArticleNumber != inArticleNumber)
		return;
	
	mSynchWin->DeleteArticle1();
	
	Uint32 i = 0;
	SArticleData *pArticleData;
	
	while (inNntpTransact->GetNextData(pArticleData, i))
	{
		if (!UMemory::Compare(pArticleData->psName + 1, pArticleData->psName[0], ".body", 5) &&
		    !UMemory::Compare(pArticleData->csFlavor, strlen(pArticleData->csFlavor), "text/plain", 10))
		{
			mSynchWin->GetArticleTextView1()->SetText(pArticleData->pData, pArticleData->nDataSize);	
		}
		else
		{
			SArticleData *pAttachmentData = (SArticleData *)UMemory::NewClear(sizeof(SArticleData));
			
			UMemory::Copy(pAttachmentData->csFlavor, pArticleData->csFlavor, sizeof(pArticleData->csFlavor) + 1);
			UMemory::Copy(pAttachmentData->psName, pArticleData->psName, pArticleData->psName[0] + 1);

			pAttachmentData->nDataSize = pArticleData->nDataSize;
			pArticleData->nDataSize = 0;
	
			pAttachmentData->pData = pArticleData->pData;
			pArticleData->pData = nil;
			
			mSynchWin->AddAttachment1(pAttachmentData);
		}
	}
}

#pragma mark -

void CMyApplication::DoSelectServer2()
{
	Uint32 id;
	CMySelectServerWin2 *win = new CMySelectServerWin2();

	win->GetServerListView()->SetItemsFromList(&mServerList2);
	win->Show();
	
	// go into modal loop to process modal options window
	for(;;)
	{
		win->ProcessModal();
		id = win->GetHitID();
		
		if (id == 1)		// select
		{
			TFSRefObj* pBundleRef;
			SMyServerBundleInfo *pServerBundleInfo;
			
			if (win->GetSelectedServer(pServerBundleInfo, pBundleRef))
			{
				bool bIsFolder;
				if (pBundleRef->Exists(&bIsFolder) && bIsFolder)
				{
					UFileSys::Dispose(mBundleRef);
					mBundleRef = pBundleRef->Clone();

					mSynchWin->DeleteAllGroup2();

					mSynchWin->GetServerNameView()->SetText(pServerBundleInfo->psServerName + 1, pServerBundleInfo->psServerName[0]);
					DoListGroups2();
				}
			}
			
			delete win;			
			break;
		}
		else if (id == 2)	// cancel
		{
			delete win;
			break;
		}
	}
}

void CMyApplication::SetServerName2()
{
	if (!mSynchWin)
		return;
	
	Uint8 psServerName[32];
	psServerName[0] = SearchServerList2(mBundleRef, psServerName + 1, sizeof(psServerName) - 1);
	mSynchWin->GetServerNameView()->SetText(psServerName + 1, psServerName[0]);
}

void CMyApplication::DoSelectBundle2()
{
	TFSRefObj* pBundleRef = UFileSys::UserSelectFolder();
	if (!pBundleRef)
		return;
		
	UFileSys::Dispose(mBundleRef);
	mBundleRef = pBundleRef;
		
	mSynchWin->DeleteAllGroup2();

	SetServerName2();
	DoListGroups2();
}

void CMyApplication::DoListGroups2()
{
	if (!mBundleRef)
		return;
	
	// clear all
	mSynchWin->DeleteAllGroup2();
	mSynchWin->DeleteAllArticle2();
	mSynchWin->DeleteArticle2();

	// set bundle name
	Uint8 psBundleName[256];
	mBundleRef->GetName(psBundleName);
	mSynchWin->GetBundleNameView()->SetText(psBundleName + 1, psBundleName[0]);
	
	Uint8 psFilterText[256];
	psFilterText[0] = mSynchWin->GetFilterText2(psFilterText + 1, sizeof(psFilterText) - 1);

	Uint32 nOffset = 0;
	Uint8 psName[256];
	Uint32 nType, nFlags;
	SMyNewsGroupInfo stGroupInfo;
	THdl hList = mBundleRef->GetListing();
	
	try
	{
		while(UFS::GetListNext(hList, nOffset, psName, &nType, nil, nil, nil, nil, &nFlags))
		{
			if ((nFlags & 1) || nType != TB((Uint32)'HLNZ')) // if invisible or not news file
				continue;
	
			StFileSysRef pNewsFile(mBundleRef, nil, psName, fsOption_PreferExistingFile);
			if (pNewsFile.IsInvalid())
				continue;
			
			StNewsDatabase pNewsDatabase(pNewsFile);
			pNewsDatabase->GetGroupInfo(stGroupInfo);

			SMyGroupInfo2 *pGroupInfo = (SMyGroupInfo2 *)UMemory::NewClear(sizeof(SMyGroupInfo2));
			pGroupInfo->nArticleCount = stGroupInfo.articleCount;
			UMemory::Copy(pGroupInfo->psGroupName, psName, psName[0] + 1);
		
			mSynchWin->AddGroup2(pGroupInfo, psFilterText);
		}
	}
	catch(...)
	{
		// don't throw here
	}

	UMemory::Dispose(hList);
}

void CMyApplication::DoListArticles2()
{
	if (!mBundleRef)
		return;

	CMyGroupListView2 *pGroupListView = mSynchWin->GetGroupListView2();

	SMyGroupInfo2 *pGroupInfo;
	Uint32 nSelectedGroup = pGroupListView->GetSelectedItem(&pGroupInfo);
	if (!nSelectedGroup)
		return;

	StFileSysRef pNewsFile(mBundleRef, nil, pGroupInfo->psGroupName, fsOption_PreferExistingFile);
	if (pNewsFile.IsInvalid())
		return;

	mSynchWin->DeleteAllArticle2();
	mSynchWin->DeleteArticle2();

	StNewsDatabase pNewsDatabase(pNewsFile);

	Uint32 nSize;
	TPtr pList = pNewsDatabase->GetArticleListing(nSize);
	if (!pList)
		return;
	
	CMyArticleTreeView2 *pArticleTreeView = mSynchWin->GetArticleTreeView2();

	try
	{
		pGroupInfo->nArticleCount = pArticleTreeView->SetItemsFromData((Uint8 *)pList, nSize);
	}
	catch(...)
	{
		// don't throw here
	}
	
	UMemory::Dispose(pList);
	
	// sort articles
	pArticleTreeView->SortArticles(mOptions.nOpts & myOpt_SortDescending2);

	pGroupListView->RefreshItem(nSelectedGroup);
}

void CMyApplication::DoOpenArticle2()
{
	if (!mBundleRef)
		return;

	SMyGroupInfo2 *pGroupInfo;
	if (!mSynchWin->GetGroupListView2()->GetSelectedItem(&pGroupInfo))
		return;

	SMyArticleInfo2 *pArticleInfo = mSynchWin->GetArticleTreeView2()->GetSelectedArticle();
	if (!pArticleInfo)
		return;

	StFileSysRef pNewsFile(mBundleRef, nil, pGroupInfo->psGroupName, fsOption_PreferExistingFile);
	if (pNewsFile.IsInvalid())
		return;
		
	mSynchWin->DeleteArticle2();
	CTextView *pArticleText = mSynchWin->GetArticleTextView2();

	StNewsDatabase pNewsDatabase(pNewsFile);

	Uint32 i = 0;
	SMyDataInfo2 *pDataInfo;
	
	while (pArticleInfo->lDataList.GetNext(pDataInfo, i))
	{	
		if (!UMemory::Compare(pDataInfo->psName + 1, pDataInfo->psName[0], ".body", 5) &&
			!UMemory::Compare(pDataInfo->csFlavor, strlen(pDataInfo->csFlavor), "text/plain", 10))
		{
			TPtr pData;
			Uint32 nDataSize;

			try
			{
				pData = pNewsDatabase->GetArticleData(pArticleInfo->nArticleID, pDataInfo->psName, nDataSize);
			}
			catch(...)
			{
				pData = nil;	// don't throw here
			}

			if (pData)
			{
				pArticleText->SetText(pData, nDataSize);
				UMemory::Dispose(pData);
			}
		}
		else
			mSynchWin->AddAttachment2(pDataInfo->psName);
	}
}

#pragma mark -

void CMyApplication::DoSynchGroup1(const Uint8 *inGroupName1, const Uint8 *inGroupName2)
{
	if (!mServerInfo.psServerAddr[0] || !inGroupName1 || !inGroupName1[0] || !inGroupName2 || !inGroupName2[0] || !mBundleRef)
		return;

	if (IsSynchGroupTask1(mServerInfo.psServerAddr, inGroupName1, mBundleRef, inGroupName2))
		return;
	
	new CMySynchGroupTask1(mServerInfo, inGroupName1, mBundleRef, inGroupName2, mOptions.nPurgeDays, mOptions.nMaxArticles);
}

void CMyApplication::DoSynchArticle1(Uint32 inArticleID2, const Uint8 *inArticleName2, CPtrList<SMyDataInfo2>& inDataList2, CPtrList<Uint8>& inParentIDs1)
{
	if (!mServerInfo.psServerAddr[0] || !inArticleID2 || !mBundleRef)
		return;

	SMyGroupInfo2 *pGroupInfo;
	if (!mSynchWin->GetGroupListView2()->GetSelectedItem(&pGroupInfo))
		return;

	if (IsSynchArticleTask1(mServerInfo.psServerAddr, mGroupInfo.psGroupName, mBundleRef, pGroupInfo->psGroupName, inArticleID2))
		return;

	new CMySynchArticleTask1(mServerInfo, mGroupInfo.psGroupName, inParentIDs1, mBundleRef, pGroupInfo->psGroupName, inArticleID2, inArticleName2, inDataList2);
}

void CMyApplication::DoSynchGroup2(const Uint8 *inGroupName1, const Uint8 *inGroupName2)
{
	if (!mServerInfo.psServerAddr[0] || !inGroupName1 || !inGroupName1[0] || !mBundleRef)
		return;
	
	const Uint8 *pGroupName2 = nil;
	if (inGroupName2 && inGroupName2[0])
		pGroupName2 = inGroupName2;
	
	if (IsSynchGroupTask2(mServerInfo.psServerAddr, inGroupName1, mBundleRef, pGroupName2))
		return;

	new CMySynchGroupTask2(mServerInfo, inGroupName1, mBundleRef, pGroupName2, mOptions.nPurgeDays, mOptions.nMaxArticles);
}

void CMyApplication::DoSynchArticle2(Uint32 inArticleID1, const Uint8 *inArticleName1, Uint32 inParentID2)
{
	if (!mServerInfo.psServerAddr[0] || !inArticleID1 || !mBundleRef)
		return;

	SMyGroupInfo2 *pGroupInfo;
	if (!mSynchWin->GetGroupListView2()->GetSelectedItem(&pGroupInfo))
		return;

	if (IsSynchArticleTask2(mServerInfo.psServerAddr, mGroupInfo.psGroupName, inArticleID1, mBundleRef, pGroupInfo->psGroupName))
		return;
	
	new CMySynchArticleTask2(mServerInfo, mGroupInfo.psGroupName, inArticleID1, inArticleName1, mBundleRef, pGroupInfo->psGroupName, inParentID2);
}

#pragma mark -

void CMyApplication::MakeSynch(SMySynchInfo& inSynchInfo)
{
	if (inSynchInfo.nSynchType == myOpt_SynchNewsHotline)
	{
		if (!IsSynchGroupTask2(inSynchInfo.stServerInfo.psServerAddr, inSynchInfo.psGroupName, inSynchInfo.pHotlineFile))
			new CMySynchGroupTask2(inSynchInfo.stServerInfo, inSynchInfo.psGroupName, inSynchInfo.pHotlineFile, inSynchInfo.nPurgeDays, inSynchInfo.nMaxArticles);
	}
	else if (inSynchInfo.nSynchType == myOpt_SynchNewsServer)
	{
		if (!IsSynchGroupTask1(inSynchInfo.stServerInfo.psServerAddr, inSynchInfo.psGroupName, inSynchInfo.pHotlineFile))
			new CMySynchGroupTask1(inSynchInfo.stServerInfo, inSynchInfo.psGroupName, inSynchInfo.pHotlineFile, inSynchInfo.nPurgeDays, inSynchInfo.nMaxArticles);
	}
	else if (inSynchInfo.nSynchType == myOpt_SynchNewsBoth)
	{
		if (!IsSynchGroupTask21(inSynchInfo.stServerInfo.psServerAddr, inSynchInfo.psGroupName, inSynchInfo.pHotlineFile))
			new CMySynchGroupTask21(inSynchInfo.stServerInfo, inSynchInfo.psGroupName, inSynchInfo.pHotlineFile, inSynchInfo.nPurgeDays, inSynchInfo.nMaxArticles);
	}

	UDateTime::GetCalendarDate(calendar_Gregorian, inSynchInfo.stLastSynch);
}

void CMyApplication::MakeAutoSynch()
{
	mSynchTimer->Stop();

	SCalendarDate stCurrentDate, stSynchDate;
	UDateTime::GetCalendarDate(calendar_Gregorian, stCurrentDate);

	Uint32 i = 0, nNextSynch = max_Uint32;
	SMyOffSynchInfo *pOffSynchInfo;
	
	while (mSynchList.GetNext(pOffSynchInfo, i))
	{
		stSynchDate = pOffSynchInfo->pSynchInfo->stLastSynch + pOffSynchInfo->pSynchInfo->nSynchHours * 3600;
		
		if (stSynchDate <= stCurrentDate)
		{			
			if (pOffSynchInfo->pSynchInfo->nSynchType == myOpt_SynchNewsHotline)
			{
				if (!IsSynchGroupTask2(pOffSynchInfo->pSynchInfo->stServerInfo.psServerAddr, pOffSynchInfo->pSynchInfo->psGroupName, pOffSynchInfo->pSynchInfo->pHotlineFile))
					new CMySynchGroupTask2(pOffSynchInfo->pSynchInfo->stServerInfo, pOffSynchInfo->pSynchInfo->psGroupName, pOffSynchInfo->pSynchInfo->pHotlineFile, pOffSynchInfo->pSynchInfo->nPurgeDays, pOffSynchInfo->pSynchInfo->nMaxArticles);
			}
			else if (pOffSynchInfo->pSynchInfo->nSynchType == myOpt_SynchNewsServer)
			{
				if (!IsSynchGroupTask1(pOffSynchInfo->pSynchInfo->stServerInfo.psServerAddr, pOffSynchInfo->pSynchInfo->psGroupName, pOffSynchInfo->pSynchInfo->pHotlineFile))
					new CMySynchGroupTask1(pOffSynchInfo->pSynchInfo->stServerInfo, pOffSynchInfo->pSynchInfo->psGroupName, pOffSynchInfo->pSynchInfo->pHotlineFile, pOffSynchInfo->pSynchInfo->nPurgeDays, pOffSynchInfo->pSynchInfo->nMaxArticles);
			}
			else if (pOffSynchInfo->pSynchInfo->nSynchType == myOpt_SynchNewsBoth)
			{
				if (!IsSynchGroupTask21(pOffSynchInfo->pSynchInfo->stServerInfo.psServerAddr, pOffSynchInfo->pSynchInfo->psGroupName, pOffSynchInfo->pSynchInfo->pHotlineFile))
					new CMySynchGroupTask21(pOffSynchInfo->pSynchInfo->stServerInfo, pOffSynchInfo->pSynchInfo->psGroupName, pOffSynchInfo->pSynchInfo->pHotlineFile, pOffSynchInfo->pSynchInfo->nPurgeDays, pOffSynchInfo->pSynchInfo->nMaxArticles);
			}
		
			pOffSynchInfo->pSynchInfo->stLastSynch = stCurrentDate;
			stSynchDate = pOffSynchInfo->pSynchInfo->stLastSynch + pOffSynchInfo->pSynchInfo->nSynchHours * 3600;

			WriteLastSynch(pOffSynchInfo->nDateOffset, pOffSynchInfo->pSynchInfo->stLastSynch);
		}
		
		Int32 nSeconds = stSynchDate - stCurrentDate;
		if (nSeconds > 0 && nSeconds < nNextSynch)
			nNextSynch = nSeconds;
	}

	if (nNextSynch != max_Uint32)
		mSynchTimer->Start(nNextSynch * 1000, kOnceTimer);
}

void CMyApplication::DoGroupFilter1(const SHitMsgData& inInfo)
{
	mSynchWin->FilterGroupList1(inInfo.type == hitType_Standard);
	inInfo.view->SetHasChanged(false);
}

void CMyApplication::DoGroupFilter2(const SHitMsgData& inInfo)
{
	mSynchWin->FilterGroupList2(inInfo.type == hitType_Standard);
	inInfo.view->SetHasChanged(false);
}

void CMyApplication::SortArticles1()
{
	if (mOptions.nOpts & myOpt_SortDescending1)
		mOptions.nOpts &= ~myOpt_SortDescending1;
	else
		mOptions.nOpts |= myOpt_SortDescending1;

	mSynchWin->GetArticleTreeView1()->SortArticles(mOptions.nOpts & myOpt_SortDescending1);
	mSynchWin->UpdateSortBtn1(mOptions.nOpts & myOpt_SortDescending1);
}

void CMyApplication::SortArticles2()
{
	if (mOptions.nOpts & myOpt_SortDescending2)
		mOptions.nOpts &= ~myOpt_SortDescending2;
	else
		mOptions.nOpts |= myOpt_SortDescending2;

	mSynchWin->GetArticleTreeView2()->SortArticles(mOptions.nOpts & myOpt_SortDescending2);
	mSynchWin->UpdateSortBtn2(mOptions.nOpts & myOpt_SortDescending2);
}

void CMyApplication::SaveAttachment1()
{
	SArticleData *pArticleData = mSynchWin->GetSelectedAttachment1();
	if (!pArticleData)
		return;

	TFSRefObj* fp = UFileSys::UserSaveFile(nil, pArticleData->psName);
	if (!fp)
		return;
		
	scopekill(TFSRefObj, fp);
	if (fp->Exists())
		fp->DeleteFile();
	
	fp->CreateFile(UMime::ConvertMime_TypeCode(pArticleData->csFlavor), 'HTLC');
	StFileOpener pFileOpener(fp, perm_ReadWrite);

	fp->Write(0, pArticleData->pData, pArticleData->nDataSize);
}

void CMyApplication::SaveAttachment2()
{
	if (!mBundleRef)
		return;

	Uint8 psAttachmentName[64];
	psAttachmentName[0] = mSynchWin->GetSelectedAttachment2(psAttachmentName + 1, sizeof(psAttachmentName) - 1);
	if (!psAttachmentName[0])
		return;

	SMyGroupInfo2 *pGroupInfo;
	if (!mSynchWin->GetGroupListView2()->GetSelectedItem(&pGroupInfo))
		return;

	SMyArticleInfo2 *pArticleInfo = mSynchWin->GetArticleTreeView2()->GetSelectedArticle();
	if (!pArticleInfo)
		return;

	StFileSysRef pNewsFile(mBundleRef, nil, pGroupInfo->psGroupName, fsOption_PreferExistingFile);
	if (pNewsFile.IsInvalid())
		return;
	
	TFSRefObj* pAttachmentFile = UFileSys::UserSaveFile(nil, psAttachmentName);
	if (!pAttachmentFile)
		return;
		
	scopekill(TFSRefObj, pAttachmentFile);
	StNewsDatabase pNewsDatabase(pNewsFile);

	TPtr pData;
	Uint32 nDataSize;
	Int8 csFlavor[32];
	
	try
	{
		pData = pNewsDatabase->GetArticleData(pArticleInfo->nArticleID, psAttachmentName, nDataSize, csFlavor);
	}
	catch(...)
	{
		pData = nil; // don't throw here
	}

	if (!pData)
		return;

	if (pAttachmentFile->Exists())
		pAttachmentFile->DeleteFile();
	
	pAttachmentFile->CreateFile(UMime::ConvertMime_TypeCode(csFlavor), 'HTLC');
	StFileOpener pFileOpener(pAttachmentFile, perm_ReadWrite);

	pAttachmentFile->Write(0, pData, nDataSize);
	UMemory::Dispose(pData);
}

#pragma mark -

Uint32 CMyApplication::MakeLog(const Uint8 *inLogText)
{
	LogToFile(inLogText);
	return mSynchWin->GetLogListView()->MakeLog(inLogText);
}

void CMyApplication::AddLog(Uint32 inLogID, const Uint8 *inLogText)
{
	LogToFile(inLogText);
	mSynchWin->GetLogListView()->AddLog(inLogID, inLogText);
}

void CMyApplication::SelectLog(Uint32 inLogID)
{
	mSynchWin->GetLogListView()->SelectLog(inLogID);	
}

void CMyApplication::LogToFile(const Uint8 *inData)
{
	if (!(mOptions.nOpts & myOpt_LogToFile))
		return;

	Uint8 psDate[64];
	Uint8 bufData[256];

	psDate[0] = UDateTime::DateToText(psDate + 1, sizeof(psDate) - 1, kShortDateFullYearText + kTimeWithSecsText);
	Uint32 nSize = UText::Format(bufData, sizeof(bufData), "\r%#-22.22s %#s", psDate, inData);
	
	LogToFile(bufData, nSize);			
}

void CMyApplication::LogToFile(const void *inData, Uint32 inSize)
{
	if (!(mOptions.nOpts & myOpt_LogToFile))
		return;
	
	// log to log file
	try
	{
		StFileSysRef logFile(kProgramFolder, nil, "\pNewsSynch Log.txt");
		if (!logFile->Exists())
			logFile->CreateFile('TEXT', 'ttxt');
			
		const Uint8 *pData = (Uint8 *)inData;
		Uint32 nSize = inSize;
			
		if (!logFile->GetSize())
		{
			pData++;
			nSize--;
		}

		StFileOpener open(logFile);
		logFile->Write(logFile->GetSize(), pData, nSize);
	}
	catch(...)
	{
	}
}

#pragma mark -

void CMyApplication::AddTask(CMyTask *inTask, const Uint8 *inDesc)
{
	if (!inTask) return;
	
	mSynchWin->GetTaskListView()->AddTask(inTask, inDesc);	
	mTaskList.AddItem(inTask);
}

void CMyApplication::RemoveTask(CMyTask *inTask)
{
	if (!inTask) return;

	mTaskList.RemoveItem(inTask);
	mSynchWin->GetTaskListView()->RemoveTask(inTask);
}

void CMyApplication::FinishTask(CMyTask *inTask)
{
	if (!inTask) return;
	
	mSynchWin->GetTaskListView()->ShowFinishedBar(inTask);
	mFinishTaskTimer->Start(1000, kOnceTimer);
}

void CMyApplication::SetTaskProgress(CMyTask *inTask, Uint32 inVal, Uint32 inMax, const Uint8 *inDesc)
{
	mSynchWin->GetTaskListView()->SetTaskProgress(inTask, inVal, inMax, inDesc);
}

void CMyApplication::SelectTask(Uint32 inLogID)
{
	mSynchWin->GetTaskListView()->SelectTask(inLogID);	
}

bool CMyApplication::StopListGroupsTask(const Uint8 *inServerAddr)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMyListGroupsTask *pListGroupTask = dynamic_cast<CMyListGroupsTask*>(pTask);
		
		if (pListGroupTask && pListGroupTask->IsListGroupsTask(inServerAddr))
		{
			if (!pListGroupTask->IsFinished())
			{
				pListGroupTask->LogStop();
				delete pListGroupTask;
			}

			return true;
		}
	}
	
	return false;	
}

bool CMyApplication::StopListArticlesTask(const Uint8 *inServerAddr, const Uint8 *inGroupName)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMyListArticlesTask *pListArticlesTask = dynamic_cast<CMyListArticlesTask*>(pTask);
		
		if (pListArticlesTask && pListArticlesTask->IsListArticlesTask(inServerAddr, inGroupName))
		{
			if (!pListArticlesTask->IsFinished())
			{
				pListArticlesTask->LogStop();
				delete pListArticlesTask;
			}
			
			return true;
		}
	}
	
	return false;	
}

bool CMyApplication::IsGetArticleTask(const Uint8 *inServerAddr, const Uint8 *inGroupName, Uint32 inArticleID)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMyGetArticleTask *pGetArticleTask = dynamic_cast<CMyGetArticleTask*>(pTask);
		
		if (pGetArticleTask && pGetArticleTask->IsGetArticleTask(inServerAddr, inGroupName, inArticleID))
			return true;
	}
	
	return false;	
}

bool CMyApplication::IsSynchArticleTask1(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inArticleID2)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMySynchArticleTask1 *pSynchArticleTask1 = dynamic_cast<CMySynchArticleTask1*>(pTask);
		
		if (pSynchArticleTask1 && pSynchArticleTask1->IsSynchArticleTask1(inServerAddr, inGroupName1, inGroupFolder, inGroupName2, inArticleID2))
			return true;
	}
	
	return false;
}

bool CMyApplication::IsSynchArticleTask2(const Uint8 *inServerAddr, const Uint8 *inGroupName1, Uint32 inArticleID1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMySynchArticleTask2 *pSynchArticleTask2 = dynamic_cast<CMySynchArticleTask2*>(pTask);
		
		if (pSynchArticleTask2 && pSynchArticleTask2->IsSynchArticleTask2(inServerAddr, inGroupName1, inArticleID1, inGroupFolder, inGroupName2))
			return true;
	}
	
	return false;	
}

bool CMyApplication::IsSynchGroupTask1(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, bool inSearchAll)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMySynchGroupTask1 *pSynchGroupTask1 = dynamic_cast<CMySynchGroupTask1*>(pTask);
		
		if (pSynchGroupTask1 && pSynchGroupTask1->IsSynchGroupTask(inServerAddr, inGroupName1, inGroupFile))
			return true;
	}
	
	if (inSearchAll)
		return IsSynchGroupTask21(inServerAddr, inGroupName1, inGroupFile, (bool)false);

	return false;
}

bool CMyApplication::IsSynchGroupTask1(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, bool inSearchAll)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMySynchGroupTask1 *pSynchGroupTask1 = dynamic_cast<CMySynchGroupTask1*>(pTask);
		
		if (pSynchGroupTask1 && pSynchGroupTask1->IsSynchGroupTask(inServerAddr, inGroupName1, inGroupFolder, inGroupName2))
			return true;
	}
	
	if (inSearchAll)
		return IsSynchGroupTask21(inServerAddr, inGroupName1, inGroupFolder, inGroupName2, false);
		
	return false;
}

bool CMyApplication::IsSynchGroupTask2(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, bool inSearchAll)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMySynchGroupTask2 *pSynchGroupTask2 = dynamic_cast<CMySynchGroupTask2*>(pTask);
		
		if (pSynchGroupTask2 && pSynchGroupTask2->IsSynchGroupTask(inServerAddr, inGroupName1, inGroupFile))
			return true;
	}

	if (inSearchAll)
		return IsSynchGroupTask21(inServerAddr, inGroupName1, inGroupFile, (bool)false);
	
	return false;
}

bool CMyApplication::IsSynchGroupTask2(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, bool inSearchAll)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMySynchGroupTask2 *pSynchGroupTask2 = dynamic_cast<CMySynchGroupTask2*>(pTask);
		
		if (pSynchGroupTask2 && pSynchGroupTask2->IsSynchGroupTask(inServerAddr, inGroupName1, inGroupFolder, inGroupName2))
			return true;
	}
	
	if (inSearchAll)
		return IsSynchGroupTask21(inServerAddr, inGroupName1, inGroupFolder, inGroupName2, false);

	return false;
}

bool CMyApplication::IsSynchGroupTask21(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, bool inSearchAll)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMySynchGroupTask21 *pSynchGroupTask21 = dynamic_cast<CMySynchGroupTask21*>(pTask);
		
		if (pSynchGroupTask21 && pSynchGroupTask21->IsSynchGroupTask(inServerAddr, inGroupName1, inGroupFile))
			return true;
	}
	
	if (inSearchAll)
		return (IsSynchGroupTask1(inServerAddr, inGroupName1, inGroupFile, (bool)false) || IsSynchGroupTask2(inServerAddr, inGroupName1, inGroupFile, (bool)false));

	return false;
}

bool CMyApplication::IsSynchGroupTask21(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, bool inSearchAll)
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		CMySynchGroupTask21 *pSynchGroupTask21 = dynamic_cast<CMySynchGroupTask21*>(pTask);
		
		if (pSynchGroupTask21 && pSynchGroupTask21->IsSynchGroupTask(inServerAddr, inGroupName1, inGroupFolder, inGroupName2))
			return true;
	}

	if (inSearchAll)
		return (IsSynchGroupTask1(inServerAddr, inGroupName1, inGroupFolder, inGroupName2, false) || IsSynchGroupTask2(inServerAddr, inGroupName1, inGroupFolder, inGroupName2, false));
	
	return false;
}

void CMyApplication::KillFinishedTasks()
{
	Uint32 i = 0;
	CMyTask *pTask = nil;
	
	while (mTaskList.GetNext(pTask, i))
	{
		if (pTask->IsFinished())
		{
			delete pTask;
			i--;
		}
	}
}

void CMyApplication::KillAllTasks()
{
	Uint32 i = 0;
	CMyTask *pTask;
	
	while (mTaskList.GetNext(pTask, i))
	{
		delete pTask;
		i--;
	}
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Prefs File ₯₯

TFSRefObj* CMyApplication::GetPrefsRef()
{
	return UFS::New(kProgramFolder, nil, "\pPrefs");
}

bool CMyApplication::WritePrefs()
{
	TFSRefObj* fp = GetPrefsRef();
	if (!fp) return false;
	scopekill(TFSRefObj, fp);

	Uint8 nTabPercent[17];
	SRect stWinRect, stViewsRect[14];
	
	mSynchWin->GetBounds(stWinRect);
	mSynchWin->GetInternalBounds(stViewsRect, nTabPercent);

	Uint8 buf[1024];
	CFlatten flat(buf);
	
	// build flat prefs file data
	flat.WriteWord(kMyPrefsVersion);
	
	flat.WriteShortRect(stWinRect);
	for (Uint32 i = 0; i < 14; i++) 
		flat.WriteShortRect(stViewsRect[i]);

	for (Uint32 i = 0; i < 17; i++) 
		flat.WriteByte(nTabPercent[i]);
		
	flat.Reserve(15);
	flat.WriteLong(mOptions.nOpts);
	flat.WriteLong(mOptions.nPurgeDays);
	flat.WriteLong(mOptions.nMaxArticles);
	flat.WritePString(mOptions.psEmailAddress);
	flat.WritePString(mOptions.psOrganization);
	flat.WriteLong(mOptions.nArticleTextSize);

	// write the prefs file
	fp->CreateFileAndOpen('pref', 'HTLS', kAlwaysOpenFile, perm_ReadWrite);
	try
	{
		Uint32 nSize = flat.GetSize();
		fp->SetSize(nSize);
		fp->Write(0, buf, nSize);
		
		if (mOptions.pAddArticleText)
			fp->Write(nSize, mOptions.pAddArticleText, mOptions.nArticleTextSize);
	}
	catch(...)
	{
		fp->Close();
		fp->DeleteFile();
		return false;
	}
	
	return true;
}

bool CMyApplication::ReadPrefs(SRect& outWinRect, SRect outViewsRect[14], Uint8 outTabPercent[17])
{
	Uint32 nSize;
	Uint8 buf[1024];

	ClearStruct(mOptions);
	
	// read prefs file data
	try
	{
		TFSRefObj* fp = GetPrefsRef();
		if (!fp) return false;
		scopekill(TFSRefObj, fp);
	
		fp->Open(perm_Read);
		nSize = fp->Read(0, buf, sizeof(buf));
	
		// prepare to extract prefs data
		if (nSize < 50) return false;;
		CUnflatten unflat(buf, nSize);

		// extract version
		Uint16 nVers = unflat.ReadWord();
		if (nVers != kMyPrefsVersion) return false;
	
		// extract prefs data
		unflat.ReadShortRect(outWinRect);
		for (Uint32 i = 0; i < 14; i++)
			unflat.ReadShortRect(outViewsRect[i]);
	
		for (Uint32 i = 0; i < 17; i++)
			outTabPercent[i] = unflat.ReadByte();
		
		unflat.Skip(15);
		mOptions.nOpts = unflat.ReadLong();
		mOptions.nPurgeDays = unflat.ReadLong();
		mOptions.nMaxArticles = unflat.ReadLong();
		unflat.ReadPString(mOptions.psEmailAddress);
		unflat.ReadPString(mOptions.psOrganization);
	
		mOptions.nArticleTextSize = unflat.ReadLong();
		if (mOptions.nArticleTextSize)
		{
			nSize  = unflat.GetPtr() - buf;
		
			mOptions.pAddArticleText = UMemory::New(mOptions.nArticleTextSize);
			if (mOptions.pAddArticleText)
				mOptions.nArticleTextSize = fp->Read(nSize, mOptions.pAddArticleText, mOptions.nArticleTextSize);
		}
	}
	catch(...)
	{
		mOptions.nOpts |= myOpt_OldNewsFormat;
		mOptions.nPurgeDays = DEFAULT_PURGE_DAYS;
		mOptions.nMaxArticles = DEFAULT_MAX_ARTICLES;
		
		return false;
	}

	return true;
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Synch File ₯₯

TFSRefObj* CMyApplication::GetSynchRef()
{
	return UFS::New(kProgramFolder, nil, "\pSynch");
}

void CMyApplication::MakeSynchBackup()
{
	TFSRefObj* fpb = UFS::New(kProgramFolder, nil, "\pSynch.bak");
	scopekill(TFSRefObj, fpb);
	
	if (fpb->Exists())
		fpb->DeleteFile();
			
	TFSRefObj* fpo = GetSynchRef();
	scopekill(TFSRefObj, fpo);
	
	if (fpo->Exists())
		fpo->SetName("\pSynch.bak");
}

void CMyApplication::WriteSynchInfo(CPtrList<SMySynchInfo> *inSynchList)
{
	MakeSynchBackup();

	Uint32 nSynchCount = inSynchList->GetItemCount();
	if (!nSynchCount) 
		return;

	// get FS ref for synch file
	TFSRefObj* fp = GetSynchRef();
	if (!fp) 
	{
		ClearSynchList(inSynchList);
		return;
	}
	
	scopekill(TFSRefObj, fp);
	ClearSynchList();

	// write synch file
	fp->CreateFileAndOpen('sync', 'HTLS', kAlwaysOpenFile, perm_ReadWrite);
	
	try
	{
		Uint16 nSynchVersion = TB((Uint16)kMySynchVersion);
		Uint32 nOffset = fp->Write(0, &nSynchVersion, 2);

		nSynchCount = TB(nSynchCount);
		nOffset += fp->Write(nOffset, &nSynchCount, 4);
		
		Uint32 i = 0;
		SMySynchInfo *pSynchInfo;
				
		while (inSynchList->GetNext(pSynchInfo, i))
		{
			Uint32 nSize = sizeof(pSynchInfo->nActive) + sizeof(pSynchInfo->stServerInfo) + sizeof(pSynchInfo->psGroupName) + sizeof(pSynchInfo->psHotlineServer) + sizeof(pSynchInfo->nSynchType);
			nOffset += fp->Write(nOffset, pSynchInfo, nSize);
			
			// synch hours
			Uint32 nSynchHours = TB(pSynchInfo->nSynchHours);
			nOffset += fp->Write(nOffset, &nSynchHours, sizeof(nSynchHours));

			// purge days
			Uint32 nPurgeDays = TB(pSynchInfo->nPurgeDays);
			nOffset += fp->Write(nOffset, &nPurgeDays, sizeof(nPurgeDays));

			// max articles
			Uint32 nMaxArticles = TB(pSynchInfo->nMaxArticles);
			nOffset += fp->Write(nOffset, &nMaxArticles, sizeof(nMaxArticles));

			// last synch
			Uint32 nDateOffset = nOffset;
			SCalendarDate stLastSynch = pSynchInfo->stLastSynch;
			stLastSynch.year = TB(stLastSynch.year);
			stLastSynch.month = TB(stLastSynch.month);
			stLastSynch.day = TB(stLastSynch.day);
			stLastSynch.hour = TB(stLastSynch.hour);
			stLastSynch.minute = TB(stLastSynch.minute);
			stLastSynch.second = TB(stLastSynch.second);
			stLastSynch.weekDay = TB(stLastSynch.weekDay);
			nOffset += fp->Write(nOffset, &stLastSynch, sizeof(SCalendarDate));
						
			SFlattenRef *pFlattenRef = pSynchInfo->pHotlineFile->FlattenRef();
			nSize = sizeof(SFlattenRef) + TB(pFlattenRef->dataSize);
			nOffset += fp->Write(nOffset, pFlattenRef, nSize);
			UMemory::Dispose((TPtr)pFlattenRef);
		
			// update servers list
			if (!AddServerList(*pSynchInfo) || pSynchInfo->nActive != 1)
			{
				UFileSys::Dispose(pSynchInfo->pHotlineFile);
				UMemory::Dispose((TPtr)pSynchInfo);
			}
			else
			{
				SMyOffSynchInfo *pOffSynchInfo = (SMyOffSynchInfo *)UMemory::New(sizeof(SMyOffSynchInfo));

				pOffSynchInfo->nDateOffset = nDateOffset;
				pOffSynchInfo->pSynchInfo = pSynchInfo;
				
				mSynchList.AddItem(pOffSynchInfo);
			}
		}
		
		inSynchList->Clear();
		fp->Close();
	}
	catch(...)
	{
		inSynchList->Clear();
		fp->Close();
		fp->DeleteFile();
		throw;
	}
}

void CMyApplication::WriteLastSynch(Uint32 inDateOffset, SCalendarDate& inLastSynch)
{
	// get FS ref for synch file
	TFSRefObj* fp = GetSynchRef();
	if (!fp) return;
	
	scopekill(TFSRefObj, fp);
	if (!fp->Exists()) return;
	
	SCalendarDate stLastSynch = inLastSynch;
	stLastSynch.year = TB(stLastSynch.year);
	stLastSynch.month = TB(stLastSynch.month);
	stLastSynch.day = TB(stLastSynch.day);
	stLastSynch.hour = TB(stLastSynch.hour);
	stLastSynch.minute = TB(stLastSynch.minute);
	stLastSynch.second = TB(stLastSynch.second);
	stLastSynch.weekDay = TB(stLastSynch.weekDay);

	fp->Open(perm_ReadWrite);
	
	try
	{
		fp->Write(inDateOffset, &stLastSynch, sizeof(SCalendarDate));
		fp->Close();
	}
	catch(...)
	{
		fp->Close();
		throw;
	}
}

void CMyApplication::ReadSynchInfo(CPtrList<SMySynchInfo> *outSynchList)
{
	TFSRefObj* fp = GetSynchRef();
	if (!fp) return;
	
	scopekill(TFSRefObj, fp);
	if (!fp->Exists()) return;
		
	StFileOpener fop(fp, perm_Read);

	// read synch file
	try
	{
		Uint16 nSynchVersion;
		Uint32 nOffset = fp->Read(0, &nSynchVersion, 2);
		
		if (nSynchVersion != TB((Uint16)kMySynchVersion))
			return;
				
		Uint32 nSynchCount = 0;
		nOffset += fp->Read(nOffset, &nSynchCount, 4);
		nSynchCount = TB(nSynchCount);
	
		if (nSynchCount <= 0)
		{
			fp->DeleteFile();
			return;
		}

		Uint32 nFlattenSize;
		SFlattenRef stFlattenRef;
	
		while (nSynchCount--)
		{
			SMySynchInfo *pSynchInfo = (SMySynchInfo *)UMemory::NewClear(sizeof(SMySynchInfo));
			
			Uint32 nSize = sizeof(pSynchInfo->nActive) + sizeof(pSynchInfo->stServerInfo) + sizeof(pSynchInfo->psGroupName) + sizeof(pSynchInfo->psHotlineServer) + sizeof(pSynchInfo->nSynchType);
			if (fp->Read(nOffset, pSynchInfo, nSize) != nSize)
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}
			
			nOffset += nSize;

			// synch hours
			if (fp->Read(nOffset, &pSynchInfo->nSynchHours, sizeof(pSynchInfo->nSynchHours)) != sizeof(pSynchInfo->nSynchHours))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}
			
			nOffset += sizeof(pSynchInfo->nSynchHours);
			pSynchInfo->nSynchHours = TB(pSynchInfo->nSynchHours);

			// purge days
			if (fp->Read(nOffset, &pSynchInfo->nPurgeDays, sizeof(pSynchInfo->nPurgeDays)) != sizeof(pSynchInfo->nPurgeDays))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}
			
			nOffset += sizeof(pSynchInfo->nPurgeDays);
			pSynchInfo->nPurgeDays = TB(pSynchInfo->nPurgeDays);

			// max articles
			if (fp->Read(nOffset, &pSynchInfo->nMaxArticles, sizeof(pSynchInfo->nMaxArticles)) != sizeof(pSynchInfo->nMaxArticles))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}
			
			nOffset += sizeof(pSynchInfo->nMaxArticles);
			pSynchInfo->nMaxArticles = TB(pSynchInfo->nMaxArticles);

			// last synch
			if (fp->Read(nOffset, &pSynchInfo->stLastSynch, sizeof(SCalendarDate)) != sizeof(SCalendarDate))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}

			nOffset += sizeof(SCalendarDate);
			pSynchInfo->stLastSynch.year = TB(pSynchInfo->stLastSynch.year);
			pSynchInfo->stLastSynch.month = TB(pSynchInfo->stLastSynch.month);
			pSynchInfo->stLastSynch.day = TB(pSynchInfo->stLastSynch.day);
			pSynchInfo->stLastSynch.hour = TB(pSynchInfo->stLastSynch.hour);
			pSynchInfo->stLastSynch.minute = TB(pSynchInfo->stLastSynch.minute);
			pSynchInfo->stLastSynch.second = TB(pSynchInfo->stLastSynch.second);
			pSynchInfo->stLastSynch.weekDay = TB(pSynchInfo->stLastSynch.weekDay);

			if (fp->Read(nOffset, &stFlattenRef, sizeof(SFlattenRef)) != sizeof(SFlattenRef))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}

			nFlattenSize = sizeof(SFlattenRef) + TB(stFlattenRef.dataSize);
			SFlattenRef *pFlattenRef = (SFlattenRef *)UMemory::New(nFlattenSize);
			if (!pFlattenRef)
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}
					
			if (fp->Read(nOffset, pFlattenRef, nFlattenSize) != nFlattenSize)
			{
				UMemory::Dispose((TPtr)pFlattenRef);
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}

			nOffset += nFlattenSize;
			
			try
			{
				pSynchInfo->pHotlineFile = UFileSys::UnflattenRef(pFlattenRef);
			}
			catch(...)
			{
				pSynchInfo->pHotlineFile = nil; // don't throw here
			}
			
			UMemory::Dispose((TPtr)pFlattenRef);

			// update servers list
			if (!AddServerList(*pSynchInfo))
			{
				UFileSys::Dispose(pSynchInfo->pHotlineFile);
				UMemory::Dispose((TPtr)pSynchInfo);
			}
			else
				outSynchList->AddItem(pSynchInfo);
		}
	}
	catch(...)
	{
		fp->DeleteFile();
		throw;
	}	
}

void CMyApplication::ReadSynchList()
{
	TFSRefObj* fp = GetSynchRef();
	if (!fp) return;

	scopekill(TFSRefObj, fp);
	if (!fp->Exists()) return;
		
	StFileOpener fop(fp, perm_Read);

	// read synch file
	try
	{
		Uint16 nSynchVersion;
		Uint32 nOffset = fp->Read(0, &nSynchVersion, 2);

		if (nSynchVersion != TB((Uint16)kMySynchVersion))
			return;
				
		Uint32 nSynchCount = 0;
		nOffset += fp->Read(nOffset, &nSynchCount, 4);
		nSynchCount = TB(nSynchCount);

		if (nSynchCount <= 0)
		{
			fp->DeleteFile();
			return;
		}
		
		Uint32 nFlattenSize;
		SFlattenRef stFlattenRef;

		while (nSynchCount--)
		{					
			SMySynchInfo *pSynchInfo = (SMySynchInfo *)UMemory::NewClear(sizeof(SMySynchInfo));
			
			Uint32 nSize = sizeof(pSynchInfo->nActive) + sizeof(pSynchInfo->stServerInfo) + sizeof(pSynchInfo->psGroupName) + sizeof(pSynchInfo->psHotlineServer) + sizeof(pSynchInfo->nSynchType);
			if (fp->Read(nOffset, pSynchInfo, nSize) != nSize)
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}
			
			nOffset += nSize;

			// synch hours
			if (fp->Read(nOffset, &pSynchInfo->nSynchHours, sizeof(pSynchInfo->nSynchHours)) != sizeof(pSynchInfo->nSynchHours))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}
			
			nOffset += sizeof(pSynchInfo->nSynchHours);
			pSynchInfo->nSynchHours = TB(pSynchInfo->nSynchHours);

			// purge days
			if (fp->Read(nOffset, &pSynchInfo->nPurgeDays, sizeof(pSynchInfo->nPurgeDays)) != sizeof(pSynchInfo->nPurgeDays))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}
			
			nOffset += sizeof(pSynchInfo->nPurgeDays);
			pSynchInfo->nPurgeDays = TB(pSynchInfo->nPurgeDays);

			// max articles
			if (fp->Read(nOffset, &pSynchInfo->nMaxArticles, sizeof(pSynchInfo->nMaxArticles)) != sizeof(pSynchInfo->nMaxArticles))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}
			
			nOffset += sizeof(pSynchInfo->nMaxArticles);
			pSynchInfo->nMaxArticles = TB(pSynchInfo->nMaxArticles);

			// last synch
			Uint32 nDateOffset = nOffset;
			if (fp->Read(nOffset, &pSynchInfo->stLastSynch, sizeof(SCalendarDate)) != sizeof(SCalendarDate))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}

			nOffset += sizeof(SCalendarDate);
			pSynchInfo->stLastSynch.year = TB(pSynchInfo->stLastSynch.year);
			pSynchInfo->stLastSynch.month = TB(pSynchInfo->stLastSynch.month);
			pSynchInfo->stLastSynch.day = TB(pSynchInfo->stLastSynch.day);
			pSynchInfo->stLastSynch.hour = TB(pSynchInfo->stLastSynch.hour);
			pSynchInfo->stLastSynch.minute = TB(pSynchInfo->stLastSynch.minute);
			pSynchInfo->stLastSynch.second = TB(pSynchInfo->stLastSynch.second);
			pSynchInfo->stLastSynch.weekDay = TB(pSynchInfo->stLastSynch.weekDay);

			if (fp->Read(nOffset, &stFlattenRef, sizeof(SFlattenRef)) != sizeof(SFlattenRef))
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}

			nFlattenSize = sizeof(SFlattenRef) + TB(stFlattenRef.dataSize);
			SFlattenRef *pFlattenRef = (SFlattenRef *)UMemory::New(nFlattenSize);
			if (!pFlattenRef)
			{
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}

			if (fp->Read(nOffset, pFlattenRef, nFlattenSize) != nFlattenSize)
			{
				UMemory::Dispose((TPtr)pFlattenRef);
				UMemory::Dispose((TPtr)pSynchInfo);
				return;
			}

			nOffset += nFlattenSize;
			
			try
			{
				pSynchInfo->pHotlineFile = UFileSys::UnflattenRef(pFlattenRef);
			}
			catch(...)
			{
				pSynchInfo->pHotlineFile = nil; // don't throw here
			}
			
			UMemory::Dispose((TPtr)pFlattenRef);

			// update servers list
			if (!AddServerList(*pSynchInfo) || pSynchInfo->nActive != 1)
			{
				UFileSys::Dispose(pSynchInfo->pHotlineFile);
				UMemory::Dispose((TPtr)pSynchInfo);
			}
			else
			{
				SMyOffSynchInfo *pOffSynchInfo = (SMyOffSynchInfo *)UMemory::New(sizeof(SMyOffSynchInfo));

				pOffSynchInfo->nDateOffset = nDateOffset;
				pOffSynchInfo->pSynchInfo = pSynchInfo;

				mSynchList.AddItem(pOffSynchInfo);
			}
		}
	}
	catch(...)
	{
		fp->DeleteFile();
		throw;
	}	
}

void CMyApplication::ClearSynchList()
{
	Uint32 i = 0;
	SMyOffSynchInfo *pOffSynchInfo;
	
	while (mSynchList.GetNext(pOffSynchInfo, i))
	{
		UFileSys::Dispose(pOffSynchInfo->pSynchInfo->pHotlineFile);
		UMemory::Dispose((TPtr)pOffSynchInfo->pSynchInfo);
		UMemory::Dispose((TPtr)pOffSynchInfo);
	}

	mSynchList.Clear();
}

void CMyApplication::ClearSynchList(CPtrList<SMySynchInfo> *inSynchList)
{
	Uint32 i = 0;
	SMySynchInfo *pSynchInfo;
				
	while (inSynchList->GetNext(pSynchInfo, i))
	{
		UFileSys::Dispose(pSynchInfo->pHotlineFile);
		UMemory::Dispose((TPtr)pSynchInfo);
	}

	inSynchList->Clear();
}

#pragma mark -

bool CMyApplication::AddServerList(const SMySynchInfo& inSynchInfo)
{
	if (!inSynchInfo.pHotlineFile)
		return false;
	
	TFSRefObj* pBundleRef;
	try
	{
		pBundleRef = inSynchInfo.pHotlineFile->GetFolder();
	}
	catch (...)
	{
		pBundleRef = nil;	// don't throw here
	}
	
	if (!pBundleRef)
		return false;
	
	scopekill(TFSRefObj, pBundleRef);
	
	bool bIsFolder;
	if (!pBundleRef->Exists(&bIsFolder) || !bIsFolder)
		return false;
		
	AddServerList1(inSynchInfo.stServerInfo);
	AddServerList2(inSynchInfo.psHotlineServer, pBundleRef);
	
	return true;
}

void CMyApplication::AddServerList1(const SMyServerInfo& inServerInfo)
{
	Uint32 i = 0;
	SMyServerInfo *pServerInfo;
	
	while (mServerList1.GetNext(pServerInfo, i))
	{
		if (!UText::CompareInsensitive(pServerInfo->psServerAddr + 1, pServerInfo->psServerAddr[0], inServerInfo.psServerAddr + 1, inServerInfo.psServerAddr[0]) &&
			!UText::CompareInsensitive(pServerInfo->psUserLogin + 1, pServerInfo->psUserLogin[0], inServerInfo.psUserLogin + 1, inServerInfo.psUserLogin[0]) &&
			!UText::CompareInsensitive(pServerInfo->psUserPassword + 1, pServerInfo->psUserPassword[0], inServerInfo.psUserPassword + 1, inServerInfo.psUserPassword[0]))
			return;
	}
	
	pServerInfo = (SMyServerInfo *)UMemory::New(sizeof(SMyServerInfo));
	if (!pServerInfo)
		return;
		
	UMemory::Copy(pServerInfo->psServerAddr, inServerInfo.psServerAddr, inServerInfo.psServerAddr[0] + 1);
	UMemory::Copy(pServerInfo->psUserLogin, inServerInfo.psUserLogin, inServerInfo.psUserLogin[0] + 1);
	UMemory::Copy(pServerInfo->psUserPassword, inServerInfo.psUserPassword, inServerInfo.psUserPassword[0] + 1);
	
	mServerList1.AddItem(pServerInfo);
}

void CMyApplication::AddServerList2(const Uint8 *inServerName, TFSRefObj* inBundleRef)
{
	Uint32 i = 0;
	SMyServerBundleInfo *pServerBundleInfo;
	
	while (mServerList2.GetNext(pServerBundleInfo, i))
	{
		if (!UText::CompareInsensitive(pServerBundleInfo->psServerName + 1, pServerBundleInfo->psServerName[0], inServerName + 1, inServerName[0]))
		{
			Uint32 j = 0;
			TFSRefObj* pBundleRef;
		
			while (pServerBundleInfo->lBundleList.GetNext(pBundleRef, j))
			{
				if (UFileSys::Equals(pBundleRef, inBundleRef))
					return;			
			}
			
			pServerBundleInfo->lBundleList.AddItem(inBundleRef->Clone());
			return;
		}
	}

	pServerBundleInfo = (SMyServerBundleInfo *)UMemory::NewClear(sizeof(SMyServerBundleInfo));
	if (!pServerBundleInfo)
		return;
	
	UMemory::Copy(pServerBundleInfo->psServerName, inServerName, inServerName[0] + 1);
	pServerBundleInfo->lBundleList.AddItem(inBundleRef->Clone());

	mServerList2.AddItem(pServerBundleInfo);
}

Uint32 CMyApplication::SearchServerList2(TFSRefObj* inBundleRef, Uint8 *outServerName, Uint32 inMaxSize)
{
	if (!inBundleRef || !outServerName || !inMaxSize)
		return 0;
	
	Uint32 i = 0;
	SMyServerBundleInfo *pServerBundleInfo;
	
	while (mServerList2.GetNext(pServerBundleInfo, i))
	{
		Uint32 j = 0;
		TFSRefObj* pBundleRef;
		
		while (pServerBundleInfo->lBundleList.GetNext(pBundleRef, j))
		{
			if (UFileSys::Equals(pBundleRef, inBundleRef))
				return UMemory::Copy(outServerName, pServerBundleInfo->psServerName + 1, pServerBundleInfo->psServerName[0] > inMaxSize ? inMaxSize : pServerBundleInfo->psServerName[0]);
		}			
	}
	
	return 0;
}

void CMyApplication::ClearServerList1()
{
	Uint32 i = 0;
	SMyServerInfo *pServerInfo;
	
	while (mServerList1.GetNext(pServerInfo, i))
		UMemory::Dispose((TPtr)pServerInfo);

	mServerList1.Clear();	
}

void CMyApplication::ClearServerList2()
{
	Uint32 i = 0;
	SMyServerBundleInfo *pServerBundleInfo;
	
	while (mServerList2.GetNext(pServerBundleInfo, i))
	{
		Uint32 j = 0;
		TFSRefObj* pBundle;
		
		while (pServerBundleInfo->lBundleList.GetNext(pBundle, j))
			UFileSys::Dispose(pBundle);
		
		pServerBundleInfo->lBundleList.Clear();
		UMemory::Dispose((TPtr)pServerBundleInfo);
	}

	mServerList2.Clear();
}

