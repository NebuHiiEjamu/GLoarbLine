/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "CView.h"
#include "CViewContainer.h"
#include "UWindow.h"

class _PUCPopupWin;

class CPopupView : public CView//, public CSingleViewContainer
{
	public:
		friend class _PUCPopupWin;
		
		// construction
		CPopupView(CViewHandler *inHandler, const SRect& inBounds);
		virtual ~CPopupView();
	
		virtual void SetView(CView *inView);
		virtual CView *DetachView();
		
		CViewHandler *GetViewHandler() const									{	return mWindow;	}

	protected:
		// data members
		CWindow *mWindow;
		Uint16 mIsPopupOnRight	: 1;
		
		// popup window functions
		virtual void GetPopupButtonRect(SRect& outRect);
		virtual void ShowPopupWin();
		virtual void HidePopupWin();
		
/*		// window messages
		virtual void HandlePopupMessage(Uint32 inMsg, const void *inData, Uint32 inDataSize);
		static void PopupMessageHandler(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);

		// view handler callbacks
		virtual void HandleRefresh(CView *inView, const SRect& inUpdateRect);
		virtual void HandleHit(CView *inView, const SHitMsgData& inInfo);
*/
		virtual void HandleGetScreenDelta(CView *inView, Int32& outHoriz, Int32& outVert);
		virtual void HandleGetVisibleRect(CView *inView, SRect& outRect);
};

