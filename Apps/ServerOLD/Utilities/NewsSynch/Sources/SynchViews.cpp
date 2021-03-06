/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "NewsSynch.h"

extern CMyApplication *gApp;


#pragma mark CMyGroupArticle
CMyGroupArticle::CMyGroupArticle(bool inGroupArticle)
{
	if (inGroupArticle)
		mRootIcon = UIcon::Load(413);
	else
		mRootIcon = UIcon::Load(223);
	
	mDropIndex = 0;
	mKeyDownTime = 0;
	mSelectTimer = nil;
}

CMyGroupArticle::~CMyGroupArticle()
{
	UIcon::Release(mRootIcon);
	UTimer::Dispose(mSelectTimer);
}

void CMyGroupArticle::StartSelectTimer()
{
	if (mSelectTimer == nil)
		mSelectTimer = UTimer::New(SelectTimerProc, this);
		
	mSelectTimer->Start(500, kOnceTimer);
}

void CMyGroupArticle::SelectTimerProc(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	#pragma unused(inObject, inData, inDataSize)
	
	if (inMsg == msg_Timer)
		((CMyGroupArticle *)inContext)->ExecuteSelectTimer();
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyGroupListView1

CMyGroupListView1::CMyGroupListView1(CViewHandler *inHandler, const SRect &inBounds)
	: CMyListStatusView(inHandler, inBounds), CMyGroupArticle(true)
{
	// set headers
	AddTab("\pName", inBounds.GetWidth() - 40, 80);
	AddTab("\pSize", 1, 1, align_CenterHoriz);

	// set status
	SetStatus("\pClick the \"Select News Server\" button or type the address and hit \"Enter\" to connect to a News Server.");
}

CMyGroupListView1::~CMyGroupListView1()
{
	DeleteAll();
}

void CMyGroupListView1::SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2)
{
	if (!inTabPercent1 && !inTabPercent2)
		return;
	
	CPtrList<Uint8> lPercentList;
	lPercentList.AddItem(&inTabPercent1);
	lPercentList.AddItem(&inTabPercent2);
	
	SetTabPercent(lPercentList);
	lPercentList.Clear();
}

void CMyGroupListView1::GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2)
{
	outTabPercent1 = GetTabPercent(1);
	outTabPercent2 = GetTabPercent(2);
}

void CMyGroupListView1::SearchText(const Uint8 *inText)
{
	Uint32 i = 0;
	SMyGroupInfo1 *pGroupInfo;

	while (mGeneralList.GetNext(pGroupInfo, i))
	{
		if (UText::SearchInsensitive(inText + 1, inText[0], pGroupInfo->stGroupInfo.psGroupName + 1, pGroupInfo->stGroupInfo.psGroupName[0]))
		{
			if (i != GetFirstSelectedItem())
			{
				DeselectAll();
				SelectItem(i);
				MakeItemVisible(i);
			}

			return;
		}
	}
}

void CMyGroupListView1::SetDropIndex(const SDragMsgData& inInfo)
{
	Uint32 nSavedIndex = mDropIndex;
	mDropIndex = PointToItem(inInfo.loc);
			
	if (mDropIndex)
	{
		SRect stBounds;
		SMyGroupInfo1 *pGroupInfo = GetItem(mDropIndex);
		
		if (pGroupInfo)
		{	
			GetItemRect(mDropIndex, stBounds);
			Uint32 nItemLeft = stBounds.left + 2;
			Uint32 nItemRight = nItemLeft + pGroupInfo->nNameSize;

			if (inInfo.loc.x < nItemLeft || inInfo.loc.x > nItemRight)
				mDropIndex = 0;
		}
	}	
	
	if (mDropIndex != nSavedIndex)
	{
		if (nSavedIndex)
			RefreshItem(nSavedIndex);
		
		if (mDropIndex)
			RefreshItem(mDropIndex);
	}
}

Uint32 CMyGroupListView1::GetDropGroupName(Uint8 *outGroupName, Uint32 inMaxSize)
{
	if (!mDropIndex || !outGroupName)
		return 0;
		
	SMyGroupInfo1 *pGroupInfo = GetItem(mDropIndex);
	if (!pGroupInfo)
		return 0;

	return UMemory::Copy(outGroupName, pGroupInfo->stGroupInfo.psGroupName + 1, pGroupInfo->stGroupInfo.psGroupName[0] > inMaxSize ? inMaxSize : pGroupInfo->stGroupInfo.psGroupName[0]);
}

void CMyGroupListView1::AddGroup(SMyGroupInfo1 *inGroupInfo, Uint8 *inFilterText, CLabelView *inGroupCount)
{
	mGroupList.AddItem(inGroupInfo);

	if (!inFilterText[0] || UText::SearchInsensitive(inFilterText + 1, inFilterText[0], inGroupInfo->stGroupInfo.psGroupName + 1, inGroupInfo->stGroupInfo.psGroupName[0]))
		AddItem(inGroupInfo, false);

	DisplayGroupCount(inGroupCount);
}

void CMyGroupListView1::FilterGroupList(Uint8 *inFilterText, CLabelView *inGroupCount)
{
	ClearList(false);

	Uint32 i = 0;
	SMyGroupInfo1 *pGroupInfo;
	
	while (mGroupList.GetNext(pGroupInfo, i))
	{
		if (!inFilterText[0] || UText::SearchInsensitive(inFilterText + 1, inFilterText[0], pGroupInfo->stGroupInfo.psGroupName + 1, pGroupInfo->stGroupInfo.psGroupName[0]))
			AddItem(pGroupInfo, false);
	}

	DisplayGroupCount(inGroupCount);
}

void CMyGroupListView1::DeleteAll(CLabelView *inGroupCount)
{
	Uint32 i = 0;
	SMyGroupInfo1 *pGroupInfo;
	
	while (mGroupList.GetNext(pGroupInfo, i))
		UMemory::Dispose((TPtr)pGroupInfo);

	mGroupList.Clear();
	ClearList(false);
	
	DisplayGroupCount(inGroupCount);
}

bool CMyGroupListView1::KeyDown(const SKeyMsgData& inInfo)
{
	if (inInfo.keyCode == key_Left || inInfo.keyCode == key_Right || inInfo.keyCode == key_Up || inInfo.keyCode == key_Down)
		mKeyDownTime = UDateTime::GetMilliseconds();
		
	return CMyListStatusView::KeyDown(inInfo);
}

void CMyGroupListView1::SetItemSelect(Uint32 inItem, bool inSelect)
{
	CMyListStatusView::SetItemSelect(inItem, inSelect);
	
	if (inSelect)
	{
		if (UDateTime::GetMilliseconds() - mKeyDownTime > 500)
			gApp->DoListArticles1();
		else
			StartSelectTimer();
	}
}

void CMyGroupListView1::DragEnter(const SDragMsgData& inInfo)
{
	CMyListStatusView::DragEnter(inInfo);
	
	if (!inInfo.drag->GetDragAction() && inInfo.drag->HasFlavor('SGP2'))
	{
		inInfo.drag->SetDragAction(dragAction_Copy);
			
		if (!IsMouseOnTab(inInfo.loc))
			SetDropIndex(inInfo);
	}
}

void CMyGroupListView1::DragMove(const SDragMsgData& inInfo)
{
	CMyListStatusView::DragMove(inInfo);
	
	if (!inInfo.drag->GetDragAction() && inInfo.drag->HasFlavor('SGP2'))
	{
		inInfo.drag->SetDragAction(dragAction_Copy);
			
		if (!IsMouseOnTab(inInfo.loc))
			SetDropIndex(inInfo);
	}
}

void CMyGroupListView1::DragLeave(const SDragMsgData& inInfo)
{
	CMyListStatusView::DragLeave(inInfo);

	if (mDropIndex)
	{
		Uint32 nSavedIndex = mDropIndex;
		mDropIndex = 0;
		
		RefreshItem(nSavedIndex);
	}
}

void CMyGroupListView1::DisplayGroupCount(CLabelView *inGroupCount)
{
	if (inGroupCount)
	{
		Uint8 psGroupCount[32];
		psGroupCount[0] = UText::Format(psGroupCount + 1, sizeof(psGroupCount) - 1, "%lu/%lu", mGeneralList.GetItemCount(), mGroupList.GetItemCount());
		
		inGroupCount->SetText(psGroupCount);
	}
}

void CMyGroupListView1::ExecuteSelectTimer()
{
	gApp->DoListArticles1();
}

