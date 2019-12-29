/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "CView.h"
#include "CViewContainer.h"
#include "CPtrList.h"

class CContainerView : public CView, public CMultiViewContainer
{
	public:
		// construction
		CContainerView(CViewHandler *inHandler, const SRect& inBounds);
		virtual ~CContainerView();
		
		// default and cancel views
		void SetDefaultView(CView *inView)			{	mDefaultView = inView;						}
		void SetCancelView(CView *inView)			{	mCancelView = inView;						}
		CView *GetDefaultView() const				{	return mDefaultView;						}
		CView *GetCancelView() const				{	return mCancelView;							}
		
		// selection tabbing
		void SetTabKeyNavigation(bool inActive)		{	mIsTabKeyNav = (inActive != 0);				}
		bool IsTabKeyNavigation() const				{	return mIsTabKeyNav;						}
		bool IsFocusViaTab() const					{	return mIsFocusViaTab;						}
		
		virtual bool TabFocusNext();
		virtual bool TabFocusPrev();

		// misc
		virtual bool SetFocusView(CView *inView);
//		virtual bool CanAcceptDrop(TDrag inDrag) const;
		virtual bool ChangeState(Uint16 inState);
		virtual bool SetBounds(const SRect& inBounds);
		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);
		CView *PointToView(const SPoint& inLocalPt) const;
		virtual void MouseCaptureReleased();
	
		// event handling
		virtual void MouseDown(const SMouseMsgData& inInfo);
		virtual void MouseUp(const SMouseMsgData& inInfo);
		virtual void MouseEnter(const SMouseMsgData& inInfo);
		virtual void MouseMove(const SMouseMsgData& inInfo);
		virtual void MouseLeave(const SMouseMsgData& inInfo);
		virtual bool KeyDown(const SKeyMsgData& inInfo);
		virtual void KeyUp(const SKeyMsgData& inInfo);
		virtual bool KeyRepeat(const SKeyMsgData& inInfo);
		virtual void DragEnter(const SDragMsgData& inInfo);
		virtual void DragMove(const SDragMsgData& inInfo);
		virtual void DragLeave(const SDragMsgData& inInfo);
		virtual bool Drop(const SDragMsgData& inInfo);

		// QuickTime events
		virtual void UpdateQuickTime();
		virtual void SendToQuickTime(const EventRecord& inInfo);

	protected:
		CView *mFocusView;
		CView *mDefaultView, *mCancelView;
		CView *mMouseWithinView, *mDragWithinView;
		CView *mMouseCaptureView;
		THdl mKeyData, mMouseData;
		Uint16 mKeyDataCount, mMouseDataCount;
		Uint16 mIsTabKeyNav				: 1;
		Uint16 mIsFocusViaTab			: 1;
//		Uint16 mDragWithinViewCanAccept	: 1;
	
		// selection tabbing
		virtual CView *GetNextTabView() const;
		virtual CView *GetPrevTabView() const;
			
		// view handler callbacks
		virtual void HandleInstall(CView *inView);
		virtual void HandleRemove(CView *inView);
		virtual void HandleRefresh(CView *inView, const SRect& inUpdateRect);
		virtual void HandleHit(CView *inView, const SHitMsgData& inInfo);
		virtual void HandleGetScreenDelta(CView *inView, Int32& outHoriz, Int32& outVert);
		virtual void HandleGetVisibleRect(CView *inView, SRect& outRect);
		virtual void HandleCaptureMouse(CView *inView);
		virtual void HandleReleaseMouse(CView *inView);
		virtual void HandleGetOrigin(SPoint& outOrigin);
		virtual CView *HandleGetCaptureMouseView();
		virtual CWindow *HandleGetParentWindow();
};

