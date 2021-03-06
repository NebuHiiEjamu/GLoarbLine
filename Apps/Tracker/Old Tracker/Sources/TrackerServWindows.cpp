/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "TrackerServ.h"


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */

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
	CScrollerView *scr;

	// make container view to hold the views
	vc = new CContainerView(nil, SRect(0,0,390,280));

	try
	{
		// make box
		box = new CBoxView(vc, SRect(10,10,370,166), boxStyle_Etched);
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
				
		lbl = new CLabelView(vc, SRect(20,46,352,164));
		lbl->SetText("\pIf you have multihoming/IP-aliasing configured on your computer, you can specify which address to bind to here.\r\rIf you have only set up a single IP address, or if you wish to use your default address, leave the IP set to 0.0.0.0.");
		lbl->Show();
		
		// make max servers per IP
		lbl = new CLabelView(vc, SRect(10,185,328,201));
		lbl->SetText("\pMax simultaneous servers listed per IP address:");
		lbl->Show();
		scr = MakeTextBoxView(vc, SRect(330,180,370,206), scrollerOption_Border, &mViews.maxServersPerIP);
		scr->Show();
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
		lbl = new CLabelView(vc, SRect(10,193,85,209));
		lbl->SetText("\pPassword:");
		lbl->Show();
	
		// make list scroller view
		scr = new CScrollerView(vc, SRect(10,10,375,180), scrollerOption_Border | scrollerOption_VertBar | scrollerOption_NoBkgnd);
		scr->SetCanFocus(true);
		mViews.loginScroll = scr;
		scr->Show();

		// make text view
		scr = MakeTextBoxView(vc, SRect(85,188,375,214), scrollerOption_Border, &mViews.passText);
		scr->Show();

		// make list view
		mViews.loginList = new CMyLoginListView(mViews.loginScroll, SRect(0,0,mViews.loginScroll->GetVisibleContentWidth(),mViews.loginScroll->GetVisibleContentHeight()), mViews.passText);
		mViews.loginList->SetSizing(sizing_FullHeight | sizing_FullWidth);
		mViews.loginList->SetCanFocus(true);
		mViews.loginList->Show();

		// make buttons
		// make buttons
		btn = new CButtonView(vc, SRect(32,222,108,244));
		btn->SetTitle("\pAdd");
		btn->SetID(3);
		btn->Show();
		btn = new CButtonView(vc, SRect(122,222,198,244));
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

		// make list scroller view
		scr = new CScrollerView(vc, SRect(10,10,375,180), scrollerOption_Border | /*scrollerOption_HorizBar |*/ scrollerOption_VertBar | scrollerOption_NoBkgnd);
		scr->SetCanFocus(true);
		mViews.permBanScroll = scr;
		scr->Show();

		// make text view
		scr = MakeTextBoxView(vc, SRect(32,188,198,214), scrollerOption_Border, &mViews.banUrlText);
		scr->Show();

		// make list view
		mViews.permBanList = new CMyPermBanListView(mViews.permBanScroll, SRect(0,0,mViews.permBanScroll->GetVisibleContentWidth(),mViews.permBanScroll->GetVisibleContentHeight()), mViews.banUrlText);
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

void CMyOptionsWin::SetInfo(SIPAddress inIPAddress, Uint32 inMaxServersPerIP, CPtrList<SMyLoginInfo> *inLoginList, CPtrList<SMyPermBanInfo> *inPermBanList)
{
	Uint8 str[32];
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
	
	if (inLoginList)
		mViews.loginList->SetItemsFromList(inLoginList);

	if (inPermBanList)
		mViews.permBanList->SetItemsFromList(inPermBanList);
}

void CMyOptionsWin::GetInfo(SIPAddress *outIPAddress, Uint32 *outMaxServersPerIP, CPtrList<SMyLoginInfo> *outLoginList, CPtrList<SMyPermBanInfo> *outPermBanList)
{
	if (outIPAddress)
		*outIPAddress = GetIPAddress();
	
	if (outMaxServersPerIP)
	{
		Uint8 str[8];
		str[0] = mViews.maxServersPerIP->GetText(str+1, sizeof(str)-1);
		*outMaxServersPerIP = UText::TextToUInteger(str+1, str[0]);
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
	: CWindow(SRect(0,0,220,102), windowLayer_Standard, windowOption_CloseBox)
{
	CContainerView *vc;
	CLabelView *lbl;
	CButtonView *btn;

	// setup window
	SetTitle("\pHotline Tracker");
	SetAutoBounds(windowPos_TopRight, windowPosOn_WinScreen);
	mServerCount = mUserCount = mListingCounter = 0;

	// make container view for content
	vc = new CContainerView(this, SRect(0,0,220,102));
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
	
	// make button
	btn = new CButtonView(vc, SRect(10,72,100,96));
	btn->SetTitle("\pOptions");
	btn->SetID(viewID_Options);
	btn->Show();
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

#if USE_LOG_WIN
CMyTextWin::CMyTextWin()
	: CWindow(SRect(0,0,520,200), windowLayer_Standard, windowOption_CloseBox + windowOption_Sizeable + windowOption_WhiteBkgnd)
{
	CContainerView *vc;

	// make container view for content
	vc = new CContainerView(this, SRect(0,0,520,200));
	vc->Show();
	
	// make text scroller
	mScrollerView = MakeTextBoxView(vc, SRect(-2,-2,522,202), scrollerOption_PlainBorder + scrollerOption_VertBar + scrollerOption_GrowSpace + scrollerOption_NoBkgnd, &mTextView);
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
#endif
