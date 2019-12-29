/*
 * TrackerServViews.cp
 * (c)1999 Hotline Communications
 */

#include "TrackerServ.h"


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */

CMyTrackerListView::CMyTrackerListView(CViewHandler *inHandler, const SRect &inBounds)
	: CGeneralCheckListView(inHandler, inBounds)
{

}
	
void CMyTrackerListView::AddTracker(const SIPAddress &inTrackerIP, bool inIsActive)
{
	SMyTrackerInfo *pTrackerInfo = (SMyTrackerInfo *)UMemory::New(sizeof(SMyTrackerInfo));
	
	pTrackerInfo->nTrackerIP = inTrackerIP;
	pTrackerInfo->nActive = inIsActive ? 1 : 0;

	AddItem(pTrackerInfo);
}

void CMyTrackerListView::UpdateTracker(Uint32 inIndex, const SIPAddress &inTrackerIP, bool inIsActive)
{
	SMyTrackerInfo *pTrackerInfo = GetItem(inIndex);
	if (!pTrackerInfo)
		return;

	pTrackerInfo->nTrackerIP = inTrackerIP;
	pTrackerInfo->nActive = inIsActive ? 1 : 0;

	RefreshItem(inIndex);	
}

Uint32 CMyTrackerListView::GetSelectedTrackerIP(SIPAddress &outTrackerIP)
{
	Uint32 nSelected = GetFirstSelectedItem();
	if (!nSelected)
		return 0;

	SMyTrackerInfo *pTrackerInfo = GetItem(nSelected);
	if (!pTrackerInfo)
		return 0;

	outTrackerIP = pTrackerInfo->nTrackerIP;
	return nSelected;
}
		
void CMyTrackerListView::SetItemSelect(Uint32 inIndex, bool inSelect)
{
	CGeneralCheckListView::SetItemSelect(inIndex, inSelect);
	
	SMyTrackerInfo *pTrackerInfo = GetItem(inIndex);
	if (pTrackerInfo)
		gApp->SelectionChanged(pTrackerInfo->nTrackerIP, inSelect);
}
				
void CMyTrackerListView::CheckChanged(Uint32 inIndex, Uint8 inActive)
{
	SMyTrackerInfo *pTrackerInfo = GetItem(inIndex);
	if (pTrackerInfo)
		gApp->CheckChanged(inIndex, pTrackerInfo->nTrackerIP, inActive);
}

void CMyTrackerListView::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	if (inItem > GetItemCount())
		return;
		
	CGeneralCheckListView::ItemDraw(inItem, inImage, inBounds, inUpdateRect, inOptions);
	
	SMyTrackerInfo *pTrackerInfo = GetItem(inItem);
	if (!pTrackerInfo)
		return;
	
	SColor stTextCol;
	if (IsFocus() && mIsEnabled && mSelectData.GetItem(inItem))
		UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
	else
		UUserInterface::GetSysColor(sysColor_Label, stTextCol);

	inImage->SetInkColor(stTextCol);
	
	SRect rBounds = inBounds;
	rBounds.left += 22;
	rBounds.top += 3;
	rBounds.bottom -= 2;
	
	Uint8 psTrackerIP[16];
	psTrackerIP[0] = UText::Format(psTrackerIP + 1, sizeof(psTrackerIP) - 1, "%d.%d.%d.%d", pTrackerInfo->nTrackerIP.un_IP.stB_IP.nB_IP1, pTrackerInfo->nTrackerIP.un_IP.stB_IP.nB_IP2, pTrackerInfo->nTrackerIP.un_IP.stB_IP.nB_IP3, pTrackerInfo->nTrackerIP.un_IP.stB_IP.nB_IP4);
	
	inImage->DrawText(rBounds, psTrackerIP + 1, psTrackerIP[0], nil, textAlign_Left);
}


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

CMyClickLabelView::CMyClickLabelView(CViewHandler *inHandler, const SRect& inBounds)
	: CLabelView(inHandler, inBounds)
{

}

