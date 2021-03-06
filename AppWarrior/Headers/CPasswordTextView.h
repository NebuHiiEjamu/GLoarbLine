/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "UEditText.h"
#include "CView.h"

class CPasswordTextView : public CView
{
	public:
		// construction
		CPasswordTextView(CViewHandler *inHandler, const SRect& inBounds);
		virtual ~CPasswordTextView();
		
		// text
		void SetText(const void *inText, Uint32 inLength);
		Uint32 GetText(void *outText, Uint32 inMaxSize) const	{	return UMemory::Read(mPassword, 0, outText, inMaxSize);	}
		bool IsEmpty() const;
		Uint32 GetPasswordSize() const							{	return UMemory::GetSize(mPassword);						}

		// misc
		void SetDummyPassword();
		bool IsDummyPassword() const		{	return mIsDummy;	}
		void SetEnterKeyAction(Uint16 inReturnAction, Uint16 inEnterAction = enterKeyAction_None);
		Uint16 GetReturnKeyAction();
		Uint16 GetEnterKeyAction();
		virtual void Timer(TTimer inTimer);
		virtual bool ChangeState(Uint16 inState);
		virtual bool TabFocusNext();
		virtual bool TabFocusPrev();
		virtual bool SetEnable(bool inEnable);
		virtual bool SetBounds(const SRect& inBounds);
		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);
		virtual void MouseDown(const SMouseMsgData& inInfo);
		virtual void MouseUp(const SMouseMsgData& inInfo);
		virtual void MouseEnter(const SMouseMsgData& inInfo);
		virtual void MouseLeave(const SMouseMsgData& inInfo);
		virtual void MouseMove(const SMouseMsgData& inInfo);
		virtual bool KeyDown(const SKeyMsgData& inInfo);
		virtual void KeyUp(const SKeyMsgData& inInfo);
		virtual bool KeyRepeat(const SKeyMsgData& inInfo);

	protected:
		TEditText mEditText;
		TTimer mCaretTimer;
		THdl mPassword;
		bool mIsDummy;
		Uint16 mReturnKeyAction;
		Uint16 mEnterKeyAction;
		
		// internal functions
		static void TextDrawHandler(TEditText inRef, const SRect& inRect);
		void UpdateActive();
};

class CScrollerView;
CScrollerView *MakePasswordBoxView(CViewHandler *inHandler, const SRect& inBounds, Uint16 inOptions = scrollerOption_Border, CPasswordTextView **outTextView = nil);


