/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "CItemsView.h"
#include "CPtrTree.h"

enum {
	optTree_NoDisclosure	= 0,
	optTree_Collapsed		= 1,
	optTree_Disclosed 		= 2 
};

struct STreeViewItem {
	bool bIsVisible;
	bool bIsSelected;
	Uint8 nDisclosure;		// optTree_NoDisclosure, optTree_Collapsed, optTree_Disclosed
	void *pTreeItem;
};


// template tree view class
template <class T>
class CTreeView : public CSelectableItemsView
{	
	public:
		// construction
		CTreeView(CViewHandler *inHandler, const SRect& inBounds, Uint32 inCollapseID, Uint32 inDiscloseID);
		virtual ~CTreeView();
		
		// add/remove
		Uint32 AddTreeItem(Uint32 inParentTreeIndex, T *inTreeItem, bool inDisclosure = true);
		Uint32 InsertTreeItem(Uint32 inParentTreeIndex, Uint32 inIndex, T *inTreeItem, bool inDisclosure = true);
		T *RemoveTreeItem(Uint32 inTreeIndex, bool inRemoveChild = true);
		Uint32 RemoveTreeItem(T *inTreeItem, bool inRemoveChild = true);
		bool RemoveChildTree(Uint32 inParentIndex);
		bool RemoveChildTree(T *inParentItem);
		void ClearTree();

		// set/get
		bool SetTreeItem(Uint32 inTreeIndex, T *inTreeItem);
		T *GetTreeItem(Uint32 inTreeIndex) const;
		Uint32 GetTreeItemIndex(T *inTreeItem) const;
		Uint32 GetTreeItemLevel(Uint32 inTreeIndex) const;
		Uint32 GetTreeItemLevel(T *inTreeItem) const;

		bool SetParentTreeItem(Uint32 inChildTreeIndex, T *inTreeItem);
		T *GetParentTreeItem(Uint32 inChildTreeIndex, Uint32 *outParentTreeIndex = nil) const;
		T *GetParentTreeItem(T *inChildTreeItem, Uint32 *outParentTreeIndex = nil) const;
		Uint32 GetParentTreeIndex(Uint32 inChildTreeIndex) const;
		Uint32 GetParentTreeIndex(T *inChildTreeItem) const;

		Uint32 GetTreeCount() const;
		Uint32 GetRootCount() const;
		Uint32 GetChildTreeCount(Uint32 inTreeIndex) const;
		Uint32 GetChildTreeCount(T *inTreeItem) const;

		bool GetNextTreeItem(T*& ioTreeItem, Uint32& ioTreeIndex, bool inSameParent = false) const;
		bool GetPrevTreeItem(T*& ioTreeItem, Uint32& ioTreeIndex, bool inSameParent = false) const;
		bool GetNextVisibleTreeItem(T*& ioTreeItem, Uint32& ioTreeIndex) const;
		bool GetPrevVisibleTreeItem(T*& ioTreeItem, Uint32& ioTreeIndex) const;
		bool IsInTreeList(T *inTreeItem) const;
		
		// select/deselect
		bool SelectTreeItem(Uint32 inTreeIndex, bool inIsSelected = true);
		Uint32 GetFirstSelectedTreeItem() const;
		Uint32 GetFirstSelectedTreeItem(T **outTreeItem);
		Uint32 GetLastSelectedTreeItem() const;
		Uint32 GetLastSelectedTreeItem(T **outTreeItem);
		bool RefreshTreeItem(Uint32 inTreeIndex);
		bool MakeTreeItemVisible(Uint32 inTreeIndex);
		bool MakeTreeItemVisibleInList(Uint32 inTreeIndex, Uint16 inAlign = align_Inside);
		void DeselectAllTreeItems();
		
		// collapse/disclose
		bool ReverseState(Uint32 inTreeIndex);
		bool ReverseState(T *inTreeItem);
		bool Collapse(Uint32 inTreeIndex);
		bool Collapse(T *inTreeItem);
		bool Disclose(Uint32 inTreeIndex);
		bool Disclose(T *inTreeItem);
		bool SetDisclosure(Uint32 inTreeIndex, Uint8 inDisclosure);
		bool SetDisclosure(T *inTreeItem, Uint8 inDisclosure);
		void CollapseAllTreeItems();
		void DiscloseAllTreeItems();

		// get status
		bool IsVisible(Uint32 inTreeIndex);
		bool IsVisible(T *inTreeItem);
		bool IsSelected(Uint32 inTreeIndex);
		bool IsSelected(T *inTreeItem);
		Uint8 GetDisclosure(Uint32 inTreeIndex);
		Uint8 GetDisclosure(T *inTreeItem);
				
		// full size
		virtual void GetFullSize(Uint32& outWidth, Uint32& outHeight) const;
		virtual Uint32 GetFullHeight() const;
		virtual Uint32 GetFullWidth() const;
		
		// sorting
		void Sort(Uint32 inParentIndex, TComparePtrsProc inProc, void *inRef = nil);

		// misc
		virtual bool GetTreeItemRect(Uint32 inTreeIndex, SRect& outRect) const;
		virtual bool GetSelectedTreeItemsRect(SRect& outRect) const;
		virtual Uint32 PointToTreeItem(const SPoint& inPt) const;

		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);
		virtual void DragEnter(const SDragMsgData& inInfo);
		virtual void DragLeave(const SDragMsgData& inInfo);

		// mouse events
		virtual void MouseDown(const SMouseMsgData& inInfo);
		
		// key events
		virtual bool KeyDown(const SKeyMsgData& inInfo);

		// tree events
		virtual void VisibilityChanged(Uint32 inTreeIndex, T *inTreeItem, bool inIsVisible);
		virtual void SelectionChanged(Uint32 inTreeIndex, T *inTreeItem, bool inIsSelected);
		virtual void DisclosureChanged(Uint32 inTreeIndex, T *inTreeItem, Uint8 inDisclosure);
	
	protected:
		// items
		virtual bool IsValidItem(Uint32 inListIndex) const;
		virtual bool GetNextItem(Uint32& ioListIndex) const;
		virtual bool GetPrevItem(Uint32& ioListIndex) const;
		virtual Uint32 GetItemCount() const;

		// item selection
		virtual void SetItemSelect(Uint32 inListIndex, bool inSelect);
		virtual bool IsItemSelected(Uint32 inListIndex) const;
		virtual Uint32 GetFirstSelectedItem() const;
		virtual Uint32 GetLastSelectedItem() const;
		virtual void SetItemRangeSelect(Uint32 inFirstListIndex, Uint32 inLastListIndex, bool inSelect);

		// misc
		virtual void GetItemRect(Uint32 inListIndex, SRect& outRect) const;
		virtual void GetSelectedItemsRect(SRect& outRect) const;
		virtual Uint32 PointToItem(const SPoint& inPt) const;

		// item events
		virtual void ItemDraw(Uint32 inListIndex, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions = 0);
		virtual void ItemMouseDown(Uint32 inListIndex, const SMouseMsgData& inInfo);
		virtual void ItemMouseUp(Uint32 inListIndex, const SMouseMsgData& inInfo);
		virtual void ItemMouseEnter(Uint32 inListIndex, const SMouseMsgData& inInfo);
		virtual void ItemMouseMove(Uint32 inListIndex, const SMouseMsgData& inInfo);
		virtual void ItemMouseLeave(Uint32 inListIndex, const SMouseMsgData& inInfo);

	protected:
		CPtrTree<STreeViewItem> mTreeView;
		Uint32 mListCount, mMaxCount;
		Uint32 *mListIndex;
		
		Uint16 mCellHeight;
		Uint16 mLevelWidth;
		
		TIcon mCollapseIcon;
		TIcon mDiscloseIcon;
						
		TComparePtrsProc mCompareProc;
		void *mCompareRef;
		
		// item events
		virtual void ItemDraw(Uint32 inTreeIndex, Uint32 inTreeLevel, T *inTreeItem, STreeViewItem *inTreeViewItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions = 0);
		virtual void TreeItemMouseDown(Uint32 inTreeIndex, const SMouseMsgData& inInfo);
		virtual void TreeItemMouseUp(Uint32 inTreeIndex, const SMouseMsgData& inInfo);
		virtual void TreeItemMouseEnter(Uint32 inTreeIndex, const SMouseMsgData& inInfo);
		virtual void TreeItemMouseMove(Uint32 inTreeIndex, const SMouseMsgData& inInfo);
		virtual void TreeItemMouseLeave(Uint32 inTreeIndex, const SMouseMsgData& inInfo);

		STreeViewItem *GetListItem(Uint32 inListIndex, Uint32 *outTreeIndex = nil) const;
		Uint32 GetListIndex(STreeViewItem *inTreeViewItem) const;
		Uint32 ConvertListToTree(Uint32 inListIndex) const;
		Uint32 ConvertTreeToList(Uint32 inTreeIndex) const;
		
		void Collapse(Uint32 inTreeIndex, Uint32 inListIndex, STreeViewItem *inTreeItem);
		void Disclose(Uint32 inTreeIndex, Uint32& ioListIndex, STreeViewItem *inTreeItem);

		// item changes
		virtual void ItemsInserted(Uint32 inAtListIndex, Uint32 inCount);
		virtual void ItemsRemoved(Uint32 inAtListIndex, Uint32 inCount);
		
		STreeViewItem *GetTreeViewItem(T *inTreeItem, Uint32 *outTreeIndex = nil) const;
		static Int32 CompareItemsProc(void *inItemA, void *inItemB, void *inRef);
		void ComposeListIndex();
};