void CMyClickLabelView::MouseUp(const SMouseMsgData& inInfo)
{
	CLabelView::MouseUp(inInfo);
	
	if (IsMouseWithin())
		Hit((inInfo.mods & modKey_Option) ? hitType_Alternate : hitType_Standard);
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

#if NEW_TRACKER
CMyLoginListView::CMyLoginListView(CViewHandler *inHandler, const SRect &inBounds, CTextView *inLoginText, CTextView *inPassText)
#else
CMyLoginListView::CMyLoginListView(CViewHandler *inHandler, const SRect &inBounds, CTextView *inPassText)
#endif
	: CGeneralCheckListView(inHandler, inBounds)
{
#if NEW_TRACKER
	mLoginText = inLoginText;
#endif
	mPassText = inPassText;

	mLoginListIndex = 0;
}

Uint32 CMyLoginListView::AddLogin()
{
#if NEW_TRACKER
	Uint8 bufLogin[32];
	bufLogin[0] = mLoginText->GetText(bufLogin + 1, sizeof(bufLogin) - 1);
	if (!bufLogin[0])
		return 0;
#endif

	Uint8 bufPass[32];
	bufPass[0] = mPassText->GetText(bufPass + 1, sizeof(bufPass) - 1);
	if (!bufPass[0])
		return 0;
	
	if (GetItemCount() >= kMaxLogins)
	{
#if NEW_TRACKER
		if (!ModifyLogin(1, bufLogin, bufPass, true))
#else
		if (!ModifyLogin(1, bufPass, true))
#endif
			return 0;
			
		mLoginListIndex = 0;
		
		DeselectAll();
		SelectItem(1);
		
		return 1;
	}
	
	SMyLoginInfo *pLoginInfo = (SMyLoginInfo *)UMemory::NewClear(sizeof(SMyLoginInfo));
	if (!pLoginInfo)
		return 0;
		
	pLoginInfo->nActive = 1;
#if NEW_TRACKER
	UMemory::Copy(pLoginInfo->psLogin, bufLogin, bufLogin[0] + 1);
#endif
	UMemory::Copy(pLoginInfo->psPassword, bufPass, bufPass[0] + 1);
	pLoginInfo->psDateTime[0] = UDateTime::DateToText(pLoginInfo->psDateTime + 1, sizeof(pLoginInfo->psDateTime) - 1, kShortDateFullYearText + kTimeWithSecsText);

	mLoginListIndex = 0;
	
	return AddItem(pLoginInfo);
}

#if NEW_TRACKER
bool CMyLoginListView::ModifyLogin(Uint32 inIndex, Uint8 *inLogin, Uint8 *inPass, bool inActive)
#else
bool CMyLoginListView::ModifyLogin(Uint32 inIndex, Uint8 *inPass, bool inActive)
#endif
{
	if (inIndex > GetItemCount())
		return false;

	SMyLoginInfo *pLoginInfo = GetItem(inIndex);
	if (!pLoginInfo)
		return false;
		
	if (inActive)
		pLoginInfo->nActive = 1;
	
#if NEW_TRACKER
	if (inLogin && UMemory::Compare(pLoginInfo->psLogin + 1, pLoginInfo->psLogin[0], inLogin + 1, inLogin[0]))
		UMemory::Copy(pLoginInfo->psLogin, inLogin, inLogin[0] + 1);
#endif
	
	if (inPass && UMemory::Compare(pLoginInfo->psPassword + 1, pLoginInfo->psPassword[0], inPass + 1, inPass[0]))
		UMemory::Copy(pLoginInfo->psPassword, inPass, inPass[0] + 1);

	RefreshItem(inIndex);
	return true;
}

bool CMyLoginListView::ModifySelectedLogin()
{
	if (!mLoginListIndex)
		return false;
	
#if NEW_TRACKER
	Uint8 bufLogin[32];
	bufLogin[0] = mLoginText->GetText(bufLogin + 1, sizeof(bufLogin) - 1);
	mLoginText->SetText(nil, 0);
#endif

	Uint8 bufPass[32];
	bufPass[0] = mPassText->GetText(bufPass + 1, sizeof(bufPass) - 1);
	mPassText->SetText(nil, 0);

	Uint32 nIndex = mLoginListIndex;
	mLoginListIndex = 0;
		
#if NEW_TRACKER
	return ModifyLogin(nIndex, bufLogin, bufPass);
#else
	return ModifyLogin(nIndex, bufPass);
#endif
}

bool CMyLoginListView::DeleteSelectedLogin()
{
	mLoginListIndex = 0;

#if NEW_TRACKER
	mLoginText->SetText(nil, 0);
#endif
	mPassText->SetText(nil, 0);

	return DeleteSelectedItem();
}

void CMyLoginListView::SetItemSelect(Uint32 inItem, bool inSelect)
{
	ModifySelectedLogin();

	CGeneralCheckListView::SetItemSelect(inItem, inSelect);

	SMyLoginInfo *pLoginInfo = nil;
	mLoginListIndex = GetSelectedItem(&pLoginInfo);

	if (mLoginListIndex && pLoginInfo)
	{
	#if NEW_TRACKER
		mLoginText->SetText(pLoginInfo->psLogin + 1, pLoginInfo->psLogin[0]);
	#endif
		mPassText->SetText(pLoginInfo->psPassword + 1, pLoginInfo->psPassword[0]);
	}
}

void CMyLoginListView::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	if (inItem > GetItemCount())
		return;
	
	CGeneralCheckListView::ItemDraw(inItem, inImage, inBounds, inUpdateRect, inOptions);
	
	SMyLoginInfo *pLoginInfo = GetItem(inItem);
	if (!pLoginInfo)
		return;
	
	SColor stTextCol;
	if (IsFocus() && mIsEnabled && mSelectData.GetItem(inItem))
		UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
	else
		UUserInterface::GetSysColor(sysColor_Label, stTextCol);

	inImage->SetInkColor(stTextCol);
	
	SRect rBounds = inBounds;
	rBounds.top += 3;
	rBounds.bottom -= 2;

#if NEW_TRACKER
	// draw login
	rBounds.left += 29;
	rBounds.right = rBounds.left + 80; 

	inImage->DrawTruncText(rBounds, pLoginInfo->psLogin + 1, pLoginInfo->psLogin[0], nil, textAlign_Left);
	
	// draw password
	rBounds.left = rBounds.right + 5;
	rBounds.right = rBounds.left + 80;

	inImage->DrawTruncText(rBounds, pLoginInfo->psPassword + 1, pLoginInfo->psPassword[0], nil, textAlign_Left);
#else
	// draw password
	rBounds.left += 29;
	rBounds.right = rBounds.left + 165; 

	inImage->DrawTruncText(rBounds, pLoginInfo->psPassword + 1, pLoginInfo->psPassword[0], nil, textAlign_Left);
#endif
	
	// draw date/time
	rBounds.left = rBounds.right + 5;
	rBounds.right = inBounds.right;
	
	inImage->DrawTruncText(rBounds, pLoginInfo->psDateTime + 1, pLoginInfo->psDateTime[0], nil, textAlign_Left);
}


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

