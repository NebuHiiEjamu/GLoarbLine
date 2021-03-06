/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CTabbedItemsView.h"

/* -------------------------------------------------------------------------- */


CTabbedItemsView::CTabbedItemsView(bool inShowTabLine)
{
	mTabHeight = 15;
	mMoveTabIndex = 0;
	mShowTabLine = inShowTabLine;

	mTabColor = color_GrayB;
	mTabTextColor = color_Black;
	mTabLineColor = mTabColor;
}

CTabbedItemsView::~CTabbedItemsView()
{
	// clear tab list
	Uint32 i = 0;
	STabItemInfo *pTabInfo;

	while (mTabInfoList.GetNext(pTabInfo, i))
		UMemory::Dispose((TPtr)pTabInfo);
	
	mTabInfoList.Clear();

	// clear rect list
	i = 0;
	SRect *pTabRect;

	while (mTabRectList.GetNext(pTabRect, i))
		UMemory::Dispose((TPtr)pTabRect);
	
	mTabRectList.Clear();
}

/* -------------------------------------------------------------------------- */
#pragma mark -

Uint32 CTabbedItemsView::AddTab(const Uint8 *inTabName, Uint16 inTabWidth, Uint16 inMinWidth, Uint8 inTabAlign)
{
	if (!inTabName || inTabWidth < 1 || inMinWidth < 1)
		return 0;

	if (inTabWidth < inMinWidth)
		inTabWidth = inMinWidth;
	
	// add new tab info
	STabItemInfo *pTabInfo = (STabItemInfo *)UMemory::NewClear(sizeof(STabItemInfo));

	pTabInfo->psTabName[0] = UMemory::Copy(pTabInfo->psTabName + 1, inTabName + 1, inTabName[0] > sizeof(pTabInfo->psTabName) - 1 ? sizeof(pTabInfo->psTabName) - 1 : inTabName[0]);
	pTabInfo->nTabWidth = inTabWidth;
	pTabInfo->nMinWidth = inMinWidth;
	pTabInfo->nTabAlign = inTabAlign;

	Uint32 nIndex = mTabInfoList.AddItem(pTabInfo);

	// add new tab rect
	SRect *pTabRect = (SRect *)UMemory::NewClear(sizeof(SRect));
	mTabRectList.AddItem(pTabRect);
	
	// set rect list
	SetTabRectList();

	return nIndex;
}

Uint32 CTabbedItemsView::InsertTab(Uint32 inIndex, const Uint8 *inTabName, Uint16 inTabWidth, Uint16 inMinWidth, Uint8 inTabAlign)
{
	if (!inTabName || inTabWidth < 1 || inMinWidth < 1)
		return 0;

	if (inTabWidth < inMinWidth)
		inTabWidth = inMinWidth;
	
	// insert new tab info
	STabItemInfo *pTabInfo = (STabItemInfo *)UMemory::NewClear(sizeof(STabItemInfo));

	pTabInfo->psTabName[0] = UMemory::Copy(pTabInfo->psTabName + 1, inTabName + 1, inTabName[0] > sizeof(pTabInfo->psTabName) - 1 ? sizeof(pTabInfo->psTabName) - 1 : inTabName[0]);
	pTabInfo->nTabWidth = inTabWidth;
	pTabInfo->nMinWidth = inMinWidth;
	pTabInfo->nTabAlign = inTabAlign;

	Uint32 nIndex = mTabInfoList.InsertItem(inIndex, pTabInfo);
	
	// insert new tab rect
	SRect *pTabRect = (SRect *)UMemory::NewClear(sizeof(SRect));
	mTabRectList.InsertItem(inIndex, pTabRect);

	// set rect list
	SetTabRectList();
	
	return nIndex;
}

