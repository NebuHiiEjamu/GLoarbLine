/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "CView.h"

enum {
	kSquareCheckBox		= 0,
	kRoundCheckBox		= 1,
	kTickBox			= kSquareCheckBox
};

class CCheckBoxView : public CView
{	
	public:
		// construction
		CCheckBoxView(CViewHandler *inHandler, const SRect& inBounds);
		CCheckBoxView(CViewHandler *inHandler, const SRect& inBounds, const Uint8 *inTitle, Uint32 inStyle = 0, TFontDesc inFont = nil);
		virtual ~CCheckBoxView();
		
		// title
		virtual void SetTitle(const void *inText, Uint32 inSize);
		void SetTitle(const Uint8 *inText)						{	SetTitle(inText+1, inText[0]);							}
		Uint32 GetTitle(void *outText, Uint32 inMaxSize) const	{	return UMemory::Read(mTitle, 0, outText, inMaxSize);	}
		virtual void SetFont(TFontDesc inFont);

		// properties
		virtual void SetStyle(Uint16 inStyle);
		Uint16 GetStyle() const									{	return mStyle;											}
		virtual void SetMark(Uint16 inMark);
		Uint16 GetMark() const									{	return mMark;											}
		void SetAutoMark(bool inAutoMark)						{	mIsAutoMark = (inAutoMark != 0);						}
		bool IsAutoMark() const									{	return mIsAutoMark;										}
		void SetExclusiveNext(CCheckBoxView *inNext)			{	mNext = inNext;											}
		CCheckBoxView *GetExclusiveNext() const					{	return mNext;											}
		
		virtual bool ChangeState(Uint16 inState);
		virtual bool SetEnable(bool inEnable);
		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);
		virtual void MouseDown(const SMouseMsgData& inInfo);
		virtual void MouseUp(const SMouseMsgData& inInfo);
		virtual void MouseEnter(const SMouseMsgData& inInfo);
		virtual void MouseLeave(const SMouseMsgData& inInfo);
		virtual bool KeyDown(const SKeyMsgData& inInfo);

	protected:
		THdl mTitle;
		TFontDesc mFont;
		CCheckBoxView *mNext;
		Uint8 mStyle, mMark;
		Uint8 mIsAutoMark		: 1;
		Uint8 mIsFocused		: 1;
		Uint8 mIsHilited		: 1;
		Uint8 mIsDrawDisabled	: 1;
		
		// misc
		void UpdateActive();
		virtual void RefreshBox();
};

