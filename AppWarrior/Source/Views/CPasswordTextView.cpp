/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CPasswordTextView.h"

#if MACINTOSH
const Uint8 kPasswordChar = 0xA5;		// bullet '�'
#else
const Uint8 kPasswordChar = '*';
#endif

/* -------------------------------------------------------------------------- */

CPasswordTextView::CPasswordTextView(CViewHandler *inHandler, const SRect& inBounds)
	: CView(inHandler, inBounds)
{
	mPassword = nil;
	mEditText = nil;
	mCaretTimer = nil;
	mIsDummy = false;
	
	mReturnKeyAction = enterKeyAction_None;
	mEnterKeyAction = enterKeyAction_None;
	
	try
	{
		mPassword = UMemory::NewHandle();
		mEditText = UEditText::New(inBounds);
		UEditText::SetActive(mEditText, false);
		UEditText::SetRef(mEditText, this);
		UEditText::SetDrawHandler(mEditText, TextDrawHandler);
	}
	catch(...)
	{
		UEditText::Dispose(mEditText);
		UMemory::Dispose(mPassword);
		throw;
	}
}

CPasswordTextView::~CPasswordTextView()
{
	delete mCaretTimer;
	UMemory::Dispose(mPassword);
	UEditText::Dispose(mEditText);
}

void CPasswordTextView::SetText(const void *inText, Uint32 inLength)
{
	Uint8 buf[256];
	Uint8 *p, *ep;
	
	if (inLength > sizeof(buf))
		inLength = sizeof(buf);
	
	UMemory::Set(mPassword, inText, inLength);
	
	p = buf;
	ep = buf + inLength;
	while (p != ep)
		*p++ = kPasswordChar;
	
	UEditText::SetText(mEditText, buf, inLength);
}

bool CPasswordTextView::IsEmpty() const
{
	return (UMemory::GetSize(mPassword) == 0);
}

void CPasswordTextView::SetDummyPassword()
{
	Uint8 buf[256];
	Uint8 *p, *ep;
	Uint32 n;
	
	// determine char count to fill entire box
	TImage img = UGraphics::GetDummyImage();
	img->SetFont(UEditText::GetFont(mEditText));
	n = mBounds.GetWidth() / UGraphics::GetCharWidth(img, kPasswordChar);
	
	if (n)
	{
		if (n > sizeof(buf)) n = sizeof(buf);
		
		p = buf;
		ep = buf + n;
		while (p != ep) *p++ = 'x';

		SetText(buf, n);
	}
	
	mIsDummy = true;
}

void CPasswordTextView::SetEnterKeyAction(Uint16 inReturnAction, Uint16 inEnterAction)
{
	mReturnKeyAction = inReturnAction;
	mEnterKeyAction = inEnterAction;
}

Uint16 CPasswordTextView::GetReturnKeyAction()
{
	return mReturnKeyAction;
}

Uint16 CPasswordTextView::GetEnterKeyAction()
{
	return mEnterKeyAction;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

inline bool IsNavigationKey(Uint16 inKeyCode)
{
	return inKeyCode >= key_Home && inKeyCode <= key_Right;
}

void CPasswordTextView::Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth)
{
	UEditText::Draw(mEditText, inImage, inUpdateRect, inDepth);
}

void CPasswordTextView::MouseDown(const SMouseMsgData& inInfo)
{
	inherited::MouseDown(inInfo);
	if (!mIsDummy) UEditText::MouseDown(mEditText, inInfo);
}

void CPasswordTextView::MouseUp(const SMouseMsgData& inInfo)
{
	inherited::MouseUp(inInfo);
	if (!mIsDummy) UEditText::MouseUp(mEditText, inInfo);
}

void CPasswordTextView::MouseEnter(const SMouseMsgData& inInfo)
{
	inherited::MouseEnter(inInfo);
	if (!mIsDummy) UEditText::MouseEnter(mEditText, inInfo);
}

void CPasswordTextView::MouseLeave(const SMouseMsgData& inInfo)
{
	inherited::MouseLeave(inInfo);
	if (!mIsDummy) UEditText::MouseLeave(mEditText, inInfo);
}

void CPasswordTextView::MouseMove(const SMouseMsgData& inInfo)
{
	inherited::MouseMove(inInfo);
	if (!mIsDummy) UEditText::MouseMove(mEditText, inInfo);
}

