/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "HotlineServ.h"


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */

#if SHOW_SERVER_INFO_IN_TOOLBAR
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
#endif

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
	Uint8 bufURL[64];
	bufURL[0] = mBanUrlText->GetText(bufURL + 1, sizeof(bufURL) - 1);

	Uint8 bufDescr[64];
	bufDescr[0] = mBanDescrText->GetText(bufDescr + 1, sizeof(bufDescr) - 1);
	
	BanRecord *pPermBanInfo = 0;
	bool isNewItem =  true;
	if (GetItemCount() >= kMaxPermBans)
	{
		DeselectAll();
		pPermBanInfo = GetItem(1);
		isNewItem = false;
	}
	else
		pPermBanInfo = new BanRecord();
		
	pPermBanInfo->nActive = 1;
	pPermBanInfo->IPrangeFromString(bufURL);
	pPermBanInfo->psDateTime[0] = UDateTime::DateToText(
			pPermBanInfo->psDateTime + 1, 
			sizeof(pPermBanInfo->psDateTime) - 1, 
			kShortDateFullYearText + kTimeWithSecsText);
	pPermBanInfo->timeout = Uint32(-1);
	pPermBanInfo->description[0] = 
			  bufDescr[0]<sizeof(pPermBanInfo->description) 
			? bufDescr[0] 
			: (sizeof(pPermBanInfo->description)-1);
	UMemory::Copy(pPermBanInfo->description+1, 
				  bufDescr+1, 
				  pPermBanInfo->description[0]);

	mPermBanListIndex = 0;
	Uint32 n = isNewItem ? AddItem(pPermBanInfo) : 0;
	SelectItem(n);
	if (!isNewItem)
		RefreshItem(n);
	return n;
}

bool CMyPermBanListView::ModifyPermBan(Uint32 inIndex,
								       Uint8 *inURL, 
								       Uint8 *inDescr, 
								       bool inActive)
{
	if (inIndex > GetItemCount())
		return false;

	BanRecord *pPermBanInfo = GetItem(inIndex);
	if (!pPermBanInfo)
		return false;
		
	if (inActive)
		pPermBanInfo->nActive = 1;
	
	pPermBanInfo->IPrangeFromString(inURL);
	
	if (inDescr)
		UMemory::Copy(pPermBanInfo->description, inDescr, inDescr[0] + 1);

	RefreshItem(inIndex);
	return true;
}

bool CMyPermBanListView::ModifySelectedPermBan()
{
	if (!mPermBanListIndex)
		return false;
	
	Uint8 bufURL[64];
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

	BanRecord *pPermBanInfo = nil;
	mPermBanListIndex = GetSelectedItem(&pPermBanInfo);

	if (mPermBanListIndex && pPermBanInfo)
	{
		Uint8 bufURL[64];
		pPermBanInfo->IPrangeToString(bufURL);
		mBanUrlText->SetText(bufURL + 1, bufURL[0]);
		mBanDescrText->SetText(pPermBanInfo->description + 1, 
							   pPermBanInfo->description[0]);
	}
}

void CMyPermBanListView::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	if (inItem > GetItemCount())
		return;
	
	CGeneralCheckListView::ItemDraw(inItem, inImage, inBounds, inUpdateRect, inOptions);
	
	BanRecord *pPermBanInfo = GetItem(inItem);
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
	rBounds.left += 19;
	rBounds.right = rBounds.left + 142; 
	
	Uint8 bufURL[64];
	pPermBanInfo->IPrangeToString(bufURL);
	inImage->DrawText(rBounds, bufURL + 1, bufURL[0], nil, textAlign_Left);
	
	// draw date/time
	rBounds.left = rBounds.right + 5;
	rBounds.right = rBounds.left + 124;
	
	inImage->DrawText(rBounds, pPermBanInfo->psDateTime + 1, pPermBanInfo->psDateTime[0], nil, textAlign_Left);

	// draw description
	rBounds.left = rBounds.right + 5;
	rBounds.right = inBounds.right;
	
	inImage->DrawTruncText(rBounds, pPermBanInfo->description + 1, pPermBanInfo->description[0], nil, textAlign_Left);
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -

CMyAccountListView::CMyAccountListView(CViewHandler *inHandler, const SRect& inBounds)
	: CListView(inHandler, inBounds)
{
	mHeadAdminIcon = UIcon::Load(191);
	mDisconnectAdminIcon = UIcon::Load(192);
	mRegularAdminIcon = UIcon::Load(193);
}

CMyAccountListView::~CMyAccountListView()
{
	Uint32 i = 0;
	SMyAccountListItem *pAccountItem;
		
	while (mAccountList.GetNext(pAccountItem, i))
		UMemory::Dispose((TPtr)pAccountItem);

	mAccountList.Clear();

	UIcon::Release(mHeadAdminIcon);
	UIcon::Release(mDisconnectAdminIcon);
	UIcon::Release(mRegularAdminIcon);
}

void CMyAccountListView::SetInfo(const Uint8 *inAdminLogin)
{
	THdl hList = gApp->GetUserFolderList();
	if (!hList)
		return;
	
	Uint8 psLogin[256];
	SMyUserDataFile info;
	Uint32 nAddCount = 0;
	Uint32 nOffset = 0, nTypeCode, nFlags;

	try
	{
		while (UFileSys::GetListNext(hList, nOffset, psLogin, &nTypeCode, nil, nil, nil, nil, &nFlags))
		{
			if (nTypeCode == TB((Uint32)'fldr') && (nFlags & 1) == 0)	// if folder and visible
			{			
				try
				{
					gApp->GetUser(psLogin, info);
				}
				catch(...)
				{
					// don't throw
					continue;
				}
				
				Uint32 nLoginSize = TB(info.loginSize);
				SMyAccountListItem *pAccountItem = (SMyAccountListItem *)UMemory::NewClear(sizeof(SMyAccountListItem) + nLoginSize + 1);

				pAccountItem->stAccess = info.access;
				pAccountItem->psLogin[0] = UMemory::Copy(pAccountItem->psLogin + 1, info.loginData, nLoginSize);

				if (inAdminLogin && !UText::CompareInsensitive(pAccountItem->psLogin + 1, pAccountItem->psLogin[0], inAdminLogin + 1, inAdminLogin[0]))
					pAccountItem->nFlags = 1;
				else if (pAccountItem->stAccess.HasPriv(myAcc_DisconUser))
					pAccountItem->nFlags = 2;
				else if (pAccountItem->stAccess.HasPriv(myAcc_CreateUser) || pAccountItem->stAccess.HasPriv(myAcc_DeleteUser) || pAccountItem->stAccess.HasPriv(myAcc_OpenUser) || pAccountItem->stAccess.HasPriv(myAcc_ModifyUser) || pAccountItem->stAccess.HasPriv(myAcc_GetClientInfo) || pAccountItem->stAccess.HasPriv(myAcc_CannotBeDiscon))
					pAccountItem->nFlags = 3;
					
				mAccountList.AddItem(pAccountItem);
				nAddCount++;				
			}
		}
	}
	catch(...)
	{
		// don't throw
	}

	UMemory::Dispose(hList);	

	if (nAddCount)
	{
		mAccountList.Sort(CompareLogins);
		ItemsInserted(1, nAddCount);
		
		SelectAdminAccount();
	}
}

bool CMyAccountListView::GetInfo(Uint8 *outAdminLogin)
{
	if (!outAdminLogin)
		return false;

	outAdminLogin[0] = 0;
	
	Uint32 nSelected = GetFirstSelectedItem();
	if (!nSelected)
		return false;
	
	SMyAccountListItem *pAccountItem = mAccountList.GetItem(nSelected);
	if (!pAccountItem)
		return false;
		
	outAdminLogin[0] = UMemory::Copy(outAdminLogin + 1, pAccountItem->psLogin + 1, pAccountItem->psLogin[0] > 31 ? 31 : pAccountItem->psLogin[0]);
	if (!pAccountItem->stAccess.HasAllPriv())
		return false;
		
	return true;
}

Uint32 CMyAccountListView::GetItemCount() const
{
	return mAccountList.GetItemCount();
}

