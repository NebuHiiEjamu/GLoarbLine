/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "Hotline.h"

#if WIN32
void _SetWinIcon(TWindow inRef, Int16 inID);
#endif

extern const Uint8 *gDefaultBanner[];


/* ������������������������������������������������������������������������� */

CMyTask::CMyTask(const Uint8 inDesc[])
{
	mStage = 1;
	mIsFinished = mIsStandAlone = false;
	
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

bool CMyTask::QueueUnder()
{
	return false;
}

Uint32 CMyTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	#pragma unused(outText, inMaxSize)

	return 0;
}

void CMyTask::ShowProgress(Uint32 inVal, Uint32 inMax, const Uint8 inDesc[])
{
	gApp->SetTaskProgress(this, inVal, inMax, inDesc);
}

void CMyTask::Finish()
{
	mIsFinished = true;
	gApp->FinishTask(this);
}

void CMyTask::CheckServerError(Uint32 inError, TFieldData inData)
{
	gApp->CheckServerError(inError, inData);
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyTransactTask::CMyTransactTask(const Uint8 inDesc[])
	: CMyTask(inDesc), mTranSession(nil)
{

}

CMyTransactTask::~CMyTransactTask()
{
	delete mTranSession;
}

TTransactSession CMyTransactTask::NewSendSession(Uint16 inType)
{
	return gApp->GetTransactor()->NewSendSession(inType);
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyDummyTask::CMyDummyTask(const Uint8 inDesc[], bool inIsLoginTask)
	: CMyTask(inDesc)
{
	mIsLoginTask = inIsLoginTask;

	ShowProgress(1, 2);
}

bool CMyDummyTask::IsLoginTask()
{
	return mIsLoginTask;
}

void CMyDummyTask::FinishDummyTask()
{
	Finish();
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyConnectTask::CMyConnectTask()
	: CMyTask("\pConnecting to Server...")
{
	gApp->MakeTransactor();
	gApp->ScheduleTaskProcess();
}

Uint32 CMyConnectTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return UText::Format(outText, inMaxSize, "Connecting to %#s...", gApp->mServerName);
}

CMyConnectTask::~CMyConnectTask()
{
	if (!mIsFinished)
	{
		gApp->DisposeTransactor();
		gApp->ShowNotConnected();
	}
}

void CMyConnectTask::Process()
{
	switch (mStage)
	{
		// send off request for connection
		case 1:
		{
			gApp->StartTransactorConnect();
			
			gApp->ShowConnecting();
			ShowProgress(1, 3);
			
			mStage++;
		}
		break;
		
		// wait until connection is established
		case 2:
		{
			Uint16 stat;
			
			try
			{
				stat = gApp->GetTransactor()->GetConnectStatus();
			}
			catch(SError& err)
			{
				if (err.type == errorType_Misc && err.id == error_VersionUnknown)
				{
					UApplication::PostMessage(1107);
					SilentFail(errorType_Misc, error_VersionUnknown);
				}
				throw;
			}
			
			if (stat == kEstablishingConnection)
			{
				ShowProgress(2, 3);
			}
			else if (stat == kConnectionEstablished)
			{
				ShowProgress(3, 3);
				Finish();
				
				gApp->mIsConnected = true;
				new CMyLoginTask(gApp->mUserLogin, gApp->mUserPassword);
			}
		}
		break;
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyGetOldNewsTask::CMyGetOldNewsTask()
	: CMyTransactTask("\pTransferring messages...")
{
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_GetMsgs);
	mTranSession->SetTimeout(400000);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyGetOldNewsTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pGetting Messages...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMyGetOldNewsTask::Process()
{
	Uint32 error, s;
	THdl h;
	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());

	// if server has replied with message text
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// allocate handle to store text
		s = mFieldData->GetFieldSize(myField_Data);
		h = UMemory::NewHandle(s);
		
		// read and display text
		try
		{
			// read text into handle
			{
				void *p;
				StHandleLocker lock(h, p);
				mFieldData->GetField(myField_Data, p, s);
			}
			
			// display text in main window
			if (!gApp->mOldNewsWin)
			{
				gApp->mOldNewsWin = new CMyOldNewsWin(gApp->mAuxParentWin);
				Uint8 str[256];
				str[0] = UText::Format(str+1, sizeof(str)-1, "News (%#s)", gApp->mServerName);
				gApp->mOldNewsWin->SetTitle(str);
				gApp->mOldNewsWin->SetTextColor(gApp->mOptions.ColorNews);
				gApp->mOldNewsWin->SetAutoBounds(windowPos_TopLeft, windowPosOn_MainScreen);
				gApp->mOldNewsWin->Show();
			}
			
			CTextView *v = gApp->mOldNewsWin->GetTextView();
			v->SetTextHandle(h);
			h = nil;
			v->SetFullHeight();
			v->SetSelection(0, 0);
		}
		catch(...)
		{
			UMemory::Dispose(h);
			throw;
		}
		
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyGetFileListTask::CMyGetFileListTask(const void *inPathData, Uint32 inPathSize, bool inListParent)
	: CMyTransactTask("\p"), mPathData(nil), mPathSize(0)
{
	mName[0] = 0;
	mListParent = inListParent;
	
	try
	{
		// save path (we'll need it later)
		if (inPathData && inPathSize)
		{
			mPathData = UMemory::New(inPathData, inPathSize);
			mPathSize = inPathSize;
		}
		
		// ask server for file listing
		if (inPathData) mFieldData->AddField(myField_FilePath, inPathData, inPathSize);
		mTranSession = NewSendSession(myTran_GetFileNameList);
		mTranSession->SendData(mFieldData);

		// set name in tasks window
		if (!inPathData)
			ShowProgress(0, 1, "\pListing root folder...");
		else
		{
			Uint8 str[256];
			mName[0] = UFS::GetPathTargetName(inPathData, inPathSize, mName+1, sizeof(mName)-1);
			str[0] = UText::Format(str+1, sizeof(str)-1, "Listing folder �%#s�", mName);
			ShowProgress(0, 1, str);
		}
	}
	catch(...)
	{
		// clean up (CMyTransactTask will dispose mTranSession)
		UMemory::Dispose(mPathData);
		throw;
	}
}

CMyGetFileListTask::~CMyGetFileListTask()
{
	if (mPathData)	
		UMemory::Dispose(mPathData);
}

bool CMyGetFileListTask::IsThisPathData(const void *inPathData, Uint32 inPathSize)
{
	if (!UMemory::Compare(mPathData, mPathSize, inPathData, inPathSize))
		return true;

	return false;
}

Uint32 CMyGetFileListTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Listing folder: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Listing root folder...");
}

void CMyGetFileListTask::Process()
{
	Uint32 error;
	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());
	
	// if server has replied
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		if (error && mListParent && mPathData && mPathSize)
		{
			Uint32 nPathSize;
			void *pPathData = UFileSys::GetParentPathData(mPathData, mPathSize, nPathSize);

			new CMyGetFileListTask(pPathData, nPathSize, false);
			UMemory::Dispose((TPtr)pPathData);
		}
		else
		{
			// check for server errors
			CheckServerError(error, mFieldData);
		
			// extract file listing into list view
			gApp->SetFolderWinContent(mPathData, mPathSize, mFieldData);
		}

		Finish();
	}
}


/* ������������������������������������������������������������������������� */
#pragma mark -

CMyStartupGetFileListTask::CMyStartupGetFileListTask(const void *inPathData, Uint32 inPathSize, Uint8 *inFileName)
	: CMyGetFileListTask(inPathData, inPathSize)
{
	mFileName[0] = 0;
	
	if (inFileName && inFileName[0])
		UMemory::Copy(mFileName, inFileName, inFileName[0] + 1);
}

void CMyStartupGetFileListTask::Process()
{	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());
	
	// if server has replied
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		Uint32 error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		if (error)
		{
			Finish();
			
			if (!mPathData || !mPathSize || mFileName[0])
			{
				gApp->DisplayStandardMessage("\pServer Message", "\pThe specified folder could not be found.", icon_Caution, 1);
				return;
			}
						
			mFileName[0] = UFS::GetPathTargetName(mPathData, mPathSize, mFileName + 1, sizeof(mFileName) - 1);
			Uint32 nNewPathSize = UFS::GetPathSize(mPathData, FB(*(Uint16 *)mPathData) - 1);
	
			if (nNewPathSize <= 2)	// if parent is root
				new CMyStartupGetFileListTask(nil, 0, mFileName);
			else
			{
			#if MACINTOSH
				(*(Uint16 *)mPathData)--;
			#else
				*(Uint16 *)mPathData = TB( (Uint16)(FB(*(Uint16 *)mPathData) - 1));
			#endif
		
				new CMyStartupGetFileListTask(mPathData, nNewPathSize, mFileName);
			}
		
			return;
		}
		
		CMyItemsWin *pFileWin = nil;	
		if (!mPathData)
			pFileWin = gApp->GetRootFolderWin();
		else
			pFileWin = gApp->GetFolderWinByPath(mPathData, mPathSize);
		
		if (pFileWin)
		{
			if (gApp->mOptions.nFilesWindow == optWindow_List)
				((CMyFileListWin *)pFileWin)->GetFileListView()->DeleteAll();
			else if (gApp->mOptions.nFilesWindow == optWindow_Tree)
				((CMyFileTreeWin *)pFileWin)->GetFileTreeView()->DeleteAll();
			else
				((CMyFileExplWin *)pFileWin)->GetFileTreeView()->DeleteAll();

			pFileWin->BringToFront();
		}
		else
		{
			TPtr pPathData;
			Uint8 psWinName[256];
			Uint32 nCheckSum;
				
			if (!mPathData)
			{
				pPathData = nil;
				psWinName[0] = UMemory::Copy(psWinName + 1, "Files", 5);
				nCheckSum = 0;
			}
			else
			{
				pPathData = UMemory::New(mPathSize);
				UMemory::Copy(pPathData, mPathData, mPathSize);
				psWinName[0] = UFS::GetPathTargetName(pPathData, mPathSize, psWinName + 1, sizeof(psWinName) - 1);
				nCheckSum = UMemory::Checksum(pPathData, mPathSize);
			}

			if (gApp->mOptions.nFilesWindow == optWindow_List)
				pFileWin = new CMyFileListWin(gApp->mAuxParentWin, pPathData, mPathSize, nCheckSum);
			else if (gApp->mOptions.nFilesWindow == optWindow_Tree)
				pFileWin = new CMyFileTreeWin(gApp->mAuxParentWin, pPathData, mPathSize, nCheckSum);
			else
				pFileWin = new CMyFileExplWin(gApp->mAuxParentWin, pPathData, mPathSize, nCheckSum);

		#if WIN32
			_SetWinIcon(*pFileWin, 401);
		#endif

			if (gApp->mWindowPat) pFileWin->SetPattern(gApp->mWindowPat);
			pFileWin->SetTitle(psWinName);
			pFileWin->SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);
			pFileWin->Show();
		}
		
		if (gApp->mOptions.nFilesWindow == optWindow_List)
			((CMyFileListWin *)pFileWin)->SetContent(mPathData, mPathSize, mFieldData);
		else if (gApp->mOptions.nFilesWindow == optWindow_Tree)
			((CMyFileTreeWin *)pFileWin)->SetContent(mPathData, mPathSize, mFieldData);
		else
			((CMyFileExplWin *)pFileWin)->SetContent(mPathData, mPathSize, mFieldData);

		Finish();
		
		if (mFileName[0])
		{
			bool bFound = false;
			if (gApp->mOptions.nFilesWindow == optWindow_List)
			{
				if (((CMyFileListWin *)pFileWin)->GetFileListView()->SelectNames(mFileName))
					bFound = true;
			}
			else if (gApp->mOptions.nFilesWindow == optWindow_Tree)
			{
				if (((CMyFileTreeWin *)pFileWin)->GetFileTreeView()->SelectNames(mFileName))
					bFound = true;
			}
			else
			{
				// this tree view display just folders (???)
				if (!((CMyFileExplWin *)pFileWin)->GetFileTreeView()->SelectNames(mFileName))
					bFound = true;
			}
			
			if (bFound)
				(dynamic_cast<CMyFileWin *>(pFileWin))->DoDownload(0);
			else
				gApp->DisplayStandardMessage("\pServer Message", "\pThe specified file could not be found.", icon_Caution, 1);
		}
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyGetNewsCatListTask::CMyGetNewsCatListTask(const void *inPathData, Uint32 inPathSize, bool inListParent)
	: CMyTransactTask("\p"), mPathData(nil), mPathSize(0)
{
	mName[0] = 0;
	mListParent = inListParent;

	try
	{
		// save path (we'll need it later)
		if (inPathData && inPathSize)
		{
			mPathData = UMemory::New(inPathData, inPathSize);
			mPathSize = inPathSize;
		}
		
		// ask server for file listing
		if (inPathData) mFieldData->AddField(myField_NewsPath, inPathData, inPathSize);
		mTranSession = NewSendSession(myTran_GetNewsCatNameList);
		mTranSession->SendData(mFieldData);

		// set name in tasks window
		if (!inPathData)
			ShowProgress(0, 1, "\pListing root news bundle...");
		else
		{
			Uint8 str[256];
			Uint8 name[256];
			name[0] = UFS::GetPathTargetName(inPathData, inPathSize, name+1, sizeof(name)-1);
			str[0] = UText::Format(str+1, sizeof(str)-1, "Listing news bundle �%#s�", name);
			ShowProgress(0, 1, str);
		}
	}
	catch(...)
	{
		// clean up (CMyTransactTask will dispose mTranSession)
		UMemory::Dispose(mPathData);
		throw;
	}
}

CMyGetNewsCatListTask::~CMyGetNewsCatListTask()
{
	if (mPathData)	
		UMemory::Dispose(mPathData);
}

Uint32 CMyGetNewsCatListTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Listing news category: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Listing news category...");
}

void CMyGetNewsCatListTask::Process()
{
	Uint32 error;
	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());
	
	// if server has replied
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		if (error && mListParent && mPathData && mPathSize)
		{
			Uint32 nPathSize;
			void *pPathData = UFileSys::GetParentPathData(mPathData, mPathSize, nPathSize);

			new CMyGetNewsCatListTask(pPathData, nPathSize, false);
			UMemory::Dispose((TPtr)pPathData);
		}
		else
		{
			// check for server errors
			CheckServerError(error, mFieldData);
		
			// extract category listing into list view
			gApp->SetBundleWinContent(mPathData, mPathSize, mFieldData);		
		}
		
		Finish();
	}
}


/* ������������������������������������������������������������������������� */
#pragma mark -

CMyStartupGetNewsCatListTask::CMyStartupGetNewsCatListTask(const void *inPathData, Uint32 inPathSize, Uint8 *inCategoryName)
	: CMyGetNewsCatListTask(inPathData, inPathSize)
{
	mCategoryName[0] = 0;
	
	if (inCategoryName && inCategoryName[0])
		UMemory::Copy(mCategoryName, inCategoryName, inCategoryName[0] + 1);
}

void CMyStartupGetNewsCatListTask::Process()
{	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());
	
	// if server has replied
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		Uint32 error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;
		
		if (error)
		{
			Finish();
			
			if (!mPathData || !mPathSize || mCategoryName[0])
			{
				gApp->DisplayStandardMessage("\pServer Message", "\pThe specified bundle could not be found.", icon_Caution, 1);
				return;
			}
						
			mCategoryName[0] = UFS::GetPathTargetName(mPathData, mPathSize, mCategoryName + 1, sizeof(mCategoryName) - 1);
			Uint32 nNewPathSize = UFS::GetPathSize(mPathData, FB(*(Uint16 *)mPathData) - 1);
	
			if (nNewPathSize <= 2)	// if parent is root
				new CMyStartupGetNewsCatListTask(nil, 0, mCategoryName);
			else
			{
			#if MACINTOSH
				(*(Uint16 *)mPathData)--;
			#else
				*(Uint16 *)mPathData = TB( (Uint16)(FB(*(Uint16 *)mPathData) - 1));
			#endif
			
				new CMyStartupGetNewsCatListTask(mPathData, nNewPathSize, mCategoryName);
			}
		
			return;
		}
	
		CMyItemsWin *pBundleWin = nil;	
		if (!mPathData)
			pBundleWin = gApp->GetRootNewsFolderWin();
		else
			pBundleWin = gApp->GetNewsFolderWinByPath(mPathData, mPathSize);
		
		if (pBundleWin)
		{
			if (gApp->mOptions.nNewsWindow == optWindow_List)
				((CMyNewsCategoryListWin *)pBundleWin)->GetNewsCategoryListView()->DeleteAll();
			else if (gApp->mOptions.nNewsWindow == optWindow_Tree)
				((CMyNewsCategoryTreeWin *)pBundleWin)->GetNewsCategoryTreeView()->DeleteAll();
			else
				((CMyNewsCategoryExplWin *)pBundleWin)->GetNewsCategoryTreeView()->DeleteAll();
		
			pBundleWin->BringToFront();
		}
		else
		{
			TPtr pPathData;
			Uint8 psWinName[256];
			Uint32 nChecksum;
				
			if (!mPathData)
			{
				pPathData = nil;
				psWinName[0] = UMemory::Copy(psWinName + 1, "News", 4);
				nChecksum = 0;
			}
			else
			{
				pPathData = UMemory::New(mPathSize);
				UMemory::Copy(pPathData, mPathData, mPathSize);
				psWinName[0] = UFS::GetPathTargetName(pPathData, mPathSize, psWinName + 1, sizeof(psWinName) - 1);
				nChecksum = UMemory::Checksum(pPathData, mPathSize);
			}

			if (gApp->mOptions.nNewsWindow == optWindow_List)
				pBundleWin = new CMyNewsCategoryListWin(gApp->mAuxParentWin, pPathData, mPathSize, nChecksum);
			else if (gApp->mOptions.nNewsWindow == optWindow_Tree)
				pBundleWin = new CMyNewsCategoryTreeWin(gApp->mAuxParentWin, pPathData, mPathSize, nChecksum);
			else
				pBundleWin = new CMyNewsCategoryExplWin(gApp->mAuxParentWin, pPathData, mPathSize, nChecksum);

			if (gApp->mWindowPat) pBundleWin->SetPattern(gApp->mWindowPat);
			pBundleWin->SetTitle(psWinName);
			pBundleWin->SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);
			pBundleWin->Show();
		}
		
		// extract file listing into list view
		if (gApp->mOptions.nNewsWindow == optWindow_List)
			((CMyNewsCategoryListWin *)pBundleWin)->SetContent(mPathData, mPathSize, mFieldData);
		else if (gApp->mOptions.nNewsWindow == optWindow_Tree)
			((CMyNewsCategoryTreeWin *)pBundleWin)->SetContent(mPathData, mPathSize, mFieldData);
		else
			((CMyNewsCategoryExplWin *)pBundleWin)->SetContent(mPathData, mPathSize, mFieldData);

		Finish();

		if (mCategoryName[0])
		{		
			bool bFound = false;
			if (gApp->mOptions.nNewsWindow == optWindow_List)
			{
				if (((CMyNewsCategoryListWin *)pBundleWin)->GetNewsCategoryListView()->SelectNames(mCategoryName))
					bFound = true;
			}
			else if (gApp->mOptions.nNewsWindow == optWindow_Tree)
			{
				if (((CMyNewsCategoryTreeWin *)pBundleWin)->GetNewsCategoryTreeView()->SelectNames(mCategoryName))
					bFound = true;
			}
			else
			{
				if (((CMyNewsCategoryExplWin *)pBundleWin)->GetNewsCategoryTreeView()->SelectNames(mCategoryName))
					bFound = true;
			}
			
			if (bFound)
				(dynamic_cast<CMyNewsCategoryWin *>(pBundleWin))->DoDoubleClick(0);
			else
				gApp->DisplayStandardMessage("\pServer Message", "\pThe specified category could not be found.", icon_Caution, 1);
		}
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -
CMyNewNewsFldrTask::CMyNewNewsFldrTask(const void *inPathData, Uint32 inPathSize, const Uint8 inName[])
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inName);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Create news bundle �%#s�", inName);
	ShowProgress(1, 2, str);

	// setup field data
	mFieldData->AddPString(myField_FileName, inName);
	mFieldData->AddField(myField_NewsPath, inPathData, inPathSize);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_NewNewsFldr);
	mTranSession->SendData(mFieldData);
	
	gApp->mCacheList.ChangedBundleList(inPathData, inPathSize);
}