template<class T> CTreeView<T>::CTreeView(CViewHandler *inHandler, const SRect& inBounds, Uint32 inCollapseID, Uint32 inDiscloseID)
	: CSelectableItemsView(inHandler, inBounds)
{
	mListCount = 0;
	mMaxCount = 0;
	mListIndex = nil;
	
	mCellHeight = 19;
	mLevelWidth = 10;
	
	mCollapseIcon = UIcon::Load(inCollapseID);
	mDiscloseIcon = UIcon::Load(inDiscloseID);
	
	mRefreshSelectedOnSetActive = true;
}

template<class T> CTreeView<T>::~CTreeView()
{
	if (mListIndex)
		UMemory::Dispose((TPtr)mListIndex);

	Uint32 nTreeCount = mTreeView.GetTreeCount();	
	for (Uint32 i=1; i <= nTreeCount; i++)
	{					
		STreeViewItem *pTreeViewItem = mTreeView.GetItem(i);
		UMemory::Dispose((TPtr)pTreeViewItem);
	}

	UIcon::Release(mCollapseIcon);
	UIcon::Release(mDiscloseIcon);
}

#pragma mark -

template<class T> Uint32 CTreeView<T>::AddTreeItem(Uint32 inParentTreeIndex, T *inTreeItem, bool inDisclosure)
{
	bool bIsVisible = false;
	if (!inParentTreeIndex)
	{
		bIsVisible = true;
	}
	else
	{
		STreeViewItem *pParentItem = mTreeView.GetItem(inParentTreeIndex);
		if (!pParentItem)
			return 0;
			
		if (pParentItem->bIsVisible && pParentItem->nDisclosure == optTree_Disclosed)
			bIsVisible = true;
	}

	STreeViewItem *pTreeViewItem = (STreeViewItem *)UMemory::NewClear(sizeof(STreeViewItem));

	pTreeViewItem->bIsVisible = bIsVisible;
	if (inDisclosure) pTreeViewItem->nDisclosure = optTree_Collapsed;
	pTreeViewItem->pTreeItem = inTreeItem;
	
	Uint32 nTreeIndex = mTreeView.AddItem(inParentTreeIndex, pTreeViewItem);
	
	if (pTreeViewItem->bIsVisible)
		mListCount++;		
		
	ComposeListIndex();
	if (pTreeViewItem->bIsVisible)
		ItemsInserted(ConvertTreeToList(nTreeIndex), 1);
			
	return nTreeIndex;
}

template<class T> Uint32 CTreeView<T>::InsertTreeItem(Uint32 inParentTreeIndex, Uint32 inIndex, T *inTreeItem, bool inDisclosure)
{
	bool bIsVisible = false;
	if (!inParentTreeIndex)
	{
		bIsVisible = true;
	}
	else
	{
		STreeViewItem *pParentItem = mTreeView.GetItem(inParentTreeIndex);
		if (!pParentItem)
			return 0;
			
		if (pParentItem->bIsVisible && pParentItem->nDisclosure == optTree_Disclosed)
			bIsVisible = true;
	}

	STreeViewItem *pTreeViewItem = (STreeViewItem *)UMemory::NewClear(sizeof(STreeViewItem));
	
	pTreeViewItem->bIsVisible = bIsVisible;
	if (inDisclosure) pTreeViewItem->nDisclosure = optTree_Collapsed;
	pTreeViewItem->pTreeItem = inTreeItem;
	
	Uint32 nTreeIndex = mTreeView.InsertItem(inParentTreeIndex, inIndex, pTreeViewItem);

	if (pTreeViewItem->bIsVisible)
		mListCount++;	

	ComposeListIndex();
	if (pTreeViewItem->bIsVisible)
		ItemsInserted(ConvertTreeToList(nTreeIndex), 1);
		
	return nTreeIndex;
}

// inRemoveChild == true --> remove this item and all his children's 
// inRemoveChild == false --> replace this item with his first child
template<class T> T *CTreeView<T>::RemoveTreeItem(Uint32 inTreeIndex, bool inRemoveChild)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem)
		return nil;

    Uint32 nTempListCount = mListCount;
    Uint32 nListIndex = ConvertTreeToList(inTreeIndex);
    
	T *pTreeItem = (T *)pTreeViewItem->pTreeItem;
	bool bIsVisible = pTreeViewItem->bIsVisible;
	UMemory::Dispose((TPtr)pTreeViewItem);
	
	if (inRemoveChild || !mTreeView.GetChildTreeCount(inTreeIndex))
	{
		if (bIsVisible)
			mListCount--;

		Uint32 nTreeCount = mTreeView.GetTreeCount();
		Uint32 nItemLevel = mTreeView.GetItemLevel(inTreeIndex);
		Uint32 nIndex = inTreeIndex + 1;
	
		while (nIndex <= nTreeCount &&  mTreeView.GetItemLevel(nIndex) > nItemLevel)
		{
			pTreeViewItem = mTreeView.GetItem(nIndex);
			if (pTreeViewItem->bIsVisible)
				mListCount--;

			UMemory::Dispose((TPtr)pTreeViewItem);
			nIndex++;
		}
	}
	else if (bIsVisible)
	{
		pTreeViewItem = mTreeView.GetItem(inTreeIndex + 1);		// first child
		if (pTreeViewItem)
		{
			if (pTreeViewItem->bIsVisible)
				mListCount--;
			else
				pTreeViewItem->bIsVisible = true;
		}
	}
	
	mTreeView.RemoveItem(inTreeIndex, inRemoveChild);
	
	ComposeListIndex();
	if (nTempListCount - mListCount)
		ItemsRemoved(nListIndex, nTempListCount - mListCount);

	return pTreeItem;
}

