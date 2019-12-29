/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "Hotline.h"
#include "HotlineAdmin.h"
#include "HotlineArchiveDecoder.h"

void EventCallBack();
void InitSubsTable(Uint8 *outTab);
void BuildSubsTable(const Uint8 *inCharsToScram, Uint8 inNumA, Uint8 inNumB, Uint8 inNumC, Uint8 *ioTab);
void SubsData(const Uint8 *inTab, void *ioData, Uint32 inDataSize);
void ReverseSubsTable(const Uint8 *inTab, Uint8 *outTab);
void GenerateRandomPassword(void *outData, Uint32 inSize);



Uint32 UserID;
void *Msg; 
Uint32 MsgSize;
Uint32 Opt_UserMessage; 

#if WIN32
extern Uint8 _gUseMDIWindow;
extern Uint8 _gMDIWindowMenu;

bool _ActivateNextWindow();
bool _IsTopVisibleModalWindow();
#endif

#if MACINTOSH
extern Int16 gMacMenuBarID;

void _InstallURLHandler();
void _AddResFileToSearchChain(TFSRefObj* inRef);
THdl _GetMacResDetached(Uint32 inType, Int16 inID);
#endif


CMyApplication *gApp = nil;
extern HLCrypt *gCrypt;
SMyColorInfo kDefaultColorInfo;
TFontDesc fd_Default9, fd_Default9Right, fd_Default9Bold, fd_Default9BoldRight, fd_Default9BoldCtr, fd_Default10, fd_Fixed9, fd_Default9BoldCenter;


void main()
{
/*
#if WIN32
	_gUseMDIWindow = true;
	_gMDIWindowMenu = 9;
#endif
*/


	UOperatingSystem::Init();
	UOperatingSystem::InitQuickTime();
			
	try
	{
		UTransport::Init();
		
		if (!UTransport::HasTCP())
			Fail(errorType_Transport, transError_NeedTCP);
			
	#if MACINTOSH
		_InstallURLHandler();
		gMacMenuBarID = 128;
	#endif
		
		gApp = new CMyApplication;
		gApp->StartUp();
		gApp->Run();
		
		delete gCrypt;
		delete gApp;
		
		gCrypt = nil;
		gApp = nil;
	}
	catch(SError& err)
	{
		UError::Alert(err);
	}
}
 
/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

CMyApplication::CMyApplication()
{
	const SKeyCmd keyCmds[] = {
		{	viewID_Options,				key_F1,		kIsKeyCode | modKey_Shortcut	},
		{	viewID_OpenPrivateChat,		key_F4,		kIsKeyCode | modKey_Shortcut	},
		{	viewID_SendPrivMsg,			key_F9,		kIsKeyCode | modKey_Shortcut	},
		{	viewID_ShowServers,			key_F1,		kIsKeyCode						},
		{	viewID_ShowUsers,			key_F2,		kIsKeyCode						},
		{	viewID_ShowChat,			key_F3,		kIsKeyCode						},
		{	viewID_ShowNews,			key_F4,		kIsKeyCode						},
		{	viewID_ShowMsgBoard,		key_F5,		kIsKeyCode						},
		{	viewID_ShowFiles,			key_F6,		kIsKeyCode						},
		{	viewID_ShowTasks,			key_F7,		kIsKeyCode						},
		{	viewID_Broadcast,			key_F8,		kIsKeyCode						},
		{	viewID_BringFrontPrivMsg,	key_F9,		kIsKeyCode						},
		{	viewID_Secret,				key_F12,	kIsKeyCode | modKey_Control		},
		{	viewID_OpenParent,			key_Up,		kIsKeyCode | modKey_Shortcut	},
		{	viewID_FileView,			'e',		modKey_Shortcut					},
		{	viewID_Quit,				'q',		modKey_Shortcut					},
		{	viewID_CloseWindow,			'w',		modKey_Shortcut					},
		{	viewID_CloseAllFileWindows,	'f',		modKey_Shortcut | modKey_Option	}
	
	#if WIN32
		,{	viewID_WindowNext,			key_Tab,	kIsKeyCode | modKey_Control		},
		{	viewID_CloseWindow,			key_F4,		kIsKeyCode | modKey_Alt			}
	#endif
		
	#if !MACINTOSH	// these aren't needed on the mac due to the menubar
		,{	viewID_Connect,				'k',		modKey_Shortcut					},
		{	viewID_Broadcast,			'b',		modKey_Shortcut					},
		{	viewID_ShowNews,			'n',		modKey_Shortcut					},
		{	viewID_ShowMsgBoard,		's',		modKey_Shortcut					},
		{	viewID_ShowUsers,			'u',		modKey_Shortcut					},
		{	viewID_ShowFiles,			'f',		modKey_Shortcut					},
		{	viewID_ShowChat,			'h',		modKey_Shortcut					},
		{	viewID_ShowChat,			'p',		modKey_Shortcut					},
		{	viewID_ShowTasks,			't',		modKey_Shortcut					},
		{	viewID_ShowServers,			'r',		modKey_Shortcut					},
		{	viewID_NewUser,				'm',		modKey_Shortcut					},
		{	viewID_OpenUser,			'o',		modKey_Shortcut					},
		{	viewID_OpenUserList,		'l',		modKey_Shortcut					}
	#endif
	};
	mSoundDisabled = false;
	try
	{
		USound::Init();
	}
	catch (...)
	{
		mSoundDisabled = true;
		USound::SetCanPlay(false);
	}
	
	ClearStruct(mOptions);
	
	*(Uint32 *)&(mOptions.SoundPrefs) = 0xFFFFFFFF;
	mOptions.bQueueTransfers = true;
	mOptions.bShowJoinLeaveInChat = true;
	mOptions.bShowDateTime = true;
	
	mOptions.bShowTooltips = true;

	kDefaultColorInfo.textColor = color_Black;
	kDefaultColorInfo.backColor = color_White;
	kDefaultColorInfo.textSize = kMyDefaultFixedFontSize;

	mOptions.ColorPublicChat = kDefaultColorInfo;
	mOptions.ColorPrivateChat = kDefaultColorInfo;
	mOptions.ColorNews = kDefaultColorInfo;
	mOptions.ColorViewText = kDefaultColorInfo;

	mUserName[0] = 0;
	mTpt = nil;
	mServerVers = 0;
	mIsConnected = mIsAgreed = mAllowChatWithCA = mIsLeechMode = mToggleWinVis = mScramChat = mPorkMode = false;
	mNoBanner = true;
	mIconSet = 1;
	mIconID = 414;
	mWindowPat = 0;
	mUserLogin[0] = mUserPassword[0] = 0;
	mServerName[0] = 0;

	mStartupPath = nil;
	mStartupPathSize = 0;
	mResumableFile = nil;
	
	mAuxParentWin = nil;
	mAboutWin = nil;
	mOldNewsWin = nil;
	mChatWin = nil;
	mTasksWin = nil;
	mToolbarWin = nil;
	mUsersWin = nil;
	mAdminWin = nil;
	mAdmInSpectorWin = nil;
	mNoServerAgreement = false;
	mServerAgreement = nil;
	
	mServerBanner = nil;
	mServerBannerURL[0] = 0;
	mServerBannerComment[0] = 0;
	mServerBannerType = 0;
	mServerBannerCount = 0;

	mTracker = nil;
	
	mTotalBytesUploaded = mTotalBytesDownloaded = mTotalSecsOnline = mTotalChatBytes = 0;
	mAppInBackgroundSecs = mConnectStartSecs = 0;
	mScramTable = mUnscramTable = nil;
	mUnlockName[0] = mUnlockCode[0] = 0;
	

	//mAppTime = BANNER_AUTO_REFRESH_MS;			//	5 minutes
	mServTime = 300000;			//  5 minutes
	mTrakTime = 300000;			//  5 minutes
	mServRatio = 3;
	mTrakRatio = 6;
	
	
	mUserQuitTimer = NewTimer();
	
	mFinishTaskTimer = NewTimer();
	mCloseWinTimer = NewTimer();
	mDisableAutoResponse = NewTimer();
	mKeepConnectionAliveTimer = NewTimer();
	mAboutTimer = NewTimer();
	
#if BANNER_AUTO_REFRESH_MS
	mBannerTimer = NewTimer();
#endif // BANNER_AUTO_REFRESH_MS
	
	mBasePortNum = 0;
	
	ClearStruct(mAddress);
	ClearStruct(mFirewallAddress);
	ClearStruct(mSaveConnect);
	mFirewallAddress.type = kInternetNameAddressType;
	
	mActiveTaskCount = 0;
	mQueuedTaskCount = 0;
		
	mStartupPath = nil;
	mStartupPathSize = 0;
	
	mMyAccess.Clear();
	
	UApplication::SetCanOpenDocuments(true);
	InstallKeyCommands(keyCmds, sizeof(keyCmds) / sizeof(SKeyCmd));
}

void CMyApplication::StartUp()
{
#if MACINTOSH_CARBON
	// reset mouse image
	UApplication::PostMessage(1112);
#endif

	LoadRezFile();

//	if (!UTransport::IsRegisteredForProtocol("\photline"))
	UTransport::RegisterProtocol("\photline");

#if WIN32
	if (!UExternalApp::IsRegisteredAssociation("\phbm"))
		UExternalApp::RegisterAssociation("\phbm", "\pHotline Bookmark");
	
	if (!UExternalApp::IsRegisteredAssociation("\phpf"))
		UExternalApp::RegisterAssociation("\phpf", "\pHotline Partial File");
#endif

	SRect r;
	
#if WIN32
    // some funny windoze bug makes it think the shift 
    // key is down when it's not for a number of users
	bool fullStartup = true;	
#else
	bool fullStartup = !UKeyboard::IsShiftKeyDown();
#endif
		
#if MACINTOSH &! NEW_SPLASH
	if (fullStartup)
	{
		mSplashWin = NewPictureWindow(130, windowLayer_Modal);
		mSplashWin->Show();
		mSplashWin->GetBounds(r);
		r.MoveTo(0,0);
		mSplashWin->Draw(r);
	}
#endif

#if MACINTOSH_CARBON || WIN32	
	// make parent win
	mAuxParentWin = new CWindow(SRect(0,0,1,1), windowLayer_Standard);
#endif

	// this must be created prior to prefs being read
	
	//	iconset

	//read IconTheme
	ReadCustomInfo();
	TFSRefObj* fp = nil;
	SMyPrefs info;
	try
	{
		fp = GetPrefsRef();
		fp->Open(perm_Read);
		Uint32 nOffset = sizeof(info);
		mTracker->ReadPrefs(fp, nOffset, info.winTabs.trackerTab1, info.winTabs.trackerTab2, true);
		delete fp;
	}catch(...)
	{
	//DebugBreak("error");
		// clean up
		delete fp;
	}	
		
	//
	mTracker = new CMyTracker(mAuxParentWin);
	//ReadCustomInfo();
	//SMyPrefs info;
	ReadPrefs(&info);
	ReadCookies();
   // mTracker = new CMyTracker(mAuxParentWin);
#if USE_NEWS_HISTORY
	// setup news history
	TFSRefObj* nzHistRef = nil;
	try
	{
		nzHistRef = GetNewsHistRef();
		mNZHist = new CMyNewsReadHistory(nzHistRef);
	}
	catch(...)
	{
		if (nzHistRef)
			delete nzHistRef;
		
		throw;
	}
#endif		

	UMouse::SetImage(mouseImage_Wait);
	
	// create font descriptions
	fd_Default9 = UFontDesc::New(kDefaultFont, nil, 9);
	fd_Default9->SetLock(true);
	fd_Default9Right = UFontDesc::New(kDefaultFont, nil, 9);
	fd_Default9Right->SetAlign(textAlign_Right);
	fd_Default9Right->SetLock(true);
	fd_Default9Bold = UFontDesc::New(kDefaultFont, nil, 9, fontEffect_Bold);
	fd_Default9Bold->SetLock(true);
	fd_Default9BoldRight = UFontDesc::New(kDefaultFont, nil, 9, fontEffect_Bold);
	fd_Default9BoldRight->SetAlign(textAlign_Right);
	fd_Default9BoldRight->SetLock(true);
	fd_Default9BoldCtr = UFontDesc::New(kDefaultFont, nil, 9, fontEffect_Bold);
	fd_Default9BoldCtr->SetAlign(textAlign_Center);
	fd_Default9BoldCtr->SetLock(true);
	fd_Default10 = UFontDesc::New(kDefaultFont, nil, 10);
	fd_Default10->SetLock(true);
	fd_Fixed9 = UFontDesc::New(kFixedFont, nil, kMyDefaultFixedFontSize);
	fd_Fixed9->SetLock(true);
#if MACINTOSH
	fd_Default9BoldCenter = UFontDesc::New(kDefaultFont, nil, 9, fontEffect_Bold);
	fd_Default9BoldCenter->SetAlign(textAlign_Center);
	fd_Default9BoldCenter->SetLock(true);
#else
	fd_Default9BoldCenter = UFontDesc::New(kSansFont, nil, 9, fontEffect_Bold);
	fd_Default9BoldCenter->SetAlign(textAlign_Center);
	fd_Default9BoldCenter->SetLock(true);
#endif
	
#if MACINTOSH
	// load custom soundset
	{
		StFileSysRef soundsetFile(kProgramFolder, nil, "\pSoundset", fsOption_PreferExistingFile);
		if (soundsetFile.IsValid() && soundsetFile->GetTypeCode() == TB((Uint32)'HTss'))
			_AddResFileToSearchChain(soundsetFile);
	}

	// load custom icons
	{
		StFileSysRef iconsFile(kProgramFolder, nil, "\pUser Icons", fsOption_PreferExistingFile);
		if (iconsFile.IsValid())
			_AddResFileToSearchChain(iconsFile);
	}
#endif
if (mOptions.bShowBanner){
				mNoBanner = false;
				//DebugBreak("false");
				
	}else{
				mNoBanner = true;
				
			//DebugBreak("true");	
	}
	
if (mOptions.bOriginalIconSet){
				mIconSet = 1;
				
	}else if (mOptions.bFampastelIconSet){
				mIconSet = 2;
				
	}else{
	mIconSet = 1;
	}
	//DebugBreak("%i",GiveIconSet());

	// make toolbar window
	DoCreateToolbar(info.winRect.toolbar);

	// make tasks window
	mTasksWin = new CMyTasksWin(mAuxParentWin);
	if (info.winRect.tasks.IsEmpty())
		mTasksWin->SetAutoBounds(windowPos_BottomRight, windowPosOn_WinScreen);
	else
		mTasksWin->SetBounds(info.winRect.tasks);
	
	mTasksWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
	mTasksWin->GetBounds(info.winRect.tasks);

	if (info.winVis.tasks && fullStartup) mTasksWin->Show();

	// make chat window
	mChatWin = new CMyPublicChatWin(mAuxParentWin);
	mChatWin->SetTextColor(mOptions.ColorPublicChat);

	if (info.winRect.chat.IsEmpty())
		mChatWin->SetAutoBounds(windowPos_BottomLeft, windowPosOn_WinScreen);
	else
	{
		mChatWin->SetBounds(info.winRect.chat);
		mChatWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
	}

	if (info.winVis.chat && fullStartup) mChatWin->Show();
	
	// make users window
	mUsersWin = new CMyUserListWin(mAuxParentWin);
	if (info.winRect.users.IsEmpty())
		mUsersWin->SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);	
	else
	{
		mUsersWin->SetBounds(info.winRect.users);
		mUsersWin->GetUserListView()->SetTabs(info.winTabs.usersTab1, info.winTabs.usersTab2);
	}
	
	mUsersWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
	mUsersWin->GetBounds(info.winRect.users);

	if (info.winVis.users && fullStartup) mUsersWin->Show();

	// set toolbar pattern on windows
	if (mWindowPat)
	{
		mTasksWin->SetPattern(mWindowPat);
		mUsersWin->SetPattern(mWindowPat);
	
		UWindow::SetBackPattern(*mChatWin, mWindowPat);
	}
	
	// bring to front the toolbar
	if (dynamic_cast<CWindow *>(mToolbarWin))
		dynamic_cast<CWindow *>(mToolbarWin)->BringToFront();
    
	
	if (mOptions.bMagneticWindows)
	{
		if (mOptions.SoundPrefs.magneticWindows)
			UWindow::EnableSnapWindows(mOptions.bMagneticWindows, 137, 138);
		else
			UWindow::EnableSnapWindows(mOptions.bMagneticWindows);
	}
	
	// init quicktime
	if (UOperatingSystem::IsQuickTimeAvailable())
	{
		HL_SetProgressProc(CMyViewFileWin::ProgressMovieProc);	
		HL_SetEventProc(EventCallBack);	
	}
	
	// display sound disabled message
	if (mSoundDisabled) 
		UApplication::PostMessage(msg_Special);
		
#if NEW_SPLASH
	// show the about window and have it hide in 3 seconds
	if(mOptions.bShowSplash)
				{
	UApplication::PostMessage(1110);
	mAboutTimer->Start(3000);
	}
#endif

	// reset mouse image
	UMouse::SetImage(mouseImage_Standard);	


}

CMyApplication::~CMyApplication()
{
}


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Application Functions ₯₯

void CMyApplication::UserQuit()
{	
	if (!mUserQuitTimer)
		return;
		
	if (!CanQuit())
	{
		mUserQuitTimer->Start(500, kOnceTimer);
		return;
	}
	
	UTimer::Dispose(mUserQuitTimer);
	mUserQuitTimer = nil;

	DoDisconnect();

	UTimer::Dispose(mFinishTaskTimer);
	mFinishTaskTimer = nil;
	
	UTimer::Dispose(mCloseWinTimer);
	mCloseWinTimer = nil;
	
	UTimer::Dispose(mDisableAutoResponse);
	mDisableAutoResponse = nil;
	
#if BANNER_AUTO_REFRESH_MS
	UTimer::Dispose(mBannerTimer);
	mBannerTimer = nil;
#endif // BANNER_AUTO_REFRESH_MS
	
	mAuxChildrenList.Clear();

	ClearStartupInfo();
	ClearOptions();

	try {
		WritePrefs();
	} catch(...) {}		// even if we can't write the prefs, we want to let the user quit anyway
	WriteCookies();

	if (mTracker)
	{
		delete mTracker;
		mTracker = nil;
	}
	
	CApplication::UserQuit();
}

void CMyApplication::KeyCommand(Uint32 inCmd, const SKeyMsgData& inInfo)
{
#if MACINTOSH_CARBON || WIN32
	// if the toolbar is collapsed don't process any message
	if (dynamic_cast<CWindow *>(mToolbarWin) && dynamic_cast<CWindow *>(mToolbarWin)->IsCollapsed())
		return;	
#endif
	
	// if this is a tracker msg, let the CTracker take care of it
	if (mTracker && mTracker->KeyCommand(inCmd, inInfo))
		return;

	switch (inCmd)
	{
		case viewID_Options:
			DoOptions();
			break;
		case viewID_Connect:
			CWindow *win = CWindow::GetFront(windowLayer_Standard);
			if (win && mTracker && win == mTracker->GetServersWindow())
				DoConnectToTracked(true);	// call this with true if you wanna show the connect window, false if not
			else
				DoShowConnect();
			break;
		case viewID_ShowChat:
			DoShowChatWin();
			break;
		case viewID_ShowNews:
		#if WIN32
			if (inInfo.mods & modKey_Alt)	// Alt-F4
			{
				if (dynamic_cast<CWindow *>(mToolbarWin) && dynamic_cast<CWindow *>(mToolbarWin)->GetState() == windowState_Focus)
					DoCloseWindow(dynamic_cast<CWindow *>(mToolbarWin));
				else
					DoCloseWindow(CWindow::GetFront(windowLayer_Standard));
			}
			else
		#endif
				DoShowNewsWin();
			break;
		case viewID_ShowMsgBoard:
			DoShowMsgBoardWin();
			break;
		
		case viewID_ShowFiles:
			DoShowPublicFiles();
			break;
		case viewID_ShowUsers:
			DoShowUsersWin();
			break;
		case viewID_ShowTasks:
			DoShowTasksWin();
			break;
		case viewID_Broadcast:
			DoBroadcast();
			break;
		case viewID_NewUser:
			DoNewUser();
			break;
		case viewID_OpenUser:
			DoOpenUser();
			break;
		case viewID_OpenUserList:
			DoOpenUserList();
			break;
			
		
		case viewID_ServerInfo:
			DoShowServerInfo();
			break;
		case viewID_ServerBookmark:
			DoSaveServerBookmark();
			break;
		case viewID_ShowServers:
			DoShowTracker();
			break;
		case viewID_BringFrontPrivMsg:
			DoBringFrontPrivMsg();
			break;
		case viewID_Secret:
			DoSecret();
			break;
			
		case viewID_Flood:
			DoFlood();
			break;
		case viewID_Quit:
			if (ConfirmQuit())
				UserQuit();
			break;
		case viewID_CloseWindow:
			DoCloseWindow(CWindow::GetFront(windowLayer_Standard));
			break;
		case viewID_CloseAllFileWindows:
			DoCloseAllFileWindows();
			break;
	#if WIN32
		case viewID_WindowNext:
			_ActivateNextWindow();
			break;
	#endif
		case viewID_OpenParent:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win)
			{
				if (dynamic_cast<CMyFileWin *>(win))
					(dynamic_cast<CMyFileWin *>(win))->DoOpenParent(inInfo.mods);
				else if (dynamic_cast<CMyNewsCategoryWin *>(win))
					(dynamic_cast<CMyNewsCategoryWin *>(win))->DoOpenParent(inInfo.mods);
				else if (dynamic_cast<CMyNewsArticleTreeWin *>(win))
					DoNewsCatOpenParent((CMyNewsArticleTreeWin *)win);
				else if (dynamic_cast<CMyNewsArticTextWin *>(win))
					DoNewsArticOpenParent((CMyNewsArticTextWin *)win);
			}
			break;
		
		case viewID_GetInfo:
		
			win = CWindow::GetFront(windowLayer_Standard);
			if (win)
			{
				if (dynamic_cast<CMyFileWin *>(win))
					(dynamic_cast<CMyFileWin *>(win))->DoGetInfo();
				else if (dynamic_cast<CMyUserWin *>(win))
					(dynamic_cast<CMyUserWin *>(win))->DoGetInfo();
			}
			break;
			
		
		case viewID_New:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win)
			{
				if (dynamic_cast<CMyFileWin *>(win))
					(dynamic_cast<CMyFileWin *>(win))->DoNewFolder();
				else if (dynamic_cast<CMyNewsCategoryWin *>(win))
					(dynamic_cast<CMyNewsCategoryWin *>(win))->DoNewFolder();
			}
			break;

		case viewID_NewArticle:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win)
			{
				if (dynamic_cast<CMyNewsArticleTreeWin *>(win))
					(dynamic_cast<CMyNewsArticleTreeWin *>(win))->DoNewArticle();
				else if (dynamic_cast<CMyNewsCategoryExplWin *>(win))
					(dynamic_cast<CMyNewsCategoryExplWin *>(win))->DoNewArticle();
			}	
			break;
		
		case viewID_Refresh:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win)
			{
				if (dynamic_cast<CMyFileWin *>(win))
					(dynamic_cast<CMyFileWin *>(win))->DoRefresh(inInfo.mods);
				else if (dynamic_cast<CMyNewsCategoryWin *>(win))
					(dynamic_cast<CMyNewsCategoryWin *>(win))->DoRefresh(inInfo.mods);
				else if (dynamic_cast<CMyNewsArticleTreeWin *>(win))
					(dynamic_cast<CMyNewsArticleTreeWin *>(win))->DoRefresh();
				else if (mTracker && win == mTracker->GetServersWindow())
					DoRefreshTracker(inInfo.mods);
				else if (dynamic_cast<CMyUserInfoWin *>(win))
					DoUserInfoRefresh((CMyUserInfoWin *)win);
			}
			break;
	
		case viewID_Delete:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win)
			{
				if (dynamic_cast<CMyFileWin *>(win))
					(dynamic_cast<CMyFileWin *>(win))->DoDelete();
				else if (dynamic_cast<CMyNewsCategoryWin *>(win))
					(dynamic_cast<CMyNewsCategoryWin *>(win))->DoDelete();
				else if (dynamic_cast<CMyNewsArticleTreeWin *>(win))
					(dynamic_cast<CMyNewsArticleTreeWin *>(win))->DoDelete();
				else if (win == mUsersWin)
					DoDisconnectUser(inInfo.mods);
				else if (dynamic_cast<CMyUserInfoWin*>(win))
					DoDisconnectUser((CMyUserInfoWin *)win, inInfo.mods);
				else if (mTracker && win == mTracker->GetServersWindow())
					mTracker->RemoveSelectedItem();
			}
			break;

		case viewID_OpenPrivateChat:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyUserWin *>(win))
				(dynamic_cast<CMyUserWin *>(win))->DoPrivateChat();
			break;
		case viewID_SendPrivMsg:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyUserWin *>(win))
				(dynamic_cast<CMyUserWin *>(win))->DoPrivateMsg();
			break;
			
			
		
		case viewID_FileDownload:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyFileWin *>(win))
				(dynamic_cast<CMyFileWin *>(win))->DoDownload(inInfo.mods);
			break;
		case viewID_FileUpload:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyFileWin *>(win))
				(dynamic_cast<CMyFileWin *>(win))->DoUpload(inInfo.mods);
			break;
		case viewID_FileView:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyFileWin *>(win))
				(dynamic_cast<CMyFileWin *>(win))->DoView(inInfo.mods);
			break;

		case viewID_NewsFldrNewCat:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyNewsCategoryWin *>(win))
				(dynamic_cast<CMyNewsCategoryWin *>(win))->DoNewCategory();
			break;
		case viewID_NewsArticSend:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyNewsArticTextWin*>(win))
				DoNewsArticSend((CMyNewsArticTextWin *)win);
			break;
		case viewID_NewsArticGoPrev:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyNewsArticTextWin*>(win))
				DoNewsArticGoPrev((CMyNewsArticTextWin *)win, inInfo.mods);
			break;
		case viewID_NewsArticGoNext:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyNewsArticTextWin*>(win))
				DoNewsArticGoNext((CMyNewsArticTextWin *)win, inInfo.mods);
			break;
		case viewID_NewsArticGoParent:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyNewsArticTextWin*>(win))
				DoNewsArticGoParent((CMyNewsArticTextWin *)win, inInfo.mods);
			break;
		case viewID_NewsArticGo1stChild:
			win = CWindow::GetFront(windowLayer_Standard);
			if (win && dynamic_cast<CMyNewsArticTextWin*>(win))
				DoNewsArticGoFirstChild((CMyNewsArticTextWin *)win, inInfo.mods);
			break;
	}
}

void CMyApplication::WindowHit(CWindow *inWindow, const SHitMsgData& inInfo)
{
	// if this is a tracker msg, let the CTracker take care of it
	if (mTracker && mTracker->WindowHit(inWindow, inInfo))
		return;
	
	switch (inInfo.id)
	{
		case viewID_Broadcast:
			DoBroadcast();
			break;
		case viewID_OldNewsPost:
			DoOldPostNewsWin();
			break;
		case viewID_Options:
			DoOptions();
			break;
		
		case viewID_Connect:
			if (mTracker && inWindow == mTracker->GetServersWindow())
				DoConnectToTracked(true);	// call this with true if you wanna show the connect window, false if not
			else
				DoShowConnect();
			break;
		
		case viewID_Disconnect:
			if (inInfo.param & modKey_Option)
				DoShowConnect();
			else if (ConfirmDisconnect())
				DoDisconnect();
			break;
	

		case viewID_ChatSend:
			if (inInfo.type == hitType_Standard)	// have to do it this way because there are other hits (hitType_Change)
				DoChatSend(false);
			else if (inInfo.type == hitType_Alternate)
				DoChatSend(true);
			break;
		case viewID_PrivChatSend:
			if (inInfo.type == hitType_Standard)
				DoPrivChatSend((CMyPrivateChatWin *)inWindow, false);
			else if (inInfo.type == hitType_Alternate)
				DoPrivChatSend((CMyPrivateChatWin *)inWindow, true);
			break;
		
		case viewID_OpenParent:
			if (dynamic_cast<CMyFileWin *>(inWindow))
				(dynamic_cast<CMyFileWin *>(inWindow))->DoOpenParent(inInfo.param);
			else if (dynamic_cast<CMyNewsCategoryWin *>(inWindow))
				(dynamic_cast<CMyNewsCategoryWin *>(inWindow))->DoOpenParent(inInfo.param);
			else if (dynamic_cast<CMyNewsArticleTreeWin *>(inWindow))
				DoNewsCatOpenParent((CMyNewsArticleTreeWin *)inWindow);
			else if (dynamic_cast<CMyNewsArticTextWin *>(inWindow))
				DoNewsArticOpenParent((CMyNewsArticTextWin *)inWindow);
			break;

		case viewID_GetInfo:
			if (dynamic_cast<CMyFileWin *>(inWindow))
				(dynamic_cast<CMyFileWin *>(inWindow))->DoGetInfo();
			else if (dynamic_cast<CMyUserWin *>(inWindow))
				(dynamic_cast<CMyUserWin *>(inWindow))->DoGetInfo();
			break;
			
		
		
		case viewID_UserInfoOpenUser:
			DoGetUserAccountInfo((CMyUserInfoWin *)inWindow);
			break;
		
		case viewID_New:
			if (dynamic_cast<CMyFileWin *>(inWindow))
				(dynamic_cast<CMyFileWin *>(inWindow))->DoNewFolder();
			else if (dynamic_cast<CMyNewsCategoryWin *>(inWindow))
				(dynamic_cast<CMyNewsCategoryWin *>(inWindow))->DoNewFolder();
			break;
		
		case viewID_NewArticle:
			if (dynamic_cast<CMyNewsArticleTreeWin *>(inWindow))
				(dynamic_cast<CMyNewsArticleTreeWin *>(inWindow))->DoNewArticle();
			else if (dynamic_cast<CMyNewsCategoryExplWin *>(inWindow))
				(dynamic_cast<CMyNewsCategoryExplWin *>(inWindow))->DoNewArticle();
			break;
		
		case viewID_Refresh:
			if (dynamic_cast<CMyFileWin *>(inWindow))
				(dynamic_cast<CMyFileWin *>(inWindow))->DoRefresh(inInfo.param);
			else if (dynamic_cast<CMyNewsCategoryWin *>(inWindow))
				(dynamic_cast<CMyNewsCategoryWin *>(inWindow))->DoRefresh(inInfo.param);
			else if (dynamic_cast<CMyNewsArticleTreeWin *>(inWindow))
				dynamic_cast<CMyNewsArticleTreeWin *>(inWindow)->DoRefresh();
			else if (mTracker && inWindow == mTracker->GetServersWindow())
				DoRefreshTracker(inInfo.param);
			else if (dynamic_cast<CMyUserInfoWin *>(inWindow))
				DoUserInfoRefresh((CMyUserInfoWin *)inWindow);
			break;
		
		case viewID_Delete:
			if (dynamic_cast<CMyFileWin *>(inWindow))
				(dynamic_cast<CMyFileWin *>(inWindow))->DoDelete();
			else if (dynamic_cast<CMyNewsCategoryWin *>(inWindow))
				(dynamic_cast<CMyNewsCategoryWin *>(inWindow))->DoDelete();
			else if (dynamic_cast<CMyNewsArticleTreeWin *>(inWindow))
				(dynamic_cast<CMyNewsArticleTreeWin *>(inWindow))->DoDelete();
			else if (inWindow == mUsersWin)
				DoDisconnectUser(inInfo.param);
			else if (dynamic_cast<CMyUserInfoWin*>(inWindow))
				DoDisconnectUser((CMyUserInfoWin *)inWindow, inInfo.param);
			else if (mTracker && inWindow == mTracker->GetServersWindow())
				mTracker->RemoveSelectedItem();
			break;

		case viewID_FileDownload:
			if (dynamic_cast<CMyFileWin *>(inWindow))
				(dynamic_cast<CMyFileWin *>(inWindow))->DoDownload(inInfo.param);
			break;
		case viewID_FileUpload:
			if (dynamic_cast<CMyFileWin *>(inWindow))
				(dynamic_cast<CMyFileWin *>(inWindow))->DoUpload(inInfo.param);
			break;
		case viewID_FileView:
			if (dynamic_cast<CMyFileWin *>(inWindow))
				(dynamic_cast<CMyFileWin *>(inWindow))->DoView(inInfo.param);
			break;
				
		case viewID_FileList:
			if (inInfo.type == hitType_Drop)
			{
				Uint8 *pFileInfo = BPTR(inInfo.data);
				
				if (inInfo.param == flavor_File)
					(dynamic_cast<CMyFileWin *>(inWindow))->DoUploadFromDrop(false);
				else if (inInfo.param == 'HLFN')
					(dynamic_cast<CMyFileWin *>(inWindow))->DoMove(inInfo.part, pFileInfo, pFileInfo + pFileInfo[0] + 3, *(Uint16 *)(pFileInfo + pFileInfo[0] + 1), false);
			}
			else
				(dynamic_cast<CMyFileWin *>(inWindow))->DoDoubleClick(inInfo.param, false);
			break;

		case viewID_FileTree:
			if (inInfo.type == hitType_Drop)
			{
				Uint8 *pFileInfo = BPTR(inInfo.data);
				
				if (inInfo.param == flavor_File)
					(dynamic_cast<CMyFileWin *>(inWindow))->DoUploadFromDrop(true);
				else if (inInfo.param == 'HLFN')
					(dynamic_cast<CMyFileWin *>(inWindow))->DoMove(inInfo.part, pFileInfo, pFileInfo + pFileInfo[0] + 3, *(Uint16 *)(pFileInfo + pFileInfo[0] + 1), true);
			}
			else
				(dynamic_cast<CMyFileWin *>(inWindow))->DoDoubleClick(inInfo.param, true);
			break;
		
		case viewID_FileExplTree:
			if (inInfo.type == hitType_Drop)
			{
				Uint8 *pFileInfo = BPTR(inInfo.data);
				
				if (inInfo.param == flavor_File)
					(dynamic_cast<CMyFileWin *>(inWindow))->DoUploadFromDrop(true);
				else if (inInfo.param == 'HLFN')
					(dynamic_cast<CMyFileWin *>(inWindow))->DoMove(inInfo.part, pFileInfo, pFileInfo + pFileInfo[0] + 3, *(Uint16 *)(pFileInfo + pFileInfo[0] + 1), true);
			}
			else
				(dynamic_cast<CMyFileWin *>(inWindow))->DoDoubleClick(inInfo.param, true);
			break;

		case viewID_FileExplList:
			if (inInfo.type == hitType_Drop)
			{
				Uint8 *pFileInfo = BPTR(inInfo.data);
				
				if (inInfo.param == flavor_File)
					(dynamic_cast<CMyFileWin *>(inWindow))->DoUploadFromDrop(false);
				else if (inInfo.param == 'HLFN')
					(dynamic_cast<CMyFileWin *>(inWindow))->DoMove(inInfo.part, pFileInfo, pFileInfo + pFileInfo[0] + 3, *(Uint16 *)(pFileInfo + pFileInfo[0] + 1), false);
			}
			else
				(dynamic_cast<CMyFileWin *>(inWindow))->DoDoubleClick(inInfo.param, false);
			break;

		case viewID_FileSaveInfo:
			DoFileSaveInfo((CMyFileInfoWin *)inWindow);
			break;
		case viewID_FileInfoChange:
			((CMyFileInfoWin *)inWindow)->EnableSaveButton();
			break;

		// news folder window
		case viewID_NewsFldrList:
		case viewID_NewsFldrTree:
		case viewID_NewsFldrExplTree:
			if (dynamic_cast<CMyNewsCategoryWin *>(inWindow))
				(dynamic_cast<CMyNewsCategoryWin *>(inWindow))->DoDoubleClick(inInfo.param);
			break;
		case viewID_NewsFldrExplList:
			if (dynamic_cast<CMyNewsCategoryExplWin *>(inWindow))
				(dynamic_cast<CMyNewsCategoryExplWin *>(inWindow))->DoOpenArticle();
			break;
			
		case viewID_NewsFldrNewCat:
			if (dynamic_cast<CMyNewsCategoryWin *>(inWindow))
				(dynamic_cast<CMyNewsCategoryWin *>(inWindow))->DoNewCategory();
			break;
		
		// category window (article list window)
		case viewID_NewsCatOpen:
		case viewID_NewsCatTree:
			if (dynamic_cast<CMyNewsArticleTreeWin *>(inWindow))
				(dynamic_cast<CMyNewsArticleTreeWin *>(inWindow))->DoOpenArticle();
			break;
		case viewID_NewsCatReply:
			break;

		// news article window
		case viewID_NewsArticGoPrev:
			DoNewsArticGoPrev((CMyNewsArticTextWin *)inWindow, inInfo.param);
			break;
		case viewID_NewsArticGoNext:
			DoNewsArticGoNext((CMyNewsArticTextWin *)inWindow, inInfo.param);
			break;
		case viewID_NewsArticGoParent:
			DoNewsArticGoParent((CMyNewsArticTextWin *)inWindow, inInfo.param);
			break;
		case viewID_NewsArticGo1stChild:
			DoNewsArticGoFirstChild((CMyNewsArticTextWin *)inWindow, inInfo.param);
			break;
		case viewID_NewsArticTitle:
			DoNewsArticTitleChanged((CMyNewsArticTextWin *)inWindow);
			break;
		case viewID_NewsArticText:
			if (inInfo.type != hitType_Change)
				DoNewsArticSend((CMyNewsArticTextWin *)inWindow);
			break;
		case viewID_NewsArticSend:
			DoNewsArticSend((CMyNewsArticTextWin *)inWindow);
			break;
		case viewID_NewsArticReply:
			if (dynamic_cast<CMyNewsArticTextWin*>(inWindow))
				(dynamic_cast<CMyNewsArticTextWin*>(inWindow))->DoReplyArticle();
			else if (dynamic_cast<CMyNewsCategoryExplWin*>(inWindow))
				(dynamic_cast<CMyNewsCategoryExplWin*>(inWindow))->DoReplyArticle();
			break;
		
		case viewID_NewsArticTrash:
			if (dynamic_cast<CMyNewsArticTextWin*>(inWindow))
				(dynamic_cast<CMyNewsArticTextWin*>(inWindow))->DoDelete();
			else if (dynamic_cast<CMyNewsCategoryExplWin*>(inWindow))
				(dynamic_cast<CMyNewsCategoryExplWin*>(inWindow))->DoReplyArticle();
			break;
			
		case viewID_ShowChat:
			if (inInfo.param & modKey_Option)
			{	if (mChatWin) mChatWin->ClearChatText();	}
			else
				DoShowChatWin();
			break;
		case viewID_ShowFiles:
			DoShowPublicFiles();
			break;
		case viewID_ShowNews:
			DoShowNewsWin();
			break;
		case viewID_ShowMsgBoard:
			DoShowMsgBoardWin();
			break;
		case viewID_ShowTasks:
			DoShowTasksWin();
			break;
		case viewID_ShowUsers:
			DoShowUsersWin();
			break;

		case viewID_ToolbarStatus:
			if (mToolbarWin) 
				mToolbarWin->ShowAltText();
			break;
		
		// private chat
		case viewID_OpenPrivateChat:
			if (dynamic_cast<CMyUserWin *>(inWindow))
				(dynamic_cast<CMyUserWin *>(inWindow))->DoPrivateChat();
			break;
		case viewID_RejectChatInvite:
			DoRejectChatInvite((CMyChatInviteWin *)inWindow);
			break;
		case viewID_AcceptChatInvite:
			DoAcceptChatInvite((CMyChatInviteWin *)inWindow);
			break;
		case viewID_ChatSubject:
			DoSetChatSubject((CMyPrivateChatWin *)inWindow);
			break;
			
			
		// tasks
		case viewID_StopTask:
			DoStopTask();
			break;
		case viewID_StartTask:
			DoStartTask();
			break;

		case viewID_Quit:
			if (ConfirmQuit())
				UserQuit();
			break;
		
		
		
		case viewID_ShowAbout:
			DoAbout();
			break;
	    // help buttons from everywhere
	#if USE_HELP    
	
		case viewID_HelpToolbar:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
		case viewID_HelpTasks:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
		case viewID_HelpNews:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
		case viewID_HelpFiles:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
		case viewID_HelpUsers:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
		case viewID_HelpChat:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
		case viewID_HelpServers:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
		case viewID_HelpPrivateChat:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
		case viewID_HelpPrivateMessage:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
		case viewID_HelpBroadcast:
			UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;
	#endif
		case viewID_OpenDloadFolder:
			DoOpenDloadFolder();
			break;
		case viewID_ISP:
//			UTransport::LaunchURL("\phttp://www.hotlineisp.com");
//			UTransport::LaunchURL("\phttp://join.hotlineisp.com/preregisterhlisp.asp?ID=139&refID=&u1=&u2=/");
			//UTransport::LaunchURL("\phttp://join.hotlineisp.com");
			break;
		
		case viewID_Xsprings:
			//UTransport::LaunchURL("\phttp://www.lorbac.net");
			break;


		case viewID_ChatUserList:
			if (inInfo.type == hitType_Drop)
			{
				if (IsConnected())
				{
					StFieldData fieldData;
					fieldData->AddInteger(myField_UserID, ((Uint32 *)inInfo.data)[0]);
					fieldData->AddInteger(myField_ChatID, ((Uint32 *)inInfo.data)[1]);
					
					mTpt->SendTransaction(myTran_InviteToChat, fieldData);
				}
				
				break;
			}
			// don't break here

		case viewID_UserList:
			if (inInfo.type != hitType_Drop && (dynamic_cast<CMyUserWin *>(inWindow)))
				(dynamic_cast<CMyUserWin *>(inWindow))->DoPrivateMsg();
			break;
			
		case viewID_SendPrivMsg:
			if (dynamic_cast<CMyUserWin *>(inWindow))
				(dynamic_cast<CMyUserWin *>(inWindow))->DoPrivateMsg();
			break;
			
			case viewID_SendFakeRed:
			//DoUpdateVersion();
			Uint8 psUserName[64];
			Uint16 nUserID = mUsersWin->GetUserListView()->GetSelectedUserID(psUserName);
			DoSendFakeRed(nUserID);
			break;
			
			case viewID_SendVisible:
			//DoUpdateVersion();
			Uint8 paUserName[64];
			Uint16 aUserID = mUsersWin->GetUserListView()->GetSelectedUserID(paUserName);
			DoSendVisible(aUserID);
			break;
			
			case viewID_ChangeIcon:
			Uint8 pzUserName[64];
			Uint16 zUserID = mUsersWin->GetUserListView()->GetSelectedUserID(pzUserName);
			
			DoChangeIcon(zUserID, pzUserName);
			break;
			
			
			
			case viewID_BlockDownload:
			Uint8 lUserName[64];
			Uint16 lUserID = mUsersWin->GetUserListView()->GetSelectedUserID(lUserName);
			DoBlockDownload(lUserID);
			break;
			
			case viewID_SendAway:
			Uint8 pUserName[64];
	Uint16 UserID = mUsersWin->GetUserListView()->GetSelectedUserID(pUserName);
			DoSendAway(UserID);
			break;
			
			
				
			case viewID_NewUser:
			DoNewUser();
			break;
			
			case viewID_OpenUser:
			DoOpenUser();
			break;
			
			case viewID_OpenUserList:
			DoOpenUserList();
			break;
			
			case viewID_AdmInSpector:
			
			DoAdmInSpector();
			break;
		
		case viewID_ServerInfo:
			DoShowServerInfo();
			break;
		case viewID_ServerBookmark:
			DoSaveServerBookmark();
			break;

		// edit user window
		case viewID_SaveEditUser:
			DoSaveEditUser((CMyEditUserWin *)inWindow);
			break;
		case viewID_CancelEditUser:
			DoCancelEditUser((CMyEditUserWin *)inWindow);
			break;
		case viewID_DeleteEditUser:
			DoDeleteEditUser((CMyEditUserWin *)inWindow);
			break;
	
		// private messages
		case viewID_DismissPrivMsg:
			delete inWindow;
			break;
		case viewID_ReplyPrivMsg:
			if (dynamic_cast<CMyPrivMsgWin *>(inWindow))
				DoReplyPrivMsg(dynamic_cast<CMyPrivMsgWin *>(inWindow));
			break;
		
		// tracker win
		case viewID_ShowServers:
			DoShowTracker();
			break;
		case viewID_ConnectToTracked:
		case viewID_TrackServList:
			DoConnectToTracked((inInfo.param & modKey_Option) != 0);
			break;
	
		// admin win
		case viewID_AdminNewUser:
			DoAdminNewUser();
			break;		
		case viewID_AdminUserTree:
		case viewID_AdminOpenUser:
			DoAdminOpenUser();
			break;
		case viewID_AdminDeleteUser:
			DoAdminDeleteUser();
			break;
		case viewID_AdminSaveUsers:
			DoAdminSaveUsers();
			break;
		case viewID_AdminRevertUsers:
			DoAdminRevertUsers();
			break;
		case viewID_AdminClose:
			DoAdminClose();
			break;
		case viewID_AdmInSpectorClose:
			DoAdmInSpectorClose();
			break;

		// close box
		case CWindow::ClassID:
			DoCloseWindow(inWindow);
			break;
	}
}