void CMyGroupListView1::ItemDraw(Uint32 inListIndex, TImage inImage, const SRect& inBounds, const CPtrList<SRect>& inTabRectList, Uint32 inOptions)
{
	#pragma unused(inBounds, inOptions)
	
	SRect stRect;
	SColor stHiliteCol, stTextCol;

	// get group item
	SMyGroupInfo1 *pGroupInfo = GetItem(inListIndex);
	if (!pGroupInfo)
		return;

	// get info
	bool bIsSelected = IsFocus() && mIsEnabled && mSelectData.GetItem(inListIndex);

	// set font
	inImage->SetFont(kDefaultFont, nil, 9);
	inImage->SetFontEffect(fontEffect_Plain);

	if (!pGroupInfo->nNameSize)
		pGroupInfo->nNameSize = inImage->GetTextWidth(pGroupInfo->stGroupInfo.psGroupName + 1, pGroupInfo->stGroupInfo.psGroupName[0]);
	
	// draw item icon and name
	const SRect* pBounds = inTabRectList.GetItem(1);
	if (pBounds && pBounds->GetWidth())
	{
		if (bIsSelected)
			UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
		else
			UUserInterface::GetSysColor(sysColor_Label, stTextCol);
	
		if (inListIndex == mDropIndex)
		{
			// fill rect
			stRect = *pBounds;
			stRect.top += 1;
			stRect.left += 24;
			stRect.right = stRect.left + pGroupInfo->nNameSize;
			if (stRect.right > pBounds->right)
				stRect.right = pBounds->right;
		
			if (stRect.right > stRect.left)
			{
				UUserInterface::GetHighlightColor(&stHiliteCol);
			
				inImage->SetInkColor(stHiliteCol);
				inImage->FillRect(stRect);
		
				UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
			}
		}

		// set rect
		stRect = *pBounds;
		stRect.top += 2;
		stRect.bottom = stRect.top + 16;
		stRect.left += 2;
		stRect.right = stRect.left + 16;

		// draw icon
		mRootIcon->Draw(inImage, stRect, align_Center, bIsSelected ? transform_Dark : transform_None);
	
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 24;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		// draw item name
		inImage->SetInkColor(stTextCol);
		inImage->DrawTruncText(stRect, pGroupInfo->stGroupInfo.psGroupName + 1, pGroupInfo->stGroupInfo.psGroupName[0], nil, align_Left | align_CenterVert);
	}

	// draw item count
	pBounds = inTabRectList.GetItem(2);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		stRect.right -= 2;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		if (bIsSelected)
			UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
		else
			UUserInterface::GetSysColor(sysColor_Label, stTextCol);

		Uint8 psText[16];
		psText[0] = UText::Format(psText + 1, sizeof(psText) - 1, "(%lu)", pGroupInfo->stGroupInfo.nArticleCount);
	
		inImage->SetInkColor(stTextCol);
		inImage->DrawTruncText(stRect, psText + 1, psText[0], 0, align_Right | align_CenterVert);
	}
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyArticleTreeView1

CMyArticleTreeView1::CMyArticleTreeView1(CViewHandler *inHandler, const SRect &inBounds)
	: CMyGroupArticle(false), CMyTreeStatusView(inHandler, inBounds)
{
	// set headers
	AddTab("\pTitle", 80, 80);
	AddTab("\pPoster", 1, 1, align_CenterHoriz);
	AddTab("\pEmail");
	AddTab("\pDate", 1, 1, align_CenterHoriz);
	AddTab("\pTime", 1, 1, align_CenterHoriz);
	SetTabs(35,20,15,15,15);

	// set status
	SetStatus("\p0 items in list.");
}

CMyArticleTreeView1::~CMyArticleTreeView1()
{
	DeleteAll();
}

void CMyArticleTreeView1::SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2, Uint8 inTabPercent3, Uint8 inTabPercent4, Uint8 inTabPercent5)
{
	if (!inTabPercent1 && !inTabPercent2 && !inTabPercent3 && !inTabPercent4 && !inTabPercent5)
		return;
	
	CPtrList<Uint8> lPercentList;
	lPercentList.AddItem(&inTabPercent1);
	lPercentList.AddItem(&inTabPercent2);
	lPercentList.AddItem(&inTabPercent3);
	lPercentList.AddItem(&inTabPercent4);
	lPercentList.AddItem(&inTabPercent5);
	
	SetTabPercent(lPercentList);
	lPercentList.Clear();
}

void CMyArticleTreeView1::GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2, Uint8& outTabPercent3, Uint8& outTabPercent4, Uint8& outTabPercent5)
{
	outTabPercent1 = GetTabPercent(1);
	outTabPercent2 = GetTabPercent(2);
	outTabPercent3 = GetTabPercent(3);
	outTabPercent4 = GetTabPercent(4);
	outTabPercent5 = GetTabPercent(5);
}

void CMyArticleTreeView1::DeleteAll()
{
	Uint32 i = 0;
	SMyArticleInfo1 *pArticleInfo;
	
	while (GetNextTreeItem(pArticleInfo, i))
		UMemory::Dispose((TPtr)pArticleInfo);
	
	ClearTree();
}

// sort articles ascending or descending
void CMyArticleTreeView1::SortArticles(bool inDescending)
{
	Sort(0, CompareArticles, &inDescending);
	
	Uint32 nSelected = GetFirstSelectedTreeItem();
	if (nSelected)
		MakeTreeItemVisibleInList(nSelected);
}

// compare article data
Int32 CMyArticleTreeView1::CompareArticles(void *inItemA, void *inItemB, void *inRef)
{
	#pragma unused(inRef)
	
	SMyArticleInfo1 *pArticleInfoA = (SMyArticleInfo1 *)inItemA;
	SMyArticleInfo1 *pArticleInfoB = (SMyArticleInfo1 *)inItemB;
	bool bDescending = *(bool *)inRef;

	if (pArticleInfoA->stArticleInfo.stArticleDate == pArticleInfoB->stArticleInfo.stArticleDate)
		return 0;

	if (pArticleInfoA->stArticleInfo.stArticleDate < pArticleInfoB->stArticleInfo.stArticleDate)
		return bDescending ? 1 : -1;
	
	return bDescending ? -1 : 1;
}

void CMyArticleTreeView1::SearchText(const Uint8 *inText)
{
	Uint32 i = 0;
	SMyArticleInfo1 *pArticleInfo;

	while (GetNextVisibleTreeItem(pArticleInfo, i))
	{
		if (UText::SearchInsensitive(inText + 1, inText[0], pArticleInfo->stArticleInfo.psArticleName + 1, pArticleInfo->stArticleInfo.psArticleName[0]))
		{
			if (i != GetFirstSelectedTreeItem())
			{
				DeselectAllTreeItems();
				SelectTreeItem(i);
				MakeTreeItemVisibleInList(i);
			}

			return;
		}
	}
}

void CMyArticleTreeView1::SetDropIndex(const SDragMsgData& inInfo)
{
	Uint32 nSavedIndex = mDropIndex;
	mDropIndex = PointToTreeItem(inInfo.loc);
			
	if (mDropIndex)
	{
		SRect stBounds;
		SMyArticleInfo1 *pArticleInfo = GetTreeItem(mDropIndex);
		
		if (pArticleInfo && GetTreeItemRect(mDropIndex, stBounds))
		{	
			Uint32 nItemLeft = stBounds.left + 24;
			Uint32 nItemRight = nItemLeft + pArticleInfo->nTitleSize;

			if (inInfo.loc.x < nItemLeft || inInfo.loc.x > nItemRight)
				mDropIndex = 0;
		}
	}	
	
	if (mDropIndex != nSavedIndex)
	{
		if (nSavedIndex)
			RefreshTreeItem(nSavedIndex);
		
		if (mDropIndex)
			RefreshTreeItem(mDropIndex);
	}
}

bool CMyArticleTreeView1::GetDropArticleID(CPtrList<Uint8>& outParentIDsList)
{
	if (!mDropIndex)
		return true;
		
	SMyArticleInfo1 *pArticleInfo = GetTreeItem(mDropIndex);
	if (!pArticleInfo)
		return false;

	Uint8 *pParentID = (Uint8 *)UMemory::New(pArticleInfo->stArticleInfo.psArticleID[0] + 1);
	UMemory::Copy(pParentID, pArticleInfo->stArticleInfo.psArticleID, pArticleInfo->stArticleInfo.psArticleID[0] + 1);
	outParentIDsList.AddItem(pParentID);
	
	Uint32 nIndex = mDropIndex;
	while (true)
	{
		pArticleInfo = GetParentTreeItem(nIndex, &nIndex);
		if (!pArticleInfo)
			break;
	
		pParentID = (Uint8 *)UMemory::New(pArticleInfo->stArticleInfo.psArticleID[0] + 1);
		UMemory::Copy(pParentID, pArticleInfo->stArticleInfo.psArticleID, pArticleInfo->stArticleInfo.psArticleID[0] + 1);
		outParentIDsList.InsertItem(1, pParentID);	
	}
	
	return true;
}

bool CMyArticleTreeView1::KeyDown(const SKeyMsgData& inInfo)
{
	if (inInfo.keyCode == key_Left || inInfo.keyCode == key_Right || inInfo.keyCode == key_Up || inInfo.keyCode == key_Down)
		mKeyDownTime = UDateTime::GetMilliseconds();
		
	return CMyTreeStatusView::KeyDown(inInfo);
}

void CMyArticleTreeView1::SelectionChanged(Uint32 inTreeIndex, SMyArticleInfo1 *inTreeItem, bool inIsSelected)
{
	#pragma unused(inTreeIndex, inTreeItem)

	if (inIsSelected)
	{
		if (UDateTime::GetMilliseconds() - mKeyDownTime > 500)
			gApp->DoGetArticle1();
		else
			StartSelectTimer();
	}
}

