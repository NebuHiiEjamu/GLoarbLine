/*
 * TrackerServWindows.cp
 * (c)1999 Hotline Communications
 */

#include "TrackerServ.h"

#if WIN32
void _SetWinIcon(TWindow inRef, Int16 inID);
#endif


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */

CMyToolbarWin::CMyToolbarWin()
	: CWindow(SRect(0,0,200,200), windowLayer_Float)
{
	enum {
		buttonHeight = 24,
		buttonWidth = 140,
		space = 3
	};
	CContainerView *vc;
	CIconButtonView *btn;
	SRect r;

	// setup window
	SetTitle("\pHotline Tracker");

	// make container view for content
	vc = new CContainerView(this, SRect(0,0,200,200));
	vc->Show();
	
	const Uint8 *btnNames[] = {
		"\pNew Tracker", "\pEdit Tracker", "\pDelete Tracker", "\pLog", "\pStatistics", "\pServers", "\pQuit"
	};
	const Int16 btnIcons[] = {
		301, 301, 301, 300, 300, 300, 303
	};
	const Uint32 btnIDs[] = {
		viewID_NewTracker, viewID_EditTracker, viewID_DeleteTracker,
		viewID_ShowLog, viewID_ShowStats, viewID_ShowServers, viewID_Quit
	};
	enum { btnCount = sizeof(btnIcons) / sizeof(Int16) };
	CIconButtonView *btns[btnCount];
	
	r.top = 1;
	r.left = space;
	r.right = r.left + buttonWidth;

#if WIN32
	r.bottom = r.top + 49;
#else
	r.bottom = r.top + 37;
#endif

	(new CBoxView(vc, SRect(0, 0, r.right + 4, r.bottom + 1), boxStyle_Sunken))->Show();
	
	// make global stats
	CMyClickLabelView *lbl = new CMyClickLabelView(vc, SRect(1, 1, 98, r.bottom));
	lbl->SetFont(kDefaultFont, nil, 9);
	lbl->SetID(viewID_GlobalStats);
	lbl->SetText("\p Server Count:\r User Count:\r Listing Counter:");
	lbl->Show();

	mViews.globalStats = new CMyClickLabelView(vc, SRect(98, 1, r.right + 3, r.bottom));
	mViews.globalStats->SetFont(kDefaultFont, nil, 9);
	mViews.globalStats->SetID(viewID_GlobalStats);
	mViews.globalStats->Show();

	// make tracker list
	r.top = r.bottom + space;
	r.bottom = r.top + 64;

	CScrollerView *scr = new CScrollerView(vc, SRect(0, r.top, r.right + 4, r.bottom), scrollerOption_Border | scrollerOption_VertBar | scrollerOption_NoBkgnd);
	scr->SetCanFocus(true);
	scr->Show();

	mViews.trackerList = new CMyTrackerListView(scr, SRect(0, 0, scr->GetVisibleContentWidth(), scr->GetVisibleContentHeight()));
	mViews.trackerList->SetSizing(sizing_FullHeight | sizing_FullWidth);
	mViews.trackerList->SetCanFocus(true);
	mViews.trackerList->SetID(viewID_TrackerList);
	mViews.trackerList->Show();

	// make buttons
	r.top = r.bottom + space;

	for (Uint16 i=0; i<btnCount; i++)
	{
		r.bottom = r.top + buttonHeight;
		
		btn = new CIconButtonView(vc, r);
		btn->SetOptions(iconBtn_MediumTitleLeft);
	#if WIN32
		btn->SetCanFocus(true);
	#endif
		btn->SetIconID(btnIcons[i]);
		btn->SetTitle(btnNames[i]);
		btn->SetID(btnIDs[i]);
		btn->Show();
		
		btns[i] = btn;
		r.top += buttonHeight + space;
	}
	
	// resize window
	r.top = r.left = 0;
	r.bottom += space;
	r.right = space + buttonWidth + space - 1;
	vc->SetBounds(r);
	SetBounds(r);
	
	mTrueHeight = r.GetHeight();
	
	SetGlobalStats(0,0,0);
}

void CMyToolbarWin::AddTracker(const SIPAddress &inTrackerIP, bool inIsActive)
{	
	mViews.trackerList->AddTracker(inTrackerIP, inIsActive);
}