Uint32 CMyNewNewsFldrTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return UText::Format(outText, inMaxSize, "Creating news bundle: %#s...", mName);
}

void CMyNewNewsFldrTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -
CMyNewNewsCatTask::CMyNewNewsCatTask(const void *inPathData, Uint32 inPathSize, const Uint8 inName[])
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inName);

	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Create news category �%#s�", inName);
	ShowProgress(1, 2, str);

	// setup field data
	mFieldData->AddPString(myField_NewsCatName, inName);
	mFieldData->AddField(myField_NewsPath, inPathData, inPathSize);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_NewNewsCat);
	mTranSession->SendData(mFieldData);

	gApp->mCacheList.ChangedBundleList(inPathData, inPathSize);
}

Uint32 CMyNewNewsCatTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return UText::Format(outText, inMaxSize, "Creating news category %#s...", mName);
}

void CMyNewNewsCatTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}


/* ������������������������������������������������������������������������� */
#pragma mark -

CMyDeleteNewsFldrItemTask::CMyDeleteNewsFldrItemTask(const Uint8 *inItemName, Uint16 inType, const void *inPathData, Uint32 inPathSize)
	: CMyTransactTask("\p")
{
	pstrcpy(mName, inItemName);
	
	Uint32 nNewPathSize;
	void *pNewPath = UFileSys::MakePathData(inPathData, inPathSize, inItemName, nNewPathSize);
	Uint32 nNewPathCS = UMemory::Checksum(pNewPath, nNewPathSize);

	Uint8 str[256];
	Uint8 *type = "\p";

	if (inType == 1)		// folder
	{
		CMyItemsWin *fWin = gApp->GetNewsFolderWinByPath(pNewPath, nNewPathSize, nNewPathCS);
		if (fWin)
			delete fWin;
			
		type = "\p news bundle";
	}
	else if (inType == 10)	// category
	{
		CMyNewsArticleTreeWin *cWin = gApp->GetNewsArticleListWinByPath(pNewPath, nNewPathSize, nNewPathCS);
		if (cWin)
			delete cWin;
			
		type = "\p news category";
	}
	
	mFieldData->AddField(myField_NewsPath, pNewPath, nNewPathSize);
	UMemory::Dispose((TPtr)pNewPath);
	
	mTranSession = NewSendSession(myTran_DelNewsItem);
	mTranSession->SendData(mFieldData);
	
	gApp->mCacheList.ChangedBundleList(inPathData, inPathSize);

	str[0] = UText::Format(str + 1, sizeof(str) - 1, "Deleting%#s %#s", type, inItemName);
	ShowProgress(1, 2, str);
}

Uint32 CMyDeleteNewsFldrItemTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return UText::Format(outText, inMaxSize, "Deleting news bundle %#s...", mName);
}

void CMyDeleteNewsFldrItemTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}


/* ������������������������������������������������������������������������� */
#pragma mark -
CMyGetNewsArtListTask::CMyGetNewsArtListTask(const void *inPathData, Uint32 inPathSize)
	: CMyTransactTask("\p"), mPathData(nil), mPathSize(0)
{
	mName[0] = 0;

	try
	{
		// save path (we'll need it later)
		if (inPathData && inPathSize)
		{
			mPathData = UMemory::New(inPathData, inPathSize);
			mPathSize = inPathSize;
		}
		else
			DebugBreak("I should never get nil here: CMyGetNewsArtListTask");
		
		// ask server for file listing
		if (inPathData) mFieldData->AddField(myField_NewsPath, inPathData, inPathSize);
		mTranSession = NewSendSession(myTran_GetNewsArtNameList);
		mTranSession->SendData(mFieldData);

		// set name in tasks window
		Uint8 str[256];
		mName[0] = UFS::GetPathTargetName(inPathData, inPathSize, mName+1, sizeof(mName)-1);
		str[0] = UText::Format(str+1, sizeof(str)-1, "Listing news category �%#s�", mName);
		ShowProgress(0, 1, str);
	}
	catch(...)
	{
		// clean up (CMyTransactTask will dispose mTranSession)
		UMemory::Dispose(mPathData);
		throw;
	}
}

CMyGetNewsArtListTask::~CMyGetNewsArtListTask()
{
	if (mPathData)
		UMemory::Dispose(mPathData);
}

Uint32 CMyGetNewsArtListTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Listing news category %#s...", mName) :
		UText::Format(outText, inMaxSize, "Listing news category...");
}

void CMyGetNewsArtListTask::Process()
{
	Uint32 error;
	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());
	
	// if server has replied
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// extract article listing into list view
		gApp->SetCategoryWinContent(mPathData, mPathSize, mFieldData);		
		
		Finish();
	}
}


/* ������������������������������������������������������������������������� */
#pragma mark -

CMyGetNewsArticleDataTask::CMyGetNewsArticleDataTask(const void *inPathData, Uint32 inPathSize, Uint32 inID, const Uint8 inArticTitle[], const Int8 inFlavor[], Uint32 inOptions)
	: CMyTransactTask("\p"), mPathData(nil), mPathSize(0), mID(0)
{
	mInfoAction = 0;
	mPathData = nil;

	if (inArticTitle)
		pstrcpy(mName, inArticTitle);
	else
		mName[0] = 0;
		
	try
	{
		// save path (we'll need it later)
		if (inPathData && inPathSize)
		{
			mPathData = UMemory::New(inPathData, inPathSize);
			mPathSize = inPathSize;
		}
		else
			DebugBreak("I should never get nil here: CMyGetNewsArticleDataTask");
		
		mID = inID;
		
		// ask server for article
		mFieldData->AddField(myField_NewsPath, inPathData, inPathSize);
		mFieldData->AddInteger(myField_NewsArtID, inID);
		mFieldData->AddCString(myField_NewsArtDataFlav, inFlavor);
		
		mTranSession = NewSendSession(myTran_GetNewsArtData);
		mTranSession->SendData(mFieldData);

		// set name in tasks window
		switch (inOptions)
		{
			case viewID_NewsArticGoNext:
				ShowProgress(0, 1, "\pGetting next article");
				mInfoAction = viewID_NewsArticGoNext;
				break;
				
			case viewID_NewsArticGoPrev:
				ShowProgress(0, 1, "\pGetting previous article");
				mInfoAction = viewID_NewsArticGoPrev;
				break;
				
			case viewID_NewsArticGoParent:
				ShowProgress(0, 1, "\pGetting parent article");
				mInfoAction = viewID_NewsArticGoParent;
				break;
				
			case viewID_NewsArticGo1stChild:
				ShowProgress(0, 1, "\pGetting first reply");
				mInfoAction = viewID_NewsArticGo1stChild;
				break;
				
			default:
				Uint8 str[256];
				str[0] = UText::Format(str+1, sizeof(str)-1, "Getting news article �%#s�", inArticTitle);
				ShowProgress(0, 1, str);
				mInfoAction = 0;
				break;	
		}
	}
	catch(...)
	{
		// clean up (CMyTransactTask will dispose mTranSession)
		UMemory::Dispose(mPathData);
		throw;
	}
}

CMyGetNewsArticleDataTask::~CMyGetNewsArticleDataTask()
{
	if (mPathData)
		UMemory::Dispose(mPathData);
}

Uint32 CMyGetNewsArticleDataTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 getPrevMsg[] = "\pGetting previous article...";
	const Uint8 getNextMsg[] = "\pGetting next article...";
	const Uint8 getParentMsg[] = "\pGetting parent article...";
	const Uint8 get1stChildMsg[] = "\pGetting first reply...";
	const Uint8 getMsg[] = "\pGetting news article...";

	switch (mInfoAction)
	{
		case viewID_NewsArticGoPrev:
			return UMemory::Copy(outText, getPrevMsg + 1, min(inMaxSize, (Uint32)getPrevMsg[0]));
			break;
		case viewID_NewsArticGoNext:
			return UMemory::Copy(outText, getNextMsg + 1, min(inMaxSize, (Uint32)getNextMsg[0]));
			break;
		case viewID_NewsArticGoParent:
			return UMemory::Copy(outText, getParentMsg + 1, min(inMaxSize, (Uint32)getParentMsg[0]));
			break;
		case viewID_NewsArticGo1stChild:
			return UMemory::Copy(outText, get1stChildMsg + 1, min(inMaxSize, (Uint32)get1stChildMsg[0]));
			break;
		default:
			return mName[0] ?
				UText::Format(outText, inMaxSize, "Getting news article: %#s...", mName) :
				UMemory::Copy(outText, getMsg + 1, min(inMaxSize, (Uint32)getMsg[0]));
			break;
	}
}

bool CMyGetNewsArticleDataTask::GetThisNewsArticle(const void *inPathData, Uint32 inPathSize, Uint32 inID)
{
	if (mID == inID && !UMemory::Compare(mPathData, mPathSize, inPathData, inPathSize))
		return true;
		
	return false;
}

void CMyGetNewsArticleDataTask::Process()
{
	Uint32 error;
	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());
	
	// if server has replied
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		gApp->SetArticleWinContent(mPathData, mPathSize, mID, mFieldData);
		
		Finish();
	}
}


/* ������������������������������������������������������������������������� */
#pragma mark -

CMyOldPostNewsTask::CMyOldPostNewsTask(const void *inData, Uint32 inDataSize)
	: CMyTransactTask("\pPosting message...")
{
	// CMyTransactTask will dispose mTranSession if we fail
	mFieldData->AddField(myField_Data, inData, inDataSize);
	mTranSession = NewSendSession(myTran_PostMsg);
	mTranSession->SendData(mFieldData);
	
	ShowProgress(1, 2);
}

void CMyOldPostNewsTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyNewsDeleteArticTask::CMyNewsDeleteArticTask(const void *inPathData, Uint32 inPathSize, Uint32 inArticID, Uint8 inTitle[], bool inDeleteChildren)
	: CMyTransactTask("\p")
{
	pstrcpy(mName, inTitle);
	
	Uint8 str[256];
	CMyNewsArticTextWin *win = gApp->GetNewsArticleWinByPathAndID(inArticID, inPathData, inPathSize);
	if (win)
		delete win;
	
	mFieldData->AddField(myField_NewsPath, inPathData, inPathSize);
	mFieldData->AddInteger(myField_NewsArtID, inArticID);
	mFieldData->AddInteger(myField_NewsArtRecurseDel, (Uint8)inDeleteChildren);
	
	mTranSession = NewSendSession(myTran_DelNewsArt);
	mTranSession->SendData(mFieldData);

	gApp->mCacheList.ChangedArticleList(inPathData, inPathSize);

	str[0] = UText::Format(str + 1, sizeof(str) - 1, "Deleting article %#s", inTitle);
	ShowProgress(1, 2, str);
}

Uint32 CMyNewsDeleteArticTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return UText::Format(outText, inMaxSize, "Deleting news article: %#s...", mName);
}

void CMyNewsDeleteArticTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyPostNewsTextArticle::CMyPostNewsTextArticle(const void *inPathData, Uint32 inPathSize, Uint32 inParentID, const Uint8 inTitle[], Uint32 inFlags, const Uint8 *inData, Uint32 inDataSize)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inTitle);
	
	mFieldData->AddField(myField_NewsPath, inPathData, inPathSize);
	mFieldData->AddInteger(myField_NewsArtID, inParentID);
	mFieldData->AddPString(myField_NewsArtTitle, inTitle);
	mFieldData->AddInteger(myField_NewsArtFlags, inFlags);
	mFieldData->AddCString(myField_NewsArtDataFlav, "text/plain");
	mFieldData->AddField(myField_NewsArtData, inData, inDataSize);
	
	mTranSession = NewSendSession(myTran_PostNewsArt);
	mTranSession->SendData(mFieldData);

	gApp->mCacheList.ChangedArticleList(inPathData, inPathSize);

	str[0] = UText::Format(str + 1, sizeof(str) - 1, "Posting article %#s", inTitle);
	ShowProgress(1, 2, str);
}

Uint32 CMyPostNewsTextArticle::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return UText::Format(outText, inMaxSize, "Posting news article: %#s...", mName);
}

void CMyPostNewsTextArticle::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMySendPrivMsgTask::CMySendPrivMsgTask( Uint16 inYourID, 
										const void *inMyMsg, Uint32 inMyMsgSize, 
										Uint16 inOptions, 
										const void* inYourMsg, Uint32 inYourMsgSize)
: CMyTransactTask("\pSending private message...")
{
	mFieldData->AddInteger(myField_UserID, inYourID);
	mFieldData->AddInteger(myField_Options, inOptions); // is an automatic response or user message
	
	if (inMyMsg && inMyMsgSize)
		mFieldData->AddField(myField_Data, inMyMsg, inMyMsgSize);
	
	if (inYourMsg && inYourMsgSize)
		mFieldData->AddField(myField_QuotingMsg, inYourMsg, inYourMsgSize);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_SendInstantMsg);
	mTranSession->SendData(mFieldData);
	
	
	ShowProgress(1, 2);
}

Uint32 CMySendPrivMsgTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pSending private message...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMySendPrivMsgTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyBroadcastTask::CMyBroadcastTask(const void *inData, Uint32 inDataSize)
	: CMyTransactTask("\pBroadcasting...")
{
	mFieldData->AddField(myField_Data, inData, inDataSize);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_UserBroadcast);
	mTranSession->SendData(mFieldData);
	
	ShowProgress(1, 2);
}

