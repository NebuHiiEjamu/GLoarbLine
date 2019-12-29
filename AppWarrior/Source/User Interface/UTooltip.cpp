#include "UTooltip.h"
#include "CWindow.h"
#include "CView.h"
#include "UGraphics.h"


enum
{
	ToolTip_Delay			= 1000,		// 1 second
	ToolTip_TimeoutDelay	= 1000
	// should there also be a timeout for when user lingers on a button?  if after 3 seconds, mouse is still on btn, hide tooltip?
};

void _GetScreenBounds(SRect& outScreenBounds);

class _TTCToolTipWin;

_TTCToolTipWin *_TTToolTipWin = nil;
Uint16 _TTTooltipState = kTooltipState_Inactive;
TTimer _TTToolTipTimer = nil;
TTimer _TTToolTipTimeoutTimer = nil;
TWindow _TTWin = nil;


/* -------------------------------------------------------------------------- */
#pragma mark _TTCToolTipView

class _TTCToolTipView : public CView
{
	public:
		_TTCToolTipView(CViewHandler *inHandler, const SRect& inBounds, const SColor &inTextColor);
		~_TTCToolTipView();
		
		void SetText(const Uint8 inText[])										{	pstrcpy(mText, inText);						}
		Uint8 *GetTextPtr()														{	return mText;								}

		void SetFont(TFontDesc inFont);
		void SetFont(const Uint8 *inName, const Uint8 *inStyle, Uint32 inSize, Uint32 inEffect = 0);
		TFontDesc GetFont() const												{	return mFontDesc;							}
		void SetFontSize(Uint32 inSize)											{	mFontDesc->SetFontSize(inSize);				}
		Uint32 GetFontSize() const												{	return mFontDesc->GetFontSize();			}

		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);

	protected:
		TFontDesc mFontDesc;
		Uint8 mText[256];
};

_TTCToolTipView::_TTCToolTipView(CViewHandler *inHandler, const SRect& inBounds, const SColor &inTextColor)
	: CView(inHandler, inBounds)
{
	mFontDesc = UFontDesc::New(kDefaultFont, nil, 9);
	mFontDesc->SetColor(inTextColor);
}

_TTCToolTipView::~_TTCToolTipView()
{
	delete mFontDesc;
}

void _TTCToolTipView::Draw(TImage inImage, const SRect &/*& inUpdateRect*/, Uint32 /*inDepth*/)
{
	inImage->SetFont(mFontDesc);
	inImage->DrawText(mBounds, mText + 1, mText[0], nil, align_Center);
//	inImage->DrawTextBox(mBounds, inUpdateRect, mText, mFontDesc->GetFontSize(), nil, align_Center);
}

/* -------------------------------------------------------------------------- */
#pragma mark -
#pragma mark _TTCToolTipWin

class _TTCToolTipWin : public CWindow
{
	public:
		_TTCToolTipWin();
		
		void SetText(const Uint8 inText[]);
		
		
		void SetFont(TFontDesc inFont);
		void SetFont(const Uint8 *inName, const Uint8 *inStyle, Uint32 inSize, Uint32 inEffect = 0);
		TFontDesc GetFont() const												{	return mTTView->GetFont();					}
		void SetFontSize(Uint32 inSize);
		Uint32 GetFontSize() const												{	return mTTView->GetFontSize();				}
		
		void SetRefRect(const SRect& inRect)									{	mRefRect = inRect;							}
		void RecalcDims();
		
	protected:
		_TTCToolTipView *mTTView;
		
		SRect mRefRect;
		
};

_TTCToolTipWin::_TTCToolTipWin()
	: CWindow(SRect(0, 100, 0, 100), windowLayer_Popup, 0, 1)
{
	// fool requ'd using system hilite color for back color - not a bad plan.
	//SColor backColor, invColor;
	//UUserInterface::GetHighlightColor(&backColor, &invColor);
	//SetBackColor(backColor);
	_TTWin = mWindow;
	
	SetBackColor(color_cadmium_lemon);
	
	mTTView = new _TTCToolTipView(this, SRect(0, 100, 0, 100), color_Black);
	mTTView->SetSizing(sizing_BottomRightSticky);
	mTTView->Show();	
}

void _TTCToolTipWin::SetFont(TFontDesc inFont)
{
	mTTView->SetFont(inFont);
	
	// recalc dimensions and refresh
	RecalcDims();
}

void _TTCToolTipWin::SetText(const Uint8 inText[])
{
	mTTView->SetText(inText);
	
	RecalcDims();
}

void _TTCToolTipWin::SetFontSize(Uint32 inSize)
{
	mTTView->SetFontSize(inSize);
	RecalcDims();
}