void CMyToolbarWin::UpdateTracker(Uint32 inIndex, const SIPAddress &inTrackerIP, bool inIsActive)
{
	mViews.trackerList->UpdateTracker(inIndex, inTrackerIP, inIsActive);
}

Uint32 CMyToolbarWin::GetSelectedTrackerIP(SIPAddress &outTrackerIP)
{
	return mViews.trackerList->GetSelectedTrackerIP(outTrackerIP);
}

void CMyToolbarWin::SelectFirstTracker()
{
	if (mViews.trackerList->GetItemCount())
	{
		mViews.trackerList->DeselectAll();
		mViews.trackerList->SelectItem(1);
		mViews.trackerList->MakeItemVisible(1);
	}
}

void CMyToolbarWin::DeleteSelectedTracker()
{
	mViews.trackerList->DeleteSelectedItem();
}

void CMyToolbarWin::SetGlobalStats(Uint32 inServerCount, Uint32 inUserCount, Uint32 inListingCounter)
{
	Uint8 psGlobalStats[256];
	psGlobalStats[0] = UText::Format(psGlobalStats + 1, sizeof(psGlobalStats) - 1, "%lu\r%lu\r%lu", inServerCount, inUserCount, inListingCounter);

	mViews.globalStats->SetText(psGlobalStats);
}

bool CMyToolbarWin::IsCollapsed()
{
	SRect bounds;
	GetBounds(bounds);
	
	if(bounds.GetHeight() < mTrueHeight)
		return true;
		
	return false;
}

void CMyToolbarWin::ToggleState()
{
	SRect bounds;
	GetBounds(bounds);
	
	if(bounds.GetHeight() < mTrueHeight)
		bounds.bottom = bounds.top + mTrueHeight;
	else
	#if WIN32
		bounds.bottom = bounds.top + 116;
	#else
		bounds.bottom = bounds.top + 106;
	#endif
	
	SetBounds(bounds);
	SetAutoBounds(windowPos_Best, windowPosOn_WinScreen);
}


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

CMyOptionsWin::CMyOptionsWin()
	: CWindow(SRect(0,0,410,336), windowLayer_Modal)
{
	CContainerView *vc;
	CTabbedView *tabs;
	
	// setup window
	SetTitle("\pOptions");
	SetAutoBounds(windowPos_Center, windowPosOn_WinScreen);

	// make container view for content
	vc = new CContainerView(this, SRect(0,0,410,336));
	vc->Show();

	// make tab view
	tabs = new CTabbedView(vc, SRect(10,10,400,290));
	tabs->SetFont(UFontDesc::New(kDefaultFont, nil, 10));
	
	// add the tabs
	tabs->AddTab("\pGeneral");
	tabs->AddTab("\pPassword");
	tabs->AddTab("\pBan");
	
	// attach views to tabs
	tabs->SetTabView(1, MakeGeneralTab());
	tabs->SetTabView(2, MakePasswordTab());
	tabs->SetTabView(3, MakeBanTab());
	tabs->SetCurrentTab(1);
	tabs->Show();

	// make buttons
	SButtons btns[] = {{1, "\pSave", btnOpt_Default, nil}, {2, "\pCancel", btnOpt_Cancel, nil}};
	CButtonView::BuildButtons(vc, SRect(200,300,380,326), btns);
}