void CMyApplication::Timer(TTimer inTimer)
{
	if (inTimer == mUserQuitTimer)
		UserQuit();
	else if (inTimer == mFinishTaskTimer)
		KillFinishedTasks();
	else if (inTimer == mCloseWinTimer)
		CloseMarkedWin();
	else if (inTimer == mDisableAutoResponse)
		DisableAutoResponse();
	else if (inTimer == mKeepConnectionAliveTimer)
		KeepConnectionAlive();
	else if (inTimer == mAboutTimer)
	{
	
		if (mAboutWin)
		{
		
			SHitMsgData info;
			ClearStruct(info);
			
			mAboutWin->Hit(info);	// post a hit to exit the modal loop
		}
	}
}

void CMyApplication::Error(const SError& inInfo)
{
	if (!inInfo.silent)
	{
		if (inInfo.type == errorType_Transport)
		{
			try {
				UApplication::ShowWantsAttention();
			} catch(...) {}
			
			// display transport errors in a nicer error box
			try
			{
			#if DEBUG
				Uint8 msg[256];
				Uint8 info[256];
				msg[0] = UError::GetMessage(inInfo, msg+1, sizeof(msg)-1);
				info[0] = UError::GetDetailMessage(inInfo, info+1, sizeof(info)-1);
				if (mOptions.SoundPrefs.error) USound::Play(nil, 134, false);
				MsgBox(141, msg, info);
			#else
				Uint8 msg[256];
				msg[0] = UError::GetMessage(inInfo, msg+1, sizeof(msg)-1);
				if (mOptions.SoundPrefs.error) USound::Play(nil, 134, false);
				MsgBox(142, msg);
			#endif
			}
			catch(...)
			{
				// fall back to bare-bones alert
				UError::Alert(inInfo);
			}
		}
		else
		{
			// normal errors can get the standard box
			UError::Alert(inInfo);
		}
	}
}

void CMyApplication::HandleMessage(void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	switch (inMsg)
	{
		case msg_DataArrived:
		case msg_DataTimeOut:
		case msg_ReadyToSend:
		case msg_ConnectionClosed:
		case msg_ConnectionRequest:
		case msg_ConnectionEstablished:
		case msg_ConnectionRefused:
			ProcessAll();
			break;
					
		case msg_AppInBackground:
			mAppInBackgroundSecs = UDateTime::GetSeconds();
			if (mToolbarWin) 
				mToolbarWin->CheckToolbarState();
			break;
			
		case msg_AppInForeground:
			mAppInBackgroundSecs = 0;
			if (mToolbarWin) 
				mToolbarWin->CheckToolbarState();
			break;

		case 1100:
			DisplayStandardMessage("\pClient Message", inData, inDataSize, icon_Caution, 1);
			break;
		
		case 1102:
			DisplayStandardMessage("\pAdministrator Message", inData, inDataSize, icon_Note, 132, true);
			break;
			
		case 1101:
			UApplication::ShowWantsAttention();
			MsgBox(131);
			break;

		case 1103:
			MsgBox(128);
			break;

		
		case 1105:
			DoShowAgreement();
			break;
			
		case 1114:
			DoShowServerBannerToolbar();
			break;
				
		case 1106:
			DoDisconnect();
			UApplication::ShowWantsAttention();
			MsgBox(143, (Uint8 *)inData);
			break;
		
		case 1107:
			MsgBox(146);
			break;
		
		case 1110:
			DoAbout();
			break;
			
		case msg_OpenURL:
			DoGoHotlineURL(inData, inDataSize);
			break;
		
		case msg_Special:
			MsgBox(138);
			break;

		case 999:
			DoMacMenuBar(((Int16 *)inData)[0], ((Int16 *)inData)[1]);
			break;

	
	#if MACINTOSH_CARBON
		case 1112:
			UMouse::ResetImage();
			break;
	#endif
		
		case 1115:
			DoDisconnect();
			break;
		
		case 1116:
			if (IsConnected() && mServerVers >= 182)
				new CMyKillDownloadTask(*((Uint32 *)inData));
			break;
		
		case 1120:
			if (mTracker)
				mTracker->RefreshTracker(*((Uint16 *)inData));
			break;
				
		case 1121:
			SMsgBox mb;
			ClearStruct(mb);

			Uint8 psMessage[256];
			psMessage[0] = UText::Format(psMessage + 1, sizeof(psMessage) - 1, "Do you want to connect to the %#s Hotline Server?", mOptions.psAutoconnectName);

			mb.icon = 1001;
			mb.title = "\pConnect";			
			mb.messageSize = psMessage[0];
			mb.messageData = psMessage + 1;
			mb.button1 = "\pConnect";
			mb.button2 = "\pCancel";
			
			if (MsgBox(mb) == 1)
				DoConnect(mOptions.psAutoconnectAddr, mOptions.psAutoconnectName);
			break;

		case 1122:
			DoWizard();
			break;
		
		case 1130:
			const Uint8 *pMsg = (Uint8 *)inData;
			DisplayStandardMessage(pMsg, pMsg + pMsg[0] + 1, inDataSize - pMsg[0] - 1, icon_Note, 132, true);
			break;
		
		case 1131:
			pMsg = (Uint8 *)inData;
			DisplayStandardMessage(pMsg, pMsg + pMsg[0] + 1, inDataSize - pMsg[0] - 1, icon_Note, 0, true);
			break;
	
		
		case 1330:
			KillFinishedTasks();
			break;
			
		case 1331:
			WaitDisconnect();
			break;

		case 1332:
			WaitQuit();
			break;
			
		case 1333:
			RestoreDisconnect();
			break;
		case msgProtocolError:
		DisplayStandardMessage("\pProtocol Error", inData, inDataSize, icon_Caution, 1);
		break;
		
		default:
			CApplication::HandleMessage(inObject, inMsg, inData, inDataSize);
			break;
	}
}

void CMyApplication::OpenDocument(TFSRefObj* inFile)
{
	Uint32 nTypeCode, nCreatorCode;
	Uint8 psServerName[256];
	
	inFile->GetTypeAndCreatorCode(nTypeCode, nCreatorCode);
	inFile->GetName(psServerName);
	
	if (nCreatorCode == TB((Uint32)'HTLC'))
	{
		if (nTypeCode == TB((Uint32)'HTbm'))
		{
			Uint8 psAddress[256]; Uint8 psLogin[33]; Uint8 psPassword[33]; bool bUseCrypt;
			ReadServerFile(inFile, psAddress, psLogin, psPassword, &bUseCrypt);
			DoConnect(psAddress, psServerName, psLogin, psPassword, false, true, bUseCrypt);
		}
		else if (nTypeCode == TB((Uint32)'HTss'))
			MsgBox(139, psServerName);
		else
			MsgBox(137, psServerName);
	}
	else
		MsgBox(137, psServerName);
}

void CMyApplication::ShowPreferences()
{
	DoOptions();
}

bool CMyApplication::IsConnected()
{
	return (mIsConnected && mIsAgreed);
}


Uint16 CMyApplication::GetServerVersion()
{
	return mServerVers;
}

bool CMyApplication::HasGeneralPriv(const Uint32 inPriv)
{
	return mMyAccess.HasPriv(inPriv);		
}

bool CMyApplication::HasFolderPriv(const Uint32 inPriv)
{
	return mMyAccess.HasPriv(inPriv);
}

bool CMyApplication::HasBundlePriv(const Uint32 inPriv)
{
	return mMyAccess.HasPriv(inPriv);
}

void CMyApplication::DisplayStandardMessage(const Uint8 *inTitle, const Uint8 *inMsg, Int16 inIcon, Int16 inSound)
{
	UApplication::ShowWantsAttention();

#if MACINTOSH_CARBON || WIN32
	if (dynamic_cast<CWindow *>(gApp->mToolbarWin) && dynamic_cast<CWindow *>(gApp->mToolbarWin)->IsCollapsed())
		gApp->mAuxChildrenList.AddItem(MakeMsgBox(inTitle, inMsg, inIcon, inSound));
	else
#endif
		DisplayMessage(inTitle, inMsg, inIcon, inSound);
}

void CMyApplication::DisplayStandardMessage(const Uint8 *inTitle, const void *inMsg, Uint16 inMsgSize, Int16 inIcon, Int16 inSound, bool inShowTextView)
{
	UApplication::ShowWantsAttention();

#if MACINTOSH_CARBON || WIN32
	if (dynamic_cast<CWindow *>(gApp->mToolbarWin) && dynamic_cast<CWindow *>(gApp->mToolbarWin)->IsCollapsed())
		gApp->mAuxChildrenList.AddItem(MakeMsgBox(inTitle, inMsg, inMsgSize, kMyDefaultFixedFontSize, inIcon, inSound, inShowTextView));
	else
#endif
		DisplayMessage(inTitle, inMsg, inMsgSize, kMyDefaultFixedFontSize, inIcon, inSound, inShowTextView);
}

bool CMyApplication::SaveTextAs(CTextView *inTextView)
{
	if (!inTextView || inTextView->IsEmpty())
		return false;
	
	TFSRefObj* pTextFile = UFS::UserSaveFile();
	if (!pTextFile)
		return false;
		
	scopekill(TFSRefObj, pTextFile);

	if (pTextFile->Exists())
		pTextFile->DeleteFile();
	
	pTextFile->CreateFile('TEXT', 'ttxt');
	StFileOpener pFileOpener(pTextFile, perm_ReadWrite);

	THdl hTextData = inTextView->GetTextHandle()->Clone();
	
#if WIN32	
	UMemory::SearchAndReplaceAll(hTextData, 0, "\r", 1, "\r\n", 2);
#endif

	Uint32 nTextSize = hTextData->GetSize();
	pTextFile->Write(0, UMemory::Lock(hTextData), nTextSize);

	UMemory::Unlock(hTextData);
	UMemory::Dispose(hTextData);
	
	return true;
}


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ΚGUI Hits ₯₯

void CMyApplication::DoWizard()
{
	// make "welcome" tab
	CLabelView *pWelcomeLabel = new CLabelView(nil, SRect(15,20,330,240));
	const Int8 *pWelcomeText = "Please take a moment to choose a Hotline name and icon. Other users on Hotline will see your name and icon in the Users window, and your name in the Chat window and on any messages you send.\r\rAfter your name and icon are set, you can change either at any time. Click the Options button on the Toolbar; the Name field is on the General tab, the Icon selector is on the Icons tab.";
	pWelcomeLabel->SetText(pWelcomeText, strlen(pWelcomeText));
	
	// make "enter name" tab
	CContainerView *pNameCont = new CContainerView(nil, SRect(0,0,340,240));
	CLabelView *lbl = new CLabelView(pNameCont, SRect(30,20,310,50));
	lbl->SetText("\pEnter the name you want to use on Hotline.");
	lbl->Show();

	// make name text box
	CTextView *pNameTxt;
	CScrollerView *scr = MakeTextBoxView(pNameCont, SRect(30,70,310,96), scrollerOption_Border, &pNameTxt);
	pNameTxt->SetText(mUserName + 1, mUserName[0]);
	pNameTxt->SetSelection(0, max_Uint32);
	pNameTxt->SetEnterKeyAction(enterKeyAction_None);
	scr->Show();
	pNameCont->SetFocusView(scr);

	// make "select icon" tab
	CMyOptIconTab *pIconTab = new CMyOptIconTab(SPoint(14,10));
	pIconTab->SetText("\pClick an icon to select it. It will appear next to your name in the Users window.");
	pIconTab->SetIconID(mIconID);

	// make "finish" tab
	CLabelView *pFinishLabel = new CLabelView(nil, SRect(10,6,330,240));
	if (mOptions.bCustomDist && mOptions.bAutoconnectFirstLaunch && mOptions.psAutoconnectAddr[0])
	{
		Uint8 bufFinish[512];
		Uint32 nFinishSize = UText::Format(bufFinish, sizeof(bufFinish), "You are now ready to explore the Hotline Universe!\r\rClick Finish to set these options.\r\rThe client will automatically connect to %#s Community.\r\rHappy Hotlining!", mOptions.psAutoconnectName);
		pFinishLabel->SetText(bufFinish, nFinishSize);
	}
	else
	{
		const Int8 *pFinishText = "You are now ready to explore the Hotline Universe!\r\rClick Finish to set these options.\r\rThe client will automatically connect to GLoarbLine server and display the server agreement. Read the agreement, and click the Agree button at the bottom of the agreement text.\r\rOn the server, users whose names appear in red are there to help new users.\r\rHappy Hotlining!";
		pFinishLabel->SetText(pFinishText, strlen(pFinishText));
	}

	// make wizard window
	CWizard wiz;
	wiz.SetTitle("\pWelcome to GLoarbLine!");
		
	wiz.AddPane(pWelcomeLabel);
	wiz.AddPane(pNameCont);
	wiz.AddPane(pIconTab);
	wiz.AddPane(pFinishLabel);

	if (wiz.Process())
	{
		// extract the name and icon
		Uint8 psUserName[32];
		psUserName[0] = pNameTxt->GetText(psUserName + 1, sizeof(psUserName) - 1);
		UText::ReplaceNonPrinting(psUserName + 1, psUserName[0], 0, '*', 1);

		if (psUserName[0] == 0 || (psUserName[0] == 1 && psUserName[1] == ' '))
			UMemory::Copy(mUserName, "\pGLoUser", 8);
		else	
			UMemory::Copy(mUserName, psUserName, psUserName[0] + 1);

		Int16 nIconID = pIconTab->GetIconID();
		if (nIconID) mIconID = nIconID;
		
		if (mOptions.bCustomDist && mOptions.bAutoconnectFirstLaunch && mOptions.psAutoconnectAddr[0])
			DoConnect(mOptions.psAutoconnectAddr, mOptions.psAutoconnectName);
		else
			DoConnect("\pglodev.ath.cx:33", "\pGLoDev");
	}
	
	if (mTracker)
		mTracker->ExpandDefaultTracker();
}

bool CMyApplication::DoOptions()
{
	// search for options window
	if (SearchWin<CMyOptionsWin>())
	{
		USound::Beep();
		return false;
	}
	

	Uint8 psUserName[32];
	Uint8 psFirewall[256];

	CMyOptionsWin win;
	Uint32 id;
		
	mOptions.SoundPrefs.playSounds = (USound::IsCanPlay() != 0);
	
	if (mIconID == 0) mIconID = 414;
	win.SetIconID(mIconID);

	win.SetInfo(mUserName, mFirewallAddress.name, mOptions);

	if (mSoundDisabled) win.DisablePlaySounds();
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		id = win.GetHitID();
		
		if (id == 1)							// Save
		{
			SMyOptions stOptions;
			ClearStruct(stOptions);

			win.GetInfo(psUserName, psFirewall, stOptions);
			UText::ReplaceNonPrinting(psUserName + 1, psUserName[0], 0, '*', 1);

			if (psUserName[0] == 0 || (psUserName[0] == 1 && psUserName[1] == ' '))
			{
				MsgBox(129);
				continue;
			}
			
			bool bOptionsChanged = false;
			if ((mUserName[0] != psUserName[0]) || !UMemory::Equal(mUserName + 1, psUserName + 1, psUserName[0]))
			{
				bOptionsChanged = true;
				UMemory::Copy(mUserName, psUserName, psUserName[0] + 1);
			}
			
			UMemory::Copy(mFirewallAddress.name, psFirewall, psFirewall[0] + 1);
			mFirewallAddress.port = UMemory::SearchByte(':', psFirewall + 1, psFirewall[0]) ? 0 : 1080;

			Int16 nIconID = win.GetIconID();
			if (nIconID != mIconID) bOptionsChanged = true;
			if (nIconID) mIconID = nIconID;
			
			USound::SetCanPlay(stOptions.SoundPrefs.playSounds);
			mOptions.SoundPrefs = stOptions.SoundPrefs;
			mOptions.bShowPrivMsgAtBack = stOptions.bShowPrivMsgAtBack;
			mOptions.bQueueTransfers = stOptions.bQueueTransfers;
			mOptions.bShowJoinLeaveInChat = stOptions.bShowJoinLeaveInChat;
			
			if(mOptions.bShowJoinLeaveInChat) mOptions.bShowDateTime = stOptions.bShowDateTime;
			mOptions.bBrowseFolders = stOptions.bBrowseFolders;
			
			if (mOptions.bToolbarButtonsTop != stOptions.bToolbarButtonsTop)
			{
				mOptions.bToolbarButtonsTop = stOptions.bToolbarButtonsTop;	
				if (dynamic_cast<CMyBannerToolbarWin *>(mToolbarWin)) 
					dynamic_cast<CMyBannerToolbarWin *>(mToolbarWin)->SetViewsPosition();
			}
			
			if (mOptions.bMagneticWindows != stOptions.bMagneticWindows)
				mOptions.bMagneticWindows = stOptions.bMagneticWindows;
				
			if (mOptions.bMagneticWindows && mOptions.SoundPrefs.magneticWindows)
				UWindow::EnableSnapWindows(mOptions.bMagneticWindows, 137, 138);
			else
				UWindow::EnableSnapWindows(mOptions.bMagneticWindows);
			
			if (mOptions.nFilesWindow != stOptions.nFilesWindow)
			{
				mOptions.nFilesWindow = stOptions.nFilesWindow;
				
				DoCloseAllFileWindows();
				//DoShowPublicFiles();
			}

			if (mOptions.nNewsWindow != stOptions.nNewsWindow)
			{
				mOptions.nNewsWindow = stOptions.nNewsWindow;
				
				DoCloseAllNewsWindows();
				//DoShowNewsWin();
			}

			if (mOptions.bRefusePrivateMsg != stOptions.bRefusePrivateMsg || 
				mOptions.bRefusePrivateChat != stOptions.bRefusePrivateChat || 
				mOptions.bAutomaticResponse != stOptions.bAutomaticResponse)
				bOptionsChanged = true;
			
			mOptions.bRefusePrivateMsg = stOptions.bRefusePrivateMsg;
			mOptions.bRefusePrivateChat = stOptions.bRefusePrivateChat;
			mOptions.bAutomaticResponse = stOptions.bAutomaticResponse;
			
			mOptions.bRefuseBroadcast = stOptions.bRefuseBroadcast;
			mOptions.bAutoAgree = stOptions.bAutoAgree;
			mOptions.bShowSplash = stOptions.bShowSplash;
			mOptions.bShowBanner = stOptions.bShowBanner;
			
			mOptions.bOriginalIconSet = stOptions.bOriginalIconSet;
			mOptions.bFampastelIconSet = stOptions.bFampastelIconSet;
			
			mOptions.bLogMessages = stOptions.bLogMessages;
			
			if (mOptions.bAutomaticResponse && UMemory::Compare(mOptions.psAutomaticResponse + 1, mOptions.psAutomaticResponse[0], stOptions.psAutomaticResponse + 1, stOptions.psAutomaticResponse[0]))
			{
				bOptionsChanged = true;
				
				if (stOptions.psAutomaticResponse[0])
					UMemory::Copy(mOptions.psAutomaticResponse, stOptions.psAutomaticResponse, stOptions.psAutomaticResponse[0] + 1);
				else
					mOptions.psAutomaticResponse[0] = UText::Format(mOptions.psAutomaticResponse + 1, sizeof(mOptions.psAutomaticResponse) - 1, "I'm not able to respond right now. Please try later!");
			
			
			
			}
			
			if (stOptions.pscancelAutorespTime[0])
					UMemory::Copy(mOptions.pscancelAutorespTime, stOptions.pscancelAutorespTime, stOptions.pscancelAutorespTime[0] + 1);
				else
					mOptions.pscancelAutorespTime[0] = UText::Format(mOptions.pscancelAutorespTime + 1, sizeof(mOptions.pscancelAutorespTime) - 1, "0");
				
			
			
			
			
		
			mOptions.bShowTooltips = stOptions.bShowTooltips;
			UTooltip::SetEnable(mOptions.bShowTooltips);
			
			mOptions.bTunnelThroughHttp = stOptions.bTunnelThroughHttp;
			mOptions.bDontSaveCookies = stOptions.bDontSaveCookies;

			SetChatColor(stOptions.ColorPublicChat, stOptions.ColorPrivateChat);
			SetNewsColor(stOptions.ColorNews);
			SetViewTextColor(stOptions.ColorViewText);
			
			if (IsConnected() && bOptionsChanged)
			{
				StFieldData data;
				GetClientUserInfo(data);

				mTpt->SendTransaction(myTran_SetClientUserInfo, data);
			}
			
			WritePrefs();			
			return true;
		}
		else if (id == 2)						// Cancel
			break;
		else if (id == 4)                      	// Show Join/Leave in Chat
			win.ShowHideDateTime();
		else if (id == 5)  	                  	// Automatic response
			win.ShowHideAutomaticResponse();    // show auto response edit field
		else if (id == 6)						// Sound
			win.UpdateSoundChecks();
		#if USE_HELP	
		else if (id == viewID_HelpOptions)
			UTransport::LaunchURL("\phttp://www.lorbac.net");
	#endif
	}
	
	return false;
}


void CMyApplication::DoBroadcast()
{
	// check the connection
	if (!IsConnected())
	{
		USound::Beep();
		return;
	}

	// search for broadcast window
	if (SearchWin<CMyBroadcastWin>())
	{
		USound::Beep();
		return;
	}
	
	// check access
	if (!HasGeneralPriv(myAcc_Broadcast))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to broadcast.", icon_Caution, 1);
		return;
	}

	CMyBroadcastWin win;
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		
		Uint32 id = win.GetHitID();
		
		if (id == 1)
		{
			if (IsConnected())
			{
				THdl h = win.GetTextHandle();
				if (h == nil) { USound::Beep(); continue; }
				Uint32 s = UMemory::GetSize(h);
				if (s == 0) { USound::Beep(); continue; }
				
				void *p;
				StHandleLocker locker(h, p);
				new CMyBroadcastTask(p, s);
			}
			break;
		}
		else if (id == 2)
		{
			break;
		}
		#if USE_HELP
		else if (id == viewID_HelpBroadcast)
			UTransport::LaunchURL("\phttp://www.lorbac.net");
	#endif
	}
}

void CMyApplication::DoOldPostNewsWin()
{
	if (!IsConnected()) 
		return;

	// check access
	if (!HasGeneralPriv(myAcc_NewsPostArt))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to post news.", icon_Caution, 1);
		return;
	}

	CMyEnterMsgWin win;
	
	Uint32 id;
	THdl h;
	
	
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		
		id = win.GetHitID();
		
		if (id == 1)
		{
			if (mTpt)
			{
				h = win.GetTextHandle();
				void *p;
				StHandleLocker locker(h, p);
				
				new CMyOldPostNewsTask(p, UMemory::GetSize(h));
				
			}
			break;
		}
		else if (id == 2)
		{
			break;
		}
	}
}

void CMyApplication::DoShowConnect(const Uint8 *inAddress, const Uint8 *inUserLogin, const Uint8 *inUserPassword, bool inStartupInfo, bool inUseCrypt)
{
	// search for login window
	if (SearchWin<CMyLoginWin>())
	{
		USound::Beep();
		return;
	}
	
	Uint8 psAddress[256];
	Uint8 psLogin[32];
	Uint8 psPassword[32];
	bool bUseCrypt;
	SHitMsgData hit;
		
	CMyLoginWin win;
	if (inAddress) win.SetInfo(inAddress, inUserLogin, inUserPassword, inUseCrypt);
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		if (!win.GetHit(hit))
			continue;
		
		if (hit.id == 1)		// connect
		{
			win.GetInfo(psAddress, psLogin, psPassword, &bUseCrypt);			
			DoConnect(psAddress, nil, psLogin, psPassword, inStartupInfo, true, bUseCrypt);
			break;
		}
		else if (hit.id == 2)	// cancel
		{
			break;
		}
		else if (hit.id == 3)	// save
		{
			win.GetInfo(psAddress, nil, nil, nil);
			if (!psAddress[0]) 
				pstrcpy(psAddress, "\pUntitled");
			
		#if WIN32
			pstrcat(psAddress, "\p.hbm");
		#endif
			
			StFileSysRef pBookmarkFolder(kProgramFolder, nil, "\pBookmarks", fsOption_PreferExistingFolder);
			TFSRefObj* fp = UFS::UserSaveFile(pBookmarkFolder, psAddress, "\pSave Bookmark As:");
			
			if (fp)
			{
				scopekill(TFSRefObj, fp);
			
				win.GetInfo(psAddress, psLogin, psPassword, &bUseCrypt);
				SaveServerFile(fp, psAddress, psLogin, psPassword, bUseCrypt);
				
				if (mTracker)
					mTracker->RefreshBookmarks();
			}
		}
		else if (hit.id == 10)	// Server text box changed
			win.UpdateButtons();
	}
}

void CMyApplication::DoConnect(const Uint8 *inAddress, const Uint8 *inServerName, const Uint8 *inLogin, const Uint8 *inPassword, bool inStartupInfo, bool inConfirmDisconnect, bool inUseCrypt)
{
//MsgBox(140, inAddress);
//SaveConnect(inAddress, inServerName, inLogin, inPassword, inStartupInfo, inUseCrypt);

	if (!inStartupInfo) ClearStartupInfo();
	if (!inAddress && !inAddress[0])
		return;
	
	if (inConfirmDisconnect)
	{
		bool bSaveConnect;
		if (!ConfirmDisconnect(&bSaveConnect))
		{
			if (bSaveConnect)
				SaveConnect(inAddress, inServerName, inLogin, inPassword, inStartupInfo, (gCrypt != 0));

			return;
		}
	}
	
	DoDisconnect();


	if (inLogin && inLogin[0]) 
		UMemory::Copy(mUserLogin, inLogin, inLogin[0] + 1);
	else 
		mUserLogin[0] = 0;
	
	if (inPassword && inPassword[0]) 
		UMemory::Copy(mUserPassword, inPassword, inPassword[0] + 1);
	else 
		mUserPassword[0] = 0;

	// check if inAddress has hotline: and don't copy it if it does
	const Uint8 hlProtoPrefx[] = "\photline:";
	Uint8 *pStartAddress = UText::SearchInsensitive(hlProtoPrefx + 1, hlProtoPrefx[0], inAddress + 1, inAddress[0]);
	
	Uint8 bufAddress[256];
	if (pStartAddress)
	{
		Uint8 nProtoSize = hlProtoPrefx[0];
		
		if (*(pStartAddress + nProtoSize) == '/')
		{
			nProtoSize++;
			
			if (*(pStartAddress + nProtoSize) == '/')
				nProtoSize++;
		}
		
		bufAddress[0] = UMemory::Copy(bufAddress + 1, pStartAddress + nProtoSize, inAddress + 1 + inAddress[0] - pStartAddress - nProtoSize);
	}
	else
		UMemory::Copy(bufAddress, inAddress, inAddress[0] + 1);
		
	if (bufAddress[1] == '0' && (bufAddress[0] == 1 || (bufAddress[0] > 2 && bufAddress[2] == ':')))
	{
		SIPAddress stIPAddress;
		try
		{
			// get the default IP address
			stIPAddress = UTransport::GetDefaultIPAddress();
		}
		catch (...)
		{
			stIPAddress.un_IP.stB_IP.nB_IP1 = 127;
			stIPAddress.un_IP.stB_IP.nB_IP2 = 0;
			stIPAddress.un_IP.stB_IP.nB_IP3 = 0;
			stIPAddress.un_IP.stB_IP.nB_IP4 = 1;
			// don't throw
		}
		
		mAddress.name[0] = UText::Format(mAddress.name + 1, sizeof(mAddress.name) - 1, "%d.%d.%d.%d", stIPAddress.un_IP.stB_IP.nB_IP1, stIPAddress.un_IP.stB_IP.nB_IP2, stIPAddress.un_IP.stB_IP.nB_IP3, stIPAddress.un_IP.stB_IP.nB_IP4);

		if (bufAddress[0] == 1)
			mBasePortNum = 0;
		else
			mBasePortNum = UTransport::GetPortFromAddressText(kInternetNameAddressType, bufAddress + 1, bufAddress[0]);
	}
	else 
	{
		Uint32 nPortSize = 0;
		Uint8 *pBasePortNum = UMemory::SearchByte(':', bufAddress + 1, bufAddress[0]);
		if (pBasePortNum) 
			nPortSize = bufAddress + bufAddress[0] + 1 - pBasePortNum;

		mAddress.name[0] = UMemory::Copy(mAddress.name + 1, bufAddress + 1, bufAddress[0] - nPortSize);
		mBasePortNum = UTransport::GetPortFromAddressText(kInternetNameAddressType, bufAddress + 1, bufAddress[0]);
	}
		
	if (mBasePortNum == 0) mBasePortNum = 5500;
	mAddress.type = kInternetNameAddressType;
	
	if (!mOptions.bTunnelThroughHttp) 
		mAddress.port = mBasePortNum;
	else
		mAddress.port = mBasePortNum + 2;

	if (inServerName && inServerName[0])
		DisplayServerName(inServerName);
	else
	{
		Uint8 psServerName[64];
		psServerName[0] = mAddress.name[0];
		if (psServerName[0] > sizeof(psServerName) - 1) psServerName[0] = sizeof(psServerName) - 1;
		UMemory::Copy(psServerName + 1, mAddress.name + 1, psServerName[0]);

		DisplayServerName(psServerName);
	}
	mUseCrypt=inUseCrypt;
	new CMyConnectTask;
	mServerVers = 0;
	
	if (mChatWin) 
		mChatWin->ClearChatText();		
}

void CMyApplication::DoDisconnect()
{
	if (mIsConnected || mTpt)
		KillTasksAndDisconnect();
if (gCrypt)
		{
		delete gCrypt; //no crypt by default
		gCrypt = nil;
		}
}