CMyPermBanListView::CMyPermBanListView(CViewHandler *inHandler, const SRect &inBounds, CTextView *inBanUrlText, CTextView *inBanDescrText)
	: CGeneralCheckListView(inHandler, inBounds)
{
	mBanUrlText = inBanUrlText;
	mBanDescrText = inBanDescrText;

	mPermBanListIndex = 0;
}

Uint32 CMyPermBanListView::AddPermBan()
{
	Uint8 bufURL[16];
	bufURL[0] = mBanUrlText->GetText(bufURL + 1, sizeof(bufURL) - 1);

	SIPAddress stIPAddress;
	UTransport::TextToIPAddress(bufURL, stIPAddress);
	if (!stIPAddress.un_IP.stDW_IP.nDW_IP)
		return 0;
	
	Uint8 bufDescr[64];
	bufDescr[0] = mBanDescrText->GetText(bufDescr + 1, sizeof(bufDescr) - 1);
	
	if (GetItemCount() >= kMaxPermBans)
	{
		if (!ModifyPermBan(1, bufURL, bufDescr, true))
			return 0;
			
		mPermBanListIndex = 0;
		
		DeselectAll();
		SelectItem(1);
		
		return 1;
	}
	
	SMyPermBanInfo *pPermBanInfo = (SMyPermBanInfo *)UMemory::NewClear(sizeof(SMyPermBanInfo));
	if (!pPermBanInfo)
		return 0;
		
	pPermBanInfo->nActive = 1;
	pPermBanInfo->nAddr = stIPAddress.un_IP.stDW_IP.nDW_IP;
	pPermBanInfo->psDateTime[0] = UDateTime::DateToText(pPermBanInfo->psDateTime + 1, sizeof(pPermBanInfo->psDateTime) - 1, kShortDateFullYearText + kTimeWithSecsText);
	UMemory::Copy(pPermBanInfo->psDescr, bufDescr, bufDescr[0] + 1);

	mPermBanListIndex = 0;
	
	return AddItem(pPermBanInfo);
}