CContainerView *CMyOptionsWin::MakeGeneralTab()
{
	CContainerView *vc;
	CBoxView *box;
	CLabelView *lbl;
	CCheckBoxView *chk;
	CScrollerView *scr;

	// make container view to hold the views
	vc = new CContainerView(nil, SRect(0,0,390,280));

	try
	{
		// make box
		box = new CBoxView(vc, SRect(10,10,370,100), boxStyle_Etched);
		box->Show();

		// make label view
		lbl = new CLabelView(vc, SRect(20,21,160,37));
		lbl->SetText("\pTracker IP:");
		lbl->Show();
		
		// make IP text views
		scr = MakeTextBoxView(vc, SRect(162,16,202,42), scrollerOption_Border, &mViews.ip1Text);
		scr->Show();
		lbl = new CLabelView(vc, SRect(204,26,210,42));
		lbl->SetText("\p.");
		lbl->Show();
		scr = MakeTextBoxView(vc, SRect(212,16,252,42), scrollerOption_Border, &mViews.ip2Text);
		scr->Show();
		lbl = new CLabelView(vc, SRect(254,26,260,42));
		lbl->SetText("\p.");
		lbl->Show();
		scr = MakeTextBoxView(vc, SRect(262,16,302,42), scrollerOption_Border, &mViews.ip3Text);
		scr->Show();
		lbl = new CLabelView(vc, SRect(304,26,310,42));
		lbl->SetText("\p.");
		lbl->Show();
		scr = MakeTextBoxView(vc, SRect(312,16,352,42), scrollerOption_Border, &mViews.ip4Text);
		scr->Show();
				
		lbl = new CLabelView(vc, SRect(20, 46, 352, 98));
		lbl->SetText("\pIf you have multihoming/IP-aliasing configured on your computer, you can specify which address to bind to here.");
		lbl->Show();
		
		// make max servers per IP
		lbl = new CLabelView(vc, SRect(10,111,328,127));
		lbl->SetText("\pMax simultaneous servers listed per IP address:");
		lbl->Show();
		scr = MakeTextBoxView(vc, SRect(330,106,370,132), scrollerOption_Border, &mViews.maxServersPerIP);
		scr->Show();

		// make checkboxes
		chk = new CCheckBoxView(vc, SRect(10,138,220,156));
		chk->SetTitle("\pLog To File");
		chk->SetAutoMark(true);
		chk->Show();
		mViews.logToFile = chk;
		chk = new CCheckBoxView(vc, SRect(10,158,220,176));
		chk->SetTitle("\pLog Server Connects");
		chk->SetAutoMark(true);
		chk->Show();
		mViews.logServerConnect = chk;
		chk = new CCheckBoxView(vc, SRect(10,178,220,196));
		chk->SetTitle("\pLog Client Connects");
		chk->SetAutoMark(true);
		chk->Show();
		mViews.logClientConnect = chk;
		chk = new CCheckBoxView(vc, SRect(10,198,220,216));
		chk->SetTitle("\pLog Tracker Start");
		chk->SetAutoMark(true);
		chk->Show();
		mViews.logTrackerStart = chk;
		chk = new CCheckBoxView(vc, SRect(10,218,220,236));
		chk->SetTitle("\pLog Tracker Stop");
		chk->SetAutoMark(true);
		chk->Show();
		mViews.logTrackerStop = chk;
	}
	catch(...)
	{
		// clean up
		delete vc;
		throw;
	}
	
	return vc;
}

CContainerView *CMyOptionsWin::MakePasswordTab()
{
	CContainerView *vc;
	CLabelView *lbl;
	CButtonView *btn;
	CScrollerView *scr;
	
	// make container view to hold the views
	vc = new CContainerView(nil, SRect(0,0,390,280));

	try
	{	
		// make label view
	#if NEW_TRACKER
		lbl = new CLabelView(vc, SRect(10,193,85,209));
		lbl->SetText("\pLogin:");
		lbl->Show();
		lbl = new CLabelView(vc, SRect(10,225,85,241));
		lbl->SetText("\pPassword:");
		lbl->Show();
	#else
		lbl = new CLabelView(vc, SRect(10,193,85,209));
		lbl->SetText("\pPassword:");
		lbl->Show();
	#endif
	
		// make list scroller view
		scr = new CScrollerView(vc, SRect(10,10,375,180), scrollerOption_Border | scrollerOption_VertBar | scrollerOption_NoBkgnd);
		scr->SetCanFocus(true);
		mViews.loginScroll = scr;
		scr->Show();

		// make text view
	#if NEW_TRACKER
		scr = MakeTextBoxView(vc, SRect(85,188,280,214), scrollerOption_Border, &mViews.loginText);
		scr->Show();
		scr = MakeTextBoxView(vc, SRect(85,220,280,246), scrollerOption_Border, &mViews.passText);
		scr->Show();
	#else
		scr = MakeTextBoxView(vc, SRect(85,188,280,214), scrollerOption_Border, &mViews.passText);
		scr->Show();
	#endif

		// make list view
	#if NEW_TRACKER
		mViews.loginList = new CMyLoginListView(mViews.loginScroll, SRect(0,0,mViews.loginScroll->GetVisibleContentWidth(),mViews.loginScroll->GetVisibleContentHeight()), mViews.loginText, mViews.passText);
	#else
		mViews.loginList = new CMyLoginListView(mViews.loginScroll, SRect(0,0,mViews.loginScroll->GetVisibleContentWidth(),mViews.loginScroll->GetVisibleContentHeight()), mViews.passText);
	#endif
		mViews.loginList->SetSizing(sizing_FullHeight | sizing_FullWidth);
		mViews.loginList->SetCanFocus(true);
		mViews.loginList->Show();

		// make buttons
		btn = new CButtonView(vc, SRect(299,190,375,212));
		btn->SetTitle("\pAdd");
		btn->SetID(3);
		btn->Show();
		btn = new CButtonView(vc, SRect(299,222,375,244));
		btn->SetTitle("\pDelete");
		btn->SetID(4);
		btn->Show();
	}
	catch(...)
	{
		// clean up
		delete vc;
		throw;
	}
	
	return vc;
}