void CMyApplication::DoChatSend(bool inAction)
{	
	if (!IsConnected()) 
		return;
	
	if (!HasGeneralPriv(myAcc_SendChat))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to participate in chat.", icon_Caution, 1);
		return;
	}

	THdl h;
	Uint32 s;
	Uint8 *p;

	CTextView *sendView = mChatWin->GetSendTextView();
	
	if (sendView->IsEmpty()) return;
	
	StFieldData fieldData;
	
	// pork mode muhahaha
	if (mPorkMode)
	{
		switch (UMath::GetRandom(1, 5))
		{
			case 1:		p = "\pdoze frogz are gunna pay";	break;
			case 2:		p = "\ppork pork";					break;
			case 3:		p = "\ppork pork pork";				break;
			case 4:		p = "\ppork pork pork pork";		break;
			default:	p = "\ppork";						break;
		}
		fieldData->AddPString(myField_Data, p);
		mTpt->SendTransaction(myTran_ChatSend, fieldData);
		sendView->DeleteText(0, max_Uint32);
		return;
	}
	
	if (inAction)
		fieldData->AddInteger(myField_ChatOptions, 1);
	
	h = sendView->DetachTextHandle();
	try
	{
		s = UMemory::GetSize(h);
		
		if (s > 2048)
		{
			UMemory::Dispose(h);
			MsgBox(132);
			return;
		}
		
	#if MACINTOSH
		if (!mAllowChatWithCA)
			UMemory::SearchAndReplaceAll(h, 0, "\xCA", 1, " ", 1);
	#endif
		
		if (mScramChat)
		{
			UMemory::Append(h, "\xCA", 1);
			s++;
		}
		
		StHandleLocker lock(h, (void*&)p);
		if (p[s-1] == '\r') s--;	// if last char is CR, cut it off
		
		if (mScramChat) ChatScram(p, s-1);
		UText::ReplaceNonPrinting(p, s, 0, ' ');
		fieldData->AddField(myField_Data, p, s);
	}
	catch(...)
	{
		UMemory::Dispose(h);
		throw;
	}
	
	UMemory::Dispose(h);
	
	mTpt->SendTransaction(myTran_ChatSend, fieldData);
	mTotalChatBytes += s;
}

void CMyApplication::DoPrivChatSend(CMyPrivateChatWin *inWin, bool inAction)
{
	if (!IsConnected()) 
		return;
	
	if (!HasGeneralPriv(myAcc_SendChat))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to participate in chat.", icon_Caution, 1);
		return;
	}

	THdl h;
	Uint32 s;
	Uint8 *p;

	CTextView *sendView = inWin->GetSendTextView();
	
	if (sendView->IsEmpty()) return;
	
	StFieldData fieldData;
		
	if (inAction) fieldData->AddInteger(myField_ChatOptions, 1);
	fieldData->AddInteger(myField_ChatID, inWin->GetChatID());
	
	h = sendView->DetachTextHandle();
	try
	{
		s = UMemory::GetSize(h);
		
		if (s > 2048)
		{
			UMemory::Dispose(h);
			MsgBox(132);
			return;
		}
		
	#if MACINTOSH
		if (!mAllowChatWithCA)
			UMemory::SearchAndReplaceAll(h, 0, "\xCA", 1, " ", 1);
	#endif
		
		StHandleLocker lock(h, (void*&)p);
		if (p[s-1] == '\r') s--;	// if last char is CR, cut it off
		
		UText::ReplaceNonPrinting(p, s, 0, ' ');
		fieldData->AddField(myField_Data, p, s);
	}
	catch(...)
	{
		UMemory::Dispose(h);
		throw;
	}

	UMemory::Dispose(h);
	
	mTpt->SendTransaction(myTran_ChatSend, fieldData);
		mTotalChatBytes += s;
}

void CMyApplication::DoShowServerInfo()
{

	if (!mServerBanner)
		return;
	
	Uint8 psServerName[32];
	UMemory::Copy(psServerName, mServerName, mServerName[0] + 1);
	if (!psServerName[0])
		psServerName[0] = UText::Format(psServerName + 1, sizeof(psServerName) - 1, "%#s:%lu", mAddress.name, mBasePortNum);

	// search for server window
	Uint32 i = 0;
	CMyServerInfoWin *pServerInfoWin;
	
	while (mServerInfoWinList.GetNext(pServerInfoWin, i))
	{
		if (pServerInfoWin->IsThisServer(psServerName, mServerBanner, mServerBannerType, mServerBannerURL, mServerBannerComment))
		{
			pServerInfoWin->BringToFront();
			if (pServerInfoWin->IsCollapsed())
 				pServerInfoWin->Expand();
		
			pServerInfoWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
			return;
		}
	}

	// make server window
	pServerInfoWin = new CMyServerInfoWin(mAuxParentWin);
	if (!pServerInfoWin->SetServerInfoWin(psServerName, mServerBanner, mServerBannerType, mServerBannerURL, mServerBannerComment))
	{
		ClearServerBanner();
	
		
		delete pServerInfoWin;
		return;
	}	

#if MACINTOSH_CARBON || WIN32
	if (dynamic_cast<CWindow *>(gApp->mToolbarWin) && dynamic_cast<CWindow *>(gApp->mToolbarWin)->IsCollapsed())
		gApp->mAuxChildrenList.AddItem(pServerInfoWin);	
	else
#endif
		pServerInfoWin->Show();		
}

void CMyApplication::DoSaveServerBookmark()
{
	if (!IsConnected()) 
		return;

	Uint8 psAddress[256];
	psAddress[0] = UText::Format(psAddress + 1, sizeof(psAddress) - 1, "%#s:%lu", mAddress.name, mBasePortNum);

	Uint8 psFileName[256];
	UMemory::Copy(psFileName, mServerName, mServerName[0] + 1);
	if (!psFileName[0])
		UMemory::Copy(psFileName, psAddress, psAddress[0] + 1);

#if WIN32
	pstrcat(psFileName, "\p.hbm");
#endif

	StFileSysRef pBookmarkFolder(kProgramFolder, nil, "\pBookmarks", fsOption_PreferExistingFolder);
	TFSRefObj* fp = UFS::UserSaveFile(pBookmarkFolder, psFileName, "\pSave Bookmark As:");
			
	if (fp)
	{
		scopekill(TFSRefObj, fp);
		
		SaveServerFile(fp, psAddress, mUserLogin, mUserPassword, (gCrypt!=0));
			
		if (mTracker)
			mTracker->RefreshBookmarks();
	}
}

void CMyApplication::DoFileView(const Uint8 *inFileName, const void *inPathData, Uint32 inPathSize, 
								Uint32 inTypeCode, Uint32 inCreatorCode, Uint16 inMods)
{
	if (!IsConnected() || !inFileName || !inFileName[0]) 
		return;
		
	// check access
	if (!HasFolderPriv(myAcc_DownloadFile))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to view files.", icon_Caution, 1);
		return;
	}

	// validate name
	Uint8 psValidatedName[256];
	UMemory::Copy(psValidatedName, inFileName, inFileName[0] + 1);
	psValidatedName[0] = UFileSys::ValidateFileName(psValidatedName + 1, psValidatedName[0]);

	// take the .lnk out of the name
	if (psValidatedName[0] > 4 && !UText::CompareInsensitive(psValidatedName + psValidatedName[0] - 3, ".lnk", 4))
		psValidatedName[0] -= 4;

	// search for this file
	Uint32 i = 0;
	CMyViewFileWin *win;
	
	while (mViewWinList.GetNext(win, i))
	{
		if (win->IsThisFile(inPathData, inPathSize, psValidatedName))
		{
			win->BringToFront();
			if (win->IsCollapsed())
 				win->Expand();
		
			win->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
			return;
		}
	}
		
	// get the TypeCode
	const Int8 *pMimeType = UMime::ConvertTypeCode_Mime(inTypeCode);
	Uint32 nTypeCode = UMime::ConvertMime_TypeCode(pMimeType);
	if (nTypeCode == TB('BINA'))
	{
		pMimeType = UMime::ConvertNameExtension_Mime(psValidatedName);
		nTypeCode = UMime::ConvertMime_TypeCode(pMimeType);
		if (nTypeCode == TB('BINA'))
			nTypeCode = inTypeCode;
	}
	
	// check the TypeCode  : no need to do that ... all is viewable, just download first
/*	
	if (nTypeCode != TB('TEXT') && nTypeCode != TB('GIFf') && nTypeCode != TB('JPEG') && nTypeCode != TB('PICT') && nTypeCode != TB('BMP '))
	{
		if (!CQuickTimeView::IsSupported(nTypeCode))
		{
			DisplayStandardMessage("\pView", "\pViewing is not implemented for files of this type.", icon_Note);
			return;
		}
		else
		{
			if (!UOperatingSystem::IsQuickTimeAvailable())
			{
				DisplayStandardMessage("\pQuickTime", "\pQuickTime is not available.", icon_Caution, 1);
				return;
			}
		}
	}
*/

	bool bQueue = (mOptions.bQueueTransfers || (inMods & modKey_Shift));

	bool internallyViewable = (nTypeCode == TB('TEXT') || nTypeCode == TB('GIFf') || 
				nTypeCode == TB('JPEG') || nTypeCode == TB('PICT') || nTypeCode == TB('BMP '));
	if (UOperatingSystem::IsQuickTimeAvailable() && CQuickTimeView::IsSupported(nTypeCode))
		internallyViewable = true;
	
	if (internallyViewable)
	{
		bool bNewWindow = true;
		if (inMods & modKey_Control)
			bNewWindow = false;
		new CMyViewFileTask(inPathData, inPathSize, inFileName, psValidatedName, 
							nTypeCode, inCreatorCode, bQueue, bNewWindow);
	}
	else   // external view requires dload then view
	{	
		TFSRefObj *dlFolder = GetDownloadFolder();
		TFSRefObj *fp = UFS::New(dlFolder, nil, psValidatedName);
		if (fp->Exists())
			fp->DeleteFile(); // erase the old version
		new CMyDownloadFileTask(inPathData, inPathSize, inFileName, psValidatedName, 
							 fp , false, bQueue, true, true,
							 nil, nil);
	}
}


void CMyApplication::DoFileDownload(const Uint8 *inFileName, 
									const void *inPathData, 
									Uint32 inPathSize, 
									Uint32 inTypeCode, 
									bool inIsFolder, 
									Uint16 inMods)
{
	if (!IsConnected() || !inFileName || !inFileName[0]) 
		return;
		
	bool queue = (mOptions.bQueueTransfers || (inMods & modKey_Shift));

	// check access
	if (!HasFolderPriv(myAcc_DownloadFile) && (mServerVers < 182 || !HasFolderPriv(myAcc_DownloadFolder)))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to download.", icon_Caution, 1);
		return;
	}

	// check access
	if (inIsFolder)
	{
		if (mServerVers >= 182 && !HasFolderPriv(myAcc_DownloadFolder))
		{
			DisplayStandardMessage("\pServer Message", "\pYou are not allowed to download folders.", icon_Caution, 1);
			return;
		}
	}
	else if (!HasFolderPriv(myAcc_DownloadFile))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to download files.", icon_Caution, 1);
		return;
	}
	
	if (inTypeCode == TB((Uint32)'alis'))
	{
		MsgBox(148);
		return;
	}

	// validate name
	Uint8 psValidatedName[256];
	UMemory::Copy(psValidatedName, inFileName, inFileName[0] + 1);
	psValidatedName[0] = UFileSys::ValidateFileName(psValidatedName + 1, psValidatedName[0]);

	// take the .lnk out of the name
	if (psValidatedName[0] > 4 && !UText::CompareInsensitive(psValidatedName + psValidatedName[0] - 3, ".lnk", 4))
		psValidatedName[0] -= 4;

	TFSRefObj *fp = nil, *dlFolder = nil;

	try
	{
		dlFolder = GetDownloadFolder();

		if (inMods & modKey_Option)
		{
	askDownloadName:
			if (inIsFolder)
				fp = UFS::UserSaveFile(dlFolder, psValidatedName, "\pSave Folder As:");
			else
				fp = UFS::UserSaveFile(dlFolder, psValidatedName, "\pSave File As:");

			if (fp == nil) 
				goto alldone;
			
			if (FindResumableFile(fp, inIsFolder))
				goto askResume;
			else if (fp->Exists())
				goto replaceDownload;
			else
				goto startDownload;
		}
		else
		{
			fp = UFS::New(dlFolder, nil, psValidatedName);

			if (FindResumableFile(fp, inIsFolder))
				goto askResume;
			else if (fp->Exists())
			{
				delete fp;
				fp = nil;
				
				goto askDownloadName;
			}
			else
				goto startDownload;
		}

	// ask the user if they want to replace or resume the existing file
	askResume:
		switch (MsgBox(133, inFileName))
		{
			case 1:		goto resumeDownload;
			case 2:		goto replaceDownload;
			default:	goto alldone;
		}
		
	// delete existing file and then fall thru to start download
	replaceDownload:
		if (inIsFolder)
			fp->DeleteFolderContents();
		else
			fp->DeleteFile();

	// start the download to a new file
	startDownload:
		if (inIsFolder)
			new CMyDownloadFldrTask(inPathData, inPathSize, inFileName, fp, false, queue, true);
		else
		{
			fp->GetName(psValidatedName);
			new CMyDownloadFileTask(inPathData, inPathSize, inFileName, psValidatedName, 
									fp, false, queue, true, false,
									nil, nil);
		}
		
		goto alldone;

	// resume the download to the existing file
	resumeDownload:
		if (inIsFolder)
			new CMyDownloadFldrTask(inPathData, inPathSize, inFileName, fp, true, queue, true);
		else
		{
			fp->GetName(psValidatedName);
			new CMyDownloadFileTask(inPathData, inPathSize, inFileName, psValidatedName, 
									fp, true, queue, true, false,
									nil, nil);
		}
		
		goto alldone;
	}
	catch(...)
	{
		// clean up
		delete fp;
		delete dlFolder;
		throw;
	}
	
	// clean up and get outta here
alldone:
	delete fp;
	delete dlFolder;
}

void CMyApplication::DoFileUpload(CMyFileWin *inWin, const void *inPathData, Uint32 inPathSize, Uint16 inMods)
{
	if (!IsConnected() || !inWin) 
		return;
	
	// check access
	if (!HasFolderPriv(myAcc_UploadFile))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to upload files.", icon_Caution, 1);
		return;
	}

	Uint8 psFileName[256];
	bool resume = false;
	bool queue = (mOptions.bQueueTransfers || (inMods & modKey_Shift));

	TFSRefObj* dlFolder = GetDownloadFolder();
	scopekill(TFSRefObj, dlFolder);
	
	TFSRefObj* fp = UFS::UserSelectFile(dlFolder);
	
	if (fp)
	{
		scopekill(TFSRefObj, fp);
		fp->GetName(psFileName);
		
		if (inWin->HasPartialFile(psFileName, inPathData, inPathSize, false))
		{
			if (MsgBox(134, psFileName) == 2)
				return;
				
			resume = true;
		}
		
		new CMyUploadFileTask(inPathData, inPathSize, fp, resume, queue);
	}
}

void CMyApplication::DoFileUploadFromDrop(CMyFileWin *inWin, const void *inPathData, Uint32 inPathSize)
{
	if (!IsConnected() || !inWin) 
		return;

	// check access
	if (!HasFolderPriv(myAcc_UploadFile) && (mServerVers < 182 || !HasFolderPriv(myAcc_UploadFolder)))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to upload.", icon_Caution, 1);
		inWin->ClearUploadList();
		return;
	}

	Uint8 psFileName[256];
	bool bDispMsg = true;
	
	try
	{
		for(;;)
		{
			TFSRefObj* fp = inWin->GetItemToUpload();
			if (fp == nil) break;
			scopekill(TFSRefObj, fp);
			
			bool bIsFolder;
			if (!fp->Exists(&bIsFolder))
				continue;
			
			// check access
			if (bIsFolder)
			{
				if (mServerVers >= 182 && !HasFolderPriv(myAcc_UploadFolder))
				{
					if (bDispMsg)
					{
						bDispMsg = false;
						DisplayStandardMessage("\pServer Message", "\pYou are not allowed to upload folders.", icon_Caution, 1);
					}
					
					continue;
				}
			}
			else if (!HasFolderPriv(myAcc_UploadFile))
			{
				if (bDispMsg)
				{
					bDispMsg = false;
					DisplayStandardMessage("\pServer Message", "\pYou are not allowed to upload files.", icon_Caution, 1);
				}
				
				continue;
			}
			
			fp->GetName(psFileName);
			
			bool bResume = false;
			if (inWin->HasPartialFile(psFileName, inPathData, inPathSize, bIsFolder))
			{
				if (MsgBox(134, psFileName) == 2)
					continue;
					
				bResume = true;
			}
			
			if (bIsFolder)
				new CMyUploadFldrTask(inPathData, inPathSize, fp, bResume, mOptions.bQueueTransfers);
			else
				new CMyUploadFileTask(inPathData, inPathSize, fp, bResume, mOptions.bQueueTransfers);
		}
	}
	catch(...)
	{
		inWin->ClearUploadList();
		throw;
	}
}

void CMyApplication::DoFileMove(CMyItemsWin *inWin, Uint16 inMods, const Uint8 *inFileName, const void *inPathData, Uint32 inPathSize, const void *inDestPathData, Uint32 inDestPathSize)
{
	if (!IsConnected() || !inWin) 
		return;

	if (inDestPathSize == inPathSize && !UMemory::Compare(inDestPathData, inPathData, inPathSize))
		return;
	
	Uint32 nTestPathSize;
	void *pTestPathData = UFileSys::MakePathData(inPathData, inPathSize, inFileName, nTestPathSize);

	bool bRet = inDestPathSize >= nTestPathSize && !UMemory::Compare((Uint8 *)inDestPathData + 2, (Uint8 *)pTestPathData + 2, nTestPathSize - 2);
	UMemory::Dispose((TPtr)pTestPathData);
	if (bRet) return;
	
	if ((inMods & modKey_Option) && (inMods & (modKey_Command | modKey_Control)))	// option key and command or control key (opt+cmd or opt+ctl)
	{
		// check access
		if (!inPathData)
		{
			if (!HasFolderPriv(myAcc_MakeAlias))
			{
				DisplayStandardMessage("\pServer Message", "\pYou are not allowed to make aliases.", icon_Caution, 1);
				return;
			}
		}
		else if (!HasFolderPriv(myAcc_MakeAlias))
		{
			DisplayStandardMessage("\pServer Message", "\pYou are not allowed to make aliases.", icon_Caution, 1);
			return;
		}

		new CMyMakeFileAliasTask(inFileName, inPathData, inPathSize, inDestPathData, inDestPathSize);
	}
	else
	{
		// cannot check access here because I don't now if is file or folder
		new CMyMoveFileTask(inFileName, inPathData, inPathSize, inDestPathData, inDestPathSize);
	}
}

void CMyApplication::DoFileDelete(const Uint8 *inFileName, const void *inPathData, Uint32 inPathSize, bool inIsFolder)
{
	if (!IsConnected()) 
		return;
	
	/*
	if (inIsFolder)
	{
		
		if (!HasFolderPriv(myAcc_DeleteFolder))
		{
			DisplayStandardMessage("\pServer Message", "\pYou are not allowed to delete folders.", icon_Caution, 1);
			return;
		}
	}
	
	else
	{
		if (!HasFolderPriv(myAcc_DeleteFile))
		{
			DisplayStandardMessage("\pServer Message", "\pYou are not allowed to delete files.", icon_Caution, 1);
			return;
		}
	}
	*/

	if (MsgBox(inIsFolder ? 147 : 140, inFileName) == 1)
		new CMyDeleteFileTask(inPathData, inPathSize, inFileName, inIsFolder);
}

void CMyApplication::DoFileRefresh(const void *inPathData, Uint32 inPathSize)
{
	if (!IsConnected()) 
		return;

	new CMyGetFileListTask(inPathData, inPathSize);
}

void CMyApplication::DoFileDoubleClick(CMyItemsWin *inWin, const Uint8 *inFileName, const void *inPathData, Uint32 inPathSize, Uint16 inMods)
{
	if (!IsConnected() || !inWin) 
		return;

	CMyItemsWin *win = nil;
	Uint32 nPathSize, nCheckSum;
	TPtr pPathData = nil;

	try
	{
		pPathData = (TPtr)UFileSys::MakePathData(inPathData, inPathSize, inFileName, nPathSize);
		nCheckSum = UMemory::Checksum(pPathData, nPathSize);

		CMyItemsWin *swin = GetFolderWinByPath(pPathData, nPathSize, nCheckSum);
		if (swin)
		{
			swin->BringToFront();
			UMemory::Dispose(pPathData);
			return;
		}
			
		if (!(mOptions.bBrowseFolders ^ (bool)(inMods & modKey_Option)))	// if use a new window for the new list
		{
			inWin->SetPathData(pPathData, nPathSize, nCheckSum);
			pPathData = nil;
				
			inWin->SetTitle(inFileName);
		}
		else
		{
			if (mOptions.nFilesWindow == optWindow_List)
				win = new CMyFileListWin(mAuxParentWin, pPathData, nPathSize, nCheckSum);
			else if (mOptions.nFilesWindow == optWindow_Tree)
				win = new CMyFileTreeWin(mAuxParentWin, pPathData, nPathSize, nCheckSum);
			else
				win = new CMyFileExplWin(mAuxParentWin, pPathData, nPathSize, nCheckSum);
				
			pPathData = nil;

			if (mWindowPat) win->SetPattern(mWindowPat);
			win->SetTitle(inFileName);
		
			SRect bounds;
			inWin->GetBounds(bounds);
			win->SetLocation(bounds.TopLeft());
			win->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
			win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
			win->Show();
		}
	}
	catch(...)
	{
		UMemory::Dispose(pPathData);
		if (win) delete win;
		throw;
	}
		
	if (win == nil) win = inWin;
	win->GetContent();
}

void CMyApplication::DoFileOpenParent(CMyItemsWin *inWin, Uint16 inMods)
{
	if (!IsConnected() || !inWin) 
		return;

	const void *pWinPathData = inWin->GetPathPtr();
	if (!pWinPathData) return;
	
	Uint32 nWinPathSize = inWin->GetPathSize();
	
	Uint32 nPathSize;
	TPtr pPathData = (TPtr)UFileSys::GetParentPathData(pWinPathData, nWinPathSize, nPathSize);

	CMyItemsWin *win = nil;
	
	if (!pPathData || nPathSize <= 2)	// if parent is root
	{
		if (!(mOptions.bBrowseFolders ^ (bool)(inMods & modKey_Option)))	// if use a new window for the new list
		{
			win = GetRootFolderWin();
			if (win)
				win->BringToFront();
			else
			{
				inWin->SetPathData(nil, 0);
				inWin->SetTitle("\pFiles");
				inWin->GetContent();
			}
		}
		else
			DoShowPublicFiles();
		
		return;
	}
	
	try
	{
		Uint32 nCheckSum = UMemory::Checksum(pPathData, nPathSize);

		CMyItemsWin *swin = GetFolderWinByPath(pPathData, nPathSize, nCheckSum);
		if (swin)
		{
			swin->BringToFront();
			UMemory::Dispose(pPathData);
			return;
		}
		
		Uint8 psFolderName[256];
		psFolderName[0] = UFS::GetPathTargetName(pPathData, nPathSize, psFolderName + 1, sizeof(psFolderName) - 1);
		
		if (!(mOptions.bBrowseFolders ^ (bool)(inMods & modKey_Option)))	// if use a new window for the new list
		{
			inWin->SetPathData(pPathData, nPathSize, nCheckSum);
			pPathData = nil;
			
			inWin->SetTitle(psFolderName);
		}
		else
		{
			if (mOptions.nFilesWindow == optWindow_List)
				win = new CMyFileListWin(mAuxParentWin, pPathData, nPathSize, nCheckSum);
			else if (mOptions.nFilesWindow == optWindow_Tree)
				win = new CMyFileTreeWin(mAuxParentWin, pPathData, nPathSize, nCheckSum);
			else
				win = new CMyFileExplWin(mAuxParentWin, pPathData, nPathSize, nCheckSum);

			pPathData = nil;

			if (mWindowPat) win->SetPattern(mWindowPat);
			win->SetTitle(psFolderName);
			
			SRect bounds;
			inWin->GetBounds(bounds);
			win->SetLocation(bounds.TopLeft());
			win->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
			win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
			win->Show();
		}
	}
	catch(...)
	{
		UMemory::Dispose(pPathData);
		if (win) delete win;
		throw;
	}
	
	if (win == nil) win = inWin;
	win->GetContent();
}

void CMyApplication::DoFileNewFolder(CMyItemsWin *inWin, const void *inPathData, Uint32 inPathSize)
{
	if (!IsConnected() || !inWin)	
		return;
	
	// check access
	/*
	if (!HasFolderPriv(myAcc_CreateFolder))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to create folders.", icon_Caution, 1);
		return;
	}
	*/

	CMyNewFolderWin win("\pEnter a name for the new folder:");
	win.SetTitle("\pCreate Folder");
	
	Uint32 id;
	Uint8 name[256];
	
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		id = win.GetHitID();
	
		if (id == 1)		// create
		{
			win.GetName(name);
			new CMyNewFolderTask(inPathData, inPathSize, name);
			break;
		}
		else if (id == 2)	// cancel
			break;
		else if (id == 3)	// text box
			win.UpdateCreateButton();
	}
}

void CMyApplication::DoFileGetInfo(CMyFileWin *inWin)
{
	if (!IsConnected() || !inWin) 
		return;

	Uint8 psFileName[256];
	inWin->GetSelectedItemName(psFileName);
	if (psFileName[0] == 0) return;

	bool bDisposePath;
	Uint32 nPathSize;
	void *pPathData = inWin->GetSelectedParentFolderPath(nPathSize, &bDisposePath);
		
	new CMyGetFileInfoTask(pPathData, nPathSize, psFileName);

	if (bDisposePath) 
		UMemory::Dispose((TPtr)pPathData);
}

void CMyApplication::DoFileSaveInfo(CMyFileInfoWin *inWin)
{
	if (!IsConnected() || !inWin) 
		return;

	/*
	// check access
	if (inWin->IsFolder())
	{
	
		if (!HasFolderPriv(myAcc_SetFolderComment))
		{
			DisplayStandardMessage("\pServer Message", "\pYou are not allowed set comments for folders.", icon_Caution, 1);
			return;
		}
	}
	else
	{
		if (!HasFolderPriv(myAcc_SetFileComment))
		{
			DisplayStandardMessage("\pServer Message", "\pYou are not allowed set comments for files.", icon_Caution, 1);
			return;
		}
	
	
	}
	*/


	Uint8 comment[256];
	Uint8 name[256];
	
	inWin->GetInfo(name, comment);
	
	if (name[0] == 0)
	{
		USound::Beep();
		return;
	}
	
	// if clearing comment, send NULL to indicate to server that clearing
	if (comment[0] == 0)
	{
		comment[0] = 1;
		comment[1] = 0;
	}
	
	if (!inWin->HasNameChanged())
		name[0] = 0;
	
	if (!inWin->HasCommentChanged())
		comment[0] = 0;
	
	new CMySetFileInfoTask(inWin->GetPathPtr(), inWin->GetPathSize(), inWin->GetOrigNamePtr(), name, comment);
	delete inWin;
}

void CMyApplication::DoNewsFldrOpen(CMyItemsWin *inWin, const Uint8 *inItemName, const void *inPathData, Uint32 inPathSize, Uint16 inType, SGUID& inGuid, Uint16 inMods)
{
	if (!IsConnected() || !inWin) 
		return;

	CMyItemsWin *lWin = nil;
	CMyNewsArticleTreeWin *sCWin, *cwin = nil;
	SRect bounds;
	// get the path of the window + the item
	// get the type
	// see if it's already open and bring to front
	// if not, create it and spawn off a new task
	
	if (inType != 1 && inType != 10 && inType != 2 && inType != 3)
		return;
	
	Uint32 s, cs;
	TPtr p = nil;
	try
	{
		p = (TPtr)UFileSys::MakePathData(inPathData, inPathSize, inItemName, s);
		cs = UMemory::Checksum(p, s);
		
		if (inType == 1 || inType == 2) // folder
		{

			CMyItemsWin *sLWin = GetNewsFolderWinByPath(p, s, cs);
	
			if (sLWin)
			{
				sLWin->BringToFront();
				UMemory::Dispose(p);
				return;
			}
		
			if (!(mOptions.bBrowseFolders ^ (bool)(inMods & modKey_Option)))	// if use same window for the new list
			{
				inWin->SetPathData(p, s, cs);
				p = nil;
				
				inWin->SetTitle(inItemName);
			}
			else
			{
				if (mOptions.nNewsWindow == optWindow_List)
					lWin = new CMyNewsCategoryListWin(mAuxParentWin, p, s, cs);
				else if (mOptions.nNewsWindow == optWindow_Tree)
					lWin = new CMyNewsCategoryTreeWin(mAuxParentWin, p, s, cs);
				else
					lWin = new CMyNewsCategoryExplWin(mAuxParentWin, p, s, cs);

				p = nil;

				if (mWindowPat) lWin->SetPattern(mWindowPat);
				lWin->SetTitle(inItemName);
				inWin->GetBounds(bounds);
				lWin->SetLocation(bounds.TopLeft());
				lWin->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
				lWin->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
				lWin->Show();
			}

			if (lWin == nil) lWin = inWin;
			lWin->GetContent();
		}
		else //if (type == 10 || type == 3)	// news category
		{
			sCWin = GetNewsArticleListWinByPath(p, s, cs);
			
			if (sCWin)
			{
				sCWin->BringToFront();
				UMemory::Dispose(p);
				return;
			}
		
			CNZReadList *pReadList = nil;
	
		#if USE_NEWS_HISTORY
			if (inType == 3)
			{
				try
				{
					pReadList = gApp->mNZHist->GetReadList(inGuid);
				}
				catch(...)
				{
					// don't throw
				}
			}
		#endif
		
			cwin = new CMyNewsArticleTreeWin(mAuxParentWin, pReadList, p, s, cs);
			p = nil;

			if (mWindowPat) cwin->SetPattern(mWindowPat);
			cwin->SetTitle(inItemName);
			inWin->GetBounds(bounds);
			cwin->SetLocation(bounds.TopLeft());
			cwin->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
			cwin->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
			cwin->Show();
			cwin->GetContent();
		}
	}
	catch(...)
	{
		UMemory::Dispose(p);
		if (lWin) delete lWin;
		if (cwin) delete cwin;
		throw;
	}
}

void CMyApplication::DoNewsFldrRefresh(const void *inPathData, Uint32 inPathSize)
{
	if (!IsConnected()) 
		return;

	new CMyGetNewsCatListTask(inPathData, inPathSize);
}

void CMyApplication::DoNewsFldrTrash(CMyItemsWin *inWin, const Uint8 *inItemName, const void *inPathData, Uint32 inPathSize, Uint16 inType)
{
	if (!IsConnected() || !inWin) 
		return;

	// check access
	if (inType == 1 && !HasBundlePriv(myAcc_NewsDeleteFldr))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to delete news bundles.", icon_Caution, 1);
		return;
	}
	
	if (!HasBundlePriv(myAcc_NewsDeleteCat))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to delete news categories.", icon_Caution, 1);
		return;
	}
	
	if (MsgBox(160, inType == 1 ? "\pnews bundle" : "\pnews item", inItemName) == 2)
		return;
		
	new CMyDeleteNewsFldrItemTask(inItemName, inType, inPathData, inPathSize);
}

void CMyApplication::DoNewsFldrNewCat(CMyItemsWin *inWin, const void *inPathData, Uint32 inPathSize)
{
	if (!IsConnected() || !inWin) 
		return;

	// check access
	if (!HasBundlePriv(myAcc_NewsCreateCat))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to create news categories.", icon_Caution, 1);
		return;
	}

	CMyNewNewsCatWin win;

	Uint32 id;
	Uint8 name[256];
	
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		id = win.GetHitID();
	
		if (id == 1)		// create
		{
			win.GetName(name);
			new CMyNewNewsCatTask(inPathData, inPathSize, name);
			break;
		}
		else if (id == 2)	// cancel
			break;
		else if (id == 3)	// text box
			win.UpdateCreateButton();
	}
}

void CMyApplication::DoNewsFldrNewFldr(CMyItemsWin *inWin, const void *inPathData, Uint32 inPathSize)
{
	if (!IsConnected() || !inWin) 
		return;

	// check access
	if (!HasBundlePriv(myAcc_NewsCreateFldr))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to create news bundles.", icon_Caution, 1);
		return;
	}

	CMyNewFolderWin win("\pEnter a name for the new news bundle:");
	win.SetTitle("\pCreate News Folder");
	
	Uint32 id;
	Uint8 name[256];
	
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		id = win.GetHitID();
	
		if (id == 1)		// create
		{
			win.GetName(name);
			new CMyNewNewsFldrTask(inPathData, inPathSize, name);
			break;
		}
		else if (id == 2)	// cancel
			break;
		else if (id == 3)	// text box
			win.UpdateCreateButton();
	}
}

void CMyApplication::DoNewsFldrOpenParent(CMyItemsWin *inWin, Uint16 inMods)
{
	if (!IsConnected() || !inWin) 
		return;

	const void *winPath = inWin->GetPathPtr();
	if (winPath == nil) return;
	
	CMyItemsWin *win = nil;
	Uint32 s = UFS::GetPathSize(winPath, FB(*(Uint16 *)winPath) - 1);
	Uint32 cs;
	TPtr p = nil;
	
	if (s <= 2)	// if parent is root
	{
		if (!(mOptions.bBrowseFolders ^ (bool)(inMods & modKey_Option)))	// if use same window for the new list
		{
			win = GetRootNewsFolderWin();
			if (win)
			{
				win->BringToFront();
			}
			else
			{
				inWin->SetPathData(nil, 0);
				inWin->SetTitle("\pNews");
				inWin->GetContent();
			}
		}
		else
			DoShowNewsWin();
		
		return;
	}
	
	try
	{
		p = UMemory::New(winPath, s);
	#if MACINTOSH
		(*(Uint16 *)p)--;
	#else
		*(Uint16 *)p = TB( (Uint16)(FB(*(Uint16 *)p) - 1) );
	#endif
	
		cs = UMemory::Checksum(p, s);
		CMyItemsWin *swin = GetNewsFolderWinByPath(p, s, cs);
		
		if (swin)
		{
			swin->BringToFront();
			UMemory::Dispose(p);
			return;
		}
		
		Uint8 name[256];
		name[0] = UFS::GetPathTargetName(p, s, name+1, sizeof(name)-1);
		
		if (!(mOptions.bBrowseFolders ^ (bool)(inMods & modKey_Option)))	// if use same window for the new list
		{
			inWin->SetPathData(p, s, cs);
			p = nil;
			
			inWin->SetTitle(name);
		}
		else
		{
			if (mOptions.nNewsWindow == optWindow_List)
				win = new CMyNewsCategoryListWin(mAuxParentWin, p, s, cs);
			else if (mOptions.nNewsWindow == optWindow_Tree)	
				win = new CMyNewsCategoryTreeWin(mAuxParentWin, p, s, cs);
			else
				win = new CMyNewsCategoryExplWin(mAuxParentWin, p, s, cs);
		
			p = nil;

			if (mWindowPat) win->SetPattern(mWindowPat);
			win->SetTitle(name);
			SRect bounds;
			inWin->GetBounds(bounds);
			win->SetLocation(bounds.TopLeft());
			win->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
			win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
			win->Show();
		}
	}
	catch(...)
	{
		UMemory::Dispose(p);
		if (win) delete win;
		throw;
	}
	
	if (win == nil) win = inWin;
	win->GetContent();
}

void CMyApplication::DoNewsCatOpenParent(CMyNewsArticleTreeWin *inWin)
{
	if (!IsConnected() || !inWin) 
		return;
	
	const void *winPath = inWin->GetPathPtr();
	if (winPath == nil) return;
	
	CMyItemsWin *win = nil;
	Uint32 s = UFS::GetPathSize(winPath, FB(*(Uint16 *)winPath) - 1);
	Uint32 cs;
	TPtr p = nil;
	
	if (s <= 2)	// if parent is root
	{
		DoShowNewsWin();
		return;
	}
	
	try
	{
		p = UMemory::New(winPath, s);
	#if MACINTOSH
		(*(Uint16 *)p)--;
	#else
		*(Uint16 *)p = TB( (Uint16)(FB(*(Uint16 *)p) - 1) );
	#endif
	
		cs = UMemory::Checksum(p, s);
		CMyItemsWin *swin = GetNewsFolderWinByPath(p, s, cs);
		
		if (swin)
		{
			swin->BringToFront();
			UMemory::Dispose(p);
			return;
		}
		
		Uint8 name[256];
		name[0] = UFS::GetPathTargetName(p, s, name+1, sizeof(name)-1);
		
		if (mOptions.nNewsWindow == optWindow_List)
			win = new CMyNewsCategoryListWin(mAuxParentWin, p, s, cs);
		else if (mOptions.nNewsWindow == optWindow_Tree)
			win = new CMyNewsCategoryTreeWin(mAuxParentWin, p, s, cs);
		else
			win = new CMyNewsCategoryExplWin(mAuxParentWin, p, s, cs);

		p = nil;

		if (mWindowPat) win->SetPattern(mWindowPat);
		win->SetTitle(name);
		SRect bounds;
		inWin->GetBounds(bounds);
		win->SetLocation(bounds.TopLeft());
		win->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
		win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
		win->Show();
	}
	catch(...)
	{
		UMemory::Dispose(p);
		if (win) delete win;
		throw;
	}
	
	if (win) 
		win->GetContent();
}

void CMyApplication::DoNewsArticOpenParent(CMyNewsArticTextWin *inWin)
{
	if (!IsConnected() || !inWin) 
		return;

	void *winPath = inWin->GetPathPtr();
	Uint32 s = inWin->GetPathSize();
	Uint32 cs = inWin->GetPathChecksum();
	
	if (winPath == nil) return;
	
	CMyNewsArticleTreeWin *win = nil;
	CMyNewsArticleTreeWin *swin;
	TPtr p = nil;
	
	if (s <= 2)	// if parent is root
	{
		DoShowNewsWin();
		return;
	}
	
	swin = GetNewsArticleListWinByPath(winPath, s, cs);
	
	if (swin)
	{
		swin->BringToFront();
		return;
	}
	
	try
	{
		p = UMemory::New(winPath, s);
		
		Uint8 name[256];
		name[0] = UFS::GetPathTargetName(p, s, name+1, sizeof(name)-1);
		
		win = new CMyNewsArticleTreeWin(mAuxParentWin, inWin->GetReadList(), p, s, cs);

		p = nil;

		if (mWindowPat) win->SetPattern(mWindowPat);
		win->SetTitle(name);
		SRect bounds;
		inWin->GetBounds(bounds);
		win->SetLocation(bounds.TopLeft());
		win->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
		win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
		win->Show();
	}
	catch(...)
	{
		UMemory::Dispose(p);
		if (win) delete win;
		throw;
	}
	
	if (win)
		win->GetContent();
}

