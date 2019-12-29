/**********************************************************

If the popup doesn't fit entirely on the screen, or would
be huge, CPopupView should put it inside a CScrollerView.
So it's a popup with a scrollbar - like windoze.
**** better way following

*********************************************

CAbstractPopupView
	- doesn't draw popup btn or anything (that's left to derived)
	- calls virtual funcs to draw popup btn, popup contents etc
	- also supports scroll bars if needed! (if doesn't fit on screen)

CPopupView : CAbstractPopupView
	- pops up any view, dunno if this is needed

CPopupMenuView : CAbstractPopupView
	- uses UPopupButton and UMenu

**********************************************************/

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CPopupView.h"


class _PUCPopupWin : public CWindow
{
	public:
		
		_PUCPopupWin(CPopupView *inPopupHandler);
		
		void RecalcBounds();
		
		virtual void MouseUp(const SMouseMsgData& inInfo);
		
		void DoShow()										{	mReleaseOnMouseUp = false; UWindow::SetMouseCaptureWindow(mWindow); Show();		}
		void DoHide()										{	UWindow::ReleaseMouseCaptureWindow(mWindow);	Hide();							}
		
	protected:
		CPopupView *mPopupHandler;
		Uint8 mReleaseOnMouseUp : 1;
		
		virtual void HandleHit(CView *inView, const SHitMsgData& inInfo);
		
};

_PUCPopupWin::_PUCPopupWin(CPopupView *inPopupHandler)
	: CWindow(SRect(0, 0, 0, 0), windowLayer_Popup)
{
	mPopupHandler = inPopupHandler;

}

void _PUCPopupWin::MouseUp(const SMouseMsgData& inInfo)
{
	SRect r;
	GetBounds(r);
	r.MoveTo(0, 0);
	
	if (mReleaseOnMouseUp || r.Contains(inInfo.loc))
	{
		CWindow::MouseUp(inInfo);
		HandleReleaseMouse(mView);
		mPopupHandler->HidePopupWin();
	}
	else
	{
		mReleaseOnMouseUp = true;
		CWindow::MouseUp(inInfo);
	}
}

void _PUCPopupWin::RecalcBounds()
{
	if (mView)
	{
		SetBounds(SRect(0, 0, mView->GetFullWidth(), mView->GetFullHeight()));
	}
}

void _PUCPopupWin::HandleHit(CView *inView, const SHitMsgData& inInfo)
{
	if (mPopupHandler && mPopupHandler->mHandler)
		mPopupHandler->mHandler->HandleHit(inView, inInfo);
}

/* -------------------------------------------------------------------------- */

CPopupView::CPopupView(CViewHandler *inHandler, const SRect& inBounds)
	: CView(inHandler, inBounds)
{
	/*
	mWindow = UWindow::New(SRect(0,0,100,100), windowLayer_Popup);
	UWindow::SetLimits(mWindow, 16, 16);
	UWindow::SetMessageHandler(mWindow, CPopupView::PopupMessageHandler, this);
	*/

	mIsPopupOnRight = false;
	mWindow = new _PUCPopupWin(this);
}

CPopupView::~CPopupView()
{
	delete mWindow;
}

void CPopupView::SetView(CView *inView)
{
	mWindow->SetView(inView);
}


CView *CPopupView::DetachView()
{
	return mWindow->DetachView();
}

void CPopupView::GetPopupButtonRect(SRect& outRect)
{
	outRect = mBounds;
}

void CPopupView::ShowPopupWin()
{
	SRect btnRect;
	SRect r;
	Int32 h, v;
	//Uint32 height, width;

	GetPopupButtonRect(btnRect);
	GetScreenDelta(h, v);
	
	if (mIsPopupOnRight)
	{
		r.left = btnRect.right + 1 + h;
		r.top = btnRect.top + 1 + v;
	}
	else	// display below rect
	{
		r.left = btnRect.left + 1 + h;
		r.top = btnRect.bottom + 1 + v;
#if WIN32
		// account for the bigger border on windoze
		r.left++;
		r.top += 2;
#endif
	}
	
	((_PUCPopupWin *)mWindow)->RecalcBounds();
	SRect bounds;
	mWindow->GetBounds(bounds);
	
	/*
	if (mView)
		mView->GetFullSize(width, height);
	else
		height = width = 100;
	*/
	
	r.bottom = r.top + bounds.GetHeight();
	r.right = r.left + bounds.GetWidth();
	
	mWindow->SetBounds(r);
//	UWindow::SetBounds(mWindow, r);
	if (mWindow->GetAutoBounds(windowPos_Best, windowPosOn_WinScreen, r))
		mWindow->SetBounds(r);
	
/*	if (mView)
	{
		r.MoveTo(0,0);
		mView->SetBounds(r);
	}
	
	UWindow::SetVisible(mWindow, true);
	//UWindow::SetFocus(mWindow);
	
	if (mView)
	{
		mView->ChangeState(UWindow::GetState(mWindow));
	}
	*/
	
	//mWindow->Show();
	((_PUCPopupWin *)mWindow)->DoShow();
}

