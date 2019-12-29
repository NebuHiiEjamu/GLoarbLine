/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CTextView.h"

#pragma options align=packed

typedef struct {
	Int16 textStyleID;
	Uint16 isEditable	: 1;
	Uint16 isSelectable	: 1;
	Uint32 selectStart, selectEnd;
	Uint32 rsvd[4];
	Int16 textID;
	Int16 script;
	Uint32 textSize;
	Uint8 textData[];
} STextView;

#pragma options align=reset

/* -------------------------------------------------------------------------- */

CTextView::CTextView(CViewHandler *inHandler, const SRect& inBounds, Uint16 inMargin)
	: CView(inHandler, inBounds)
{
	mCaretTimer = nil;

	mReturnKeyAction = enterKeyAction_NewLine;
	mEnterKeyAction = enterKeyAction_None;

	mTabSelect = true;
	
	mText = UEditText::New(inBounds, inMargin);
	
	try
	{
		UEditText::SetActive(mText, false);
		UEditText::SetRef(mText, this);
		UEditText::SetDrawHandler(mText, TextDrawHandler);
		UEditText::SetScreenDeltaHandler(mText, TextScreenDeltaHandler);
	}
	catch(...)
	{
		UEditText::Dispose(mText);
		throw;
	}
}

CTextView::~CTextView()
{
	delete mCaretTimer;
	UEditText::Dispose(mText);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CTextView::SetFont(TFontDesc inFont)
{
	UEditText::SetFont(mText, inFont);
	if (mSizing & sizing_FullHeight) SetFullHeight();
}

void CTextView::SetFont(const Uint8 *inName, const Uint8 *inStyle, Uint32 inSize, Uint32 inEffect)
{
	UEditText::SetFont(mText, inName, inStyle, inSize, inEffect);
	if (mSizing & sizing_FullHeight) SetFullHeight();
}

void CTextView::SetFontSize(Uint32 inSize)
{
	UEditText::SetFontSize(mText, inSize);
	if (mSizing & sizing_FullHeight) SetFullHeight();
}

void CTextView::SetText(const void *inText, Uint32 inLength)
{
	UEditText::SetText(mText, inText, inLength);
	if (mSizing & sizing_FullHeight) SetFullHeight();
}

void CTextView::SetTextHandle(THdl inHdl)
{
	UEditText::SetTextHandle(mText, inHdl);	
	if (mSizing & sizing_FullHeight) SetFullHeight();
}

THdl CTextView::DetachTextHandle()
{
	THdl h = UEditText::DetachTextHandle(mText);
	if (mSizing & sizing_FullHeight) SetFullHeight();
	return h;
}

void CTextView::InsertText(Uint32 inOffset, const void *inText, Uint32 inTextLength, bool inUpdateSelect)
{
	UEditText::InsertText(mText, inOffset, inText, inTextLength, inUpdateSelect);
	if (mSizing & sizing_FullHeight) SetFullHeight();
}

void CTextView::ReplaceText(Uint32 inOffset, Uint32 inExistingLength, const void *inText, Uint32 inTextLength, bool inUpdateSelect)	
{
	UEditText::ReplaceText(mText, inOffset, inExistingLength, inText, inTextLength, inUpdateSelect);
	if (mSizing & sizing_FullHeight) SetFullHeight();
}

void CTextView::DeleteText(Uint32 inOffset, Uint32 inTextLength, bool inUpdateSelect)
{
	UEditText::DeleteText(mText, inOffset, inTextLength, inUpdateSelect);
	if (mSizing & sizing_FullHeight) SetFullHeight();
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CTextView::Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth)
{
	UEditText::Draw(mText, inImage, inUpdateRect, inDepth);
}

void CTextView::MouseDown(const SMouseMsgData& inInfo)
{
	inherited::MouseDown(inInfo);
	UEditText::MouseDown(mText, inInfo);
}

void CTextView::MouseUp(const SMouseMsgData& inInfo)
{
	inherited::MouseUp(inInfo);
	UEditText::MouseUp(mText, inInfo);
}

void CTextView::MouseEnter(const SMouseMsgData& inInfo)
{
	inherited::MouseEnter(inInfo);
	UEditText::MouseEnter(mText, inInfo);
}

void CTextView::MouseLeave(const SMouseMsgData& inInfo)
{
	inherited::MouseLeave(inInfo);
	UEditText::MouseLeave(mText, inInfo);
}

void CTextView::MouseMove(const SMouseMsgData& inInfo)
{
	inherited::MouseMove(inInfo);
	UEditText::MouseMove(mText, inInfo);
}

bool CTextView::KeyDown(const SKeyMsgData& inInfo)
{
	SRect r;
	
	if (inInfo.keyCode == key_Return && mReturnKeyAction != enterKeyAction_NewLine)
	{
		if (mReturnKeyAction == enterKeyAction_Hit)
		{
			Hit(inInfo.mods & modKey_Option ? hitType_Alternate : hitType_Standard);
			return true;
		}

		return false;
	}
	
	if (inInfo.keyCode == key_nEnter && mEnterKeyAction != enterKeyAction_NewLine)
	{
		if (mEnterKeyAction == enterKeyAction_Hit)
		{
			Hit(inInfo.mods & modKey_Option ? hitType_Alternate : hitType_Standard);
			return true;
		}

		return false;
	}

	bool bRet = UEditText::KeyChar(mText, inInfo);
		
	if (mSizing & sizing_FullHeight)
		SetFullHeight();
		
	if (UEditText::GetCaretRect(mText, r))
		MakeRectVisible(r, align_Inside);
		
	if (!mHasChanged && UEditText::IsEditable(mText))
	{
		mHasChanged = true;
		Hit(hitType_Change);
	}
		
	return bRet;
}

void CTextView::KeyUp(const SKeyMsgData& /* inInfo */)
{
	// nothing to do
}

bool CTextView::KeyRepeat(const SKeyMsgData& inInfo)
{
	return CTextView::KeyDown(inInfo);
}

bool CTextView::SetBounds(const SRect& inBounds)
{
	if (CView::SetBounds(inBounds))
	{
		UEditText::SetBounds(mText, inBounds);
		return true;
	}
	
	return false;
}

void CTextView::UpdateActive()
{
	bool isActive = IsFocus() && mIsEnabled;
	
	UEditText::SetActive(mText, isActive);
	
	if (isActive)
	{
		// text is active, so start caret timer
		if (mCaretTimer == nil && IsEditable())
			mCaretTimer = StartNewTimer(UText::GetCaretTime(), kRepeatingTimer);
	}
	else
	{
		// text is inactive, so stop caret timer
		delete mCaretTimer;
		mCaretTimer = nil;
	}
}

bool CTextView::ChangeState(Uint16 inState)
{
	if (mState != inState)	// if change
	{
		mState = inState;
		UpdateActive();
		return true;
	}
	
	return false;
}

void CTextView::SetTabSelectText(bool inTabSelect)
{
	mTabSelect = inTabSelect;
}

bool CTextView::TabFocusNext()
{
	if(mTabSelect && CView::TabFocusNext())
	{
		SetSelection(0, max_Uint32);
		return true;
	}
	
	SetSelection(0, 0);
	return false;
}

bool CTextView::TabFocusPrev()
{
	if(mTabSelect && CView::TabFocusPrev())
	{
		SetSelection(0, max_Uint32);
		return true;
	}
	
	SetSelection(0, 0);
	return false;
}

bool CTextView::SetEnable(bool inEnable)
{
	if (mIsEnabled != (inEnable != 0))		// if change
	{
		mIsEnabled = (inEnable != 0);
		UpdateActive();
		
		return true;
	}
	
	return false;
}

void CTextView::DragEnter(const SDragMsgData& inInfo)
{
	inherited::DragEnter(inInfo);
	UEditText::DragEnter(mText, inInfo);
}

void CTextView::DragMove(const SDragMsgData& inInfo)
{
	inherited::DragMove(inInfo);
	UEditText::DragMove(mText, inInfo);
}

void CTextView::DragLeave(const SDragMsgData& inInfo)
{
	inherited::DragLeave(inInfo);
	UEditText::DragLeave(mText, inInfo);
}

bool CTextView::Drop(const SDragMsgData& inInfo)
{
	if (UEditText::Drop(mText, inInfo))
	{
		if (mSizing & sizing_FullHeight)
			SetFullHeight();
				
		if (!mHasChanged && UEditText::IsEditable(mText))
		{
			mHasChanged = true;
			Hit(hitType_Change);
		}
		
		return true;
	}
	
	return false;
}

void CTextView::TextDrawHandler(TEditText inRef, const SRect& inRect)
{
	CTextView *obj = (CTextView *)UEditText::GetRef(inRef);
	obj->Refresh(inRect);
}

void CTextView::TextScreenDeltaHandler(TEditText inRef, Int32& outHoriz, Int32& outVert)
{
	CTextView *obj = (CTextView *)UEditText::GetRef(inRef);
	obj->GetScreenDelta(outHoriz, outVert);
}

void CTextView::Timer(TTimer /* inTimer */)
{
	UEditText::BlinkCaret(mText);
}

/*
bool CTextView::CanAcceptDrop(TDrag inDrag) const
{
	return UEditText::CanAcceptDrop(mText, inDrag);
}
*/
bool CTextView::IsSelfDrag(TDrag inDrag) const
{
	return UEditText::IsSelfDrag(mText, inDrag);
}

Uint32 CTextView::GetFullHeight() const
{
	return UEditText::GetFullHeight(mText);
}

void CTextView::SetEnterKeyAction(Uint16 inReturnAction, Uint16 inEnterAction)
{
	mReturnKeyAction = inReturnAction;
	mEnterKeyAction = inEnterAction;
}

Uint16 CTextView::GetReturnKeyAction()
{
	return mReturnKeyAction;
}

Uint16 CTextView::GetEnterKeyAction()
{
	return mEnterKeyAction;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

#include "CScrollerView.h"

// standard height is 26 (one line)
CScrollerView *MakeTextBoxView(CViewHandler *inHandler, const SRect& inBounds, Uint16 inOptions, CTextView **outTextView)
{
	CScrollerView *scr;
	CTextView *txt;
	SRect r;
	
	// create scroller view
	scr = new CScrollerView(inHandler, inBounds, inOptions, 2);
	
	try
	{
		// setup scroller view
		scr->SetCanFocus(true);
		
		// create text view
		r.left = r.top = 0;
		r.right = scr->GetVisibleContentWidth();
		r.bottom = scr->GetVisibleContentHeight();
		txt = new CTextView(scr, r);
		txt->SetCanFocus(true);
		txt->SetSizing(sizing_FullHeight + sizing_RightSticky);
		txt->Show();
	}
	catch(...)
	{
		// clean up
		delete scr;
		throw;
	}
	
	if (outTextView)
		*outTextView = txt;
	return scr;
}