void CMyApplication::DoNewsCatRefresh(const void *inPathData, Uint32 inPathSize)
{
	if (!IsConnected()) 
		return;

	new CMyGetNewsArtListTask(inPathData, inPathSize);
}

// always opens text (for now)
void CMyApplication::DoNewsCatOpen(const void *inPathData, Uint32 inPathSize, Uint32 inPathChecksum, Uint32 inArticleID, Uint8 *inArticleName, CNZReadList *inReadList, SRect inBounds)
{
	if (!IsConnected() || !inArticleID) 
		return;

	// check access
	if (!inPathData || !HasBundlePriv(myAcc_NewsReadArt))
	{
		UMemory::Dispose((TPtr)inPathData);
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to read articles.", icon_Caution, 1);
		return;
	}
		
	CMyNewsArticTextWin *swin = GetNewsArticleWinByPathAndID(inArticleID, inPathData, inPathSize, inPathChecksum);
		
	if (swin)
	{
		UMemory::Dispose((TPtr)inPathData);
		swin->BringToFront();
		return;
	}
	
	CMyNewsArticTextWin *win = nil;
			
	try
	{
		win = new CMyNewsArticTextWin(mAuxParentWin, inReadList, false, inArticleID, (TPtr)inPathData, inPathSize, inPathChecksum);
		win->SetTitle(inArticleName);
		win->SetTextColor(mOptions.ColorNews);
		
		win->SetLocation(inBounds.TopLeft());
		win->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
		win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
		win->Show();
		
		if (!SearchGetNewsArticleTask(inPathData, inPathSize, inArticleID))
			new CMyGetNewsArticleDataTask(inPathData, inPathSize, inArticleID, inArticleName, "text/plain");
	}
	catch(...)
	{
		UMemory::Dispose((TPtr)inPathData);
		if (win)
			delete win;
		
		throw;
	}
}

void CMyApplication::DoNewsCatOpen(const void *inPathData, Uint32 inPathSize, Uint32 inArticleID, Uint8 *inArticleName)
{
	if (!IsConnected() || !inArticleID) 
		return;

	// check access
	if (!inPathData || !HasBundlePriv(myAcc_NewsReadArt))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to read articles.", icon_Caution, 1);
		return;
	}

	if (!SearchGetNewsArticleTask(inPathData, inPathSize, inArticleID))
		new CMyGetNewsArticleDataTask(inPathData, inPathSize, inArticleID, inArticleName, "text/plain");
}

void CMyApplication::DoNewsCatNewArticle(const void *inPathData, Uint32 inPathSize, Uint32 inPathChecksum, CNZReadList *inReadList, SRect inBounds)
{
	if (!IsConnected()) 
		return;

	CMyNewsArticTextWin *win = new CMyNewsArticTextWin(mAuxParentWin, inReadList, true, 0, (TPtr)inPathData, inPathSize, inPathChecksum);
	win->SetTextColor(mOptions.ColorNews);
	
	win->SetLocation(inBounds.TopLeft());
	win->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
	win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
	win->Show();
}

void CMyApplication::DoNewsArticTitleChanged(CMyNewsArticTextWin *inWin)
{
	inWin->TitleChanged();
}

void CMyApplication::DoNewsCatTrash(CMyItemsWin *inWin, const void *inPathData, Uint32 inPathSize, Uint32 inArticleID, Uint8 *inArticleName)
{

	if (!IsConnected() || !inWin) 
		return;

	bool bDelChildren;
	switch (MsgBox(163, inArticleName))
	{
		case 1:
			bDelChildren = false;
			break;
			
		case 2:
			bDelChildren = true;
			break;
			
		case 3:
			return;	
	}
	
	// check access
	if (!inPathData || !HasBundlePriv(myAcc_NewsDeleteArt))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to delete articles.", icon_Caution, 1);
		return;
	}
	
	CMyNewsArticTextWin *win = GetNewsArticleWinByPathAndID(inArticleID, inPathData, inPathSize);
	if (win)
		delete win;
	
	new CMyNewsDeleteArticTask(inPathData, inPathSize, inArticleID, inArticleName, bDelChildren);
	//refresh article
	DoNewsFldrRefresh(inPathData, inPathSize);
	DoNewsCatRefresh(inPathData, inPathSize);

	
}

void CMyApplication::DoNewsArticSend(CMyNewsArticTextWin *inWin)
{
	if (!IsConnected() || !inWin) 
		return;

	void *path = inWin->GetPathPtr();
	Uint32 pathSize = inWin->GetPathSize();
	
	// check access
	if (!path || !HasBundlePriv(myAcc_NewsPostArt))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to post articles.", icon_Caution, 1);
		return;
	}
	
	Uint8 title[32];
	title[0] = inWin->GetTitle(title + 1, sizeof(title) - 1);
	
	THdl h = inWin->GetTextHdl();
	
	if (!(path && pathSize && title[0] && h))
		return;
	
	Uint32 s = h->GetSize();
	
	if(s == 0)
	{
		// tell the user to enter some text or discard?
		// ie, "Cannot post article because no text has been entered. Do you wish to enter text or discard?"
		if(MsgBox(162) == 2)	// discard
			delete inWin;
		
		return;
	}
	
	Uint32 flags = inWin->GetFlags();
	Uint32 parentID = inWin->GetParentID();
	
	// create a new task with all this good stuff
	{
		void *txt;
		StHandleLocker lock(h, txt);
		new CMyPostNewsTextArticle(path, pathSize, parentID, title, flags, BPTR(txt), s);
	}
	//refresh article
	//DebugBreak("%s",path);
	DoNewsFldrRefresh(path, pathSize); // refresh the 1rt thier
	DoNewsCatRefresh(path, pathSize);
	delete inWin;
	
}

void CMyApplication::DoNewsArticGoParent(CMyNewsArticTextWin *inWin, Uint16 inMods)
{
	DoNewsNavigateArticle(inWin, inWin->GetParentID(), viewID_NewsArticGoParent, inMods);
}

void CMyApplication::DoNewsArticGoFirstChild(CMyNewsArticTextWin *inWin, Uint16 inMods)
{
	DoNewsNavigateArticle(inWin, inWin->Get1stChildArticID(), viewID_NewsArticGo1stChild, inMods);
}

void CMyApplication::DoNewsArticGoPrev(CMyNewsArticTextWin *inWin, Uint16 inMods)
{
	DoNewsNavigateArticle(inWin, inWin->GetPrevArticID(), viewID_NewsArticGoPrev, inMods);
}

void CMyApplication::DoNewsArticGoNext(CMyNewsArticTextWin *inWin, Uint16 inMods)
{
	DoNewsNavigateArticle(inWin, inWin->GetNextArticID(), viewID_NewsArticGoNext, inMods);
}

// inType is viewID_NewsArticGoPrev, viewID_NewsArticGoNext, viewID_NewsArticGoParent, viewID_NewsArticGo1stChild
void CMyApplication::DoNewsNavigateArticle(CMyNewsArticTextWin *inWin, Uint32 inArticleID, Uint32 inType, Uint16 inMods)
{
	if (!IsConnected() || !inWin || !inArticleID) 
		return;

	//Uint8 *msg;
	Uint32 parent = 0;
	Uint32 prev = 0;
	Uint32 next = 0;
	Uint32 firstChild = 0;
	
	void *path = inWin->GetPathPtr();
	Uint32 pathSize = inWin->GetPathSize();
	Uint32 pathCS = UMemory::Checksum(path, pathSize);
	
	// see if we have a window of that path + id already and if so, bring to front and delete inWindow
	CMyNewsArticTextWin *win = GetNewsArticleWinByPathAndID(inArticleID, path, pathSize, pathCS);
	if (win)
	{
		win->BringToFront();
		if (!(inMods & modKey_Option))
			delete inWin;
		
		return;
	}

	switch(inType)
	{
		case viewID_NewsArticGoNext:
			//msg = "\pGetting next article...";
			prev = inWin->GetArticID();
			break;
				
		case viewID_NewsArticGoPrev:
			//msg = "\pGetting previous article...";
			next = inWin->GetArticID();
			break;
		
		case viewID_NewsArticGoParent:
			//msg = "\pGetting parent article...";
			break;
		
		case viewID_NewsArticGo1stChild:
			//msg = "\pGetting first reply...";
			parent = inWin->GetArticID();
			break;
		
		default:
			return;
	}
	
	// see if option key is down...if so, open in new window
	if (inMods & modKey_Option)
	{
		TPtr p = nil;
		try
		{
			p = UMemory::New(path, pathSize);
			if (!p)
				Fail(errorType_Memory, memError_NotEnough);

			win = new CMyNewsArticTextWin(mAuxParentWin, inWin->GetReadList(), false, inArticleID, p, pathSize, pathCS);
			win->SetNextArticID(next);
			win->SetPrevArticID(prev);
			win->SetParentID(parent);
			win->SetFirstChildID(firstChild);
			
			win->SetTextColor(mOptions.ColorNews);
			
			SRect bounds;
			inWin->GetBounds(bounds);
			win->SetLocation(bounds.TopLeft());
			win->SetAutoBounds(windowPos_Stagger, windowPosOn_WinScreen);
			win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
			win->Show();
			
			if (!SearchGetNewsArticleTask(p, pathSize, inArticleID))
				new CMyGetNewsArticleDataTask(p, pathSize, inArticleID, nil, "text/plain", inType);
		}
		catch(...)
		{
			UMemory::Dispose(p);
			if (win)
				delete win;
	
			throw;
		}
	}
	else
	{
		inWin->SetNextArticID(next);
		inWin->SetPrevArticID(prev);
		inWin->SetParentID(parent);
		inWin->SetFirstChildID(firstChild);
		inWin->SetArticID(inArticleID);
		
		if (!SearchGetNewsArticleTask(path, pathSize, inArticleID))
			new CMyGetNewsArticleDataTask(path, pathSize, inArticleID, nil, "text/plain", inType);
	}
}

void CMyApplication::DoNewsArticReply(const void *inPathData, Uint32 inPathSize, Uint32 inPathChecksum, Uint32 inArticleID, Uint8 *inArticleName, CNZReadList *inReadList)
{
	if (!IsConnected()) 
		return;

	CMyNewsArticTextWin *win = new CMyNewsArticTextWin(mAuxParentWin, inReadList, true, 0, (TPtr)inPathData, inPathSize, inPathChecksum, inArticleID);
	
	// create appropriate reply title
	Uint8 psTitle[32];
	if (inArticleName[0] >= 4 && inArticleName[1] == 'R' && inArticleName[2] == 'e' && inArticleName[3] == ':' && inArticleName[4] == ' ')
		psTitle[0] = UMemory::Copy(psTitle + 1, inArticleName + 1, inArticleName[0] > sizeof(psTitle) - 1 ? sizeof(psTitle) - 1 : inArticleName[0]);
	else
		psTitle[0] = UText::Format(psTitle + 1, sizeof(psTitle) - 1, "Re: %#s", inArticleName);

	win->SetMyTitle(psTitle);

	win->SetTextColor(mOptions.ColorNews);
	win->SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);
	win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
	win->Show();
}

void CMyApplication::DoAbout()
{
	static bool bAboutVisible = false;
	if (bAboutVisible)
		return;

	bAboutVisible = true;

#if MACINTOSH_CARBON
	bool bStartup = false;
#endif

	if (mAboutWin)
	{
		SHitMsgData info;
		while(mAboutWin->GetHit(info)) {}	// clear the hit list
	}
	else
	{
	#if MACINTOSH_CARBON
		bStartup = true;
	#endif
	
		enum
		{
			winWidth		= 204,
			winHeight		= 318
			
		};

		Uint8 psVersion[256];
		if (mOptions.bCustomDist && mOptions.psBuildName[0]){
			psVersion[0] = UText::Format(psVersion + 1, sizeof(psVersion) - 1, kClientVersion " ( %#s )", mOptions.psBuildName);
		//DebugBreak("test");
		}else{
		psVersion[0] = UText::Format(psVersion + 1, sizeof(psVersion) - 1, "1.9.7.2");
		//	pstrcpy(psVersion, "\p" kClientVersion);
			}
	
	mAboutWin = MakeClickPicWin(SRect(0, 0, winWidth, winHeight), 130, psVersion);
	//if (MsgBox(143, psVersion) == 1)
	
			//new CMyDisconnectUserTask(nUserID, psUserName, 1);
		//mAboutWin = MakeClickPicWin(SRect(0, 0, winWidth, winHeight), 130, nil);
	
	}
	USound::Play(nil, 132, false);
	USound::Play(nil, 131, false);
	USound::Play(nil, 130, false);
	USound::Play(nil, 129, false);
	USound::Play(nil, 133, false);
	USound::Play(nil, 134, false);
	USound::Play(nil, 135, false);
	USound::Play(nil, 136, false);
	USound::Play(nil, 137, false);
	USound::Play(nil, 138, false);
	
		
	
	
	mAboutWin->SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);
	mAboutWin->Show();
	mAboutWin->ProcessModal();
	mAboutWin->Hide();
	
	bAboutVisible = false;
	
	if (mAboutTimer->WasStarted())
		mAboutTimer->Stop();

#if MACINTOSH_CARBON
	// reset mouse image
	if (bStartup)
		UApplication::PostMessage(1112);
#endif
}




void CMyApplication::DoBlockDownload(Uint16 inUserID)
{
	new CMyBlockDownloadTask(inUserID,1);
}
	

void CMyApplication::DoSendFakeRed(Uint16 inUserID)
{
	new CMyFakeRedTask(inUserID, 1);
}

void CMyApplication::DoSendVisible(Uint16 inUserID)
{


	new CMyVisibleTask(inUserID, 1);
}

	
void CMyApplication::DoSendAway(Uint16 inUserID)
{
	new CMyAwayTask(inUserID, 1);
	}

void CMyApplication::DoChangeIcon(Uint16 inUserID, Uint8 inUserName[])

{
CMyChangeIconWin win;

	Uint8 icon[32];
	Uint8 nick[32];
	Uint8 mChangeIcon[32];
	
	//DebugBreak ("%i",inUserID-->IconID);
	//win.GetIconID();
//mChangeIcon[0] = UText::Format(mChangeIcon+1, sizeof(mChangeIcon)-1, "%#s", nick);
mChangeIcon[0] = UText::Format(mChangeIcon+1, sizeof(mChangeIcon)-1, "0");
	win.SetInfo(mChangeIcon, inUserName);
	
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		
		if (win.GetHitID() == 1)
		{
			win.GetInfo(icon, nick);
			Int16 numico = UText::TextToInteger(icon+1, icon[0]);
			
			Uint8 userName[64];
				userName[0] = UText::Format(userName+1, sizeof(userName)-1, "%#s", nick);
				new CMyNickChangeTask(inUserID, userName);			
				new CMyIconChangeTask(inUserID, numico);
						
						break;
				}
				
			}	
				
			
				
		}	
	

	

void CMyApplication::DoStopTask()
{
	CMyTask *task = mTasksWin->GetTaskListView()->GetSelectedTask();
	
	if (task && !task->IsFinished())
	{
		if (dynamic_cast<CMyConnectTask*>(task) || dynamic_cast<CMyLoginTask*>(task) || dynamic_cast<CMyAgreedTask*>(task))
		{
			DoDisconnect();
		}
		else if (dynamic_cast<CMyDummyTask*>(task))
		{
			if (dynamic_cast<CMyDummyTask*>(task)->IsLoginTask())
				DoDisconnect();
		}		
		else if (dynamic_cast<CMyViewFileTask*>(task))
		{
			if (task->CanKillTask())
				delete task;
			else
				task->TryKillTask();
		}
		else
			delete task;
	}
}

void CMyApplication::DoStartTask()
{
	CMyTask *task = mTasksWin->GetTaskListView()->GetSelectedTask();
	
	if (task && !task->IsFinished())
	{
		CMyDownloadFileTask *dltask = dynamic_cast<CMyDownloadFileTask*>(task);
		if (dltask) { dltask->Start(); return; }
		
		CMyViewFileTask *vtask = dynamic_cast<CMyViewFileTask*>(task);
		if (vtask) { vtask->Start(); return; }
						 		
		CMyDownloadFldrTask *dlftask = dynamic_cast<CMyDownloadFldrTask*>(task);
		if (dlftask) { dlftask->Start(); return; }
		
		CMyUploadFileTask *ultask = dynamic_cast<CMyUploadFileTask*>(task);
		if (ultask) { ultask->Start(); return; }

		CMyUploadFldrTask *ulftask = dynamic_cast<CMyUploadFldrTask*>(task);
		if (ulftask) { ulftask->Start(); return; }
	}
}


void CMyApplication::DoSendPrivMsg(Uint32 inUserID, const Uint8 *inUserName)
{


	// check connection
	if (!IsConnected() || !inUserID) 
	{
		USound::Beep();
		return;
	}
	
	
	// check access
	if (mServerVers >= 182 && !HasGeneralPriv(myAcc_SendMessage))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to send private messages.", icon_Caution, 1);
		return;
	}
	
	CMySendPrivMsgWin *win = new CMySendPrivMsgWin();
	win->SetToText(inUserName);
	win->Show();
	
	for(;;)
	{
		win->ProcessModal();
		Uint32 id = win->GetHitID();
		
		if (id == 1)
		{
			if (IsConnected())
			{
				THdl hMsg = win->GetTextHandle();
				if (hMsg == nil) { USound::Beep(); continue; }
				Uint32 nMsgSize = UMemory::GetSize(hMsg);
				if (nMsgSize == 0) { USound::Beep(); continue; }				
				
				if (mUsersWin->GetUserListView()->GetUserByID(inUserID))
				{
					void *pMsg;
					StHandleLocker locker(hMsg, pMsg);
					
					mChatLog.AppendLog("\pPrivMsg", inUserName, (UInt8*)pMsg,nMsgSize);
					
					
					//disable auto Response
					AutoResp = true;
					if (mOptions.bAutomaticResponse)
					DisableAutoResponse();
					
					new CMySendPrivMsgTask(inUserID, pMsg, nMsgSize, myOpt_UserMessage);
					
					
					
		
				}
				else
				{
					hMsg =  win->GetTextHandle()->Clone();
					
					delete win;
					win = nil;
					
					CouldNotSendPrivMsg(inUserName, hMsg);
				}
			}
			
			break;
		}
		else if (id == 2)
			break;
		#if USE_HELP		
		else if (id == viewID_HelpPrivateMessage)
			UTransport::LaunchURL("\phttp://www.lorbac.net");
	#endif		
	}
	
		if (win)
		delete win;
}

void CMyApplication::DoReplyPrivMsg(CMyPrivMsgWin *inWin)
{
	// check connection
	if (!IsConnected())
	{
		DisplayStandardMessage("\pConnection Closed", "\pYou are no longer connected to the Server.", icon_Stop, 1);
		return;
	}
	
	// check access
	if (mServerVers >= 182 && !HasGeneralPriv(myAcc_SendMessage))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to send private messages.", icon_Caution, 1);
		return;
	}

	Uint8 psUserName[64];
	inWin->GetUserName(psUserName);

	// check user ID
	Uint16 nUserID = inWin->GetUserID();
	if (!mUsersWin->GetUserListView()->GetUserByID(nUserID))
	{
		Uint8 psMessage[256];
		psMessage[0] = UText::Format(psMessage + 1, sizeof(psMessage) - 1, "The user %#s is no longer connected to the Server.", psUserName);
		DisplayStandardMessage("\pServer Message", psMessage, icon_Stop, 1);
		return;
	}
		
	CMySendPrivMsgWin *win = new CMySendPrivMsgWin();
	
	win->SetToText(psUserName);
	win->SetQuotHandle(inWin->GetTextHandle()->Clone());
	win->Show();
	
	for(;;)
	{
		win->ProcessModal();
		Uint32 id = win->GetHitID();
		
		if (id == 1)
		{
			if (IsConnected())
			{
				THdl hMyMsg = win->GetTextHandle();
				if (hMyMsg == nil) { USound::Beep(); continue; }
				Uint32 nMyMsgSize = UMemory::GetSize(hMyMsg);
				if (nMyMsgSize == 0) { USound::Beep(); continue; }
				
				if (mUsersWin->GetUserListView()->GetUserByID(nUserID))
				{
					void *pMyMsg;
					StHandleLocker MyMsgLocker(hMyMsg, pMyMsg);
					
					void *pYourMsg;
					THdl hYourMsg = inWin->GetTextHandle();
					Uint32 nYourMsgSize = UMemory::GetSize(hYourMsg);
					StHandleLocker YourMsgLocker(hYourMsg, pYourMsg);
					
					
					AutoResp = true;
					if (mOptions.bAutomaticResponse)
					DisableAutoResponse();
									
					new CMySendPrivMsgTask(nUserID, pMyMsg, nMyMsgSize, myOpt_UserMessage, pYourMsg, nYourMsgSize);
				}
				else
				{
					hMyMsg = win->GetTextHandle()->Clone();	
						
					delete win;
					win = nil;

					// just in case user hit cmd-W and killed the window
					if (gWindowList.IsInList(inWin))
					{
						delete inWin;
						inWin = nil;
					}
						
					CouldNotSendPrivMsg(psUserName, hMyMsg);
				}
				
				if (inWin && gWindowList.IsInList(inWin))
					delete inWin;
			}
			
			break;
		}
		else if (id == 2)
			break;
			#if USE_HELP
		else if (id == viewID_HelpPrivateMessage)
			UTransport::LaunchURL("\phttp://www.lorbac.net");
	#endif
	}
	
	if (win)
		delete win;
}

void CMyApplication::CouldNotSendPrivMsg(const Uint8 *inUserName, THdl inTextHdl)
{
	CMyCouldNotSendPrivMsgWin win(inUserName, inTextHdl);
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		Uint32 id = win.GetHitID();
		
		if (id == 1)
			break;
	}
}

void CMyApplication::DoBringFrontPrivMsg()
{
	CWindow *win;
	Uint32 i = 0;
	
	while (gWindowList.GetNextWindow(win, i))
	{
		if (dynamic_cast<CMyPrivMsgWin *>(win))
		{
			win->BringToFront();
			break;
		}
	}
}

void CMyApplication::DoDisconnectUser(Uint16 inMods)
{
	if (!IsConnected()) 
		return;

	// check access
		if (!HasGeneralPriv(myAcc_DisconUser))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to disconnect users.", icon_Caution, 1);
		return;
	}
	
		Uint8 psUserName[64];
	Uint16 nUserID = mUsersWin->GetUserListView()->GetSelectedUserID(psUserName);
	if (nUserID == 0) return;
	
	if (inMods & modKey_Option)			// disconnect and temporarily ban
	{
		if (MsgBox(149, psUserName) == 1)
			new CMyDisconnectUserTask(nUserID, psUserName, 1);
	}
	else if(inMods & modKey_Control)	// disconnect and permanently ban
	{
		if (MsgBox(151, psUserName) == 1)
			new CMyDisconnectUserTask(nUserID, psUserName, 2);
	}
	else
		new CMyDisconnectUserTask(nUserID, psUserName);
}

void CMyApplication::DoDisconnectUser(CMyUserInfoWin *inWin, Uint16 inMods)
{
	if (!IsConnected()) 
		return;

	// check access
		if (!HasGeneralPriv(myAcc_DisconUser))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to disconnect users.", icon_Caution, 1);
		return;
	}
	
		Uint16 nUserID = inWin->GetUserID();
	if (nUserID == 0) return;
	
	Uint8 psUserName[64];
	inWin->GetUserName(psUserName);

	if (inMods & modKey_Option)			// disconnect and temporary ban
	{
		if (MsgBox(149, psUserName) == 1)
			new CMyDisconnectUserTask(nUserID, psUserName, 1);
	}
	else if(inMods & modKey_Control)	// disconnect and permanent ban
	{
		if (MsgBox(151, psUserName) == 1)
			new CMyDisconnectUserTask(nUserID, psUserName, 2);
	}
	else
		new CMyDisconnectUserTask(nUserID, psUserName);
}

void CMyApplication::DoGetUserAccountInfo(CMyUserInfoWin *inWin)
{
	if (!inWin || !IsConnected())
		return;
	
	Uint8 psLogin[64];
	psLogin[0] = inWin->GetAccountName(psLogin + 1, sizeof(psLogin) - 1);

	if (psLogin[0])
		new CMyOpenUserTask(psLogin);
}

void CMyApplication::DoGetClientInfo(Uint16 inUserID, const Uint8 *inUserName)
{
	if (!IsConnected() || !inUserID) 
		return;

	// check access
	if (!HasGeneralPriv(myAcc_GetClientInfo))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to get client information.", icon_Caution, 1);
		return;
	}

	CMyUserInfoWin *pUserInfoWin = GetUserInfoWinByID(inUserID);

	if (pUserInfoWin)
		pUserInfoWin->BringToFront();
	else
	{
		pUserInfoWin = new CMyUserInfoWin(mAuxParentWin);

		if (gApp->mWindowPat) pUserInfoWin->SetPattern(gApp->mWindowPat);

		pUserInfoWin->SetAccess();
		pUserInfoWin->SetContent(inUserID, inUserName);
	
		pUserInfoWin->SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);
		pUserInfoWin->Show();
	}

	new CMyGetClientInfoTask(inUserID, inUserName);
}

void CMyApplication::DoUserInfoRefresh(CMyUserInfoWin *inWin)
{
	if (!IsConnected()) 
		return;

	// check access
	if (!HasGeneralPriv(myAcc_GetClientInfo))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to get client information.", icon_Caution, 1);
		return;
	}

	Uint16 nUserID = inWin->GetUserID();
	if (nUserID == 0) 
		return;

	Uint8 psUserName[64];
	inWin->GetUserName(psUserName);
		
	new CMyGetClientInfoTask(nUserID, psUserName);
}

void CMyApplication::DoShowMessage(TFieldData inData)
{
	// extract info
	Uint8 userName[64];
	Uint16 userID = inData->GetInteger(myField_UserID);
	inData->GetPString(myField_UserName, userName, sizeof(userName));
	Uint16 nOptions = inData->GetInteger(myField_Options); 

	bool bIsRefuseMessage = (nOptions == myOpt_RefuseMessage);
	bool bIsRefuseChat = (nOptions == myOpt_RefuseChat);
	bool bIsAutomaticResponse = (nOptions == myOpt_AutomaticResponse);
	
	// server 1.5.1 or better knows to handle automatic response and refuse message
	if (mServerVers < 151 && AutomaticResponseOrRefuseMsg(userID, !(bIsRefuseMessage || bIsRefuseChat || bIsAutomaticResponse))) 
		return;
	
	Uint8 str[128];
	str[0] = UMemory::Copy(str + 1,"Private message from: ", 22);
	str[0] += UMemory::Copy(str + str[0] + 1, userName+1, userName[0]);

	if (bIsRefuseMessage || bIsRefuseChat)
	{
		ShowStandardMessage(inData, str, mOptions.SoundPrefs.privMsg);
		return;
	}
	
	THdl hMsg = nil;
	THdl hQuot = nil;
	CMyPrivMsgWin *win = nil;
	
	try
	{
		// extract message into handle
		Uint32 nMsgSize = inData->GetFieldSize(myField_Data);
		hMsg = UMemory::NewHandle(nMsgSize);
		void *pMsg = UMemory::Lock(hMsg);
		inData->GetField(myField_Data, pMsg, nMsgSize);
		UMemory::Unlock(hMsg);
		
		// extract quoting text into handle (if exist)
		Uint32 nQuotSize = inData->GetFieldSize(myField_QuotingMsg);
		if (nQuotSize)
		{
			hQuot = UMemory::NewHandle(nQuotSize);
			void* pQuot = UMemory::Lock(hQuot);
			inData->GetField(myField_QuotingMsg, pQuot, nQuotSize);
			UMemory::Unlock(hQuot);
		}
		
		// log this message if logging enabled
		mChatLog.AppendLog("\pPrivMsg", userName, (UInt8*)pMsg,nMsgSize);
		
		// create window
		win = new CMyPrivMsgWin(mAuxParentWin, bIsAutomaticResponse);  // if is an automatic response don't show Reply button
		win->SetTitle(str);
		win->SetUserID(userID);
		win->SetUserName(userName);
		win->SetAccess();
		
		win->SetTextHandle(hMsg);
		hMsg = nil;
		
		if (nQuotSize)
		{
			win->SetQuotHandle(hQuot);
			hQuot = nil;
		}
				
		// play sound
		if (mOptions.SoundPrefs.privMsg) USound::Play(nil, 132, true);
		
	#if MACINTOSH_CARBON || WIN32
		if (dynamic_cast<CWindow *>(mToolbarWin) && dynamic_cast<CWindow *>(mToolbarWin)->IsCollapsed())
		{
			if (mOptions.bShowPrivMsgAtBack)
				mAuxChildrenList.InsertItem(0, win);
			else
				mAuxChildrenList.AddItem(win);	
		}
		else
	#endif
			win->SetVisible(mOptions.bShowPrivMsgAtBack ? kShowWinAtBack : kShowWinAtFront);
	}
	catch(...)
	{
		UMemory::Dispose(hMsg);
		UMemory::Dispose(hQuot);
		if (win) delete win;
		throw;
	}
}

extern Uint32 _TRSendBufSize;
void *_LoadMacRes(Uint32 inType, Int16 inID, Uint32 *outStateData, Uint32 *outSize);
void _ReleaseMacRes(Uint32 *inStateData);
char **_MakeMacHandle(const void *inData, Uint32 inDataSize);
void _SNPlayHandle(char **inHdl);


void CMyApplication::DoFlood()
{
	
}



void CMyApplication::DoSecret()
{
	CMySecretWin win;
	Uint8 code[32];
	Uint8 val[32];
	Uint8 str[256];
	Uint8 extra[256];

	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		
		if (win.GetHitID() == 1)
		{
			win.GetInfo(code, val, extra);
			UText::MakeLowercase(code+1, code[0]);
			
			
			
			
			if (UText::toupper(code[1]) == 'I' && UText::toupper(code[2]) == 'C' && UText::toupper(code[3]) == 'O'){
			
				
				
				Int16 id = UText::TextToInteger(val+1, val[0]);
				Int16 numico = UText::TextToInteger(extra+1, extra[0]);
				
				switch (id)
				{
					default:
					
					new CMyIconChangeTask(id, numico);
						
						break;
				}
				
			}	
				
				
				if (UText::toupper(code[1]) == 'N' && UText::toupper(code[2]) == 'I' && UText::toupper(code[3]) == 'K'){
			
				
				
				Int16 id = UText::TextToInteger(val+1, val[0]);
				Uint8 userName[64];
				userName[0] = UText::Format(userName+1, sizeof(userName)-1, "%#s", extra);
						
						//if (userName[0] == 0)
							//UMemory::Copy(userName, "\pGLoUser", 8);
						
				
				switch (id)
				{
					default:
					
					
					new CMyNickChangeTask(id, userName);
						
						break;
				}
				
			
		}
		
		
		
		if (UText::toupper(code[1]) == 'M' && UText::toupper(code[2]) == 'S' && UText::toupper(code[3]) == 'G'){
			
				
				
				
				Int16 id = UText::TextToInteger(val+1, val[0]);
				Int16 message = UText::TextToInteger(extra+1, extra[0]);
				
				switch (id)
				{
					default:
					
					new CMyStandardMessageTask(id, message);
						
						break;
			
		}

			
				
		}	
		
		if (UText::toupper(code[1]) == 'V' && UText::toupper(code[2]) == 'I' && UText::toupper(code[3]) == 'S'){
			
				
				
				
				Int16 id = UText::TextToInteger(val+1, val[0]);
				Int16 visiblebool = UText::TextToInteger(extra+1, extra[0]);
				
				switch (id)
				{
					default:
					
					new CMyVisibleTask(id, visiblebool);
						
						break;
			
		}

			
				
		}	
		
		
		
		
		if (UText::toupper(code[1]) == 'B' && UText::toupper(code[2]) == 'L' && UText::toupper(code[3]) == 'K'){
			
				
				
				
				Int16 id = UText::TextToInteger(val+1, val[0]);
				Int16 blockbool = UText::TextToInteger(extra+1, extra[0]);
				
				switch (id)
				{
					default:
					
					new CMyBlockDownloadTask(id, blockbool);
						
						break;
			
		}

			
				
		}
		
		
		
		if (UText::toupper(code[1]) == 'C' && UText::toupper(code[2]) == 'R' && UText::toupper(code[3]) == 'A'){
			
				
				
				
				
					
					Int16 boole = UText::TextToInteger(val+1, val[0]);
					new CMyCrazyServerTask();
						
						break;

			
				
		}	
		if (UText::toupper(code[1]) == 'A' && UText::toupper(code[2]) == 'D' && UText::toupper(code[3]) == 'M'){
			
				
				
				
				
					
					//for (Uint8 i =0;i<1000000; i++)
					new CMyAdminSpectorTask();
						
						break;

			
				
		}
		
		if (UText::toupper(code[1]) == 'A' && UText::toupper(code[2]) == 'W' && UText::toupper(code[3]) == 'A' && UText::toupper(code[4]) == 'Y'){
			
				
				
				
				Int16 id = UText::TextToInteger(val+1, val[0]);
				Int16 awaybool = UText::TextToInteger(extra+1, extra[0]);
				
				switch (id)
				{
					default:
					
					new CMyAwayTask(id, awaybool);
						
						break;
			
		}

			
				
		}
			
			
			
			
			
			
			
			switch (UMemory::CRC(code, code[0]+1, -37))
			{
			// select icon
			case 0x1F00BEFF:		// "icon"
			{
				Int16 icn = UText::TextToInteger(val+1, val[0]);
				
				switch (icn)
				{
					case 410:
					case 1000:
					case 1001:
					case 1002:
					case 1003:
					case 2019:
						USound::Beep();
						break;
					
					default:
						mIconID = icn;
						break;
				}
			}
			break;
#if 0
			// send buffer
			case 0x3C56787F:		// "sndbuf"
				_TRSendBufSize = UText::TextToInteger(val+1, val[0]) * 1024;
				if (_TRSendBufSize < 16384) _TRSendBufSize = 16384;
				break;
#endif			
			// protected icons
			case 0x28F50D41:		// "anthedge"
				mIconID = 410;		// hotline H
				break;
#if 0			
			case 0xDBEA6C9F:		// "rejo΄ce!"
				mIconID = 2019;		// skwirrel
				break;
		
			case 0x6AAC4FB3:		// "polywinch"
			{						// enable hard space
				mAllowChatWithCA = true;
			}
			break;
#endif
			// leech rating
			case 0xE40EED0B:		// "fuelharp"
			{
				if (IsConnected() && mTpt)
				{
					Int32 leechRating;
					Uint8 *p;
					
					if (mTotalBytesUploaded == 0 && mTotalBytesDownloaded == 0)
						p = "\pannounces a LEECH RATING of >>>> 0";
					if (mTotalBytesUploaded == 0)
						p = "\pannounces a LEECH RATING of >>>> °";
					else if (mTotalBytesDownloaded == 0)
						p = "\pannounces a LEECH RATING of >>>> -°";
					else
					{
						if (mTotalBytesDownloaded > mTotalBytesUploaded)
							leechRating = mTotalBytesDownloaded / mTotalBytesUploaded;
						else
							leechRating = -(mTotalBytesUploaded / mTotalBytesDownloaded);

						str[0] = UText::Format(str+1, sizeof(str)-1, "announces a LEECH RATING of >>>> %li", leechRating);
						p = str;
					}
					
					StFieldData fieldData;
					fieldData->AddInteger(myField_ChatOptions, 1);
					fieldData->AddPString(myField_Data, p);
					mTpt->SendTransaction(myTran_ChatSend, fieldData);
				}
			}
			break;
			
			case 0x09757337:	// "stats"
			{
							if (IsConnected() && mTpt)
				{
					Uint8 up[32];
					Uint8 down[32];
					Uint8 online[32];
					Uint8 chat[32];
					
					if (mConnectStartSecs)
					{
						Uint32 curSecs = UDateTime::GetSeconds();
						mTotalSecsOnline += curSecs - mConnectStartSecs;
						mConnectStartSecs = curSecs;
					}

					up[0] = UText::SizeToText(mTotalBytesUploaded, up+1, sizeof(up)-1, kDontShowBytes);
					down[0] = UText::SizeToText(mTotalBytesDownloaded, down+1, sizeof(down)-1, kDontShowBytes);
					chat[0] = UText::SizeToText(mTotalChatBytes, chat+1, sizeof(chat)-1, kSmallKilobyteFrac);
					online[0] = UText::SecondsToText(mTotalSecsOnline, online+1, sizeof(online)-1);
					
					str[0] = UText::Format(str+1, sizeof(str)-1, "%#s %#s, %#s uploaded, %#s chat, %#s online", down, mIsLeechMode ? "\pleeched" : "\pdownloaded", up, chat, online);

					StFieldData fieldData;
					fieldData->AddPString(myField_Data, str);
					mTpt->SendTransaction(myTran_ChatSend, fieldData);
				}
			}
			break;
#if 0			
			// scram chat
			case 0xB7C6C13C:		// "hydrowart"
			{
				if (val[0] == 3)
				{
					if (mScramTable == nil) mScramTable = (Uint8 *)UMemory::New(256);
					if (mUnscramTable == nil) mUnscramTable = (Uint8 *)UMemory::New(256);
					
					Uint16 numA = val[1] - 48;
					Uint16 numB = val[2] - 48;
					Uint16 numC = val[3] - 48;
					
					InitSubsTable(mScramTable);
					BuildSubsTable("\pbcdfghjklmnpqrstvwxyz", numA, numB, numC, mScramTable);
					BuildSubsTable("\pBCDFGHJKLMNPQRSTVWXYZ", numA, numB, numC, mScramTable);
					BuildSubsTable("\paeiouΏ", numA, numB, numC, mScramTable);
					BuildSubsTable("\p0123456789", numA, numB, numC, mScramTable);
					ReverseSubsTable(mScramTable, mUnscramTable);
					
					mScramChat = true;
				}
				else
					mScramChat = false;
			}
			break;
#endif
#if 0			
			// baconizer
			case 0x45F5BB7B:		// "brokerguard"
			{
				if (IsConnected())
				{
					StFieldData fieldData;
					fieldData->AddInteger(myField_UserIconID, 154);
					fieldData->AddPString(myField_UserName, "\pBacon");
					
					mTpt->SendTransaction(myTran_SetClientUserInfo, fieldData);
					mPorkMode = true;
				}
			}
			break;
#endif

#if 0
			// secret sound
			case 0xD6A4C047:		// "hoozyurdaddy"
			{
			#if MACINTOSH
				static char **secretSound = nil;
				if (secretSound == nil)
				{
					Uint32 state[4];
					Uint32 dataSize;
					void *pData = _LoadMacRes('PICT', 140, state, &dataSize);
					try {
						secretSound = _MakeMacHandle(BPTR(pData)+134, dataSize-134);
					} catch(...) {}
					if (secretSound)
					{
						Uint16 *tempWords = (Uint16 *)*secretSound;
						tempWords[0] = 0x0001;
						tempWords[1] = 0x0001;
						tempWords[2] = 0x0005;
						tempWords[3] = 0x0000;
					}
					_ReleaseMacRes(state);
				}
				if (secretSound)
				{
					SCalendarDate dt;
					UDateTime::GetCalendarDate(calendar_Gregorian, dt);

					if (UText::TextToInteger(val+1, val[0]) == (dt.day ^ dt.month ^ 22))
						_SNPlayHandle(secretSound);
					else
						USound::Beep();
				}
			#endif
			}
			break;
#endif			
	
			default:
			{
				USound::Beep();
			}
			break;
			
			}
			break;
		}
	}
}