void CMyArticleTreeView1::DragEnter(const SDragMsgData& inInfo)
{
	CMyTreeStatusView::DragEnter(inInfo);
	
	if (!inInfo.drag->GetDragAction() && inInfo.drag->HasFlavor('SAD2'))
	{
		inInfo.drag->SetDragAction(dragAction_Copy);
			
		if (!IsMouseOnTab(inInfo.loc))
			SetDropIndex(inInfo);
	}
}

void CMyArticleTreeView1::DragMove(const SDragMsgData& inInfo)
{
	CMyTreeStatusView::DragMove(inInfo);
	
	if (!inInfo.drag->GetDragAction() && inInfo.drag->HasFlavor('SAD2'))
	{
		inInfo.drag->SetDragAction(dragAction_Copy);
			
		if (!IsMouseOnTab(inInfo.loc))
			SetDropIndex(inInfo);
	}
}

void CMyArticleTreeView1::DragLeave(const SDragMsgData& inInfo)
{
	CMyTreeStatusView::DragLeave(inInfo);

	if (mDropIndex)
	{
		Uint32 nSavedIndex = mDropIndex;
		mDropIndex = 0;
		
		RefreshTreeItem(nSavedIndex);
	}
}

void CMyArticleTreeView1::ExecuteSelectTimer()
{
	gApp->DoGetArticle1();
}

void CMyArticleTreeView1::ItemDraw(Uint32 inTreeIndex, Uint32 inTreeLevel, SMyArticleInfo1 *inTreeItem, STreeViewItem *inTreeViewItem, TImage inImage, const CPtrList<SRect>& inTabRectList, Uint32 inOptions)
{
	#pragma unused(inTreeIndex, inTreeLevel, inOptions)
	
	Uint32 nSize;
	Uint8 bufText[32];

	SRect stRect;
	SColor stHiliteCol, stTextCol;

	// get info
	bool bIsSelected = IsFocus() && mIsEnabled && inTreeViewItem->bIsSelected;

	// set font
	inImage->SetFont(kDefaultFont, nil, 9);
	inImage->SetFontEffect(fontEffect_Plain);

	if (!inTreeItem->nTitleSize)
		inTreeItem->nTitleSize = inImage->GetTextWidth(inTreeItem->stArticleInfo.psArticleName + 1, inTreeItem->stArticleInfo.psArticleName[0]);
	
	if (bIsSelected)
		UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
	else
		UUserInterface::GetSysColor(sysColor_Label, stTextCol);
	
	// set color
	inImage->SetInkColor(stTextCol);
		
	// draw time
	const SRect* pBounds = inTabRectList.GetItem(5);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		stRect.right -= 2;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;
		
		nSize = UDateTime::DateToText(inTreeItem->stArticleInfo.stArticleDate, bufText, sizeof(bufText), kTimeText);
		inImage->DrawTruncText(stRect, bufText, nSize, nil, textAlign_Right);
	}
	
	// draw date
	pBounds = inTabRectList.GetItem(4);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		stRect.right -= 1;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		nSize = UDateTime::DateToText(inTreeItem->stArticleInfo.stArticleDate, bufText, sizeof(bufText), kShortDateText);
		inImage->DrawTruncText(stRect, bufText, nSize, nil, textAlign_Right);
	}
	
	// draw poster email
	pBounds = inTabRectList.GetItem(3);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;
	
		inImage->DrawTruncText(stRect, inTreeItem->stArticleInfo.psPosterAddr + 1, inTreeItem->stArticleInfo.psPosterAddr[0], nil, textAlign_Left);
	}
	
	// draw poster name
	pBounds = inTabRectList.GetItem(2);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		inImage->DrawTruncText(stRect, inTreeItem->stArticleInfo.psPosterName + 1, inTreeItem->stArticleInfo.psPosterName[0], nil, textAlign_Left);
	}
	
	// draw icon and title
	pBounds = inTabRectList.GetItem(1);
	if (pBounds && pBounds->GetWidth())
	{
		if (inTreeLevel == 1)
		{
			// set rect
			stRect = *pBounds;
			stRect.top += 2;
			stRect.bottom = stRect.top + 16;
			stRect.left += 2;
			stRect.right = stRect.left + 16;
	
			// draw icon
			if (stRect.right < pBounds->right)
				mRootIcon->Draw(inImage, stRect, align_Center, bIsSelected ? transform_Dark : transform_None);
		}	
	
		if (inTreeIndex == mDropIndex)
		{
			// fill rect
			stRect = *pBounds;
			stRect.top += 1;
			stRect.left += 24;
			stRect.right = stRect.left + inTreeItem->nTitleSize;
			if (stRect.right > pBounds->right)
				stRect.right = pBounds->right;
		
			if (stRect.right > stRect.left)
			{
				UUserInterface::GetHighlightColor(&stHiliteCol);
		
				inImage->SetInkColor(stHiliteCol);
				inImage->FillRect(stRect);

				UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
				inImage->SetInkColor(stTextCol);
			}
		}

		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 24;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		// draw title
		inImage->DrawTruncText(stRect, inTreeItem->stArticleInfo.psArticleName + 1, inTreeItem->stArticleInfo.psArticleName[0], nil, textAlign_Left);
	}
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyGroupListView2

CMyGroupListView2::CMyGroupListView2(CViewHandler *inHandler, const SRect &inBounds)
	: CMyListStatusView(inHandler, inBounds), CMyGroupArticle(true)
{
	// set headers
	AddTab("\pName", inBounds.GetWidth() - 40, 80);
	AddTab("\pSize", 1, 1, align_CenterHoriz);

	// set status
	SetStatus("\pClick the \"Select Hotline Server\" or \"Select Hotline Bundle\" button to select a bundle.");
}

CMyGroupListView2::~CMyGroupListView2()
{
	DeleteAll();
}

void CMyGroupListView2::SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2)
{
	if (!inTabPercent1 && !inTabPercent2)
		return;
	
	CPtrList<Uint8> lPercentList;
	lPercentList.AddItem(&inTabPercent1);
	lPercentList.AddItem(&inTabPercent2);
	
	SetTabPercent(lPercentList);
	lPercentList.Clear();
}

void CMyGroupListView2::GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2)
{
	outTabPercent1 = GetTabPercent(1);
	outTabPercent2 = GetTabPercent(2);
}

void CMyGroupListView2::SearchText(const Uint8 *inText)
{
	Uint32 i = 0;
	SMyGroupInfo2 *pGroupInfo;

	while (mGeneralList.GetNext(pGroupInfo, i))
	{
		if (UText::SearchInsensitive(inText + 1, inText[0], pGroupInfo->psGroupName + 1, pGroupInfo->psGroupName[0]))
		{
			if (i != GetFirstSelectedItem())
			{
				DeselectAll();
				SelectItem(i);
				MakeItemVisible(i);
			}

			return;
		}
	}
}

void CMyGroupListView2::SetDropIndex(const SDragMsgData& inInfo)
{
	Uint32 nSavedIndex = mDropIndex;
	mDropIndex = PointToItem(inInfo.loc);
			
	if (mDropIndex)
	{
		SRect stBounds;
		SMyGroupInfo2 *pGroupInfo = GetItem(mDropIndex);
		
		if (pGroupInfo)
		{	
			GetItemRect(mDropIndex, stBounds);
			Uint32 nItemLeft = stBounds.left + 2;
			Uint32 nItemRight = nItemLeft + pGroupInfo->nNameSize;

			if (inInfo.loc.x < nItemLeft || inInfo.loc.x > nItemRight)
				mDropIndex = 0;
		}
	}	
	
	if (mDropIndex != nSavedIndex)
	{
		if (nSavedIndex)
			RefreshItem(nSavedIndex);
		
		if (mDropIndex)
			RefreshItem(mDropIndex);
	}
}

Uint32 CMyGroupListView2::GetDropGroupName(Uint8 *outGroupName, Uint32 inMaxSize)
{
	if (!mDropIndex || !outGroupName)
		return 0;
		
	SMyGroupInfo2 *pGroupInfo = GetItem(mDropIndex);
	if (!pGroupInfo)
		return 0;

	return UMemory::Copy(outGroupName, pGroupInfo->psGroupName + 1, pGroupInfo->psGroupName[0] > inMaxSize ? inMaxSize : pGroupInfo->psGroupName[0]);
}

void CMyGroupListView2::AddGroup(SMyGroupInfo2 *inGroupInfo, Uint8 *inFilterText, CLabelView *inGroupCount)
{
	mGroupList.AddItem(inGroupInfo);

	if (!inFilterText[0] || UText::SearchInsensitive(inFilterText + 1, inFilterText[0], inGroupInfo->psGroupName + 1, inGroupInfo->psGroupName[0]))
		AddItem(inGroupInfo, false);

	DisplayGroupCount(inGroupCount);
}

