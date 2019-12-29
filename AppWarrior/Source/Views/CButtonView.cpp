/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UButton.h"
#include "UDateTime.h"

#pragma options align=packed

typedef struct {
	Int16 textStyle;
	Uint16 isDefault : 1;
	Int16 script;
	Uint16 titleLen;
	Uint8 titleData[];
} SButtonView;

#pragma options align=reset

/* -------------------------------------------------------------------------- */

CButtonView::CButtonView(CViewHandler *inHandler, const SRect& inBounds)
	: CView(inHandler, inBounds)
{
#if WIN32
	mCanFocus = true;
#endif

	mFont = nil;
	mTitle = nil;
	mFlashTimer = nil;
	mIsDefault = mIsFocused = mIsHilited = mIsFlashHilited = mIsAltHit = mIsDrawDisabled = false;
}

CButtonView::CButtonView(CViewHandler *inHandler, const SRect& inBounds, Uint32 inCmdID, const Uint8 *inTitle, Uint32 inDefault, TFontDesc inFont)
	: CView(inHandler, inBounds)
{
#if WIN32
	mCanFocus = true;
#endif

	mCommandID = inCmdID;
	mFont = inFont;
	mFlashTimer = nil;
	mIsDefault = inDefault ? true : false;
	mIsFocused = mIsHilited = mIsFlashHilited = mIsAltHit = mIsDrawDisabled = false;
	mTitle = inTitle ? UMemory::NewHandle(inTitle+1, inTitle[0]) : nil;
}