// inRemoveChild == true --> remove this item and all his children's 
// inRemoveChild == false --> replace this item with his first child
template<class T> Uint32 CTreeView<T>::RemoveTreeItem(T *inTreeItem, bool inRemoveChild)
{
	Uint32 nTreeIndex = 0;
	if (GetTreeViewItem(inTreeItem, &nTreeIndex))
		RemoveTreeItem(nTreeIndex, inRemoveChild);
					
	return nTreeIndex;
}

template<class T> bool CTreeView<T>::RemoveChildTree(Uint32 inParentIndex)
{
	if (!mTreeView.GetChildTreeCount(inParentIndex))
	    return false;
	
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inParentIndex);
	if (!pTreeViewItem)
		return false;

    Uint32 nTempListCount = mListCount;
    Uint32 nListIndex = ConvertTreeToList(inParentIndex) + 1;

	Uint32 nTreeCount = mTreeView.GetTreeCount();
	Uint32 nItemLevel = mTreeView.GetItemLevel(inParentIndex);
	Uint32 nIndex = inParentIndex + 1;
	
	while (nIndex <= nTreeCount &&  mTreeView.GetItemLevel(nIndex) > nItemLevel)
	{
		pTreeViewItem = mTreeView.GetItem(nIndex);
		if (pTreeViewItem->bIsVisible)
			mListCount--;

		UMemory::Dispose((TPtr)pTreeViewItem);
		nIndex++;
	} 

	mTreeView.RemoveChildTree(inParentIndex);

	ComposeListIndex();
	if (nTempListCount - mListCount)
		ItemsRemoved(nListIndex, nTempListCount - mListCount);
	
	return true;
}

template<class T> bool CTreeView<T>::RemoveChildTree(T *inParentItem)
{
	Uint32 nParentIndex = 0;
	if (GetTreeViewItem(inParentItem, &nParentIndex))
		return RemoveChildTree(nParentIndex);
					
	return false;
}

template<class T> void CTreeView<T>::ClearTree()
{
	Uint32 nTreeCount = mTreeView.GetTreeCount();
	if (!nTreeCount)
		return;
	
	for (Uint32 i=1; i <= nTreeCount; i++)
	{					
		STreeViewItem *pTreeViewItem = mTreeView.GetItem(i);
		UMemory::Dispose((TPtr)pTreeViewItem);
	}
	
	ItemsRemoved(1, mListCount);

	if (mListIndex)
	{
		UMemory::Dispose((TPtr)mListIndex);
		mListIndex = nil;
	}
		
	mListCount = 0;
	mMaxCount = 0;

	mTreeView.Clear();	
}

#pragma mark -

template<class T> bool CTreeView<T>::SetTreeItem(Uint32 inTreeIndex, T *inTreeItem)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem)
		return false;
	
	pTreeViewItem->pTreeItem = inTreeItem;
	
	return true;
}

template<class T> T *CTreeView<T>::GetTreeItem(Uint32 inTreeIndex) const
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	
	if (pTreeViewItem)
		return (T *)pTreeViewItem->pTreeItem;
				
	return nil;	
}

template<class T> Uint32 CTreeView<T>::GetTreeItemIndex(T *inTreeItem) const
{
	Uint32 nTreeIndex;
	if (!GetTreeViewItem(inTreeItem, &nTreeIndex))
		return 0;

	return nTreeIndex;
}

template<class T> Uint32 CTreeView<T>::GetTreeItemLevel(Uint32 inTreeIndex) const
{
	return mTreeView.GetItemLevel(inTreeIndex);
}

template<class T> Uint32 CTreeView<T>::GetTreeItemLevel(T *inTreeItem) const
{
	Uint32 nTreeIndex;
	if (!GetTreeViewItem(inTreeItem, &nTreeIndex))
		return 0;

	return mTreeView.GetItemLevel(nTreeIndex);
}

template<class T> bool CTreeView<T>::SetParentTreeItem(Uint32 inChildTreeIndex, T *inTreeItem)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetParentItem(inChildTreeIndex);	
	if (!pTreeViewItem)
		return false;
		
	pTreeViewItem->pTreeItem = inTreeItem;
			
	return true;	
}

template<class T> T *CTreeView<T>::GetParentTreeItem(Uint32 inChildTreeIndex, Uint32 *outParentTreeIndex) const
{
	STreeViewItem *pTreeViewItem = mTreeView.GetParentItem(inChildTreeIndex, outParentTreeIndex);
	if (!pTreeViewItem)
		return nil;
		
	return (T *)pTreeViewItem->pTreeItem;
}

template<class T> T *CTreeView<T>::GetParentTreeItem(T *inChildTreeItem, Uint32 *outParentTreeIndex) const
{
	Uint32 nChildTreeIndex;
	if (!GetTreeViewItem(inChildTreeItem, &nChildTreeIndex))
		return nil;

	return GetParentTreeItem(nChildTreeIndex, outParentTreeIndex);
}

template<class T> Uint32 CTreeView<T>::GetParentTreeIndex(Uint32 inChildTreeIndex) const
{
	return mTreeView.GetParentIndex(inChildTreeIndex);
}

template<class T> Uint32 CTreeView<T>::GetParentTreeIndex(T *inChildTreeItem) const
{
	Uint32 inChildTreeIndex;
	if (!GetTreeViewItem(inChildTreeItem, &inChildTreeIndex))
		return nil;
		
	return mTreeView.GetParentIndex(inChildTreeIndex);
}

template<class T> Uint32 CTreeView<T>::GetTreeCount() const
{
	return mTreeView.GetTreeCount();
}

template<class T> Uint32 CTreeView<T>::GetRootCount() const
{
	return mTreeView.GetRootCount();
}

template<class T> Uint32 CTreeView<T>::GetChildTreeCount(Uint32 inTreeIndex) const
{
	return mTreeView.GetChildTreeCount(inTreeIndex);
}

template<class T> Uint32 CTreeView<T>::GetChildTreeCount(T *inTreeItem) const
{
	Uint32 nTreeIndex;
	if (!GetTreeViewItem(inTreeItem, &nTreeIndex))
		return 0;

	return mTreeView.GetChildTreeCount(nTreeIndex);
}

template<class T> bool CTreeView<T>::GetNextTreeItem(T*& ioTreeItem, Uint32& ioTreeIndex, bool inSameParent) const
{
	STreeViewItem *pTreeViewItem = nil;
	
	if (mTreeView.GetNext(pTreeViewItem, ioTreeIndex, inSameParent))
	{
		ioTreeItem = (T *)pTreeViewItem->pTreeItem;
		return true;
	}
	
	return false;
}

template<class T> bool CTreeView<T>::GetPrevTreeItem(T*& ioTreeItem, Uint32& ioTreeIndex, bool inSameParent) const
{
	STreeViewItem *pTreeViewItem = nil;
	
	if (mTreeView.GetPrev(pTreeViewItem, ioTreeIndex, inSameParent))
	{
		ioTreeItem = (T *)pTreeViewItem->pTreeItem;
		return true;
	}
	
	return false;
}