Uint32 CMyAccountListView::GetFullHeight() const
{
	if (mHandler && !mAccountList.GetItemCount())
	{
		CScrollerView *pHandler = dynamic_cast<CScrollerView *>(mHandler);
		if (pHandler)
		{
			SRect stBounds;
			pHandler->GetBounds(stBounds);
			
			return stBounds.GetHeight() - 2;
		}		
	}

	return CListView::GetFullHeight();
}

void CMyAccountListView::SelectAdminAccount()
{
	Uint32 i = 0;
	SMyAccountListItem *pAccountItem;
		
	while (mAccountList.GetNext(pAccountItem, i))
	{
		if (pAccountItem->nFlags == 1)
		{
			SelectItem(i);
			MakeItemVisible(i);
			break;
		}
	}
}

void CMyAccountListView::Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth)
{
	UGraphics::SetFont(inImage, kDefaultFont, nil, 9);
#if MACINTOSH
	inImage->SetInkMode(mode_Or);
#endif
	
	CListView::Draw(inImage, inUpdateRect, inDepth);
}

void CMyAccountListView::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	#pragma unused(inUpdateRect, inOptions)
	
	SMyAccountListItem *pAccountItem = mAccountList.GetItem(inItem);
	if (!pAccountItem)
		return;
	
	// get info
	bool bIsActive = IsFocus() && mIsEnabled;
	bool bIsSelected = mSelectData.GetItem(inItem);
	
	SRect stRect;
	SColor stHiliteCol, stTextCol;

	// draw selection
	if (bIsSelected)
	{
		// calc background rect
		stRect.Set(inBounds.left + 1, inBounds.top + 1, inBounds.right - 1, inBounds.bottom);

		UUserInterface::GetHighlightColor(&stHiliteCol);
		inImage->SetInkColor(stHiliteCol);

		if (bIsActive)
			inImage->FillRect(stRect);
		else
			inImage->FrameRect(stRect);
	}

	// draw light lines around this item
	if (inOptions != kDrawItemForDrag)
	{
		UUserInterface::GetSysColor(sysColor_Light, stHiliteCol);
		inImage->SetInkColor(stHiliteCol);
		stRect = inBounds;
		stRect.bottom++;
		inImage->FrameRect(stRect);
	}

	if (bIsActive && bIsSelected)
		UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
	else
		UUserInterface::GetSysColor(sysColor_Label, stTextCol);

	inImage->SetInkColor(stTextCol);
	
	// draw login
	stRect = inBounds;
	stRect.top += 3;
	stRect.bottom -= 2;
	stRect.left += 2;
	stRect.right -= 20;
	
	inImage->DrawTruncText(stRect, pAccountItem->psLogin + 1, pAccountItem->psLogin[0], nil, textAlign_Left);

	// draw icon
	stRect = inBounds;
	stRect.top += 2;
	stRect.bottom = stRect.top + 16;
	stRect.right -= 2;
	stRect.left = stRect.right - 16;
	
	if (pAccountItem->nFlags == 1)
		mHeadAdminIcon->Draw(inImage, stRect, align_Center, bIsActive && bIsSelected ? transform_Dark : transform_None);
	else if (pAccountItem->nFlags == 2)
		mDisconnectAdminIcon->Draw(inImage, stRect, align_Center, bIsActive && bIsSelected ? transform_Dark : transform_None);
	else if (pAccountItem->nFlags == 3)
		mRegularAdminIcon->Draw(inImage, stRect, align_Center, bIsActive && bIsSelected ? transform_Dark : transform_None);
}

Int32 CMyAccountListView::CompareLogins(void *inPtrA, void *inPtrB, void *inRef)
{
	#pragma unused(inRef)

	Uint8 *psLoginA = ((SMyAccountListItem *)inPtrA)->psLogin;
	Uint8 *psLoginB = ((SMyAccountListItem *)inPtrB)->psLogin;

	Int32 nOutVal = UText::CompareInsensitive(psLoginA + 1, psLoginB + 1, min(psLoginA[0], psLoginB[0]));
	if (nOutVal == 0 && psLoginA[0] != psLoginB[0])
		nOutVal = psLoginA[0] > psLoginB[0] ? 1 : -1;
	
	return nOutVal;
}