void CMyGroupListView2::FilterGroupList(Uint8 *inFilterText, CLabelView *inGroupCount)
{
	ClearList(false);

	Uint32 i = 0;
	SMyGroupInfo2 *pGroupInfo;
	
	while (mGroupList.GetNext(pGroupInfo, i))
	{
		if (!inFilterText[0] || UText::SearchInsensitive(inFilterText + 1, inFilterText[0], pGroupInfo->psGroupName + 1, pGroupInfo->psGroupName[0]))
			AddItem(pGroupInfo, false);
	}

	DisplayGroupCount(inGroupCount);
}

void CMyGroupListView2::DeleteAll(CLabelView *inGroupCount)
{
	Uint32 i = 0;
	SMyGroupInfo2 *pGroupInfo;
	
	while (mGroupList.GetNext(pGroupInfo, i))
		UMemory::Dispose((TPtr)pGroupInfo);

	mGroupList.Clear();
	ClearList(false);
	
	DisplayGroupCount(inGroupCount);
}

bool CMyGroupListView2::KeyDown(const SKeyMsgData& inInfo)
{
	if (inInfo.keyCode == key_Left || inInfo.keyCode == key_Right || inInfo.keyCode == key_Up || inInfo.keyCode == key_Down)
		mKeyDownTime = UDateTime::GetMilliseconds();
		
	return CMyListStatusView::KeyDown(inInfo);
}

void CMyGroupListView2::SetItemSelect(Uint32 inItem, bool inSelect)
{
	CMyListStatusView::SetItemSelect(inItem, inSelect);
	
	if (inSelect)
	{
		if (UDateTime::GetMilliseconds() - mKeyDownTime > 500)
			gApp->DoListArticles2();
		else
			StartSelectTimer();
	}
}

void CMyGroupListView2::DragEnter(const SDragMsgData& inInfo)
{
	CMyListStatusView::DragEnter(inInfo);
	
	if (!inInfo.drag->GetDragAction() && inInfo.drag->HasFlavor('SGP1'))
	{
		inInfo.drag->SetDragAction(dragAction_Copy);
			
		if (!IsMouseOnTab(inInfo.loc))
			SetDropIndex(inInfo);
	}
}

void CMyGroupListView2::DragMove(const SDragMsgData& inInfo)
{
	CMyListStatusView::DragMove(inInfo);
	
	if (!inInfo.drag->GetDragAction() && inInfo.drag->HasFlavor('SGP1'))
	{
		inInfo.drag->SetDragAction(dragAction_Copy);
			
		if (!IsMouseOnTab(inInfo.loc))
			SetDropIndex(inInfo);
	}
}

void CMyGroupListView2::DragLeave(const SDragMsgData& inInfo)
{
	CMyListStatusView::DragLeave(inInfo);

	if (mDropIndex)
	{
		Uint32 nSavedIndex = mDropIndex;
		mDropIndex = 0;
		
		RefreshItem(nSavedIndex);
	}
}

void CMyGroupListView2::DisplayGroupCount(CLabelView *inGroupCount)
{
	if (inGroupCount)
	{
		Uint8 psGroupCount[32];
		psGroupCount[0] = UText::Format(psGroupCount + 1, sizeof(psGroupCount) - 1, "%lu/%lu", mGeneralList.GetItemCount(), mGroupList.GetItemCount());
		
		inGroupCount->SetText(psGroupCount);
	}
}

void CMyGroupListView2::ExecuteSelectTimer()
{
	gApp->DoListArticles2();
}

void CMyGroupListView2::ItemDraw(Uint32 inListIndex, TImage inImage, const SRect& inBounds, const CPtrList<SRect>& inTabRectList, Uint32 inOptions)
{
	#pragma unused(inBounds, inOptions)
	
	SRect stRect;
	SColor stHiliteCol, stTextCol;

	// get group item
	SMyGroupInfo2 *pGroupInfo = GetItem(inListIndex);
	if (!pGroupInfo)
		return;

	// get info
	bool bIsSelected = IsFocus() && mIsEnabled && mSelectData.GetItem(inListIndex);

	// set font
	inImage->SetFont(kDefaultFont, nil, 9);
	inImage->SetFontEffect(fontEffect_Plain);

	if (!pGroupInfo->nNameSize)
		pGroupInfo->nNameSize = inImage->GetTextWidth(pGroupInfo->psGroupName + 1, pGroupInfo->psGroupName[0]);
	
	// draw item icon and name
	const SRect* pBounds = inTabRectList.GetItem(1);
	if (pBounds && pBounds->GetWidth())
	{
		if (bIsSelected)
			UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
		else
			UUserInterface::GetSysColor(sysColor_Label, stTextCol);
	
		if (inListIndex == mDropIndex)
		{
			// fill rect
			stRect = *pBounds;
			stRect.top += 1;
			stRect.left += 24;
			stRect.right = stRect.left + pGroupInfo->nNameSize;
			if (stRect.right > pBounds->right)
				stRect.right = pBounds->right;
		
			if (stRect.right > stRect.left)
			{
				UUserInterface::GetHighlightColor(&stHiliteCol);
			
				inImage->SetInkColor(stHiliteCol);
				inImage->FillRect(stRect);
		
				UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
			}
		}

		// set rect
		stRect = *pBounds;
		stRect.top += 2;
		stRect.bottom = stRect.top + 16;
		stRect.left += 2;
		stRect.right = stRect.left + 16;

		// draw icon
		mRootIcon->Draw(inImage, stRect, align_Center, bIsSelected ? transform_Dark : transform_None);
	
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 24;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		// draw item name
		inImage->SetInkColor(stTextCol);
		inImage->DrawTruncText(stRect, pGroupInfo->psGroupName + 1, pGroupInfo->psGroupName[0], nil, align_Left | align_CenterVert);
	}

	// draw item count
	pBounds = inTabRectList.GetItem(2);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		stRect.right -= 2;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		if (bIsSelected)
			UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
		else
			UUserInterface::GetSysColor(sysColor_Label, stTextCol);

		Uint8 psText[16];
		psText[0] = UText::Format(psText + 1, sizeof(psText) - 1, "(%lu)", pGroupInfo->nArticleCount);
	
		inImage->SetInkColor(stTextCol);
		inImage->DrawTruncText(stRect, psText + 1, psText[0], 0, align_Right | align_CenterVert);
	}
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyArticleTreeView2

CMyArticleTreeView2::CMyArticleTreeView2(CViewHandler *inHandler, const SRect &inBounds)
	: CMyGroupArticle(false), CMyTreeStatusView(inHandler, inBounds)
{
	// set headers
	AddTab("\pTitle", 80, 80);
	AddTab("\pSize", 1, 1, align_CenterHoriz);
	AddTab("\pPoster");
	AddTab("\pDate", 1, 1, align_CenterHoriz);
	AddTab("\pTime", 1, 1, align_CenterHoriz);
	SetTabs(40,5,25,15,15);

	// set status
	SetStatus("\p0 items in list.");
}

CMyArticleTreeView2::~CMyArticleTreeView2()
{
	DeleteAll();
}

void CMyArticleTreeView2::SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2, Uint8 inTabPercent3, Uint8 inTabPercent4, Uint8 inTabPercent5)
{
	if (!inTabPercent1 && !inTabPercent2 && !inTabPercent3 && !inTabPercent4 && !inTabPercent5)
		return;
	
	CPtrList<Uint8> lPercentList;
	lPercentList.AddItem(&inTabPercent1);
	lPercentList.AddItem(&inTabPercent2);
	lPercentList.AddItem(&inTabPercent3);
	lPercentList.AddItem(&inTabPercent4);
	lPercentList.AddItem(&inTabPercent5);
	
	SetTabPercent(lPercentList);
	lPercentList.Clear();
}

void CMyArticleTreeView2::GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2, Uint8& outTabPercent3, Uint8& outTabPercent4, Uint8& outTabPercent5)
{
	outTabPercent1 = GetTabPercent(1);
	outTabPercent2 = GetTabPercent(2);
	outTabPercent3 = GetTabPercent(3);
	outTabPercent4 = GetTabPercent(4);
	outTabPercent5 = GetTabPercent(5);
}

void CMyArticleTreeView2::DeleteAll()
{
	Uint32 i = 0;
	SMyArticleInfo2 *pArticleInfo;
	
	while (GetNextTreeItem(pArticleInfo, i))
	{
		Uint32 j = 0;
		SMyDataInfo2 *pDataInfo;
		
		while (pArticleInfo->lDataList.GetNext(pDataInfo, j))
			UMemory::Dispose((TPtr)pDataInfo);
		
		pArticleInfo->lDataList.Clear();
		UMemory::Dispose((TPtr)pArticleInfo);
	}
	
	ClearTree();
}