CContainerView *CMyOptionsWin::MakeBanTab()
{
	CContainerView *vc;
	CLabelView *lbl;
	CButtonView *btn;
	CScrollerView *scr;
	
	// make container view to hold the views
	vc = new CContainerView(nil, SRect(0,0,390,280));

	try
	{	
		// make label view
		lbl = new CLabelView(vc, SRect(10,193,32,209));
		lbl->SetText("\pIP:");
		lbl->Show();

		lbl = new CLabelView(vc, SRect(170,193,204,209));
		lbl->SetText("\pInfo:");
		lbl->Show();

		// make list scroller view
		scr = new CScrollerView(vc, SRect(10,10,375,180), scrollerOption_Border | /*scrollerOption_HorizBar |*/ scrollerOption_VertBar | scrollerOption_NoBkgnd);
		scr->SetCanFocus(true);
		mViews.permBanScroll = scr;
		scr->Show();

		// make text view
		scr = MakeTextBoxView(vc, SRect(32,188,162,214), scrollerOption_Border, &mViews.banUrlText);
		scr->Show();
		scr = MakeTextBoxView(vc, SRect(204,188,375,214), scrollerOption_Border, &mViews.banDescrText);
		scr->Show();

		// make list view
		mViews.permBanList = new CMyPermBanListView(mViews.permBanScroll, SRect(0,0,mViews.permBanScroll->GetVisibleContentWidth(),mViews.permBanScroll->GetVisibleContentHeight()), mViews.banUrlText, mViews.banDescrText);
		mViews.permBanList->SetSizing(sizing_FullHeight | sizing_FullWidth);
		mViews.permBanList->SetCanFocus(true);
		mViews.permBanList->Show();

		// make buttons
		btn = new CButtonView(vc, SRect(32,222,108,244));
		btn->SetTitle("\pAdd");
		btn->SetID(5);
		btn->Show();
		btn = new CButtonView(vc, SRect(122,222,198,244));
		btn->SetTitle("\pDelete");
		btn->SetID(6);
		btn->Show();
	}
	catch(...)
	{
		// clean up
		delete vc;
		throw;
	}
	
	return vc;
}

