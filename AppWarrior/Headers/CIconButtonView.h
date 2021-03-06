/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "CView.h"
#include "UIconButton.h"
#include "URez.h"

class CIconButtonView : public CView
{	
	public:
		// construction
		CIconButtonView(CViewHandler *inHandler, const SRect& inBounds);
		CIconButtonView(CViewHandler *inHandler, const SRect& inBounds, Uint32 inCmdID, const Uint8 *inTitle, Int32 inIconID, Uint32 inOptions, TFontDesc inFont = nil);
		virtual ~CIconButtonView();
		
		// title
		virtual void SetTitle(const void *inText, Uint32 inSize);
		void SetTitle(const Uint8 *inText)						{	SetTitle(inText+1, inText[0]);							}
		Uint32 GetTitle(void *outText, Uint32 inMaxSize) const	{	return UMemory::Read(mTitle, 0, outText, inMaxSize);	}
		virtual void SetFont(TFontDesc inFont);

		// properties
#if MACINTOSH
		virtual void SetIcon(TIcon inIcon);
		void SetIconID(Int32 inID);
		TIcon GetIcon() const;
#else
		void SetIcon(Int32 inID)								{	mRez.Reload('ICON', inID); Refresh();					}
		void SetIcon(TRez inRez, Int32 inID)					{	mRez.Reload(inRez, 'ICON', inID, true); Refresh();		}
		void SetIcon(THdl inHdl)								{	mRez.SetHdl(inHdl); Refresh();							}
		void SetIconID(Int32 inID)								{	SetIcon(inID);											}
#endif
		virtual void SetOptions(Uint16 inOptions);
		Uint16 GetOptions() const								{	return mOptions;										}

		bool IsFlashHilited() const								{	return mIsFlashHilited;									}
		virtual void Flash();
		
		// misc
		virtual void Timer(TTimer inTimer);
		void HitButton(Uint16 inButton = 1, Uint16 inMods = 0)	{	Hit(hitType_Standard, inButton, inMods);				}
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
#if MACINTOSH
		TIcon mIcon;
#else
		StRezLoader mRez;
#endif
		TTimer mFlashTimer;
		Uint16 mOptions;
		Uint8 mIsFocused		: 1;
		Uint8 mIsHilited		: 1;
		Uint8 mIsFlashHilited	: 1;
		Uint8 mIsAltHit			: 1;
		Uint8 mIsDrawDisabled	: 1;
		
		// internal functions
		void UpdateActive();
};