Uint32 CMyArticleTreeView2::SetItemsFromData(const Uint8 *inData, Uint32 inDataSize)
{
	CUnflatten unflat(inData, inDataSize);
	if (unflat.NotEnufData(4))
		return 0;
	
	// article count
	Uint32 nCount = unflat.ReadLong();
	if (!nCount)
		return 0;
		
	SMyArticleInfo2 *pArticleInfo = nil;

	try
	{
		// now for the individual articles
		while (nCount--)
		{
			pArticleInfo = (SMyArticleInfo2 *)UMemory::NewClear(sizeof(SMyArticleInfo2));
			
			if (unflat.NotEnufData(4 + 8 + 4 + 4 + 2 + 3))
			{
				// corrupt
				Fail(errorType_Misc, error_Corrupt);
			}
			
			pArticleInfo->nArticleID = unflat.ReadLong();
						
			unflat.ReadDateTimeStamp(pArticleInfo->stDateTime);
			Uint32 nParentID = unflat.ReadLong();
			Uint32 nParentIndex = 0;
			
			if (nParentID)	// search parent begining from the end of the list
			{
				Uint32 i = GetTreeCount() + 1;
				SMyArticleInfo2 *pTmpArticleInfo;
				
				while (GetPrevTreeItem(pTmpArticleInfo, i))
				{
					if (pTmpArticleInfo->nArticleID == nParentID)
					{
						nParentIndex = i;
						
						if (GetTreeItemLevel(nParentIndex) == 1)
							SetDisclosure(nParentIndex, optTree_Collapsed);
						else
							SetDisclosure(nParentIndex, optTree_Disclosed);
							
						break;
					}
				}
				
			#if DEBUG
				if (!nParentIndex)
					DebugBreak("Corrupt data - it says I have a parent, but none of that ID exists!");
			#endif
			}
			
			pArticleInfo->nFlags = unflat.ReadLong();
			Uint16 nFlavCount = unflat.ReadWord();
			
			// title
			Uint8 pstrLen = unflat.ReadByte();
			
			if (unflat.NotEnufData(pstrLen + 1))
			{
				// corrupt!
				Fail(errorType_Misc, error_Corrupt);
			}
			
			pArticleInfo->psTitle[0] = UMemory::Copy(pArticleInfo->psTitle + 1, unflat.GetPtr(), min((Uint32)pstrLen, (Uint32)(sizeof(pArticleInfo->psTitle) - 1)));
			unflat.Skip(pstrLen);
			
			// poster
			pstrLen = unflat.ReadByte();
			if (unflat.NotEnufData(pstrLen))
			{
				// corrupt!
				Fail(errorType_Misc, error_Corrupt);
			}
			
			pArticleInfo->psPoster[0] = UMemory::Copy(pArticleInfo->psPoster + 1, unflat.GetPtr(), min((Uint32)pstrLen, (Uint32)(sizeof(pArticleInfo->psPoster) - 1)));
			unflat.Skip(pstrLen);			
			
			// article ID
			pstrLen = unflat.ReadByte();
			if (unflat.NotEnufData(pstrLen))
			{
				// corrupt!
				Fail(errorType_Misc, error_Corrupt);
			}
			
			unflat.Skip(pstrLen);	
			
			// flavors
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
								
				SMyDataInfo2 *pDataInfo = (SMyDataInfo2 *)UMemory::NewClear(sizeof(SMyDataInfo2));
				
				pDataInfo->psName[0] = UMemory::Copy(pDataInfo->psName + 1, psName + 1, psName[0] > sizeof(pDataInfo->psName) - 1 ? sizeof(pDataInfo->psName) - 1 : psName[0]);
				Uint32 nFlavSize = UMemory::Copy(pDataInfo->csFlavor, psFlav + 1, psFlav[0] > sizeof(pDataInfo->csFlavor) - 1 ? sizeof(pDataInfo->csFlavor) - 1 : psFlav[0]);
				*(pDataInfo->csFlavor + nFlavSize) = 0;
	
				pArticleInfo->lDataList.AddItem(pDataInfo);
				pArticleInfo->nDataSize += unflat.ReadWord();
			}
			
			AddTreeItem(nParentIndex, pArticleInfo, false);
			pArticleInfo = nil;
		}		
	}
	catch(...)
	{
		UMemory::Dispose((TPtr)pArticleInfo);
		return GetTreeCount();
	}
	
	return GetTreeCount();
}

// sort articles ascending or descending
void CMyArticleTreeView2::SortArticles(bool inDescending)
{
	Sort(0, CompareArticles, &inDescending);

	Uint32 nSelected = GetFirstSelectedTreeItem();
	if (nSelected)
		MakeTreeItemVisibleInList(nSelected);
}

// compare article data
Int32 CMyArticleTreeView2::CompareArticles(void *inItemA, void *inItemB, void *inRef)
{
	#pragma unused(inRef)
	
	SMyArticleInfo2 *pArticleInfoA = (SMyArticleInfo2 *)inItemA;
	SMyArticleInfo2 *pArticleInfoB = (SMyArticleInfo2 *)inItemB;
	bool bDescending = *(bool *)inRef;

	if (pArticleInfoA->stDateTime == pArticleInfoB->stDateTime)
		return 0;

	if (pArticleInfoA->stDateTime < pArticleInfoB->stDateTime)
		return bDescending ? 1 : -1;
	
	return bDescending ? -1 : 1;
}

void CMyArticleTreeView2::SearchText(const Uint8 *inText)
{
	Uint32 i = 0;
	SMyArticleInfo2 *pArticleInfo;

	while (GetNextVisibleTreeItem(pArticleInfo, i))
	{
		if (UText::SearchInsensitive(inText + 1, inText[0], pArticleInfo->psTitle + 1, pArticleInfo->psTitle[0]))
		{
			if (i != GetFirstSelectedTreeItem())
			{
				DeselectAllTreeItems();
				SelectTreeItem(i);
				MakeTreeItemVisibleInList(i);
			}
			
			return;
		}
	}
}

void CMyArticleTreeView2::SetDropIndex(const SDragMsgData& inInfo)
{
	Uint32 nSavedIndex = mDropIndex;
	mDropIndex = PointToTreeItem(inInfo.loc);
			
	if (mDropIndex)
	{
		SRect stBounds;
		SMyArticleInfo2 *pArticleInfo = GetTreeItem(mDropIndex);
		
		if (pArticleInfo && GetTreeItemRect(mDropIndex, stBounds))
		{	
			Uint32 nItemLeft = stBounds.left + 24;
			Uint32 nItemRight = nItemLeft + pArticleInfo->nTitleSize;

			if (inInfo.loc.x < nItemLeft || inInfo.loc.x > nItemRight)
				mDropIndex = 0;
		}
	}	
	
	if (mDropIndex != nSavedIndex)
	{
		if (nSavedIndex)
			RefreshTreeItem(nSavedIndex);
		
		if (mDropIndex)
			RefreshTreeItem(mDropIndex);
	}
}

Uint32 CMyArticleTreeView2::GetDropArticleID()
{
	if (!mDropIndex)
		return 0;
		
	SMyArticleInfo2 *pArticleInfo = GetTreeItem(mDropIndex);
	if (!pArticleInfo)
		return 0;

	return pArticleInfo->nArticleID;
}

SMyArticleInfo2 *CMyArticleTreeView2::GetSelectedArticle()
{
	Uint32 nSelected = GetFirstSelectedTreeItem();
	if (!nSelected)
		return nil;
		
	return GetTreeItem(nSelected);		
}

CPtrList<SMyDataInfo2> *CMyArticleTreeView2::GetDataList(Uint32 inArticleID)
{
	Uint32 i = 0;
	SMyArticleInfo2 *pArticleInfo;

	while (GetNextTreeItem(pArticleInfo, i))
	{
		if (pArticleInfo->nArticleID == inArticleID)
			return &pArticleInfo->lDataList;	
	}
	
	return nil;
}

bool CMyArticleTreeView2::KeyDown(const SKeyMsgData& inInfo)
{
	if (inInfo.keyCode == key_Left || inInfo.keyCode == key_Right || inInfo.keyCode == key_Up || inInfo.keyCode == key_Down)
		mKeyDownTime = UDateTime::GetMilliseconds();
		
	return CMyTreeStatusView::KeyDown(inInfo);
}

void CMyArticleTreeView2::SelectionChanged(Uint32 inTreeIndex, SMyArticleInfo2 *inTreeItem, bool inIsSelected)
{
	#pragma unused(inTreeIndex, inTreeItem)

	if (inIsSelected)
	{
		if (UDateTime::GetMilliseconds() - mKeyDownTime > 500)
			gApp->DoOpenArticle2();
		else
			StartSelectTimer();
	}
}

void CMyArticleTreeView2::DragEnter(const SDragMsgData& inInfo)
{
	CMyTreeStatusView::DragEnter(inInfo);
	
	if (!inInfo.drag->GetDragAction() && inInfo.drag->HasFlavor('SAD1'))
	{
		inInfo.drag->SetDragAction(dragAction_Copy);
			
		if (!IsMouseOnTab(inInfo.loc))
			SetDropIndex(inInfo);
	}
}

void CMyArticleTreeView2::DragMove(const SDragMsgData& inInfo)
{
	CMyTreeStatusView::DragMove(inInfo);
	
	if (!inInfo.drag->GetDragAction() && inInfo.drag->HasFlavor('SAD1'))
	{
		inInfo.drag->SetDragAction(dragAction_Copy);
			
		if (!IsMouseOnTab(inInfo.loc))
			SetDropIndex(inInfo);
	}
}