void CMyOptionsWin::SetInfo(SIPAddress inIPAddress, Uint32 inMaxServersPerIP, Uint32 inOpts, CPtrList<SMyLoginInfo> *inLoginList, CPtrList<SMyPermBanInfo> *inPermBanList)
{
	Uint8 str[32];
	
	str[0] = UText::Format(str + 1, sizeof(str) - 1, "Options - %d.%d.%d.%d", inIPAddress.un_IP.stB_IP.nB_IP1, inIPAddress.un_IP.stB_IP.nB_IP2, inIPAddress.un_IP.stB_IP.nB_IP3, inIPAddress.un_IP.stB_IP.nB_IP4);
	SetTitle(str);

	str[0] = UText::IntegerToText(str+1, sizeof(str)-1, inIPAddress.un_IP.stB_IP.nB_IP1);
	mViews.ip1Text->SetText(str+1, str[0]);
	str[0] = UText::IntegerToText(str+1, sizeof(str)-1, inIPAddress.un_IP.stB_IP.nB_IP2);
	mViews.ip2Text->SetText(str+1, str[0]);
	str[0] = UText::IntegerToText(str+1, sizeof(str)-1, inIPAddress.un_IP.stB_IP.nB_IP3);
	mViews.ip3Text->SetText(str+1, str[0]);
	str[0] = UText::IntegerToText(str+1, sizeof(str)-1, inIPAddress.un_IP.stB_IP.nB_IP4);
	mViews.ip4Text->SetText(str+1, str[0]);
	
	str[0] = UText::IntegerToText(str+1, sizeof(str)-1, inMaxServersPerIP);
	mViews.maxServersPerIP->SetText(str+1, str[0]);
	
	mViews.logToFile->SetMark((inOpts & myOpt_LogToFile) != 0);
	mViews.logServerConnect->SetMark((inOpts & myOpt_LogServerConnect) != 0);
	mViews.logClientConnect->SetMark((inOpts & myOpt_LogClientConnect) != 0);
	mViews.logTrackerStart->SetMark((inOpts & myOpt_LogTrackerStart) != 0);
	mViews.logTrackerStop->SetMark((inOpts & myOpt_LogTrackerStop) != 0);

	if (inLoginList)
		mViews.loginList->SetItemsFromList(inLoginList);

	if (inPermBanList)
		mViews.permBanList->SetItemsFromList(inPermBanList);
}

void CMyOptionsWin::GetInfo(SIPAddress *outIPAddress, Uint32 *outMaxServersPerIP, Uint32 *outOpts, CPtrList<SMyLoginInfo> *outLoginList, CPtrList<SMyPermBanInfo> *outPermBanList)
{
	if (outIPAddress)
		*outIPAddress = GetIPAddress();
	
	if (outMaxServersPerIP)
	{
		Uint8 str[32];
		str[0] = mViews.maxServersPerIP->GetText(str+1, sizeof(str)-1);
		*outMaxServersPerIP = UText::TextToUInteger(str+1, str[0]);
	}
	
	if (outOpts)
	{
		*outOpts = 0;		
		if (mViews.logToFile->GetMark()) *outOpts |= myOpt_LogToFile;
		if (mViews.logServerConnect->GetMark()) *outOpts |= myOpt_LogServerConnect;
		if (mViews.logClientConnect->GetMark()) *outOpts |= myOpt_LogClientConnect;
		if (mViews.logTrackerStart->GetMark()) *outOpts |= myOpt_LogTrackerStart;
		if (mViews.logTrackerStop->GetMark()) *outOpts |= myOpt_LogTrackerStop;
	}

	if (outLoginList)
		mViews.loginList->GetItemsFromList(outLoginList);

	if (outPermBanList)
		mViews.permBanList->GetItemsFromList(outPermBanList);
}

SIPAddress CMyOptionsWin::GetIPAddress()
{
	Uint8 str[8];

	SIPAddress stIPAddress;
	stIPAddress.un_IP.stDW_IP.nDW_IP = 0;
	Uint32 nIPAddress;
		
	str[0] = mViews.ip1Text->GetText(str+1, sizeof(str)-1);
	nIPAddress = UText::TextToUInteger(str+1, str[0]);
	if (nIPAddress < 256) stIPAddress.un_IP.stB_IP.nB_IP1 = nIPAddress;
		
	str[0] = mViews.ip2Text->GetText(str+1, sizeof(str)-1);
	nIPAddress = UText::TextToUInteger(str+1, str[0]);
	if (nIPAddress < 256) stIPAddress.un_IP.stB_IP.nB_IP2 = nIPAddress;

	str[0] = mViews.ip3Text->GetText(str+1, sizeof(str)-1);
	nIPAddress = UText::TextToUInteger(str+1, str[0]);
	if (nIPAddress < 256) stIPAddress.un_IP.stB_IP.nB_IP3 = nIPAddress;
		
	str[0] = mViews.ip4Text->GetText(str+1, sizeof(str)-1);
	nIPAddress = UText::TextToUInteger(str+1, str[0]);
	if (nIPAddress < 256) stIPAddress.un_IP.stB_IP.nB_IP4 = nIPAddress;
	
	return stIPAddress;
}

