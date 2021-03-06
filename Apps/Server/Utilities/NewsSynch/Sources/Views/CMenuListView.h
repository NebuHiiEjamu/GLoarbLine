/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "CItemsView.h"

class CMenuListView : public CListView
{	
	public:
		// construction
		CMenuListView(CViewHandler *inHandler, const SRect& inBounds);
		virtual ~CMenuListView();
		
		// adding and removing items
		Uint32 InsertItems(Uint32 inAtItem, Uint32 inCount, const void *inText, Uint32 inSize);
		Uint32 InsertItem(Uint32 inAtItem, const void *inText, Uint32 inSize);
		Uint32 AddItem(const void *inText, Uint32 inSize);
		Uint32 AddItem(const Uint8 inTitle[]);
		void RemoveItems(Uint32 inAtItem, Uint32 inCount);
		void RemoveItem(Uint32 inItem);
		virtual Uint32 GetItemCount() const;
		void Clear();
		
		// adding dividers
		Uint32 InsertDivider(Uint32 inAtItem);
		Uint32 AddDivider();
		
		// set misc item properties
		void SetItemMark(Uint32 inItem, Char16 inMarkChar);
		void SetItemIcon(Uint32 inItem, Int16 inIconID);
		void SetItemCommand(Uint32 inItem, Uint32 inCmdNum);
		void SetItemRef(Uint32 inItem, Uint32 inRef);
		void SetItemEnable(Uint32 inItem, bool inEnable);
		void SetItemShortcut(Uint32 inItem, Uint16 inShortcut, bool inKeyCode = false);
		void SetItemTick(Uint32 inItem, bool inTicked);

		// get misc item properties
		Char16 GetItemMark(Uint32 inItem) const;
		Int16 GetItemIcon(Uint32 inItem) const;
		Uint32 GetItemCommand(Uint32 inItem) const;
		Uint32 GetItemRef(Uint32 inItem) const;
		bool IsItemEnabled(Uint32 inItem) const;
		bool IsItemTicked(Uint32 inItem) const;
		
		// font
		void SetItemFont(Uint32 inItem, Int16 inFontID, Uint16 inFontSize, Uint16 inFontStyle);
		void SetItemTextStyle(Uint32 inItem, Int16 inID);
		Int16 GetItemTextStyle(Uint32 inItem) const;
		void AppendFontNames();
		
		// title
		void SetItemTitle(Uint32 inItem, const void *inText, Uint32 inSize);
		Uint32 GetItemTitle(Uint32 inItem, void *outText, Uint32 inMaxSize) const;
		
		// full size
		virtual Uint32 GetFullWidth() const;

		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);

	protected:
		THdl mData, mItemOffsets;
		
		// internal functions
		virtual void ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions);
		static void AddFontNameProc(const Uint8 *inName, Uint32 inEncoding, Uint32 inFlags, void *inRef);
};


