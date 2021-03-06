/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "MoreTypes.h"
#include "UGraphics.h"
#include "GrafTypes.h"
#include "CPtrList.h"

// window type
typedef class TWindowObj *TWindow;

#include "CView.h"	// need the above type prior to including cview.h


// options
enum {
	windowOption_CloseBox		= 0x01,	// does the window have a close box the user can click?
	windowOption_ZoomBox		= 0x02,	// does the window have a zoom box the user can click?
	windowOption_Sizeable		= 0x04,	// can the user resize the window?
	windowOption_WhiteBkgnd		= 0x10	// does the window have a white background? (as opposed to the default color, eg gray)
};

// layers
enum {
	windowLayer_Popup			= 1,	// eg popup menus, tooltips (top-most layer)
	windowLayer_Modal			= 2,	// eg modal windows
	windowLayer_Float			= 3,	// eg toolbars
	windowLayer_Standard		= 4		// eg normal windows (bottom-most layer)
};

// auto positioning
enum {
	windowPos_None				= 0,
	windowPos_Best				= 1,
	windowPos_Center			= 2,
	windowPos_CenterHoriz		= 3,
	windowPos_CenterVert		= 4,
	windowPos_Alert				= 5,
	windowPos_TopLeft			= 6,
	windowPos_TopRight			= 7,
	windowPos_BottomLeft		= 8,
	windowPos_BottomRight		= 9,
	windowPos_Stagger			= 10,
	windowPos_Full				= 11,
	windowPos_Unobstructed		= 12
};

// auto position on what
enum {
	windowPosOn_SameScreen		= 0,	// screen that window is currently on
	windowPosOn_MainScreen		= 1,	// screen that user has selected as the main
	windowPosOn_DeepestScreen	= 2,	// screen with the most colors
	windowPosOn_BiggestScreen	= 3,	// screen with the biggest pixel area
	windowPosOn_WinScreen		= 4,	// screen with window specified by SetMainWindow()
	windowPosOn_DocumentScreen	= 5,	// screen that topmost document window is on
	windowPosOn_EmptyScreen		= 6,	// screen with the least number of windows
	windowPosOn_SecondScreen	= 7,	// second screen (not the main screen)
	windowPosOn_ThirdScreen		= 8,	// third screen
	windowPosOn_FourthScreen	= 9		// fourth screen
};

// window states
enum {
	windowState_Hidden			= 1,	// window is hidden and not available to user
	windowState_Inactive		= 2,	// window is visible but temporarily not available to user
	windowState_Active			= 3,	// window is visible and available to user but not the keyboard focus
	windowState_Focus			= 4		// window is active and is the keyboard focus
};

// SetVisible constants
enum {
	kHideWindow					= 0,
	kShowWinAtFront				= 1,
	kShowWinAtBack				= 2
};

class CWindow;

// class prototype
class UWindow
{
	public:
		// new and dispose
		static void Init();
		static TWindow New(const SRect& inBounds, Uint16 inLayer = windowLayer_Standard, Uint16 inOptions = 0, Uint16 inStyle = 0, TWindow inParent = nil);
		static void Dispose(TWindow inWindow);
		
		// title
		static void SetTitle(TWindow inWindow, const Uint8 *inTitle);
		static void SetNoTitle(TWindow inWindow);
		static Uint32 GetTitle(TWindow inWindow, void *outText, Uint32 inMaxSize);

		// user reference
		static void SetRef(TWindow inWindow, void *inRef);
		static void *GetRef(TWindow inWindow);
		
		// visible
		static void SetVisible(TWindow inWindow, Uint16 inVisible);
		static bool IsVisible(TWindow inWindow);
		
		// child
		static bool IsChild(TWindow inParent, TWindow inChild);
		static bool GetChildrenList(TWindow inParent, CPtrList<CWindow>& outChildrenList);
	
		// background color
		static void SetBackColor(TWindow inWindow, const SColor& inColor);
		static void GetBackColor(TWindow inWindow, SColor& outColor);
		static void SetBackPattern(TWindow inWindow, Int16 inID);
		
		// layer
		static void SetLayer(TWindow inWindow, Uint16 inLayer);
		static Uint16 GetLayer(TWindow inWindow);
		