bool CMyPermBanListView::ModifyPermBan(Uint32 inIndex, Uint8 *inURL, Uint8 *inDescr, bool inActive)
{
	if (inIndex > GetItemCount())
		return false;

	SMyPermBanInfo *pPermBanInfo = GetItem(inIndex);
	if (!pPermBanInfo)
		return false;
		
	if (inActive)
		pPermBanInfo->nActive = 1;
	
	SIPAddress stIPAddress;
	if(inURL && UTransport::TextToIPAddress(inURL, stIPAddress) && pPermBanInfo->nAddr != stIPAddress.un_IP.stDW_IP.nDW_IP)
		pPermBanInfo->nAddr = stIPAddress.un_IP.stDW_IP.nDW_IP;
	
	if (inDescr && UMemory::Compare(pPermBanInfo->psDescr + 1, pPermBanInfo->psDescr[0], inDescr + 1, inDescr[0]))
		UMemory::Copy(pPermBanInfo->psDescr, inDescr, inDescr[0] + 1);

	RefreshItem(inIndex);
	return true;
}

bool CMyPermBanListView::ModifySelectedPermBan()
{
	if (!mPermBanListIndex)
		return false;
	
	Uint8 bufURL[16];
	bufURL[0] = mBanUrlText->GetText(bufURL + 1, sizeof(bufURL) - 1);
	mBanUrlText->SetText(nil, 0);

	Uint8 bufDescr[64];
	bufDescr[0] = mBanDescrText->GetText(bufDescr + 1, sizeof(bufDescr) - 1);
	mBanDescrText->SetText(nil, 0);

	Uint32 nIndex = mPermBanListIndex;
	mPermBanListIndex = 0;
		
	return ModifyPermBan(nIndex, bufURL, bufDescr);
}

bool CMyPermBanListView::DeleteSelectedPermBan()
{
	mPermBanListIndex = 0;
	mBanUrlText->SetText(nil, 0);
	mBanDescrText->SetText(nil, 0);

	return DeleteSelectedItem();
}

void CMyPermBanListView::SetItemSelect(Uint32 inItem, bool inSelect)
{
	ModifySelectedPermBan();

	CGeneralCheckListView::SetItemSelect(inItem, inSelect);

	SMyPermBanInfo *pPermBanInfo = nil;
	mPermBanListIndex = GetSelectedItem(&pPermBanInfo);

	if (mPermBanListIndex && pPermBanInfo)
	{
		SIPAddress stIPAddress;
		stIPAddress.un_IP.stDW_IP.nDW_IP = pPermBanInfo->nAddr;
		
		Uint8 bufURL[16];
		bufURL[0] = UTransport::IPAddressToText(stIPAddress, bufURL + 1, sizeof(bufURL) - 1);
	
		mBanUrlText->SetText(bufURL + 1, bufURL[0]);
		mBanDescrText->SetText(pPermBanInfo->psDescr + 1, pPermBanInfo->psDescr[0]);
	}
}

void CMyPermBanListView::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	if (inItem > GetItemCount())
		return;
	
	CGeneralCheckListView::ItemDraw(inItem, inImage, inBounds, inUpdateRect, inOptions);
	
	SMyPermBanInfo *pPermBanInfo = GetItem(inItem);
	if (!pPermBanInfo)
		return;
	
	SColor stTextCol;
	if (IsFocus() && mIsEnabled && mSelectData.GetItem(inItem))
		UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
	else
		UUserInterface::GetSysColor(sysColor_Label, stTextCol);

	inImage->SetInkColor(stTextCol);
	
	SRect rBounds = inBounds;
	rBounds.top += 3;
	rBounds.bottom -= 2;

	// draw addr
	rBounds.left += 29;
	rBounds.right = rBounds.left + 106; 
	
	SIPAddress stIPAddress;
	stIPAddress.un_IP.stDW_IP.nDW_IP = pPermBanInfo->nAddr;
		
	Uint8 bufURL[16];
	bufURL[0] = UTransport::IPAddressToText(stIPAddress, bufURL + 1, sizeof(bufURL) - 1);
	
	inImage->DrawText(rBounds, bufURL + 1, bufURL[0], nil, textAlign_Left);
	
	// draw date/time
	rBounds.left = rBounds.right + 5;
	rBounds.right = rBounds.left + 150;
	
	inImage->DrawText(rBounds, pPermBanInfo->psDateTime + 1, pPermBanInfo->psDateTime[0], nil, textAlign_Left);

	// draw description
	rBounds.left = rBounds.right + 5;
	rBounds.right = inBounds.right;
	
	inImage->DrawTruncText(rBounds, pPermBanInfo->psDescr + 1, pPermBanInfo->psDescr[0], nil, textAlign_Left);
}