template<class T> bool CTreeView<T>::GetNextVisibleTreeItem(T*& ioTreeItem, Uint32& ioTreeIndex) const
{
	STreeViewItem *pTreeViewItem = nil;
	
	while (mTreeView.GetNext(pTreeViewItem, ioTreeIndex))
	{
		if (pTreeViewItem->bIsVisible)
		{
			ioTreeItem = (T *)pTreeViewItem->pTreeItem;
			return true;
		}
	}
	
	return false;
}

template<class T> bool CTreeView<T>::GetPrevVisibleTreeItem(T*& ioTreeItem, Uint32& ioTreeIndex) const
{
	STreeViewItem *pTreeViewItem = nil;
	
	while (mTreeView.GetPrev(pTreeViewItem, ioTreeIndex))
	{
		if (pTreeViewItem->bIsVisible)
		{
			ioTreeItem = (T *)pTreeViewItem->pTreeItem;
			return true;
		}
	}
	
	return false;
}

template<class T> bool CTreeView<T>::IsInTreeList(T *inTreeItem) const
{
	if (GetTreeViewItem(inTreeItem))
		return true;
			
	return false;
}

#pragma mark -

template<class T> bool CTreeView<T>::SelectTreeItem(Uint32 inTreeIndex, bool inIsSelected)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem || !pTreeViewItem->bIsVisible)
		return false;
	
	if (pTreeViewItem->bIsSelected == inIsSelected)
		return true;
		
	pTreeViewItem->bIsSelected = inIsSelected;
	RefreshItem(ConvertTreeToList(inTreeIndex));
	
	SelectionChanged(inTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->bIsSelected);
	return true;
}

template<class T> Uint32 CTreeView<T>::GetFirstSelectedTreeItem() const
{
	Uint32 i = 0;
	STreeViewItem *pTreeViewItem;
	
	while (mTreeView.GetNext(pTreeViewItem, i))
	{
		if (pTreeViewItem->bIsVisible && pTreeViewItem->bIsSelected)
			return i;
	}
	
	return 0;
}

template<class T> Uint32 CTreeView<T>::GetFirstSelectedTreeItem(T **outTreeItem)
{
	Uint32 nSelected = GetFirstSelectedTreeItem();
	if (!nSelected)
	{
		*outTreeItem = nil;
		return 0;
	}

	*outTreeItem = GetTreeItem(nSelected);
	if (!*outTreeItem)
		return 0;
		
	return nSelected;
}

template<class T> Uint32 CTreeView<T>::GetLastSelectedTreeItem() const
{
	Uint32 i = mTreeView.GetTreeCount() + 1;
	STreeViewItem *pTreeViewItem;
	
	while (mTreeView.GetPrev(pTreeViewItem, i))
	{
		if (pTreeViewItem->bIsVisible && pTreeViewItem->bIsSelected)
			return i;
	}
	
	return 0;
}

template<class T> Uint32 CTreeView<T>::GetLastSelectedTreeItem(T **outTreeItem)
{
	Uint32 nSelected = GetFirstSelectedLastItem();
	if (!nSelected)
	{
		*outTreeItem = nil;
		return 0;
	}

	*outTreeItem = GetTreeItem(nSelected);
	if (!*outTreeItem)
		return 0;
		
	return nSelected;
}

template<class T> bool CTreeView<T>::RefreshTreeItem(Uint32 inTreeIndex)
{
	Uint32 nListIndex = ConvertTreeToList(inTreeIndex);
	if (!nListIndex)
		return false;
	
	RefreshItem(nListIndex);

	return true;	
}

template<class T> bool CTreeView<T>::MakeTreeItemVisible(Uint32 inTreeIndex)
{	
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem)
		return false;
	
	Uint32 nTreeIndex = inTreeIndex;
		
	while (!pTreeViewItem->bIsVisible)
	{
		nTreeIndex = GetParentTreeIndex(nTreeIndex);
		if (!nTreeIndex)
			return false;
		
		SetDisclosure(nTreeIndex, optTree_Disclosed);
	}

	return true;	
}

template<class T> bool CTreeView<T>::MakeTreeItemVisibleInList(Uint32 inTreeIndex, Uint16 inAlign)
{
	Uint32 nListIndex = ConvertTreeToList(inTreeIndex);
	if (!nListIndex)
		return false;
	
	MakeItemVisible(nListIndex, inAlign);

	return true;	
}

template<class T> void CTreeView<T>::DeselectAllTreeItems()
{
	Uint32 i = 0;
	STreeViewItem *pTreeViewItem;
	
	while (mTreeView.GetNext(pTreeViewItem, i))
	{
		if (pTreeViewItem->bIsVisible && pTreeViewItem->bIsSelected)
		{
			pTreeViewItem->bIsSelected = false;
			RefreshItem(ConvertTreeToList(i));
	
			SelectionChanged(i, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->bIsSelected);
		}
	}
}

#pragma mark -

template<class T> bool CTreeView<T>::ReverseState(Uint32 inTreeIndex)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem || !pTreeViewItem->bIsVisible || pTreeViewItem->nDisclosure == optTree_NoDisclosure)
		return false;
	
	Uint32 nListIndex = ConvertTreeToList(inTreeIndex);
	Uint32 nNextListIndex = nListIndex + 1;
	
	if (pTreeViewItem->nDisclosure == optTree_Collapsed)
	{
		pTreeViewItem->nDisclosure = optTree_Disclosed;
		Disclose(inTreeIndex, nNextListIndex, pTreeViewItem);
	}
	else if (pTreeViewItem->nDisclosure == optTree_Disclosed)
	{
		pTreeViewItem->nDisclosure = optTree_Collapsed;
		Collapse(inTreeIndex, nNextListIndex, pTreeViewItem);	
	}
	
	DisclosureChanged(inTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->nDisclosure);
	RefreshItem(nListIndex);
	
	return true;
}

template<class T> bool CTreeView<T>::ReverseState(T *inTreeItem)
{
	Uint32 nTreeIndex;
	if (!GetTreeViewItem(inTreeItem, &nTreeIndex))
		return false;

	return ReverseState(nTreeIndex);
}

template<class T> bool CTreeView<T>::Collapse(Uint32 inTreeIndex)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem || !pTreeViewItem->bIsVisible || pTreeViewItem->nDisclosure != optTree_Disclosed)
		return false;
			
	Uint32 nListIndex = ConvertTreeToList(inTreeIndex);
	Uint32 nNextListIndex = nListIndex + 1;

	pTreeViewItem->nDisclosure = optTree_Collapsed;
	Collapse(inTreeIndex, nNextListIndex, pTreeViewItem);	
	
	DisclosureChanged(inTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->nDisclosure);
	RefreshItem(nListIndex);

	return true;
}

template<class T> bool CTreeView<T>::Collapse(T *inTreeItem)
{
	Uint32 nTreeIndex;
	if (!GetTreeViewItem(inTreeItem, &nTreeIndex))
		return false;

	return Collapse(nTreeIndex);
}