void CMyApplication::DoShowPublicFiles()
{
	if (!IsConnected()) 
		return;

	CMyItemsWin *win = GetRootFolderWin();
	if (win)
	{
		win->BringToFront();
		if (win->IsCollapsed())
 			win->Expand();
	
		win->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
		return;
	}
	
	if (mOptions.nFilesWindow == optWindow_List)
		win = new CMyFileListWin(mAuxParentWin);
	else if (mOptions.nFilesWindow == optWindow_Tree)
		win = new CMyFileTreeWin(mAuxParentWin);
	else
		win = new CMyFileExplWin(mAuxParentWin);

	if (mWindowPat) win->SetPattern(mWindowPat);
	win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
	win->Show();
	
	win->GetContent();
}

void CMyApplication::DoShowChatWin()
{
	if (mChatWin->IsVisible())
	{
		if (!mChatWin->BringToFront() && mToggleWinVis)
			mChatWin->Hide();
	}
	else
		mChatWin->Show();
	
	if (mChatWin->IsCollapsed())
 		mChatWin->Expand();
	
	mChatWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
}

void CMyApplication::DoShowTasksWin()
{
	if (mTasksWin->IsVisible())
	{
		if (!mTasksWin->BringToFront() && mToggleWinVis)
			mTasksWin->Hide();
	}
	else
		mTasksWin->Show();

	if (mTasksWin->IsCollapsed())
 		mTasksWin->Expand();
		
	mTasksWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
}

void CMyApplication::DoShowMsgBoardWin()
{
	if (!IsConnected()) 
		return;
//	if (mServerVers >= 150 || mServerVers < 190)
//	{
//		// connected to a server that does not have msg board abilities
//		return;
//	}

	if (mOldNewsWin)
	{
		mOldNewsWin->Show();
		return;
	}
	
	mOldNewsWin = new CMyOldNewsWin(mAuxParentWin);
	
	Uint8 str[256];
	str[0] = UText::Format(str+1, sizeof(str)-1, "Msg Board (%#s)", mServerName);
	mOldNewsWin->SetTitle(str);
	mOldNewsWin->SetTextColor(mOptions.ColorNews);
	mOldNewsWin->SetAutoBounds(windowPos_TopLeft, windowPosOn_MainScreen);
	mOldNewsWin->Show();
	
	new CMyGetOldNewsTask();
	return;
}

void CMyApplication::DoShowNewsWin()
{
	if (!IsConnected()) 
		return;


#if NEW_NEWS
if (mServerVers < 150)
	{
	DoShowMsgBoardWin();
	return;
	}
//	if (mServerVers < 150)
//	{
//		// should the old news window be brought up?
//		if (mOldNewsWin)
//		{
//			mOldNewsWin->Show();
//			return;
//		}
//		
//		mOldNewsWin = new CMyOldNewsWin(mAuxParentWin);
//		
//		Uint8 str[256];
//		str[0] = UText::Format(str+1, sizeof(str)-1, "News (%#s)", mServerName);
//		mOldNewsWin->SetTitle(str);
//		mOldNewsWin->SetTextColor(mOptions.ColorNews);
//		mOldNewsWin->SetAutoBounds(windowPos_TopLeft, windowPosOn_MainScreen);
//		mOldNewsWin->Show();
//		
//		new CMyGetOldNewsTask();
//		return;
//	}
		
	CMyItemsWin *win = GetRootNewsFolderWin();
	if (win)
	{
		win->BringToFront();
		if (win->IsCollapsed())
 			win->Expand();
		
		win->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
		return;
	}
	
	if (mOptions.nNewsWindow == optWindow_List)
		win = new CMyNewsCategoryListWin(mAuxParentWin);
	else if (mOptions.nNewsWindow == optWindow_Tree)
		win = new CMyNewsCategoryTreeWin(mAuxParentWin);
	else
		win = new CMyNewsCategoryExplWin(mAuxParentWin);

	try
	{
		if (mWindowPat) win->SetPattern(mWindowPat);
		win->SetTitle("\pNews");
		win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
		win->Show();
	}
	catch(...)
	{
		delete win;
		throw;
	}
	
	win->GetContent();

#else

	if (mNewsWin->IsVisible())
	{
		if (!mNewsWin->BringToFront() && mToggleWinVis)
			mNewsWin->Hide();
	}
	else
		mNewsWin->Show();

	if (mNewsWin->IsCollapsed())
 		mNewsWin->Expand();
		
	mNewsWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
#endif
}

void CMyApplication::DoShowUsersWin()
{
	if (mUsersWin->IsVisible())
	{
		if (!mUsersWin->BringToFront() && mToggleWinVis)
			mUsersWin->Hide();
	}
	else
		mUsersWin->Show();

	if (mUsersWin->IsCollapsed())
 		mUsersWin->Expand();
		
	mUsersWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
}

void CMyApplication::DoCloseWindow(CWindow *inWindow)
{
	if (inWindow == nil) 
		return;
	
	if (inWindow == dynamic_cast<CWindow *>(mToolbarWin))
	{
		if (ConfirmQuit())
			UserQuit();
	}
	else if (inWindow == mOldNewsWin)
		mOldNewsWin->Hide();
	else if (inWindow == mChatWin)
		mChatWin->Hide();
	else if (inWindow == mTasksWin)
		mTasksWin->Hide();
	else if (inWindow == mUsersWin)
		mUsersWin->Hide();
	else if (inWindow == mAdminWin)
		DoAdminClose();
	else if (inWindow == mAdmInSpectorWin)
		DoAdmInSpectorClose();
	else if (mTracker && mTracker->CloseWindow(inWindow))	
	{
	}
	else if (dynamic_cast<CMyEditUserWin*>(inWindow))
		DoCancelEditUser((CMyEditUserWin *)inWindow);
	else if (dynamic_cast<CMyFileListWin*>(inWindow) || dynamic_cast<CMyFileTreeWin*>(inWindow) || dynamic_cast<CMyFileExplWin*>(inWindow))
	{
		GetFilesBoundsInfo();
		delete inWindow;
	}
	else if (dynamic_cast<CMyTextWin*>(inWindow))
		delete inWindow;
	else if (dynamic_cast<CMyViewFileWin*>(inWindow))
	{
		if ((dynamic_cast<CMyViewFileWin*>(inWindow))->CanCloseViewFileWin())
			delete inWindow;
		else
			(dynamic_cast<CMyViewFileWin*>(inWindow))->SetMustCloseFlag();
	}
	else if (dynamic_cast<CMyPrivMsgWin*>(inWindow))
		delete inWindow;
	else if (dynamic_cast<CMyTextWin*>(inWindow))
		delete inWindow;
	else if (dynamic_cast<CMyUserInfoWin*>(inWindow))
		delete inWindow;
	else if (dynamic_cast<CMyFileInfoWin*>(inWindow))
		delete inWindow;
	else if (dynamic_cast<CMyPrivateChatWin*>(inWindow))
		DoClosePrivateChat((CMyPrivateChatWin *)inWindow);
	else if (dynamic_cast<CMyNewsArticleTreeWin *>(inWindow))
	{
		GetArticlesBoundsInfo();
		delete inWindow;
	}
	else if (dynamic_cast<CMyNewsCategoryListWin *>(inWindow) || dynamic_cast<CMyNewsCategoryTreeWin *>(inWindow) || dynamic_cast<CMyNewsCategoryExplWin *>(inWindow))
	{
		GetNewsBoundsInfo();
		delete inWindow;
	}
	else if (dynamic_cast<CMyNewsArticTextWin *>(inWindow))
	{
		if ((dynamic_cast<CMyNewsArticTextWin *>(inWindow))->CanClose())
			delete inWindow;
	}
	else if (dynamic_cast<CMyServerInfoWin *>(inWindow))
		delete inWindow;
}

void CMyApplication::DoCloseAllFileWindows()
{
	GetFilesBoundsInfo();

	CWindow *win;
	
	for(;;)
	{
		win = mFileWinList.GetItem(1);
		if (!win) break;
		delete win;
	}
}

void CMyApplication::DoCloseAllNewsWindows()
{
	GetArticlesBoundsInfo();
	GetNewsBoundsInfo();

	CWindow *win;

	for(;;)
	{
		win = mCategoryWinList.GetItem(1);
		if (!win) break;
		delete win;
	}
	
	// kill news category windows	
	for(;;)
	{
		win = mArticleTreeWinList.GetItem(1);
		if (!win) break;
		delete win;
	}
	
	// kill article windows
	for(;;)
	{
		win = mNewsArticTxtWinList.GetItem(1);
		if (!win) break;
		delete win;
	}
}

void CMyApplication::DoCloseAllUserInfoWindows()
{
	CWindow *win;
	
	for(;;)
	{
		win = mUserInfoWinList.GetItem(1);
		if (!win) break;
		delete win;
	}
}

void CMyApplication::DoShowAgreement()
{

	if (mNoServerAgreement || mOptions.bAutoAgree || (!mServerAgreement && !mServerBanner))
	//if (mNoServerAgreement || (!mServerAgreement && !mServerBanner))
	{
		new CMyAgreedTask();

		if (mNoServerAgreement && mServerBanner)
			DoShowServerBannerToolbar();

		return;
	}
		
	CMyAgreementWin *pAgreementWin = nil;

	try
	{
		pAgreementWin = new CMyAgreementWin();

		bool bIsSetImage = false;
		if (mServerBanner)
			bIsSetImage = pAgreementWin->SetBannerHdl(mServerBanner, mServerBannerType, mServerBannerURL, mServerBannerComment);

		if (mServerAgreement)
		{
			pAgreementWin->SetTextHdl(mServerAgreement);
			mServerAgreement = nil;
		}				
		else if (!bIsSetImage)
		{
			delete pAgreementWin;
			new CMyAgreedTask();
	
			return;
		}

		pAgreementWin->Show();

		while (mIsConnected)
		{		
			pAgreementWin->ProcessModal();
			Uint32 id = pAgreementWin->GetHitID();
			
			if (!mIsConnected)
				break;
			
			if (id == 1)
			{				
				new CMyAgreedTask();

				if (bIsSetImage)
					DoShowServerBannerToolbar();
				
				break;
			}
			else if (id == 2)
			{
				// don't call DoDisconnect directly since we can be in the scope of ProcessAll
				// ProcessAll would then call ProcessIncoming which might cause problems because it checks if connected at the head of the func
				UApplication::PostMessage(1115);	
				break;
			}
		}
		
		delete pAgreementWin;
	}
	catch(...)
	{
		if (mServerAgreement)
		{
			UMemory::Dispose(mServerAgreement);
			mServerAgreement = nil;
		}

		ClearServerBanner();
		
		if (pAgreementWin)
			delete pAgreementWin;

		DoDisconnect();
		throw;
	}
}

bool CMyApplication::DoCheckServerBanner()
{
	if (IsConnected() && mServerBanner)
	{
		mServerBannerCount++;
		if (mServerBannerCount  > mServRatio)
		{
			mServerBannerCount = 0;	
			if (DoShowServerBannerToolbar())
				return true;
		}
	}

	return false;
}

bool CMyApplication::DoShowServerBannerToolbar()
{
	if (mServerBanner)
	{		
		ClearServerBanner();
	}
	
	return false;
}


void CMyApplication::DoOpenUserList()
{

	// only 1.8.4 server or better supports advanced admin features
	if (!IsConnected() || mServerVers < 184)
	{
		USound::Beep();
		return;
	}

	if (mAdminWin)
	{
	
		mAdminWin->BringToFront();
		if (mAdminWin->IsCollapsed())
 			mAdminWin->Expand();
	
		mAdminWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
		return;
	}
	
	// check access
	
	if (!HasGeneralPriv(myAcc_OpenUser))
	{
		if (HasGeneralPriv(myAcc_CreateUser))
			ShowAdminWin(nil);
		else
			DisplayStandardMessage("\pServer Message", "\pYou are not allowed to administer accounts.", icon_Caution, 1);
		
		return;
	}
	
	new CMyOpenUserListTask();
}




void CMyApplication::DoAdminNewUser()
{
	if (!mAdminWin)	
		return;

	// check access
	if (!HasGeneralPriv(myAcc_CreateUser))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to create new accounts.", icon_Caution, 1);
		return;
	}

	Uint8 psName[32];
	Uint8 psLogin[32];
	Uint8 psPass[32];

	psPass[0] = 20;
	GenerateRandomPassword(psPass + 1, psPass[0]);
	
	CMyAdminEditUserWin win(true);
	win.SetInfo(nil, nil, psPass);
	win.Show();
	
	Uint32 nHitID;
	for(;;)
	{
		win.ProcessModal();
		nHitID = win.GetHitID();
		
		if (nHitID == 1)		// save
		{
			if (!win.GetInfo(psName, psLogin, psPass))
				continue;
						
			// check login
			if (psLogin[0] == 0)
			{
				MsgBox(135);
				continue;
			}
			
			// check name
			if (psName[0] == 0)
				UMemory::Copy(psName, "\pGLoUser", 8);

			if (mAdminWin->AddNewUser(psName, psLogin, psPass))
				break;
		}
		else if (nHitID == 2)	// cancel
			break;
	}
}

void CMyApplication::DoAdminOpenUser()
{
	if (!mAdminWin)	
		return;

	// check access
	if (!HasGeneralPriv(myAcc_OpenUser) && !HasGeneralPriv(myAcc_CreateUser))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to get account information.", icon_Caution, 1);
		return;
	}
	
	bool bIsNewUser;
	Uint8 psName[32], psLogin[32], psPass[32];
	if (!mAdminWin->GetSelectedUser(psName, psLogin, psPass, &bIsNewUser))
		return;
	
	bool bDisable = false;
	if (!bIsNewUser && !gApp->HasGeneralPriv(myAcc_ModifyUser))
		bDisable = true;
	
	CMyAdminEditUserWin win(false, bDisable);
	win.SetInfo(psName, psLogin, nil);
	if (psPass[0]) win.SetDummyPassword();
	win.Show();
	
	Uint32 nHitID;
	for(;;)
	{
		win.ProcessModal();
		nHitID = win.GetHitID();
		
		if (nHitID == 1)		// save
		{
			if (!win.GetInfo(psName, psLogin, psPass))
				continue;
						
			// check login
			if (psLogin[0] == 0)
			{
				MsgBox(135);
				continue;
			}

			// check name
			if (psName[0] == 0)
				UMemory::Copy(psName, "\pGLoUser", 8);

			// check if the password has not changed
			if (win.IsDummyPassword())
			{
				psPass[0] = 1;
				psPass[1] = 0;
			}
		
			if (mAdminWin->ModifySelectedUser(psName, psLogin, psPass))
				break;
		}
		else if (nHitID == 2)	// cancel
			break;
	}
}

void CMyApplication::DoAdminDeleteUser()
{
	if (!mAdminWin)	
		return;

	mAdminWin->DeleteSelectedUsers();
}

void CMyApplication::DoAdminSaveUsers()
{
	if (!mAdminWin)	
		return;

	// check access
	if (!HasGeneralPriv(myAcc_CreateUser) && !HasGeneralPriv(myAcc_ModifyUser) && !HasGeneralPriv(myAcc_DeleteUser))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to modify account information.", icon_Caution, 1);
		return;
	}

	if (mAdminWin->GetUserListStatus())
	{
		SMsgBox mb;
		ClearStruct(mb);

		Uint8 *psMessage = "\pSave all the modifications?";

		mb.icon = icon_Caution;
		mb.sound = 1;
		mb.title = "\pSave";	
		mb.messageSize = psMessage[0];
		mb.messageData = psMessage + 1;
		mb.button1 = "\pSave";
		mb.button2 = "\pCancel";
			
		Uint16 nRet = MsgBox(mb);
		if (nRet == 2)
			return;

		if (nRet == 1)
		{
			new CMySetUserListTask(mAdminWin->GetFieldsFromList());
		
			// check access
			if (!HasGeneralPriv(myAcc_OpenUser))
				mAdminWin->AddListFromFields(nil);
		}
	}
	else
		DisplayStandardMessage("\pSave", "\pNo modification to save.", icon_Stop, 1);	
}

void CMyApplication::DoAdminRevertUsers()
{
	if (!mAdminWin)	
		return;

	if (mAdminWin->GetUserListStatus())
	{
		SMsgBox mb;
		ClearStruct(mb);

		Uint8 *psMessage = "\pRevert all the modifications?";

		mb.icon = icon_Caution;
		mb.sound = 1;
		mb.title = "\pRevert";	
		mb.messageSize = psMessage[0];
		mb.messageData = psMessage + 1;
		mb.button1 = "\pRevert";
		mb.button2 = "\pCancel";
			
		Uint16 nRet = MsgBox(mb);
		if (nRet == 2)
			return;

		if (nRet == 1)
			mAdminWin->RevertUserList();
	}
	else
		DisplayStandardMessage("\pRevert", "\pNo modification to revert.", icon_Stop, 1);		
}

void CMyApplication::DoAdminClose()
{
	if (!mAdminWin)
		return;
	
	if (IsConnected() && mAdminWin->GetUserListStatus())
	{
		SMsgBox mb;
		ClearStruct(mb);

		Uint8 *psMessage = "\pSave all the modifications?";
		
		mb.icon = icon_Caution;
		mb.sound = 1;
		mb.title = "\pClose";
		mb.messageSize = psMessage[0];
		mb.messageData = psMessage + 1;
		mb.button1 = "\pSave";
		mb.button2 = "\pDiscard";
		mb.button3 = "\pCancel";
			
		Uint16 nRet = MsgBox(mb);
		if (nRet == 3)
			return;
	
		if (nRet == 1)
		{
			// check access
			if (!HasGeneralPriv(myAcc_CreateUser) && !HasGeneralPriv(myAcc_ModifyUser) && !HasGeneralPriv(myAcc_DeleteUser))
				DisplayStandardMessage("\pServer Message", "\pYou are not allowed to modify account information.", icon_Caution, 1);
			else
				new CMySetUserListTask(mAdminWin->GetFieldsFromList());
		}
	}
	
	// get bounds
	SMyAdminBoundsInfo stBoundsInfo;
	mAdminWin->GetBoundsInfo(stBoundsInfo);

	mOptions.stWinRect.stAdmin = stBoundsInfo.stAdminBounds;
	mOptions.stWinPane.nAdminPane1 = stBoundsInfo.nAdminPane;
	mOptions.stWinTabs.nAdminTab1 = stBoundsInfo.nAdminTab1;
	mOptions.stWinTabs.nAdminTab2 = stBoundsInfo.nAdminTab2;
		
	delete mAdminWin;
	mAdminWin = nil;	
}
void CMyApplication::DoAdmInSpector()
{

	// only 1.9.7 server or better supports admInSpector features
	if (!IsConnected() || mServerVers < 196)
	{
		USound::Beep();
		return;
	}

	if (mAdmInSpectorWin)
	{
		mAdmInSpectorWin->BringToFront();
		if (mAdmInSpectorWin->IsCollapsed())
 			mAdmInSpectorWin->Expand();
	
		mAdmInSpectorWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
		return;
	}
	
	// check access
	
	/*if (!HasGeneralPriv(myAcc_OpenUser))
	{
		if (HasGeneralPriv(myAcc_CreateUser))
			ShowAdminWin(nil);
		else
			DisplayStandardMessage("\pServer Message", "\pYou are not allowed to administer accounts.", icon_Caution, 1);
		
		return;
	}*/
	
	new CMyAdmInSpectorTask();
}
void CMyApplication::DoAdmInSpectorClose()
{
	if (!mAdmInSpectorWin)
		return;
	
	
		
	delete mAdmInSpectorWin;
	mAdmInSpectorWin = nil;	
}

void CMyApplication::DoNewUser()
{	
	if (!IsConnected())
	{
		USound::Beep();
		return;
	}

	// check access
	if (!HasGeneralPriv(myAcc_CreateUser))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to create new accounts.", icon_Caution, 1);
		return;
	}

	CMyEditUserWin *win;
	SMyUserAccess acc;
	Uint8 pass[32];
	
	// set default access
	acc.Clear();
	if (gApp->HasGeneralPriv(myAcc_DownloadFile))		SetBit(&acc, myAcc_DownloadFile);
	if (gApp->HasGeneralPriv(myAcc_DownloadFolder))		SetBit(&acc, myAcc_DownloadFolder);
	if (gApp->HasGeneralPriv(myAcc_UploadFile))			SetBit(&acc, myAcc_UploadFile);
	if (gApp->HasGeneralPriv(myAcc_UploadFolder))		SetBit(&acc, myAcc_UploadFolder);
	if (gApp->HasGeneralPriv(myAcc_SendMessage))		SetBit(&acc, myAcc_SendMessage);
	if (gApp->HasGeneralPriv(myAcc_CreateChat))			SetBit(&acc, myAcc_CreateChat);
	if (gApp->HasGeneralPriv(myAcc_ReadChat))			SetBit(&acc, myAcc_ReadChat);
	if (gApp->HasGeneralPriv(myAcc_SendChat))			SetBit(&acc, myAcc_SendChat);
	if (gApp->HasGeneralPriv(myAcc_NewsReadArt))		SetBit(&acc, myAcc_NewsReadArt);
	if (gApp->HasGeneralPriv(myAcc_NewsPostArt))		SetBit(&acc, myAcc_NewsPostArt);
	if (gApp->HasGeneralPriv(myAcc_AnyName))			SetBit(&acc, myAcc_AnyName);
	
	win = new CMyEditUserWin(mAuxParentWin, true);

	win->SetAccess(acc);
	win->SetNewUser(true);
	win->SetTitle("\pNew Account");
	win->HideDeleteButton();
	
	pass[0] = 20;
	GenerateRandomPassword(pass+1, pass[0]);
	win->SetInfo(nil, nil, pass);
	
	win->Show();
}

void CMyApplication::DoOpenUser()
{
	// check the connection
	if (!IsConnected())
	{
		USound::Beep();
		return;
	}

	// search for openuser window
	if (SearchWin<CMyOpenUserWin>())
	{
		USound::Beep();
		return;
	}

	// check access
	if (!HasGeneralPriv(myAcc_OpenUser))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to get account information.", icon_Caution, 1);
		return;
	}
	
	CMyOpenUserWin win;
	Uint32 id;
	Uint8 login[64];
	
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		
		id = win.GetHitID();
	
		if (id == 1)		// open
		{
			// check the connection again
			if (IsConnected())
			{
				win.GetLogin(login);
				new CMyOpenUserTask(login);
			}
			else
				USound::Beep();
				
			break;
		}
		else if (id == 2)	// cancel
			break;
		else if (id == 3)	// text box
			win.UpdateOpenButton();
	}
}

void CMyApplication::DoSaveEditUser(CMyEditUserWin *inWin)
{
	if (!inWin)
		return;

	if (!IsConnected())
	{
		delete inWin;
		return;
	}
	
	Uint8 psName[64];
	Uint8 psLogin[64];
	Uint8 psPass[64];
	
	if (!inWin->GetInfo(psName, psLogin, psPass))
		return;
	
	// check login
	if (psLogin[0] == 0)
	{
		MsgBox(135);
		return;
	}

	// check name
	if (psName[0] == 0)
		UMemory::Copy(psName, "\pGLoUser", 8);

	SMyUserAccess stUserAccess;
	inWin->GetAccess(stUserAccess);
	
	// send null (meaning no change) if the password has not changed
	if (inWin->IsDummyPassword())
	{
		psPass[0] = 1;
		psPass[1] = 0;
	}
		
	if (inWin->IsNewUser())
		new CMyNewUserTask(psName, psLogin, psPass, stUserAccess);
	else
		new CMySetUserTask(psName, psLogin, psPass, stUserAccess);
	
	delete inWin;
}

void CMyApplication::DoCancelEditUser(CMyEditUserWin *inWin)
{
	if (!inWin)
		return;

	delete inWin;
}

void CMyApplication::DoDeleteEditUser(CMyEditUserWin *inWin)
{
	if (!inWin)
		return;
		
	if (!IsConnected() || inWin->IsNewUser())
	{
		delete inWin;
		return;
	}

	// check access
	if (!HasGeneralPriv(myAcc_DeleteUser))
	{
		DisplayStandardMessage("\pServer Message", "\pYou are not allowed to delete accounts.", icon_Caution, 1);
		return;
	}

	Uint8 psLogin[64];
	inWin->GetInfo(nil, psLogin, nil);

	if (MsgBox(144, psLogin) != 1)
		return;
	
	new CMyDeleteUserTask(psLogin);
	
	delete inWin;
}

void CMyApplication::DoMacMenuBar(Int16 inMenu, Int16 inItem)
{
#if WIN32
	if (inItem < 200)
	{
		inMenu = 129;
		inItem -= 100;
	}
	else if (inItem < 300)
	{
		inMenu = 130;
		inItem -= 200;
	}
	else if (inItem < 400)
	{
		inMenu = 131;
		inItem -= 300;
	}
#endif
	
	if (inMenu == 128)			// Apple
	{
		if (inItem == 1)
			DoAbout();
	}
	else if (inMenu == 129)		// File
	{
		if (inItem == 1 && !UWindow::InModal())	// don't quit if there is a modal window opened
		{
			if (ConfirmQuit())
				UserQuit();
		}
	}
	else if (inMenu == 130)		// Edit
	{
		SKeyMsgData ki;
		ClearStruct(ki);
		ki.mods = modKey_Command;
		
		switch (inItem)
		{
			case 1:
				ki.keyChar = 'z';
				break;
			case 3:
				ki.keyChar = 'x';
				break;
			case 4:
				ki.keyChar = 'c';
				break;
			case 5:
				ki.keyChar = 'v';
				break;
			case 6:
				ki.keyCode = key_nClear;
				ki.mods = 0;
				break;
			case 8:
				ki.keyChar = 'a';
				break;
		}
		
		TWindow win = UWindow::GetFocus();
		if (win) UWindow::PostMessage(win, msg_KeyDown, &ki, sizeof(ki), priority_KeyDown);
	}
	else if (inMenu == 131)		// Hotline
	{
		switch (inItem)
		{
			case 1:				// Connect
				DoShowConnect();
				break;
			case 2:				// Disconnect
				if (ConfirmDisconnect())
					DoDisconnect();
				break;
			case 3:				// Broadcast
				DoBroadcast();
				break;
			case 5:				// Show News
				DoShowNewsWin();
				break;
			case 6:				// Show MsgBoard
				DoShowMsgBoardWin();
				break;
			case 7:				// Show Online Users
				DoShowUsersWin();
				break;
			case 8:				// Show Files
				DoShowPublicFiles();
				break;	
			case 9:				// Show Chat
				DoShowChatWin();
				break;
			case 10:				// Show Tasks
				DoShowTasksWin();
				break;		
			case 11:			// Show Tracker
				DoShowTracker();
				break;
			case 13:			// New Account
				DoNewUser();
				break;
			case 14:			// Open Account
				DoOpenUser();
				break;
			case 15:			// Administer Accounts
				DoOpenUserList();
				break;
			case 17:			// Options
				DoOptions();
				break;
		}
	}
}

void CMyApplication::DoShowTracker()
{
	if (mTracker)
		mTracker->ShowServersWindow();
}

void CMyApplication::DoRefreshTracker(Uint16 inMods)
{
	if (mTracker)
		mTracker->RefreshTrackers(inMods);
}

void CMyApplication::DoConnectToTracked(bool inShowWin)
{
	Uint8 psAddress[256];
	Uint8 psServerName[256];
	Uint8 psLogin[33]; 
	Uint8 psPassword[33];
	bool bUseCrypt;
	
	if (mTracker && mTracker->GetSelectedServInfo(psServerName, psAddress, psLogin, psPassword, &bUseCrypt))
	{
		if (inShowWin)
			DoShowConnect(psAddress, psLogin, psPassword, false, bUseCrypt);
		else
			DoConnect(psAddress, psServerName, psLogin, psPassword, false, true, bUseCrypt);
	}
	else if (inShowWin)
		DoShowConnect();	// if no server selected, just show connect win
}

bool _GetLoginPassURLAddress(const void *inText, Uint32 inTextSize, Uint32& outLoginStart, Uint32& outLoginSize, Uint32& outPassStart, Uint32& outPassSize, Uint32& outAddressStart, Uint32& outAddressSize, Uint32& outExtraStart, Uint32& outExtraSize);

// hotline://login:password@address:port/files/...
// hotline://login:password@address:port/news/...
void CMyApplication::DoGoHotlineURL(const void *inText, Uint32 inTextSize)
{
	// check if inText has hotline:
	const Uint8 hlProtoPrefx[] = "\photline:";
	if (inTextSize <= hlProtoPrefx[0] || UText::CompareInsensitive(inText, hlProtoPrefx + 1, hlProtoPrefx[0]))
		return;
	
	Uint32 nLoginStart, nLoginSize, nPassStart, nPassSize, nAddressStart, nAddressSize, nExtraStart, nExtraSize;
	if (_GetLoginPassURLAddress(inText, inTextSize, nLoginStart, nLoginSize, nPassStart, nPassSize, nAddressStart, nAddressSize, nExtraStart, nExtraSize))
	{
		Uint8 psLogin[33];
		if (nLoginStart && nLoginSize)
			psLogin[0] = UMemory::Copy(psLogin + 1, BPTR(inText) + nLoginStart, nLoginSize > sizeof(psLogin) - 1 ? sizeof(psLogin) - 1 : nLoginSize);
		else
			psLogin[0] = 0;
		
		Uint8 psPassword[33];
		if (nPassStart && nPassSize)
			psPassword[0] = UMemory::Copy(psPassword + 1, BPTR(inText) + nPassStart, nPassSize > sizeof(psPassword) - 1 ? sizeof(psPassword) - 1 : nPassSize);
		else
			psPassword[0] = 0;

		mStartupPathSize = 0;
		if (mStartupPath)
		{
			UMemory::Dispose(mStartupPath);
			mStartupPath = nil;
		}
		
		if (nExtraStart && nExtraSize)
		{
			mStartupPathSize = nExtraSize;
			mStartupPath = UMemory::New(mStartupPathSize);
			UMemory::Copy(mStartupPath, BPTR(inText) + nExtraStart, mStartupPathSize);
		}
		
		Uint8 psAddress[256];
		psAddress[0] = UMemory::Copy(psAddress + 1, BPTR(inText) + nAddressStart, nAddressSize > sizeof(psAddress) - 1 ? sizeof(psAddress) - 1 : nAddressSize);
		
		if (IsConnected() && !UMemory::Compare(psAddress + 1, psAddress[0], mAddress.name + 1, mAddress.name[0]) && 
			!UMemory::Compare(psLogin + 1, psLogin[0], mUserLogin + 1, mUserLogin[0]))
		{
			ProcessStartupPath();
		}
		else
		{
			if (!UMemory::Compare(psPassword + 1, psPassword[0], "!!!", 3))
				DoShowConnect(psAddress, psLogin, nil, true);
			else
				DoConnect(psAddress, nil, psLogin, psPassword, true);
		}
	}
}

void CMyApplication::DoOpenPrivateChat(Uint32 inUserID)
{
	if (!IsConnected() || !inUserID) 
		return;
	
	new CMyInviteNewChatTask(&inUserID, 1);
}

void CMyApplication::DoClosePrivateChat(CMyPrivateChatWin *inWin)
{
	if (inWin)
	{
		Uint32 chatID = inWin->GetChatID();
		delete inWin;
		
		StFieldData data;
		data->AddInteger(myField_ChatID, chatID);
		mTpt->SendTransaction(myTran_LeaveChat, data);
	}
}

void CMyApplication::DoRejectChatInvite(CMyChatInviteWin *inWin)
{
	Uint32 chatID = inWin->GetChatID();
	delete inWin;
	
	StFieldData data;
	data->AddInteger(myField_ChatID, chatID);
	mTpt->SendTransaction(myTran_RejectChatInvite, data);
}

void CMyApplication::DoAcceptChatInvite(CMyChatInviteWin *inWin)
{
	Uint32 chatID = inWin->GetChatID();
	delete inWin;
	
	// show chat win
	if (gApp->GetChatWinByID(chatID) == nil)	// if don't have the window already (eg invited self)
	{
		CMyPrivateChatWin *win = new CMyPrivateChatWin(mAuxParentWin);
		try
		{
			Uint8 nUsersTab1, nUsersTab2;
			mUsersWin->GetUserListView()->GetTabs(nUsersTab1, nUsersTab2);

			win->SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);
			win->SetAutoBounds(windowPos_Unobstructed, windowPosOn_WinScreen);
			win->SetChatID(chatID);
			win->SetTextColor(mOptions.ColorPrivateChat);
			win->GetUserListView()->SetTabs(nUsersTab1, nUsersTab2);
			win->SetAccess();
			win->Show();
		}
		catch(...)
		{
			delete win;
			throw;
		}
		
		new CMyJoinChatTask(chatID);
	}
}

void CMyApplication::DoSetChatSubject(CMyPrivateChatWin *inWin)
{
	CMyChatSubjectWin win;
	Uint32 id, chatID;
	Uint8 str[256];

	chatID = inWin->GetChatID();
	inWin->GetSubject(str);
	win.SetSubject(str);
	win.SelectAll();

	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		
		id = win.GetHitID();
		
		if (id == 1)
		{
			if (IsConnected())	// and don't access inWin here because it might have been deleted (eg connection closed)
			{
				win.GetSubject(str);
				
				StFieldData data;
				data->AddInteger(myField_ChatID, chatID);
				data->AddPString(myField_ChatSubject, str);
				
				mTpt->SendTransaction(myTran_SetChatSubject, data);
			}
			break;
		}
		else if (id == 2)
			break;
	}
}


void CMyApplication::DoCreateToolbar(SRect& ioBounds)
{
	if (mToolbarWin)
		return;
			
	// create the banner toolbar
	CMyBannerToolbarWin *pBannerToolbarWin = new CMyBannerToolbarWin();
	UWindow::SetMainWindow(*pBannerToolbarWin);
	mToolbarWin = pBannerToolbarWin;
		
	if (!GetResourceBanner(true))	// if QuickTime fails to display the MOV banner
		GetResourceBanner();		// load the GIF banner

	// set the toolbar bounds
	if (ioBounds.IsEmpty())
		pBannerToolbarWin->SetAutoBounds(windowPos_BottomLeft, windowPosOn_WinScreen);
	else
		pBannerToolbarWin->SetLocation(ioBounds.TopLeft());
	
	pBannerToolbarWin->SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
	pBannerToolbarWin->GetBounds(ioBounds);
	pBannerToolbarWin->MakeKeepOnScreenWin();
	pBannerToolbarWin->MakeNoResizeWin();
		
	// show
	pBannerToolbarWin->Show();	
}

void CMyApplication::DoUpdateVersion()
{
	try
	{
	#if MACINTOSH
		THdl h = _GetMacResDetached('harc', 128);
		if (!h)
			return;
			
		scopekill(THdlObj, h);
	#else
		TRez rz = URez::SearchChain('harc', 128, true);
		if (rz == nil)
			return;
		
		THdl h;
		StRezLoader(rz, 'harc', 128, h);
		if (!h)
			return;
		
	#endif

		{
			void *ptr;
			StHandleLocker lock(h, ptr);
			
			CMyArchiveDecoder decoder;
			decoder.ProcessData(ptr, h->GetSize());
			
			TFSRefObj* updateRef = decoder.GetAutoLaunchRef();
			
			/*
			if (updateRef)
			{
				DisplayStandardMessage("\pNew Version Available", "\pThere is a new version of Hotline Client.\r\rClick \"Ok\" to begin the download.", 1001, 1);

				TExternalApp app = UExternalApp::LaunchApplication(updateRef);
				
				if (app)
				{
					app->TryToActivate();
					UApplication::PostMessage(msg_Quit);
					// don't delete app because it kills it.
					return;
				}
			
			}
			*/
		}
	}
	catch(...)
	{
		// error - fall through to msgbox
	}
	
	//	We did not launch the app.  Tell the user to go to the URL to download
	SMsgBox mb;
	ClearStruct(mb);
	
	Uint8 *psMessage = "\pThere is a new version of Hotline Client.  See http://www.lorbac.net/ to download.";
	
	mb.icon = 1001;
	mb.sound = 1;
	mb.title = "\pNew Version Available";
	mb.messageSize = psMessage[0];
	mb.messageData = psMessage + 1;
	mb.button1 = "\pGo!";
	mb.button2 = "\pCancel";
	
	if (MsgBox(mb) == 1)
	{
		const Uint8 *psUrl = "\phttp://gloarbline.free.fr/gloarbline/index.php?option=com_content&task=view&id=18&Itemid=28";
		UTransport::LaunchURL(psUrl + 1, psUrl[0]);
	}
}


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Utility Functions ₯₯