void CPopupView::HidePopupWin()
{
	((_PUCPopupWin *)mWindow)->DoHide();
	//if (mView) mView->ChangeState(viewState_Hidden);
}

#if 0
void CPopupView::HandlePopupMessage(Uint32 inMsg, const void *inData, Uint32 /* inDataSize */)
{
	if (mView == nil) return;
	
	switch (inMsg)
	{
		case msg_Draw:
		{
			TImage image;
			SRect r;
			while (UWindow::GetRedrawRect(mWindow, r))
			{
				image = UWindow::BeginDraw(mWindow, r);
				try
				{
					mView->Draw(image, r, UWindow::GetDepth(mWindow));
				}
				catch(...)
				{
					UWindow::AbortDraw(mWindow);
					throw;
				}
				UWindow::EndDraw(mWindow);
			}
		}
		break;
		
		case msg_HidePopupWindow:
			HidePopupWin();
			break;
		
		case msg_KeyDown:
			mView->KeyDown(*(SKeyMsgData *)inData);
			break;
		case msg_KeyUp:
			mView->KeyUp(*(SKeyMsgData *)inData);
			break;
		case msg_MouseDown:
			SMouseMsgData *mouseData = (SMouseMsgData *)inData;
			if (mView->IsPointWithin(mouseData->loc))
				mView->MouseDown(*mouseData);
			else
			{
				UWindow::ReleaseMouseCaptureWindow(mWindow);
				HidePopupWin();
			}
				
			break;
		case msg_MouseUp:
			mView->MouseUp(*(SMouseMsgData *)inData);
			break;
		case msg_MouseEnter:
			mView->MouseEnter(*(SMouseMsgData *)inData);
			break;
		case msg_MouseLeave:
			mView->MouseLeave(*(SMouseMsgData *)inData);
			break;
		case msg_MouseMove:
			mView->MouseMove(*(SMouseMsgData *)inData);
			break;
		case msg_DragEnter:
			mView->DragEnter(*(SDragMsgData *)inData);
			break;
		case msg_DragMove:
			mView->DragMove(*(SDragMsgData *)inData);
			break;
		case msg_DragLeave:
			mView->DragLeave(*(SDragMsgData *)inData);
			break;
		case msg_DragDropped:
			mView->Drop(*(SDragMsgData *)inData);
			break;
		//case msg_Hit:
		//	Hit(*(SHitMsgData *)inData);
		//	break;
	}
}

void CPopupView::PopupMessageHandler(void *inContext, void */* inObject */, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	((CPopupView *)inContext)->HandlePopupMessage(inMsg, inData, inDataSize);
}

void CPopupView::HandleRefresh(CView */* inView */, const SRect& inUpdateRect)
{
	UWindow::Refresh(mWindow, inUpdateRect);
}

void CPopupView::HandleHit(CView *inView, const SHitMsgData& inInfo)
{
	HidePopupWin();
	if (mHandler) mHandler->HandleHit(inView, inInfo);
}
#endif

void CPopupView::HandleGetScreenDelta(CView */* inView */, Int32& outHoriz, Int32& outVert)
{
	SRect stBounds;
	mWindow->GetBounds(stBounds);
	
	outHoriz = stBounds.left;
	outVert = stBounds.top;
}

void CPopupView::HandleGetVisibleRect(CView */* inView */, SRect& outRect)
{
	SRect stBounds;
	mWindow->GetBounds(stBounds);

	outRect.left = outRect.top = 0;
	outRect.right = stBounds.GetWidth();
	outRect.bottom = stBounds.GetHeight();
}