bool CTabbedItemsView::DeleteTab(Uint32 inIndex)
{
	if (!inIndex)
		return false;

	// remove tab info
	STabItemInfo *pTabInfo = mTabInfoList.RemoveItem(inIndex);
	if (!pTabInfo)
		return false;
	
	UMemory::Dispose((TPtr)pTabInfo);

	// remove tab rect
	SRect *pTabRect = mTabRectList.RemoveItem(inIndex);
	if (!pTabRect)
		return false;

	UMemory::Dispose((TPtr)pTabRect);

	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CTabbedItemsView::SetTab(Uint32 inIndex, const Uint8 *inTabName, Uint16 inTabWidth, Uint16 inMinWidth, Uint8 inTabAlign)
{
	if (!inTabName || inTabWidth < 1 || inMinWidth < 1)
		return false;

	if (inTabWidth < inMinWidth)
		inTabWidth = inMinWidth;
	
	// set tab info
	STabItemInfo *pTabInfo = mTabInfoList.GetItem(inIndex);
	if (!pTabInfo)
		return false;
	
	pTabInfo->psTabName[0] = UMemory::Copy(pTabInfo->psTabName + 1, inTabName + 1, inTabName[0] > sizeof(pTabInfo->psTabName) - 1 ? sizeof(pTabInfo->psTabName) - 1 : inTabName[0]);
	pTabInfo->nTabWidth = inTabWidth;
	pTabInfo->nMinWidth = inMinWidth;
	pTabInfo->nTabAlign = inTabAlign;
	
	// set rect list
	SetTabRectList();

	return true;
}

bool CTabbedItemsView::SetTabName(Uint32 inIndex, const Uint8 *inTabName)
{
	if (!inTabName)
		return false;
	
	STabItemInfo *pTabInfo = mTabInfoList.GetItem(inIndex);
	if (!pTabInfo)
		return false;
	
	pTabInfo->psTabName[0] = UMemory::Copy(pTabInfo->psTabName + 1, inTabName + 1, inTabName[0] > sizeof(pTabInfo->psTabName) - 1 ? sizeof(pTabInfo->psTabName) - 1 : inTabName[0]);

	return true;
}

bool CTabbedItemsView::SetTabWidth(Uint32 inIndex, Uint16 inTabWidth)
{
	if (inTabWidth < 1)
		return false;

	STabItemInfo *pTabInfo = mTabInfoList.GetItem(inIndex);
	if (!pTabInfo)
		return false;

	pTabInfo->nTabWidth = inTabWidth;
	if (pTabInfo->nTabWidth < pTabInfo->nMinWidth)
		pTabInfo->nTabWidth = pTabInfo->nMinWidth;

	// set rect list
	SetTabRectList();

	return true;
}

bool CTabbedItemsView::SetTabPercent(const CPtrList<Uint8>& inPercentList)
{
	if (!inPercentList.GetItemCount())
		return false;
		
	Uint32 nTabCount = mTabInfoList.GetItemCount();
	if (!nTabCount)
		return false;
	
	SRect stBounds;
	GetTabViewBounds(stBounds);

	Uint32 nListWidth = stBounds.GetWidth();
	Uint32 nSpaceWidth = (nTabCount - 1) * 3;
	if (nListWidth > nSpaceWidth)
		nListWidth -= nSpaceWidth;

	Uint32 i = 0;
	Uint8 *pPercent;

	while (inPercentList.GetNext(pPercent, i))
	{
		STabItemInfo *pTabInfo = mTabInfoList.GetItem(i);
		if (!pTabInfo)
			break;
		
		Uint8 nPercent = *pPercent;
		if (nPercent > 100)
			nPercent = 100;
		
		if (nPercent == 100)
			pTabInfo->nTabWidth = stBounds.GetWidth();
		else
			pTabInfo->nTabWidth = (double)nListWidth * nPercent / 100 + 0.5;
		
		if (pTabInfo->nTabWidth < pTabInfo->nMinWidth)
			pTabInfo->nTabWidth = pTabInfo->nMinWidth;
	}
	
	// set rect list
	SetTabRectList();

	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

STabItemInfo *CTabbedItemsView::GetTab(Uint32 inIndex)
{
	return mTabInfoList.GetItem(inIndex);
}

Uint16 CTabbedItemsView::GetTabWidth(Uint32 inIndex)
{
	STabItemInfo *pTabInfo = mTabInfoList.GetItem(inIndex);
	if (!pTabInfo)
		return 0;

	return pTabInfo->nTabWidth;
}

Uint8 CTabbedItemsView::GetTabPercent(Uint32 inIndex)
{
	Uint32 nTabCount = mTabInfoList.GetItemCount();
	if (!nTabCount)
		return 0;

	STabItemInfo *pTabInfo = mTabInfoList.GetItem(inIndex);
	if (!pTabInfo)
		return 0;
	
	SRect stBounds;
	GetTabViewBounds(stBounds);

	Uint32 nListWidth = stBounds.GetWidth();
	if (!nListWidth)
		return 0;
	
	Uint32 nSpaceWidth = (nTabCount - 1) * 3;
	if (nListWidth > nSpaceWidth)
		nListWidth -= nSpaceWidth;
		
	Uint8 nPercent = (double)100 * pTabInfo->nTabWidth / nListWidth + 0.5;
	if (nPercent > 100)
		nPercent = 100;
	
	return nPercent;
}

Uint32 CTabbedItemsView::GetTabCount()
{
	return mTabInfoList.GetItemCount();
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CTabbedItemsView::TabMouseDown(const SMouseMsgData& inInfo)
{
	if (mMoveTabIndex || (!mShowTabLine && !IsMouseOnTab(inInfo.loc)))
		return;

	mMoveTabIndex = GetTabIndex(inInfo.loc);
	if (mMoveTabIndex)
	{	
		if (UMouse::GetImage() != mouseImage_SizeHeadWE)
			UMouse::SetImage(mouseImage_SizeHeadWE);
			
		RefreshTabView();
	}	
}

void CTabbedItemsView::TabMouseUp(const SMouseMsgData& inInfo)
{
	#pragma unused(inInfo)
	
	if (mMoveTabIndex)
	{
		mMoveTabIndex = 0;

		if ((!mShowTabLine && !IsMouseOnTab(inInfo.loc)) || !GetTabIndex(inInfo.loc))
		{
			if (UMouse::GetImage() != mouseImage_Standard)
				UMouse::SetImage(mouseImage_Standard);
		}
	
		RefreshTabView();
	}
}

void CTabbedItemsView::TabMouseMove(const SMouseMsgData& inInfo)
{
	if (!mMoveTabIndex)
	{
		if ((!mShowTabLine && !IsMouseOnTab(inInfo.loc)) || !GetTabIndex(inInfo.loc))
		{
			if (UMouse::GetImage() != mouseImage_Standard)
				UMouse::SetImage(mouseImage_Standard);
		}
		else if (UMouse::GetImage() != mouseImage_SizeHeadWE)
			UMouse::SetImage(mouseImage_SizeHeadWE);
		
		return;
	}
	
	STabItemInfo *pTabInfo = mTabInfoList.GetItem(mMoveTabIndex - 1);
	SRect *pTabRect = mTabRectList.GetItem(mMoveTabIndex - 1);
	if (!pTabInfo || !pTabRect)
	{
		mMoveTabIndex = 0;
		return;
	}

	SRect stBounds;
	GetTabViewBounds(stBounds);
	
	Int32 nHorizLoc = inInfo.loc.x;
#if MACINTOSH
	nHorizLoc -= 1;
#endif

	if (nHorizLoc < pTabRect->left)
		nHorizLoc = pTabRect->left;
	else if (nHorizLoc > stBounds.right)
		nHorizLoc = stBounds.right;
	
	Int32 nTabWidth = nHorizLoc - pTabRect->left - 1;
	if (nTabWidth < pTabInfo->nMinWidth)
		nTabWidth = pTabInfo->nMinWidth;
	
	if (pTabInfo->nTabWidth != nTabWidth)
	{
		pTabInfo->nTabWidth = nTabWidth;

		SetTabRectList();
		RefreshTabView();
	}
}

void CTabbedItemsView::TabMouseLeave(const SMouseMsgData& inInfo)
{
	#pragma unused(inInfo)

	if (UMouse::GetImage() != mouseImage_Standard)
		UMouse::SetImage(mouseImage_Standard);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CTabbedItemsView::DrawTabHeader(TImage inImage, const SRect& inBounds)
{
	if (!mTabHeight)
		return;
	
	Uint32 nHorizPos, nVertPos;
	GetTabViewScrollPos(nHorizPos, nVertPos);
	
	SRect stBounds;
	GetTabViewBounds(stBounds);	

	Uint32 nHeaderTop = stBounds.top + nVertPos;
	Uint32 nHeaderBottom = nHeaderTop + mTabHeight;
	if (nHeaderBottom < inBounds.top)
		return;
	
	// fill header rect
	inImage->SetInkColor(mTabColor);
	inImage->FillRect(SRect(stBounds.left, nHeaderTop, stBounds.right, nHeaderBottom));
	
	Uint32 i = 0;
	SRect *pTabRect;
	
	while (mTabRectList.GetNext(pTabRect, i))
	{
		// draw separators
		if (i > 1)
		{
			Int32 nSepHoriz = pTabRect->left - 3;
			if (nSepHoriz > stBounds.right)
				break;

			inImage->SetInkColor(color_Gray6);
			inImage->DrawLine(SLine(nSepHoriz, nHeaderTop, nSepHoriz, nHeaderBottom));

			nSepHoriz++;
			if (nSepHoriz > stBounds.right)
				break;

			inImage->SetInkColor(color_Black);
			inImage->DrawLine(SLine(nSepHoriz, nHeaderTop, nSepHoriz, nHeaderBottom));

			nSepHoriz++;
			if (nSepHoriz > stBounds.right)
				break;

			inImage->SetInkColor(color_White);
			inImage->DrawLine(SLine(nSepHoriz, nHeaderTop, nSepHoriz, nHeaderBottom));
		}
			
		// draw tab text
		if (nHeaderBottom - nHeaderTop > 4)
		{
			STabItemInfo *pTabInfo = mTabInfoList.GetItem(i);
			if (pTabInfo)
			{
				Uint32 nTabDepl = (pTabRect->right - pTabRect->left)/2;
				if (nTabDepl > 3)
					nTabDepl = 3;
				
				Int32 nTextLeft = pTabRect->left + nTabDepl;
				if (nTextLeft > stBounds.right)
					break;
	
				Int32 nTextRight = pTabRect->right - nTabDepl;
				if (nTextRight > stBounds.right)
					nTextRight = stBounds.right;
				
				// set font
				inImage->SetFont(kDefaultFont, nil, 9);
				inImage->SetFontEffect(fontEffect_Bold);

				// set color
				inImage->SetInkColor(mTabTextColor);
				
				inImage->DrawTruncText(SRect(nTextLeft, nHeaderTop, nTextRight, nHeaderBottom), pTabInfo->psTabName + 1, pTabInfo->psTabName[0], nil, pTabInfo->nTabAlign);
			}
		}
	}
	
	// 3d effect
	inImage->SetInkColor(color_Gray6);
	inImage->DrawLine(SLine(stBounds.left, nHeaderBottom - 1, stBounds.right, nHeaderBottom - 1));

	inImage->SetInkColor(color_Black);
	inImage->DrawLine(SLine(stBounds.left, nHeaderBottom, stBounds.right, nHeaderBottom));	
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CTabbedItemsView::SetTabRectList()
{
	Uint32 nTabCount = mTabInfoList.GetItemCount();
	if (!nTabCount)
		return;
	
	SRect stBounds;
	GetTabViewBounds(stBounds);
	Uint32 nTabLeft = stBounds.left;

	Uint32 i = 0;
	STabItemInfo *pTabInfo;

	while (mTabInfoList.GetNext(pTabInfo, i))
	{
		SRect *pTabRect = mTabRectList.GetItem(i);
		if (!pTabRect)
			break;
	
		pTabRect->left = nTabLeft;
				
		if (i == nTabCount)
			pTabRect->right = stBounds.right;
		else
			pTabRect->right = pTabRect->left + pTabInfo->nTabWidth;

		if (pTabRect->right > stBounds.right)
			pTabRect->right = stBounds.right;

		if (pTabRect->left > pTabRect->right)
			pTabRect->left = pTabRect->right;

		nTabLeft = pTabRect->right + 3;
	}
}

bool CTabbedItemsView::IsMouseOnTab(const SPoint& inLoc)
{
	if (!mTabHeight)
		return false;

	Uint32 nHorizPos, nVertPos;
	GetTabViewScrollPos(nHorizPos, nVertPos);

	SRect stBounds;
	GetTabViewBounds(stBounds);

	if (inLoc.y < stBounds.top + nVertPos || inLoc.y > stBounds.top + nVertPos + mTabHeight)
		return false;

	return true;
}

Uint32 CTabbedItemsView::GetTabIndex(const SPoint& inLoc)
{
	Int32 nHorizLoc = inLoc.x;
#if MACINTOSH
	nHorizLoc -= 1;
#endif

	Uint32 i = 1;
	SRect *pTabRect;

	while (mTabRectList.GetNext(pTabRect, i))
	{
		if (nHorizLoc >= pTabRect->left - 3 && nHorizLoc <= pTabRect->left - 1)
			return i;
	}
	
	return 0;	
}