void CMyArticleTreeView2::DragLeave(const SDragMsgData& inInfo)
{
	CMyTreeStatusView::DragLeave(inInfo);

	if (mDropIndex)
	{
		Uint32 nSavedIndex = mDropIndex;
		mDropIndex = 0;
		
		RefreshTreeItem(nSavedIndex);
	}
}

void CMyArticleTreeView2::ExecuteSelectTimer()
{
	gApp->DoOpenArticle2();
}

void CMyArticleTreeView2::ItemDraw(Uint32 inTreeIndex, Uint32 inTreeLevel, SMyArticleInfo2 *inTreeItem, STreeViewItem *inTreeViewItem, TImage inImage, const CPtrList<SRect>& inTabRectList, Uint32 inOptions)
{
	#pragma unused(inTreeIndex, inTreeLevel, inOptions)
	
	Uint32 nSize;
	Uint8 bufText[32];

	SRect stRect;
	SColor stHiliteCol, stTextCol;

	// get info
	bool bIsSelected = IsFocus() && mIsEnabled && inTreeViewItem->bIsSelected;

	// set font
	inImage->SetFont(kDefaultFont, nil, 9);
	inImage->SetFontEffect(fontEffect_Plain);

	if (!inTreeItem->nTitleSize)
		inTreeItem->nTitleSize = inImage->GetTextWidth(inTreeItem->psTitle + 1, inTreeItem->psTitle[0]);
	
	if (bIsSelected)
		UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
	else
		UUserInterface::GetSysColor(sysColor_Label, stTextCol);
	
	// set color
	inImage->SetInkColor(stTextCol);
		
	// draw time
	const SRect* pBounds = inTabRectList.GetItem(5);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		stRect.right -= 2;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;
		
		nSize = UDateTime::DateToText(inTreeItem->stDateTime, bufText, sizeof(bufText), kTimeText);
		inImage->DrawTruncText(stRect, bufText, nSize, nil, textAlign_Right);
	}
	
	// draw date
	pBounds = inTabRectList.GetItem(4);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		stRect.right -= 1;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		nSize = UDateTime::DateToText(inTreeItem->stDateTime, bufText, sizeof(bufText), kShortDateText);
		inImage->DrawTruncText(stRect, bufText, nSize, nil, textAlign_Right);
	}
	
	// draw poster name
	pBounds = inTabRectList.GetItem(3);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;
	
		inImage->DrawTruncText(stRect, inTreeItem->psPoster + 1, inTreeItem->psPoster[0], nil, textAlign_Left);
	}
	
	// draw size
	pBounds = inTabRectList.GetItem(2);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		nSize = UText::Format(bufText, sizeof(bufText), "%luK", (inTreeItem->nDataSize + 1023) / 1024);
		inImage->DrawTruncText(stRect, bufText, nSize, nil, align_CenterHoriz | align_CenterVert);
	}
	
	// draw icon and title
	pBounds = inTabRectList.GetItem(1);
	if (pBounds && pBounds->GetWidth())
	{
		if (inTreeLevel == 1)
		{
			// set rect
			stRect = *pBounds;
			stRect.top += 2;
			stRect.bottom = stRect.top + 16;
			stRect.left += 2;
			stRect.right = stRect.left + 16;
	
			// draw icon
			if (stRect.right < pBounds->right)
				mRootIcon->Draw(inImage, stRect, align_Center, bIsSelected ? transform_Dark : transform_None);
		}	
	
		// if the item has no data, make it gray
		if (!inTreeItem->lDataList.GetItemCount())
			inImage->SetInkColor(color_Gray);

		if (inTreeIndex == mDropIndex)
		{
			// fill rect
			stRect = *pBounds;
			stRect.top += 1;
			stRect.left += 24;
			stRect.right = stRect.left + inTreeItem->nTitleSize;
			if (stRect.right > pBounds->right)
				stRect.right = pBounds->right;
		
			if (stRect.right > stRect.left)
			{
				UUserInterface::GetHighlightColor(&stHiliteCol);
		
				inImage->SetInkColor(stHiliteCol);
				inImage->FillRect(stRect);

				UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
				inImage->SetInkColor(stTextCol);
			}
		}

		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 24;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		// draw title
		inImage->DrawTruncText(stRect, inTreeItem->psTitle + 1, inTreeItem->psTitle[0], nil, textAlign_Left);
	}
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMySynchListView

CMySynchListView::CMySynchListView(CViewHandler *inHandler, const SRect &inBounds, CMyOptionsWin *inWin)
	: CGeneralCheckListView(inHandler, inBounds)
{
	mWin = inWin;
}

CMySynchListView::~CMySynchListView()
{
	ClearList(false);
}

SMySynchInfo *CMySynchListView::GetSelectedSynch()
{
	Uint32 nSelectedItem = GetFirstSelectedItem();
	if (!nSelectedItem)
		return nil;

	return GetItem(nSelectedItem);
}

SMySynchInfo *CMySynchListView::GetNearSelectedSynch()
{
	Uint32 nSelectedItem = GetFirstSelectedItem();
	if (!nSelectedItem)
		return nil;

	if (nSelectedItem > 1)
		nSelectedItem--;
	else
		nSelectedItem++;

	return GetItem(nSelectedItem);
}

void CMySynchListView::SetItemSelect(Uint32 inItem, bool inSelect)
{
	CGeneralCheckListView::SetItemSelect(inItem, inSelect);
	
	if (inSelect)
	{
		SMySynchInfo *pSynchInfo;
		if (GetSelectedItem(&pSynchInfo))
			mWin->SetSynchInfo(pSynchInfo);
	}
}

void CMySynchListView::MouseDown(const SMouseMsgData& inInfo)
{
	CGeneralCheckListView::MouseDown(inInfo);
	
	if ((inInfo.mods & kIsDoubleClick) && inInfo.loc.x > 242 && inInfo.loc.x < 267)
	{
		Uint32 nSelectedItem = GetFirstSelectedItem();
		if (!nSelectedItem)
			return;

		SMySynchInfo *pSynchInfo = GetItem(nSelectedItem);
		if (!pSynchInfo)
			return;
		
		if (pSynchInfo->nSynchType == myOpt_SynchNewsHotline)
			pSynchInfo->nSynchType = myOpt_SynchNewsServer;
		else if (pSynchInfo->nSynchType == myOpt_SynchNewsServer)
			pSynchInfo->nSynchType = myOpt_SynchNewsBoth;
		else
			pSynchInfo->nSynchType = myOpt_SynchNewsHotline;
			
		RefreshItem(nSelectedItem);
	}
}

void CMySynchListView::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	if (inItem > GetItemCount())
		return;
	
	CGeneralCheckListView::ItemDraw(inItem, inImage, inBounds, inUpdateRect, inOptions);
	
	SMySynchInfo *pSynchInfo = GetItem(inItem);
	if (!pSynchInfo)
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

	// group1
	rBounds.left += 29;
	rBounds.right = rBounds.left + 210; 

	inImage->DrawTruncText(rBounds, pSynchInfo->psGroupName + 1, pSynchInfo->psGroupName[0], nil, textAlign_Left);
	
	// type
	rBounds.left = rBounds.right + 3;
	rBounds.right = rBounds.left + 25;
	
	Uint8 *pSynchType = "\p";
	if (pSynchInfo->nSynchType == myOpt_SynchNewsHotline)
		pSynchType = "\p--->";
	else if (pSynchInfo->nSynchType == myOpt_SynchNewsServer)
		pSynchType = "\p<---";
	else if (pSynchInfo->nSynchType == myOpt_SynchNewsBoth)
		pSynchType = "\p<-->";
	
	inImage->DrawText(rBounds, pSynchType + 1, pSynchType[0], nil, textAlign_Center);

	// group2
	rBounds.left = rBounds.right + 3;
	rBounds.right = rBounds.left + 210;

	Uint8 psFileName[256];
	pSynchInfo->pHotlineFile->GetName(psFileName);
	inImage->DrawTruncText(rBounds, psFileName + 1, psFileName[0], nil, textAlign_Left);
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyLogListView

CMyLogListView::CMyLogListView(CViewHandler *inHandler, const SRect& inBounds)
	: CTabbedTreeView(inHandler, inBounds, 240, 241)
{
#if WIN32	
	mTabHeight = 18;
#endif

	nLogCount = 0;

	// set headers
	AddTab("\pLog", 80, 80);
	AddTab("\pDate", 1, 1, align_CenterHoriz);
	AddTab("\pTime", 1, 1, align_CenterHoriz);
	SetTabs(80,10,10);
}

CMyLogListView::~CMyLogListView()
{
	Uint32 i = 0;
	SMyLogInfo *pLogInfo;
	
	while (GetNextTreeItem(pLogInfo, i))
	{
		UMemory::Dispose((TPtr)pLogInfo->psLogText);
		UMemory::Dispose((TPtr)pLogInfo);
	}
	
	ClearTree();
}

