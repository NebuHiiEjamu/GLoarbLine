/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CWindow.h"
#include "CView.h"

CWindowList gWindowList;
CSnapWindows gSnapWindows;

// begin structures that must be cross-platform
#pragma options align=packed

typedef struct {
	SRect bounds;
	Int16 style;
	Int16 layer;
	Uint16 options;
	Int16 viewResID;
	Int32 minWidth, minHeight, maxWidth, maxHeight;
	Int16 autoPos, autoPosOn;
	SColor backColor;
	Int16 type;
	Uint32 rsvd[6];
	Int16 script;
	Uint16 titleLen;
	Uint8 titleData[];
} SWindowObj;

// end structures that must be cross-platform
#pragma options align=reset


/* -------------------------------------------------------------------------- */

CWindow::CWindow(const SRect& inBounds, Uint16 inLayer, Uint16 inOptions, Uint16 inStyle, CWindow *inParent)
{
	mWindow = nil;
	mType = 0;

	mIsModal = false;
	mIsMouseCapturedView = false;
	
	try
	{
		if (inParent)
			mWindow = UWindow::New(inBounds, inLayer, inOptions, inStyle, *inParent);
		else
			mWindow = UWindow::New(inBounds, inLayer, inOptions, inStyle);

		UWindow::SetRef(mWindow, this);
		UWindow::SetMessageHandler(mWindow, MessageHandler, this);
		
		gWindowList.AddWindow(this);
	}
	catch(...)
	{
		UWindow::Dispose(mWindow);
		throw;
	}
}