CButtonView::~CButtonView()
{
	delete mFont;
	delete mTitle;
	delete mFlashTimer;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CButtonView::SetTitle(const void *inText, Uint32 inSize)
{
	if (mTitle == nil)
		mTitle = UMemory::NewHandle(inText, inSize);
	else
		UMemory::Set(mTitle, inText, inSize);
	
	Refresh();
}

// takes ownership of the TFontDesc
void CButtonView::SetFont(TFontDesc inFont)
{
	delete mFont;
	mFont = inFont;
	Refresh();
}

void CButtonView::SetDefault(bool inDefault)
{
	if (mIsDefault != (inDefault != 0))
	{
		mIsDefault = (inDefault != 0);
		Refresh();
	}
}

void CButtonView::Flash()
{
	if (!mIsFlashHilited)
	{
		mFlashTimer = StartNewTimer(150);
		
		mIsFlashHilited = true;
		if (!mIsHilited)
			Refresh();
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CButtonView::Draw(TImage inImage, const SRect& /* inUpdateRect */, Uint32 inDepth)
{
	SButtonInfo info;

	if (mTitle == nil)
	{
		info.title = nil;
		info.titleSize = 0;
	}
	else
	{
		info.titleSize = UMemory::GetSize(mTitle);
		info.title = UMemory::Lock(mTitle);
	}
	
	info.encoding = 0;
	
	try
	{
		info.options = mIsDefault ? 1 : 0;
		if (inDepth < 4) info.options |= 0x8000;
		
		inImage->SetFont(mFont);

		if (mIsDrawDisabled)
			UButton::DrawDisabled(inImage, mBounds, info);
		else if (mIsHilited || mIsFlashHilited)
			UButton::DrawHilited(inImage, mBounds, info);
		else if (mIsFocused)
			UButton::DrawFocused(inImage, mBounds, info);
		else
			UButton::Draw(inImage, mBounds, info);
	}
	catch(...)
	{
		// don't throw
	}
	
	if (mTitle) 
		UMemory::Unlock(mTitle);
}

void CButtonView::MouseDown(const SMouseMsgData& inInfo)
{
	inherited::MouseDown(inInfo);
	
	if (IsEnabled() && IsActive())
	{
		mIsFlashHilited = false;
		mIsHilited = true;
		Refresh();
	}
}

void CButtonView::MouseUp(const SMouseMsgData& inInfo)
{
	inherited::MouseUp(inInfo);
	
	if (mIsHilited)
	{
		mIsHilited = false;
		Refresh();
		
		if (IsMouseWithin())
			Hit((inInfo.mods & modKey_Option) ? hitType_Alternate : hitType_Standard, inInfo.button, inInfo.mods);
	}
}

void CButtonView::MouseEnter(const SMouseMsgData& inInfo)
{
	inherited::MouseEnter(inInfo);
	
	if (!mIsHilited && IsAnyMouseBtnDown() && IsEnabled() && IsActive())
	{
		mIsHilited = true;
		Refresh();
	}
}

void CButtonView::MouseLeave(const SMouseMsgData& inInfo)
{
	inherited::MouseLeave(inInfo);
	
	if (mIsHilited)
	{
		mIsHilited = false;
		Refresh();
	}
}

bool CButtonView::KeyDown(const SKeyMsgData& inInfo)
{
	if (IsEnabled() && IsActive())
	{
		if (inInfo.keyCode == key_nEnter || inInfo.keyCode == key_Return || inInfo.keyCode == key_Escape || (inInfo.keyChar == '.' && (inInfo.mods & modKey_Command)))
		{
			if (!mIsFlashHilited)
			{
				mIsAltHit = (inInfo.mods & modKey_Option) != 0;
				Flash();
			}
			
			return true;
		}
	}
	
	return false;
}

void CButtonView::UpdateActive()
{
	bool bStateChanged = false;
	
	bool bIsFocused = IsEnabled() && IsFocus();
	if (bIsFocused != mIsFocused)
	{
		mIsFocused = bIsFocused;
		if (mIsFocused) mIsDrawDisabled = mIsHilited = mIsFlashHilited = false;
		bStateChanged = true;
	}

	bool bDrawDisab = IsDisabled() || IsInactive();
	if (bDrawDisab != mIsDrawDisabled)	// if change
	{
		mIsDrawDisabled = bDrawDisab;
		if (mIsDrawDisabled) mIsHilited = mIsFlashHilited = false;
		bStateChanged = true;
	}
	
	if (bStateChanged)
		Refresh();
}

bool CButtonView::ChangeState(Uint16 inState)
{
	if (mState != inState)	// if change
	{
		mState = inState;
		UpdateActive();
		return true;
	}
	
	return false;
}

bool CButtonView::SetEnable(bool inEnable)
{
	if (mIsEnabled != (inEnable != 0))		// if change
	{
		mIsEnabled = (inEnable != 0);
		UpdateActive();
		
		return true;
	}
	
	return false;
}

void CButtonView::Timer(TTimer inTimer)
{
	if (inTimer == mFlashTimer)
	{
		delete mFlashTimer;
		mFlashTimer = nil;
		
		if (mIsFlashHilited)
		{
			mIsFlashHilited = false;
			Refresh();
			Hit(mIsAltHit ? hitType_Alternate : hitType_Standard);
		}
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

#if WIN32

bool CButtonView::BuildButton(CContainerView *inHandler, const SRect& inBounds, SButtons *inButton)
{
	if (!inHandler || !inButton)
		return false;
		
	CButtonView *btn = nil;
	if (inButton->options & btnOpt_Regular || inButton->options & btnOpt_Default || inButton->options & btnOpt_Cancel)
	{
		Int32 nDeplHoriz = (inBounds.right - inBounds.left - 80)/2;
		if (nDeplHoriz < 0) nDeplHoriz = 0;

		Int32 nDeplVert = (inBounds.bottom - inBounds.top - 24)/2;
		if (nDeplVert < 0) nDeplVert = 0;

		btn = new CButtonView(inHandler, SRect(inBounds.left + nDeplHoriz, inBounds.top + nDeplVert, inBounds.left + nDeplHoriz + 80, inBounds.top + nDeplVert + 24));
	}
	
	if (!btn)
		return false;

	if (inButton->options & btnOpt_CommandID)
		btn->SetCommandID(inButton->id);
	else
		btn->SetID(inButton->id);
	
	btn->SetTitle(inButton->title);

	if (inButton->options & btnOpt_Default)
	{
		btn->SetDefault(true);
		inHandler->SetDefaultView(btn);	
	}
	else if (inButton->options & btnOpt_Cancel)
	{
		inHandler->SetCancelView(btn);
	}

	btn->Show();

	if (inButton->outBtn)
		*inButton->outBtn = btn;

	return true;
}

bool CButtonView::BuildButtons(CContainerView *inHandler, const SRect& inBounds, SButtons *inButtons)
{
	if (!inHandler || !inButtons)
		return false;

	SButtons *button1, *button2;
	if (inButtons[0].options & btnOpt_Default && inButtons[1].options & btnOpt_Cancel)
	{ 
		button1 = &inButtons[0];
		button2 = &inButtons[1];
	}
	else if (inButtons[0].options & btnOpt_Cancel && inButtons[1].options & btnOpt_Default)
	{
		button1 = &inButtons[1];
		button2 = &inButtons[0];
	}
	else 
		return false;

	Int32 nDeplVert = (inBounds.bottom - inBounds.top - 24)/2;
	if (nDeplVert < 0) nDeplVert = 0;
	
	CButtonView *btn = new CButtonView(inHandler, SRect(inBounds.left, inBounds.top + nDeplVert, inBounds.left + 80, inBounds.top + nDeplVert + 24));

	if (button1->options & btnOpt_CommandID)
		btn->SetCommandID(button1->id);
	else
		btn->SetID(button1->id);
	
	btn->SetTitle(button1->title);
	btn->SetDefault(true);
	inHandler->SetDefaultView(btn);	
	btn->Show();
	
	if (button1->outBtn)
		*button1->outBtn = btn;
	
	btn = new CButtonView(inHandler, SRect(inBounds.right - 80, inBounds.top + nDeplVert, inBounds.right, inBounds.top + nDeplVert + 24));

	if (button2->options & btnOpt_CommandID)
		btn->SetCommandID(button2->id);
	else
		btn->SetID(button2->id);
	
	btn->SetTitle(button2->title);
	inHandler->SetCancelView(btn);
	btn->Show();

	if (button2->outBtn)
		*button2->outBtn = btn;

	return true;
} 

#elif MACINTOSH

bool CButtonView::BuildButton(CContainerView *inHandler, const SRect& inBounds, SButtons *inButton)
{
	if (!inHandler || !inButton)
		return false;
		
	CButtonView *btn = nil;
	if (inButton->options & btnOpt_Regular || inButton->options & btnOpt_Cancel)
	{
		Int32 nDeplHoriz = (inBounds.right - inBounds.left - 80)/2;
		if (nDeplHoriz < 0) nDeplHoriz = 0;

		Int32 nDeplVert = (inBounds.bottom - inBounds.top - 20)/2;
		if (nDeplVert < 0) nDeplVert = 0;

		btn = new CButtonView(inHandler, SRect(inBounds.left + nDeplHoriz, inBounds.top + nDeplVert, inBounds.left + nDeplHoriz + 80, inBounds.top + nDeplVert + 20));
	}
	else if (inButton->options & btnOpt_Default)
	{
		Int32 nDeplHoriz = (inBounds.right - inBounds.left - 86)/2;
		if (nDeplHoriz < 0) nDeplHoriz = 0;

		Int32 nDeplVert = (inBounds.bottom - inBounds.top - 26)/2;
		if (nDeplVert < 0) nDeplVert = 0;

		btn = new CButtonView(inHandler, SRect(inBounds.left + nDeplHoriz, inBounds.top + nDeplVert, inBounds.left + nDeplHoriz + 86, inBounds.top + nDeplVert + 26));
	}
	
	if (!btn)
		return false;
	
	if (inButton->options & btnOpt_CommandID)
		btn->SetCommandID(inButton->id);
	else
		btn->SetID(inButton->id);
	
	btn->SetTitle(inButton->title);

	if (inButton->options & btnOpt_Default)
	{
		btn->SetDefault(true);
		inHandler->SetDefaultView(btn);	
	}
	else if (inButton->options & btnOpt_Cancel)
	{
		inHandler->SetCancelView(btn);
	}

	btn->Show();

	if (inButton->outBtn)
		*inButton->outBtn = btn;

	return true;
}

bool CButtonView::BuildButtons(CContainerView *inHandler, const SRect& inBounds, SButtons *inButtons)
{
	if (!inHandler || !inButtons)
		return false;

	SButtons *button1, *button2;
	if (inButtons[0].options & btnOpt_Default && inButtons[1].options & btnOpt_Cancel)
	{
		button1 = &inButtons[0];
		button2 = &inButtons[1];
	}
	else if (inButtons[0].options & btnOpt_Cancel && inButtons[1].options & btnOpt_Default)
	{
		button1 = &inButtons[1];
		button2 = &inButtons[0];
	}
	else 
		return false;

	Int32 nDeplVert = (inBounds.bottom - inBounds.top - 26)/2;
	if (nDeplVert < 0) nDeplVert = 0;
	
	CButtonView *btn = new CButtonView(inHandler, SRect(inBounds.right - 86, inBounds.top + nDeplVert, inBounds.right, inBounds.top + nDeplVert + 26));

	if (button1->options & btnOpt_CommandID)
		btn->SetCommandID(button1->id);
	else
		btn->SetID(button1->id);

	btn->SetTitle(button1->title);
	btn->SetDefault(true);
	inHandler->SetDefaultView(btn);	
	btn->Show();
	
	if (button1->outBtn)
		*button1->outBtn = btn;

	btn = new CButtonView(inHandler, SRect(inBounds.left, inBounds.top + nDeplVert + 3, inBounds.left + 80, inBounds.top + nDeplVert + 23));

	if (button2->options & btnOpt_CommandID)
		btn->SetCommandID(button2->id);
	else
		btn->SetID(button2->id);

	btn->SetTitle(button2->title);
	inHandler->SetCancelView(btn);
	btn->Show();

	if (button2->outBtn)
		*button2->outBtn = btn;

	return true;
}

#endif