Uint32 CMyBroadcastTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pSending broadcast...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMyBroadcastTask::Process()
{
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		Uint32 error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyKeepConnectionAliveTask::CMyKeepConnectionAliveTask()
	: CMyTransactTask(nil)
{
	mTranSession = NewSendSession(myTran_KeepConnectionAlive);
	mTranSession->SendData(mFieldData);
}

void CMyKeepConnectionAliveTask::Process()
{		
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;
		
		// all done
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyGetOnlineUsersTask::CMyGetOnlineUsersTask(Uint8 *inDesc)
	: CMyTransactTask(inDesc)
{
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_GetUserNameList);
	mTranSession->SendData(mFieldData);

	if (inDesc)
		gApp->mUsersWin->GetUserListView()->SetStatus(listStat_Loading);
}

Uint32 CMyGetOnlineUsersTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pGetting list of online users...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMyGetOnlineUsersTask::Process()
{	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		Uint32 error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		gApp->SetOnlineUsers(mFieldData);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyLoginTask::CMyLoginTask(const Uint8 inLogin[], const Uint8 inPassword[])
	: CMyTransactTask("\pLogging in...")
{
	if (gCrypt)
		{
		delete gCrypt; //no crypt by default
		gCrypt = nil;
		}

	if(gApp->mUseCrypt)
		{
		//HOPE login:
		Uint8 zero=0;
		char MacString [] = {0,2,9,'H','M','A','C','-','S','H','A','1',8,'H','M','A','C','-','M','D','5'};
		char CipherString [] = {0,1,8,'B','L','O','W','F','I','S','H'};
		
		mStage=1;
		mFieldData->AddField(myField_UserLogin,&zero,1);
		mFieldData->AddField(myField_UserPassword,&zero,1);		
		mFieldData->AddField(myField_MacAlg,&MacString,12);		
		mFieldData->AddField(myField_C_CipherAlg,&CipherString,11);

		// CMyTransactTask will dispose mTranSession if we fail
		mTranSession = NewSendSession(myTran_Login);
		mTranSession->SendData(mFieldData);		
		}
	else
		{
		//normal login:
		Uint8 str[256];
		Uint8 *p;
		Uint32 s;		
		
		mStage=2;
		
		// scramble and store login
		UMemory::Copy(str, inLogin, inLogin[0]+1);
		s = str[0];
		p = str + 1;
		while (s--) { *p = ~(*p); p++; }
		mFieldData->AddPString(myField_UserLogin, str);

		// scramble and store password
		UMemory::Copy(str, inPassword, inPassword[0]+1);
		s = str[0];
		p = str + 1;
		while (s--) { *p = ~(*p); p++; }
		mFieldData->AddPString(myField_UserPassword, str);

		mFieldData->AddInteger(myField_Vers, 197);	// let the server know that we're vers 1.9.0
		
		// CMyTransactTask will dispose mTranSession if we fail
		mTranSession = NewSendSession(myTran_Login);
		mTranSession->SendData(mFieldData);
		}
		
	ShowProgress(mStage, 3);
}

Uint32 CMyLoginTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return UText::Format(outText, inMaxSize, "Logging in to: %#s", gApp->mServerName);
}

void CMyLoginTask::Process()
{
	Uint32 error=1;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply and error
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		switch (mStage)
		{
		case 1:
			{
			mStage=2;	
			//HOPE login
			HLCipher *theCipher;
			HLHash *theHash;
			DataBuffer serverHash=(DataBuffer){0,0};
			DataBuffer serverCipher=(DataBuffer){0,0};				
			DataBuffer sessionKey=(DataBuffer){0,0};
			UInt8 *MacLogin=nil;
			UInt8 *MacPassword=nil;
			UInt32 MacLen;

			// check for server errors
			CheckServerError(error, mFieldData);
							
			/////extract session key
			sessionKey.len=mFieldData->GetFieldSize(myField_SessionKey);
			if(sessionKey.len==0)
				{
				UApplication::PostMessage(msgProtocolError, "No session key received", 23, priority_High);
				goto fail_1;
				}
			if(sessionKey.len<32)
				{
				UApplication::PostMessage(msgProtocolError, "Session key not long enough", 27, priority_High);
				goto fail_1;
				}						
			sessionKey.data = new UInt8 [sessionKey.len];
			mFieldData->ReadField(myField_SessionKey,0,sessionKey.data,sessionKey.len);
			

			/////extract and match server hash
			serverHash.len=mFieldData->GetFieldSize(myField_MacAlg);
			if(serverHash.len==0)
				{
				UApplication::PostMessage(msgProtocolError, "No server MAC algorithm received", 32, priority_High);
				goto fail_1;
				}
			serverHash.data = new UInt8 [serverHash.len];
			mFieldData->ReadField(myField_MacAlg,0,serverHash.data,serverHash.len);
			
			if(memcmp(serverHash.data+2,HLSha1::GetName(),serverHash.len-2)==0)
				{
				theHash = new HLSha1();
				}
			else if (memcmp(serverHash.data+2,HLMD5::GetName(),serverHash.len-2)==0)
				{
				theHash = new HLMD5();
				}				
			else
				{
				UApplication::PostMessage(msgProtocolError, "Unsupported server MAC algorithm", 32, priority_High);
				goto fail_1;
				}							

			
			/////extract and match server cipher
			serverCipher.len=mFieldData->GetFieldSize(myField_S_CipherAlg);
			if(serverCipher.len==0)
				{
				UApplication::PostMessage(msgProtocolError, "No server cipher received", 25, priority_High);
				goto fail_1;
				}					
			serverCipher.data = new UInt8 [serverCipher.len];
			mFieldData->ReadField(myField_S_CipherAlg,0,serverCipher.data,serverCipher.len);

			if(memcmp(serverCipher.data+2,HLBlowfish::GetName(),serverCipher.len-2)==0)
				{
				theCipher = new HLBlowfish();
				}
			else
				{
				UApplication::PostMessage(msgProtocolError, "Unsupported server cipher", 25, priority_High);
				goto fail_1;
				}	
			if(!theCipher->SelfTest())
				{
				UApplication::PostMessage(msgProtocolError, "Cipher failed self test", 23, priority_High);
				goto fail_1;
				}


			//hash login and password				
			MacLen=theHash->GetMacLen();
			MacLogin=new UInt8 [MacLen];
			MacPassword=new UInt8 [MacLen];
			
			theHash->HMAC_XXX(MacLogin, (DataBuffer){gApp->mUserLogin+1, *gApp->mUserLogin}, sessionKey);
			theHash->HMAC_XXX(MacPassword, (DataBuffer){gApp->mUserPassword+1, *gApp->mUserPassword}, sessionKey);				
			
			//crypt login
			mFieldData->DeleteAllFields();
			mFieldData->AddField(myField_UserLogin,MacLogin,MacLen);
			mFieldData->AddField(myField_UserPassword,MacPassword,MacLen);		
			mFieldData->AddField(myField_C_CipherAlg,serverCipher.data,serverCipher.len);
			

			mFieldData->AddInteger(myField_Vers, 197);	// let the server know that we're vers 1.9.6	
		
			mTranSession = NewSendSession(myTran_Login);
			mTranSession->SendData(mFieldData);
			
			
			gCrypt=new HLCrypt();
			gCrypt->Init(theCipher, theHash, sessionKey, (DataBuffer){gApp->mUserPassword+1, *gApp->mUserPassword}, true);				

			ShowProgress(2, 3);
			if(serverHash.data) delete [] serverHash.data;
			if(serverCipher.data) delete [] serverCipher.data;
			if(sessionKey.data) delete [] sessionKey.data;				
			if(MacLogin) delete [] MacLogin;
			if(MacPassword) delete [] MacPassword;	
			break;
			
	fail_1:			
			if(serverHash.data) delete [] serverHash.data;
			if(serverCipher.data) delete [] serverCipher.data;
			if(sessionKey.data) delete [] sessionKey.data;				
			if(MacLogin) delete [] MacLogin;
			if(MacPassword) delete [] MacPassword;				
			SilentFail(errorType_Misc, error_Unknown);
			}
		
		case 2:
			{
			// check for server errors
			CheckServerError(error, mFieldData);
			
			// check server version
			gApp->mServerVers = mFieldData->GetInteger(myField_Vers);
			if (gApp->mServerVers >= 150)
				{
				new CMyDummyTask("\pLoading Agreement...", true);
							
				// get the server name
				Uint32 nSize = mFieldData->ReadField(myField_ServerName, 0, gApp->mServerName + 1, sizeof(gApp->mServerName) - 1);
				if (nSize) gApp->mServerName[0] = nSize;
				}
			else	// to ensure compatibility with old servers
				{
				mFieldData->DeleteAllFields();
				
				// setup field data - old vers wants me to list right away
				mFieldData->AddPString(myField_UserName, gApp->mUserName);
				mFieldData->AddInteger(myField_UserIconID, gApp->mIconID);

				gApp->mTpt->SendTransaction(myTran_SetClientUserInfo, mFieldData);
				
				gApp->mIsAgreed = true;
				gApp->ShowConnected(); 

				new CMyGetOnlineUsersTask("\pTransferring list of online users..."); 
				
				gApp->ProcessStartupPath();
					
				gApp->mMyAccess.Fill();				// give me full access
				gApp->ProcessTran_UserAccess(nil);	// set up all the windows to show full access
				}
					
			// all done
			if (gApp->mOptions.SoundPrefs.login) USound::Play(nil, 133, true);
			ShowProgress(3, 3);
			
			Finish();
			}
		}
		
		
		
		
		
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyAgreedTask::CMyAgreedTask()
	: CMyTransactTask("\pLogging in...")
{
	if (gApp->mServerVers >= 150)
	{
		gApp->GetClientUserInfo(mFieldData);
		
		// CMyTransactTask will dispose mTranSession if we fail
		mTranSession = NewSendSession(myTran_Agreed);
		mTranSession->SendData(mFieldData);
		
		ShowProgress(1, 2);
	}
	else // connecting to an old server that does not accept the agreed task
		Finish();
}

CMyAgreedTask::~CMyAgreedTask()
{
	if (!mIsFinished)
		gApp->ShowNotConnected();
}

Uint32 CMyAgreedTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pAgreeing...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMyAgreedTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
				
		gApp->mIsAgreed = true;
		gApp->ShowConnected(); 

		// all done
		ShowProgress(2, 2);
		Finish();

		new CMyGetOnlineUsersTask("\pTransferring list of online users...");
		
	#if !NEW_NEWS
		new CMyGetNewsTask();
	#endif
		
		gApp->ProcessStartupPath();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -


CMyGetBannerTimeTask::CMyGetBannerTimeTask(Uint8 *inDesc, THdl inAddress, Uint32 inMsg, bool inNext)
: CMyTask(inDesc)
{
	#pragma unused(inNext)

	if (gApp->mToolbarWin)
		gApp->mToolbarWin->SetAccess();
	
	mMsg = inMsg;
	mFinishTimer = nil;

	if (!inAddress)
	{
		Finish();
		return;
	}
	
	Uint32 nAddressSize = inAddress->GetSize();
	void *pAddress = UMemory::Lock(inAddress);
	mStartAddress[0] = UMemory::Copy(mStartAddress + 1
			, pAddress
			, nAddressSize > sizeof(mStartAddress) - 1 ? sizeof(mStartAddress) - 1 : nAddressSize);
	UMemory::Unlock(inAddress);

	UMemory::Copy(mAddress, mStartAddress, mStartAddress[0] + 1);
	
	Uint8 *pReferer = nil;
	pReferer = gApp->mAddress.name;
		
	Uint8 psCustomField[256];
	psCustomField[0] = 0;
	bool bSetHttpIDList = false;
		
	gApp->SetupHttpTransaction(mHttpTransact, pReferer, psCustomField, bSetHttpIDList);
	
	mFinishTimer = UTimer::New(FinishTimerProc, this);
	mFinishTimer->Start(30000);		// 30 seconds
}		

CMyGetBannerTimeTask::~CMyGetBannerTimeTask()
{
	if (mFinishTimer)
		UTimer::Dispose(mFinishTimer);
	
	if (gApp->mToolbarWin)
		gApp->mToolbarWin->SetAccess();
}

void CMyGetBannerTimeTask::FinishTimerProc(void *inContext, void */*inObject*/, Uint32 /*inMsg*/, const void */*inData*/, Uint32 /*inDataSize*/)
{	
	CMyGetBannerTimeTask *pGetBannerTimeTask = (CMyGetBannerTimeTask *)inContext;

	if (!pGetBannerTimeTask->IsFinished())
	{
		pGetBannerTimeTask->Process();
	
		if (!pGetBannerTimeTask->IsFinished())
			pGetBannerTimeTask->FinishBannerTask();
	}
}

void CMyGetBannerTimeTask::FinishBannerTask()
{

	// finish the task
	Finish();
}

void CMyGetBannerTimeTask::Process()
{
startagain:

	switch(mStage)
	{
		case 1:		
			if (!mHttpTransact->StartConnect(mAddress, protocol_HTTP_1_1, true))
			{	
				FinishBannerTask();
				return;
			}
			
			ShowProgress(1, 6);
			
			mStage = 2;
			break;
			
		case 2:
			if (!mHttpTransact->IsComplete())
				return;
			
			if (mHttpTransact->GetLocation(mAddress, sizeof(mAddress)))
			{
				mStage = 1;	
				goto startagain;
			}

			if (mHttpTransact->IsError())
			{
				FinishBannerTask();
				return;
			}
			
			mStage = 3;			
			break;
			
		case 3:			
			ShowProgress(3, 6);
		
			mStage = 4;			
			goto startagain;
			break;
			
		case 4:
			ShowProgress(6, 6);
			FinishBannerTask();			
	};
}

bool CMyGetBannerTimeTask::IsLoginTask()
{
	return true;
}


/* ������������������������������������������������������������������������� */
#pragma mark -

CMyGetBannerTask::CMyGetBannerTask(Uint8 *inDesc, THdl inAddress, Uint32 inMsg, bool inNext)
	: CMyGetBannerTimeTask(inDesc, inAddress, inMsg, inNext)
{
	mImgSrc[0] = 0;
	mImgHref[0] = 0;
	mImgComment[0] = 0;

	mNoCache = false;		
}

void CMyGetBannerTask::Process()
{
	if (mStage <= 2)
		CMyGetBannerTimeTask::Process();

startagain:

	switch(mStage)
	{
		case 3:			
			// get the http data
			Int8 csType[32];
			Uint32 nDataSize;
			
			void *pData = mHttpTransact->ReceiveHttpData(nDataSize, csType);
			if (!pData || !nDataSize)
			{
				if (!mHttpTransact->IsConnected())
					FinishBannerTask();
				
				return;
			}
						
			if (!UMemory::Compare(csType, "text/html", 9))
			{
								
				if (SearchImageTag(pData, nDataSize))
				{
					mStage = 4;
				}
				else
					mStage = 7;				
			}
			else
			{
				mStage = 6;
			}

			ShowProgress(2, 6);

			goto startagain;
			break;

		case 4:
		
			if (mHttpTransact->StartConnect(mImgSrc, protocol_HTTP_1_1, true))
				mStage = 5;
			else
				mStage = 7;
			
			ShowProgress(3, 6);

			goto startagain;
			break;
			
		case 5:
			if (!mHttpTransact->IsComplete())
				return;
			
			if (mHttpTransact->GetLocation(mImgSrc, sizeof(mImgSrc)))
			{
			
				mStage = 4;	
				goto startagain;
			}
			
			if (mHttpTransact->IsError())
			{
				FinishBannerTask();
				return;
			}

			pData = mHttpTransact->ReceiveHttpData(nDataSize, csType);
			if (!pData || !nDataSize)
			{
				if (!mHttpTransact->IsConnected())
					FinishBannerTask();

				return;
			}
				
			ShowProgress(4, 6);

			mStage = 6;
			goto startagain;
			break;

		case 6:
			Uint32 nTypeCode = UMime::ConvertMime_TypeCode(csType);
				
			gApp->SetServerBanner(pData, nDataSize, nTypeCode, mImgHref, mImgComment);
	
			ShowProgress(5, 6);

			mStage = 7;
			goto startagain;
			break;
			
		case 7:
			ShowProgress(6, 6);
			FinishBannerTask();
	};
}

void CMyGetBannerTask::FinishBannerTask()
{
	CMyGetBannerTimeTask::FinishBannerTask();
	
		UApplication::PostMessage(mMsg);
}


bool CMyGetBannerTask::SearchImageTag(void *inHtml, Uint32 inHtmlSize)
{
	Uint8 *pImgBegin = UText::SearchInsensitive("<img", 4, inHtml, inHtmlSize);
	if (!pImgBegin)
		return false;
	
	Uint32 nDepl = inHtmlSize - (pImgBegin + 4 - inHtml);
	Uint8 *pSrcBegin = UText::SearchInsensitive("src=\"", 5, pImgBegin + 4, (nDepl > 256 ? 256 : nDepl));
	if (!pSrcBegin)
		return false;
		
	pSrcBegin += 5;
	nDepl = inHtmlSize - (pSrcBegin - inHtml);
			
	Uint8 *pSrcEnd = UMemory::SearchByte('\"', pSrcBegin, (nDepl > 255 ? 255 : nDepl));
	if (!pSrcEnd)
		return false;
	
	mImgSrc[0] = UMemory::Copy(mImgSrc + 1, pSrcBegin, pSrcEnd - pSrcBegin);

	nDepl = 1;
	while(*(pImgBegin - nDepl++) != '<' && nDepl < pImgBegin - inHtml){}
	
	Uint8 *pABegin = UText::SearchInsensitive("<a", 2, pImgBegin - nDepl, nDepl);
				
	if (pABegin)
	{
		pABegin += 2;
		Uint8 *pHrefBegin = UText::SearchInsensitive("href=\"", 6, pABegin, pImgBegin - pABegin);

		if (pHrefBegin)
		{
			pHrefBegin += 6;
			Uint8 *pHrefEnd = UMemory::SearchByte('\"', pHrefBegin, pImgBegin - pHrefBegin);

			if (pHrefEnd)
			{
				mImgHref[0] = UMemory::Copy(mImgHref + 1, pHrefBegin, pHrefEnd - pHrefBegin);
				CheckURL(mImgHref, true);								
			}
		}
	}
	
	nDepl = inHtmlSize - (pSrcEnd - inHtml);
	Uint8 *pAltBegin = UText::SearchInsensitive("alt=\"", 5, pSrcEnd, (nDepl > 25 ? 25 : nDepl));
	
	if (pAltBegin)
	{
		pAltBegin += 5;
		nDepl = inHtmlSize - (pAltBegin - inHtml);
			
		Uint8 *pAltEnd = UMemory::SearchByte('\"', pAltBegin, (nDepl > 255 ? 255 : nDepl));

		if (pAltEnd)
			mImgComment[0] = UMemory::Copy(mImgComment + 1, pAltBegin, pAltEnd - pAltBegin);
	}

	if (UText::SearchInsensitive("nocache", 7, pSrcEnd, nDepl))
		mNoCache = true;

	return true;
}

bool CMyGetBannerTask::CheckURL(Uint8 *ioUrl, bool inProtocolsAllowed)
{
	Uint8 psCleanUrl[256];
	UMemory::Copy(psCleanUrl, ioUrl, ioUrl[0] + 1);
	ioUrl[0] = UTransport::CleanUpAddressText(kInternetNameAddressType, psCleanUrl + 1, psCleanUrl[0], ioUrl + 1, 255);

	if (ioUrl[0] > 7)
	{
		if (!UText::CompareInsensitive(ioUrl + 1, "http://", 7))
			return true;

		if (inProtocolsAllowed && !UText::CompareInsensitive(ioUrl + 1, "mailto:", 7))
			return true;
	}

	if (ioUrl[0] > 8)
	{
		if (inProtocolsAllowed && !UText::CompareInsensitive(ioUrl + 1, "hotline:", 8))
			return true;
	}
	
	if (mAddress[0] < 7 || UMemory::Compare(mAddress + 1, "http://", 7))
	{
		ioUrl[0] = 0;
		return false;
	}

	Uint8 nCount = mAddress[0];
	while(mAddress[nCount--] != '/' && nCount != 0){}

	if (nCount <= 6)
		nCount = mAddress[0];
	
	Uint8 psNewUrl[256];
	if (!UMemory::Compare(ioUrl + 1 , "/", 1) || !UMemory::Compare(ioUrl + 1, "../", 3))
	{
		Uint8 nNewCount = nCount;
		while(mAddress[nNewCount--] != '/' && nNewCount != 0){}

		if (nNewCount > 6)
			nCount = nNewCount;
				
		if (!UMemory::Compare(ioUrl + 1, "/", 1))
			psNewUrl[0] = UMemory::Copy(psNewUrl + 1, ioUrl + 1, ioUrl[0]);
		else
			psNewUrl[0] = UMemory::Copy(psNewUrl + 1, ioUrl + 3, ioUrl[0] - 2);
	}
	else
	{
		psNewUrl[0] = 1;
		psNewUrl[1] = '/';
	
		psNewUrl[0] += UMemory::Copy(psNewUrl + psNewUrl[0] + 1, ioUrl + 1, ioUrl[0]);
	}
	
	ioUrl[0] = 0;
	
	if (nCount)
	{
		ioUrl[0] = UMemory::Copy(ioUrl + 1, mAddress + 1, nCount); 			
		ioUrl[0] += UMemory::Copy(ioUrl + ioUrl[0] + 1, psNewUrl + 1, psNewUrl[0]);
		
		return true;
	}
	
	return false;
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyLaunchUrlTask::CMyLaunchUrlTask(THdl inAddress)
	: CMyTask("\pLaunching browser...")
{
	mFinishTimer = nil;
	
	if (!inAddress)
	{
		Finish();
		return;
	}
	
	Uint32 nAddressSize = inAddress->GetSize();
	void *pAddress = UMemory::Lock(inAddress);
	mAddress[0] = UMemory::Copy(mAddress + 1, pAddress, (nAddressSize < 255 ? nAddressSize : 255));
	UMemory::Unlock(inAddress);

	// if we have "hotline://", "mailto:" or another protocol just launch the url
	if (mAddress[0]>7 && 0==UText::CompareInsensitive(mAddress + 1, "http://", 7))
	{
		UTransport::LaunchURL(mAddress + 1, mAddress[0]);
		Finish();
		return;
	}

	gApp->SetupHttpTransaction(mHttpTransact);
//	if (!mHttpTransact->StartConnect(mAddress, protocol_HTTP_1_1, true, true))	// sometimes HEAD command returns wrong location
	if (!mHttpTransact->StartConnect(mAddress, protocol_HTTP_1_1, true))
	{	
		Finish();	
		return;
	}
	
	mFinishTimer = UTimer::New(FinishTimerProc, this);
	mFinishTimer->Start(30000);		// 30 seconds
	
	ShowProgress(1, 3);
}		

CMyLaunchUrlTask::~CMyLaunchUrlTask()
{
	if (mFinishTimer)
		UTimer::Dispose(mFinishTimer);
}

void CMyLaunchUrlTask::FinishTimerProc(void *inContext, void */*inObject*/, Uint32 /*inMsg*/, const void */*inData*/, Uint32 /*inDataSize*/)
{	
	CMyLaunchUrlTask *pLaunchUrlTask = (CMyLaunchUrlTask *)inContext;

	if (!pLaunchUrlTask->IsFinished())
	{
		pLaunchUrlTask->Process();
	
		if (!pLaunchUrlTask->IsFinished())
			pLaunchUrlTask->Finish();		
	}
}

void CMyLaunchUrlTask::Process()
{
startagain:

	switch (mStage)
	{			
		case 1:
			if (!mHttpTransact->IsComplete())
				return;
							
			if (mHttpTransact->GetLocation(mAddress, sizeof(mAddress)))
			{
				// if we have "hotline://" or another protocol just launch the url
				if (UText::CompareInsensitive(mAddress + 1, "http://", 7) && UMemory::Search("://", 3, mAddress + 1, mAddress[0]))
				{
					UTransport::LaunchURL(mAddress + 1, mAddress[0]);
			
					mStage = 3;
					goto startagain;
				}
				
				mStage = 2;	
				goto startagain;
			}
			
			if (mHttpTransact->GetUrl(mAddress, sizeof(mAddress)))
				UTransport::LaunchURL(mAddress + 1, mAddress[0]);
			
			mStage = 3;
			goto startagain;
			break;

		case 2:
//			if (mHttpTransact->StartConnect(mAddress, protocol_HTTP_1_1, true, true)) // sometimes HEAD command returns wrong location
			if (mHttpTransact->StartConnect(mAddress, protocol_HTTP_1_1, true))
				mStage = 1;
			else
				mStage = 3;
			
			ShowProgress(2, 3);
			
			goto startagain;
			break;

		case 3:
			ShowProgress(3, 3);
			Finish();
	};
}

/* ������������������������������������������������������������������������� */
#pragma mark -
CMyDownloadTask::CMyDownloadTask(const void *inPathData, Uint32 inPathSize, const Uint8 *inFileFldrName, const Uint8 *inValidatedName, bool inQueued)
	: CMyTransactTask("\p")
{
	mTpt = nil;
	mWaitingTimer = nil;
	
	mStartSecs = 0;
	mTotalSize = 0; 
	mDownloadedSize = 0;

	mDisplayedSize = 0;
	mDisplayedBPS = 0; 
	mDisplayedETR = 0;
	mRefNum = 0;

	mPathData = nil;
	mPathSize = 0;
	
	// save path (we'll need it later)
	if (inPathData && inPathSize)
	{
		mPathData = UMemory::New(inPathData, inPathSize);
		mPathSize = inPathSize;
	}
	
	// grab a copy of the file/folder name
	UMemory::Copy(mFileFldrName, inFileFldrName, inFileFldrName[0] + 1);

	// grab a copy of the validated file name
	UMemory::Copy(mValidatedName, inValidatedName, inValidatedName[0] + 1);

#if MACINTOSH_CLASSIC
	// max 31 chars for the file name on classic MAC
	if (mValidatedName[0] > 31)
		mValidatedName[0] = 31;
#endif

	mWasQueued = inQueued;
	mWaitingCount = 0;
	mMaxWaitingCount = 0;
}

CMyDownloadTask::~CMyDownloadTask()
{
	if (mTpt)
		delete mTpt;

	if (mWaitingTimer)
		UTimer::Dispose(mWaitingTimer);
	
	if (mPathData)
		UMemory::Dispose(mPathData);
		
	// if was killed in the queue	
	if (mStage == 10)
		UApplication::PostMessage(1116, &mRefNum, sizeof(Uint32));
}

bool CMyDownloadTask::ShowWaitingCount(Uint32 inRefNum, Uint32 inWaitingCount)
{
	if (mRefNum != inRefNum)
		return false;
	
	mWaitingCount = inWaitingCount;
	if (mMaxWaitingCount < mWaitingCount)
		mMaxWaitingCount = mWaitingCount;
		   
	Uint8 psMsg[256];
	psMsg[0] = UText::Format(psMsg + 1, sizeof(psMsg) - 1, "All downloads slots are full. �%#s� is number %lu in the server queue. Please wait...", mFileFldrName, mWaitingCount);
	ShowProgress(mMaxWaitingCount - mWaitingCount, mMaxWaitingCount, psMsg);

	if (!mWaitingCount)
	{
		// stop the timer
		StopWaitingTimer();
		
		// need time at which we started to generate stats
		mStartSecs = UDateTime::GetSeconds();
	}
	
	return true;
}

bool CMyDownloadTask::QueueUnder()
{
	return !mIsFinished;
}

void CMyDownloadTask::Start()
{
	if (mStage == 0)
	{
		mStage = 1;
		gApp->ScheduleTaskProcess();
	}
}

void CMyDownloadTask::StartWaitingTimer()
{
	if (!mTpt || !mTpt->IsRegular())
		return;
	
	if (!mWaitingTimer)
		mWaitingTimer = UTimer::New(WaitingTimerProc, this);

	mWaitingTimer->Start(kKeepConnectionAliveTime, kRepeatingTimer);
}

void CMyDownloadTask::StopWaitingTimer()
{
	if (mWaitingTimer)
		mWaitingTimer->Stop();
}

void CMyDownloadTask::SendWaitingData()
{
	if (!mTpt || !mTpt->IsRegular() || !mTpt->IsConnected())
		return;
	
	// send padding data while wait in the server queue (1 byte each 3 minutes)
	Uint8 nData = 0;
	mTpt->Send(&nData, sizeof(nData));
}

void CMyDownloadTask::WaitingTimerProc(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	#pragma unused(inObject, inData, inDataSize)
	
	if (inMsg == msg_Timer)
		((CMyDownloadTask *)inContext)->SendWaitingData();
}

/* ������������������������������������������������������������������������� */
#pragma mark -

// does NOT take ownership of inDestFile
CMyDownloadFileTask::CMyDownloadFileTask(
			const void *inPathData, Uint32 inPathSize, 
			const Uint8 *inFileName, const Uint8 *inValidatedName, 
			TFSRefObj* inDestFile, 
			bool inResume, bool inQueued, bool inCreateFile, bool inViewAfterDload, 
			bool *outDropEnd, bool *outDropError)
: CMyDownloadTask(inPathData, inPathSize, inFileName, inValidatedName, inQueued) 
, mFile(0)
, mFileSize(0)
, mResumeData(0)
, mViewAfterDownload(inViewAfterDload)
{
	Uint8 str[256];
	
	try
	{
		// set text description in task window
		if (mWasQueued)
			str[0] = UText::Format(str+1, sizeof(str)-1, "Downloading �%#s� (queued)", inFileName);
		else
			str[0] = UText::Format(str+1, sizeof(str)-1, "Downloading �%#s�", inFileName);
		
		ShowProgress(0, 1, str);
				
		// setup field data to send
		mFieldData->AddPString(myField_FileName, inFileName);
		mFieldData->AddField(myField_FilePath, inPathData, inPathSize);
		
		// create file on disk for download and prepare to unflatten
		mFile = inDestFile->Clone();
		if (inResume)
			mResumeData = mFile->ResumeUnflatten();
		else
		{
		#if WIN32
			// put the .hpf extension
			pstrcat(mValidatedName, "\p.hpf");
			
			if (inCreateFile)
				mFile->SetRefName(mValidatedName);
			else
				mFile->SetName(mValidatedName);
		#endif

			if (inCreateFile)
				mFile->CreateFile('HTft', 'HTLC');
			
			try
			{
				mFile->SetComment(gApp->mAddress.name+1, gApp->mAddress.name[0]);
			}
			catch (...)
			{
				// don't throw
			}
			
			mFile->StartUnflatten();
		}
		
		// if being queued, we'll start at an earlier stage
		if (inQueued)
			mStage = 0;

		mDropEnd = outDropEnd;
		mDropError = outDropError;
		
		// ready to rock!!!
		gApp->ScheduleTaskProcess();
			}
	catch(...)
	{
		// clean up
		if (mResumeData)
		{
			UMemory::Dispose(mResumeData);
			mResumeData = nil;
		}
		
		if (mFile)
		{
			if (!inResume) try { mFile->DeleteFile(); } catch(...) {}
			delete mFile;
			mFile = nil;
		}
		
		throw;
	}
}

CMyDownloadFileTask::~CMyDownloadFileTask()
{
	if (mResumeData)
		UMemory::Dispose(mResumeData);
		
	if (mFile)
	{
		if (!mIsFinished)
		{
			if (mDropError)
				*mDropError = true;
			
			try
			{
				mFile->Close();		// important so size includes all forks
				if (mFile->GetSize() == 0)
					mFile->DeleteFile();
			}
			catch(...) {}
		}
		
		delete mFile;
	}

	if (mDropEnd)
		*mDropEnd = true;
}

bool CMyDownloadFileTask::ShowWaitingCount(Uint32 inRefNum, Uint32 inWaitingCount)
{
	if (!CMyDownloadTask::ShowWaitingCount(inRefNum, inWaitingCount))
		return false;

	if (!inWaitingCount && !mTpt)
	{
		mTpt = gApp->StartTransferTransportConnect();
		mStage = 3;
	}
	
	return true;
}

Uint32 CMyDownloadFileTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mFileFldrName[0] ?
		UText::Format(outText, inMaxSize, "Downloading file: %#s...", mFileFldrName) :
		UText::Format(outText, inMaxSize, "Downloading file...");
}

void CMyDownloadFileTask::Process()
{
	Uint8 str[256];
	
goFromStart:

	switch (mStage)
	{
		// do nothing if queued and waiting
		case 0:
		{
			if (gApp->IsFirstQueuedTask(this))
			{
				mStage = 1;
				goto goFromStart;
			}
		}
		break;
	
		// send download-file transaction
		case 1:
		{
			if (mWasQueued)
			{
				str[0] = UText::Format(str+1, sizeof(str)-1, "Downloading �%#s�", mFileFldrName);
				ShowProgress(0, 10, str);
			}
			
			if (mResumeData)
			{
				{
					void *p;
					StHandleLocker lock(mResumeData, p);
					mFieldData->AddField(myField_FileResumeData, p, UMemory::GetSize(mResumeData));
				}
				
				UMemory::Dispose(mResumeData);
				mResumeData = nil;
			}
			
			mTranSession = NewSendSession(myTran_DownloadFile);
			mTranSession->SendData(mFieldData);
						
			mStage++;
		}
		break;
		
		// if server has replied, we can request an xfer connection
		case 2:
		{
			if (mTranSession->IsReceiveComplete())
			{
				// extract reply
				mTranSession->ReceiveData(mFieldData);
				Uint32 error = mTranSession->GetReceiveError();
				
				// don't need session anymore
				delete mTranSession;
				mTranSession = nil;

				// check for server errors
				CheckServerError(error, mFieldData);
				
				// extract info
				mTotalSize = mFieldData->GetInteger(myField_TransferSize);
				mFileSize = mFieldData->GetInteger(myField_FileSize);
				mRefNum = mFieldData->GetInteger(myField_RefNum);

				// check waiting count
				mWaitingCount = mFieldData->GetInteger(myField_WaitingCount);

					if (mWaitingCount)	// we are in the server queue
					{
						ShowWaitingCount(mRefNum, mWaitingCount);
						
						if (gApp->mServerVers >= 185)
							mStage = 10;	// waiting stage
						else
						{
							// request xfer connection with server
							mTpt = gApp->StartTransferTransportConnect();
							mStage++;		// next stage
						}
					}
					else
					{
						// request xfer connection with server
						mTpt = gApp->StartTransferTransportConnect();
						mStage++;		// next stage					
					}
			}
		}
		break;
		
		// if xfer connection has opened, tell the server to begin
		case 3:
		{
			if (!mTpt)
			{
				Finish();
				return;
			}
			
			if (mTpt->GetConnectStatus() == kConnectionEstablished)
			{
				// identify ourself to the server
#pragma options align=packed
				struct {
					Uint32 protocol;	// HTXF
					Uint32 refNum;
					Uint32 dataSize;
					Uint32 rsvd;
				} sndData = { TB((Uint32)0x48545846), TB(mRefNum), 0, 0 };
#pragma options align=reset
				mTpt->Send(&sndData, sizeof(sndData));
				
				// need time at which we started to generate stats
				mStartSecs = UDateTime::GetSeconds();
				
				// start the timer if we are in the server queue
				if (mWaitingCount)
					StartWaitingTimer();
				
				// next stage
				mStage++;
			}
		}
		break;
		
		// manage the xfer connection (process incoming data etc)
		case 4:
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				UApplication::PostMessage(1103);
				SilentFail(errorType_Transport, transError_ConnectionClosed);
			}
						
			// receive and process data
			void *rcvBuf;
				Uint32 s;
				while (mTpt->ReceiveBuffer(rcvBuf, s))
				{
					// process data
					try
					{
						mFile->ProcessUnflatten(rcvBuf, s);
						mDownloadedSize += s;
						gApp->mTotalBytesDownloaded += s;
					}
					catch(...)
					{
						UTransport::DisposeBuffer(rcvBuf);
						throw;
					}
					UTransport::DisposeBuffer(rcvBuf);
				}
			if (mDownloadedSize >= mTotalSize) // check if finished
			{
			#if DEBUG
				if (mDownloadedSize > mTotalSize)
					DebugBreak("mDownloadedSize = %lu, mTotalSize = %lu", mDownloadedSize, mTotalSize);
			#endif
				// complete unflatten
				mFile->ProcessUnflatten(nil, 0);

			#if WIN32
				// take the .hpf out of the name
				if (mValidatedName[0] >= 4 && !UMemory::Compare(mValidatedName + mValidatedName[0] - 3, ".hpf", 4))
				{
					mValidatedName[0] -= 4;
					
					mFile->StopUnflatten();	// can't rename open file
					mFile->SetName(mValidatedName);
				}
			#endif

					// delete stuff
					mTpt->Disconnect();
					delete mTpt;
					mTpt = nil;
				delete mFile;
				mFile = nil;
								
				// all done, clean up
				Finish();
				if (mViewAfterDownload)
					gApp->DoViewDloadFile(mFileFldrName);
				else if (gApp->mOptions.SoundPrefs.fileTransfer) 
					USound::Play(nil, 135, true);
			}

			// calc stats
			Uint32 downloadedSize = mDownloadedSize;
			Uint32 totalSize = mTotalSize;
			Uint32 elapsedSecs = UDateTime::GetSeconds() - mStartSecs;
			
			if (elapsedSecs > 0 && downloadedSize > elapsedSecs && downloadedSize > 1024)
			{
				Uint32 bps = downloadedSize / elapsedSecs;
				Uint32 etr = (totalSize - downloadedSize) / bps;
				
				// display stats if changed
				if (downloadedSize != mDisplayedSize || bps != mDisplayedBPS || etr != mDisplayedETR)
				{
					Uint8 stat[256];
					gApp->GetTransferStatText(downloadedSize, totalSize, bps, etr, stat);
					
					str[0] = UText::Format(str+1, sizeof(str)-1, "Downloading �%#s� (%#s)", mFileFldrName, stat);
					ShowProgress(downloadedSize, totalSize, str);

					mDisplayedSize = downloadedSize;
					mDisplayedBPS = bps;
					mDisplayedETR = etr;
				}
			}
			else if (downloadedSize)
				ShowProgress(downloadedSize, totalSize);
		}
		break;
		
		case 10:
			// do nothing stage, waiting in the Server queue
		break;
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyViewFileTask::CMyViewFileTask(const void *inPathData, Uint32 inPathSize, 
								 const Uint8 *inFileName, const Uint8 *inValidatedName, 
								 Uint32 inTypeCode, Uint32 inCreatorCode, 
								 bool inQueued, bool inNewWindow)
	: CMyDownloadTask(inPathData, inPathSize, inFileName, inValidatedName, inQueued), mFileSize(0), mViewFileWin(0)
{
	mTypeCode = inTypeCode;
	mCreatorCode = inCreatorCode;
	
	mCanKillTask = true;
	mTryKillTask = false;
	mNewViewFileWin = inNewWindow;

	Uint8 str[256];

	try
	{
		// set text description in task window
		if (inQueued)
			str[0] = UText::Format(str+1, sizeof(str)-1, "Viewing �%#s� (queued)", inFileName);
		else
			str[0] = UText::Format(str+1, sizeof(str)-1, "Viewing �%#s�", inFileName);

		ShowProgress(0, 1, str);
		
		// for text and images we need just data
		if (IsTextFile() || IsImageFile())
			mFieldData->AddInteger(myField_FileXferOptions, 2);

		// setup field data to send
		mFieldData->AddPString(myField_FileName, inFileName);
		mFieldData->AddField(myField_FilePath, inPathData, inPathSize);
				
		// if being queued, we'll start at an earlier stage
		if (inQueued)
			mStage = 0;
		
		gApp->ScheduleTaskProcess();
	}
	catch(...)
	{
		throw;
	}
}

CMyViewFileTask::~CMyViewFileTask()
{
	if (mViewFileWin)
	{
		mViewFileWin->EndViewFileTask();

		if (!mIsFinished && !mViewFileWin->StopStreamMovie())
		{
			if (!mViewFileWin->CanCloseViewFileWin())
				mViewFileWin->SetMustCloseFlag();
			else
				delete mViewFileWin;
		}	
	}
}

bool CMyViewFileTask::ShowWaitingCount(Uint32 inRefNum, Uint32 inWaitingCount)
{
	if (!CMyDownloadTask::ShowWaitingCount(inRefNum, inWaitingCount))
		return false;

	if (!inWaitingCount)
	{
		if (IsQuickTimeFile())
		{
			mStage = 11;	// do nothing stage
			StartQuickTime();
		}
		else if (!mTpt)
		{
			// request xfer connection with server
			mTpt = gApp->StartTransferTransportConnect();
			mStage = 3;
		}
	}
	
	return true;
}

void CMyViewFileTask::EndViewFileWin()
{
	mViewFileWin = nil;
}

bool CMyViewFileTask::MakeViewFileWindow()
{
	static Uint32 nCount = 0;
	
	mCanKillTask = false;
	try
	{
		if (mNewViewFileWin)
			nCount = 0;
		else
		{
			if (nCount > gApp->mViewWinList.GetItemCount() - 1)
				nCount = 0;
		
			CMyViewFileWin *win;
			while (gApp->mViewWinList.GetNext(win, nCount))
			{
				if (win->IsEndViewFileTask() && win->CanCloseViewFileWin())
				{
					mViewFileWin = win;
					break;
				}
			}
		}
	
		if (!mViewFileWin)
		{
			mViewFileWin = new CMyViewFileWin(gApp->mAuxParentWin);
			mNewViewFileWin = true;
		}
	
		if (!mViewFileWin->SetViewFileWin(this, mPathData, mPathSize, mValidatedName, mTypeCode, mCreatorCode, mFileSize, mTotalSize, mRefNum, mNewViewFileWin) || mTryKillTask)
		{
			mViewFileWin->EndViewFileTask();
		
			if (!mViewFileWin->CanCloseViewFileWin())
				mViewFileWin->SetMustCloseFlag();
			else
				delete mViewFileWin;

			mViewFileWin = nil;		
			
			mCanKillTask = true;
			return false;
		}
		
		if (IsTextFile())
			mViewFileWin->SetTextColor(gApp->mOptions.ColorViewText);
	}
	catch (...)
	{
		// don't throw
		mCanKillTask = true;
		return false;
	}
		
	mCanKillTask = true;
	return true;
}

bool CMyViewFileTask::IsTextFile()
{
	if (mTypeCode == TB((Uint32)'TEXT'))
		return true;
		
	return false;
}

bool CMyViewFileTask::IsImageFile()
{
	if (mTypeCode == TB((Uint32)'JPEG') || mTypeCode == TB((Uint32)'GIFf') || mTypeCode == TB((Uint32)'BMP ') || mTypeCode == TB((Uint32)'PICT'))
		return true;
		
	return false;
}
bool CMyViewFileTask::IsQuickTimeFile()
{
	if (IsTextFile() || IsImageFile())	
		return false;
		
	return true;
}

bool CMyViewFileTask::StartQuickTime()
{
	// need time at which we started to generate stats
	mStartSecs = UDateTime::GetSeconds();

	if (!MakeViewFileWindow())
	{
		Finish();
		
		if (!mTryKillTask)
		{
			Uint8 psMsg[256];
			psMsg[0] = UText::Format(psMsg + 1, sizeof(psMsg) - 1, "Couldn't open %#s because it is not a file that QuickTime understands.", mFileFldrName);
			
			gApp->DisplayStandardMessage("\pView", psMsg, icon_Stop);
		}
		
		return false;
	}
	
	// check if finished
	if (mDownloadedSize >= mTotalSize)
		Finish();
	
	return true;
}

Uint32 CMyViewFileTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mFileFldrName[0] ?
		UText::Format(outText, inMaxSize, "Viewing file: %#s...", mFileFldrName) :
		UText::Format(outText, inMaxSize, "Viewing file...");
}

void CMyViewFileTask::Process()
{
	Uint8 str[256];

goFromStart:

	switch (mStage)
	{
		// do nothing if queued and waiting
		case 0:
		{
			if (gApp->IsFirstQueuedTask(this))
			{
				mStage = 1;
				goto goFromStart;
			}
		}
		break;
	
		// send download-file transaction
		case 1:
		{
			if (mWasQueued)
			{
				str[0] = UText::Format(str+1, sizeof(str)-1, "Viewing �%#s�", mFileFldrName);
				ShowProgress(0, 10, str);
			}
			
			mTranSession = NewSendSession(myTran_DownloadFile);
			mTranSession->SendData(mFieldData);
						
			mStage++;
		}
		break;
		
		// if server has replied, we can request an xfer connection
		case 2:
		{
			if (mTranSession->IsReceiveComplete())
			{
				// extract reply
				mTranSession->ReceiveData(mFieldData);
				Uint32 error = mTranSession->GetReceiveError();
				
				// don't need session anymore
				delete mTranSession;
				mTranSession = nil;

				// check for server errors
				CheckServerError(error, mFieldData);
				
				// extract info
				mTotalSize = mFieldData->GetInteger(myField_TransferSize);
				if (mTotalSize == 0)
				{	
					Finish();
					
					// this message may be changed
					str[0] = UText::Format(str + 1, sizeof(str) - 1, "%#s is empty.", mFileFldrName);
					gApp->DisplayStandardMessage("\pView", str, icon_Stop);

					return;
				}

				mFileSize = mFieldData->GetInteger(myField_FileSize);
				mRefNum = mFieldData->GetInteger(myField_RefNum);
				
				// check waiting count
				mWaitingCount = mFieldData->GetInteger(myField_WaitingCount);
				if (mWaitingCount)	// we are in the server queue
				{ 
					ShowWaitingCount(mRefNum, mWaitingCount);
					
					if (IsQuickTimeFile() || gApp->mServerVers >= 185)
						mStage = 10;	// waiting stage
					else
					{
						// request xfer connection with server
						mTpt = gApp->StartTransferTransportConnect();
						mStage++;		// next stage
					}
				}
				else if (IsQuickTimeFile())
				{
					mStage = 11;	// do nothing stage
					StartQuickTime();
				}
				else
				{
					// request xfer connection with server
					mTpt = gApp->StartTransferTransportConnect();
					mStage++;		// next stage
				}
			}
		}
		break;
		
		// if xfer connection has opened, tell the server to begin
		case 3:
		{
			if (!mTpt)
			{
				Finish();
				return;
			}

			if (mTpt->GetConnectStatus() == kConnectionEstablished)
			{
				if (!MakeViewFileWindow())
				{
					Finish();
					return;
				}

				// identify ourself to the server
#pragma options align=packed
				struct {
					Uint32 protocol;	// HTXF
					Uint32 refNum;
					Uint32 dataSize;
					Uint32 rsvd;
				} sndData = { TB((Uint32)0x48545846), TB(mRefNum), 0, 0 };
#pragma options align=reset
				mTpt->Send(&sndData, sizeof(sndData));
				
				// need time at which we started to generate stats
				mStartSecs = UDateTime::GetSeconds();

				// start the timer if we are in the server queue
				if (mWaitingCount)
					StartWaitingTimer();

				// next stage
				mStage++;												
			}
		}
		break;
		
		// manage the xfer connection (process incoming data etc)
		case 4:
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				UApplication::PostMessage(1103);
				SilentFail(errorType_Transport, transError_ConnectionClosed);
			}
						
			// receive and process data
			Uint32 size;
			void *rcvBuf;
			while (mTpt->ReceiveBuffer(rcvBuf, size))
			{
				// process data
				try
				{
					if (mViewFileWin)
					{
						if (IsTextFile())
							mViewFileWin->InsertText(rcvBuf, size); 
						else if (IsImageFile())
							mViewFileWin->InsertImage(rcvBuf, size);
					}

					mDownloadedSize += size;
					gApp->mTotalBytesDownloaded += size;
				}
				catch(...)
				{
					UTransport::DisposeBuffer(rcvBuf);
					throw;
				}

				UTransport::DisposeBuffer(rcvBuf);
			}
			
			ShowFileProgress();
		}
		break;
		
		case 10:
			// do nothing stage, waiting in the Server queue
		break;
		
		case 11:
			// do nothing stage
		break;
	}
}

