/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "CView.h"


class CLaunchUrlView : public CView
{
	public:
		// construction
		CLaunchUrlView(CViewHandler *inHandler, const SRect& inBounds);
		virtual ~CLaunchUrlView();
		
		// mouse events
		virtual void MouseUp(const SMouseMsgData& inInfo);
		virtual void MouseEnter(const SMouseMsgData& inInfo);
		virtual void MouseMove(const SMouseMsgData& inInfo);
		virtual void MouseLeave(const SMouseMsgData& inInfo);

		// view state
		virtual bool ChangeState(Uint16 inState);

		// set URL 
		void SetURL(const Uint8 *inURL, const Uint8 *inComment = nil);
		
		// get URL
		Uint32 GetURL(void *outURL, Uint32 inMaxSize);
		Uint32 GetComment(void *outComment, Uint32 inMaxSize);
		
		// launch URL
		virtual bool LaunchURL();

	protected:
		THdl mURL, mComment;
		
		void SetMouseLaunch();
		void SetMouseStandard();
};