		// bounds
		static void SetBounds(TWindow inWindow, const SRect& inBounds);
		static void GetBounds(TWindow inWindow, SRect& outBounds);
		static void SetLocation(TWindow inWindow, SPoint& inTopLeft);
		static void SetLimits(TWindow inWindow, Int32 inMinWidth, Int32 inMinHeight, Int32 inMaxWidth = max_Int16, Int32 inMaxHeight = max_Int16);

		// automagic bounds
		static bool GetAutoBounds(TWindow inWindow, Uint16 inPosition, const SRect& inEnclosure, SRect& outBounds);
		static bool GetAutoBounds(TWindow inWindow, Uint16 inPosition, Uint16 inPosOn, SRect& outBounds);
		static void SetMainWindow(TWindow inWindow);
		static TWindow GetMainWindow();
		static bool GetZoomBounds(TWindow inWindow, const SRect& inIdealSize, SRect& outBounds);

		// misc properties
		static Uint16 GetStyle(TWindow inWindow);
		static Uint16 GetOptions(TWindow inWindow);
		static Uint16 GetState(TWindow inWindow);
		
		// messaging
		static void SetMessageHandler(TWindow inWindow, TMessageProc inProc, void *inContext = nil);
		static void PostMessage(TWindow inWindow, Uint32 inMsg, const void *inData = nil, Uint32 inDataSize = 0, Int16 inPriority = priority_Normal);
		static void InstallKeyCommands(const SKeyCmd *inCmds, Uint32 inCount, TMessageProc inProc, void *inContext);
		
		// layers
		static TWindow GetFront(Uint16 inLayer);
		static bool IsFront(TWindow inWindow);
		static bool BringToFront(TWindow inWindow, bool inActivate = true);
		static bool InsertAfter(TWindow inRef, TWindow inInsertAfter, bool inActivate = true);
		static bool SendToBack(TWindow inWindow);
		static TWindow GetNextVisible(TWindow inWindow);
		static bool InModal();
		
		// keyboard focus
		static void SetFocus(TWindow inWindow);
		static TWindow GetFocus();
		
		// mouse capture
		static void SetMouseCaptureWindow(TWindow inWindow);
		static void ReleaseMouseCaptureWindow(TWindow inWindow);
		static TWindow GetMouseCaptureWindow();
		
		// collapsing
		static void Collapse(TWindow inWindow);
		static void Expand(TWindow inWindow);
		static bool IsCollapsed(TWindow inWindow);
		static bool IsZoomed(TWindow inWindow);
		
		// snapping
		static void EnableSnapWindows(bool inEnableDisable, Int32 inSnapSoundID = 0, Int32 inDetachSoundID = 0);
		
		// coordinates
		static void GlobalToLocal(TWindow inWindow, SPoint& ioPoint);
		static void GlobalToLocal(TWindow inWindow, SRect& ioRect);
		static void LocalToGlobal(TWindow inWindow, SPoint& ioPoint);
		static void LocalToGlobal(TWindow inWindow, SRect& ioRect);
		
		// drawing
		static bool GetRedrawRect(TWindow inWin, SRect& outRect);
		static void Refresh(TWindow inWindow);
		static void Refresh(TWindow inWindow, const SRect& inRect);
		static bool IsColor(TWindow inWindow);
		static Uint16 GetDepth(TWindow inWindow);
		static TImage BeginDraw(TWindow inWindow, const SRect& inUpdateRect);
		static void EndDraw(TWindow inWindow);
		static void AbortDraw(TWindow inWindow);
};
typedef UWindow UWin;

/*
 * UWindow Object Interface
 */

class TWindowObj
{
	public:
		void SetTitle(const Uint8 *inTitle)																					{	UWindow::SetTitle(this, inTitle);											}
		void SetNoTitle()																									{	UWindow::SetNoTitle(this);													}
		Uint32 GetTitle(void *outText, Uint32 inMaxSize)																	{	return UWindow::GetTitle(this, outText, inMaxSize);							}

		void SetRef(void *inRef)																							{	UWindow::SetRef(this, inRef);												}
		void *GetRef()																										{	return UWindow::GetRef(this);												}
		
		void SetVisible(Uint16 inVisible)																					{	UWindow::SetVisible(this, inVisible);										}
		bool IsVisible()																									{	return UWindow::IsVisible(this);											}
		