template<class T> bool CTreeView<T>::Disclose(Uint32 inTreeIndex)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem || !pTreeViewItem->bIsVisible || pTreeViewItem->nDisclosure != optTree_Collapsed)
		return false;
	
	Uint32 nListIndex = ConvertTreeToList(inTreeIndex);
	Uint32 nNextListIndex = nListIndex + 1;

	pTreeViewItem->nDisclosure = optTree_Disclosed;
	Disclose(inTreeIndex, nNextListIndex, pTreeViewItem);
	
	DisclosureChanged(inTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->nDisclosure);
	RefreshItem(nListIndex);

	return true;
}

template<class T> bool CTreeView<T>::Disclose(T *inTreeItem)
{
	Uint32 nTreeIndex;
	if (!GetTreeViewItem(inTreeItem, &nTreeIndex))
		return false;

	return Disclose(nTreeIndex);
}

template<class T> bool CTreeView<T>::SetDisclosure(Uint32 inTreeIndex, Uint8 inDisclosure)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem)
		return false;
		
	if (pTreeViewItem->nDisclosure == inDisclosure)
		return true;
		
	Uint32 nDisclosure = pTreeViewItem->nDisclosure;
	pTreeViewItem->nDisclosure = inDisclosure;

	if (pTreeViewItem->bIsVisible)
	{
		Uint32 nListIndex = ConvertTreeToList(inTreeIndex);
		Uint32 nNextListIndex = nListIndex + 1;

		if ((inDisclosure == optTree_NoDisclosure || inDisclosure == optTree_Collapsed) && nDisclosure == optTree_Disclosed)
			Collapse(inTreeIndex, nNextListIndex, pTreeViewItem);
		else if (inDisclosure == optTree_Disclosed)
			Disclose(inTreeIndex, nNextListIndex, pTreeViewItem);
		
		DisclosureChanged(inTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->nDisclosure);
		RefreshItem(nListIndex);
	}
	
	return true;	
}

template<class T> bool CTreeView<T>::SetDisclosure(T *inTreeItem, Uint8 inDisclosure)
{
	Uint32 nTreeIndex;
	if (!GetTreeViewItem(inTreeItem, &nTreeIndex))
		return false;

	return SetDisclosre(nTreeIndex, inDisclosure);
}

template<class T> void CTreeView<T>::CollapseAllTreeItems()
{
	Uint32 i = mTreeView.GetTreeCount() + 1;
	STreeViewItem *pTreeViewItem;
	
	while (mTreeView.GetPrev(pTreeViewItem, i))
	{
		if (pTreeViewItem->bIsVisible && pTreeViewItem->nDisclosure == optTree_Disclosed)
		{
			Uint32 nListIndex = ConvertTreeToList(i);
			Uint32 nNextListIndex = nListIndex + 1;

			pTreeViewItem->nDisclosure = optTree_Collapsed;
			Collapse(i, nNextListIndex, pTreeViewItem);	
	
			DisclosureChanged(i, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->nDisclosure);
			RefreshItem(nListIndex);
		}
	}
}

template<class T> void CTreeView<T>::DiscloseAllTreeItems()
{
	Uint32 i = 0;
	STreeViewItem *pTreeViewItem;
	
	while (mTreeView.GetNext(pTreeViewItem, i))
	{
		if (pTreeViewItem->bIsVisible && pTreeViewItem->nDisclosure == optTree_Collapsed)
		{
			Uint32 nListIndex = ConvertTreeToList(i);
			Uint32 nNextListIndex = nListIndex + 1;

			pTreeViewItem->nDisclosure = optTree_Disclosed;
			Disclose(i, nNextListIndex, pTreeViewItem);
	
			DisclosureChanged(i, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->nDisclosure);
			RefreshItem(nListIndex);
		}
	}
}

#pragma mark -

template<class T> bool CTreeView<T>::IsVisible(Uint32 inTreeIndex)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem)
		return false;

	return pTreeViewItem->bIsVisible;
}

template<class T> bool CTreeView<T>::IsVisible(T *inTreeItem)
{
	STreeViewItem *pTreeViewItem = GetTreeViewItem(inTreeItem);
	if (!pTreeViewItem)
		return false;

	return pTreeViewItem->bIsVisible;
}

template<class T> bool CTreeView<T>::IsSelected(Uint32 inTreeIndex)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem)
		return false;

	return pTreeViewItem->bIsSelected;
}

template<class T> bool CTreeView<T>::IsSelected(T *inTreeItem)
{
	STreeViewItem *pTreeViewItem = GetTreeViewItem(inTreeItem);
	if (!pTreeViewItem)
		return false;

	return pTreeViewItem->bIsSelected;
}

template<class T> Uint8 CTreeView<T>::GetDisclosure(Uint32 inTreeIndex)
{
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem)
		return optTree_NoDisclosure;

	return pTreeViewItem->nDisclosure;
}

template<class T> Uint8 CTreeView<T>::GetDisclosure(T *inTreeItem)
{
	STreeViewItem *pTreeViewItem = GetTreeViewItem(inTreeItem);
	if (!pTreeViewItem)
		return optTree_NoDisclosure;

	return pTreeViewItem->nDisclosure;
}

template<class T> void CTreeView<T>::GetFullSize(Uint32& outWidth, Uint32& outHeight) const
{
	outHeight = GetFullHeight();
	outWidth = GetFullWidth();
}

template<class T> Uint32 CTreeView<T>::GetFullHeight() const
{
	return GetItemCount() ? GetTabHeight() + GetItemCount() * mCellHeight : mBounds.GetHeight();
}

template<class T> Uint32 CTreeView<T>::GetFullWidth() const
{
	return mBounds.GetWidth();
}

template<class T> void CTreeView<T>::Sort(Uint32 inParentIndex, TComparePtrsProc inProc, void *inRef)
{
	mCompareProc = inProc;
	mCompareRef = inRef;
	
	mTreeView.Sort(inParentIndex, CompareItemsProc, this);
		
	mCompareProc = nil;
	mCompareRef = nil;
	
	ComposeListIndex();
	Refresh(mBounds);
}

template<class T> bool CTreeView<T>::GetTreeItemRect(Uint32 inTreeIndex, SRect& outRect) const
{
	outRect.SetEmpty();
	
	STreeViewItem *pTreeViewItem = mTreeView.GetItem(inTreeIndex);
	if (!pTreeViewItem || !pTreeViewItem->bIsVisible)
		return false;

	Uint32 nTreeViewLevel = mTreeView.GetItemLevel(inTreeIndex);

	Uint32 nListIndex = ConvertTreeToList(inTreeIndex);
	GetItemRect(nListIndex, outRect);
	
	outRect.left += 24;
	if (nTreeViewLevel) 
		outRect.left += mLevelWidth * (nTreeViewLevel - 1);

	if (outRect.right < outRect.left)
		outRect.right = outRect.left;
	
	return true;
}

template<class T> bool CTreeView<T>::GetSelectedTreeItemsRect(SRect& outRect) const
{
	Uint32 nSelected = GetFirstSelectedTreeItem();
	if (!nSelected)
		return false;
		
	return GetTreeItemRect(nSelected, outRect);
}

template<class T> Uint32 CTreeView<T>::PointToTreeItem(const SPoint& inPt) const
{
	Uint32 nListIndex = PointToItem(inPt);
	return ConvertListToTree(nListIndex);
}