CWindow::~CWindow()
{
	UApplication::FlushMessages(CApplication::WindowHitHandler, this);
	
	gWindowList.RemoveWindow(this);
	gSnapWindows.RemoveWindow(this);
	
	// kill hit queue
	CLink *hitp = mHitQueue.GetFirst();
	CLink *nextp;
	while (hitp)
	{
		nextp = hitp->GetNext();
		UMemory::Dispose((TPtr)hitp);
		hitp = nextp;
	}
	
	// kill window
	UWindow::Dispose(mWindow);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CWindow::Draw(const SRect& inUpdateRect)
{
	if (mView && UWindow::IsVisible(mWindow))
	{
		// if we have just a QuickTime view there is no need to draw the background
		if (!dynamic_cast<CQuickTimeView *>(mView))
		{
			TImage pImage = UWindow::BeginDraw(mWindow, inUpdateRect);

			try
			{
				mView->Draw(pImage, inUpdateRect, UWindow::GetDepth(mWindow));
			}
			catch(...)
			{
				UWindow::AbortDraw(mWindow);
				throw;
			}
		
			UWindow::EndDraw(mWindow);		
		}
		
		// we must to update QuickTime after we draw the background
		mView->UpdateQuickTime();
	}
}

void CWindow::MouseDown(const SMouseMsgData& inInfo)
{
	if (mView) mView->MouseDown(inInfo);
}

void CWindow::MouseUp(const SMouseMsgData& inInfo)
{
	if (mView) mView->MouseUp(inInfo);
}

void CWindow::MouseEnter(const SMouseMsgData& inInfo)
{	
	UMouse::SetImage(mouseImage_Standard);
	if (mView) mView->MouseEnter(inInfo);
}

void CWindow::MouseMove(const SMouseMsgData& inInfo)
{
	if (mView) mView->MouseMove(inInfo);
}

void CWindow::MouseLeave(const SMouseMsgData& inInfo)
{
	if (mView) mView->MouseLeave(inInfo);
}

void CWindow::KeyDown(const SKeyMsgData& inInfo)
{
	if (mView) mView->KeyDown(inInfo);
}

void CWindow::KeyUp(const SKeyMsgData& inInfo)
{
	if (mView) mView->KeyUp(inInfo);
}

void CWindow::KeyRepeat(const SKeyMsgData& inInfo)
{
	if (mView) mView->KeyRepeat(inInfo);
}

/*
bool CWindow::CanAcceptDrop(TDrag inDrag) const
{
	if (mView) return mView->CanAcceptDrop(inDrag);
	return false;
}
*/
void CWindow::DragEnter(const SDragMsgData& inInfo)
{
	if (mView)
	{
//		mDragWithinViewCanAccept = (mView->CanAcceptDrop(inInfo.drag) != 0);
//		if (mDragWithinViewCanAccept)
			mView->DragEnter(inInfo);
	}
}

void CWindow::DragMove(const SDragMsgData& inInfo)
{
	if (mView)// && mDragWithinViewCanAccept)
		mView->DragMove(inInfo);
}

void CWindow::DragLeave(const SDragMsgData& inInfo)
{
	if (mView)// && mDragWithinViewCanAccept)
		mView->DragLeave(inInfo);
}

bool CWindow::Drop(const SDragMsgData& inInfo)
{
	if (mView)// && mDragWithinViewCanAccept && mView->Drop(inInfo))
	{
		//UDragAndDrop::AcceptDrop(inInfo.drag);
		if (mView->Drop(inInfo))
		{
			inInfo.drag->AcceptDrop();
			return true;
		}
	}
	
	UDragAndDrop::RejectDrop(inInfo.drag);
	return false;
}

void CWindow::SendToQuickTime(const EventRecord& inInfo)
{
	if (!IsVisible() || IsCollapsed())
		return;

	if (mView && mView->IsVisible()) mView->SendToQuickTime(inInfo);
}

void CWindow::UserClose(const SMouseMsgData& /* inInfo */)
{
	SHitMsgData info;
	
	info.view = nil;
	info.id = info.cmd = ClassID;
	info.type = hitType_WindowCloseBox;
	info.part = 1;
	info.param = 0;
	info.dataSize = 0;
	
	Hit(info);
}

void CWindow::UserZoom(const SMouseMsgData& /* inInfo */)
{
	SRect ideal, r;
	
	ideal.top = ideal.left = 0;
	ideal.bottom = ideal.right = 5000;
	
	if (UWindow::GetZoomBounds(mWindow, ideal, r))
		SetBounds(r);
}

void CWindow::BoundsChanged()
{
	if (mView)
	{
		//#error "sizing should be BottomRightSticky"		
		//#error "for BottomRightSticky, we need the old width/height - how do we get it?? will need to save"
		// see the #error about this further down
		
		SRect r;
		UWindow::GetBounds(mWindow, r);
		r.right = r.GetWidth();
		r.bottom = r.GetHeight();
		r.top = r.left = 0;
		mView->SetBounds(r);
	}
}

void CWindow::StateChanged()
{
	if (mView) 
		mView->ChangeState(GetState());
}

void CWindow::Hit(const SHitMsgData& inInfo)
{
	if (mIsModal)
	{
		Uint8 *p = (Uint8 *)UMemory::New(sizeof(CLink) + sizeof(SHitMsgData) + inInfo.dataSize);
		UMemory::Copy(p + sizeof(CLink), &inInfo, sizeof(SHitMsgData) + inInfo.dataSize);
		mHitQueue.AddLast((CLink *)p);
	}
	else
		UApplication::PostMessage(msg_WindowHit, &inInfo, sizeof(SHitMsgData) + inInfo.dataSize, priority_High, CApplication::WindowHitHandler, this);
}

// does not return until there is a hit for this window
void CWindow::ProcessModal()
{
	mIsModal = true;
	
	for(;;)
	{
		if (mHitQueue.IsNotEmpty()) return;
		
		// processes all messages and returns straight away
		UApplication::Process();
		
		if (mHitQueue.IsNotEmpty()) return;
		
		// no more messages so we can sleep
		UApplication::ProcessAndSleep();
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CWindow::TimerHandler(void *inContext, void *inObject, Uint32 /* inMsg */, const void */* inData */, Uint32 /* inDataSize */)
{
	((CWindow *)inContext)->Timer((TTimer)inObject);
}

void CWindow::Timer(TTimer /* inTimer */)
{
	// by default, do nothing if a timer went off
}

/* -------------------------------------------------------------------------- */
#pragma mark -

Uint32 CWindow::mLastKeyMouseEventSecs = 0;

void CWindow::MessageHandler(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	((CWindow *)inContext)->HandleMessage(inObject, inMsg, inData, inDataSize);
}

void CWindow::HandleMessage(void */* inObject */, Uint32 inMsg, const void *inData, Uint32 /* inDataSize */)
{
	switch (inMsg)
	{
		case msg_Draw:
		{
			// we set the state here to avoid drawing the window in a no-longer-current state.  ChangeState() does nothing if it is already in the specified state
			if (mView && !dynamic_cast<CQuickTimeView *>(mView))
				mView->ChangeState(GetState());
			
			SRect stUpdateRect;
			while (UWindow::GetRedrawRect(mWindow, stUpdateRect))
				Draw(stUpdateRect);
		}
		break;
		
		case msg_KeyDown:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			KeyDown(*(SKeyMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SKeyMsgData)});
			break;
		case msg_KeyUp:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			KeyUp(*(SKeyMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SKeyMsgData)});
			break;
		case msg_KeyRepeat:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			KeyRepeat(*(SKeyMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SKeyMsgData)});
			break;
		case msg_MouseDown:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			MouseDown(*(SMouseMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SMouseMsgData)});
			break;
		case msg_MouseUp:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			MouseUp(*(SMouseMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SMouseMsgData)});
			break;
		case msg_MouseEnter:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			MouseEnter(*(SMouseMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SMouseMsgData)});
			break;
		case msg_MouseLeave:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			MouseLeave(*(SMouseMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SMouseMsgData)});
			break;
		case msg_MouseMove:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			MouseMove(*(SMouseMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SMouseMsgData)});
			break;
		case msg_DragEnter:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			DragEnter(*(SDragMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SDragMsgData)});
			break;
		case msg_DragMove:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			DragMove(*(SDragMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SDragMsgData)});
			break;
		case msg_DragLeave:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			DragLeave(*(SDragMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SDragMsgData)});
			break;
		case msg_DragDropped:
			mLastKeyMouseEventSecs = UDateTime::GetSeconds();
			Drop(*(SDragMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SDragMsgData)});
			break;
		case msg_UserClose:
			UserClose(*(SMouseMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SMouseMsgData)});
			break;
		case msg_UserZoom:
			UserZoom(*(SMouseMsgData *)inData);
			HLRand::Churn((DataBuffer){(UInt8*)inData,sizeof(SMouseMsgData)});
			break;
		case msg_BoundsChanged:
			BoundsChanged();
			break;
		case msg_StateChanged:
			StateChanged();
			break;
		case msg_CaptureReleased:
			MouseCaptureReleased();
			break;
		case msg_SendToQuickTime:
			SendToQuickTime(*(EventRecord *)inData);
			break;
		case msg_Hit:
			Hit(*(SHitMsgData *)inData);
			break;
	}
}

bool CWindow::GetHit(SHitMsgData& outInfo, Uint32 inMaxDataSize)
{
	TPtr p;
	SHitMsgData *hit;
	Uint32 s;
	
	p = (TPtr)mHitQueue.RemoveFirst();
	if (p == nil) return false;
	hit = (SHitMsgData *)(BPTR(p) + sizeof(CLink));
	
	s = hit->dataSize;
	if (s > inMaxDataSize) s = inMaxDataSize;
	
	UMemory::Copy(&outInfo, hit, sizeof(SHitMsgData) + s);
	outInfo.dataSize = s;
	
	UMemory::Dispose(p);
	return true;
}

Uint32 CWindow::GetHitID()
{
	TPtr p = (TPtr)mHitQueue.RemoveFirst();
	if (p == nil) return 0;
	SHitMsgData *hit = (SHitMsgData *)(BPTR(p) + sizeof(CLink));
	
	Uint32 id = hit->id;
	UMemory::Dispose(p);
	
	return id;
}