void _TTCToolTipWin::RecalcDims()
{	
	SRect stScreenBounds;
	_GetScreenBounds(stScreenBounds);

	TImage dummyImg = UGraphics::GetDummyImage();		
	dummyImg->SetFont(mTTView->GetFont());
	Uint8 *txtPtr = mTTView->GetTextPtr();
	
	SRect stBounds;
	stBounds.SetEmpty();
	
	// horiz bounds
	stBounds.right = dummyImg->GetTextWidth(txtPtr + 1, txtPtr[0]) + 7;	
	stBounds.CenterHoriz(mRefRect);
	
	if (stBounds.right > stScreenBounds.right)
		stBounds.MoveBack(stBounds.right - stScreenBounds.right, 0);

	if (stBounds.left < stScreenBounds.left)
		stBounds.Move(stScreenBounds.left - stBounds.left, 0);

	// vert bounds
	stBounds.top = mRefRect.bottom + 3;
	stBounds.bottom = stBounds.top + dummyImg->GetFontHeight() + 3;
	
	if (stBounds.bottom > stScreenBounds.bottom)
	{
		stBounds.bottom = mRefRect.top - 3;
		stBounds.top = stBounds.bottom - dummyImg->GetFontHeight() - 3;
	}
	
	// set bounds
	SetBounds(stBounds);
	Refresh();
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void _TTTimerHandler(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	#pragma unused(inContext, inMsg, inData, inDataSize)
	
	TTimer timer = (TTimer)inObject;
	
	if (timer == _TTToolTipTimer)
	{
		_TTToolTipWin->Show();
		_TTTooltipState = kTooltipState_ActiveVisible;
	}
	else if (timer == _TTToolTipTimeoutTimer)
	{
		_TTToolTipWin->Hide();
		_TTTooltipState = kTooltipState_Inactive;
	}
}

void _TTTimerStop()
{
	if (_TTToolTipTimer->WasStarted())
		_TTToolTipTimer->Stop();

	if (_TTToolTipTimeoutTimer->WasStarted())
		_TTToolTipTimeoutTimer->Stop();

	UApplication::FlushMessages(_TTTimerHandler, nil);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UTooltip::Init()
{
	if (!_TTToolTipWin)
	{
		_TTToolTipWin = new _TTCToolTipWin();
		
		_TTToolTipTimer = UTimer::New(_TTTimerHandler, nil);
		_TTToolTipTimeoutTimer = UTimer::New(_TTTimerHandler, nil);
	}
}

void UTooltip::Activate(CView *inRefView, const Uint8 *inText)
{
	if (!_TTToolTipWin || _TTTooltipState == kTooltipState_Disabled || !inRefView || !inText || !inText[0])
		return;
		
	Int32 dx, dy;
	inRefView->GetScreenDelta(dx, dy);
	
	SRect stBounds;
	inRefView->GetVisibleRect(stBounds);
	stBounds.Move(dx, dy);
	
	_TTToolTipWin->SetRefRect(stBounds);
	_TTToolTipWin->SetText(inText);

	_TTTimerStop();

	if (_TTTooltipState == kTooltipState_ActiveVisible || _TTTooltipState == kTooltipState_ActiveHidden)
	{
		_TTToolTipWin->Show();
		_TTTooltipState = kTooltipState_ActiveVisible;
	}
	else
	{			
		_TTTooltipState = kTooltipState_Activating;
		_TTToolTipTimer->Start(ToolTip_Delay);
	}
}

void UTooltip::Hide()
{
	if (!_TTToolTipWin)
		return;
	
	_TTTimerStop();
	_TTToolTipWin->Hide();

	if (_TTTooltipState == kTooltipState_ActiveVisible)
	{
		_TTTooltipState = kTooltipState_ActiveHidden;
		_TTToolTipTimeoutTimer->Start(ToolTip_TimeoutDelay);
	}
	else if (_TTTooltipState == kTooltipState_Activating)
		_TTTooltipState = kTooltipState_Inactive;
}

Uint16 UTooltip::GetState()
{
	return _TTTooltipState;
}

void UTooltip::SetEnable(bool inEnable)
{
	if (inEnable)
	{
		if (_TTTooltipState == kTooltipState_Disabled)
			_TTTooltipState = kTooltipState_Inactive;
	}
	else
	{
		if (_TTTooltipState == kTooltipState_ActiveVisible)
			Hide();
		
		_TTTooltipState = kTooltipState_Disabled;
	}
}

bool UTooltip::IsEnabled()
{
	return _TTTooltipState != kTooltipState_Disabled;
}