bool CPasswordTextView::KeyDown(const SKeyMsgData& inInfo)
{
	SKeyMsgData info = inInfo;
	
	if (mIsDummy && IsNavigationKey(info.keyCode)) 
		return false;
	
	if (inInfo.keyCode == key_Return)
	{
		if (mReturnKeyAction == enterKeyAction_Hit)
		{
			Hit(inInfo.mods & modKey_Option ? hitType_Alternate : hitType_Standard);
			return true;
		}

		return false;
	}
	
	if (inInfo.keyCode == key_nEnter)
	{
		if (mEnterKeyAction == enterKeyAction_Hit)
		{
			Hit(inInfo.mods & modKey_Option ? hitType_Alternate : hitType_Standard);
			return true;
		}

		return false;
	}

	bool bRet = false;

	if ((inInfo.mods & (modKey_Command|modKey_Control)) && inInfo.keyChar == 'v')
	{
		bRet = true;

		THdl h = UClipboard::GetData("text/plain");

		if (h)
		{
			void *p = UMemory::Lock(h);
			try
			{
				SetText(p, UMemory::GetSize(h));
			}
			catch(...)
			{
				UMemory::Unlock(h);
				UMemory::Dispose(h);
				throw;
			}
			UMemory::Unlock(h);
			UMemory::Dispose(h);
		}
	}
	else if (info.keyCode == key_Delete || info.keyCode == key_nClear || info.keyCode == key_Del)
	{
		bRet = true;

		UMemory::SetSize(mPassword, 0);
		UEditText::SetText(mEditText, nil, 0);
	}
	else if (UText::IsPrinting(info.keyChar))
	{
		Uint32 selectStart, selectEnd;
		Uint8 c = info.keyChar;
		
		UEditText::GetSelect(mEditText, selectStart, selectEnd);
		
		UMemory::Replace(mPassword, selectStart, selectEnd - selectStart, &c, sizeof(c));
		
		info.keyChar = kPasswordChar;
		bRet = UEditText::KeyChar(mEditText, info);
	}
	else
	{
		bRet = UEditText::KeyChar(mEditText, info);
	}

	if (!mHasChanged)
	{
		mHasChanged = true;
		Hit(hitType_Change);
	}
	
	mIsDummy = false;
	return bRet;
}

void CPasswordTextView::KeyUp(const SKeyMsgData& /* inInfo */)
{
	// nothing to do
}

bool CPasswordTextView::KeyRepeat(const SKeyMsgData& inInfo)
{
	if (mIsDummy && IsNavigationKey(inInfo.keyCode)) 
		return false;

	return CPasswordTextView::KeyDown(inInfo);
}

bool CPasswordTextView::SetBounds(const SRect& inBounds)
{
	if (CView::SetBounds(inBounds))
	{
		UEditText::SetBounds(mEditText, inBounds);
		return true;
	}
	
	return false;
}

void CPasswordTextView::UpdateActive()
{
	bool isActive = IsFocus() && mIsEnabled;
	
	UEditText::SetActive(mEditText, isActive);
	
	if (isActive)
	{
		// text is active, so start caret timer
		if (mCaretTimer == nil)
			mCaretTimer = StartNewTimer(UText::GetCaretTime(), kRepeatingTimer);
	}
	else
	{
		// text is inactive, so stop caret timer
		delete mCaretTimer;
		mCaretTimer = nil;
	}
}

bool CPasswordTextView::ChangeState(Uint16 inState)
{
	if (mState != inState)	// if change
	{
		mState = inState;
		
		if (inState == viewState_Focus && mIsDummy)
			UEditText::SetSelect(mEditText, 0, max_Uint32);

		UpdateActive();
		return true;
	}
	
	return false;
}

bool CPasswordTextView::TabFocusNext()
{
	if(CView::TabFocusNext())
	{
		if(mEditText)
			UEditText::SetSelect(mEditText, 0, max_Uint32);
		
		return true;
	}

	if(mEditText)
		UEditText::SetSelect(mEditText, 0, 0);
	
	return false;
}


bool CPasswordTextView::TabFocusPrev()
{
	if(CView::TabFocusPrev())
	{
		if(mEditText)
			UEditText::SetSelect(mEditText, 0, max_Uint32);
		
		return true;
	}

	if(mEditText)
		UEditText::SetSelect(mEditText, 0, 0);
	
	return false;
}


bool CPasswordTextView::SetEnable(bool inEnable)
{
	if (mIsEnabled != (inEnable != 0))		// if change
	{
		mIsEnabled = (inEnable != 0);
		UpdateActive();
		
		return true;
	}
	
	return false;
}

void CPasswordTextView::TextDrawHandler(TEditText inRef, const SRect& inRect)
{
	CPasswordTextView *obj = (CPasswordTextView *)UEditText::GetRef(inRef);
	obj->Refresh(inRect);
}

void CPasswordTextView::Timer(TTimer /* inTimer */)
{
	UEditText::BlinkCaret(mEditText);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

CScrollerView *MakePasswordBoxView(CViewHandler *inHandler, const SRect& inBounds, Uint16 inOptions, CPasswordTextView **outTextView)
{
	CScrollerView *scr;
	CPasswordTextView *txt;
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
		txt = new CPasswordTextView(scr, r);
		txt->SetCanFocus(true);
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