bool CMyViewFileTask::CanKillTask()	
{	
	return (mIsFinished || mCanKillTask);
}

void CMyViewFileTask::TryKillTask()
{
	mTryKillTask = true;

	if (mViewFileWin)
		mViewFileWin->StopStreamMovie();
}

void CMyViewFileTask::SetProgress(Uint32 inTotalSize, Uint32 inDownloadedSize)
{
	if (inTotalSize) mTotalSize = inTotalSize;
	mDownloadedSize = inDownloadedSize;

	ShowFileProgress();
}

void CMyViewFileTask::ShowFileProgress()
{
	// check if finished
	if (mDownloadedSize >= mTotalSize)
	{
	#if DEBUG
		if (mDownloadedSize > mTotalSize)
			DebugBreak("mDownloadedSize = %lu, mTotalSize = %lu", mDownloadedSize, mTotalSize);
	#endif
		
		// delete stuff
		if (mTpt)
		{
			mTpt->Disconnect();
			delete mTpt;
			mTpt = nil;
		}

		if (mCanKillTask)
		{
			// all done, clean up
			Finish();
			if (gApp->mOptions.SoundPrefs.fileTransfer) USound::Play(nil, 135, true);
		
			#if WIN32
				if (IsQuickTimeFile())
					UApplication::PostMessage(1330);	// this will fix the timer issue	
			#endif
		}
	}

	// calc stats
	Uint32 downloadedSize = mDownloadedSize;
	Uint32 totalSize = mTotalSize;
	Uint32 elapsedSecs = UDateTime::GetSeconds() - mStartSecs;
			
	if (elapsedSecs > 0 && downloadedSize > elapsedSecs && downloadedSize > 1024)
	{
		Uint32 bps = downloadedSize / elapsedSecs;
		Uint32 etr = (totalSize - downloadedSize) / bps;
				
		// display stats if changed
		if (downloadedSize != mDisplayedSize || bps != mDisplayedBPS || etr != mDisplayedETR)
		{
			Uint8 stat[256];
			gApp->GetTransferStatText(downloadedSize, totalSize, bps, etr, stat);
					
			Uint8 str[256];
			str[0] = UText::Format(str+1, sizeof(str)-1, "Viewing �%#s� (%#s)", mFileFldrName, stat);
			ShowProgress(downloadedSize, totalSize, str);

			mDisplayedSize = downloadedSize;
			mDisplayedBPS = bps;
			mDisplayedETR = etr;
		}
	}
	else if (downloadedSize)
		ShowProgress(downloadedSize, totalSize);
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyDownloadFldrTask::CMyDownloadFldrTask(const void *inPathData, Uint32 inPathSize, const Uint8 *inFldrName, TFSRefObj* inDestFldr, bool inResume, bool inQueued, bool inCreateFldr, bool *outDropEnd, bool *outDropError)
	: CMyDownloadTask(inPathData, inPathSize, inFldrName, inFldrName, inQueued), 
	  mFldr(0), mTotalItems(0), mDownloadedItems(0), mFile(0), mFileTotalSize(0), mFileDlSize(0), mHeaderSize(0)
{
	// open a connection to the server and req this item
	#pragma unused(inCreateFldr)
	Uint8 str[256];
	
	mFilePath = nil;
	mFilePathSize = 0;

	try
	{
		// set text description in task window
		if (mWasQueued)
			str[0] = UText::Format(str+1, sizeof(str)-1, "Downloading Folder �%#s� (queued)", inFldrName);
		else
			str[0] = UText::Format(str+1, sizeof(str)-1, "Downloading Folder �%#s�", inFldrName);
		
		ShowProgress(0, 1, str);

		// setup field data to send
		mFieldData->AddPString(myField_FileName, inFldrName);
		mFieldData->AddField(myField_FilePath, inPathData, inPathSize);

		mFldr = inDestFldr->Clone();
				
		// to do - check if this is a file!
		if (!mFldr->Exists())
			mFldr->CreateFolder();

		// if being queued, we'll start at an earlier stage
		if (inQueued)
			mStage = 0;

		mDropEnd = outDropEnd;
		mDropError = outDropError;

		gApp->ScheduleTaskProcess();
	}
	catch(...)
	{
		if (mFldr)
		{
			if (!inResume) try { mFldr->DeleteFolder(); } catch(...) {}
			delete mFldr;
			mFldr = nil;
		}
		
		throw;
	}
}

CMyDownloadFldrTask::~CMyDownloadFldrTask()
{
	if (mFile)
	{
		if (!mIsFinished)
		{
			if (mDropError)
				*mDropError = true;
		
			try
			{
				mFile->Close();		// important so size includes all forks
				if (mFile->GetSize() == 0)
					mFile->DeleteFile();
			}
			catch(...) {}
		}
		
		delete mFile;
	}
	
	if (mFldr)
		delete mFldr;

	if (mFilePath)
		UMemory::Dispose(mFilePath);

	if (mDropEnd)
		*mDropEnd = true;
}

bool CMyDownloadFldrTask::ShowWaitingCount(Uint32 inRefNum, Uint32 inWaitingCount)
{
	if (!CMyDownloadTask::ShowWaitingCount(inRefNum, inWaitingCount))
		return false;

	if (!inWaitingCount && !mTpt)
	{
		// request xfer connection with server
		mTpt = gApp->StartTransferTransportConnect();
		mStage = 3;
	}
	
	return true;
}

Uint32 CMyDownloadFldrTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mFileFldrName[0] ?
		UText::Format(outText, inMaxSize, "Downloading folder: %#s...", mFileFldrName) :
		UText::Format(outText, inMaxSize, "Downloading folder...");
}