void CMyOptionsWin::AddLogin()
{
	Uint32 nIndex = mViews.loginList->AddLogin();
	if (!nIndex)
		return;
	
	if(nIndex == 1)
		mViews.loginScroll->ScrollToTop();
	else
		mViews.loginScroll->ScrollToBottom();
}

void CMyOptionsWin::DeleteLogin()
{
	mViews.loginList->DeleteSelectedLogin();
}

void CMyOptionsWin::AddPermBan()
{
	Uint32 nIndex = mViews.permBanList->AddPermBan();
	if (!nIndex)
		return;
	
	if(nIndex == 1)
		mViews.permBanScroll->ScrollToTop();
	else
		mViews.permBanScroll->ScrollToBottom();
}

void CMyOptionsWin::DeletePermBan()
{
	mViews.permBanList->DeleteSelectedPermBan();
}


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

CMyStatsWin::CMyStatsWin()
	: CWindow(SRect(0,0,220,80), windowLayer_Standard, windowOption_CloseBox)
{
#if WIN32
	_SetWinIcon(*this, 206);
#endif

	CContainerView *vc;
	CLabelView *lbl;

	// setup window
	SetAutoBounds(windowPos_TopRight, windowPosOn_WinScreen);
	mServerCount = mUserCount = mListingCounter = 0;

	// make container view for content
	vc = new CContainerView(this, SRect(0,0,220,80));
	vc->Show();

	// make labels
	lbl = new CLabelView(vc, SRect(10,10,130,26));
	lbl->SetText("\pServer Count:");
	lbl->Show();
	lbl = new CLabelView(vc, SRect(160,10,200,26));
	lbl->SetText("\p0");
	lbl->Show();
	mViews.servCountText = lbl;
	lbl = new CLabelView(vc, SRect(10,30,130,46));
	lbl->SetText("\pUser Count:");
	lbl->Show();
	lbl = new CLabelView(vc, SRect(160,30,200,46));
	lbl->SetText("\p0");
	lbl->Show();
	mViews.userCountText = lbl;
	lbl = new CLabelView(vc, SRect(10,50,130,66));
	lbl->SetText("\pListing Counter:");
	lbl->Show();
	lbl = new CLabelView(vc, SRect(160,50,220,66));
	lbl->SetText("\p0");
	lbl->Show();
	mViews.listCountText = lbl;
}

void CMyStatsWin::SetServerCount(Uint32 inNum)
{
	if (inNum != mServerCount)
	{
		Uint8 str[32];
		str[0] = UText::IntegerToText(str+1, sizeof(str)-1, inNum);
		mViews.servCountText->SetText(str);
		mServerCount = inNum;
	}
}

void CMyStatsWin::SetUserCount(Uint32 inNum)
{
	if (inNum != mUserCount)
	{
		Uint8 str[32];
		str[0] = UText::IntegerToText(str+1, sizeof(str)-1, inNum);
		mViews.userCountText->SetText(str);
		mUserCount = inNum;
	}
}

void CMyStatsWin::SetListingCounter(Uint32 inNum)
{
	if (inNum != mListingCounter)
	{
		Uint8 str[32];
		str[0] = UText::IntegerToText(str+1, sizeof(str)-1, inNum);
		mViews.listCountText->SetText(str);
		mListingCounter = inNum;
	}
}


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

CMyTextWin::CMyTextWin()
	: CWindow(SRect(0,0,540,180), windowLayer_Standard, windowOption_CloseBox + windowOption_Sizeable + windowOption_WhiteBkgnd)
{
#if WIN32
	_SetWinIcon(*this, 206);
#endif

	CContainerView *vc;

	// make container view for content
	vc = new CContainerView(this, SRect(0,0,540,180));
	vc->Show();
	
	// make text scroller
	mScrollerView = MakeTextBoxView(vc, SRect(-2,-2,542,182), scrollerOption_PlainBorder + scrollerOption_VertBar + scrollerOption_GrowSpace + scrollerOption_NoBkgnd, &mTextView);
	mScrollerView->SetSizing(sizing_BottomRightSticky);
	mScrollerView->Show();
	mTextView->SetSizing(sizing_FullHeight + sizing_RightSticky);
#if MACINTOSH
	mTextView->SetFont(kFixedFont, nil, 9);
#else
	mTextView->SetFont(kFixedFont, nil, 12);
#endif
	mTextView->SetEditable(false);
}

