/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"


struct STabItemInfo {
	Uint8 psTabName[64];
	Uint16 nTabWidth;
	Uint16 nMinWidth;
	Uint8 nTabAlign;
};


// tabbed view class
class CTabbedItemsView
{
	public:
		// construction
		CTabbedItemsView(bool inShowTabLine = false);
		virtual ~CTabbedItemsView();

		// add/remove
		Uint32 AddTab(const Uint8 *inTabName, Uint16 inTabWidth = 1, Uint16 inMinWidth = 1, Uint8 inTabAlign = textAlign_Left);
		Uint32 InsertTab(Uint32 inIndex, const Uint8 *inTabName, Uint16 inTabWidth = 1, Uint16 inMinWidth = 1, Uint8 inTabAlign = textAlign_Left);
		bool DeleteTab(Uint32 inIndex);
		
		// set
		bool SetTab(Uint32 inIndex, const Uint8 *inTabName, Uint16 inTabWidth = 1, Uint16 inMinWidth = 1, Uint8 inTabAlign = textAlign_Left);
		bool SetTabName(Uint32 inIndex, const Uint8 *inTabName);
		bool SetTabWidth(Uint32 inIndex, Uint16 inTabWidth);
		bool SetTabPercent(const CPtrList<Uint8>& inPercentList);

		// get
		STabItemInfo *GetTab(Uint32 inIndex);
		Uint16 GetTabWidth(Uint32 inIndex);
		Uint8 GetTabPercent(Uint32 inIndex);
		Uint32 GetTabCount();

		// mouse events
		void TabMouseDown(const SMouseMsgData& inInfo);
		void TabMouseUp(const SMouseMsgData& inInfo);
		void TabMouseMove(const SMouseMsgData& inInfo);
		void TabMouseLeave(const SMouseMsgData& inInfo);
		
	protected:
		CPtrList<STabItemInfo> mTabInfoList;
		CPtrList<SRect> mTabRectList;

		Uint16 mTabHeight;
		Uint32 mMoveTabIndex;
		bool mShowTabLine;
		
		SColor mTabColor;
		SColor mTabTextColor;
		SColor mTabLineColor;

		virtual void RefreshTabView() = 0;
		virtual void GetTabViewBounds(SRect& outBounds) = 0;
		virtual void GetTabViewScrollPos(Uint32& outHorizPos, Uint32& outVertPos) = 0;

		void DrawTabHeader(TImage inImage, const SRect& inBounds);

		void SetTabRectList();
		bool IsMouseOnTab(const SPoint& inLoc);
		Uint32 GetTabIndex(const SPoint& inLoc);
};