template<class T> void CTreeView<T>::Draw(TImage inImage, const SRect& inUpdateRect, Uint32 /* inDepth */)
{
	Int32 nTabHeight = GetTabHeight();
	
	// draw tab
	SRect r;
	if (nTabHeight)
	{
		r.SetEmpty();
		ItemDraw(0, inImage, r, r);
	}

	// draw items
	Uint32 itemCount = GetItemCount();
	Uint32 i, endItem;

	// calc starting item
	if (inUpdateRect.top > mBounds.top + nTabHeight)
	{
		i = (inUpdateRect.top - mBounds.top - nTabHeight) / mCellHeight + 1;
		if (i > itemCount) return;
	}
	else
		i = 1;
	
	// calc ending item
	endItem = ((inUpdateRect.bottom <= mBounds.bottom ? inUpdateRect.bottom : mBounds.bottom) - mBounds.top - nTabHeight) / mCellHeight + 1;
	if (endItem > itemCount) endItem = itemCount;
		
	// draw each item
	while (i <= endItem)
	{
		GetItemRect(i, r);
		ItemDraw(i, inImage, r, inUpdateRect);
		i++;
	}
}

template<class T> void CTreeView<T>::DragEnter(const SDragMsgData& inInfo)
{
	CSelectableItemsView::DragEnter(inInfo);
}

template<class T> void CTreeView<T>::DragLeave(const SDragMsgData& inInfo)
{
	CSelectableItemsView::DragLeave(inInfo);
}

template<class T> void CTreeView<T>::MouseDown(const SMouseMsgData& inInfo)
{
	Uint32 nTabHeight = GetTabHeight();
	if (nTabHeight)
	{
		Uint32 nHorizPos = 0, nVertPos = 0;
		if (dynamic_cast<CScrollerView *>(mHandler))
			(dynamic_cast<CScrollerView *>(mHandler))->GetScroll(nHorizPos, nVertPos);

		if (inInfo.loc.y <= mBounds.top + nVertPos + nTabHeight)
		{
			CSelectableItemsView::MouseDown(inInfo);
			return;
		}
	}

	Uint32 nListIndex = PointToItem(inInfo.loc);	
	if (!nListIndex)
	{
		CSelectableItemsView::MouseDown(inInfo);
		return;
	}
		
	Uint32 nTreeIndex;
	STreeViewItem *pTreeViewItem = GetListItem(nListIndex, &nTreeIndex);
		
	if (pTreeViewItem && pTreeViewItem->bIsVisible && pTreeViewItem->nDisclosure != optTree_NoDisclosure)
	{
		Uint32 nIconLeft = mBounds.left;
	#if WIN32
		Uint32 nTreeViewLevel = mTreeView.GetItemLevel(nTreeIndex);

		if (nTreeViewLevel) 
			nIconLeft += mLevelWidth * (nTreeViewLevel - 1);
	#endif
		Uint32 nIconRight = nIconLeft + 24;

		if (inInfo.loc.x >= nIconLeft && inInfo.loc.x <= nIconRight)
		{				
			Uint32 nNextListIndex = nListIndex + 1;

			if (pTreeViewItem->nDisclosure == optTree_Collapsed)
			{
				pTreeViewItem->nDisclosure = optTree_Disclosed;
				Disclose(nTreeIndex, nNextListIndex, pTreeViewItem);
			}
			else if (pTreeViewItem->nDisclosure == optTree_Disclosed)
			{
				pTreeViewItem->nDisclosure = optTree_Collapsed;
				Collapse(nTreeIndex, nNextListIndex, pTreeViewItem);	
			}
			
			DisclosureChanged(nTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->nDisclosure);
			RefreshItem(nListIndex);
							
			return;
		}
	}
			
	CSelectableItemsView::MouseDown(inInfo);
}

template<class T> bool CTreeView<T>::KeyDown(const SKeyMsgData& inInfo)
{
	if (inInfo.keyCode == key_Left || inInfo.keyCode == key_Right)
	{
		Uint32 nTreeIndex = GetFirstSelectedTreeItem();
		
		if (nTreeIndex)
		{
			STreeViewItem *pTreeViewItem = mTreeView.GetItem(nTreeIndex);

			if (pTreeViewItem && pTreeViewItem->bIsVisible && ((inInfo.keyCode == key_Left && pTreeViewItem->nDisclosure == optTree_Disclosed) || (inInfo.keyCode == key_Right && pTreeViewItem->nDisclosure == optTree_Collapsed)))
			{		
				Uint32 nListIndex = ConvertTreeToList(nTreeIndex);
				Uint32 nNextListIndex = nListIndex + 1;

				if (inInfo.keyCode == key_Left)
				{
					pTreeViewItem->nDisclosure = optTree_Collapsed;
					Collapse(nTreeIndex, nNextListIndex, pTreeViewItem);	
				}
	
				if (inInfo.keyCode == key_Right)
				{
					pTreeViewItem->nDisclosure = optTree_Disclosed;
					Disclose(nTreeIndex, nNextListIndex, pTreeViewItem);
				}
				
				DisclosureChanged(nTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->nDisclosure);
				RefreshItem(nListIndex);
				
				return true;
			}
		}
	}

	if (inInfo.mods & modKey_Shift)
	{
		if (inInfo.keyCode == key_Up)
		{
			CollapseAllTreeItems();
			return true;
		}

		if (inInfo.keyCode == key_Down)
		{
			DiscloseAllTreeItems();
			return true;
		}
	}
	
	return CSelectableItemsView::KeyDown(inInfo);
}

// derived classes can provide implementation
template<class T> void CTreeView<T>::VisibilityChanged(Uint32, T*, bool){}
template<class T> void CTreeView<T>::SelectionChanged(Uint32, T*, bool){}
template<class T> void CTreeView<T>::DisclosureChanged(Uint32, T*, Uint8){}

#pragma mark -

template<class T> bool CTreeView<T>::IsValidItem(Uint32 inListIndex) const
{
	return (inListIndex != 0 && inListIndex <= GetItemCount());
}

template<class T> bool CTreeView<T>::GetNextItem(Uint32& ioListIndex) const
{
	ioListIndex++;
	return (ioListIndex <= GetItemCount());
}

template<class T> bool CTreeView<T>::GetPrevItem(Uint32& ioListIndex) const
{
	if (ioListIndex == 0)
	{
		ioListIndex = GetItemCount();
		return (ioListIndex != 0);
	}
		
	ioListIndex--;
	return (ioListIndex != 0 && ioListIndex <= GetItemCount());
}

template<class T> Uint32 CTreeView<T>::GetItemCount() const
{			
	return mListCount;
}

template<class T> void CTreeView<T>::SetItemSelect(Uint32 inListIndex, bool inSelect)
{
	Uint32 nTreeIndex;
	STreeViewItem *pTreeViewItem = GetListItem(inListIndex, &nTreeIndex);
	if (!pTreeViewItem)
		return;
	
	if (pTreeViewItem->bIsSelected != inSelect)	// if change
	{
		pTreeViewItem->bIsSelected = inSelect;;
		RefreshItem(inListIndex);
		
		SelectionChanged(nTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->bIsSelected);
	}
}

template<class T> bool CTreeView<T>::IsItemSelected(Uint32 inListIndex) const
{
	STreeViewItem *pTreeViewItem = GetListItem(inListIndex);
	if (!pTreeViewItem)
		return false;

	return pTreeViewItem->bIsSelected;
}

template<class T> Uint32 CTreeView<T>::GetFirstSelectedItem() const
{
	return ConvertTreeToList(GetFirstSelectedTreeItem());
}

