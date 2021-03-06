/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "CPopupView.h"

class CButtonPopupView : public CPopupView
{
	public:
		// construction
		CButtonPopupView(CViewHandler *inHandler, const SRect& inBounds);
			
		virtual bool ChangeState(Uint16 inState);
		virtual bool SetEnable(bool inEnable);
		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);
		virtual void MouseDown(const SMouseMsgData& inInfo);
		virtual bool KeyDown(const SKeyMsgData& inInfo);

	protected:
		Uint8 mIsFocused		: 1;
		Uint8 mIsHilited		: 1;
		Uint8 mIsDrawDisabled	: 1;

		// popup functions
		virtual void GetPopupButtonRect(SRect& outRect);
		virtual void ShowPopupWin();
		virtual void HidePopupWin();

		// internal functions
		void UpdateActive();
};

class CMenuListView;
CButtonPopupView *MakePopupMenuView(CViewHandler *inHandler, const SRect& inBounds, CMenuListView **outMenu = nil);