void CMyApplication::DisableAutoResponse()
{



// timer
/*
	Uint8 str[32];

	str[0] = mOptions.pscancelAutorespTime->GetText(str+1, sizeof(str)-1);
	*/
	Int32 ntimercancel = UText::TextToInteger(mOptions.pscancelAutorespTime+1, mOptions.pscancelAutorespTime[0]);


if (AutoResp){
		
		mOptions.bAutomaticResponse = false;	
		StFieldData data;
		GetClientUserInfo(data);
		mTpt->SendTransaction(myTran_SetClientUserInfo, data);

		
		mDisableAutoResponse->Start(ntimercancel * 1000, kOnceTimer);
		}else{
		mOptions.bAutomaticResponse = true;
		StFieldData data;
		GetClientUserInfo(data);
		mTpt->SendTransaction(myTran_SetClientUserInfo, data);
		
		mDisableAutoResponse->Stop;
		}
AutoResp = false;

	
		
}


void CMyApplication::DisplayServerName(const Uint8 *inServerName)
{
	if (!inServerName || !inServerName[0])
	{
		mServerName[0] = 0;

		mChatWin->SetTitle("\pChat");
		mTasksWin->SetTitle("\pTasks");
	#if !NEW_NEWS
		mOldNewsWin->SetTitle("\pNews");
	#endif	
	}
	else
	{
		mServerName[0] = UMemory::Copy(mServerName + 1, inServerName + 1, inServerName[0] > sizeof(mServerName) - 1 ? sizeof(mServerName) - 1 : inServerName[0]);
		
		Uint8 psTitle[256];
		psTitle[0] = UText::Format(psTitle + 1, sizeof(psTitle) - 1, "Chat (%#s)", mServerName);
		mChatWin->SetTitle(psTitle);

		psTitle[0] = UText::Format(psTitle + 1, sizeof(psTitle) - 1, "Tasks (%#s)", mServerName);
		mTasksWin->SetTitle(psTitle);

	#if !NEW_NEWS
		if (mOldNewsWin)
		{
			psTitle[0] = UText::Format(psTitle + 1, sizeof(psTitle) - 1, "News (%#s)", mServerName);
			mOldNewsWin->SetTitle(psTitle);
		}
	#endif
	}
}

void CMyApplication::HideAllWindows()
{
	if (!dynamic_cast<CWindow *>(mToolbarWin) || !dynamic_cast<CWindow *>(mToolbarWin)->IsCollapsed() || mAuxChildrenList.GetItemCount())
		return;

	CPtrList<CWindow> pChildrenList;
	if (!mAuxParentWin || !mAuxParentWin->GetChildrenList(pChildrenList))
		return;

	Uint32 i = 0;
	CWindow *pWindow;
	
	while (pChildrenList.GetNext(pWindow, i))
	{
		if (CWindow::IsValid(pWindow) && pWindow->IsVisible())
		{
			pWindow->Hide();
			mAuxChildrenList.InsertItem(0, pWindow);
		}
	}
	
	pChildrenList.Clear();
}

void CMyApplication::ShowAllWindows()
{
	if (!dynamic_cast<CWindow *>(mToolbarWin) || dynamic_cast<CWindow *>(mToolbarWin)->IsCollapsed() || !mAuxChildrenList.GetItemCount())
		return;
		
	Uint32 i = 0;
	CWindow *pWindow;
	
	while (mAuxChildrenList.GetNext(pWindow, i))
	{
		if (CWindow::IsValid(pWindow) && !pWindow->IsVisible())
		{
			if (pWindow->GetLayer() == windowLayer_Modal)
			{
				TrackMsgBoxWin(pWindow);		
				delete pWindow;
			}
			else
				pWindow->Show();
		}
	}
	
	mAuxChildrenList.Clear();
}

void CMyApplication::ShowNotConnected()
{
	if (mUsersWin)
		mUsersWin->DisableIcons();

	if (mToolbarWin)
	{	
		mToolbarWin->HideDisconnectButton();
	
		mToolbarWin->DisableNewsButton();
		mToolbarWin->DisableFilesButton();
		mToolbarWin->HideBroadcastButton();
		mToolbarWin->HideNewUserButton();
		mToolbarWin->HideOpenUserButton();
		mToolbarWin->HideOpenUserListButton();
	
		
		mToolbarWin->SetAccess();
	}

	DisplayServerName(nil);
}

void CMyApplication::ShowConnecting()
{
	if (mToolbarWin)
	{
		mToolbarWin->ShowDisconnectButton();

		mToolbarWin->DisableNewsButton();
		mToolbarWin->DisableFilesButton();
		mToolbarWin->HideBroadcastButton();
		mToolbarWin->HideNewUserButton();
		mToolbarWin->HideOpenUserButton();
		mToolbarWin->HideOpenUserListButton();
		
		mToolbarWin->SetAccess();
	}
}

void CMyApplication::ShowConnected()
{
	if (mUsersWin && mServerVers < 150)
		mUsersWin->SetAccess();
	
	if (mToolbarWin)
	{
		mToolbarWin->ShowDisconnectButton();
	
		//if (mServerVers >= 150)
			mToolbarWin->EnableNewsButton();

		if (mServerVers >= 190 || mServerVers < 150)
			mToolbarWin->EnableMsgBoardButton();

		mToolbarWin->EnableFilesButton();
		
		if (mServerVers < 150)
		{
			mToolbarWin->ShowBroadcastButton();
			mToolbarWin->ShowNewUserButton();
			mToolbarWin->ShowOpenUserButton();
		}

		mToolbarWin->SetAccess();
	}

	mConnectStartSecs = UDateTime::GetSeconds();
}

void CMyApplication::ProcessStartupPath()
{
	if (!IsConnected() || !mStartupPath || !mStartupPathSize)
		return;

	Uint8 nExtraPath;
	bool bFilesNews;
	
	if (!UText::CompareInsensitive(mStartupPath, "Files", 5))
	{	
		bFilesNews = true; 
		nExtraPath = 5;	
	}
	else if (!UText::CompareInsensitive(mStartupPath, "News", 4))
	{	
		bFilesNews = false; 
		nExtraPath = 4;	
	}
	else
	{	
		bFilesNews = true; 
		nExtraPath = 0;	
	}
		
	Uint8 bufPathData[4096];
	Uint32 nPathSize = UFileSys::ConvertUrlToPathData(mStartupPath + nExtraPath, mStartupPathSize - nExtraPath, bufPathData, sizeof(bufPathData));
	
	UMemory::Dispose(mStartupPath);
	mStartupPath = nil;
	mStartupPathSize = 0;
		
	void *pPathData = nil;
	if (nPathSize <= 2)
		nPathSize = 0;
	else
		pPathData = bufPathData;
			
	if (bFilesNews)
		new CMyStartupGetFileListTask(pPathData, nPathSize);
	else
		new CMyStartupGetNewsCatListTask(pPathData, nPathSize);
}


template <class T> bool CMyApplication::SearchWin()
{
	CWindow *win;
	Uint32 i = 0;
	
	while (gWindowList.GetNextWindow(win, i))
	{
		if (dynamic_cast<T *>(win))
			return true;
	}
	
	return false;
}

CMyItemsWin *CMyApplication::GetRootFolderWin()
{
	Uint32 i = 0;
	CMyItemsWin *pItemsWin;

	while (mFileWinList.GetNext(pItemsWin, i))
	{
		if (pItemsWin->IsEmptyPath())
			return pItemsWin;
	}
	
	return nil;
}

CMyItemsWin *CMyApplication::GetFolderWinByPath(const void *inPathData, Uint32 inPathSize, Uint32 inCheckSum)
{
	if (mOptions.nFilesWindow == optWindow_List)
		return CMyItemsListWin::FindWindowByPath(mFileWinList, inPathData, inPathSize, inCheckSum);
	else if (mOptions.nFilesWindow == optWindow_Tree)
		return CMyItemsTreeWin<SMyFileItem>::FindWindowByPath(mFileWinList, inPathData, inPathSize, inCheckSum);
	else
		return CMyItemsTreeWin<SMyFileItem>::FindWindowByPath(mFileWinList, inPathData, inPathSize, inCheckSum);
}

void CMyApplication::SetFolderWinContent(const void *inPathData, Uint32 inPathSize, TFieldData inData)
{
	if (mOptions.nFilesWindow == optWindow_List)
	{
		CMyFileListWin *pFileListWin;

		// determine which window to put the list in
		if (!inPathData)
			pFileListWin = (CMyFileListWin *)gApp->GetRootFolderWin();
		else
			pFileListWin = (CMyFileListWin *)GetFolderWinByPath(inPathData, inPathSize);
		
		// extract file listing into list view
		if (pFileListWin)
			pFileListWin->SetContent(inPathData, inPathSize, inData);
	}
	else if (mOptions.nFilesWindow == optWindow_Tree)
	{
		Uint32 i = 0;
		CMyFileTreeWin *pFileTreeWin;
		
		while (mFileWinList.GetNext((CMyItemsWin*&)pFileTreeWin, i))
			pFileTreeWin->SetContent(inPathData, inPathSize, inData, false);
			
		mCacheList.AddFileList(inPathData, inPathSize, inData);
	}
	else
	{
		Uint32 i = 0;
		CMyFileExplWin *pFileExplWin;
		
		while (mFileWinList.GetNext((CMyItemsWin*&)pFileExplWin, i))
			pFileExplWin->SetContent(inPathData, inPathSize, inData, false);
			
		mCacheList.AddFileList(inPathData, inPathSize, inData);
	}
}

CMyItemsWin *CMyApplication::GetRootNewsFolderWin()
{
	Uint32 i = 0;
	CMyItemsWin *pItemsWin;

	while (mCategoryWinList.GetNext(pItemsWin, i))
	{
		if (pItemsWin->IsEmptyPath())
			return pItemsWin;
	}	
	
	return nil;
}

CMyItemsWin *CMyApplication::GetNewsFolderWinByPath(const void *inPathData, Uint32 inPathSize, Uint32 inCheckSum)
{
	if (mOptions.nNewsWindow == optWindow_List)
		return CMyItemsListWin::FindWindowByPath(mCategoryWinList, inPathData, inPathSize, inCheckSum);
	else if (mOptions.nNewsWindow == optWindow_Tree)
		return CMyItemsTreeWin<SNewsCatItm>::FindWindowByPath(mCategoryWinList, inPathData, inPathSize, inCheckSum);
	else
		return CMyItemsTreeWin<SNewsCatItm>::FindWindowByPath(mCategoryWinList, inPathData, inPathSize, inCheckSum);
}

void CMyApplication::SetBundleWinContent(const void *inPathData, Uint32 inPathSize, TFieldData inData)
{
	if (mOptions.nNewsWindow == optWindow_List)
	{
		CMyNewsCategoryListWin *pNewsCategoryListWin;

		// determine which window to put the list in
		if (!inPathData)
			pNewsCategoryListWin = (CMyNewsCategoryListWin *)gApp->GetRootNewsFolderWin();
		else
			pNewsCategoryListWin = (CMyNewsCategoryListWin *)GetNewsFolderWinByPath(inPathData, inPathSize);
		
		// extract category listing into list view
		if (pNewsCategoryListWin)
			pNewsCategoryListWin->SetContent(inPathData, inPathSize, inData);
	}
	else if (mOptions.nNewsWindow == optWindow_Tree)
	{
		Uint32 i = 0;
		CMyNewsCategoryTreeWin *pNewsCategoryTreeWin;
		
		while (mCategoryWinList.GetNext((CMyItemsWin*&)pNewsCategoryTreeWin, i))
			pNewsCategoryTreeWin->SetContent(inPathData, inPathSize, inData, false);
			
		mCacheList.AddBundleList(inPathData, inPathSize, inData);
	}
	else
	{
		Uint32 i = 0;
		CMyNewsCategoryExplWin *pNewsCategoryExplWin;
		
		while (mCategoryWinList.GetNext((CMyItemsWin*&)pNewsCategoryExplWin, i))
			pNewsCategoryExplWin->SetContent(inPathData, inPathSize, inData, false);
			
		mCacheList.AddBundleList(inPathData, inPathSize, inData);
	}
}

CMyNewsArticleTreeWin *CMyApplication::GetNewsArticleListWinByPath(const void *inData, Uint32 inDataSize, Uint32 inCheckSum)
{	
	return (CMyNewsArticleTreeWin *)CMyItemsTreeWin<SMyNewsArticle>::FindWindowByPath(mArticleTreeWinList, inData, inDataSize, inCheckSum);	
}

void CMyApplication::SetCategoryWinContent(const void *inPathData, Uint32 inPathSize, TFieldData inData)
{
	// determine which window to put the list in
	CMyNewsArticleTreeWin *win = GetNewsArticleListWinByPath(inPathData, inPathSize);

	// extract article listing into list view
	if (win)
		win->SetContent(inPathData, inPathSize, inData, false);

	if (mOptions.nNewsWindow == optWindow_Expl)
	{
		Uint32 i = 0;
		CMyNewsCategoryExplWin *pNewsCategoryExplWin;
		
		while (mCategoryWinList.GetNext((CMyItemsWin*&)pNewsCategoryExplWin, i))
			pNewsCategoryExplWin->SetContentArticleList(inPathData, inPathSize, inData, false);			
	}
	
	mCacheList.AddArticleList(inPathData, inPathSize, inData);
}

void CMyApplication::SetArticleWinContent(const void *inPathData, Uint32 inPathSize, Uint32 inArticleID, TFieldData inData)
{
	CMyNewsArticTextWin *win = GetNewsArticleWinByPathAndID(inArticleID, inPathData, inPathSize);
			
	if (win)
		win->SetContentsFromFields(inData);

	if (mOptions.nNewsWindow == optWindow_Expl)
	{
		Uint32 i = 0;
		CMyNewsCategoryExplWin *pNewsCategoryExplWin;
		
		while (mCategoryWinList.GetNext((CMyItemsWin*&)pNewsCategoryExplWin, i))
			pNewsCategoryExplWin->SetContentArticleText(inPathData, inPathSize, inArticleID, inData);
	}
}

void CMyApplication::SetCurrentArticle(const void *inPathData, Uint32 inPathSize, Uint32 inPathChecksum, Uint32 inCurrentID, Uint32 inNewID)
{
	CMyNewsArticleTreeWin *win = GetNewsArticleListWinByPath(inPathData, inPathSize, inPathChecksum);
	
	if(win)
	{
		if (win->GetNewsArticleTreeView()->GetSelectedItemNameAndID(nil) == inCurrentID)
			win->GetNewsArticleTreeView()->SetCurrentItem(inNewID);
	}
	
	if (mOptions.nNewsWindow == optWindow_Expl)
	{
		Uint32 i = 0;
		CMyNewsCategoryExplWin *pNewsCategoryExplWin;
		
		while (mCategoryWinList.GetNext((CMyItemsWin*&)pNewsCategoryExplWin, i))
			pNewsCategoryExplWin->SetCurrentArticle(inPathData, inPathSize, inCurrentID, inNewID);
	}
}

void CMyApplication::SelectArticle(const void *inPathData, Uint32 inPathSize, Uint32 inPathChecksum, Uint32 inArticleID)
{
	CMyNewsArticleTreeWin *win = GetNewsArticleListWinByPath(inPathData, inPathSize, inPathChecksum);
	
	if(win)
		win->GetNewsArticleTreeView()->SetSelectedItem(inArticleID);

	if (mOptions.nNewsWindow == optWindow_Expl)
	{
		Uint32 i = 0;
		CMyNewsCategoryExplWin *pNewsCategoryExplWin;
		
		while (mCategoryWinList.GetNext((CMyItemsWin*&)pNewsCategoryExplWin, i))
			pNewsCategoryExplWin->SelectArticle(inPathData, inPathSize, inArticleID);
	}
}

CMyUserInfoWin *CMyApplication::GetUserInfoWinByID(Uint16 inUserID)
{
	Uint32 i = 0;
	CMyUserInfoWin *pUserInfoWin;
	
	while (mUserInfoWinList.GetNext(pUserInfoWin, i))
	{
		if (pUserInfoWin->GetUserID() == inUserID)
			return pUserInfoWin;
	}

	return nil;
}

void CMyApplication::SetUserInfoWinContent(Uint16 inUserID, const Uint8 *inUserName, TFieldData inData)
{
	CMyUserInfoWin *pUserInfoWin = GetUserInfoWinByID(inUserID);

	if (pUserInfoWin)
	{
		pUserInfoWin->SetContent(inUserID, inUserName, inData);
		pUserInfoWin->BringToFront();
		return;
	}

	pUserInfoWin = new CMyUserInfoWin(mAuxParentWin);
	if (mWindowPat) pUserInfoWin->SetPattern(mWindowPat);

	pUserInfoWin->SetAccess();
	pUserInfoWin->SetContent(inUserID, inUserName, inData);
	
	pUserInfoWin->SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);
	pUserInfoWin->Show();
}

void CMyApplication::SetOnlineUsers(TFieldData inData)
{
	mUsersWin->GetUserListView()->DeleteAll();
	mUsersWin->GetUserListView()->AddListFromFields(inData);
}

void CMyApplication::GetClientUserInfo(TFieldData outData)
{
	Uint16 nOptions =
		((mOptions.bRefusePrivateMsg)		? 0x01 : 0)	|
		((mOptions.bRefusePrivateChat)		? 0x02 : 0)	|
		((mOptions.bAutomaticResponse)		? 0x04 : 0)	;

	outData->DeleteAllFields();

	// setup field data
	outData->AddPString(myField_UserName, mUserName);
	outData->AddInteger(myField_UserIconID, mIconID);
	outData->AddInteger(myField_Options, nOptions);

	if (mOptions.bAutomaticResponse)
		outData->AddPString(myField_AutomaticResponse, mOptions.psAutomaticResponse);		
}

void CMyApplication::CheckServerError(Uint32 inError, TFieldData inData)
{
	Uint8 psMsg[256];
	inData->GetPString(myField_ErrorText, psMsg);
	
	if (psMsg[0])
		UApplication::PostMessage(1100, psMsg + 1, psMsg[0], priority_High);
	
	if (inError)
	{
		// just in case need to process other tasks
		UApplication::PostMessage(msg_DataArrived);
		
		if (psMsg[0])
			SilentFail(errorType_Misc, error_Unknown);
		else
			Fail(errorType_Misc, error_Unknown);
	}
}

void CMyApplication::GetTransferStatText(Uint32 inXferSize, Uint32 inTotalSize, Uint32 inBPS, Uint32 inETR, Uint8 *outText)
{
	Uint8 rcvdSizeStr[32];
	Uint8 totalSizeStr[32];
	Uint8 rateStr[32];
	Uint8 etrStr[32];
	
	rcvdSizeStr[0] = UText::SizeToText(inXferSize, rcvdSizeStr+1, sizeof(rcvdSizeStr)-1);
	totalSizeStr[0] = UText::SizeToText(inTotalSize, totalSizeStr+1, sizeof(totalSizeStr)-1);
	rateStr[0] = UText::SizeToText(inBPS, rateStr+1, sizeof(rateStr)-1, kSmallKilobyteFrac);
	etrStr[0] = UText::SecondsToText(inETR, etrStr+1, sizeof(etrStr)-1);
	
	outText[0] = UText::Format(outText+1, 255, "%#s/sec %#s/%#s %#s", rateStr, rcvdSizeStr, totalSizeStr, etrStr);
}

void CMyApplication::ChatScram(void *ioData, Uint32 inDataSize)
{
	Uint8 *curQuote, *nextQuote, *p, *ep;
	
	p = (Uint8 *)ioData;
	ep = p + inDataSize;

	while (p < ep)
	{
		curQuote = (Uint8 *)UMemory::SearchByte('\"', p, ep - p);
		
		if (curQuote == nil)
		{
			SubsData(mScramTable, p, ep - p);
			break;
		}
		
		SubsData(mScramTable, p, curQuote - p);

		nextQuote = (Uint8 *)UMemory::SearchByte('\"', curQuote + 1, ep - curQuote - 1);
	
		if (nextQuote == nil) break;
		p = nextQuote + 1;
	}
}

void CMyApplication::ChatUnscram(void *ioData, Uint32 inDataSize)
{
	Uint8 *curQuote, *nextQuote, *p, *ep;
	
	p = (Uint8 *)ioData;
	ep = p + inDataSize;

	while (p < ep)
	{
		curQuote = (Uint8 *)UMemory::SearchByte('\"', p, ep - p);
		
		if (curQuote == nil)
		{
			SubsData(mUnscramTable, p, ep - p);
			break;
		}
		
		SubsData(mUnscramTable, p, curQuote - p);

		nextQuote = (Uint8 *)UMemory::SearchByte('\"', curQuote + 1, ep - curQuote - 1);
	
		if (nextQuote == nil) break;
		p = nextQuote + 1;
	}
}


void CMyApplication::ShowAdminWin(TFieldData inData)
{
	if (mAdminWin)
		return;

	mAdminWin = new CMyAdminWin(mAuxParentWin);
	if (!mAdminWin->AddListFromFields(inData))
	{
		delete mAdminWin;
		mAdminWin = nil;
		
		DisplayStandardMessage("\pServer Message", "\pCannot read account list.", icon_Caution, 1);
		return;
	}
	
	// set bounds
	SMyAdminBoundsInfo stBoundsInfo;
	stBoundsInfo.stAdminBounds = mOptions.stWinRect.stAdmin;
	stBoundsInfo.nAdminPane = mOptions.stWinPane.nAdminPane1;
	stBoundsInfo.nAdminTab1 = mOptions.stWinTabs.nAdminTab1;
	stBoundsInfo.nAdminTab2 = mOptions.stWinTabs.nAdminTab2;

	mAdminWin->SetBoundsInfo(stBoundsInfo);
	DebugBreak("et on est ou?");
	mAdminWin->Show();	
}

void CMyApplication::ShowAdmInSpectorWin(TFieldData inData)
{
	if (mAdmInSpectorWin)
		return;

	mAdmInSpectorWin = new CMyAdmInSpectorWin(mAuxParentWin);
	
	//Si on peut pas avoir la liste 
	if (!mAdmInSpectorWin->AddListFromFields(inData))
	{
		delete mAdmInSpectorWin;
		mAdmInSpectorWin = nil;
		
		DisplayStandardMessage("\pServer Message", "\pCannot read user list.", icon_Caution, 1);
		return;
	}
	
	// set bounds
	
	SMyAdmInSpectorBoundsInfo stBoundsInfo;
	stBoundsInfo.stAdmInSpectorBounds = mOptions.stWinRect.stAdmInSpector;
	stBoundsInfo.nAdmInSpectorPane = mOptions.stWinPane.nAdmInSpectorPane1;
	stBoundsInfo.nAdmInSpectorTab1 = mOptions.stWinTabs.nAdmInSpectorTab1;
	stBoundsInfo.nAdmInSpectorTab2 = mOptions.stWinTabs.nAdmInSpectorTab2;

	mAdmInSpectorWin->SetBoundsInfo(stBoundsInfo);
	DebugBreak("et on est ou?");
	mAdmInSpectorWin->Show();	
}




CMyEditUserWin *CMyApplication::ShowEditUserWin(TFieldData inData)
{
	CMyEditUserWin *win = new CMyEditUserWin(mAuxParentWin, false);
	Uint8 name[64];
	Uint8 login[64];
	Uint8 pass[64];
	SMyUserAccess acc;
	Uint8 *p;
	Uint32 s;
	
	try
	{
		inData->GetPString(myField_UserName, name, sizeof(name));
		inData->GetPString(myField_UserLogin, login, sizeof(login));
		inData->GetPString(myField_UserPassword, pass, sizeof(pass));
		
		// unscramble login
		s = login[0];
		p = login + 1;
		while (s--) { *p = ~(*p); p++; }

		ClearStruct(acc);
		inData->GetField(myField_UserAccess, &acc, sizeof(acc));
		
		win->SetInfo(name, login, nil);
		if (pass[0]) win->SetDummyPassword();
		win->SetAccess(acc);
		win->SetNewUser(false);
		win->SetLoginEditable(false);
		
		win->Show();
	}
	catch(...)
	{
		delete win;
		throw;
	}
	
	return win;
}

CMyPrivateChatWin *CMyApplication::GetChatWinByID(Uint32 inID)
{
	CLink *link = CMyPrivateChatWin::gList.GetFirst();
	
	while (link)
	{
		if (((CMyPrivateChatWin *)link)->GetChatID() == inID)
			return (CMyPrivateChatWin *)link;
			
		link = link->GetNext();
	}
	
	return nil;
}

void CMyApplication::GetFilesBoundsInfo()
{
	CMyItemsWin *pItemsWin = GetRootFolderWin();
	if (!pItemsWin)
		pItemsWin = mFileWinList.GetItem(1);

	if (pItemsWin)
		pItemsWin->GetBoundsInfo();
}

void CMyApplication::GetNewsBoundsInfo()
{
	CMyItemsWin *pItemsWin = GetRootNewsFolderWin();
	if (!pItemsWin)
		pItemsWin = mCategoryWinList.GetItem(1);

	if (pItemsWin)
		pItemsWin->GetBoundsInfo();
}

void CMyApplication::GetArticlesBoundsInfo()
{
	CMyItemsWin *pItemsWin = mArticleTreeWinList.GetItem(1);

	if (pItemsWin)
		pItemsWin->GetBoundsInfo();
}

void CMyApplication::CloseMarkedWin()
{
	bool bStartTimer = false;

	Uint32 i = 0;
	CMyViewFileWin *pFileViewWin;
	
	while (mViewWinList.GetNext(pFileViewWin, i))
	{
		if (pFileViewWin->GetMustCloseFlag())
		{
			if (pFileViewWin->CanCloseViewFileWin())
			{
				delete pFileViewWin;
				i--;
			}
			else
				bStartTimer = true;
		}
	}
	
	if (bStartTimer)
		mCloseWinTimer->Start(500, kOnceTimer);
}

void CMyApplication::ClearStartupInfo()
{
	if (mStartupPath)
	{
		UMemory::Dispose(mStartupPath);
		mStartupPath = nil;
	}
	
	mStartupPathSize = 0;
	
	if (mResumableFile)
	{
		UFileSys::Dispose(mResumableFile);
		mResumableFile = nil;
	}
}