template<class T> Uint32 CTreeView<T>::GetLastSelectedItem() const
{
	return ConvertTreeToList(GetLastSelectedTreeItem());
}

template<class T> void CTreeView<T>::SetItemRangeSelect(Uint32 inFirstListIndex, Uint32 inLastListIndex, bool inSelect)
{
	Uint32 nItemCount = GetItemCount();
	SRect updateRect, r;
	
	// ensure that start <= end
	if (inFirstListIndex > inLastListIndex)
	{
		Uint32 nIndex = inFirstListIndex;
		inFirstListIndex = inLastListIndex;
		inLastListIndex = nIndex;
	}

	// bring into range
	if (nItemCount == 0) return;
	if (inFirstListIndex < 1) inFirstListIndex = 1;
	if (inLastListIndex > nItemCount) inLastListIndex = nItemCount;

	// by default, nothing needs to be updated
	updateRect.SetEmpty();

	// set each item
	for (; inFirstListIndex <= inLastListIndex; inFirstListIndex++)
	{
		Uint32 nTreeIndex;
		STreeViewItem *pTreeViewItem = GetListItem(inFirstListIndex, &nTreeIndex);
		
		if (pTreeViewItem && pTreeViewItem->bIsSelected != inSelect)	// if change
		{
			pTreeViewItem->bIsSelected = inSelect;
			SelectionChanged(nTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->bIsSelected);
			
			GetItemRect(inFirstListIndex, r);
			updateRect |= r;
		}
	}
	
	// refresh area that changed
	Refresh(updateRect);
}

template<class T> void CTreeView<T>::GetItemRect(Uint32 inListIndex, SRect& outRect) const
{
	outRect.left = mBounds.left;
	outRect.right = mBounds.right;
	outRect.top = mBounds.top + GetTabHeight() + (inListIndex - 1) * mCellHeight;
	outRect.bottom = outRect.top + mCellHeight;
}

template<class T> void CTreeView<T>::GetSelectedItemsRect(SRect& outRect) const
{
	Uint32 nFirstListIndex = GetFirstSelectedItem();
	Uint32 nLastListIndex = GetLastSelectedItem();
	
	if (nFirstListIndex == 0 || nLastListIndex == 0)
		outRect.SetEmpty();
	else
	{
		outRect.left = mBounds.left;
		outRect.right = mBounds.right;
		outRect.top = mBounds.top + GetTabHeight() + (nFirstListIndex - 1) * mCellHeight;
		outRect.bottom = mBounds.top + GetTabHeight() + nLastListIndex * mCellHeight;
	}
}

template<class T> Uint32 CTreeView<T>::PointToItem(const SPoint& inPt) const
{
	Uint32 nIndex;
	Int32 nDepl = inPt.v - mBounds.top - GetTabHeight();

	if (nDepl <= 0)
		nIndex = 1;
	else
		nIndex = (Uint32)nDepl / mCellHeight + 1;
	
	Uint32 nCount = GetItemCount();
	if (nIndex > nCount)
		nIndex = 0;
	
	return nIndex;
}

template<class T> void CTreeView<T>::ItemDraw(Uint32 inListIndex, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions)
{
	// draw tab
	if (!inListIndex && GetTabHeight())
	{
		ItemDraw(0, 0, nil, nil, inImage, inBounds, inUpdateRect, inOptions);
		return;
	}
	
	// draw item
	Uint32 nTreeViewIndex;
	STreeViewItem *pTreeViewItem = GetListItem(inListIndex, &nTreeViewIndex);
	if (!pTreeViewItem)
		return;
		
	Uint32 nTreeViewLevel = mTreeView.GetItemLevel(nTreeViewIndex);
	
	SRect r;
	SColor stHiliteCol;
	bool bIsActive = IsFocus() && mIsEnabled;

    inImage->SetPenSize(1);
	
	if (pTreeViewItem->bIsSelected)
	{
		if (inOptions == kDrawItemForDrag)
			r = inBounds;
		else
			r.Set(inBounds.left + 1, inBounds.top + 1, inBounds.right - 1, inBounds.bottom);

		UUserInterface::GetHighlightColor(&stHiliteCol);
		inImage->SetInkColor(stHiliteCol);

		if (bIsActive)
			inImage->FillRect(r);
		else
			inImage->FrameRect(r);
	}

	// draw light lines around this item
	if (inOptions != kDrawItemForDrag)
	{
		UUserInterface::GetSysColor(sysColor_Light, stHiliteCol);
		inImage->SetInkColor(stHiliteCol);
		r = inBounds;
		r.bottom++;
		inImage->FrameRect(r);
	}

	if (pTreeViewItem->nDisclosure != optTree_NoDisclosure)
	{
		// draw icon
		r.top = inBounds.top + 2;
		r.bottom = r.top + 16;
		r.left = inBounds.left + 4;
	#if WIN32
		if (nTreeViewLevel) 
			r.left += mLevelWidth * (nTreeViewLevel - 1);
	#endif
		r.right = r.left + 16;
	
		if (pTreeViewItem->nDisclosure == optTree_Collapsed)
			mCollapseIcon->Draw(inImage, r, align_Center, (bIsActive && pTreeViewItem->bIsSelected) ? transform_Dark : transform_None);
		else if (pTreeViewItem->nDisclosure == optTree_Disclosed)
			mDiscloseIcon->Draw(inImage, r, align_Center, (bIsActive && pTreeViewItem->bIsSelected) ? transform_Dark : transform_None);
	}
	
	r = inBounds;
	r.left += 24;
	if (nTreeViewLevel) 
		r.left += mLevelWidth * (nTreeViewLevel - 1);
	
	ItemDraw(nTreeViewIndex, nTreeViewLevel, (T *)pTreeViewItem->pTreeItem, pTreeViewItem, inImage, r, inUpdateRect, inOptions);
}

template<class T> void CTreeView<T>::ItemMouseDown(Uint32 inListIndex, const SMouseMsgData& inInfo)
{
	CSelectableItemsView::ItemMouseDown(inListIndex, inInfo);
	
	TreeItemMouseDown(ConvertListToTree(inListIndex), inInfo);
}

template<class T> void CTreeView<T>::ItemMouseUp(Uint32 inListIndex, const SMouseMsgData& inInfo)
{
	CSelectableItemsView::ItemMouseUp(inListIndex, inInfo);

	TreeItemMouseUp(ConvertListToTree(inListIndex), inInfo);
}

template<class T> void CTreeView<T>::ItemMouseEnter(Uint32 inListIndex, const SMouseMsgData& inInfo)
{
	CSelectableItemsView::ItemMouseEnter(inListIndex, inInfo);

	TreeItemMouseEnter(ConvertListToTree(inListIndex), inInfo);
}

template<class T> void CTreeView<T>::ItemMouseMove(Uint32 inListIndex, const SMouseMsgData& inInfo)
{
	CSelectableItemsView::ItemMouseMove(inListIndex, inInfo);

	TreeItemMouseMove(ConvertListToTree(inListIndex), inInfo);
}

template<class T> void CTreeView<T>::ItemMouseLeave(Uint32 inListIndex, const SMouseMsgData& inInfo)
{
	CSelectableItemsView::ItemMouseLeave(inListIndex, inInfo);

	TreeItemMouseLeave(ConvertListToTree(inListIndex), inInfo);
}

#pragma mark -