Uint32 CWindow::GetHitCommandID()
{
	TPtr p = (TPtr)mHitQueue.RemoveFirst();
	if (p == nil) return 0;
	SHitMsgData *hit = (SHitMsgData *)(BPTR(p) + sizeof(CLink));
	
	Uint32 id = hit->cmd;
	UMemory::Dispose(p);
	
	return id;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CWindow::IsChild(CWindow *inWindow)
{	
	if (!inWindow)
		return false;
	
	return UWindow::IsChild(*this, *inWindow);								
}

bool CWindow::GetChildrenList(CPtrList<CWindow>& outChildrenList)
{
	return UWindow::GetChildrenList(*this, outChildrenList);
}

void CWindow::SetVisibleChildren(Uint16 inVisible)
{
	Uint32 i = 0;
	CWindow *win;
		
	while (gWindowList.GetNextWindow(win, i))
	{
		if (IsChild(win) && win->IsVisible() != inVisible)
			win->SetVisible(inVisible);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CWindow::SetBounds(const SRect& inBounds)
{
	UWindow::SetBounds(mWindow, inBounds);

	//#error
	/****************************************************
	
	Perhaps it would be better to resize the view like it
	was sizing_BottomRightSticky.  That way you wouldn't
	need a CContainerView if you just want a CScrollerView
	in the window.
	
	*******************************************************/
	if (mView)
	{
		SRect r;
		r.right = inBounds.GetWidth();
		r.bottom = inBounds.GetHeight();
		r.top = r.left = 0;
		mView->SetBounds(r);
	}
	
	
	//#error "should we be doing this here or in BoundsChanged() ?"
	//#error "if BoundsChanged() does it again doesn't really matter since setting it again to the same rect won't do anything"

	//#error see the #error about this further up
}

void CWindow::SetAutoBounds(Uint16 inPosition, Uint16 inPosOn)
{
	SRect r;
	if (GetAutoBounds(inPosition, inPosOn, r))
		SetBounds(r);
}

void CWindow::SetLocation(const SPoint& inTopLeft)
{
	SRect r;
	GetBounds(r);
	r.MoveTo(inTopLeft);
	SetBounds(r);
}

bool CWindow::InsertAfter(CWindow *inInsertAfter, bool inActivate)
{	
	if (!inInsertAfter)
		return false;
	
	return UWindow::InsertAfter(mWindow, *inInsertAfter, inActivate);
}

CWindow *CWindow::GetFocus()
{
	TWindow win = UWindow::GetFocus();
	if (win) return (CWindow *)UWindow::GetRef(win);
	
	return nil;
}

CWindow *CWindow::GetFront(Uint16 inLayer)
{
	TWindow win = UWindow::GetFront(inLayer);
	if (win) return (CWindow *)UWindow::GetRef(win);
	
	return nil;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CWindow::CaptureMouse()
{
	UWindow::SetMouseCaptureWindow(mWindow);
}

void CWindow::ReleaseMouse()
{
	UWindow::ReleaseMouseCaptureWindow(mWindow);
}

bool CWindow::IsMouseCaptureWindow()
{
	if (GetMouseCaptureWindow() == this)
		return true;
	
	return false;
}

CWindow *CWindow::GetMouseCaptureWindow()
{
	TWindow win = UWindow::GetMouseCaptureWindow();
	if (win) return (CWindow *)UWindow::GetRef(win);
	
	return nil;
}

void CWindow::MouseCaptureReleased()
{
	if (mIsMouseCapturedView)
	{
		if (mView)
			mView->MouseCaptureReleased();
	
		mIsMouseCapturedView = false;
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CWindow::MakeNoSnapWin()
{	
	gSnapWindows.AddNoSnapWin(this);
}

void CWindow::MakeNoResizeWin()
{		
	gSnapWindows.AddNoResizeWin(this);
}

void CWindow::MakeKeepOnScreenWin()
{
	gSnapWindows.AddKeepOnScreenWin(this);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CWindow::IsValid(CWindow *inWindow)
{
	return gWindowList.IsInList(inWindow);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CWindow::HandleInstall(CView *inView)
{
	ASSERT(inView);
	
	if (mView != inView)
	{
		inView->ChangeState(GetState());
		CSingleViewContainer::HandleInstall(inView);
		Refresh();
	}
}

void CWindow::HandleRemove(CView *inView)
{
	if (mView == inView)
	{
		CSingleViewContainer::HandleRemove(inView);
		
		// might be hits from this view in the hit queue, so just wipe the whole thing
		CLink *hitp = mHitQueue.GetFirst();
		CLink *nextp;
		while (hitp)
		{
			nextp = hitp->GetNext();
			UMemory::Dispose((TPtr)hitp);
			hitp = nextp;
		}
		
		mIsMouseCapturedView = false;
		Refresh();
	}
}

void CWindow::HandleRefresh(CView */* inView */, const SRect& inUpdateRect)
{
	Refresh(inUpdateRect);
}

void CWindow::HandleHit(CView */* inView */, const SHitMsgData& inInfo)
{
	UWindow::PostMessage(mWindow, msg_Hit, &inInfo, sizeof(SHitMsgData) + inInfo.dataSize, priority_High);
}

void CWindow::HandleGetScreenDelta(CView */* inView */, Int32& outHoriz, Int32& outVert)
{
	SRect stBounds;
	GetBounds(stBounds);
	
	outHoriz = stBounds.left;
	outVert = stBounds.top;
}

void CWindow::HandleGetVisibleRect(CView */* inView */, SRect& outRect)
{
	SRect stBounds;
	GetBounds(stBounds);

	outRect.left = outRect.top = 0;
	outRect.right = stBounds.GetWidth();
	outRect.bottom = stBounds.GetHeight();
}

void CWindow::HandleCaptureMouse(CView *inView)
{
	if (!mIsMouseCapturedView && inView == mView)
	{
		mIsMouseCapturedView = true;
		UWindow::SetMouseCaptureWindow(mWindow);
	}
}

void CWindow::HandleReleaseMouse(CView *inView)
{
	if (mIsMouseCapturedView && inView == mView)
		UWindow::ReleaseMouseCaptureWindow(mWindow);
}

CView *CWindow::HandleGetCaptureMouseView()
{
	if (mIsMouseCapturedView)
		return mView;

	return nil;
}

CWindow *CWindow::HandleGetParentWindow()
{
	return this;
}

#pragma mark -
/* -------------------------------------------------------------------------- */

CWindowList::CWindowList()
{
}

CWindowList::~CWindowList()
{
	ClearWindowList();
}

Uint32 CWindowList::AddWindow(CWindow *inWindow)
{
	if (!inWindow) 
		return 0;
	
	return mWindowList.AddItem(inWindow);
}

Uint32 CWindowList::RemoveWindow(CWindow *inWindow)
{
	if (!inWindow) 
		return 0;

	return mWindowList.RemoveItem(inWindow);
}

void CWindowList::ClearWindowList()
{
	CWindow *pWindow = mWindowList.GetItem(1);

	while (pWindow)
	{
		delete pWindow;
		pWindow = mWindowList.GetItem(1);
	}
}


#pragma mark -
/* -------------------------------------------------------------------------- */

void _GetRealWinBounds(CWindow *inWindow, SRect& outWindowBounds);
void _SetRealWinBounds(CWindow *inWindow, SRect inWindowBounds);
void _SetRealWinLocation(CWindow *inWindow, SPoint inWindowLocation, CWindow *inInsertAfter = nil);
void _GetScreenBounds(SRect& outScreenBounds);


#define WIN_SNAP_DIST_EXT	12
#define WIN_SNAP_DIST_INT	8

void UWindow::EnableSnapWindows(bool inEnableDisable, Int32 inSnapSoundID, Int32 inDetachSoundID)
{
	gSnapWindows.EnableSnapWindows(inEnableDisable, inSnapSoundID, inDetachSoundID);
}

CSnapWindows::CSnapWindows()
{
	mEnableDisable = false;
	mSnapSoundID = 0;
	mDetachSoundID = 0;
	
	mBringToFront = false;
	mAnchorSnapWin = nil;
	mAnchorSnapWinBounds.SetEmpty();

	mSnapLeft = mSnapTop = mSnapRight = mSnapBottom = false;
	mSnapRightTop = mSnapLeftTop = mSnapLeftBottom = mSnapRightBottom = false;
	mScreenLeft = mScreenTop = mScreenRight = mScreenBottom = false;

	mTempSnapWin = nil;
}

CSnapWindows::~CSnapWindows()
{
	mNoSnapWinList.Clear();
	mNoResizeWinList.Clear();
	mScreenWinList.Clear();

	mTempSnapWinList.Clear();
}

void CSnapWindows::EnableSnapWindows(bool inEnableDisable, Int32 inSnapSoundID, Int32 inDetachSoundID)
{
	mEnableDisable = inEnableDisable;

	mSnapSoundID = inSnapSoundID;
	mDetachSoundID = inDetachSoundID;
}

bool CSnapWindows::IsEnableSnapWindows()
{
	return mEnableDisable;
}

void CSnapWindows::AddNoSnapWin(CWindow *inWindow)
{
	if (!inWindow)
		return;
	
	mNoSnapWinList.AddItem(inWindow);
}

void CSnapWindows::AddNoResizeWin(CWindow *inWindow)
{
	if (!inWindow)
		return;
	
	mNoResizeWinList.AddItem(inWindow);
}

void CSnapWindows::AddKeepOnScreenWin(CWindow *inWindow)
{
	if (!inWindow)
		return;

	mScreenWinList.AddItem(inWindow);
}

void CSnapWindows::RemoveWindow(CWindow *inWindow)
{
	if (!inWindow) 
		return;

	mNoSnapWinList.RemoveItem(inWindow);
	mNoResizeWinList.RemoveItem(inWindow);
	mScreenWinList.RemoveItem(inWindow);
}

void CSnapWindows::CreateSnapWinList(CWindow *inWindow)
{
	if (mTempSnapWinList.GetItemCount()) 
		mTempSnapWinList.Clear();
	
	mBringToFront = true;
	mAnchorSnapWin = inWindow;
	_GetRealWinBounds(mAnchorSnapWin, mAnchorSnapWinBounds);

	mTempSnapWinList.AddItem(mAnchorSnapWin);
	ComposeSnapWinList(mAnchorSnapWin, mAnchorSnapWinBounds);	
	
	ComposeSnapSound(mAnchorSnapWin, mAnchorSnapWinBounds, false);
}

void CSnapWindows::CreateSnapSound(CWindow *inWindow)
{
	SRect rWindowBounds;
	_GetRealWinBounds(inWindow, rWindowBounds);

	ComposeSnapSound(inWindow, rWindowBounds, false);
}

void CSnapWindows::DestroySnapWinListSound(bool inClear)
{
	if (mAnchorSnapWin)
	{
		if (mBringToFront)
			mBringToFront = false;
		
		mTempSnapWinList.Clear();
		mAnchorSnapWinBounds.SetEmpty();
		
		if (inClear)
			mAnchorSnapWin = nil;
		else if (mTempSnapWinList.GetItemCount() > 1)
			mTempSnapWinList.AddItem(mAnchorSnapWin);
	}

	if (inClear)
	{
		mSnapLeft = mSnapTop = mSnapRight = mSnapBottom = false;
		mSnapRightTop = mSnapLeftTop = mSnapLeftBottom = mSnapRightBottom = false;
		mScreenLeft = mScreenTop = mScreenRight = mScreenBottom = false;
	}
}

void CSnapWindows::WinSnapTogether(CWindow *inWindow, bool inMoveSize)
{
	SRect rWindowBounds;
	_GetRealWinBounds(inWindow, rWindowBounds);

	if (WinSnapTogether(inWindow, rWindowBounds, inMoveSize))
		_SetRealWinBounds(inWindow, rWindowBounds);
}

bool CSnapWindows::WinSnapTogether(CWindow *inWindow, SRect& ioWindowBounds, bool inMoveSize)
{
	if (inWindow->GetLayer() == windowLayer_Modal || !inWindow->IsVisible() || inWindow->IsCollapsed() || inWindow->IsZoomed() || ioWindowBounds.IsEmpty()  || inWindow == mTempSnapWin)
		return false;
	
	bool bChangeScreen = false;
	if (mNoSnapWinList.IsInList(inWindow) || (inWindow == mAnchorSnapWin && mTempSnapWinList.GetItemCount() > 1))
	{
		bChangeScreen = RectSnapScreen(ioWindowBounds, inMoveSize);
		ComposeSnapSound(inWindow, ioWindowBounds, true, false);
	
		return bChangeScreen;
	}

	Int32 nWindowWidth = ioWindowBounds.GetWidth();
	Int32 nWindowHeight = ioWindowBounds.GetHeight();

	CWindow *win = nil;
	Uint32 i = 0;
		
	bool bChangedWin = false;
	while (gWindowList.GetNextWindow(win, i))
	{
		if (win == inWindow || win->GetLayer() == windowLayer_Modal || !win->IsVisible() || win->IsCollapsed() || win->IsZoomed())
			continue;
		
		SRect rWinBounds;
		_GetRealWinBounds(win, rWinBounds);
		
		if (rWinBounds.IsEmpty())		
			continue;
	
		if (RectSnapTogether(ioWindowBounds, rWinBounds, inMoveSize))
			bChangedWin = true;
			
		if (inMoveSize && mNoResizeWinList.IsInList(inWindow))
		{
			ioWindowBounds.right = ioWindowBounds.left + nWindowWidth;
			ioWindowBounds.bottom = ioWindowBounds.top + nWindowHeight;
		}
	}

	if (!bChangedWin  || !inMoveSize)
		bChangeScreen = RectSnapScreen(ioWindowBounds, inMoveSize);

	ComposeSnapSound(inWindow, ioWindowBounds);
	
	return (bChangedWin || bChangeScreen);
}

void CSnapWindows::WinMoveTogether(CWindow *inWindow)
{
	if (inWindow == mAnchorSnapWin && mTempSnapWinList.GetItemCount() > 1)
	{
		WinMoveTogether(inWindow, mAnchorSnapWinBounds);
		_GetRealWinBounds(mAnchorSnapWin, mAnchorSnapWinBounds);
	}	
}

#if MACINTOSH
bool _WNKeepUnderSystemMenu(SRect& ioWindowBounds);
#endif

void CSnapWindows::WinMoveTogether(CWindow *inWindow, SRect inWindowBounds)
{
	SRect rNewWindowsBounds;
	_GetRealWinBounds(inWindow, rNewWindowsBounds);

	Int32 nHorizDepl = rNewWindowsBounds.left - inWindowBounds.left;
	Int32 nVertDepl =  rNewWindowsBounds.top - inWindowBounds.top;

	if (nHorizDepl == 0 && nVertDepl == 0)
		return;
		
	CWindow *prev = nil;
	if (mBringToFront)
		prev = mTempSnapWinList.GetItem(1);

	bool bRefreshSnapList = false;
	
	CWindow *win = nil;
	Uint32 i = 1;
	
	while (mTempSnapWinList.GetNext(win, i))
	{
		if (!gWindowList.IsInList(win) || !win->IsVisible() || win->IsCollapsed() || win->IsZoomed())
		{
			bRefreshSnapList = true;
			mTempSnapWinList.RemoveItem(win);
			i--;
			
			continue;
		}
		
		SRect rWinBounds;
		_GetRealWinBounds(win, rWinBounds);
		rWinBounds.Move(nHorizDepl, nVertDepl);

		if ((mScreenWinList.IsInList(win) && RectKeepOnScreen(rWinBounds))
		#if MACINTOSH
			|| _WNKeepUnderSystemMenu(rWinBounds)
		#endif
			)
		{
			bRefreshSnapList = true;
			mTempSnapWinList.RemoveItem(win);
			i--;
		}
		
		mTempSnapWin = win;
		_SetRealWinLocation(win, rWinBounds.TopLeft(), prev);
		
		if (mBringToFront)
			prev = win;
	}
	
	if (mBringToFront)
	{
		mBringToFront = false;
		
		prev = mTempSnapWinList.GetItem(1);
		if (prev)
			UWindow::BringToFront(*prev);
	}
	
	mTempSnapWin = nil;
	
	if (bRefreshSnapList)
		CreateSnapWinList(mTempSnapWinList.GetItem(1));
}

bool CSnapWindows::WinKeepOnScreen(CWindow *inWindow, SRect& inWindowBounds)
{
	if (mScreenWinList.IsInList(inWindow) && !inWindow->IsCollapsed())
		return RectKeepOnScreen(inWindowBounds);
		
	return false;
}

bool CSnapWindows::RectSnapTogether(SRect& inWindowBounds, SRect inWinBounds, bool inMoveSize)
{
	bool bChangedHoriz = false;
	bool bChangedVert = false; 
	Int32 nDepl;
	
	if (inWindowBounds.Intersects(inWinBounds))
	{
		nDepl = inWindowBounds.right - inWinBounds.left;
		if (nDepl > 0 && nDepl <= WIN_SNAP_DIST_INT)
		{	
			bChangedHoriz = true; 
			
			if (inMoveSize)	
				inWindowBounds.MoveBack(nDepl, 0);
			else			
				inWindowBounds.right -= nDepl;
		}
		else
		{
			nDepl = inWinBounds.right - inWindowBounds.left;
			if (nDepl > 0 && nDepl <= WIN_SNAP_DIST_INT)
			{
				bChangedHoriz = true;
				
				if (inMoveSize)	
					inWindowBounds.Move(nDepl, 0);
				else
					inWindowBounds.left += nDepl;
			}
		}

		nDepl = inWindowBounds.bottom - inWinBounds.top;
		if (nDepl > 0 && nDepl <= WIN_SNAP_DIST_INT)
		{
			bChangedVert = true;
			
			if (inMoveSize)	
				inWindowBounds.MoveBack(0, nDepl);
			else
				inWindowBounds.bottom -= nDepl;
		}
		else
		{
			nDepl = inWinBounds.bottom - inWindowBounds.top;
			if (nDepl > 0 && nDepl <= WIN_SNAP_DIST_INT)
			{
				bChangedVert = true;
				
				if (inMoveSize)	
					inWindowBounds.Move(0, nDepl);
				else
					inWindowBounds.top += nDepl;
			}
		}
	}
	else
	{						
		if (inWindowBounds.top < inWinBounds.bottom && inWindowBounds.bottom > inWinBounds.top)
		{
			nDepl = inWinBounds.left - inWindowBounds.right;
			if (nDepl > 0 && nDepl <= WIN_SNAP_DIST_EXT)
			{				
				bChangedHoriz = true;
				
				if (inMoveSize)	
					inWindowBounds.Move(nDepl, 0);
				else
					inWindowBounds.right += nDepl;
			}
			else
			{
				nDepl = inWindowBounds.left - inWinBounds.right;
				if (nDepl > 0 && nDepl <= WIN_SNAP_DIST_EXT)
				{
					bChangedHoriz = true;
					
					if (inMoveSize)	
						inWindowBounds.MoveBack(nDepl, 0);
					else
						inWindowBounds.left -= nDepl;
				}
			}
		
		} 
		else if (inWindowBounds.left < inWinBounds.right && inWindowBounds.right > inWinBounds.left)
		{
			nDepl = inWinBounds.top - inWindowBounds.bottom;
			if (nDepl > 0 && nDepl <= WIN_SNAP_DIST_EXT)
			{
				bChangedVert = true;
								
				if (inMoveSize)	
					inWindowBounds.Move(0, nDepl);
				else
					inWindowBounds.bottom += nDepl;
			}
			else 
			{
				nDepl = inWindowBounds.top - inWinBounds.bottom;
				if (nDepl > 0 && nDepl <= WIN_SNAP_DIST_EXT)
				{
					bChangedVert = true;
					
					if (inMoveSize)	
						inWindowBounds.MoveBack(0, nDepl);
					else
						inWindowBounds.top -= nDepl;
				}
			}
		}
		else
		{
			Int32 nDeplHoriz = inWinBounds.left - inWindowBounds.right;
			Int32 nDelpVert = inWindowBounds.top - inWinBounds.bottom;
			if (nDeplHoriz > 0 && nDeplHoriz <= WIN_SNAP_DIST_INT && nDelpVert > 0 && nDelpVert <= WIN_SNAP_DIST_INT)
			{
				if (inMoveSize)
				{
					inWindowBounds.Move(nDeplHoriz, -nDelpVert);
				}
				else
				{
					inWindowBounds.right += nDeplHoriz;
					inWindowBounds.top -= nDelpVert;	
				}
				
				return true;
			}

			nDeplHoriz = inWindowBounds.left - inWinBounds.right;
			if (nDeplHoriz > 0 && nDeplHoriz <= WIN_SNAP_DIST_INT && nDelpVert > 0 && nDelpVert <= WIN_SNAP_DIST_INT)
			{
				if (inMoveSize)
				{
					inWindowBounds.MoveBack(nDeplHoriz, nDelpVert);
				}
				else
				{
					inWindowBounds.left -= nDeplHoriz;
					inWindowBounds.top -= nDelpVert;	
				}
	
				return true;
			}

			nDelpVert = inWinBounds.top - inWindowBounds.bottom;
			if (nDeplHoriz > 0 && nDeplHoriz <= WIN_SNAP_DIST_INT && nDelpVert > 0 && nDelpVert <= WIN_SNAP_DIST_INT)
			{
				if (inMoveSize)
				{
					inWindowBounds.Move(-nDeplHoriz, nDelpVert);
				}
				else
				{
					inWindowBounds.left -= nDeplHoriz;
					inWindowBounds.bottom += nDelpVert;	
				}
	
				return true;
			}

			nDeplHoriz = inWinBounds.left - inWindowBounds.right;
			if (nDeplHoriz > 0 && nDeplHoriz <= WIN_SNAP_DIST_INT && nDelpVert > 0 && nDelpVert <= WIN_SNAP_DIST_INT)
			{
				if (inMoveSize)
				{
					inWindowBounds.Move(nDeplHoriz, nDelpVert);
				}
				else
				{
					inWindowBounds.right += nDeplHoriz;
					inWindowBounds.bottom += nDelpVert;	
				}
	
				return true;
			}
		}
	}
		
	if (inWindowBounds.right == inWinBounds.left || inWindowBounds.left == inWinBounds.right)
	{
		nDepl = inWindowBounds.top - inWinBounds.top;
		if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
		{
			bChangedVert = true;

			if (inMoveSize)
				inWindowBounds.MoveBack(0, nDepl);
			else
				inWindowBounds.top -= nDepl;
		}
		else 
		{
			nDepl = inWinBounds.bottom - inWindowBounds.top;
			if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
			{
				bChangedVert = true;
				
				if (inMoveSize)
					inWindowBounds.Move(0, nDepl);
				else
					inWindowBounds.top += nDepl;
			}
			else 
			{
				nDepl = inWindowBounds.bottom - inWinBounds.bottom;
				if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
				{
					bChangedVert = true;
					
					if (inMoveSize)
						inWindowBounds.MoveBack(0, nDepl);
					else
						inWindowBounds.bottom -= nDepl;
				}
				else 
				{
					nDepl = inWinBounds.top - inWindowBounds.bottom;
					if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
					{
						bChangedVert = true;
						
						if (inMoveSize)
							inWindowBounds.Move(0, nDepl);			
						else
							inWindowBounds.bottom += nDepl;
					}
				}
			}
		}

		if (inMoveSize)
		{
			nDepl = inWinBounds.top - inWindowBounds.top;
			if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
			{
				bChangedVert = true;
				inWindowBounds.top += nDepl;
			}
			else
			{
				nDepl = inWinBounds.bottom - inWindowBounds.bottom;
				if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
				{
					bChangedVert = true;
					inWindowBounds.bottom += nDepl;
				}
			}
		}
	}
	else if (inWindowBounds.bottom == inWinBounds.top || inWindowBounds.top == inWinBounds.bottom)
	{
		nDepl = inWindowBounds.left - inWinBounds.left;
		if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
		{
			bChangedHoriz = true;
			
			if (inMoveSize)
				inWindowBounds.MoveBack(nDepl, 0);
			else
				inWindowBounds.left -= nDepl;
		}
		else 
		{
			nDepl = inWinBounds.right - inWindowBounds.left;
			if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
			{
				bChangedHoriz = true;
				
				if (inMoveSize)
					inWindowBounds.Move(nDepl, 0);
				else
					inWindowBounds.left += nDepl;
			}
			else 
			{
				nDepl = inWindowBounds.right - inWinBounds.right;
				if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
				{
					bChangedHoriz = true;
				
					if (inMoveSize)
						inWindowBounds.MoveBack(nDepl, 0);
					else
						inWindowBounds.right -= nDepl;
				}
				else 
				{
					nDepl = inWinBounds.left - inWindowBounds.right;
					if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
					{
						bChangedHoriz = true;
						
						if (inMoveSize)
							inWindowBounds.Move(nDepl, 0);
						else
							inWindowBounds.right += nDepl;
					}
				}
			}
		}

		if (inMoveSize)
		{
			nDepl = inWinBounds.left - inWindowBounds.left;
			if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
			{
				bChangedHoriz = true;
				inWindowBounds.left += nDepl;
			}
			else
			{
				nDepl = inWinBounds.right - inWindowBounds.right;
				if (nDepl != 0 && ::abs(nDepl) <= WIN_SNAP_DIST_EXT)
				{
					bChangedHoriz = true;
					inWindowBounds.right += nDepl;
				}
			}
		}
	}

	return (bChangedHoriz | bChangedVert);
}

bool CSnapWindows::RectSnapScreen(SRect& ioWindowBounds, bool inMoveSize)
{
	bool bChanged = false;
	
	SRect rScreenBounds;
	_GetScreenBounds(rScreenBounds);

	if (::abs(ioWindowBounds.left - rScreenBounds.left) <= WIN_SNAP_DIST_EXT)
	{
		bChanged = true;
		
		if (inMoveSize)
			ioWindowBounds.MoveBack(ioWindowBounds.left - rScreenBounds.left, 0);
		else
			ioWindowBounds.left = rScreenBounds.left;
	}
	
	if (::abs(rScreenBounds.right - ioWindowBounds.right) <= WIN_SNAP_DIST_EXT)
	{
		bChanged = true;
		
		if (inMoveSize)
			ioWindowBounds.Move(rScreenBounds.right - ioWindowBounds.right, 0);
		else
			ioWindowBounds.right = rScreenBounds.right;
	}
	
	if (::abs(ioWindowBounds.top - rScreenBounds.top) <= WIN_SNAP_DIST_EXT)
	{
		bChanged = true;
		
		if (inMoveSize)
			ioWindowBounds.MoveBack(0, ioWindowBounds.top - rScreenBounds.top);
		else
			ioWindowBounds.top = rScreenBounds.top;
	}
	
	if (::abs(rScreenBounds.bottom - ioWindowBounds.bottom) <= WIN_SNAP_DIST_EXT)
	{
		bChanged = true;
	
		if (inMoveSize)
			ioWindowBounds.Move(0, rScreenBounds.bottom - ioWindowBounds.bottom);
		else
			ioWindowBounds.bottom = rScreenBounds.bottom;
	}

	return bChanged;
}

bool CSnapWindows::RectKeepOnScreen(SRect& ioWindowBounds)
{
	bool bChanged = false;
	
	SRect rScreenBounds;
	_GetScreenBounds(rScreenBounds);

	if (rScreenBounds.left > ioWindowBounds.left)
	{
		bChanged = true;
		ioWindowBounds.Move(rScreenBounds.left - ioWindowBounds.left, 0);
	}
	
	if (ioWindowBounds.right > rScreenBounds.right)
	{
		bChanged = true;
		ioWindowBounds.MoveBack(ioWindowBounds.right - rScreenBounds.right, 0);
	}
	
	if (rScreenBounds.top > ioWindowBounds.top)
	{
		bChanged = true;
		ioWindowBounds.Move(0, rScreenBounds.top - ioWindowBounds.top);
	}
	
	if (ioWindowBounds.bottom > rScreenBounds.bottom)
	{
		bChanged = true;
		ioWindowBounds.MoveBack(0, ioWindowBounds.bottom - rScreenBounds.bottom);
	}

	return bChanged;
}

void CSnapWindows::ComposeSnapWinList(CWindow *inWindow, SRect inWindowBounds)
{
	if (inWindow->GetLayer() == windowLayer_Modal || !inWindow->IsVisible() || inWindow->IsCollapsed() || inWindow->IsZoomed())
		return;

	CWindow *win = nil;
	Uint32 i = 0;
	
	while (gWindowList.GetNextWindow(win, i))
	{
		if (win == inWindow || win->GetLayer() == windowLayer_Modal || !win->IsVisible() || win->IsCollapsed() || win->IsZoomed())
			continue;
		
		SRect rWinBounds;
		_GetRealWinBounds(win, rWinBounds);
		
		if (rWinBounds.IsEmpty())		
			continue;

		if (!mTempSnapWinList.IsInList(win) && 
			((rWinBounds.top < inWindowBounds.bottom && rWinBounds.bottom > inWindowBounds.top && (rWinBounds.right == inWindowBounds.left || rWinBounds.left == inWindowBounds.right)) ||
		     (rWinBounds.left < inWindowBounds.right && rWinBounds.right > inWindowBounds.left && (rWinBounds.bottom == inWindowBounds.top || rWinBounds.top == inWindowBounds.bottom)) ||
		     (rWinBounds.left == inWindowBounds.right && inWindowBounds.top == rWinBounds.bottom) || (inWindowBounds.left == rWinBounds.right && inWindowBounds.top == rWinBounds.bottom) ||
		     (inWindowBounds.left == rWinBounds.right && rWinBounds.top == inWindowBounds.bottom) || (rWinBounds.left == inWindowBounds.right && rWinBounds.top == inWindowBounds.bottom)))
		{
			mTempSnapWinList.AddItem(win);
			ComposeSnapWinList(win, rWinBounds);
		}
	}
}

void CSnapWindows::ComposeSnapSound(CWindow *inWindow, SRect inWindowBounds, bool inPlaySound, bool inCheckWin)
{
	if (inWindow->GetLayer() == windowLayer_Modal || !inWindow->IsVisible() || inWindow->IsCollapsed() || inWindow->IsZoomed() || (!mSnapSoundID && !mDetachSoundID))
		return;

	bool bSnapLeft = false, bSnapTop = false, bSnapRight = false, bSnapBottom = false;
	bool bSnapRightTop = false, bSnapLeftTop = false, bSnapLeftBottom = false, bSnapRightBottom = false;
	bool bScreenLeft = false, bScreenTop = false, bScreenRight = false, bScreenBottom = false;

	bool bPlaySnap = false, bPlayDetach = false;

	if (inCheckWin)
	{
		CWindow *win = nil;
		Uint32 i = 0;
	
		while (gWindowList.GetNextWindow(win, i))
		{
			if (win == inWindow || win->GetLayer() == windowLayer_Modal || !win->IsVisible() || win->IsCollapsed() || win->IsZoomed())
				continue;
		
			SRect rWinBounds;
			_GetRealWinBounds(win, rWinBounds);
		
			if (rWinBounds.IsEmpty())		
				continue;
		
			if (rWinBounds.top < inWindowBounds.bottom && rWinBounds.bottom > inWindowBounds.top)
			{
				if (rWinBounds.right == inWindowBounds.left)
					bSnapLeft = true;
				else if (rWinBounds.left == inWindowBounds.right)
					bSnapRight = true;
			}
			else if (rWinBounds.left < inWindowBounds.right && rWinBounds.right > inWindowBounds.left)
			{
			 	if (rWinBounds.bottom == inWindowBounds.top)
		 			bSnapTop = true;
			 	else if (rWinBounds.top == inWindowBounds.bottom)
			 		bSnapBottom = true;
			}				
			else if (rWinBounds.left == inWindowBounds.right && inWindowBounds.top == rWinBounds.bottom)
			{
				bSnapRightTop = true;
			}
			else if (inWindowBounds.left == rWinBounds.right && inWindowBounds.top == rWinBounds.bottom)
			{
				bSnapLeftTop = true;
			}
			else if (inWindowBounds.left == rWinBounds.right && rWinBounds.top == inWindowBounds.bottom)
			{
				bSnapLeftBottom = true;
			}
			else if (rWinBounds.left == inWindowBounds.right && rWinBounds.top == inWindowBounds.bottom)
			{
				bSnapRightBottom = true;
			}
		}

		if (mSnapLeft != bSnapLeft)	
		{
			mSnapLeft = bSnapLeft;
			if (mSnapLeft)
				bPlaySnap = true;
			else
				bPlayDetach = true;
		}

		if (mSnapRight != bSnapRight)	
		{
			mSnapRight = bSnapRight;
			if (mSnapRight)
				bPlaySnap = true;
			else
				bPlayDetach = true;
		}

		if (mSnapTop != bSnapTop)	
		{
			mSnapTop = bSnapTop;
			if (mSnapTop)
				bPlaySnap = true;
			else
				bPlayDetach = true;
		}

		if (mSnapBottom != bSnapBottom)	
		{
			mSnapBottom = bSnapBottom;
			if (mSnapBottom)
				bPlaySnap = true;
			else
				bPlayDetach = true;
		}

		if (mSnapRightTop != bSnapRightTop)	
		{
			mSnapRightTop = bSnapRightTop;
		
			if (!mSnapRight && !mSnapTop)
			{
				if (mSnapRightTop)
					bPlaySnap = true;
				else
					bPlayDetach = true;
			}
		}

		if (mSnapLeftTop != bSnapLeftTop)	
		{
			mSnapLeftTop = bSnapLeftTop;
		
			if (!mSnapLeft && !mSnapTop)
			{
				if (mSnapLeftTop)
					bPlaySnap = true;
				else
					bPlayDetach = true;
			}
		}

		if (mSnapLeftBottom != bSnapLeftBottom)	
		{
			mSnapLeftBottom = bSnapLeftBottom;
		
			if (!mSnapLeft && !mSnapBottom)
			{
				if (mSnapLeftBottom)
					bPlaySnap = true;
				else
					bPlayDetach = true;
			}
		}

		if (mSnapRightBottom != bSnapRightBottom)	
		{
			mSnapRightBottom = bSnapRightBottom;
		
			if (!mSnapRight && !mSnapBottom)
			{
				if (mSnapRightBottom)
					bPlaySnap = true;
				else
					bPlayDetach = true;
			}
		}
	}	
	
	SRect rScreenBounds;
	_GetScreenBounds(rScreenBounds);

	if (rScreenBounds.left == inWindowBounds.left)
		bScreenLeft = true;
	else if (rScreenBounds.right == inWindowBounds.right)
		bScreenRight = true;

	if (rScreenBounds.top == inWindowBounds.top)
		bScreenTop = true;
	else if (rScreenBounds.bottom == inWindowBounds.bottom)
		bScreenBottom = true;
	
	if (mScreenLeft != bScreenLeft)	
	{
		mScreenLeft = bScreenLeft;
		if (mScreenLeft)
			bPlaySnap = true;
		else
			bPlayDetach = true;
	}

	if (mScreenRight != bScreenRight)	
	{
		mScreenRight = bScreenRight;
		if (mScreenRight)
			bPlaySnap = true;
		else
			bPlayDetach = true;
	}

	if (mScreenTop != bScreenTop)	
	{
		mScreenTop = bScreenTop;
		if (mScreenTop)
			bPlaySnap = true;
		else
			bPlayDetach = true;
	}

	if (mScreenBottom != bScreenBottom)	
	{
		mScreenBottom = bScreenBottom;
		if (mScreenBottom)
			bPlaySnap = true;
		else
			bPlayDetach = true;
	}

	if (inPlaySound)
	{
		if (bPlaySnap)
		{
			if (mSnapSoundID) 
				USound::Play(nil, mSnapSoundID, false);
		}
		else if (bPlayDetach) 
		{
			if (mDetachSoundID) 
				USound::Play(nil, mDetachSoundID, false);
		}
	}
}