void CMyLogListView::SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2, Uint8 inTabPercent3)
{
	if (!inTabPercent1 && !inTabPercent2)
		return;
	
	CPtrList<Uint8> lPercentList;
	lPercentList.AddItem(&inTabPercent1);
	lPercentList.AddItem(&inTabPercent2);
	lPercentList.AddItem(&inTabPercent3);
	
	SetTabPercent(lPercentList);
	lPercentList.Clear();
}

void CMyLogListView::GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2, Uint8& outTabPercent3)
{
	outTabPercent1 = GetTabPercent(1);
	outTabPercent2 = GetTabPercent(2);
	outTabPercent3 = GetTabPercent(3);
}

Uint32 CMyLogListView::MakeLog(const Uint8 *inLogText)
{
	if (!inLogText || !inLogText[0])
		return 0;
	
	if (GetRootCount() >= kMaxLogs)
		DeleteFirstLog();
	
	SMyLogInfo *pLogInfo = (SMyLogInfo *)UMemory::New(sizeof(SMyLogInfo));
	
	pLogInfo->nLogID = ++nLogCount;
	UDateTime::GetCalendarDate(calendar_Gregorian, pLogInfo->stLogDate);
	pLogInfo->psLogText = (Uint8 *)UMemory::New(inLogText, inLogText[0] + 1);

	MakeTreeItemVisibleInList(AddTreeItem(0, pLogInfo));
	
	return pLogInfo->nLogID;
}

void CMyLogListView::AddLog(Uint32 inLogID, const Uint8 *inLogText)
{
	if (!inLogID || !inLogText || !inLogText[0])
		return;

	Uint32 nParentIndex = 0;
	
	Uint32 i = 0;
	SMyLogInfo *pParentInfo;
	
	while (GetNextTreeItem(pParentInfo, i))
	{
		if (pParentInfo->nLogID == inLogID)
		{
			nParentIndex = i;
			break;
		}
	}

	if (!nParentIndex)
		return;
	
	if (GetDisclosure(nParentIndex) != optTree_Disclosed)
		SetDisclosure(nParentIndex, optTree_Disclosed);
	
	SMyLogInfo *pLogInfo = (SMyLogInfo *)UMemory::New(sizeof(SMyLogInfo));
	
	pLogInfo->nLogID = 0;
	UDateTime::GetCalendarDate(calendar_Gregorian, pLogInfo->stLogDate);
	pLogInfo->psLogText = (Uint8 *)UMemory::New(inLogText, inLogText[0] + 1);

	AddTreeItem(nParentIndex, pLogInfo, false);
}

void CMyLogListView::SelectLog(Uint32 inLogID)
{
	Uint32 i = 0;
	SMyLogInfo *pLogInfo;
	
	while (GetNextTreeItem(pLogInfo, i))
	{
		if (pLogInfo->nLogID == inLogID)
		{
			Uint32 nIndex = GetFirstSelectedTreeItem();
			if (GetTreeItemLevel(nIndex) != 1)
				nIndex = GetParentTreeIndex(nIndex);
			
			if (nIndex != i)
			{
				DeselectAllTreeItems();
				SelectTreeItem(i);
				MakeTreeItemVisibleInList(i);
			}
			
			return;
		}
	}
	
	DeselectAllTreeItems();
}

void CMyLogListView::DeleteFirstLog()
{
	SMyLogInfo *pLogInfo = GetTreeItem(1);
	if (!pLogInfo)
		return;
	
	UMemory::Dispose((TPtr)pLogInfo->psLogText);
	UMemory::Dispose((TPtr)pLogInfo);

	if (GetChildTreeCount(1))
	{
		Uint32 i = 2;
		pLogInfo = GetTreeItem(i);

		do
		{
			UMemory::Dispose((TPtr)pLogInfo->psLogText);
			UMemory::Dispose((TPtr)pLogInfo);
		
		} while (GetNextTreeItem(pLogInfo, i, true));
	}

	RemoveTreeItem(1);
}

void CMyLogListView::SelectionChanged(Uint32 inTreeIndex, SMyLogInfo *inTreeItem, bool inIsSelected)
{
	if (inIsSelected)
	{
		Uint32 nLogID = inTreeItem->nLogID;
		
		if (!nLogID)
		{
			SMyLogInfo *pLogInfo = GetParentTreeItem(inTreeIndex);
			if (pLogInfo)
				nLogID = pLogInfo->nLogID;
		}

		gApp->SelectTask(nLogID);
	}
}

void CMyLogListView::ItemDraw(Uint32 inTreeIndex, Uint32 inTreeLevel, SMyLogInfo *inTreeItem, STreeViewItem *inTreeViewItem, TImage inImage, const CPtrList<SRect>& inTabRectList, Uint32 inOptions)
{
	#pragma unused(inTreeIndex, inTreeLevel, inOptions)
	
	Uint32 nSize;
	Uint8 bufText[32];

	SRect stRect;
	SColor stTextCol;
	
	// get info
	bool bIsSelected = IsFocus() && mIsEnabled && inTreeViewItem->bIsSelected;
	
	// set font
	inImage->SetFont(kDefaultFont, nil, 9);
	inImage->SetFontEffect(fontEffect_Plain);

	if (bIsSelected)
		UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
	else
		UUserInterface::GetSysColor(sysColor_Label, stTextCol);

	// set color
	inImage->SetInkColor(stTextCol);

	// draw log
	const SRect* pBounds = inTabRectList.GetItem(1);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		stRect.right -= 1;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;
	
		inImage->DrawTruncText(stRect, inTreeItem->psLogText + 1, inTreeItem->psLogText[0], nil, textAlign_Left);
	}

	// draw date
	pBounds = inTabRectList.GetItem(2);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		stRect.right -= 1;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;

		nSize = UDateTime::DateToText(inTreeItem->stLogDate, bufText, sizeof(bufText), kShortDateText);
		inImage->DrawTruncText(stRect, bufText, nSize, nil, textAlign_Right);
	}

	// draw time
	pBounds = inTabRectList.GetItem(3);
	if (pBounds && pBounds->GetWidth())
	{
		// set rect
		stRect = *pBounds;
		stRect.top += 3;
		stRect.bottom -= 2;
		stRect.left += 1;
		stRect.right -= 2;
		if (stRect.right < stRect.left)
			stRect.right = stRect.left;
		
		nSize = UDateTime::DateToText(inTreeItem->stLogDate, bufText, sizeof(bufText), kTimeWithSecsText);
		inImage->DrawTruncText(stRect, bufText, nSize, nil, textAlign_Right);
	}
}


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyTaskListView

CMyTaskListView::CMyTaskListView(CViewHandler *inHandler, const SRect& inBounds)
	: CListView(inHandler, inBounds)
{
	mCellHeight = 39;
	mRefreshSelectedOnSetActive = true;
}

CMyTaskListView::~CMyTaskListView()
{
	Uint32 i = 0;
	SMyTaskInfo *pTaskInfo = nil;
	
	while (mTaskList.GetNext(pTaskInfo, i))
		UMemory::Dispose((TPtr)pTaskInfo);
}

Uint32 CMyTaskListView::GetItemCount() const
{
	return mTaskList.GetItemCount();
}

void CMyTaskListView::AddTask(CMyTask *inTask, const Uint8 *inDesc)
{
	if (!inDesc) return;
	
	SMyTaskInfo *pTaskInfo = nil;
	Uint8 *pTextData = nil;
	
	try
	{
		pTaskInfo = (SMyTaskInfo *)UMemory::NewClear(sizeof(SMyTaskInfo));
		pTaskInfo->pTask = inTask;
		
		if (inDesc && inDesc[0])
		{
			pTaskInfo->nTextSize = inDesc[0];
			pTaskInfo->pTextData = pTextData = (Uint8 *)UMemory::New(inDesc + 1, inDesc[0]);
		}
		
		mTaskList.AddItem(pTaskInfo);
		ItemsInserted(mTaskList.GetItemCount(), 1);
	}
	catch(...)
	{
		mTaskList.RemoveItem(pTaskInfo);
		UMemory::Dispose((TPtr)pTaskInfo);
		UMemory::Dispose((TPtr)pTextData);
		throw;
	}
}

void CMyTaskListView::RemoveTask(CMyTask *inTask)
{
	Uint32 nIndex;
	SMyTaskInfo *pTaskInfo = TaskToInfo(inTask, nIndex);
	
	if (pTaskInfo)
	{
		UMemory::Dispose((TPtr)pTaskInfo->pTextData);
		UMemory::Dispose((TPtr)pTaskInfo);
		
		mTaskList.RemoveItem(nIndex);
		ItemsRemoved(nIndex, 1);
	}
}

void CMyTaskListView::SetTaskProgress(CMyTask *inTask, Uint32 inVal, Uint32 inMax, const Uint8 *inDesc)
{
	Uint32 nIndex;
	SMyTaskInfo *pTaskInfo = TaskToInfo(inTask, nIndex);
	bool bRefresh = false;
	
	if (pTaskInfo)
	{
		if (pTaskInfo->nProgVal != inVal || pTaskInfo->nProgMax != inMax)
		{
			pTaskInfo->nProgVal = inVal;
			pTaskInfo->nProgMax = inMax;
			bRefresh = true;
		}
		
		if (inDesc)
		{
			UMemory::Dispose((TPtr)pTaskInfo->pTextData);
			pTaskInfo->pTextData = nil;
			pTaskInfo->nTextSize = 0;
			
			if (inDesc[0])
			{
				pTaskInfo->pTextData = (Uint8 *)UMemory::New(inDesc + 1, inDesc[0]);
				pTaskInfo->nTextSize = inDesc[0];
			}
			
			bRefresh = true;
		}
		
		if (bRefresh) 
			RefreshItem(nIndex);
	}
}