void CMyDownloadFldrTask::Process()
{
	Uint8 str[256];
	Uint8 data[4];
	Uint32 s;

goFromStart:
	if (mIsFinished)
		DebugBreak("Finished but still processing!!");

	switch (mStage)
	{
		// do nothing if queued and waiting
		case 0:
		{
			if (gApp->IsFirstQueuedTask(this))
			{
				mStage = 1;
				goto goFromStart;
			}
		}
		break;
	
		// send download-file transaction
		case 1:
		{
			if (mWasQueued)
			{
				str[0] = UText::Format(str+1, sizeof(str)-1, "Downloading Folder �%#s�", mFileFldrName);
				ShowProgress(0, 10, str);
			}
			
			mTranSession = NewSendSession(myTran_DownloadFldr);
			mTranSession->SendData(mFieldData);
						
			mStage++;
		}
		break;


		// if server has replied, we can request an xfer connection
		case 2:
		{
			if (mTranSession->IsReceiveComplete())
			{
				// extract reply
				mTranSession->ReceiveData(mFieldData);
				Uint32 error = mTranSession->GetReceiveError();
				
				// don't need session anymore
				delete mTranSession;
				mTranSession = nil;

				// check for server errors
				CheckServerError(error, mFieldData);
				
				// extract info
				mTotalItems = mFieldData->GetInteger(myField_FldrItemCount);
				if (mTotalItems == 0)	// no items in this folder
				{
					Finish();
					return;
				}
				
				mRefNum = mFieldData->GetInteger(myField_RefNum);
				mTotalSize = mFieldData->GetInteger(myField_TransferSize);
				
				// check waiting count
				mWaitingCount = mFieldData->GetInteger(myField_WaitingCount);
				if (mWaitingCount)	// we are in the server queue
				{
					ShowWaitingCount(mRefNum, mWaitingCount);
					
					if (gApp->mServerVers >= 185)
						mStage = 10;	// waiting stage
					else
					{
						// request xfer connection with server
						mTpt = gApp->StartTransferTransportConnect();
						mStage++;		// next stage
					}
				}
				else
				{
					// request xfer connection with server
					mTpt = gApp->StartTransferTransportConnect();
					mStage++;		// next stage
				}
			}
		}
		break;
		
		// if xfer connection has opened, tell the server to begin
		case 3:
		{
			if (!mTpt)
			{
				Finish();
				return;
			}

			if (mTpt->GetConnectStatus() == kConnectionEstablished)
			{
				// identify ourself to the server
#pragma options align=packed
				struct {
					Uint32 protocol;	// HTXF
					Uint32 refNum;
					Uint32 dataSize;
					Uint16 type;
					Uint16 rsvd;
					
					Uint16 nextMsg;		// this is the GetNextMsg (not part of the struct, but we want to send it all at once)
				} sndData = { TB((Uint32)0x48545846), TB(mRefNum), 0, TB((Uint16)1), 0 , TB((Uint16)dlFldrAction_NextFile)};
#pragma options align=reset
				mTpt->Send(&sndData, sizeof(sndData));
				
				// need time at which we started to generate stats
				mStartSecs = UDateTime::GetSeconds();

				// start the timer if we are in the server queue
				if (mWaitingCount)
					StartWaitingTimer();

				// next stage
				mStage++;
			}
		}
		break;
		
		case 4:
		{
			// this is the header of the file to xfer
			// get the full path, name, size, and type?
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				UApplication::PostMessage(1103);
				SilentFail(errorType_Transport, transError_ConnectionClosed);
			}

			if (!mTpt->IsReadyToReceive(sizeof(Uint16)))
				break;

			mTpt->Receive(data, sizeof(Uint16));
			mHeaderSize = FB(*((Uint16 *)data));
			
			mStage++;
			goto goFromStart;
		}
		break;
		
		case 5:
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				UApplication::PostMessage(1103);
				SilentFail(errorType_Transport, transError_ConnectionClosed);
			}

			if (!mTpt->IsReadyToReceive(mHeaderSize))
				break;

			// receive the header
			StPtr header(mHeaderSize);
			mTpt->Receive(header, mHeaderSize);
			Uint8 *p = BPTR(header);
			Uint16 type = FB(*((Uint16 *)p)++);

			mFilePathSize = UFS::ValidateFilePath(p, mHeaderSize - 2);

			if (mFilePath)
				mFilePath = UMemory::Reallocate(mFilePath, mFilePathSize);
			else
				mFilePath = UMemory::New(mFilePathSize);
	
			UMemory::Copy(mFilePath, p, mFilePathSize);
			mFile = UFS::New(mFldr, mFilePath, mFilePathSize, nil);
				
			if (type & 1)
			{
				bool isFolder;
				if (!mFile->Exists(&isFolder))
				{
					mFile->CreateFolder();
				}
				else if (!isFolder)
				{
					// to do - complain that the file exists but is not a folder!!
				}
					
				*((Uint16 *)data) = TB((Uint16)dlFldrAction_NextFile);
				mTpt->Send(data, sizeof(Uint16));
					
				delete mFile;
				mFile = nil;
					
				if (++mDownloadedItems == mTotalItems)
				{
					// do all sortsa cleanup here
					mTpt->Disconnect();
					delete mTpt;
					mTpt = nil;
						
					Finish();
					if (gApp->mOptions.SoundPrefs.fileTransfer) USound::Play(nil, 135, true);
					return;	// don't goFromStart!
				}
				else
					mStage = 4;
			}
			else if (gApp->FindResumableFile(mFile, false))
			{
				// send a "do it!" with resume data
				mStage++;
					
				THdl hdl = mFile->ResumeUnflatten();
						
				s = hdl->GetSize();
				void *hdlPtr;
						
				StHandleLocker lock(hdl, hdlPtr);
				StPtr buf(s + sizeof(Uint16) + sizeof(Uint16));
				CFlatten flat(buf);
				flat.WriteWord(dlFldrAction_ResumeFile);
				flat.WriteWord(s);
				flat.Write(hdlPtr, s);
					
				mTpt->Send(buf, s + sizeof(Uint16) + sizeof(Uint16));
				mTotalSize -= mFile->GetSize();
			}
			else if (mFile->Exists())
			{
				*((Uint16 *)data) = TB((Uint16)dlFldrAction_NextFile);
				mTpt->Send(data, sizeof(Uint16));

				mTotalSize -= mFile->GetSize();
					
				delete mFile;
				mFile = nil;
										
				if (++mDownloadedItems == mTotalItems)
				{
					// do all sortsa cleanup here
					mTpt->Disconnect();
					delete mTpt;
					mTpt = nil;
					
					Finish();
					if (gApp->mOptions.SoundPrefs.fileTransfer) USound::Play(nil, 135, true);
					return;	// don't goFromStart!
				}
				else
					mStage = 4;
			}
			else
			{
				// send a "do it!" msg
				*((Uint16 *)data) = TB((Uint16)dlFldrAction_SendFile);
				mTpt->Send(data, sizeof(Uint16));
					
			#if WIN32
				// put the .hpf extension
				mFile->GetName(str);
				pstrcat(str, "\p.hpf");
				mFile->SetRefName(str);
			#endif

				mFile->CreateFile('HTft', 'HTLC');
				mFile->StartUnflatten();

				mStage++;
			}
				
			mHeaderSize = 0;
			goto goFromStart;
		}
		break;
		
		case 6:		// receive the total size of this file
		{
			if (!mTpt->IsReadyToReceive(sizeof(Uint32)))
				break;

			mTpt->Receive(data, sizeof(Uint32));
			mFileTotalSize = FB(*((Uint32 *)data));
			mFileDlSize = 0;
				
			mStage++;
			goto goFromStart;
		}
		break;
				
		case 7:
		{
			// xfer the file...
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				UApplication::PostMessage(1103);
				SilentFail(errorType_Transport, transError_ConnectionClosed);
			}
						
			// receive and process data
			void *rcvBuf;
			Uint32 s;
			while (mTpt->ReceiveBuffer(rcvBuf, s))
			{
				// process data
				try
				{
					mFile->ProcessUnflatten(rcvBuf, s);
					mFileDlSize += s;
					mDownloadedSize += s;
					gApp->mTotalBytesDownloaded += s;
				}
				catch(...)
				{
					UTransport::DisposeBuffer(rcvBuf);
					throw;
				}
				
				UTransport::DisposeBuffer(rcvBuf);
			}
						
			// once the file is done, decrement the stage, and go back to 4 for the next file
			// also, increment the # items snagged, and if we've reached the total, kill kill kill
			
			if (mFileDlSize >= mFileTotalSize)
			{
				// complete unflatten
				mFile->ProcessUnflatten(nil, 0);

			#if WIN32
				mFile->GetName(str);

				// take the .hpf out of the name
				if (str[0] >= 4 && !UMemory::Compare(str + str[0] - 3, ".hpf", 4))
				{
					str[0] -= 4;
					
					mFile->StopUnflatten();	// can't rename open file
					mFile->SetName(str);
				}
			#endif

				// delete stuff
				delete mFile;
				mFile = nil;

				if (++mDownloadedItems == mTotalItems)
				{
					// do all sortsa cleanup here
					mTpt->Disconnect();
					delete mTpt;
					mTpt = nil;
					
					Finish();
					if (gApp->mOptions.SoundPrefs.fileTransfer) USound::Play(nil, 135, true);					
				}
				else
				{
					mStage = 4;

					*((Uint16 *)data) = TB((Uint16)dlFldrAction_NextFile);
					mTpt->Send(data, sizeof(Uint16));
				}
			}
			
			// calc stats
			Uint32 downloadedSize = mDownloadedSize;
			Uint32 totalSize = mTotalSize;
			Uint32 elapsedSecs = UDateTime::GetSeconds() - mStartSecs;
			
			if (elapsedSecs > 0 && downloadedSize > elapsedSecs && downloadedSize > 1024)
			{
				Uint32 bps = downloadedSize / elapsedSecs;
				Uint32 etr = (totalSize - downloadedSize) / bps;
				
				// display stats if changed
				if (downloadedSize != mDisplayedSize || bps != mDisplayedBPS || etr != mDisplayedETR)
				{
					Uint8 stat[256];
					gApp->GetTransferStatText(downloadedSize, totalSize, bps, etr, stat);
					
					str[0] = UText::Format(str+1, sizeof(str)-1, "Downloading folder �%#s� (%#s) [%lu of %lu]", mFileFldrName, stat, mDownloadedItems + 1 > mTotalItems ? mTotalItems : mDownloadedItems + 1, mTotalItems);
					ShowProgress(downloadedSize, totalSize, str);

					mDisplayedSize = downloadedSize;
					mDisplayedBPS = bps;
					mDisplayedETR = etr;					
				}
			}
			else if (downloadedSize)
				ShowProgress(downloadedSize, totalSize);
		}
		break;
		
		case 10:
			// do nothing stage, waiting in the Server queue
		break;
	}
}