// derived classes can provide implementation
template<class T> void CTreeView<T>::ItemDraw(Uint32, Uint32, T*, STreeViewItem*, TImage, const SRect&, const SRect&, Uint32){}
template<class T> void CTreeView<T>::TreeItemMouseDown(Uint32, const SMouseMsgData&){}
template<class T> void CTreeView<T>::TreeItemMouseUp(Uint32, const SMouseMsgData&){}
template<class T> void CTreeView<T>::TreeItemMouseEnter(Uint32, const SMouseMsgData&){}
template<class T> void CTreeView<T>::TreeItemMouseMove(Uint32, const SMouseMsgData&){}
template<class T> void CTreeView<T>::TreeItemMouseLeave(Uint32, const SMouseMsgData&){}

template<class T> STreeViewItem *CTreeView<T>::GetListItem(Uint32 inListIndex, Uint32 *outTreeIndex) const
{
	if (!inListIndex || inListIndex > mListCount)
		return nil;
	
	Uint32 nTreeIndex = mListIndex[inListIndex - 1];
	
    if (outTreeIndex)
		*outTreeIndex = nTreeIndex;
	
	return mTreeView.GetItem(nTreeIndex);
}

template<class T> Uint32 CTreeView<T>::GetListIndex(STreeViewItem *inTreeViewItem) const
{
	if (!inTreeViewItem)
		return nil;
	
	Uint32 i = 0;
	STreeViewItem *pTreeViewItem;

	Uint32 nIndexCount = 0;
	while (mTreeView.GetNext(pTreeViewItem, i))
	{
		if (pTreeViewItem->bIsVisible)
		{
			nIndexCount++;
		 	if (pTreeViewItem == inTreeViewItem)
				return nIndexCount;
		}
	}
			
	return 0;
}

template<class T> Uint32 CTreeView<T>::ConvertListToTree(Uint32 inListIndex) const
{
	if (!inListIndex || inListIndex > mListCount)
		return 0;
	
	return mListIndex[inListIndex - 1];
}

template<class T> Uint32 CTreeView<T>::ConvertTreeToList(Uint32 inTreeIndex) const
{
	if (!inTreeIndex)
		return 0;
	
	for (Uint32 i=0; i < mListCount; i++)
	{
	    if (mListIndex[i] == inTreeIndex)
	        return i + 1; 
	}
			
	return 0;
}

template<class T> void CTreeView<T>::Collapse(Uint32 inTreeIndex, Uint32 inListIndex, STreeViewItem *inTreeViewItem)
{
	if (!inTreeViewItem)
		return;
		
	if (!mTreeView.GetChildTreeCount(inTreeIndex))
		return;

	STreeViewItem *pTreeViewItem = mTreeView.GetItem(++inTreeIndex);
	do
	{			
		pTreeViewItem->bIsVisible = false;
		VisibilityChanged(inTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->bIsVisible);

		if (pTreeViewItem->bIsSelected)
		{
			pTreeViewItem->bIsSelected = false;
			SelectionChanged(inTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->bIsSelected);
		}
			
		mListCount--;
		ItemsRemoved(inListIndex, 1);
		
		if (pTreeViewItem->nDisclosure == optTree_Disclosed)
			Collapse(inTreeIndex, inListIndex, pTreeViewItem);
	
	} while (mTreeView.GetNext(pTreeViewItem, inTreeIndex, true));
	
	ComposeListIndex();
}

template<class T> void CTreeView<T>::Disclose(Uint32 inTreeIndex, Uint32& ioListIndex, STreeViewItem *inTreeViewItem)
{
	if (!inTreeViewItem)
		return;
		
	if (!mTreeView.GetChildTreeCount(inTreeIndex))
		return;

	STreeViewItem *pTreeViewItem = mTreeView.GetItem(++inTreeIndex);
	do
	{		
		pTreeViewItem->bIsVisible = true;
		VisibilityChanged(inTreeIndex, (T *)pTreeViewItem->pTreeItem, pTreeViewItem->bIsVisible);

		mListCount++;		
		ItemsInserted(ioListIndex++, 1);
		
		if (pTreeViewItem->nDisclosure == optTree_Disclosed)
			Disclose(inTreeIndex, ioListIndex, pTreeViewItem);
	
	} while (mTreeView.GetNext(pTreeViewItem, inTreeIndex, true));
	
	ComposeListIndex();
}

template<class T> void CTreeView<T>::ItemsInserted(Uint32 inAtListIndex, Uint32 inCount)
{
	SRect stUpdateRect;
	stUpdateRect = mBounds;
	stUpdateRect.top = mBounds.top + GetTabHeight() + (inAtListIndex - 1) * mCellHeight;

	Refresh(stUpdateRect);
	
	CSelectableItemsView::ItemsInserted(inAtListIndex, inCount);
}

template<class T> void CTreeView<T>::ItemsRemoved(Uint32 inAtListIndex, Uint32 inCount)
{
	SRect stUpdateRect;
	stUpdateRect = mBounds;
	stUpdateRect.top = mBounds.top + GetTabHeight() + (inAtListIndex - 1) * mCellHeight;

	Refresh(stUpdateRect);
	
	CSelectableItemsView::ItemsRemoved(inAtListIndex, inCount);
}

template<class T> STreeViewItem *CTreeView<T>::GetTreeViewItem(T *inTreeItem, Uint32 *outTreeIndex) const
{
	if (!inTreeItem)
		return nil;
	
	Uint32 i = 0;
	STreeViewItem *pTreeViewItem;
	
	while (mTreeView.GetNext(pTreeViewItem, i))
	{
		if (pTreeViewItem->pTreeItem == inTreeItem)
		{
			if (outTreeIndex)
				*outTreeIndex = i;
			
			return pTreeViewItem;
		}
	}
	
	return nil;
}

template<class T> Int32 CTreeView<T>::CompareItemsProc(void *inItemA, void *inItemB, void *inRef)
{
	if (!((CTreeView *)inRef)->mCompareProc || !inItemA || !inItemB)
		return 0;
	
	return ((CTreeView *)inRef)->mCompareProc(((STreeViewItem *)inItemA)->pTreeItem, ((STreeViewItem *)inItemB)->pTreeItem, ((CTreeView *)inRef)->mCompareRef);
}

template<class T> void CTreeView<T>::ComposeListIndex()
{
	if (!mListCount)
	{
		if (mListIndex)
		{
			UMemory::Dispose((TPtr)mListIndex);
			mListIndex = nil;
		}
		
		mMaxCount = 0;
		return;
	}
	
	if (mListIndex)
	{
		if (mListCount >= mMaxCount || mMaxCount - mListCount > 32)
		{
			mMaxCount = RoundUp8(mListCount + 8);
			mListIndex = (Uint32 *)UMemory::Reallocate((TPtr)mListIndex, mMaxCount * sizeof(Uint32));
		}
	}
	else
	{
		mMaxCount = RoundUp8(mListCount + 8);
		mListIndex = (Uint32 *)UMemory::New(mMaxCount * sizeof(Uint32));
	}
	
	Uint32 i = 0;
	STreeViewItem *pTreeViewItem;

	Uint32 nIndexCount = 0;
	while (mTreeView.GetNext(pTreeViewItem, i))
	{
		if (pTreeViewItem->bIsVisible)
		{
			mListIndex[nIndexCount] = i;
			
		    if (++nIndexCount >= mListCount)
			    break;
		} 
	}
}