CMyTask *CMyTaskListView::GetSelectedTask()
{
	Uint32 nSelected = GetFirstSelectedItem();
	
	if (nSelected)
	{
		SMyTaskInfo *pTaskInfo = mTaskList.GetItem(nSelected);
		if (pTaskInfo) return pTaskInfo->pTask;
	}
	
	return nil;
}

void CMyTaskListView::ShowFinishedBar(CMyTask *inTask)
{
	Uint32 nIndex;
	SMyTaskInfo *pTaskInfo = TaskToInfo(inTask, nIndex);
	
	if (pTaskInfo)
	{
		if ((pTaskInfo->nProgVal < pTaskInfo->nProgMax) || (pTaskInfo->nProgMax == 0))
		{
			pTaskInfo->nProgVal = 100;
			pTaskInfo->nProgMax = 100;
			RefreshItem(nIndex);
		}
	}
}

void CMyTaskListView::SelectTask(Uint32 inLogID)
{
	Uint32 i = 0;
	SMyTaskInfo *pTaskInfo;
	
	while (mTaskList.GetNext(pTaskInfo, i))
	{
		CMyConnectTask *pConnectTask = dynamic_cast<CMyConnectTask *>(pTaskInfo->pTask);
		if (!pConnectTask)
			continue;
	
		if (pConnectTask->GetLogID() == inLogID)
		{
			if (GetFirstSelectedItem() != i)
			{
				DeselectAll();
				SelectItem(i);
				MakeItemVisible(i);
			}
			
			return;
		}
	}
	
	DeselectAll();
}

SMyTaskInfo *CMyTaskListView::TaskToInfo(CMyTask *inTask, Uint32& outIndex)
{
	Uint32 i = 0;
	SMyTaskInfo *pTaskInfo;
	
	while (mTaskList.GetNext(pTaskInfo, i))
	{
		if (pTaskInfo->pTask == inTask)
		{
			outIndex = i;
			return pTaskInfo;
		}
	}
	
	outIndex = 0;
	return nil;
}

void CMyTaskListView::SetItemSelect(Uint32 inItem, bool inSelect)
{
	CListView::SetItemSelect(inItem, inSelect);
	
	if (inSelect)
	{
		SMyTaskInfo *pTaskInfo = mTaskList.GetItem(inItem);
		if (!pTaskInfo)
			return;
		
		CMyConnectTask *pConnectTask = dynamic_cast<CMyConnectTask *>(pTaskInfo->pTask);
		if (!pConnectTask)
			return;
			
		pConnectTask->SelectLog();
	}
}

void CMyTaskListView::Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth)
{
	UGraphics::SetFont(inImage, kDefaultFont, nil, 9);

#if MACINTOSH
	inImage->SetInkMode(mode_Or);
#endif
	inherited::Draw(inImage, inUpdateRect, inDepth);
}

void CMyTaskListView::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& /* inUpdateRect */, Uint32 /* inOptions */)
{
	enum {
		borderSpace		= 4,
		innerSpace		= 4,
		textHeight		= 10,
		barHeight		= 13
	};
	
	SRect bounds, r;
	SColor hiliteCol;
	SMyTaskInfo *pTaskInfo;
	SProgressBarInfo stProgBarInfo;

	bool isSelected = mSelectData.GetItem(inItem);
	bool isActive = IsFocus() && mIsEnabled;
	
	// get info
	pTaskInfo = mTaskList.GetItem(inItem);
	
	// calc background rect
	bounds.top = inBounds.top + 1;
	bounds.bottom = inBounds.bottom - 2;
	bounds.left = inBounds.left + 1;
	bounds.right = inBounds.right - 1;
	
	// draw selection
	if (isSelected)
	{
		UUserInterface::GetHighlightColor(&hiliteCol);
		inImage->SetInkColor(hiliteCol);
		
		if (isActive)
			inImage->FillRect(bounds);
		else
			inImage->FrameRect(bounds);
	}

	// draw divider line between items
	r.top = bounds.bottom + 1;
	r.bottom = r.top + 1;
	r.left = inBounds.left;
	r.right = inBounds.right;
	inImage->SetInkColor(color_Black);
	inImage->FillRect(r);
	
	// draw light frame around this item
	r = inBounds;
	r.bottom--;
	UUserInterface::GetSysColor(sysColor_Light, hiliteCol);
	inImage->SetInkColor(hiliteCol);
	inImage->FrameRect(r);
	
	// draw name
	if (pTaskInfo->pTextData && pTaskInfo->nTextSize)
	{
		// determine color to draw text in
		UUserInterface::GetSysColor(isSelected && isActive ? sysColor_InverseHighlight : sysColor_Label, hiliteCol);

		r.top = bounds.top + borderSpace;
		r.bottom = r.top + textHeight;
		r.left = bounds.left + borderSpace + 1;
		r.right = bounds.right;
		inImage->SetInkColor(hiliteCol);
		UGraphics::DrawText(inImage, r, pTaskInfo->pTextData, pTaskInfo->nTextSize, 0, align_Left + align_CenterVert);
	}
	
	// draw progress bar
	r.top = bounds.top + borderSpace + textHeight + innerSpace;
	r.bottom = r.top + barHeight;
	r.left = bounds.left + borderSpace;
	r.right = bounds.right - borderSpace;
	stProgBarInfo.val = pTaskInfo->nProgVal;
	stProgBarInfo.max = pTaskInfo->nProgMax;
	stProgBarInfo.options = 0;

	UProgressBar::Draw(inImage, r, stProgBarInfo);
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyServerListView1

CMyServerListView1::CMyServerListView1(CViewHandler *inHandler, const SRect &inBounds)
	: CGeneralListView(inHandler, inBounds)
{
}

void CMyServerListView1::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	if (inItem > GetItemCount())
		return;
	
	CGeneralListView::ItemDraw(inItem, inImage, inBounds, inUpdateRect, inOptions);
	
	SMyServerInfo *pServerInfo = GetItem(inItem);
	if (!pServerInfo)
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
	rBounds.left += 4;
	rBounds.right -= 4;

	inImage->DrawText(rBounds, pServerInfo->psServerAddr + 1, pServerInfo->psServerAddr[0], nil, textAlign_Left);
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyServerListView2

CMyServerListView2::CMyServerListView2(CViewHandler *inHandler, const SRect &inBounds, CMySelectServerWin2 *inWin)
	: CGeneralListView(inHandler, inBounds)
{
	mWin = inWin;
}

void CMyServerListView2::SetItemSelect(Uint32 inItem, bool inSelect)
{
	CGeneralListView::SetItemSelect(inItem, inSelect);

	if (inSelect)
	{
		SMyServerBundleInfo *pServerBundleInfo;
		if (GetSelectedItem(&pServerBundleInfo))
			mWin->SetBundleList(pServerBundleInfo);
	}
}

void CMyServerListView2::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	if (inItem > GetItemCount())
		return;
	
	CGeneralListView::ItemDraw(inItem, inImage, inBounds, inUpdateRect, inOptions);
	
	SMyServerBundleInfo *pServerBundleInfo = GetItem(inItem);
	if (!pServerBundleInfo)
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
	rBounds.left += 4;
	rBounds.right -= 4;

	inImage->DrawText(rBounds, pServerBundleInfo->psServerName + 1, pServerBundleInfo->psServerName[0], nil, textAlign_Left);
}

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark CMyBundleListView

CMyBundleListView::CMyBundleListView(CViewHandler *inHandler, const SRect &inBounds)
	: CGeneralListView(inHandler, inBounds)
{
}

CMyBundleListView::~CMyBundleListView()
{
	ClearList(false);	// disable automatic dispose items
}

void CMyBundleListView::ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	if (inItem > GetItemCount())
		return;
	
	CGeneralListView::ItemDraw(inItem, inImage, inBounds, inUpdateRect, inOptions);
	
	TFSRefObj* pBundle = GetItem(inItem);
	if (!pBundle)
		return;
	
	Uint8 psBundleName[256];
	pBundle->GetName(psBundleName);
	
	SColor stTextCol;
	if (IsFocus() && mIsEnabled && mSelectData.GetItem(inItem))
		UUserInterface::GetSysColor(sysColor_InverseHighlight, stTextCol);
	else
		UUserInterface::GetSysColor(sysColor_Label, stTextCol);

	inImage->SetInkColor(stTextCol);
	
	SRect rBounds = inBounds;
	rBounds.top += 3;
	rBounds.bottom -= 2;
	rBounds.left += 4;
	rBounds.right -= 4;

	inImage->DrawText(rBounds, psBundleName + 1, psBundleName[0], nil, textAlign_Left);
}