/* ������������������������������������������������������������������������� */
#pragma mark -
// banner hide
CMyDownloadBannerTask::CMyDownloadBannerTask(Uint32 inBannerType, Uint8 *inBannerURL)
	: CMyTransactTask("\pLoading Server Banner...")
{

	mTpt = nil;
	
	mBanner = nil;
	mBannerType = inBannerType;

	if (inBannerURL && inBannerURL[0])
		UMemory::Copy(mBannerURL, inBannerURL, inBannerURL[0] + 1);
	else
		mBannerURL[0] = 0;
	
	mBannerSize = 0;
	mDownloadedSize = 0;
	mRefNum = 0;

	ShowProgress(0, 1);
				
	// setup field data to send
	mTranSession = NewSendSession(myTran_DownloadBanner);
	mTranSession->SendData(mFieldData);
	//CMyDownloadBannerTask::Process();
	
}
//
// banner hide
CMyDownloadBannerTask::~CMyDownloadBannerTask()
{

	if (mTpt)
		delete mTpt;
		
	if (mBanner)
		UMemory::Dispose(mBanner);
		
}
//
// banner hide
void CMyDownloadBannerTask::Process()
{

	switch (mStage)
	{
		// if server has replied, we can request an xfer connection
		case 1:
		{
			if (mTranSession->IsReceiveComplete())
			{
				// extract reply
				mTranSession->ReceiveData(mFieldData);
				Uint32 error = mTranSession->GetReceiveError();
				
				// don't need session anymore
				delete mTranSession;
				mTranSession = nil;

				// check for server errors
				CheckServerError(error, mFieldData);
				
				// extract info
				mRefNum = mFieldData->GetInteger(myField_RefNum);
				mBannerSize = mFieldData->GetInteger(myField_TransferSize);

				if (!mBannerSize)
				{	
					Finish();
					return;
				}
		
				// request xfer connection with server
				mTpt = gApp->StartTransferTransportConnect();
					
				// next stage
				mStage++;
			}
		}
		break;
		
		// if xfer connection has opened, tell the server to begin
		case 2:
		{
			if (!mTpt)
			{
				Finish();
				return;
			}

			if (mTpt->GetConnectStatus() == kConnectionEstablished)
			{
				// identify ourself to the server
			#pragma options align=packed
				struct {
					Uint32 protocol;	// HTXF
					Uint32 refNum;
					Uint32 dataSize;
					Uint16 type;
					Uint16 rsvd;
				} sndData = { TB((Uint32)0x48545846), TB(mRefNum), 0, TB((Uint16)2), 0 };
			#pragma options align=reset
				mTpt->Send(&sndData, sizeof(sndData));
									
				mBanner = UMemory::New(mBannerSize);
				if (!mBanner)
				{	
					Finish();
					return;
				}
				
				// next stage
				mStage++;						
			}
		}
		break;
		
		// manage the xfer connection (process incoming data etc)
		case 3:
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				gApp->ClearServerBanner();
				
				Finish();
				return;
			}
									
			// receive and process data
			Uint32 nSize;
			void *rcvBuf;
			while (mTpt->ReceiveBuffer(rcvBuf, nSize))
			{
				if (mDownloadedSize < mBannerSize)
					UMemory::Copy((Uint8 *)mBanner + mDownloadedSize, rcvBuf, mDownloadedSize + nSize <= mBannerSize ? nSize : mBannerSize - mDownloadedSize);

				mDownloadedSize += nSize;
				UTransport::DisposeBuffer(rcvBuf);
			}
			
			// check if finished
			if (mDownloadedSize >= mBannerSize)
			{
				// delete stuff
				mTpt->Disconnect();
				delete mTpt;
				mTpt = nil;

				// all done, clean up
				Finish();
				
				// set the banner
				
				gApp->SetServerBanner(mBanner, mBannerSize, mBannerType, mBannerURL, nil);
				UApplication::PostMessage(1114);
			}
			
			if (mDownloadedSize)
				ShowProgress(mDownloadedSize, mBannerSize);
		}
		break;
	}
	
}

/* ������������������������������������������������������������������������� */
#pragma mark -
CMyKillDownloadTask::CMyKillDownloadTask(Uint32 inRefNum)
	: CMyTransactTask(nil)
{
	// setup field data
	mFieldData->AddInteger(myField_RefNum, inRefNum);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_KillDownload);
	mTranSession->SendData(mFieldData);	
}

void CMyKillDownloadTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyUploadTask::CMyUploadTask(TFSRefObj* inSpec, bool inQueued)
	: CMyTransactTask("\p")
{	
	mTpt = nil;
	
	mFile = nil;
	inSpec->GetName(mFileFldrName);

	mStartSecs = 0;
	mTotalSize = 0;
	mUploadedSize = 0;
	mSendSize = 0;
	mSendSecs = 0;
	
	mDisplayedSize = 0;
	mDisplayedBPS = 0;
	mDisplayedETR = 0;
	mRefNum = 0;

	mWasQueued = inQueued;
}

CMyUploadTask::~CMyUploadTask()
{
	if (mTpt)
		delete mTpt;
	
	if (mFile)
		delete mFile;
}

bool CMyUploadTask::QueueUnder()
{
	return !mIsFinished;
}

void CMyUploadTask::Start()
{
	if (mStage == 0)
	{
		mStage = 1;
		gApp->ScheduleTaskProcess();
	}
}


/* ������������������������������������������������������������������������� */
#pragma mark -

// does NOT take ownership of inFile
CMyUploadFileTask::CMyUploadFileTask(const void *inPathData, Uint32 inPathSize, TFSRefObj* inFile, bool inResume, bool inQueued)
	: CMyUploadTask(inFile, inQueued), mDisplayedFinish(false)
{
	Uint8 str[256];
	mSendSize = 51200;

	try
	{
		// set text description in task window
		if (mWasQueued)
			str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading �%#s� (queued)", mFileFldrName);
		else
			str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading �%#s�", mFileFldrName);
	
		ShowProgress(0, 1, str);
		
		// setup field data to send
		mFieldData->AddPString(myField_FileName, mFileFldrName);
		mFieldData->AddField(myField_FilePath, inPathData, inPathSize);
		
		// prepare for file flattening
		mFile = inFile->Clone();
		if (inResume)
			mFieldData->AddInteger(myField_FileXferOptions, 1);
		else
		{
			mFile->StartFlatten();
			mTotalSize = mFile->GetFlattenSize();
			mFieldData->AddInteger(myField_TransferSize, mTotalSize);
		}
		
		gApp->mCacheList.ChangedFileList(inPathData, inPathSize);

		// if being queued, we'll start at an earlier stage
		if (inQueued)
			mStage = 0;

		// ready to rock!!!
		gApp->ScheduleTaskProcess();
		
	}
	catch(...)
	{
		// clean up
		if (mFile)
		{
		
			delete mFile;
			mFile = nil;
		}
		
		throw;
	}
}

CMyUploadFileTask::~CMyUploadFileTask()
{
}

Uint32 CMyUploadFileTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mFileFldrName[0] ?
		UText::Format(outText, inMaxSize, "Uploading file: %#s...", mFileFldrName) :
		UText::Format(outText, inMaxSize, "Uploading file...");
}

void CMyUploadFileTask::Process()
{
	Uint8 str[256];

goFromStart:

	switch (mStage)
	{
		// do nothing if queued and waiting
		case 0:
		{
			if (gApp->IsFirstQueuedTask(this))
			{
				mStage = 1;
				goto goFromStart;
			}
		}
		break;

		// send upload-file transaction
		case 1:
		{
			if (mWasQueued)
			{
				str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading �%#s�", mFileFldrName);
				ShowProgress(0, 10, str);
			}
			
			mTranSession = NewSendSession(myTran_UploadFile);
			mTranSession->SendData(mFieldData);
			
			mStage++;
		}
		break;
		
		// if server has replied, we can request an xfer connection
		case 2:
		{
			if (mTranSession->IsReceiveComplete())
			{
			
				// extract reply
				mTranSession->ReceiveData(mFieldData);
				Uint32 error = mTranSession->GetReceiveError();
				
				// don't need session anymore
				delete mTranSession;
				mTranSession = nil;

				// check for server errors
				CheckServerError(error, mFieldData);
				
				// if applicable, extract resume data
				if (!mFile->IsFlattening())
				{
					Uint8 resumeData[1024];
					Uint32 resumeDataSize = mFieldData->GetField(myField_FileResumeData, resumeData, sizeof(resumeData));
					if (resumeDataSize == 0) Fail(errorType_Misc, error_VersionUnknown);
					mFile->ResumeFlatten(resumeData, resumeDataSize);
					mTotalSize = mFile->GetFlattenSize();
				}
				
				// extract info
				mRefNum = mFieldData->GetInteger(myField_RefNum);
				
				// request xfer connection with server
				mTpt = gApp->StartTransferTransportConnect();

				// next stage
				mStage++;
			}
		}
		break;
		
		// if xfer connection has opened, we can begin sending the data
		case 3:
		{
			if (!mTpt)
			{
				Finish();
				return;
			}

			if (mTpt->GetConnectStatus() == kConnectionEstablished)
			{
				// identify ourself to the server
#pragma options align=packed
				struct {
					Uint32 protocol;	// HTXF
					Uint32 refNum;
					Uint32 dataSize;
					Uint32 rsvd;
				} sndData = { TB((Uint32)0x48545846), TB(mRefNum), TB(mTotalSize), 0 };
#pragma options align=reset
				mTpt->Send(&sndData, sizeof(sndData));
				
				// need time at which we started to generate stats
				mStartSecs = UDateTime::GetSeconds();

				// next stage
				mStage++;
				goto goFromStart;
			}
		}
		break;
		
		// manage the xfer connection (send data etc)
		case 4:
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				if (mUploadedSize >= mTotalSize)
				{
					// delete stuff
					delete mTpt;
					delete mFile;
					mFile = nil;
					mTpt = nil;
					
					// all done, clean up
					if (gApp->mOptions.SoundPrefs.fileTransfer) USound::Play(nil, 135, true);
					Finish();
				}
				else
				{
					UApplication::PostMessage(1103);
					SilentFail(errorType_Transport, transError_ConnectionClosed);
				}
			}
			
			// send more data if necessary
			if (mFile)
			{
				Uint32 s = mTpt->GetUnsentSize();
				
				if ((s <= mSendSize) || mTpt->IsReadyToSend())
				{
					// check for stall
					if (s == 0)					// if STALLED (had no data to send)
						mSendSize += 20480;		// then we didn't send enough
						//bug 2048
					
					// if processed within the last 10 seconds, we didn't send enough
					Uint32 curSecs = UDateTime::GetSeconds();
					if ((curSecs - mSendSecs) < 10)
						mSendSize += 51200;
					
					mSendSecs = curSecs;
					
					// don't let sendSize go over 2M
					if (mSendSize > 2097152)
						mSendSize = 2097152;
					
					// determine size to send
					s = mSendSize;
					Uint32 n = mTotalSize - mUploadedSize;
					if (s > n) s = n;
					
					// read data and send
					void *buf = UTransport::NewBuffer(s);
					try
					{
						mFile->ProcessFlatten(buf, s);
						mTpt->SendBuffer(buf);
					}
					catch(...)
					{
						UTransport::DisposeBuffer(buf);
						throw;
					}
					
					// get ready for next send
					mUploadedSize += s;
					mTpt->NotifyReadyToSend();
					gApp->mTotalBytesUploaded += s;
					
					// check if finished
					if (mUploadedSize >= mTotalSize)
					{
						delete mFile;
						mFile = nil;
					}
				}
			}
									
			// calc stats
			Uint32 uploadedSize = mUploadedSize;
			Uint32 totalSize = mTotalSize;
			Uint32 elapsedSecs = UDateTime::GetSeconds() - mStartSecs;
			
			if (mTpt)
			{
				Uint32 unsentSize = mTpt->GetUnsentSize();
				if (unsentSize > uploadedSize)	// b/c of other stuff we sent before started sending file
					uploadedSize = 0;
				else
					uploadedSize -= unsentSize;
			}
			
			if (uploadedSize >= totalSize)
			{
				if (!mDisplayedFinish)
				{
					str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading �%#s� (finishing)", mFileFldrName);
					ShowProgress(10, 10, str);
					mDisplayedFinish = true;
					
				}
			}
			else if (mFile && elapsedSecs > 0 && uploadedSize > elapsedSecs && uploadedSize > 1024)
			{
				Uint32 bps = uploadedSize / elapsedSecs;
				Uint32 etr = (totalSize - uploadedSize) / bps;
				
				// display stats if changed
				if (uploadedSize != mDisplayedSize || bps != mDisplayedBPS || etr != mDisplayedETR)
				{
					Uint8 stat[256];
					gApp->GetTransferStatText(uploadedSize, totalSize, bps, etr, stat);
					
					str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading �%#s� (%#s)", mFileFldrName, stat);
					
					ShowProgress(uploadedSize, totalSize, str);

					mDisplayedSize = uploadedSize;
					mDisplayedBPS = bps;
					mDisplayedETR = etr;
				}
			}
			else if (uploadedSize)
				ShowProgress(uploadedSize, totalSize);
		}
		break;
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

// does NOT take ownership of inFldr
CMyUploadFldrTask::CMyUploadFldrTask(const void *inPathData, Uint32 inPathSize, TFSRefObj* inFldr, bool inResume, bool inQueued)
	: CMyUploadTask(inFldr, inQueued), mFldr(nil), mFileTotalSize(0), mFileUlSize(0), mTotalItems(0), mUploadedItems(0), mResumeHdrSize(0)
{
	Uint8 str[256];
	mSendSize = 102400;

	try
	{
		// set text description in task window
		if (mWasQueued)
			str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading folder �%#s� (queued)", mFileFldrName);
		else
			str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading folder �%#s�", mFileFldrName);
		
		ShowProgress(0, 1, str);
		
		mFldr = new CMyDLFldr(inFldr->Clone(), nil, true);
		mTotalSize = mFldr->GetTotalSize();
		
		// skip over the first item since it's the root folder...eventually I might snag other options from it
		Int16 pathSize = -1;
		Uint8 *p;
		Uint16 count = 0;
		bool outIsFolder = false;
		if (!mFldr->GetNextItem(p, pathSize, count, &outIsFolder) || !outIsFolder)
		{
			// something's wrong!
			DebugBreak("corrupt folder hierarchy!");
		}
		
		mTotalItems = mFldr->GetTotalItems() - 1;

		// setup field data to send
		mFieldData->AddPString(myField_FileName, mFileFldrName);
		mFieldData->AddField(myField_FilePath, inPathData, inPathSize);
		mFieldData->AddInteger(myField_TransferSize, mTotalSize);
		mFieldData->AddInteger(myField_FldrItemCount, mTotalItems);
		if (inResume) mFieldData->AddInteger(myField_FileXferOptions, 1);
		
		gApp->mCacheList.ChangedFileList(inPathData, inPathSize);
		
		// if being queued, we'll start at an earlier stage
		if (inQueued)
			mStage = 0;

		// ready to rock!!!
		gApp->ScheduleTaskProcess();
		
	}
	catch(...)
	{
		// clean up
		if (mFldr)
		{
			delete mFldr;
			mFldr = nil;
		}

		throw;
	}
}

CMyUploadFldrTask::~CMyUploadFldrTask()
{
	if (mFldr)
		delete mFldr;
}

Uint32 CMyUploadFldrTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mFileFldrName[0] ?
		UText::Format(outText, inMaxSize, "Uploading folder: %#s...", mFileFldrName) :
		UText::Format(outText, inMaxSize, "Uploading folder...");
}