		void GetBackColor(SColor& outColor)																					{	UWindow::GetBackColor(this, outColor);										}
		void SetBackPattern(Int16 inID)																						{	UWindow::SetBackPattern(this, inID);										}
		
		void SetLayer(Uint16 inLayer)																						{	UWindow::SetLayer(this, inLayer);											}
		Uint16 GetLayer()																									{	return UWindow::GetLayer(this);												}
		
		void SetBounds(const SRect& inBounds)																				{	UWindow::SetBounds(this, inBounds);											}
		void GetBounds(SRect& outBounds)																					{	UWindow::GetBounds(this, outBounds);										}
		void SetLocation(SPoint& inTopLeft)																					{	UWindow::SetLocation(this, inTopLeft);										}
		void SetLimits(Int32 inMinWidth, Int32 inMinHeight, Int32 inMaxWidth = max_Int16, Int32 inMaxHeight = max_Int16)	{	UWindow::SetLimits(this, inMinWidth, inMinHeight, inMaxWidth, inMaxHeight);	}

		bool GetAutoBounds(Uint16 inPosition, const SRect& inEnclosure, SRect& outBounds)									{	return UWindow::GetAutoBounds(this, inPosition, inEnclosure, outBounds);	}
		bool GetAutoBounds(Uint16 inPosition, Uint16 inPosOn, SRect& outBounds)												{	return UWindow::GetAutoBounds(this, inPosition, inPosOn, outBounds);		}
		void SetMainWindow()																								{	UWindow::SetMainWindow(this);												}
		bool GetZoomBounds(const SRect& inIdealSize, SRect& outBounds)														{	return UWindow::GetZoomBounds(this, inIdealSize, outBounds);				}

		Uint16 GetStyle()																									{	return UWindow::GetStyle(this);												}
		Uint16 GetOptions()																									{	return UWindow::GetOptions(this);											}
		Uint16 GetState()																									{	return UWindow::GetState(this);												}
		
		void SetMessageHandler(TMessageProc inProc, void *inContext = nil)													{	UWindow::SetMessageHandler(this, inProc, inContext);						}
		void PostMessage(Uint32 inMsg, const void *inData = nil, Uint32 inDataSize = 0, Int16 inPriority = priority_Normal)	{	UWindow::PostMessage(this, inMsg, inData, inDataSize, inPriority);			}
		
		bool IsFront()																										{	return UWindow::IsFront(this);												}
		bool BringToFront()																									{	return UWindow::BringToFront(this);											}
		bool SendToBack()																									{	return UWindow::SendToBack(this);											}
		TWindow GetNextVisible()																							{	return UWindow::GetNextVisible(this);										}
		
		void SetFocus()																										{	UWindow::SetFocus(this);													}

		void Collapse()																										{	UWindow::Collapse(this);													}
		void Expand()																										{	UWindow::Expand(this);														}
		bool IsCollapsed()																									{	return UWindow::IsCollapsed(this);											}
		
		void GlobalToLocal(SPoint& ioPoint)																					{	UWindow::GlobalToLocal(this, ioPoint);										}
		void GlobalToLocal(SRect& ioRect)																					{	UWindow::GlobalToLocal(this, ioRect);										}
		void LocalToGlobal(SPoint& ioPoint)																					{	UWindow::LocalToGlobal(this, ioPoint);										}
		void LocalToGlobal(SRect& ioRect)																					{	UWindow::LocalToGlobal(this, ioRect);										}
		
		bool GetRedrawRect(SRect& outRect)																					{	return UWindow::GetRedrawRect(this, outRect);								}
		void Refresh()																										{	UWindow::Refresh(this);														}
		void Refresh(const SRect& inRect)																					{	UWindow::Refresh(this, inRect);												}
		bool IsColor()																										{	return UWindow::IsColor(this);												}
		Uint16 GetDepth()																									{	return UWindow::GetDepth(this);												}
		TImage BeginDraw(const SRect& inUpdateRect)																			{	return UWindow::BeginDraw(this, inUpdateRect);								}
		void EndDraw()																										{	UWindow::EndDraw(this);	}
		void AbortDraw()																									{	UWindow::AbortDraw(this);													}
		
		void operator delete(void *p)																						{	UWindow::Dispose((TWindow)p);												}
	protected:
		TWindowObj() {}				// force creation via UWindow
};