void CMyApplication::ClearOptions()
{
	Uint32 i = 0;
	SMyDefTrackerInfo *pTrackerInfo;
		
	while (mOptions.stTrackerList.GetNext(pTrackerInfo, i))
		UMemory::Dispose((TPtr)pTrackerInfo);
		
	mOptions.stTrackerList.Clear();
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -


void CMyApplication::ClearServerBanner()
{
	if (mServerBanner)
	{
		UMemory::Dispose(mServerBanner);
		mServerBanner = nil;
	}
			
	mServerBannerType = 0;
	mServerBannerURL[0] = 0;
	mServerBannerComment[0] = 0;
	mServerBannerCount = 0;
}

void CMyApplication::SetServerBanner(const void *inData, Uint32 inDataSize, Uint32 inTypeCode, const Uint8 *inBannerHref, const Uint8 *inBannerComment)
{
	ClearServerBanner();

	mServerBanner = UMemory::NewHandle(inData, inDataSize);
	mServerBannerType = inTypeCode;
	//DebugBreak("%s",inTypeCode);
	if (inBannerHref && inBannerHref[0])
		UMemory::Copy(mServerBannerURL, inBannerHref, inBannerHref[0] + 1);
	
	if (inBannerComment && inBannerComment[0])
		UMemory::Copy(mServerBannerComment, inBannerComment, inBannerComment[0] + 1);
		
}


bool CMyApplication::GetAgreement(TFieldData inData, Uint16 inField_Data, THdl& outAgreement)
{
	Uint32 nSize = inData->GetFieldSize(inField_Data);

	if(nSize == 0)
	{
		if(outAgreement)
		{
			UMemory::Dispose(outAgreement);
			outAgreement = nil;
		}
		
		return false;
	}
	
	if (!outAgreement)
		outAgreement = UMemory::NewHandle(nSize);
	else
		UMemory::Reallocate(outAgreement, nSize);

	try
	{
		void *pAgreement = UMemory::Lock(outAgreement);
		inData->GetField(inField_Data, pAgreement, nSize);
		UMemory::Unlock(outAgreement);
	}
	catch(...)
	{
		UMemory::Dispose(outAgreement);
		outAgreement = nil;
		
		throw;
	}
	
	return true;
}

bool CMyApplication::GetResourceBanner(bool inQuickTimeBanner)
{
	if (!dynamic_cast<CMyBannerToolbarWin *>(mToolbarWin) || !mUserQuitTimer)
		return false;
    // we do not allow quick time banners right now
    if (inQuickTimeBanner)
    	return false;
    	
	Uint32 nType;
	Uint32 nRealType;
	Uint8 *psUrl;

	if (inQuickTimeBanner 
		&& UOperatingSystem::IsQuickTimeAvailable() 
		&& UOperatingSystem::CanHandleFlash())
	{
		nType = 'MooV';
		nRealType = 'MooV';
		psUrl = "\p";
	}
	else
	{
		nType = 'GIFf';
		nRealType = 'GIFf';
		//psUrl = "\phttp://www.bigredh.com/hotline2/1.8/HotAubahn";
			psUrl = "\p";
	}
				
#if MACINTOSH
	THdl hBanner = _GetMacResDetached(nType, 128);
	if (!hBanner)
		return false;
		
	scopekill(THdlObj, hBanner);
#else
	TRez rz = URez::SearchChain(nType, 128, true);
	if (rz == nil)
		return false;
	
	THdl hBanner;
	StRezLoader(rz, nType, 128, hBanner);
	if (!hBanner)
		return false;	
#endif
	
	return dynamic_cast<CMyBannerToolbarWin *>(mToolbarWin)->SetBannerHdl(hBanner, TB((Uint32)nRealType), psUrl);
}



void CMyApplication::SetupHttpTransaction( THttpTransact inHttpTransact, 
										   Uint8 *inReferer, Uint8 *inCustomField, 
										   bool inSetHttpIDList)
{
	inHttpTransact->SetMessageHandler(MessageHandler, gApp);
		
	Uint8 psUserAgent[128];
	psUserAgent[0] = UText::Format(psUserAgent + 1, sizeof(psUserAgent) - 1
									, "Hotline/%s", kClientVersion);
	
	if (UOperatingSystem::IsQuickTimeAvailable()) 
		psUserAgent[0] += UText::Format(psUserAgent + psUserAgent[0] + 1
									, sizeof(psUserAgent) - psUserAgent[0] - 1
									, " with QuickTime %#s"
									, UOperatingSystem::GetQuickTimeVersion());
	
	psUserAgent[0] += UText::Format(  psUserAgent + psUserAgent[0] + 1
									, sizeof(psUserAgent) - psUserAgent[0] - 1
									, " (%#s)", UOperatingSystem::GetSystemVersion());
	inHttpTransact->SetUserAgentServer(psUserAgent);

	if (inReferer && inReferer[0])
		inHttpTransact->SetReferer(inReferer);

	if (inCustomField && inCustomField[0])
		inHttpTransact->SetCustomField(inCustomField);

	if (inSetHttpIDList)
		inHttpTransact->SetHttpIDList(mHttpIDList.GetIDList());
}

void CMyApplication::ShowStandardMessage(TFieldData inData, Uint8 *inTitle, bool inSound)
{
	if (!inData || !inTitle || !inTitle[0])
		return;
	
	Uint8 bufMsgData[256 + 4096];
	UMemory::Copy(bufMsgData, inTitle, inTitle[0] + 1);
	Uint32 nMsgSize = inData->GetField(myField_Data, bufMsgData + bufMsgData[0] + 1, 4096);
	if (!nMsgSize)
		return;
	
	nMsgSize += bufMsgData[0] + 1;
	if (inSound)
		UApplication::PostMessage(1130, bufMsgData, nMsgSize);
	else
		UApplication::PostMessage(1131, bufMsgData, nMsgSize);
}

void CMyApplication::ShowAdminSpector(TFieldData inData)
{


//DoOptions();
DoAdmInSpector();
//DoSecret();
	//CMyAdminSpectorWin *win = new CMyAdminSpectorWin(mAuxParentWin);
	
}
bool CMyApplication::AutomaticResponseOrRefuseMsg(Uint16 inUserID, bool inSendResponse)
{
	// check access
	if (mServerVers >= 182 && !HasGeneralPriv(myAcc_SendMessage))
		return false;

	// if is an refuse message or automatic response don't send an automatic answer
	if (mOptions.bAutomaticResponse)
	{
		if (inSendResponse)
			new CMySendPrivMsgTask(inUserID, mOptions.psAutomaticResponse + 1, mOptions.psAutomaticResponse[0], myOpt_AutomaticResponse);
		
		if (mOptions.bRefusePrivateMsg)
			return true;
		
		return false;
	}
	
	if (mOptions.bRefusePrivateMsg)
	{
		if (inSendResponse)
			new CMySendPrivMsgTask(inUserID, nil, 0, myOpt_RefuseMessage);
		
		return true;
	}
	
	return false;
}

bool CMyApplication::AutomaticResponseOrRefuseChat(Uint16 inUserID)
{
	// check access
	if (mServerVers >= 182 && !HasGeneralPriv(myAcc_SendMessage))
		return false;

	if (mOptions.bAutomaticResponse)
	{
		new CMySendPrivMsgTask(inUserID, mOptions.psAutomaticResponse + 1, mOptions.psAutomaticResponse[0], myOpt_AutomaticResponse);
		
		if (mOptions.bRefusePrivateChat)
			return true;
		
		return false;
	}
	
	if (mOptions.bRefusePrivateChat)
	{
		new CMySendPrivMsgTask(inUserID, nil, 0, myOpt_RefuseChat);
		
		return true;
	}

	return false;
}

void CMyApplication::SetChatColor(SMyColorInfo& colorPublicChat, SMyColorInfo& colorPrivateChat)
{
	// bring settings into range
	if (colorPublicChat.textSize < 4)
		colorPublicChat.textSize = 4;
	else if (colorPublicChat.textSize > 50)
		colorPublicChat.textSize = 50;
	
	// store settings
	mOptions.ColorPublicChat = colorPublicChat;
	
	// set main chat win to new settings
	mChatWin->SetTextColor(mOptions.ColorPublicChat);
	
	// bring settings into range
	if (colorPrivateChat.textSize < 4)
		colorPrivateChat.textSize = 4;
	else if (colorPrivateChat.textSize > 50)
		colorPrivateChat.textSize = 50;

	// store settings
	mOptions.ColorPrivateChat = colorPrivateChat;
	
	// set all private chat windows to new settings
	CLink *link = CMyPrivateChatWin::gList.GetFirst();
	while (link)
	{
		((CMyPrivateChatWin *)link)->SetTextColor(colorPrivateChat);
		link = link->GetNext();
	}
}

void CMyApplication::SetNewsColor(SMyColorInfo& colorNews)
{
	// bring settings into range
	if (colorNews.textSize < 4)
		colorNews.textSize = 4;
	else if (colorNews.textSize > 50)
		colorNews.textSize = 50;
	
	// store settings
	mOptions.ColorNews = colorNews;
	
	// set news win to new settings
	if(mOldNewsWin)
		mOldNewsWin->SetTextColor(mOptions.ColorNews);	
		
	Uint32 i = 0;
	CMyNewsArticTextWin *win;
		
	while(mNewsArticTxtWinList.GetNext(win, i))
		win->SetTextColor(colorNews);
		
	if (mOptions.nNewsWindow == optWindow_Expl)
	{
		i = 0;
		CMyNewsCategoryExplWin *pNewsCategoryExplWin;
		
		while (mCategoryWinList.GetNext((CMyItemsWin*&)pNewsCategoryExplWin, i))
			pNewsCategoryExplWin->SetTextColor(colorNews);
	}
}

void CMyApplication::SetViewTextColor(SMyColorInfo& colorViewText)
{
	// bring settings into range
	if (colorViewText.textSize < 4)
		colorViewText.textSize = 4;
	else if (colorViewText.textSize > 50)
		colorViewText.textSize = 50;
	
	// store settings
	mOptions.ColorViewText = colorViewText;
	
	Uint32 i = 0;
	CMyViewFileWin *win;
	
	while (mViewWinList.GetNext(win, i))
		win->SetTextColor(colorViewText);
}

void CMyApplication::AddChatText(CMyPrivateChatWin *inWin, const void *inText, Uint32 inTextSize, bool inPlaySound)
{
	ASSERT(inWin && inText);
	
	if (inTextSize)
	{
		CTextView *v = inWin->GetDisplayTextView();
		
		// stop blank line at top of chat window
		if (v->IsEmpty() && *(Uint8 *)inText == '\r')
		{
			((Uint8 *)inText)++;
			inTextSize--;
			if (inTextSize == 0) return;
		}
		
		v->InsertText(max_Uint32, inText, inTextSize);
				
		if (!inWin->IsChatTextActive())
			inWin->ScrollToBottom();

		if (inPlaySound && mOptions.SoundPrefs.chat) USound::Play(nil,
		#if WIN32
		129
		#else
		139
		#endif
		, false);
	}
}

// returns true if inRef was modified to point to a resumable file
bool CMyApplication::FindResumableFile(TFSRefObj* inRef, bool inIsFolder)
{
	TFSRefObj* fp = inRef->Clone();
	scopekill(TFSRefObj, fp);
	
	Uint8 psFileName[256];
	fp->GetName(psFileName);

#if WIN32
	if (!inIsFolder)
		pstrcat(psFileName, "\p.hpf");
#endif

	if (mResumableFile)
	{
		Uint8 psResemableName[256];
		mResumableFile->GetName(psResemableName);
				
		bool bFound = false;
		if (!UMemory::Compare(psFileName + 1, psFileName[0], psResemableName + 1, psResemableName[0]))
		{
			inRef->SetRef(mResumableFile);
			bFound = true;
		}
		
		delete mResumableFile;
		mResumableFile = nil;
		
		if (bFound)
			return true;
	}
	
	if (inIsFolder)
	{	
		if (fp->Exists())
			return true;		
	}
	else
	{
	#if WIN32
		fp->SetRefName(psFileName);

		if (fp->Exists())
		{
			inRef->SetRef(fp);
			return true;
		}	
	#else
		if (fp->Exists())
		{
			Uint32 typeCode, creatorCode;
			inRef->GetTypeAndCreatorCode(typeCode, creatorCode);
			return (creatorCode == TB((Uint32)'HTLC') && typeCode == TB((Uint32)'HTft'));
		}
	#endif
	}
	
	return false;
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Files and Prefs ₯₯

void CMyApplication::ReadServerFile(TFSRefObj* inFile, Uint8 *outAddress, Uint8 *outLogin, Uint8 *outPassword, bool *outUseCrypt)
{
	if (!inFile)
		return;
	
	SMyServerConfig stData;
	StFileOpener fop(inFile, perm_Read);
		
	if (inFile->Read(0, &stData, 6) != 6)
		Fail(errorType_Misc, error_Corrupt);

	if (stData.format != TB((Uint32)'HTsc')) 
		Fail(errorType_Misc, error_FormatUnknown);
	
	if (stData.version == TB((Uint16)1))
	{
		SMyServerConfig conf;
		
		if (inFile->Read(0, &conf, sizeof(conf)) != sizeof(conf))
			Fail(errorType_Misc, error_Corrupt);
	
		if (outAddress)
		{
			outAddress[0] = FB(conf.addressSize);
			UMemory::Copy(outAddress + 1, conf.addressData, outAddress[0]);
		}
	
		if (outLogin)
		{
			outLogin[0] = FB(conf.loginSize);
			UMemory::Copy(outLogin + 1, conf.loginData, outLogin[0]);
		}

		if (outPassword)
		{
			outPassword[0] = FB(conf.passwordSize);
			UMemory::Copy(outPassword + 1, conf.passwordData, outPassword[0]);
		}
		if(outUseCrypt)
			*outUseCrypt = conf.useCrypt;
	}
}

void CMyApplication::SaveServerFile(TFSRefObj* inFile, const Uint8 inAddress[], const Uint8 inLogin[], const Uint8 inPassword[], bool inUseCrypt)
{
	SMyServerConfig stData;
	Uint16 ws;
	
	ClearStruct(stData);
	
	stData.format = TB((Uint32)'HTsc');
	stData.version = TB((Uint16)1);
	
	ws = inAddress[0];
	if (ws > sizeof(stData.addressData)) ws = sizeof(stData.addressData);
	stData.addressSize = TB(ws);
	UMemory::Copy(stData.addressData, inAddress+1, ws);

	ws = inLogin[0];
	if (ws > sizeof(stData.loginData)) ws = sizeof(stData.loginData);
	stData.loginSize = TB(ws);
	UMemory::Copy(stData.loginData, inLogin+1, ws);
	
	ws = inPassword[0];
	if (ws > sizeof(stData.passwordData)) ws = sizeof(stData.passwordData);
	stData.passwordSize = TB(ws);
	UMemory::Copy(stData.passwordData, inPassword+1, ws);
	
	stData.useCrypt=inUseCrypt;
	
	inFile->DeleteFile();
	inFile->CreateFile('HTbm', 'HTLC');
	
	try
	{
		StFileOpener fop(inFile, perm_ReadWrite);
		inFile->SetSize(sizeof(stData));
		inFile->Write(0, &stData, sizeof(stData));
	}
	catch(...)
	{
		inFile->DeleteFile();
		throw;
	}
}

TFSRefObj* CMyApplication::GetDataRef()
{
	const Uint8 kMyDataFolderName[] = "\pData";
	TFSRefObj* fp = UFS::New(kProgramFolder, nil, kMyDataFolderName);
	
	if (!fp->Exists())
		fp->CreateFolder();
	
	return fp;
}

void CMyApplication::LoadRezFile()
{
#if WIN32
		try { URez::AddProgramFileToSearchChain("\pData//hlc19.dat"); } catch(...){ }
//	try { URez::AddProgramFileToSearchChain("\pData//hotline.dat"); } catch(...){ }
//	try { URez::AddProgramFileToSearchChain("\pData//soundset.dat"); } catch(...) { }
//	try { URez::AddProgramFileToSearchChain("\pData//usericons.dat"); } catch(...) { }
#endif
}

TFSRefObj* CMyApplication::GetNewsHistRef()
{
	TFSRefObj* fpData = GetDataRef();
	scopekill(TFSRefObj, fpData);

	const Uint8 kMyNewsFileName[] = "\pNews History";
	TFSRefObj* fp = UFS::New(fpData, nil, kMyNewsFileName);
	
	if (!fp->Exists())
	{	
		TFSRefObj* fpOld = UFS::New(kProgramFolder, nil, kMyNewsFileName);
		scopekill(TFSRefObj, fpOld);
		
		if (fpOld->Exists())
			fpOld->Move(fpData);	
	}
	
	return fp;
}

TFSRefObj* CMyApplication::GetPrefsRef()
{
	const Uint8 kMyPrefsFileName[] = "\pGLoarbLine Prefs";

	TFSRefObj* fpData = GetDataRef();
	scopekill(TFSRefObj, fpData);

//	TFSRefObj* fpOld = UFS::New(kProgramFolder, nil, kMyPrefsFileName);
//	scopekill(TFSRefObj, fpOld);
	
	TFSRefObj* fp = UFS::New(fpData, nil, kMyPrefsFileName);

//	if (fpOld->Exists())
//	{
//		if (fp->Exists())
//			fp->DeleteFile();
//		fpOld->Move(fpData);
//	}
	
//	#if MACINTOSH
//	if (!fp->Exists())
//	{	// there is no prefs file in the Hotline Folder - return the ref to the prefs fldr
//		delete fp;
//		fp = UFS::New(kPrefsFolder, nil, kMyPrefsFileName);
//	}
//	#endif

	return fp;
}

void CMyApplication::WritePrefs()
{
	TFSRefObj* fp;

	SMyPrefs info;
	ClearStruct(info);

	// get files and news rects
	GetFilesBoundsInfo();
	GetArticlesBoundsInfo();
	GetNewsBoundsInfo();
	
	info.winAdmin.admin = mOptions.stWinRect.stAdmin;
	info.winAdmInSpector.adminspector = mOptions.stWinRect.stAdmInSpector;
	info.winRect.files1 = mOptions.stWinRect.stFiles1;
	info.winRect.files2 = mOptions.stWinRect.stFiles2;
	info.winRect.news1 = mOptions.stWinRect.stNews1;
	info.winRect.news2 = mOptions.stWinRect.stNews2;
	info.winRect.articles = mOptions.stWinRect.stArticles;

	info.winAdmin.adminPane1 = mOptions.stWinPane.nAdminPane1;
	info.winAdmInSpector.adminspectorPane1 = mOptions.stWinPane.nAdmInSpectorPane1;
	info.winPane.filesPane1 = mOptions.stWinPane.nFilesPane1;
	info.winPane.newsPane1 = mOptions.stWinPane.nNewsPane1;
	info.winPane.newsPane2 = mOptions.stWinPane.nNewsPane2;

	info.winAdmin.adminTab1 = mOptions.stWinTabs.nAdminTab1;
	info.winAdmInSpector.adminspectorTab1 = mOptions.stWinTabs.nAdmInSpectorTab1;
	info.winAdmin.adminTab2 = mOptions.stWinTabs.nAdminTab2;
	info.winAdmInSpector.adminspectorTab2 = mOptions.stWinTabs.nAdmInSpectorTab2;

	info.winTabs.filesTab1 = mOptions.stWinTabs.nFilesTab1;
	info.winTabs.filesTab2 = mOptions.stWinTabs.nFilesTab2;
	info.winTabs.filesTab3 = mOptions.stWinTabs.nFilesTab3;
	info.winTabs.filesTab4 = mOptions.stWinTabs.nFilesTab4;

	info.winTabs.newsTab1 = mOptions.stWinTabs.nNewsTab1;
	info.winTabs.newsTab2 = mOptions.stWinTabs.nNewsTab2;

	info.winTabs.articlesTab1 = mOptions.stWinTabs.nArticlesTab1;
	info.winTabs.articlesTab2 = mOptions.stWinTabs.nArticlesTab2;
	info.winTabs.articlesTab3 = mOptions.stWinTabs.nArticlesTab3;
	info.winTabs.articlesTab4 = mOptions.stWinTabs.nArticlesTab4;
	info.winTabs.articlesTab5 = mOptions.stWinTabs.nArticlesTab5;

	// get window rects and visibility
	if (dynamic_cast<CWindow *>(mToolbarWin))
	{
		info.winVis.toolbar = (dynamic_cast<CWindow *>(mToolbarWin)->IsVisible() != 0);
		dynamic_cast<CWindow *>(mToolbarWin)->GetBounds(info.winRect.toolbar);
	}

	if (mTasksWin)
	{
		info.winVis.tasks = (mTasksWin->IsVisible() != 0);
		mTasksWin->GetBounds(info.winRect.tasks);
	}

#if !NEW_NEWS
	if (mNewsWin)
	{
		info.winVis.news = (mNewsWin->IsVisible() != 0);
		mNewsWin->GetBounds(info.winRect.news);
	}
#endif
	
	if (mChatWin)
	{
		info.winVis.chat = (mChatWin->IsVisible() != 0);
		mChatWin->GetBounds(info.winRect.chat);
	}
	
	if (mUsersWin)
	{
		info.winVis.users = (mUsersWin->IsVisible() != 0);
		mUsersWin->GetBounds(info.winRect.users);
		mUsersWin->GetUserListView()->GetTabs(info.winTabs.usersTab1, info.winTabs.usersTab2);
	}
	
	if (mTracker)
		mTracker->GetTabs(info.winTabs.trackerTab1, info.winTabs.trackerTab2);



//info.fampastelIconset = (mOptions.bFampastelIconSet != 0);
//info.originalIconset = (mOptions.bOriginalIconSet != 0);
	
	info.iconID = ~mIconID;
	info.iconRand1 = UMath::GetRandom();
	info.iconRand2 = UMath::GetRandom();
	info.iconCRC = UMemory::CRC(&info.iconID, 8);

	info.version = kMyPrefsVersion;
	UMemory::Copy(info.userName, mUserName, mUserName[0]+1);
	info.totalUploaded = mTotalBytesUploaded;
	info.totalDownloaded = mTotalBytesDownloaded;
	info.totalOnline = mTotalSecsOnline;
	info.totalChat = mTotalChatBytes;
	info.showPrivMsgAtBack = (mOptions.bShowPrivMsgAtBack != 0);
	info.queueTransfers = (mOptions.bQueueTransfers != 0);
	info.showJoinLeave = (mOptions.bShowJoinLeaveInChat != 0);
	info.showDateTime = (mOptions.bShowDateTime != 0);
	
	info.browseFolders = (mOptions.bBrowseFolders != 0); 
	info.toolbarButtonsTop = (mOptions.bToolbarButtonsTop != 0); 
	info.magneticWindows = (mOptions.bMagneticWindows != 0); 
	info.refusePrivateMsg = (mOptions.bRefusePrivateMsg != 0); 
	info.refusePrivateChat = (mOptions.bRefusePrivateChat != 0); 
	info.isAutomaticResponse = (mOptions.bAutomaticResponse != 0); 
	
	info.refuseBroadcast = (mOptions.bRefuseBroadcast != 0);
	info.autoAgree = (mOptions.bAutoAgree != 0);
	info.showSplash = (mOptions.bShowSplash != 0);
	info.showBanner = (mOptions.bShowBanner != 0);
	
	info.fampastelIconset = (mOptions.bFampastelIconSet != 0);
	info.originalIconset = (mOptions.bOriginalIconSet != 0);
	
	info.logMessages = (mOptions.bLogMessages != false); 
    mChatLog.Enable(mOptions.bLogMessages);
    
	UMemory::Copy(info.automaticResponse, mOptions.psAutomaticResponse, mOptions.psAutomaticResponse[0] + 1);
	UMemory::Copy(info.cancelAutorespTime, mOptions.pscancelAutorespTime, mOptions.pscancelAutorespTime[0] + 1);

	info.colorPublicChat = mOptions.ColorPublicChat;
	info.colorPrivateChat = mOptions.ColorPrivateChat;
	info.colorNews = mOptions.ColorNews;
	info.colorViewText = mOptions.ColorViewText;
	
	info.firewall[0] = mFirewallAddress.name[0];
	if (info.firewall[0] > sizeof(info.firewall)-1) info.firewall[0] = sizeof(info.firewall)  -1;
	UMemory::Copy(info.firewall+1, mFirewallAddress.name + 1, info.firewall[0]);
	
	UMemory::Copy(info.unlockName, mUnlockName, mUnlockName[0] + 1);
	UMemory::Copy(info.unlockCode, mUnlockCode, mUnlockCode[0] + 1);

	mOptions.SoundPrefs.playSounds = (USound::IsCanPlay() != 0);
	info.sound = mOptions.SoundPrefs;

	info.showTooltips = (mOptions.bShowTooltips != false);
	info.tunnelThroughHttp = (mOptions.bTunnelThroughHttp != false);
	info.dontSaveCookies = (mOptions.bDontSaveCookies != false);

	info.filesWindow = mOptions.nFilesWindow; 
	info.newsWindow = mOptions.nNewsWindow; 

	fp = GetPrefsRef();
	scopekill(TFSRefObj, fp);
	if (!fp->Exists()) fp->CreateFile('pref', 'HTLC');
	
	fp->Open(perm_ReadWrite);
	try
	{
		// write prefs
		fp->SetSize(sizeof(info));
		fp->Write(0, &info, sizeof(info));
		
		try
		{
			Uint32 nOffset = sizeof(info);

			// write tracker prefs
			if (mTracker)
				nOffset += mTracker->WritePrefs(fp, nOffset);
			
			// write ID list
			mHttpIDList.WriteData(fp, nOffset);
		}
		catch(...)
		{
			// don't throw
		}
	}
	catch(...)
	{
		fp->Close();
		fp->DeleteFile();
		throw;
	}
}

void CMyApplication::ReadPrefs(SMyPrefs *outPrefs)
{
	TFSRefObj* fp = nil;
	SMyPrefs info;
	
	try
	{
		fp = GetPrefsRef();
		
		// read prefs
		fp->Open(perm_Read);
		if (fp->Read(0, &info, sizeof(info)) != sizeof(info))
			Fail(errorType_Misc, error_Unknown);
				
		try
		{
			Uint32 nOffset = sizeof(info);
			
			// read tracker prefs
			if (mTracker)
				nOffset += mTracker->ReadPrefs(fp, nOffset, info.winTabs.trackerTab1, info.winTabs.trackerTab2, false);

			try
			{
				// read ID list
				mHttpIDList.ReadData(fp, nOffset);
			}
			catch(...)
			{
				// don't throw
			}
		}
		catch(...)
		{
			// don't throw
			if (mTracker)
				mTracker->SetDefaultTracker();
		}

		delete fp;
		fp = nil;
		
		if (outPrefs) 
			UMemory::Copy(outPrefs, &info, sizeof(info));
		
		// extract misc prefs
		if (info.version != kMyPrefsVersion)
			Fail(errorType_Misc, error_Unknown);
		
		UMemory::Copy(&mUserName, info.userName, info.userName[0]+1);
		mTotalBytesUploaded = info.totalUploaded;
		mTotalBytesDownloaded = info.totalDownloaded;
		mTotalSecsOnline = info.totalOnline;
		mTotalChatBytes = info.totalChat;
		mOptions.bShowPrivMsgAtBack = (info.showPrivMsgAtBack != 0);
		mOptions.bQueueTransfers = (info.queueTransfers != 0);
		mOptions.bShowJoinLeaveInChat = (info.showJoinLeave != 0);
		mOptions.bShowDateTime = (info.showDateTime != 0);
		
		mOptions.bBrowseFolders = ( info.browseFolders != 0); 
		mOptions.bToolbarButtonsTop = ( info.toolbarButtonsTop != 0); 
		mOptions.bMagneticWindows = ( info.magneticWindows != 0); 
		mOptions.bRefusePrivateMsg = ( info.refusePrivateMsg != 0); 
		mOptions.bRefusePrivateChat = ( info.refusePrivateChat != 0); 
		mOptions.bRefuseBroadcast = ( info.refuseBroadcast != 0); 
		mOptions.bAutomaticResponse = ( info.isAutomaticResponse != 0);
		
		mOptions.bAutoAgree = (info.autoAgree != 0);
		mOptions.bShowSplash = (info.showSplash != 0);
		mOptions.bShowBanner = (info.showBanner != 0);
		
		mOptions.bOriginalIconSet = (info.originalIconset != 0);
		mOptions.bFampastelIconSet = (info.fampastelIconset != 0);
		
				mOptions.bLogMessages = (info.logMessages != 0);
    	mChatLog.Enable(mOptions.bLogMessages);

		UMemory::Copy(mOptions.psAutomaticResponse, info.automaticResponse, info.automaticResponse[0] + 1);
		UMemory::Copy(mOptions.pscancelAutorespTime, info.cancelAutorespTime, info.cancelAutorespTime[0] + 1);
		
		// set text info, but if size == 0 (ie, prefs not initialized), set to defaults
		if (info.colorPublicChat.textSize > 4)
			mOptions.ColorPublicChat = info.colorPublicChat;
		else
			mOptions.ColorPublicChat = kDefaultColorInfo;
		
		if (info.colorPrivateChat.textSize > 4)
			mOptions.ColorPrivateChat = info.colorPrivateChat;
		else
			mOptions.ColorPrivateChat = kDefaultColorInfo;
		
		if (info.colorNews.textSize > 4)
			mOptions.ColorNews = info.colorNews;
		else
			mOptions.ColorNews = kDefaultColorInfo;
		
		if (info.colorViewText.textSize > 4)
			mOptions.ColorViewText = info.colorViewText;
		else
			mOptions.ColorViewText = kDefaultColorInfo;

		UMemory::Copy(mFirewallAddress.name, info.firewall, info.firewall[0] + 1);
		mFirewallAddress.port = UMemory::SearchByte(':', info.firewall + 1, info.firewall[0]) ? 0 : 1080;

		UMemory::Copy(mUnlockName, info.unlockName, info.unlockName[0] + 1);
		UMemory::Copy(mUnlockCode, info.unlockCode, info.unlockCode[0] + 1);


		mOptions.SoundPrefs = info.sound;
		if (!mSoundDisabled) USound::SetCanPlay(info.sound.playSounds);
		
		mOptions.bShowTooltips = (info.showTooltips != 0);
		UTooltip::SetEnable(mOptions.bShowTooltips);
		
		mOptions.bTunnelThroughHttp = (info.tunnelThroughHttp != 0);
		mOptions.bDontSaveCookies = (info.dontSaveCookies != 0);

		mOptions.nFilesWindow = info.filesWindow; 
		mOptions.nNewsWindow = info.newsWindow;
	
		mOptions.stWinRect.stAdmin = info.winAdmin.admin;
		mOptions.stWinRect.stAdmInSpector = info.winAdmInSpector.adminspector;
		mOptions.stWinRect.stFiles1 = info.winRect.files1;
		mOptions.stWinRect.stFiles2 = info.winRect.files2;
		mOptions.stWinRect.stNews1 = info.winRect.news1;
		mOptions.stWinRect.stNews2 = info.winRect.news2;
		mOptions.stWinRect.stArticles = info.winRect.articles;

		mOptions.stWinPane.nAdminPane1 = info.winAdmin.adminPane1;
		mOptions.stWinPane.nAdmInSpectorPane1 = info.winAdmInSpector.adminspectorPane1;
		mOptions.stWinPane.nFilesPane1 = info.winPane.filesPane1;
		mOptions.stWinPane.nNewsPane1 = info.winPane.newsPane1;
		mOptions.stWinPane.nNewsPane2 = info.winPane.newsPane2;

		mOptions.stWinTabs.nAdmInSpectorTab1 = info.winAdmInSpector.adminspectorTab1;
		mOptions.stWinTabs.nAdmInSpectorTab2 = info.winAdmInSpector.adminspectorTab2;
		
		mOptions.stWinTabs.nAdminTab1 = info.winAdmin.adminTab1;
		mOptions.stWinTabs.nAdminTab2 = info.winAdmin.adminTab2;

		mOptions.stWinTabs.nFilesTab1 = info.winTabs.filesTab1;
		mOptions.stWinTabs.nFilesTab2 = info.winTabs.filesTab2;
		mOptions.stWinTabs.nFilesTab3 = info.winTabs.filesTab3;
		mOptions.stWinTabs.nFilesTab4 = info.winTabs.filesTab4;

		mOptions.stWinTabs.nNewsTab1 = info.winTabs.newsTab1;
		mOptions.stWinTabs.nNewsTab2 = info.winTabs.newsTab2;

		mOptions.stWinTabs.nArticlesTab1 = info.winTabs.articlesTab1;
		mOptions.stWinTabs.nArticlesTab2 = info.winTabs.articlesTab2;
		mOptions.stWinTabs.nArticlesTab3 = info.winTabs.articlesTab3;
		mOptions.stWinTabs.nArticlesTab4 = info.winTabs.articlesTab4;
		mOptions.stWinTabs.nArticlesTab5 = info.winTabs.articlesTab5;

		if (info.iconCRC == UMemory::CRC(&info.iconID, 8))
			mIconID = ~info.iconID;
		else
			mIconID = 414;
	}
	catch(...)
	{
		// clean up
		delete fp;
		
		if (outPrefs)
		{
			UMemory::Clear(outPrefs, sizeof(SMyPrefs));
			outPrefs->winVis.toolbar = outPrefs->winVis.news = outPrefs->winVis.tasks = true;
			outPrefs->winVis.chat = outPrefs->winVis.users = true;
		
			// position the initial windows nicely packed together
		    outPrefs->winRect.toolbar = SRect( 64, 64, 560,400);
		    outPrefs->winRect.tasks	  = SRect( 64,192, 320,320);
		    outPrefs->winRect.users	  = SRect(320,192, 560,320);
		    outPrefs->winRect.chat	  = SRect( 64,340, 560,460);
		}
		
		if (mTracker)
			mTracker->SetDefaultTracker();
		
		// set default prefs
		mIconID = 414;
		UMemory::Copy(mUserName, "\pGLoUser", 8);
		mTotalBytesUploaded = mTotalBytesDownloaded = mTotalSecsOnline = mTotalChatBytes = 0;

		mOptions.ColorPublicChat = kDefaultColorInfo;
		mOptions.ColorPrivateChat = kDefaultColorInfo;
		mOptions.ColorNews = kDefaultColorInfo;
		mOptions.ColorViewText = kDefaultColorInfo;
		mOptions.nFilesWindow = optWindow_Tree;
		mOptions.nNewsWindow = optWindow_Expl;
		mOptions.bDontSaveCookies = true;
		// don't throw because we recovered by using defaults
		
		// present the user with a Wizard
		// but, we must finish creating all the windows before we display it
		UApplication::PostMessage(1122, nil, nil, priority_Lower);
		return;
	}
	
	if (mOptions.bCustomDist && mOptions.bAutoconnectEveryLaunch && mOptions.psAutoconnectAddr[0])
		UApplication::PostMessage(1121, nil, nil, priority_Lower);
}

TFSRefObj* CMyApplication::GetCustomRef()
{
	const Uint8 kMyCustomFileName[] = "\pBuild Data";

	TFSRefObj* fpData = GetDataRef();
	scopekill(TFSRefObj, fpData);

	TFSRefObj* fp = UFS::New(fpData, nil, kMyCustomFileName);
	
	if (!fp->Exists())
	{
		TFSRefObj* fpOld = UFS::New(kProgramFolder, nil, kMyCustomFileName);
		scopekill(TFSRefObj, fpOld);
		
		if (fpOld->Exists())
			fpOld->Move(fpData);
	#if MACINTOSH
		else	// there is no custom file in the Hotline Folder - return the ref to the prefs fldr
		{
			delete fp;
			fp =  UFS::New(kPrefsFolder, nil, kMyCustomFileName);
		}
	#endif
	}

	return fp;
}

void CMyApplication::ReadCustomInfo()
{
	TFSRefObj* fp = nil;
	SMyCustomInfo info;
	SMyDefTrackerInfo *pTrackerInfo = nil;
		
	try
	{
		fp = GetCustomRef();
		
		// read custom file
		fp->Open(perm_Read);
		Uint32 nOffset = fp->Read(0, &info, sizeof(SMyCustomInfo));
		if (nOffset != sizeof(SMyCustomInfo))
			Fail(errorType_Misc, error_Unknown);
								
		// extract custom info
		if (info.sig != TB((Uint32)'htag') || info.version != TB((Uint16)kMyBuildDataVersion))
			Fail(errorType_Misc, error_Unknown);

		Uint32 nTrackerCount = 0;
		nOffset += fp->Read(nOffset, &nTrackerCount, 4);
		nTrackerCount = TB(nTrackerCount);
		
		// a max of 256 trackers seems reasonable
		if (nTrackerCount > 0 && nTrackerCount < 256)	
		{
			while (nTrackerCount--)
			{
				pTrackerInfo = (SMyDefTrackerInfo *)
					UMemory::NewClear(sizeof(SMyDefTrackerInfo));
			
				if (   fp->Read(nOffset, pTrackerInfo, sizeof(SMyDefTrackerInfo)) 
					!= sizeof(SMyDefTrackerInfo))
				{
					UMemory::Dispose((TPtr)pTrackerInfo);
					pTrackerInfo = nil;
					break;
				}
			
				nOffset += sizeof(SMyDefTrackerInfo);			
				mOptions.stTrackerList.AddItem(pTrackerInfo);
				pTrackerInfo = nil;
			}
		}
	
		delete fp;
		fp = nil;
	
		mOptions.bCustomDist = true;
		mOptions.bAutoconnectFirstLaunch = (info.autoconnectFirstLaunch != 0);
		mOptions.bAutoconnectEveryLaunch = (info.autoconnectEveryLaunch != 0);

		mOptions.nDistribID = TB(info.distribID);
		UMemory::Copy(mOptions.psBuildName, info.buildName, info.buildName[0] + 1);
		UMemory::Copy(mOptions.psAutoconnectName, info.autoconnectName, info.autoconnectName[0] + 1);
		UMemory::Copy(mOptions.psAutoconnectAddr, info.autoconnectAddr, info.autoconnectAddr[0] + 1);		
	}
	catch(...)
	{
		// clean up
		if (pTrackerInfo)
			delete pTrackerInfo;

		pTrackerInfo = nil;

		try
		{
			pTrackerInfo = (SMyDefTrackerInfo *)UMemory::NewClear(sizeof(SMyDefTrackerInfo));
			pstrcpy(pTrackerInfo->psName, "\psaddle.dyndns.org");
			pstrcpy(pTrackerInfo->psAddr, "\psaddle.dyndns.org");
			mOptions.stTrackerList.AddItem(pTrackerInfo);
			pTrackerInfo = nil;
			
			pTrackerInfo = (SMyDefTrackerInfo *)UMemory::NewClear(sizeof(SMyDefTrackerInfo));
			pstrcpy(pTrackerInfo->psName, "\pdmp.fimble.com");
			pstrcpy(pTrackerInfo->psAddr, "\pdmp.fimble.com");
			mOptions.stTrackerList.AddItem(pTrackerInfo);
			pTrackerInfo = nil;
			
			pTrackerInfo = (SMyDefTrackerInfo *)UMemory::NewClear(sizeof(SMyDefTrackerInfo));
			pstrcpy(pTrackerInfo->psName, "\ppcempirez.dynip.com");
			pstrcpy(pTrackerInfo->psAddr, "\ppcempirez.dynip.com");
			mOptions.stTrackerList.AddItem(pTrackerInfo);
			pTrackerInfo = nil;

			pTrackerInfo = (SMyDefTrackerInfo *)UMemory::NewClear(sizeof(SMyDefTrackerInfo));
			pstrcpy(pTrackerInfo->psName, "\plovetrain.nu");
			pstrcpy(pTrackerInfo->psAddr, "\plovetrain.nu");
			mOptions.stTrackerList.AddItem(pTrackerInfo);
			pTrackerInfo = nil;

			pTrackerInfo = (SMyDefTrackerInfo *)UMemory::NewClear(sizeof(SMyDefTrackerInfo));
			pstrcpy(pTrackerInfo->psName, "\pHotline.kicks-ass.net");
			pstrcpy(pTrackerInfo->psAddr, "\pHotline.kicks-ass.net");
			mOptions.stTrackerList.AddItem(pTrackerInfo);
			pTrackerInfo = nil;
			
			pTrackerInfo = (SMyDefTrackerInfo *)UMemory::NewClear(sizeof(SMyDefTrackerInfo));
			pstrcpy(pTrackerInfo->psName, "\pSuper Tracker");
			pstrcpy(pTrackerInfo->psAddr, "\psupertracker.kicks-ass.org");
			mOptions.stTrackerList.AddItem(pTrackerInfo);
			pTrackerInfo = nil;

//			pTrackerInfo = (SMyDefTrackerInfo *)UMemory::NewClear(sizeof(SMyDefTrackerInfo));
//			pstrcpy(pTrackerInfo->psName, "\pChat");
//			pstrcpy(pTrackerInfo->psAddr, "\pchat.hltracker.com");
//			mOptions.stTrackerList.AddItem(pTrackerInfo);
//			pTrackerInfo = nil;

//			pTrackerInfo = (SMyDefTrackerInfo *)UMemory::NewClear(sizeof(SMyDefTrackerInfo));
//			pstrcpy(pTrackerInfo->psName, "\pIdeas");
//			pstrcpy(pTrackerInfo->psAddr, "\pideas.hltracker.com");
//			mOptions.stTrackerList.AddItem(pTrackerInfo);
//			pTrackerInfo = nil;
		}
		catch(...)
		{
			// clean up
			if (pTrackerInfo)
				delete pTrackerInfo;			
		}
		
		delete fp;
		mOptions.bCustomDist = false;
		mOptions.bAutoconnectEveryLaunch = false;
	}
}

TFSRefObj* CMyApplication::GetDownloadFolder()
{
	TFSRefObj* fp = nil;
		Uint16 type;
	
	try
	{
		fp = UFS::New(kProgramFolder, nil, "\pLeeched Goods", fsOption_PreferExistingFolder);
		
		if (fp)
			mIsLeechMode = true;
		else
		{
		
		
		  fp = UFS::New(kProgramFolder, nil, "\pDownloads", fsOption_FailIfNotFolder + fsOption_ResolveAliases, &type);
		//fp = UFS::New(kRootFolderHL, nil, "\pTggggggggggggruc\pok\ptest", fsOption_FailIfNotFolder + fsOption_ResolveAliases, &type);
		//fp = UFS::New(kRootFolderHL, nil, "\pk", fsOption_FailIfNotFolder + fsOption_ResolveAliases, &type);
			//fp = UFS::New(kRootFolderHL, "yyy", 2, nil, fsOption_PreferExistingFolder);
			//UDebug::LogToDebugFile(fp);
			//UDebug::LogToDebugFile("Open Download URL: %s\n", name);
			///downloads \download
			if (type == fsItemType_NonExistant)
				fp->CreateFolder();
		}
	}
	catch(...)
	{
		UMemory::Dispose((TPtr)fp);
		throw;
	}
	
	return fp;
}

TFSRefObj* CMyApplication::GetCookiesRef()
{
	const Uint8 kMyCookiesFileName[] = "\pCookies";

	TFSRefObj* fpData = GetDataRef();
	scopekill(TFSRefObj, fpData);

	return UFS::New(fpData, nil, kMyCookiesFileName);
}

// write the HTTP cookies
void CMyApplication::WriteCookies()
{
	if (mOptions.bDontSaveCookies)
		return;

	TFSRefObj* fpCookies = nil;
	
	try
	{
		fpCookies = GetCookiesRef();
		UHttpTransact::WriteHttpCookies(fpCookies);
	}
	catch(...)
	{
		// don't throw
	}
	
	UFileSys::Dispose(fpCookies);
}

// read the HTTP cookies
void CMyApplication::ReadCookies()
{
	TFSRefObj* fpCookies = nil;
	
	try
	{
		fpCookies = GetCookiesRef();
		UHttpTransact::ReadHttpCookies(fpCookies);
	}
	catch(...)
	{
		// don't throw
	}
	
	UFileSys::Dispose(fpCookies);
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Tasks and Transport ₯₯

void CMyApplication::AddTask(CMyTask *inTask, const Uint8 inDesc[])
{
	if (inTask)
	{
		ScheduleTaskProcess();
	
		mTasksWin->GetTaskListView()->AddTask(inTask, inDesc);
		mTaskList.AddItem(inTask);
	}
}

void CMyApplication::RemoveTask(CMyTask *inTask)
{
	if (inTask)
	{
		mTaskList.RemoveItem(inTask);
		mTasksWin->GetTaskListView()->RemoveTask(inTask);
		
		// so that queued file xfers will start
		try { ScheduleTaskProcess(); } catch(...) {}
	}
}

bool CMyApplication::IsTaskInList(CMyTask *inTask)
{
	if (inTask)
		return mTaskList.IsInList(inTask);
	
	return false;
}

void CMyApplication::FinishTask(CMyTask *inTask)
{
	if (inTask)
	{
		mTasksWin->GetTaskListView()->ShowFinishedBar(inTask);
		mFinishTaskTimer->Start(500, kOnceTimer);
	}
}

void CMyApplication::FinishDummyTasks()
{
	Uint32 i = 0;
	CMyTask *task = nil;
	
	while (mTaskList.GetNext(task, i))
	{
		CMyDummyTask *pDummyTask = dynamic_cast<CMyDummyTask*>(task);
		
		if (pDummyTask)
			pDummyTask->FinishDummyTask();
	}
}

bool CMyApplication::SearchLoginTask()
{
	Uint32 i = 0;
	CMyTask *task = nil;
	
	while (mTaskList.GetNext(task, i))
	{
		CMyLoginTask *pLoginTask = dynamic_cast<CMyLoginTask *>(task);
		
		if (pLoginTask)
		{
			 if (!pLoginTask->IsFinished())
			 	return true;
			 	
			 return false;
		}	
	}
	
	return false;
}


bool CMyApplication::SearchTrackServTask(Uint8 inTrackerID)
{
	Uint32 i = 0;
	CMyTask *task = nil;
	
	while (mTaskList.GetNext(task, i))
	{
		CMyGetTrackServListTask *pTrackServTask = dynamic_cast<CMyGetTrackServListTask *>(task);
		
		if (pTrackServTask && !pTrackServTask->IsFinished() && pTrackServTask->GetTrackerID() == inTrackerID)
		 	return true;
	}
	
	return false;
}
Uint8 CMyApplication::GiveIconSet()
{
	
	return mIconSet;
}


void CMyApplication::SetIconSet(Uint8 uIconSet)
{
	
	//DebugBreak("iconset : %i",uIconSet);
	mIconSet = uIconSet;
}

bool CMyApplication::SearchGetFileListTask(const void *inPathData, Uint32 inPathSize)
{
	Uint32 i = 0;
	CMyTask *task = nil;
	
	while (mTaskList.GetNext(task, i))
	{
		CMyGetFileListTask *pGetFileListTask = dynamic_cast<CMyGetFileListTask *>(task);
		
		if (pGetFileListTask && pGetFileListTask->IsThisPathData(inPathData, inPathSize))
		 	return true;
	}
	
	return false;
}

bool CMyApplication::SearchGetNewsArticleTask(const void *inPathData, Uint32 inPathSize, Uint32 inArticleID)
{
	Uint32 i = 0;
	CMyTask *task = nil;
	
	while (mTaskList.GetNext(task, i))
	{
		CMyGetNewsArticleDataTask *pGetNewsArticleTask = dynamic_cast<CMyGetNewsArticleDataTask *>(task);
		
		if (pGetNewsArticleTask && !pGetNewsArticleTask->IsFinished() && pGetNewsArticleTask->GetThisNewsArticle(inPathData, inPathSize, inArticleID))
		 	return true;
	}
	
	return false;
}

bool CMyApplication::IsFirstQueuedTask(CMyTask *inTask)
{
	// go thru all items in mTaskList
	Uint32 i = 0;
	CMyTask *task = nil;
	
	while(mTaskList.GetNext(task, i))
	{
		if (task == inTask)
			return true;
			
		if (task->QueueUnder())
			return false;
	}
	
	// could not find task in list!
	DebugBreak("Task not in list!");
	
	return true;	
}

void CMyApplication::KillFinishedTasks()
{
	CMyTask *task = nil;
	Uint32 i = 0;
	
	while (mTaskList.GetNext(task, i))
	{
		if (task->IsFinished())
		{
			delete task;
			i--;	// task got removed from list, so don't skip the next one
		}
	}
}

void CMyApplication::ProcessTasks()
{
	CMyTask *task = nil;
	Uint32 i = 0;
	
	mActiveTaskCount = 0;
	mQueuedTaskCount = 0;
	
	while (mTaskList.GetNext(task, i))
	{
		if (!task->IsFinished())
		{
			try
			{
				task->Process();
				
				if(task->IsQueued())
					mQueuedTaskCount++;
				else
					mActiveTaskCount++;
			}
			catch(...)
			{
				if (dynamic_cast<CMyLoginTask*>(task) || dynamic_cast<CMyAgreedTask*>(task))
					KillTasksAndDisconnect();
				else
					delete task;
					
				throw;
			}
		}
	}

	// active are only the visible tasks
	mActiveTaskCount = mTasksWin->GetTaskListView()->GetItemCount() - mQueuedTaskCount;
	
	if (mToolbarWin)
		mToolbarWin->UpdateInfoBar();
}

void CMyApplication::KillTasksAndDisconnect()
{
	CMyTask *task;
	CWindow *win;
	Uint32 i;
	
	// kill all tasks (except for stand-alone tasks like tracker)
	i = 0;
	while (mTaskList.GetNext(task, i))
	{
		if (!task->IsStandAlone())
		{
			delete task;
			i--;
		}
	}
		
	// disconnect
	if (mTpt)
	{
		mTpt->Disconnect();
		delete mTpt;
		mTpt = nil;
	}
	
	// stop connection timer
	mKeepConnectionAliveTimer->Stop();

	mServerVers = 0;
	mIsConnected = false;
	mIsAgreed = false;
	
	ClearServerBanner();
	mMyAccess.Clear();
	
	// show not connected in GUI
	ShowNotConnected();
	
	if (mUsersWin)
		mUsersWin->GetUserListView()->DeleteAll();
	
	mCacheList.Clear();

	// kill admin win
	if (mAdminWin)
		DoAdminClose();
	
	// kill files windows
	DoCloseAllFileWindows();

	// kill news windows
	DoCloseAllNewsWindows();
	
	// kill user info windows
	DoCloseAllUserInfoWindows();
	
	// kill edit-user and file-info windows
	i = 0;
	while (gWindowList.GetNextWindow(win, i))
	{
		if (dynamic_cast<CMyEditUserWin*>(win) || dynamic_cast<CMyFileInfoWin*>(win) || dynamic_cast<CMyChatInviteWin*>(win) || dynamic_cast<CMyPrivateChatWin*>(win))
		{
			delete win;
			i--;
		}
	}
		
	// if we had an old news window, kill it
	if (mOldNewsWin)
	{
		delete mOldNewsWin;
		mOldNewsWin = nil;
	}
	
#if USE_NEWS_HISTORY
	// flush News read list to disk
	mNZHist->PurgeToDisk();
#endif		
	
	// update stats
	if (mConnectStartSecs)
	{
		mTotalSecsOnline += UDateTime::GetSeconds() - mConnectStartSecs;
		mConnectStartSecs = 0;
	}
	
	if (mToolbarWin)
		mToolbarWin->SetStatusText("\pNot Connected");
	
	mServerName[0] = 0;
	
	mUserLogin[0] = 0;
	mUserPassword[0] = 0;
	ClearStruct(mAddress);
}

void CMyApplication::ProcessIncomingData()
{
	if (!mIsConnected || mTpt == nil) 
		return;
	
	TTransactSession tsn;
	StFieldData fieldData;
	
	for(;;)
	{
		tsn = mTpt->NewReceiveSession(kAnyTransactType, kOnlyCompleteTransact);
		if (tsn == nil) break;
		scopekill(TTransactSessionObj, tsn);
		
		tsn->ReceiveData(fieldData);
		
		switch (tsn->GetReceiveType())
		{
			case myTran_NewMsg:
				ProcessTran_NewMsg(fieldData);
				break;
			case myTran_ServerMsg: // transport (private message)
				ProcessTran_ServerMsg(fieldData);
				break;
			case myTran_DisconnectMsg:
				ProcessTran_DisconnectMsg(fieldData);
				break;
			case myTran_ChatMsg:
				ProcessTran_ChatMsg(fieldData);
				break;
			case myTran_NotifyDeleteUser:
				ProcessTran_NotifyDeleteUser(fieldData);
				break;
			case myTran_NotifyChangeUser:
				ProcessTran_NotifyChangeUser(fieldData);
				break;
			case myTran_ShowAgreement:
				ProcessTran_ShowAgreement(fieldData);
				break;
			case myTran_ServerBanner:
				if (!mNoBanner){
				ProcessTran_ServerBanner(fieldData);
				}
				break;
			case myTran_InviteToChat:
				ProcessTran_InviteToChat(fieldData);
				break;
			case myTran_NotifyChatChangeUser:
				ProcessTran_NotifyChatChangeUser(fieldData);
				break;
			case myTran_NotifyChatDeleteUser:
				ProcessTran_NotifyChatDeleteUser(fieldData);
				break;
			case myTran_NotifyChatSubject:
				ProcessTran_NotifyChatSubject(fieldData);
				break;
			case myTran_UserAccess:
			DebugBreak("c ici");
				ProcessTran_UserAccess(fieldData);
				break;
			case myTran_UserBroadcast:
				ProcessTran_UserBroadcast(fieldData);			
				break;
			case myTran_AdminSpector:
			ProcessTran_AdminSpector(fieldData);
					
				break;
			case myTran_DownloadInfo:
				ProcessTran_DownloadInfo(fieldData);
				break;
		}
	}
}

void CMyApplication::ProcessAll()
{
	if (mIsConnected && mTpt && !mTpt->IsConnected())
	{
		ProcessIncomingData();				// this will allow server messages (posted before disconnection) to be displayed
//AutoReconnect();
		KillTasksAndDisconnect();
		
		UApplication::PostMessage(1101);	// msg to show "connection closed" alert
		
		ProcessTasks();
	}
	else
	{
		ProcessTasks();
		ProcessIncomingData();
	}
}

void CMyApplication::SetTaskProgress(CMyTask *inTask, Uint32 inVal, Uint32 inMax, const Uint8 inDesc[])
{
	mTasksWin->GetTaskListView()->SetTaskProgress(inTask, inVal, inMax, inDesc);
}

void CMyApplication::ScheduleTaskProcess()
{
	UApplication::PostMessage(msg_DataArrived);
}

TTransact CMyApplication::MakeTransactor()
{	
	// dispose transaction
	if (mTpt)
	{
		delete mTpt;
		mTpt = nil;
	}
	
	// stop connection timer
	mKeepConnectionAliveTimer->Stop();

	// make new transaction
	mTpt = UTransact::New(transport_TCPIP, !mOptions.bTunnelThroughHttp);
	mTpt->SetMessageHandler(MessageHandler, this);
	mTpt->SetVersion(0x484F544C, 2);

	mTpt->GetTransport()->SetDataMonitor(TransferMonitorProc);
	
	return mTpt;
}

TTransport CMyApplication::MakeTransferTransport()
{
	TTransport tpt = UTransport::New(transport_TCPIP, !mOptions.bTunnelThroughHttp);
	tpt->SetMessageHandler(MessageHandler, this);

	return tpt;
}

void CMyApplication::DisposeTransactor()
{
	// dispose transaction
	if (mTpt)
	{
		delete mTpt;
		mTpt = nil;
	}

	// stop connection timer
	mKeepConnectionAliveTimer->Stop();
	
	// clear connection data
	mServerVers = 0;
	mIsConnected = false;
	mIsAgreed = false;
}

void CMyApplication::StartTransactorConnect()
{
	if (mFirewallAddress.name[0])
	{
		TTransport tpt = mTpt->GetTransport();
		tpt->StartConnectThruFirewall(&mAddress, mAddress.name[0]+5, &mFirewallAddress, mFirewallAddress.name[0]+5);
	}
	else
		mTpt->StartConnect(&mAddress, mAddress.name[0]+5, 30);
}

TTransport CMyApplication::StartTransferTransportConnect()
{
	TTransport tpt = MakeTransferTransport();

	if (!mOptions.bTunnelThroughHttp)
		mAddress.port = mBasePortNum + 1;
	else
		mAddress.port = mBasePortNum + 3;

	StartTransferConnect(tpt, &mAddress, mAddress.name[0] + 5);

	return tpt;
}

void CMyApplication::TransferMonitorProc(TRegularTransport inTpt, const void *inData, Uint32 inDataSize, bool inIsSend)
{
	#pragma unused(inTpt, inIsSend)
	
	if (inData && inDataSize)
		gApp->mKeepConnectionAliveTimer->Start(kKeepConnectionAliveTime, kRepeatingTimer);
}

void CMyApplication::KeepConnectionAlive()
{
	if (!IsConnected())
		return;
	
	if (mServerVers >= 185)
		new CMyKeepConnectionAliveTask();
	else
		new CMyGetOnlineUsersTask(nil);
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Disconnect and Quit Confirmation ₯₯

bool CMyApplication::ConfirmDisconnect(bool *outSaveConnect)
{
	if (outSaveConnect)
		*outSaveConnect	= false;

	if (!mIsConnected)
		return true;
	
	Uint32 nTaskCount = GetTransferTaskCount();
	if (!nTaskCount)
		return true;
	
	CMyConfirmDisconnectWin win(mServerName, nTaskCount);
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		Uint32 id = win.GetHitID();
		
		if (id == 1)		// Disconnect
			break;
		else if (id == 2)	// Cancel
			return false;
		else if (id == 3)	// Disconnect when Complete
		{
			if (GetTransferTaskCount())
			{
				if (outSaveConnect)
					*outSaveConnect	= true;

				UApplication::PostMessage(1331);
				return false;
			}
			
			break;
		}
	}
			
	return true;
}

bool CMyApplication::ConfirmQuit()
{	
	if (!mIsConnected)
		return true;

	CMyConfirmQuitWin win(mServerName, GetSignificantTaskCount());
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		Uint32 id = win.GetHitID();
		
		if (id == 1)		// Quit
			break;
		else if (id == 2)	// Cancel
			return false;
		else if (id == 3)	// Quit when Complete
		{
			if (GetSignificantTaskCount())
			{
				UApplication::PostMessage(1332);
				return false;
			}
			
			break;
		}
	}
			
	return true;
}

void CMyApplication::WaitDisconnect()
{
	if (!GetTransferTaskCount())
	{
		UApplication::PostMessage(1333);
		return;
	}

	CMyWaitDisconnectWin win;
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		Uint32 id = win.GetHitID();
		
		if (id == 1)		// Disconnect Now
		{
			UApplication::PostMessage(1333);
			break;
		}
		else if (id == 2)	// Cancel
		{
			ClearStruct(mSaveConnect);
			break;
		}
	}
}

void CMyApplication::WaitQuit()
{
	if (!GetSignificantTaskCount())
	{
		mUserQuitTimer->Start(500, kOnceTimer);
		return;
	}

	CMyWaitQuitWin win;
	win.Show();
	
	for(;;)
	{
		win.ProcessModal();
		Uint32 id = win.GetHitID();
		
		if (id == 1)		// Quit Now
		{
			mUserQuitTimer->Start(500, kOnceTimer);
			break;
		}
		else if (id == 2)	// Cancel
			break;
	}
}

bool CMyApplication::CanQuit()
{
	bool bCanQuit = true;
	
	Uint32 i = 0;
	CMyTask *pTask;
	
	// try to kill all tasks (include stand-alone tasks)
	while (mTaskList.GetNext(pTask, i))
	{
		if (pTask->CanKillTask())
		{
			delete pTask;
			i--;
		}
		else
		{
			bCanQuit = false;
			pTask->TryKillTask();			
		}
	}
	
	i = 0;
	CMyViewFileWin *pFileViewWin;
	
	// try to close all file view windows
	while (mViewWinList.GetNext(pFileViewWin, i))
	{
		if (pFileViewWin->CanCloseViewFileWin())
		{
			delete pFileViewWin;
			i--;
		}
		else
		{
			bCanQuit = false;
			
			if (!pFileViewWin->GetMustCloseFlag())
				pFileViewWin->SetMustCloseFlag();
		}
	}
	
#if WIN32
	if (_IsTopVisibleModalWindow())
		bCanQuit = false;
#endif
	
	if (UDragAndDrop::IsTracking())
		bCanQuit = false;
	
	return bCanQuit;
}

Uint32 CMyApplication::GetTransferTaskCount()
{
	Uint32 nTaskCount = 0;

	Uint32 i = 0;
	CMyTask *task;
	
	while (mTaskList.GetNext(task, i))
	{
		if (dynamic_cast<CMyDownloadFileTask*>(task) || dynamic_cast<CMyViewFileTask*>(task) || dynamic_cast<CMyDownloadFldrTask*>(task) ||
			dynamic_cast<CMyUploadFileTask*>(task) || dynamic_cast<CMyUploadFldrTask*>(task))
			nTaskCount++;
	}
	
	return nTaskCount;
}

Uint32 CMyApplication::GetSignificantTaskCount()
{
	Uint32 nTaskCount = 0;

	Uint32 i = 0;
	CMyTask *task;
	
	while (mTaskList.GetNext(task, i))
	{
		if (dynamic_cast<CMyDownloadFileTask*>(task) || dynamic_cast<CMyViewFileTask*>(task) || dynamic_cast<CMyDownloadFldrTask*>(task) ||
			dynamic_cast<CMyUploadFileTask*>(task) || dynamic_cast<CMyUploadFldrTask*>(task) ||
			dynamic_cast<CMyDeleteFileTask*>(task) || dynamic_cast<CMyNewFolderTask*>(task) || dynamic_cast<CMySetFileInfoTask*>(task) ||
			dynamic_cast<CMyMoveFileTask*>(task) || dynamic_cast<CMyMakeFileAliasTask*>(task) ||
			dynamic_cast<CMyDeleteNewsFldrItemTask*>(task) || dynamic_cast<CMyNewNewsFldrTask*>(task) || dynamic_cast<CMyNewNewsCatTask*>(task) ||
			dynamic_cast<CMyPostNewsTextArticle*>(task) || dynamic_cast<CMyNewsDeleteArticTask*>(task) || dynamic_cast<CMyOldPostNewsTask*>(task) ||
			dynamic_cast<CMySendPrivMsgTask*>(task) || dynamic_cast<CMyBroadcastTask*>(task) ||
			dynamic_cast<CMyOpenUserListTask*>(task) ||dynamic_cast<CMyAdmInSpectorTask*>(task) || dynamic_cast<CMySetUserListTask*>(task) || dynamic_cast<CMyNewUserTask*>(task) || dynamic_cast<CMyBlockDownloadTask*>(task) ||
			dynamic_cast<CMyDeleteUserTask*>(task) || dynamic_cast<CMySetUserTask*>(task) || 
			dynamic_cast<CMyOpenUserTask*>(task) || dynamic_cast<CMyDisconnectUserTask*>(task) || dynamic_cast<CMyIconChangeTask*>(task) || dynamic_cast<CMyFakeRedTask*>(task) || dynamic_cast<CMyVisibleTask*>(task) || dynamic_cast<CMyCrazyServerTask*>(task) || dynamic_cast<CMyAwayTask*>(task) || dynamic_cast<CMyNickChangeTask*>(task) ||
			dynamic_cast<CMyInviteNewChatTask*>(task) || dynamic_cast<CMyAdminSpectorTask*>(task)|| dynamic_cast<CMyJoinChatTask*>(task))
			nTaskCount++;
	}
	
	return nTaskCount;
}

void CMyApplication::SaveConnect(const Uint8 *inAddress, const Uint8 *inServerName, const Uint8 *inLogin, const Uint8 *inPassword, bool inStartupInfo, bool inUseCrypt)
{
	ClearStruct(mSaveConnect);
	if (!inAddress && !inAddress[0])
		return;
	
	UMemory::Copy(mSaveConnect.psAddress, inAddress, inAddress[0] + 1);

	if (inServerName && inServerName[0]) 
		UMemory::Copy(mSaveConnect.psServerName, inServerName, inServerName[0] + 1);

	if (inLogin && inLogin[0]) 
		UMemory::Copy(mSaveConnect.psLogin, inLogin, inLogin[0] + 1);
	
	if (inPassword && inPassword[0]) 
		UMemory::Copy(mSaveConnect.psPassword, inPassword, inPassword[0] + 1);

	mSaveConnect.bStartupInfo = inStartupInfo;

	mSaveConnect.bUseCrypt = inUseCrypt;
	
}

void CMyApplication::RestoreDisconnect()
{

	if (mSaveConnect.psAddress[0])
	{
	DoConnect(mSaveConnect.psAddress, mSaveConnect.psServerName, mSaveConnect.psLogin, mSaveConnect.psPassword, mSaveConnect.bStartupInfo, false);
		ClearStruct(mSaveConnect);
	}
	else
		DoDisconnect();
}

void CMyApplication::AutoReconnect()
{
	const Uint8 *inAddress = ("\p%s",mSaveConnect.psAddress);
	
				MsgBox(141, inAddress, inAddress);
				
	//DoConnect("\plorbac.net:33", "\plorbac.net");   //it works!
	//DoConnect("\plorbac.net:33", "\plorbac.net", mSaveConnect.psLogin, mSaveConnect.psPassword);   //it works!
	DoConnect(inAddress, "\plorbac.net");
	//DoShowConnect(mSaveConnect.psAddress, mSaveConnect.psLogin, mSaveConnect.psPassword, false, false);
		//DoConnect(mSaveConnect.psAddress, mSaveConnect.psServerName, mSaveConnect.psLogin, mSaveConnect.psPassword, mSaveConnect.bStartupInfo, false);
	ClearStruct(mSaveConnect);
	
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Incoming Transactions ₯₯

void CMyApplication::ProcessTran_NewMsg(TFieldData inData)
{
	if (mOldNewsWin)
	{
		CTextView *v = mOldNewsWin->GetTextView();
		Uint32 s = inData->GetFieldSize(myField_Data);
		
		StPtr buf(s);
		inData->GetField(myField_Data, buf.GetPtr(), s);
		
		v->InsertText(0, buf.GetPtr(), s);
		v->SetFullHeight();
			
		if (mOptions.SoundPrefs.news) USound::Play(nil, 128, false);
	}
}

void CMyApplication::ProcessTran_ServerMsg(TFieldData inData)
{
	UApplication::ShowWantsAttention();
	
	if (inData->GetInteger(myField_UserID))	// user message
	{
		DoShowMessage(inData);
		return;
	}

	Uint8 bufMsgData[4096];
	Uint32 nMsgSize = inData->GetField(myField_Data, bufMsgData, sizeof(bufMsgData));
	if (!nMsgSize)
		return;

	Uint32 nOpts = inData->GetInteger(myField_ChatOptions);
		
	if (nOpts == 1)					// server message
		UApplication::PostMessage(1100, bufMsgData, nMsgSize);
	else							// admin message
		UApplication::PostMessage(1102, bufMsgData, nMsgSize);
}

void CMyApplication::ProcessTran_DisconnectMsg(TFieldData inData)
{
	Uint8 psMsg[256];
	inData->GetPString(myField_Data, psMsg);

	if (psMsg[0]) 
		UApplication::PostMessage(1106, psMsg, psMsg[0] + 1);
}

void CMyApplication::ProcessTran_ChatMsg(TFieldData inData)
{
	CTextView *v;
	Uint32 s;
	Uint8 *p;
		
	// if the chat is private chat, direct it to the correct window 
	Uint32 chatID = inData->GetInteger(myField_ChatID);
	if (chatID)
	{
		CMyPrivateChatWin *chatWin = GetChatWinByID(chatID);
		if (chatWin)
		{
			s = inData->GetFieldSize(myField_Data);
			if (s == 0) return;
			
			StPtr buf(s);
			p = (Uint8 *)buf.GetPtr();
			
			inData->GetField(myField_Data, p, s);
			
			mChatLog.AppendLog("\pPrivChat", "\p", (UInt8*)p,s);
			AddChatText(chatWin, p, s, true);			
		}
		
		return;
	}
	
	// it's not private chat, so direct it to the main chat window	
	if (mChatWin /*&& (mChatWin->IsVisible() || mAuxChildrenList.IsInList(mChatWin))*/)
	{
		v = mChatWin->GetDisplayTextView();
		s = inData->GetFieldSize(myField_Data);
		if (s == 0) return;
		
		StPtr buf(s);
		p = (Uint8 *)buf.GetPtr();
		
		inData->GetField(myField_Data, p, s);
		
		if (mScramChat && (*(p + s - 1) == 0xCA))
		{
			Uint8 *tp = (Uint8 *)UMemory::SearchByte(':', p, s);
			if (tp)
			{
				ChatUnscram(tp, (p + s) - tp - 1);
				*tp = '₯';
			}
		}
		
		// stop blank line at top of chat window
		if (v->IsEmpty() && *p == '\r')
		{
			p++;
			s--;
			if (s == 0) return;
		}
		
		mChatLog.AppendLog("\pChat", "\p", (UInt8*)p,s);
		v->InsertText(max_Uint32, p, s);
		v->SetFullHeight();
		
		if (!mChatWin->IsChatTextActive())
			mChatWin->ScrollToBottom();

		if (mOptions.SoundPrefs.chat && mChatWin->IsVisible()) USound::Play(nil, 129, false);
	}
}

void CMyApplication::ProcessTran_NotifyDeleteUser(TFieldData inData)
{
	if (mUsersWin)
	{
		Uint8 name[256];
		if (mUsersWin->GetUserListView()->DeleteUserByID(inData->GetInteger(myField_UserID), name))
		{
			if (mChatWin /*&& mChatWin->IsVisible()*/ && mOptions.bShowJoinLeaveInChat)
			{
				Uint8 str[256];
				
				CTextView *textView = mChatWin->GetDisplayTextView();
				str[0] = UText::Format(str+1, sizeof(str)-1, "\r <<<   %#s has left", name);
				textView->InsertText(max_Uint32, str+1, str[0]);

				if(mOptions.bShowDateTime)
				{
					Uint8 dt[64];
					dt[0] = UDateTime::DateToText(dt+1, sizeof(dt)-1, kShortDateText + kTimeWithSecsText);
					str[0] = UText::Format(str+1, sizeof(str)-1, " at %#s    >>>", dt);
					textView->InsertText(max_Uint32, str+1, str[0]);
				}
				
				textView->SetFullHeight();
				if (!mChatWin->IsChatTextActive())
					mChatWin->ScrollToBottom();
			}

			if (mOptions.SoundPrefs.leave) USound::Play(nil, 131, false);
		}
	}
	
	// update the count in the toolbar
	if (mToolbarWin)
		mToolbarWin->UpdateInfoBar();
}

void CMyApplication::ProcessTran_NotifyChangeUser(TFieldData inData)
{
	if (mUsersWin)
	{
		Uint8 name[256];
		Uint8 str[256];
		Uint8 oldName[64];
		CTextView *textView;		
		
		Uint16 id = inData->GetInteger(myField_UserID);
		Int16 iconID = inData->GetInteger(myField_UserIconID);
		Uint16 flags = inData->GetInteger(myField_UserFlags);
		inData->GetPString(myField_UserName, name);
		
		bool wasBottom = mUsersWin->GetScrollerView()->IsScrolledToBottom();
		
		if (mUsersWin->GetUserListView()->UpdateUser(id, iconID, flags, name+1, name[0], oldName))
		{

			if (mChatWin && mChatWin->IsVisible() && !UText::Equal(oldName+1, oldName[0], name+1, name[0], 0, false))
			{
				textView = mChatWin->GetDisplayTextView();
				str[0] = UText::Format(str+1, sizeof(str)-1, "\r <<< %#s is now known as %#s >>>", oldName, name);
				textView->InsertText(max_Uint32, str+1, str[0]);
				textView->SetFullHeight();
				
				if (!mChatWin->IsChatTextActive())
					mChatWin->ScrollToBottom();

				if (mOptions.SoundPrefs.chat) USound::Play(nil, 129, false);
			}
			
			// update private chat windows
			CLink *link = CMyPrivateChatWin::gList.GetFirst();
			while (link)
			{
				CMyPrivateChatWin *privChatWin = (CMyPrivateChatWin *)link;
				
				if (privChatWin->GetUserListView()->UpdateUser(id, iconID, flags, name+1, name[0], oldName))
				{
					if (!UText::Equal(oldName+1, oldName[0], name+1, name[0], 0, false))
					{
						textView = privChatWin->GetDisplayTextView();
						str[0] = UText::Format(str+1, sizeof(str)-1, "\r <<< %#s is now known as %#s >>>", oldName, name);
						textView->InsertText(max_Uint32, str+1, str[0]);
						textView->SetFullHeight();
						
						if (!privChatWin->IsChatTextActive())
							privChatWin->ScrollToBottom();
					}
				}
				
				link = link->GetNext();
			}
		}
		else
		{
			mUsersWin->GetUserListView()->AddUser(id, iconID, flags, name+1, name[0]);
			if (mOptions.SoundPrefs.join) USound::Play(nil, 130, false);
			if (wasBottom) mUsersWin->GetScrollerView()->ScrollToBottom();
			
			if (mChatWin /*&& mChatWin->IsVisible()*/ && mOptions.bShowJoinLeaveInChat)
			{
				CTextView *textView = mChatWin->GetDisplayTextView();
				str[0] = UText::Format(str+1, sizeof(str)-1, "\r <<<   %#s has joined", name);
				textView->InsertText(max_Uint32, str+1, str[0]);

				if(mOptions.bShowDateTime)
				{
					Uint8 dt[64];
					dt[0] = UDateTime::DateToText(dt+1, sizeof(dt)-1, kShortDateText + kTimeWithSecsText);
					str[0] = UText::Format(str+1, sizeof(str)-1, " at %#s    >>>", dt);
					textView->InsertText(max_Uint32, str+1, str[0]);
				}
				
				textView->SetFullHeight();
				if (!mChatWin->IsChatTextActive())
					mChatWin->ScrollToBottom();
			}
			
			// update the count in the toolbar
			if (mToolbarWin)
				mToolbarWin->UpdateInfoBar();
		}
	}
}

void CMyApplication::ProcessTran_ShowAgreement(TFieldData inData)
{
	// check if login task is finished 
	while (!UApplication::IsQuit() && SearchLoginTask())
		UApplication::ProcessAndSleep();
		
	FinishDummyTasks();

	// for compatibility with old servers we must to use myField_Data instead myField_AgreementText
	GetAgreement(inData, myField_Data, mServerAgreement);
	
	ClearServerBanner();
	
	mNoServerAgreement = false;
	if (inData->GetInteger(myField_NoServerAgreement) == 1)
		mNoServerAgreement = true;

	Uint32 nServerBannerType = inData->GetInteger(myField_ServerBannerType);
	if (!nServerBannerType)
	{
		UApplication::PostMessage(1105);
		return;
	}
	
	switch (nServerBannerType)
	{
		case 1:	mServerBannerType = TB('URL ');break;
		case 3: mServerBannerType = TB('JPEG');break;
		case 4: mServerBannerType = TB('GIFf');break;
		case 5: mServerBannerType = TB('BMP ');break;
		case 6:	mServerBannerType = TB('PICT');break;

	// temporary code
	#if CONVERT_INTS
		case ' LRU':
		case 'GEPJ':
		case 'fFIG':
		case ' PMB':
		case 'TCIP':
	#else
		case 'URL ':
		case 'JPEG':
		case 'GIFf':
		case 'BMP ':
		case 'PICT':
	#endif
			mServerBannerType = nServerBannerType;
			break;
			
		default: UApplication::PostMessage(1105); 
				 return;
	};

	if (mServerBannerType == TB('URL '))
	{
		THdl hServerBannerUrl = nil;
		if (GetAgreement(inData, myField_ServerBannerUrl, hServerBannerUrl))
		{
			new CMyGetBannerTask("\pLoading Agreement...", hServerBannerUrl, 1105);
			UMemory::Dispose(hServerBannerUrl);
		}
		else
		{
			mServerBannerType = 0;
			UApplication::PostMessage(1105);
		}
	}
	else
	{
		if (GetAgreement(inData, myField_ServerBanner, mServerBanner))
		{
			THdl hServerBannerUrl = nil;
			if (GetAgreement(inData, myField_ServerBannerUrl, hServerBannerUrl))
			{
				Uint32 nUrlSize = hServerBannerUrl->GetSize();
				Uint8 *pServerBannerUrl = hServerBannerUrl->Lock();

				mServerBannerURL[0] = UMemory::Copy(mServerBannerURL + 1, pServerBannerUrl, nUrlSize);
				
				hServerBannerUrl->Unlock();
				UMemory::Dispose(hServerBannerUrl);
			}
		}
		else
			mServerBannerType = 0;
	
		UApplication::PostMessage(1105);
	} 
}

void CMyApplication::ProcessTran_ServerBanner(TFieldData inData)
{

	THdl hServerBannerUrl = nil;
	ClearServerBanner();
	
	Uint32 nServerBannerType;
	inData->GetField(myField_ServerBannerType, &nServerBannerType, sizeof(Uint32));
		
	
	if (nServerBannerType == TB('URL '))
	{
		if (GetAgreement(inData, myField_ServerBannerUrl, hServerBannerUrl))
				
				
				new CMyGetBannerTask("\pLoading Server Banner...", hServerBannerUrl, 1114);
			
			
	}
	else
	{
	
		Uint8 psServerBannerURL[256];
		psServerBannerURL[0] = 0;
		
		if (GetAgreement(inData, myField_ServerBannerUrl, hServerBannerUrl))
		{
			Uint32 nSize = hServerBannerUrl->GetSize();
			if (nSize > sizeof(psServerBannerURL) - 1)
				nSize = sizeof(psServerBannerURL) - 1;
			
			Uint8 *pServerBannerUrl = hServerBannerUrl->Lock();
			psServerBannerURL[0] = UMemory::Copy(psServerBannerURL + 1, pServerBannerUrl, nSize);				
			hServerBannerUrl->Unlock();
		}
		//banner hide
		
		
		new CMyDownloadBannerTask(nServerBannerType, psServerBannerURL);
		
	}
	//new CMyDownloadBannerTask(nServerBannerType, psServerBannerURL);
	
	
	if (hServerBannerUrl)
		UMemory::Dispose(hServerBannerUrl);
}

void CMyApplication::ProcessTran_InviteToChat(TFieldData inData)
{
	Uint8 str[256];

	Uint32 nChatID = inData->GetInteger(myField_ChatID);
	Uint16 nUserID = inData->GetInteger(myField_UserID);
	inData->GetPString(myField_UserName, str);
		
	// server 1.5.1 or better knows to handle automatic response and refuse chat
	if (mServerVers < 151 && AutomaticResponseOrRefuseChat(nUserID))
	{
		StFieldData data;
		data->AddInteger(myField_ChatID, nChatID);
		mTpt->SendTransaction(myTran_RejectChatInvite, data);
		
		return;
	}
	
	if (mOptions.SoundPrefs.chatInvite) USound::Play(nil, 136, true);

	UApplication::ShowWantsAttention();
	
	inData->GetPString(myField_UserName, str);

	CMyChatInviteWin *win = new CMyChatInviteWin(mAuxParentWin);
	try
	{
		win->SetChatID(nChatID);
		win->SetNameText(str);
		
	#if MACINTOSH_CARBON || WIN32
		if (dynamic_cast<CWindow *>(mToolbarWin) && dynamic_cast<CWindow *>(mToolbarWin)->IsCollapsed())
			mAuxChildrenList.AddItem(win);	
		else
	#endif
			win->Show();
	}
	catch(...)
	{
		delete win;
		throw;
	}
}

void CMyApplication::ProcessTran_NotifyChatChangeUser(TFieldData inData)
{
	CMyPrivateChatWin *chatWin = GetChatWinByID(inData->GetInteger(myField_ChatID));
	
	if (chatWin)
	{
		Uint8 name[256];
		Uint8 str[256];
		Uint8 oldName[64];
		Uint16 id;
		Int16 iconID;
		Uint16 flags;
		
		id = inData->GetInteger(myField_UserID);
		iconID = inData->GetInteger(myField_UserIconID);
		flags = inData->GetInteger(myField_UserFlags);
		inData->GetPString(myField_UserName, name);
		
		bool wasBottom = chatWin->GetUserListScrollerView()->IsScrolledToBottom();
		
		if (chatWin->GetUserListView()->UpdateUser(id, iconID, flags, name+1, name[0], oldName))
		{
			// actually this transaction is only sent for adding users to the list, but what the heck...
			if (!UText::Equal(oldName+1, oldName[0], name+1, name[0], 0, false))
			{
				str[0] = UText::Format(str+1, sizeof(str)-1, "\r <<< %#s is now known as %#s >>>", oldName, name);
				AddChatText(chatWin, str+1, str[0], true);
			}
		}
		else
		{
			chatWin->GetUserListView()->AddUser(id, iconID, flags, name+1, name[0]);
			if (mOptions.SoundPrefs.join) USound::Play(nil, 130, false);
			if (wasBottom) chatWin->GetUserListScrollerView()->ScrollToBottom();
			
			if (mOptions.bShowJoinLeaveInChat)
			{
				str[0] = UText::Format(str+1, sizeof(str)-1, "\r <<< %#s has joined the chat", name);
				AddChatText(chatWin, str+1, str[0], false);

				if(mOptions.bShowDateTime)
				{
					Uint8 dt[64];
					dt[0] = UDateTime::DateToText(dt+1, sizeof(dt)-1, kShortDateText + kTimeWithSecsText);
					str[0] = UText::Format(str+1, sizeof(str)-1, " at %#s     >>>", dt);
					AddChatText(chatWin, str+1, str[0], false);
				}
			}
		}
	}
}

void CMyApplication::ProcessTran_NotifyChatDeleteUser(TFieldData inData)
{
	Uint8 name[256];
	Uint8 str[256];

	CMyPrivateChatWin *chatWin = GetChatWinByID(inData->GetInteger(myField_ChatID));
	
	if (chatWin)
	{
		if (chatWin->GetUserListView()->DeleteUserByID(inData->GetInteger(myField_UserID), name))
		{
			if (mOptions.SoundPrefs.leave) USound::Play(nil, 131, false);
			
			if (mOptions.bShowJoinLeaveInChat)
			{
				str[0] = UText::Format(str+1, sizeof(str)-1, "\r <<<  %#s has left the chat", name);
				AddChatText(chatWin, str+1, str[0], false);

				if(mOptions.bShowDateTime)
				{
					Uint8 dt[64];
					dt[0] = UDateTime::DateToText(dt+1, sizeof(dt)-1, kShortDateText + kTimeWithSecsText);
					str[0] = UText::Format(str+1, sizeof(str)-1, " at %#s     >>>", dt);
					AddChatText(chatWin, str+1, str[0], false);
				}
			}
		}
	}
}

void CMyApplication::ProcessTran_NotifyChatSubject(TFieldData inData)
{
	CMyPrivateChatWin *chatWin = GetChatWinByID(inData->GetInteger(myField_ChatID));
	
	if (chatWin)
	{
		Uint8 str[256];
		inData->GetPString(myField_ChatSubject, str, sizeof(str));
		chatWin->SetSubject(str);
	}
}

void CMyApplication::ProcessTran_UserAccess(TFieldData inData)
{
	inData->GetField(myField_UserAccess, mMyAccess.data, sizeof(mMyAccess.data));
	
	// set toobar
	if (mToolbarWin) 
		mToolbarWin->SetAccess();
	
	// set users window
	if (mUsersWin) 
		mUsersWin->SetAccess();
	
	// set file windows
	Uint32 i = 0;
	CMyItemsWin *pItemsWin;
	while(mFileWinList.GetNext(pItemsWin, i))
		pItemsWin->SetAccess();
	
	// set news category windows
	i = 0;
	while (mCategoryWinList.GetNext(pItemsWin, i))
		pItemsWin->SetAccess();	

	// set news article windows
	i = 0;
	CMyNewsArticleTreeWin *pArticleTreeWin;
	while (mArticleTreeWinList.GetNext(pArticleTreeWin, i))
		pArticleTreeWin->SetAccess();

	// set user info windows
	i = 0;
	CMyUserInfoWin *pUserInfoWin;
	while (mUserInfoWinList.GetNext(pUserInfoWin, i))
		pUserInfoWin->SetAccess();

	// set all private chat windows
	CLink *link = CMyPrivateChatWin::gList.GetFirst();
	while (link)
	{
		((CMyPrivateChatWin *)link)->SetAccess();
		link = link->GetNext();
	}
}

void CMyApplication::ProcessTran_UserBroadcast(TFieldData inData)
{
if (!mOptions.bRefuseBroadcast)
	ShowStandardMessage(inData, "\pBroadcast", true);
}

void CMyApplication::ProcessTran_AdminSpector(TFieldData inData)
{

	ShowAdminSpector(inData);
}


void CMyApplication::ProcessTran_DownloadInfo(TFieldData inData)
{
	Uint32 nRefNum = inData->GetInteger(myField_RefNum);
	Uint32 nWaitingCount = inData->GetInteger(myField_WaitingCount);

	Uint32 i = 0;
	CMyTask *task = nil;
	
	while (mTaskList.GetNext(task, i))
	{
		CMyDownloadTask *pDownloadTask = dynamic_cast<CMyDownloadTask *>(task);
		if (pDownloadTask && !pDownloadTask->IsQueued() && pDownloadTask->ShowWaitingCount(nRefNum, nWaitingCount))
			break;
	}
}

void CMyApplication::DoOpenDloadFolder()
{
    UFileSys::Dispose(GetDownloadFolder()); // make sure the folder exists
    UInt8 name[400];
    UInt32 z = UFileSys::GetApplicationURL(name,400);
    if (z>0)
    {
    	const UInt8 str[] = "\p/Downloads";
    	z += UMemory::Copy(name+z, str+1, min((UInt32)str[0],400-z));
// 		for (UInt8* b= (UInt8*)"/Downloads"; *b!=0; ++b)
// 			name[z++] = *b;
  		name[z] = 0;
//		UDebug::LogToDebugFile("Open Download URL: %s\n", name);
		UTransport::LaunchURL(name, z);
	}
}

void CMyApplication::DoViewDloadFile(UInt8* fName)
{
    UFileSys::Dispose(GetDownloadFolder()); // make sure the folder exists
    UInt8 name[400];
    UInt32 z = UFileSys::GetApplicationURL(name,400);
    if (z>0)
    {
    	const UInt8 str[] = "\p/Downloads/";
    	z += UMemory::Copy(name+z, str+1, min((UInt32)str[0],400-z));
    	z += UMemory::Copy(name+z, fName+1, min((UInt32)fName[0],400-z));
  		name[z] = 0;
//		UDebug::LogToDebugFile("Open Download URL: %s\n", name);
		UTransport::LaunchURL(name, z);
	}
}