void CMyUploadFldrTask::Process()
{
	Uint32 s, n;
	Uint8 str[256];
	Uint8 data[4];

goFromStart:

	switch (mStage)
	{
		// do nothing if queued and waiting
		case 0:
		{
			if (gApp->IsFirstQueuedTask(this))
			{
				mStage = 1;
				goto goFromStart;
			}
		}
		break;

		// send upload-file transaction
		case 1:
		{
			if (mWasQueued)
			{
				str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading folder �%#s�", mFileFldrName);
				ShowProgress(0, 10, str);
			}
			
			mTranSession = NewSendSession(myTran_UploadFldr);
			mTranSession->SendData(mFieldData);
			
			mStage++;
		}
		break;
				
		// if server has replied, we can request an xfer connection
		case 2:
		{
			if (mTranSession->IsReceiveComplete())
			{
				// extract reply
				mTranSession->ReceiveData(mFieldData);
				Uint32 error = mTranSession->GetReceiveError();
				
				// don't need session anymore
				delete mTranSession;
				mTranSession = nil;

				// check for server errors
				CheckServerError(error, mFieldData);
								
				if (mTotalItems == 0)	// no items in this folder
				{
					Finish();
					return;
				}

				// extract info
				mRefNum = mFieldData->GetInteger(myField_RefNum);
				
				// request xfer connection with server
				mTpt = gApp->StartTransferTransportConnect();

				// next stage
				mStage++;
			}
		}
		break;
		
		// if xfer connection has opened, we can begin sending the data
		case 3:
		{
			if (!mTpt)
			{
				Finish();
				return;
			}

			if (mTpt->GetConnectStatus() == kConnectionEstablished)
			{					
				// identify ourself to the server
#pragma options align=packed
				struct {
					Uint32 protocol;	// HTXF
					Uint32 refNum;
					Uint32 dataSize;
					Uint16 type;
					Uint16 rsvd;
				} sndData = { TB((Uint32)0x48545846), TB(mRefNum), 0, TB((Uint16)1), 0 };
#pragma options align=reset
				mTpt->Send(&sndData, sizeof(sndData));
				
				// need time at which we started to generate stats
				mStartSecs = UDateTime::GetSeconds();

				// next stage
				mStage++;
				goto goFromStart;
			}
		}
		break;
		
		case 4:  // waiting for the next message
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				if (mUploadedItems == mTotalItems && mFileUlSize >= mFileTotalSize)
				{	
					// all done, clean up
					if (gApp->mOptions.SoundPrefs.fileTransfer) USound::Play(nil, 135, true);
					Finish();	
					return;
				}
				else
				{
					UApplication::PostMessage(1103);
					SilentFail(errorType_Transport, transError_ConnectionClosed);
				}
			}

			if (!mTpt->IsReadyToReceive(sizeof(Uint16)))
				break;
			
			mTpt->Receive(data, sizeof(Uint16));
						
			if (FB(*((Uint16 *)data)) == dlFldrAction_NextFile)
			{
goNextFile:
				// send the header for the next file
							
				Uint8 path[1024];
				Int16 maxPathSize = sizeof(path);
				Uint8 *startPtr = path + maxPathSize;
				Uint16 pathCount = 0;
				bool folder = false;

				TFSRefObj* file = mFldr->GetNextItem(startPtr, maxPathSize, pathCount, &folder);
							
				if (!file)
				{
					// we're done!
					if (gApp->mOptions.SoundPrefs.fileTransfer) USound::Play(nil, 135, true);
					Finish();
					return;
				}
							
				mUploadedItems++;
							
				if (maxPathSize < 6)
				{
					// if the path was too large:
					// to do - increment the count
					// check if count == total
					// if so, kill!						
					goto goNextFile;
				}	
														
				if (mFile) delete mFile;
				mFile = file->Clone();

				// skip the first folder in the path since this is our root
				startPtr += 2;
				startPtr += *startPtr + 1;
				pathCount--;
				Uint32 s = path + sizeof(path) - startPtr + 4;
							
				// send that data:
				*--((Uint16 *)startPtr) = TB((Uint16)pathCount);
				*--((Uint16 *)startPtr) = TB((Uint16)(folder ? 1 : 0));
				*--((Uint16 *)startPtr) = TB((Uint16)s);
							
				mTpt->Send(startPtr, s + 2);
				mStage = folder ? 4 : 5;
			}
		}				
		break;
		
		case 5:	// waiting for instructions
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				if (mTpt->GetReceiveSize() >= sizeof(Uint16))
				{
					mTpt->Receive(data, sizeof(Uint16));				
					Uint16 type = FB(*((Uint16 *)data));
						
					if (type == dlFldrAction_NextFile)
					{
						mTotalSize -= mFile->GetSize();

						mStage = 4;
						goto goFromStart;				
					}
					else
					{
						UApplication::PostMessage(1103);
						SilentFail(errorType_Transport, transError_ConnectionClosed);
					}
				}
				else
				{
					UApplication::PostMessage(1103);
					SilentFail(errorType_Transport, transError_ConnectionClosed);
				}
			}

			if (!mTpt->IsReadyToReceive(sizeof(Uint16)))
				break;

			mTpt->Receive(data, sizeof(Uint16));				
			Uint16 type = FB(*((Uint16 *)data));
						
			if (type == dlFldrAction_NextFile)
			{
				mTotalSize -= mFile->GetSize();
				goto goNextFile;
			}
			else if (type == dlFldrAction_SendFile)
			{
				mFile->StartFlatten();
sendFileInfo:							
				mFileTotalSize = mFile->GetFlattenSize();
					
				// send the size and stuff
				*((Uint32 *)data) = FB(mFileTotalSize);
				mTpt->Send(data, sizeof(Uint32));
					
				// check if is a resume file
				Uint32 nFileSize = mFile->GetSize();
				if (nFileSize > mFileTotalSize)
					mTotalSize -= nFileSize - mFileTotalSize;
		
				mStage = 6;
				goto goFromStart;
			}
			else if (type == dlFldrAction_ResumeFile)
			{
				mStage = 10;  
				goto goFromStart;
			}
		}
		break;

		case 6:	// sending data
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				UApplication::PostMessage(1103);
				SilentFail(errorType_Transport, transError_ConnectionClosed);
			}

			// send more data if necessary
			try
			{
				if (mFile)
				{
					s = mTpt->GetUnsentSize();
							
					if ((s <= mSendSize) || mTpt->IsReadyToSend())
					{
						// if STALLED (had no data to send) then we didn't send enough
						if (mFileTotalSize < 102400)
						{
							// we could be stalled because of many small files, and if that's the case, don't overdo it!
							if (mSendSize < mFileTotalSize)
								mSendSize = mFileTotalSize;
						}
						else	// a real stall - let's fix it.
						{
							if (s == 0)
								mSendSize += 102400;
										
							// if processed within the last 10 seconds, we didn't send enough
							Uint32 curSecs = UDateTime::GetSeconds();
							if ((curSecs - mSendSecs) < 10)
								mSendSize += 102400;
								
							mSendSecs = curSecs;
						}
								
						// don't let sendSize go over 2M
						if (mSendSize > 2097152)
							mSendSize = 2097152;
								
						// determine size to send
						s = mSendSize;
						n = mFileTotalSize - mFileUlSize;
						if (s > n) s = n;
								
						// read data and send
						void *buf = UTransport::NewBuffer(s);
						try
						{
							mFile->ProcessFlatten(buf, s);
									
							mTpt->SendBuffer(buf);
						}
						catch(...)
						{
							UTransport::DisposeBuffer(buf);
							throw;
						}
								
						// get ready for next send
						mUploadedSize += s;
						mFileUlSize += s;
						gApp->mTotalBytesUploaded += s;
								
						mTpt->NotifyReadyToSend();
					}
				}
			}
			catch(...)
			{
				// error occured while trying to send data, so kill the download
				mTpt->Disconnect();
				throw;
			}
						
			// calc stats
			Uint32 uploadedSize = mUploadedSize;
			Uint32 totalSize = mTotalSize;
			Uint32 elapsedSecs = UDateTime::GetSeconds() - mStartSecs;
			
			if (elapsedSecs > 0 && uploadedSize > elapsedSecs && uploadedSize > 1024)
			{
				Uint32 bps = uploadedSize / elapsedSecs;
				Uint32 etr = (totalSize - uploadedSize) / bps;
				
				// display stats if changed
				if (uploadedSize != mDisplayedSize || bps != mDisplayedBPS || etr != mDisplayedETR)
				{
					Uint8 stat[256];
					gApp->GetTransferStatText(uploadedSize, totalSize, bps, etr, stat);
					
					str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading folder �%#s� (%#s) [%lu of %lu]", mFileFldrName, stat, mUploadedItems, mTotalItems);
					ShowProgress(uploadedSize, totalSize, str);

					mDisplayedSize = uploadedSize;
					mDisplayedBPS = bps;
					mDisplayedETR = etr;					
				}
			}
			else if (uploadedSize)
				ShowProgress(uploadedSize, totalSize);
			
			// check if finished
			if (mFileUlSize >= mFileTotalSize)
			{
				delete mFile;
				mFile = nil;
			
				if (mUploadedItems == mTotalItems)
				{
					// we're done!
					str[0] = UText::Format(str+1, sizeof(str)-1, "Uploading folder �%#s� (finishing)", mFileFldrName);
					ShowProgress(10, 10, str);
				}

				mFileTotalSize = 0;;
				mFileUlSize = 0;
				mStage = 4;
				goto goFromStart;
			}		
		}
		break;

		case 10:	// resume: receive two byte header
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				UApplication::PostMessage(1103);
				SilentFail(errorType_Transport, transError_ConnectionClosed);
			}

			if (!mTpt->IsReadyToReceive(sizeof(Uint16)))
				break;

			mTpt->Receive(data, sizeof(Uint16));	
			mResumeHdrSize = FB(*((Uint16 *)data));
						
			mStage++;
			goto goFromStart;
		}
		break;
				
		case 11:	// resume: receive the header
		{
			// check if connection closed
			if (!mTpt->IsConnected())
			{
				UApplication::PostMessage(1103);
				SilentFail(errorType_Transport, transError_ConnectionClosed);
			}

			if (!mTpt->IsReadyToReceive(mResumeHdrSize))
				break;

			StPtr hdr(mResumeHdrSize);
			mFile->ResumeFlatten(hdr, mTpt->Receive(hdr, mResumeHdrSize));

			goto sendFileInfo;
		}
		break;
	};
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyOpenUserListTask::CMyOpenUserListTask()
	: CMyTransactTask("\pTransferring list of accounts...")
{
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_GetUserList);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyOpenUserListTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pGetting list of accounts...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMyOpenUserListTask::Process()
{
	Uint32 error;
	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());

	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		Finish();

		// show admin window
		gApp->ShowAdminWin(mFieldData);		
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyAdmInSpectorTask::CMyAdmInSpectorTask()
	: CMyTransactTask("\pTransferring AdmInSpector...")
{
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_GetUserNameList);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyAdmInSpectorTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pGetting list of adminspector...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMyAdmInSpectorTask::Process()
{
	Uint32 error;
	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());

	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		Finish();

		// show admin window
		gApp->ShowAdmInSpectorWin(mFieldData);		
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

// take ownership of inData
CMySetUserListTask::CMySetUserListTask(TFieldData inData)
	: CMyTransactTask("\pSaving list of accounts...")
{
	mFieldData->SetDataHandle(inData->DetachDataHandle());
	UFieldData::Dispose(inData);
		
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_SetUserList);
	mTranSession->SendData(mFieldData);
}

Uint32 CMySetUserListTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pSaving list of accounts...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMySetUserListTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyNewUserTask::CMyNewUserTask(const Uint8 inName[], const Uint8 inLogin[], const Uint8 inPass[], const SMyUserAccess& inAccess)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	Uint8 *p;
	Uint32 s;
	
	pstrcpy(mName, inLogin);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Creating account �%#s�", inLogin);
	ShowProgress(1, 2, str);

	// scramble and store login
	UMemory::Copy(str, inLogin, inLogin[0]+1);
	s = str[0];
	p = str + 1;
	while (s--) { *p = ~(*p); p++; }
	mFieldData->AddPString(myField_UserLogin, str);

	// scramble and store password
	UMemory::Copy(str, inPass, inPass[0]+1);
	s = str[0];
	p = str + 1;
	while (s--) { *p = ~(*p); p++; }
	mFieldData->AddPString(myField_UserPassword, str);

	// setup field data
	mFieldData->AddPString(myField_UserName, inName);
	mFieldData->AddField(myField_UserAccess, &inAccess, sizeof(SMyUserAccess));

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_NewUser);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyNewUserTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Creating account: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Creating account...");
}


void CMyNewUserTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyDeleteUserTask::CMyDeleteUserTask(const Uint8 inLogin[])
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	Uint8 *p;
	Uint32 s;
	
	pstrcpy(mName, inLogin);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Deleting account �%#s�", inLogin);
	ShowProgress(1, 2, str);

	// scramble and store login
	UMemory::Copy(str, inLogin, inLogin[0]+1);
	s = str[0];
	p = str + 1;
	while (s--) { *p = ~(*p); p++; }
	mFieldData->AddPString(myField_UserLogin, str);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_DeleteUser);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyDeleteUserTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Deleting account: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Deleting account...");
}

void CMyDeleteUserTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMySetUserTask::CMySetUserTask(const Uint8 inName[], const Uint8 inLogin[], const Uint8 inPass[], const SMyUserAccess& inAccess)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	Uint8 *p;
	Uint32 s;
	
	pstrcpy(mName, inLogin);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Modifying account �%#s�", inLogin);
	ShowProgress(1, 2, str);

	// scramble and store login
	UMemory::Copy(str, inLogin, inLogin[0]+1);
	s = str[0];
	p = str + 1;
	while (s--) { *p = ~(*p); p++; }
	mFieldData->AddPString(myField_UserLogin, str);

	// scramble and store password
	UMemory::Copy(str, inPass, inPass[0]+1);
	if (str[0] != 1 || str[1] != 0)		// if not "no change" marker
	{
		s = str[0];
		p = str + 1;
		while (s--) { *p = ~(*p); p++; }
	}
	mFieldData->AddPString(myField_UserPassword, str);

	// setup field data
	mFieldData->AddPString(myField_UserName, inName);
	mFieldData->AddField(myField_UserAccess, &inAccess, sizeof(SMyUserAccess));

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_SetUser);
	mTranSession->SendData(mFieldData);
}

Uint32 CMySetUserTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Modifying account: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Modifying account...");
}

void CMySetUserTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyOpenUserTask::CMyOpenUserTask(const Uint8 inLogin[])
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inLogin);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Opening account �%#s�", inLogin);
	ShowProgress(0, 10, str);

	// setup field data
	mFieldData->AddPString(myField_UserLogin, inLogin);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_GetUser);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyOpenUserTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Opening account: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Opening account...");
}

void CMyOpenUserTask::Process()
{
	Uint32 error;
	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());

	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// show edit user window
		gApp->ShowEditUserWin(mFieldData);
		
		// all done
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyDisconnectUserTask::CMyDisconnectUserTask(Uint16 inID, const Uint8 inName[], Uint8 inBanOptions)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inName);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Disconnect user �%#s�", inName);
	ShowProgress(1, 2, str);

	// setup field data
	mFieldData->AddInteger(myField_UserID, inID);
	if (inBanOptions)
	{
		mFieldData->AddInteger(myField_Options, inBanOptions);
			
		if(inBanOptions == 2 && inName[0])
			mFieldData->AddField(myField_Data, inName + 1, inName[0]);
	}

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_DisconnectUser);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyDisconnectUserTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Disconnecting user: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Disconnecting user...");
}

void CMyDisconnectUserTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
/* ������������������������������������������������������������������������� */
#pragma mark -

CMyIconChangeTask::CMyIconChangeTask(Uint16 inID, Uint16 inIconID)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	
	
	mFieldData->AddInteger(myField_IconId, inIconID);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Change Icon...");
	ShowProgress(1, 2, str);
	
	// setup field data
	mFieldData->AddInteger(myField_UserID, inID);
	
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_IconChange);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyIconChangeTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{


// all done
		ShowProgress(2, 2);
		Finish();
	}
void CMyIconChangeTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		
	}
}

/* ������������������������������������������������������������������������� */
/* ������������������������������������������������������������������������� */
#pragma mark -

CMyBlockDownloadTask::CMyBlockDownloadTask(Uint16 inID, Uint16 inblockdownload)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	
	
	mFieldData->AddInteger(myField_BlockDownload, inblockdownload);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Block Download...");
	ShowProgress(1, 2, str);
	
	// setup field data
	mFieldData->AddInteger(myField_UserID, inID);
	
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_BlockDownload);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyBlockDownloadTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{

// all done
		ShowProgress(2, 2);
		Finish();
	}
void CMyBlockDownloadTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		
	}
}

/* ������������������������������������������������������������������������� */
/* ������������������������������������������������������������������������� */
#pragma mark -

CMyFakeRedTask::CMyFakeRedTask(Uint16 inID, Uint16 infakebool)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	
	
	mFieldData->AddInteger(myField_FakeRed, infakebool);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Change Color...");
	ShowProgress(1, 2, str);
	
	// setup field data
	mFieldData->AddInteger(myField_UserID, inID);
	
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_FakeRed);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyFakeRedTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{

// all done
		ShowProgress(2, 2);
		Finish();
	}
void CMyFakeRedTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		
	}
}


#pragma mark -

CMyStandardMessageTask::CMyStandardMessageTask(Uint16 inID, Uint16 inmessage)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	
	
	mFieldData->AddInteger(myField_StandardMessage, inmessage);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "StandardMessage...");
	ShowProgress(1, 2, str);
	
	// setup field data
	mFieldData->AddInteger(myField_UserID, inID);
	
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_StandardMessage);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyStandardMessageTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{

// all done
		ShowProgress(2, 2);
		Finish();
	}
void CMyStandardMessageTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		
	}
}


#pragma mark -
CMyVisibleTask::CMyVisibleTask(Uint16 inID, Uint16 invisiblebool)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	
	
	mFieldData->AddInteger(myField_Visible, invisiblebool);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Change Visibility...");
	ShowProgress(1, 2, str);
	
	// setup field data
	mFieldData->AddInteger(myField_UserID, inID);
	
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_Visible);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyVisibleTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{

// all done
		ShowProgress(2, 2);
		Finish();
	}
void CMyVisibleTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyCrazyServerTask::CMyCrazyServerTask()
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	
	
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Set Crazy Server ;-)");
	ShowProgress(1, 2, str);
	
	
	
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_CrazyServer);
	
}

Uint32 CMyCrazyServerTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{

// all done
		ShowProgress(2, 2);
		Finish();
	}
void CMyCrazyServerTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		
	}
}

/* ������������������������������������������������������������������������� */
/* ������������������������������������������������������������������������� */
#pragma mark -

CMyAdminSpectorTask::CMyAdminSpectorTask()
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	
	
	mFieldData->AddInteger(myField_AdminSpector, 1);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "AdminSpector...");
	ShowProgress(1, 2, str);
	
	
	
	// CMyTransactTask will dispose mTranSession if we fail
	
	mTranSession = NewSendSession(myTran_AdminSpector);
	
	mTranSession->SendData(mFieldData);
	
}

Uint32 CMyAdminSpectorTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{

// all done
		ShowProgress(2, 2);
		Finish();
	}
void CMyAdminSpectorTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyAwayTask::CMyAwayTask(Uint16 inID, Uint16 inawaybool)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	
	
	mFieldData->AddInteger(myField_Away, inawaybool);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Set Away...");
	ShowProgress(1, 2, str);
	
	// setup field data
	mFieldData->AddInteger(myField_UserID, inID);
	
	// CMyTransactTask will dispose mTranSession if we fail
	
	mTranSession = NewSendSession(myTran_Away);
	
	mTranSession->SendData(mFieldData);
	
}

Uint32 CMyAwayTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{

// all done
		ShowProgress(2, 2);
		Finish();
	}
void CMyAwayTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		
	}
}


/* ������������������������������������������������������������������������� */
#pragma mark -
CMyNickChangeTask::CMyNickChangeTask(Uint16 inID, Uint8 inNickName[])
 :CMyTransactTask("\p")
{
	Uint8 str[256];
	
	
	
	mFieldData->AddPString(myField_NickName, inNickName);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Change Name...");
	ShowProgress(1, 2, str);
	
	// setup field data
	mFieldData->AddInteger(myField_UserID, inID);
	
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_NickChange);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyNickChangeTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{

// all done
		ShowProgress(2, 2);
		Finish();
	}
void CMyNickChangeTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyGetClientInfoTask::CMyGetClientInfoTask(Uint16 inUserID, const Uint8 *inUserName)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	mUserID = inUserID;
	UMemory::Copy(mUserName, inUserName, inUserName[0] + 1);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Get info for �%#s�", mUserName);
	ShowProgress(1, 2, str);

	// setup field data
	mFieldData->AddInteger(myField_UserID, mUserID);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_GetClientInfoText);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyGetClientInfoTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mUserName[0] ?
		UText::Format(outText, inMaxSize, "Get info for: %#s...", mUserName) :
		UText::Format(outText, inMaxSize, "Get user info...");
}

void CMyGetClientInfoTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// extract user info
		gApp->SetUserInfoWinContent(mUserID, mUserName, mFieldData);

		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyDeleteFileTask::CMyDeleteFileTask(const void *inPathData, Uint32 inPathSize, const Uint8 inFileName[], bool inIsFolder)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inFileName);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, inIsFolder ? "Delete folder �%#s�" : "Delete file �%#s�", inFileName);
	ShowProgress(1, 2, str);

	// setup field data
	mFieldData->AddPString(myField_FileName, inFileName);
	mFieldData->AddField(myField_FilePath, inPathData, inPathSize);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_DeleteFile);
	mTranSession->SendData(mFieldData);

	gApp->mCacheList.ChangedFileList(inPathData, inPathSize);
	gApp->DoFileRefresh(inPathData, inPathSize);
}

Uint32 CMyDeleteFileTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Deleting file: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Deleting file...");
}

void CMyDeleteFileTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyNewFolderTask::CMyNewFolderTask(const void *inPathData, Uint32 inPathSize, const Uint8 inName[])
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inName);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Create folder �%#s�", inName);
	ShowProgress(1, 2, str);

	// setup field data
	mFieldData->AddPString(myField_FileName, inName);
	mFieldData->AddField(myField_FilePath, inPathData, inPathSize);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_NewFolder);
	mTranSession->SendData(mFieldData);

	gApp->mCacheList.ChangedFileList(inPathData, inPathSize);
	gApp->DoFileRefresh(inPathData, inPathSize);
}

Uint32 CMyNewFolderTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Creating folder: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Creating folder...");
}

void CMyNewFolderTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyGetTrackServListTask::CMyGetTrackServListTask(const Uint8 inAddr[], const Uint8 inName[], Uint16 inTrackerID, const Uint8 inLogin[], const Uint8 inPasswd[])
	: CMyTransactTask("\p"), mTpt(0), mCount(0), mTotalCount(0),
	  mMsgDataSize(0), mMsgType(0), mClearedList(0), mTrackerID(inTrackerID)
{
#if !NEW_TRACKSERV
	#pragma unused(inLogin, inPasswd)
#endif
	
	pstrcpy(mName, inName);

	mTrackerAddress.type = kInternetNameAddressType;
	if (!gApp->mOptions.bTunnelThroughHttp) 
		mTrackerAddress.port = 5498;
	else
		mTrackerAddress.port = 5497;

	mTrackerAddress.name[0] = UMemory::Copy(mTrackerAddress.name + 1, inAddr + 1, inAddr[0]);
	
	mIsStandAlone = true;
	mMsgData = UMemory::NewHandle();
	
	// store login + passwd in the header that gets sent on connect
	Uint8 *p = mLoginHeader;
#if NEW_TRACKSERV
	p+= UMemory::Copy(mLoginHeader, "HTRK\0\2", sizeof(Uint32) + sizeof(Uint16));
	
	Uint32 s = UMemory::Copy(p + 1, inLogin + 1, min((Uint32)31, (Uint32)inLogin[0]));
	p[0] = s;
	p+= s + 1;
	
	s = 31 - s;
	while(s--)		// pad the rest of the data
		*p++ = 0;
		
	s = UMemory::Copy(p + 1, inPasswd + 1, min((Uint32)31, (Uint32)inPasswd[0]));
	p[0] = s;
	p+= s + 1;
	
	s = 31 - s;
	while(s--)
		*p++ = 0;
#else
	p+= UMemory::Copy(mLoginHeader, "HTRK\0\1", sizeof(Uint32) + sizeof(Uint16));

#endif	
	
	Uint8 str[256];
	
	str[0] = UText::Format(str + 1, sizeof(str) - 1, "Listing Tracker \"%#s\"", inName);
	ShowProgress(0, 1, str);
}

CMyGetTrackServListTask::~CMyGetTrackServListTask()
{
	if (!mIsFinished)
		gApp->mTracker->SetStatusMsg(mTrackerID, "\pClosed");
	
	UMemory::Dispose(mMsgData);
	if (mTpt)
	{
		mTpt->Disconnect();
		delete mTpt;
	}
}

Uint16 CMyGetTrackServListTask::GetTrackerID()
{
	return mTrackerID;
}

Uint32 CMyGetTrackServListTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Listing tracker: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Listing tracker...");
}

void CMyGetTrackServListTask::Process()
{
	Uint8 buf[32];
	Uint8 *p;
goFromStart:
	
	switch (mStage)
	{
		// request connection with tracker
		case 1:
		{
			mTpt = UTransport::New(transport_TCPIP, !gApp->mOptions.bTunnelThroughHttp);
			mTpt->SetMessageHandler(gApp->MessageHandler, gApp);
			gApp->StartTransferConnect(mTpt, &mTrackerAddress, mTrackerAddress.name[0]+5);
			mStage++;
		}
		break;
		
		// identify ourself to the server
		case 2:
		{
			try
			{
				// GetConnectStatus throws an SError if connect refused
				if (mTpt->GetConnectStatus() == kConnectionEstablished)
				{
				#if NEW_TRACKSERV
					mTpt->Send(mLoginHeader, sizeof(mLoginHeader));
				#else
					mTpt->Send(mLoginHeader, sizeof(Uint32) + sizeof(Uint16));
				#endif
				
					mStage++;
					goto goFromStart;
				}
			}
			catch(SError &err)
			{
				if(err.type == errorType_Transport)
				{
					// could not connect to this specific tracker
					Uint8 str[128];
					str[0] = UText::Format(str + 1, sizeof(str) - 1, "Tracker %#s refused connection", mName);
					Finish();
					ShowProgress(0, 2, str);	// do this after Finish because Finish sets the progress bar to 100/100
					gApp->mTracker->SetStatusMsg(mTrackerID, "\pRefused");
				}
				else
					throw err;
			}
		}
		break;
		
		// look for server ack
		case 3:
		{
			if (!mTpt->IsConnected()) 
				Fail(errorType_Transport, transError_ConnectionClosed);
			
			if (!mTpt->IsReadyToReceive(6))
				break;

			mTpt->Receive(buf, 6);
				
			if (FB(*(Uint32 *)buf) != 0x4854524B)	// 'HTRK'
				Fail(errorType_Misc, error_FormatUnknown);

		#if NEW_TRACKSERV
			if (FB(*(Uint16 *)(buf+4)) != 2)
		#else
			if (FB(*(Uint16 *)(buf+4)) != 1)
		#endif
				Fail(errorType_Misc, error_VersionUnknown);
				
			mStage++;
			goto goFromStart;
		}
		break;
		
		// look for incoming messages
		case 4:
		{
			if (!mTpt->IsConnected()) 
				Fail(errorType_Transport, transError_ConnectionClosed);

			if (!mTpt->IsReadyToReceive(4))
				break;

			mTpt->Receive(buf, 4);
			mMsgType = FB( *(Uint16 *)buf );
			mMsgDataSize = FB( *(Uint16 *)(buf+2) );
			mRcvdSize = 0;
			UMemory::Reallocate(mMsgData, mMsgDataSize);

			mStage++;
			goto goFromStart;
		}
		break;
		
		// receive message data
		case 5:
		{
			if (!mTpt->IsConnected()) 
				Fail(errorType_Transport, transError_ConnectionClosed);

			if (mTpt->GetReceiveSize())
			{
				StHandleLocker locker(mMsgData, (void*&)p);
				mRcvdSize += mTpt->Receive(p + mRcvdSize, mMsgDataSize - mRcvdSize);
				
				if (mRcvdSize >= mMsgDataSize)	// if finished receiving message data
				{
					mStage--;
					
					if (mMsgType == 1)
					{
						mTotalCount = FB( *(Uint16 *)p );
						mCount += gApp->mTracker->AddListFromData(mTrackerID, p + 4, mMsgDataSize - 4);

						ShowProgress(mCount, mTotalCount);
						
						if (mCount >= mTotalCount)
						{
							ShowProgress(10, 10);
							Finish();
						}
					}
					
					goto goFromStart;
				}
			}
		}
		break;
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyGetFileInfoTask::CMyGetFileInfoTask(const void *inPathData, Uint32 inPathSize, const Uint8 inFileName[])
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inFileName);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Get info for file/folder �%#s�", inFileName);
	ShowProgress(1, 2, str);

	// setup field data to send
	mFieldData->AddPString(myField_FileName, inFileName);
	if (inPathData && inPathSize)
	{
		mFieldData->AddField(myField_FilePath, inPathData, inPathSize);
		mPathData = UMemory::New(inPathData, inPathSize);
		mPathSize = inPathSize;
	}
	else
	{
		mPathData = nil;
		mPathSize = 0;
	}
	
	try
	{
		// CMyTransactTask will dispose mTranSession if we fail
		mTranSession = NewSendSession(myTran_GetFileInfo);
		mTranSession->SendData(mFieldData);
	}
	catch(...)
	{
		UMemory::Dispose(mPathData);
		throw;
	}
}

CMyGetFileInfoTask::~CMyGetFileInfoTask()
{
	UMemory::Dispose(mPathData);
}

Uint32 CMyGetFileInfoTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Get info for: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Getting file info...");
}

void CMyGetFileInfoTask::Process()
{
	Uint32 error, s;
	CMyFileInfoWin *win;
	Uint8 str[256];
	Uint8 comment[256];
	Uint8 name[256];
	Uint8 createDateStr[64];
	Uint8 modifyDateStr[64];
	Uint8 sizeStr[32];
	Uint8 byteSizeStr[32];
	Uint8 typeStr[32];
	Uint8 creatorStr[16];
	SDateTimeStamp createDate, modifyDate;
	Uint32 typeCode;
	Int16 icon;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// extract misc info
		mFieldData->GetPString(myField_FileName, name, sizeof(name));
		mFieldData->GetPString(myField_FileTypeString, typeStr, sizeof(typeStr));
		mFieldData->GetPString(myField_FileCreatorString, creatorStr, sizeof(creatorStr));
		mFieldData->GetPString(myField_FileComment, comment, sizeof(comment));
		typeCode = 0;
		mFieldData->GetField(myField_FileType, &typeCode, sizeof(typeCode));
		
		// extract create date
		createDate.year = createDate.msecs = createDate.seconds = 0;
		mFieldData->GetField(myField_FileCreateDate, &createDate, sizeof(createDate));
	#if CONVERT_INTS
		createDate.year = FB(createDate.year);
		createDate.msecs = FB(createDate.msecs);
		createDate.seconds = FB(createDate.seconds);
	#endif
		createDateStr[0] = UDateTime::DateToText(createDate, createDateStr+1, sizeof(createDateStr)-1, kAbbrevDateText + kTimeWithSecsText);
		
		// extract modify date
		modifyDate.year = modifyDate.msecs = modifyDate.seconds = 0;
		mFieldData->GetField(myField_FileModifyDate, &modifyDate, sizeof(modifyDate));
	#if CONVERT_INTS
		modifyDate.year = FB(modifyDate.year);
		modifyDate.msecs = FB(modifyDate.msecs);
		modifyDate.seconds = FB(modifyDate.seconds);
	#endif
		modifyDateStr[0] = UDateTime::DateToText(modifyDate, modifyDateStr+1, sizeof(modifyDateStr)-1, kAbbrevDateText + kTimeWithSecsText);
		
		// extract file size
		s = mFieldData->GetInteger(myField_FileSize);
		sizeStr[0] = UText::SizeToText(s, sizeStr+1, sizeof(sizeStr)-1, kDontShowBytes);
		byteSizeStr[0] = UText::IntegerToText(byteSizeStr+1, sizeof(byteSizeStr)-1, s, true);
				
		// format info string and determine icon iD
		if (typeCode == TB((Uint32)'fldr'))
		{
			UMemory::Copy(str, name, name[0]+1);
			UText::MakeLowercase(str+1, str[0]);
			
			if (UMemory::Search("upload", 6, str+1, str[0])){
				
				
					if (gApp->mIconSet == 2){
							icon = 3001;
						}else if (gApp->mIconSet == 1){
							icon = 421;
						}
				
				
			}else if (UMemory::Search("drop box", 8, str+1, str[0])){
				
				if (gApp->mIconSet == 2){
							icon = 3002;
						}else if (gApp->mIconSet == 1){
							icon = 196;
						}
						
			}else{
			
			if (gApp->mIconSet == 2){
							icon = 3003;
						}else if (gApp->mIconSet == 1){
							icon = 401;
						}
						
			
			}

			str[0] = UText::Format(str+1, sizeof(str)-1, "%#s\r%#s\rn/a\r%#s\r%#s", typeStr, creatorStr, createDateStr, modifyDateStr);
		}
		else
		{
			str[0] = UText::Format(str+1, sizeof(str)-1, "%#s\r%#s\r%#s (%#s bytes)\r%#s\r%#s", typeStr, creatorStr, sizeStr, byteSizeStr, createDateStr, modifyDateStr);
			icon = HotlineFileTypeToIcon(typeCode);
		}
		
		// read and display info
		win = new CMyFileInfoWin(gApp->mAuxParentWin, typeCode == TB((Uint32)'fldr'));
		try
		{
		#if WIN32
			_SetWinIcon(*win, 215);
		#endif

			win->SetPathData(mPathData, mPathSize);
			mPathData = nil;
			
			win->SetOrigName(name);
			win->SetTitle(name);
			win->SetInfo(name, comment, str, icon);
			win->Show();
		}
		catch(...)
		{
			delete win;
			throw;
		}

		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

CMySetFileInfoTask::CMySetFileInfoTask(const void *inPathData, Uint32 inPathSize, const Uint8 inFileName[], const Uint8 inNewName[], const Uint8 inComment[])
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	pstrcpy(mName, inFileName);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Set info for file/folder �%#s�", inFileName);
	ShowProgress(1, 2, str);

	// setup field data to send
	mFieldData->AddPString(myField_FileName, inFileName);
	if (inPathData && inPathSize) mFieldData->AddField(myField_FilePath, inPathData, inPathSize);
	if (inNewName && inNewName[0]) mFieldData->AddPString(myField_FileNewName, inNewName);
	if (inComment && inComment[0]) mFieldData->AddPString(myField_FileComment, inComment);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_SetFileInfo);
	mTranSession->SendData(mFieldData);

	if (inNewName && inNewName[0]) 
		gApp->mCacheList.ChangedFileList(inPathData, inPathSize);
		gApp->DoFileRefresh(inPathData, inPathSize);
}

Uint32 CMySetFileInfoTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Set info for: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Setting file info...");
}

void CMySetFileInfoTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
		
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyMoveFileTask::CMyMoveFileTask(const Uint8 *inFileName, const void *inPathData, Uint32 inPathSize, const void *inNewPath, Uint32 inNewPathSize)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inFileName);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Move file/folder �%#s�", inFileName);
	ShowProgress(1, 2, str);

	// setup field data to send
	mFieldData->AddPString(myField_FileName, inFileName);
	mFieldData->AddField(myField_FilePath, inPathData, inPathSize);
	mFieldData->AddField(myField_FileNewPath, inNewPath, inNewPathSize);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_MoveFile);
	mTranSession->SendData(mFieldData);
	
	gApp->mCacheList.ChangedFileList(inPathData, inPathSize);
	gApp->mCacheList.ChangedFileList(inNewPath, inNewPathSize);
	gApp->DoFileRefresh(inPathData, inPathSize);
}

Uint32 CMyMoveFileTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)

{
	return mName[0] ?
		UText::Format(outText, inMaxSize, "Moving: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Moving file/folder...");
}

void CMyMoveFileTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyMakeFileAliasTask::CMyMakeFileAliasTask(const Uint8 *inFileName, const void *inPathData, Uint32 inPathSize, const void *inDestPath, Uint32 inDestPathSize)
	: CMyTransactTask("\p")
{
	Uint8 str[256];
	
	pstrcpy(mName, inFileName);
	
	// set text description in task window
	str[0] = UText::Format(str+1, sizeof(str)-1, "Make alias of �%#s�", inFileName);
	ShowProgress(1, 2, str);

	// setup field data to send
	mFieldData->AddPString(myField_FileName, inFileName);
	mFieldData->AddField(myField_FilePath, inPathData, inPathSize);
	mFieldData->AddField(myField_FileNewPath, inDestPath, inDestPathSize);

	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_MakeFileAlias);
	mTranSession->SendData(mFieldData);

	gApp->mCacheList.ChangedFileList(inDestPath, inDestPathSize);
}

Uint32 CMyMakeFileAliasTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	return mName[0] ?
	#if MACINTOSH	
		UText::Format(outText, inMaxSize, "Making alias to: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Making alias...");
	#else
		UText::Format(outText, inMaxSize, "Making link to: %#s...", mName) :
		UText::Format(outText, inMaxSize, "Making link...");
	#endif
}

void CMyMakeFileAliasTask::Process()
{
	Uint32 error;
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyInviteNewChatTask::CMyInviteNewChatTask(const Uint32 *inUserIDs, Uint32 inCount)
	: CMyTransactTask("\pOpening private chat...")
{
	// add user IDs to invite
	for (Uint32 i=0; i!=inCount; i++)
		mFieldData->AddInteger(myField_UserID, inUserIDs[i]);
	
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_InviteNewChat);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyInviteNewChatTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pOpening private chat...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMyInviteNewChatTask::Process()
{
	Uint32 error;
	Uint8 str[256];
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;

		// check for server errors
		CheckServerError(error, mFieldData);
		
		// extract info
		Uint32 userID = mFieldData->GetInteger(myField_UserID);
		Int16 iconID = mFieldData->GetInteger(myField_UserIconID);
		Uint16 userFlags = mFieldData->GetInteger(myField_UserFlags);
		mFieldData->GetPString(myField_UserName, str, sizeof(str));
				
		// show chat win
		CMyPrivateChatWin *win = new CMyPrivateChatWin(gApp->mAuxParentWin);
		try
		{
			Uint8 nUsersTab1, nUsersTab2;
			gApp->mUsersWin->GetUserListView()->GetTabs(nUsersTab1, nUsersTab2);

		#if WIN32
			_SetWinIcon(*win, 415);
		#endif
			win->SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);
			win->SetChatID(mFieldData->GetInteger(myField_ChatID));
			win->SetTextColor(gApp->mOptions.ColorPrivateChat);
			win->GetUserListView()->SetTabs(nUsersTab1, nUsersTab2);
			win->SetAccess();

			if (!win->GetUserListView()->UpdateUser(userID, iconID, userFlags, str+1, str[0]))
				win->GetUserListView()->AddUser(userID, iconID, userFlags, str+1, str[0]);
			
			win->Show();
		}
		catch(...)
		{
			delete win;
			throw;
		}
		
		// all done
		ShowProgress(2, 2);
		Finish();
	}
}

/* ������������������������������������������������������������������������� */
#pragma mark -

CMyJoinChatTask::CMyJoinChatTask(Uint32 inChatID)
	: CMyTransactTask("\pJoining private chat..."), mChatID(inChatID)
{
	mFieldData->AddInteger(myField_ChatID, inChatID);
	
	// CMyTransactTask will dispose mTranSession if we fail
	mTranSession = NewSendSession(myTran_JoinChat);
	mTranSession->SendData(mFieldData);
}

Uint32 CMyJoinChatTask::GetShortDesc(Uint8 *outText, Uint32 inMaxSize)
{
	const Uint8 desc[] = "\pJoining private chat...";
	return UMemory::Copy(outText, desc + 1, min(inMaxSize, (Uint32)desc[0]));
}

void CMyJoinChatTask::Process()
{
	Uint32 error;
	
	ShowProgress(mTranSession->GetReceiveTransferedSize(), mTranSession->GetReceiveSize());
	
	if (mTranSession->IsReceiveComplete())
	{
		// extract reply
		mTranSession->ReceiveData(mFieldData);
		error = mTranSession->GetReceiveError();
		
		// don't need session anymore
		delete mTranSession;
		mTranSession = nil;
		
		CMyPrivateChatWin *chatWin = gApp->GetChatWinByID(mChatID);
		if (chatWin)
		{
			if (error)
			{
				delete chatWin;
				CheckServerError(error, mFieldData);
			}
			
			Uint8 str[256];
			mFieldData->GetPString(myField_ChatSubject, str, sizeof(str));
			chatWin->SetSubject(str);
			
			chatWin->GetUserListView()->DeleteAll();
			chatWin->GetUserListView()->AddListFromFields(mFieldData);
		}
		
		// all done
		Finish();
	}
}

/* ������������������������������������������������������������������������� */

