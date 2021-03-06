/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CCheckBoxView.h"
#include "UCheckBox.h"

#pragma options align=packed

typedef struct {
	Int16 titleStyle;
	Int16 style;
	Int16 mark;
	Uint16 isAutoMark : 1;
	Uint32 nextID;
	Int16 script;
	Uint16 titleLen;
	Uint8 titleData[];
} SCheckBoxView;

#pragma options align=reset

/* -------------------------------------------------------------------------- */

CCheckBoxView::CCheckBoxView(CViewHandler *inHandler, const SRect& inBounds)
	: CView(inHandler, inBounds)
{
#if WIN32
	mCanFocus = true;
#endif

	mFont = nil;
	mStyle = kSquareCheckBox;
	mMark = 0;
	mIsAutoMark = true;
	mIsFocused = mIsHilited = mIsDrawDisabled = false;
	mNext = nil;
	mTitle = nil;
}

CCheckBoxView::CCheckBoxView(CViewHandler *inHandler, const SRect& inBounds, const Uint8 *inTitle, Uint32 inStyle, TFontDesc inFont)
	: CView(inHandler, inBounds)
{
#if WIN32
	mCanFocus = true;
#endif

	mFont = inFont;
	mStyle = inStyle;
	mMark = 0;
	mIsAutoMark = true;
	mIsFocused = mIsHilited = mIsDrawDisabled = false;
	mNext = nil;
	mTitle = inTitle ? UMemory::NewHandle(inTitle+1, inTitle[0]) : nil;
}

CCheckBoxView::~CCheckBoxView()
{
	delete mFont;
	UMemory::Dispose(mTitle);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CCheckBoxView::SetTitle(const void *inText, Uint32 inSize)
{
	if (mTitle == nil)
		mTitle = UMemory::NewHandle(inText, inSize);
	else
		UMemory::Set(mTitle, inText, inSize);
	
	Refresh();
}

// takes ownership of the TFontDesc
void CCheckBoxView::SetFont(TFontDesc inFont)
{
	delete mFont;
	mFont = inFont;
	Refresh();
}

void CCheckBoxView::SetStyle(Uint16 inStyle)
{
	if (mStyle != inStyle)
	{
		mStyle = inStyle;
		RefreshBox();
	}
}

void CCheckBoxView::SetMark(Uint16 inMark)
{
	if (mMark != inMark)
	{
		mMark = inMark;
		mHasChanged = true;
		RefreshBox();
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CCheckBoxView::Draw(TImage inImage, const SRect& /* inUpdateRect */, Uint32 inDepth)
{
	SCheckBoxInfo info;
	
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
		info.style = mStyle;
		info.mark = mMark;
		info.options = (inDepth >= 4) ? 0 : 0x8000;

		inImage->SetFont(mFont);
		
		if (mIsDrawDisabled)
			UCheckBox::DrawDisabled(inImage, mBounds, info);
		else if (mIsHilited)
			UCheckBox::DrawHilited(inImage, mBounds, info);
		else if (mIsFocused)
			UCheckBox::DrawFocused(inImage, mBounds, info);
		else
			UCheckBox::Draw(inImage, mBounds, info);
	}
	catch(...)
	{
		// don't throw
	}
	
	if (mTitle) 
		UMemory::Unlock(mTitle);
}

void CCheckBoxView::MouseDown(const SMouseMsgData& inInfo)
{
	CView::MouseDown(inInfo);
	
	if (IsEnabled() && IsActive())
	{
		mIsHilited = true;
		RefreshBox();
	}
}

void CCheckBoxView::MouseUp(const SMouseMsgData& inInfo)
{
	CView::MouseUp(inInfo);
	
	if (mIsHilited)
	{
		mIsHilited = false;
		RefreshBox();
		
		if (mNext)
		{
			if (mMark == 0)
			{
				CCheckBoxView *v = mNext;
				
				while (v && v != this)
				{
					v->SetMark(false);
					v = v->mNext;
				}
				
				SetMark(true);
				Hit(hitType_Change);
			}
		}
		else if (mIsAutoMark && IsMouseWithin())
		{
			SetMark(!mMark);
			Hit(hitType_Change);
		}
	}
}

void CCheckBoxView::MouseEnter(const SMouseMsgData& inInfo)
{
	CView::MouseEnter(inInfo);
	
	if (!mIsHilited && IsAnyMouseBtnDown() && IsEnabled() && IsActive())
	{
		mIsHilited = true;
		Refresh();
	}
}

void CCheckBoxView::MouseLeave(const SMouseMsgData& inInfo)
{
	CView::MouseLeave(inInfo);
	
	if (mIsHilited)
	{
		mIsHilited = false;
		RefreshBox();
	}
}

bool CCheckBoxView::KeyDown(const SKeyMsgData& inInfo)
{
	if (IsEnabled() && IsActive() && inInfo.keyCode == key_Space)
	{
		if (mNext)
		{
			if (mMark == 0)
			{
				CCheckBoxView *v = mNext;
				
				while (v && v != this)
				{
					v->SetMark(false);
					v = v->mNext;
				}
				
				SetMark(true);
				Hit(hitType_Change);
			}
		}
		else if (mIsAutoMark)
		{
			SetMark(!mMark);
			Hit(hitType_Change);
		}
		
		return true;
	}
	
	return false;
}

void CCheckBoxView::UpdateActive()
{
	bool bStateChanged = false;
	
	bool bIsFocused = IsEnabled() && IsFocus();
	if (bIsFocused != mIsFocused)
	{
		mIsFocused = bIsFocused;
		if (mIsFocused) mIsDrawDisabled = mIsHilited = false;
		bStateChanged = true;
	}

	bool bDrawDisab = IsDisabled() || IsInactive();
	if (bDrawDisab != mIsDrawDisabled)	// if change
	{
		mIsDrawDisabled = bDrawDisab;
		if (mIsDrawDisabled) mIsHilited = false;
		bStateChanged = true;
	}
	
	if (bStateChanged)
		Refresh();
}

bool CCheckBoxView::ChangeState(Uint16 inState)
{
	if (mState != inState)	// if change
	{
		mState = inState;
		UpdateActive();
		return true;
	}
	
	return false;
}

bool CCheckBoxView::SetEnable(bool inEnable)
{
	if (mIsEnabled != (inEnable != 0))		// if change
	{
		mIsEnabled = (inEnable != 0);
		UpdateActive();
		
		return true;
	}
	
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CCheckBoxView::RefreshBox()
{
	SRect r;

	r = mBounds;
	r.left += 2;
	r.right = r.left + 12;
	r.top = r.top + (((r.bottom - r.top)/2) - (12/2));
	r.bottom = r.top + 12;
	
	Refresh(r);
}

