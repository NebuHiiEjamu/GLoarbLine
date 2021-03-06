#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
/*
 
Functions for window management.

-- Sizing Options --

Type of sizing:
	windowPos_None			0	bounds is used unchanged
	windowPos_Center		1	window is centered
	windowPos_CenterHoriz	2	horizontally centered, vertical is untouched
	windowPos_CenterVert	3	vertically centered, horizontal is untouched
	windowPos_Alert			4	window is placed in top-third, and centered horizontally
	windowPos_Stagger		5	window is placed just below and to the right of existing windows
	windowPos_Full			6	sized and positioned to take up the whole area
	windowPos_Bottom		7	window is placed at the bottom of the screen
	windowPos_BottomLeft	8	window is placed at the bottom left of the screen
	windowPos_Best			9	window is moved onscreen if not already entirely onscreen
	
Size according to:
	windowPosOn_Screen		0	main screen
	windowPosOn_Window		1	top-most window of same layer (main screen if none)
	windowPosOn_WinScreen	2	screen which top-most window of same layer resides on
	windowPosOn_Deepest		3	screen which has the greatest number of colours (depth)
*/

#include "UWindow.h"
#include "UMemory.h"
#include "UGraphics.h"
#include "UApplication.h"

#include <Windows.h>
#include <Errors.h>
#include <QDOffscreen.h>
#include <LowMem.h>

/*
 * Structures
 */

struct SWindow {
	WindowRef macWin;
	TMessageProc msgProc;
	void *msgProcContext;	
	void *userRef;
	TImage img;
	GWorldPtr gworld;
	SRect redrawRect;
	Rect saveBounds;
	Rect updateRect;
	Int16 minHeight, minWidth, maxHeight, maxWidth;
	Uint16 flags, style;
	Uint8 layer, state;
	Uint8 needsRedraw;
	Uint8 isCollapsed;
	Uint8 isQuickTimeView;
	CPtrList<SWindow> childrenList;
};

#define WIN		((SWindow *)inWin)

/*
 * Function Prototypes
 */

static void _WNCheckWinStates();
static void _WNCheckCollapseState(TWindow inWin);
static WindowRef _WNFindPlaneForNewWindow(Uint16 layer);
static Int16 _WNStyleToProcNum(Uint16 layer, Uint16 flags, Uint16 style);
static void _WNSendBehind(WindowRef macWindow, WindowRef behindWindow);
static bool _WNSendToBack(TWindow inWin);
static bool _WNBringToFront(TWindow inWin);
static void _WNSetVisible(TWindow inWin, bool inVis);
static void _WNDrawGrowBox(TWindow inWin);
static void _WNResetClip(TWindow inWin);
static Int16 _WNGetTitleBarHeight(TWindow inWin);
static void _WNCalcLayersAboveRgn(TWindow inWin, RgnHandle rgn);
static void _WNCalcAboveRgn(WindowRef win, RgnHandle rgn);
static bool _WNTrackMove(TWindow inWin, Uint16 inMods, Point mouseLoc, Point& newLoc);
static bool _WNTrackGrow(TWindow inWin, Point startPt, Int32& newWidth, Int32& newHeight);
static void _WNClearUpdate(WindowRef win);
void _GetRealWinBounds(TWindow inWindow, Rect& outWindowBounds);
static void _WNCalcAdjustedBounds(TWindow inWin, Rect *r);
static void _WNCalcZoomedBounds(TWindow inWin, Rect *idealBounds, Rect& bestBounds);
static Int16 _WNCalculateOffsetAmount(Int16 idealStartPoint, Int16 idealEndPoint, Int16 idealOnScreenStartPoint, Int16 idealOnScreenEndPoint, Int16 screenEdge1, Int16 screenEdge2);
bool _WNKeepUnderSystemMenu(SRect& ioWindowBounds);
GDHandle _GetRectDevice(Rect *theRect);
static GDHandle _GetWindowDevice(WindowRef win);
static void _WNAdjustBoundsToScreen(Rect *theRect, bool shrink);
static void _WNCalcStaggeredBounds(TWindow inWin, Rect *r);
static void _WNCalcUnobstructedBounds(TWindow inWin, Rect *r);
static WindowRef _WNGetFirstOfLayer(Int16 layer);
static void _CenterRect(Rect& theRect, Rect base);
static void _CenterRectH(Rect& theRect, Rect base);
static void _CenterRectV(Rect& theRect, Rect base);
static void _WNUpdateHandler(WindowRef inWin);
static void _SetupMacMenuBar();
static void _DoMacMenuBar(Point startPt);
void _WNDispatchMouse(Uint32 inType, const SMouseMsgData& inInfo, bool inResumeClick = false);
void _DeleteFromParentList(SWindow *inWindow);

Int16 _GetSystemVersion();
GrafPtr _ImageToGrafPtr(TImage inImage);
TImage _NewGWImage(GWorldPtr inGW);
void _DisposeGWImage(TImage inImage);

#if TARGET_API_MAC_CARBON
	bool _InstallEventHandler(SWindow *inWin);
	pascal OSStatus _EventHandlerProc(EventHandlerCallRef inHandler, EventRef inEvent, void *inUserData);

	static EventHandlerUPP _EventHandlerProcUPP = ::NewEventHandlerUPP(_EventHandlerProc);
#endif

/*
 * Global Variables
 */

static struct {
	Uint16 visModalCount, visPopupCount;
	SWindow *mouseWithinWin;
	SWindow *mouseDownWin;
	SWindow *dragWithinWin;
	SWindow *mainWindow;
	SWindow *focusWin;
	SWindow *mouseCaptureWin;
	
	SKeyCmd *keyCmds;
	Uint32 keyCmdCount;
	TMessageProc keyCmdMsgProc;
	void *keyCmdMsgProcContext;

	Uint8 isInitted	: 1;
} _WNData = {0,0,0,0,0,0,0,0,0,0,0,0,0};

// special mac specific menu bar support
Int16 gMacMenuBarID = 0;
#if !TARGET_API_MAC_CARBON
static Uint16 _WNMacAppleMenuSize = 0;
#endif

extern void (*_APUpdateProc)(WindowRef inWin);
extern void (*_APWindowProcess)();
extern Uint8 _APInBkgndFlag;

Uint8 _WNCheckWinStatesFlag = false;
Uint8 _WNCheckMouseWithinWinFlag = false;

/*
 * Macros
 */
 
#ifndef topLeft
#define topLeft(r)	(((Point *) &(r))[0])
#define botRight(r)	(((Point *) &(r))[1])
#endif

// mac error handling
void _FailMacError(Int16 inMacError, const Int8 *inFile, Uint32 inLine);
inline void _CheckMacError(Int16 inMacError, const Int8 *inFile, Uint32 inLine) { if (inMacError) _FailMacError(inMacError, inFile, inLine); }
#if DEBUG
	#define FailMacError(id)		_FailMacError(id, __FILE__, __LINE__)
	#define CheckMacError(id)		_CheckMacError(id, __FILE__, __LINE__)
#else
	#define FailMacError(id)		_FailMacError(id, nil, 0)
	#define CheckMacError(id)		_CheckMacError(id, nil, 0)
#endif

/* -------------------------------------------------------------------------- */

void UWindow::Init()
{
	if (!_WNData.isInitted)
	{
		_APUpdateProc = _WNUpdateHandler;
		_APWindowProcess = _WNCheckWinStates;
		
		_SetupMacMenuBar();
		
		_WNData.isInitted = true;
	}
}

// creates a new hidden window
TWindow UWindow::New(const SRect& inBounds, Uint16 inLayer, Uint16 inFlags, Uint16 inStyle, TWindow inParent)
{
	SWindow *win = nil;
	WindowRef macWin = nil;
	GrafPtr savePort;
	Rect r;
	
	Init();
	
	// don't even THINK about creating a window if there's less than 4K available
	if (!UMemory::IsAvailable(4096))
		Fail(errorType_Memory, memError_NotEnough);

	try
	{
		// alloc extended data
		win = (SWindow *)UMemory::NewClear(sizeof(SWindow));
		
		// convert inBounds to mac rect
		r.top = inBounds.top;
		r.left = inBounds.left;
		r.bottom = inBounds.bottom;
		r.right = inBounds.right;
		
		// validate the layer
		switch (inLayer)
		{
			case windowLayer_Popup:
			case windowLayer_Modal:
			case windowLayer_Float:
			case windowLayer_Standard:
				break;
			default:
				inLayer = windowLayer_Standard;
				break;
		}
		
		// fill in extended data 
		win->layer = inLayer;
		win->flags = inFlags;
		win->style = inStyle;
		win->minHeight = win->minWidth = 100;
		win->maxHeight = win->maxWidth = 30000;
		win->userRef = nil;
		win->saveBounds = r;
		win->state = windowState_Hidden;
		
		// create the window.  IMPORTANT: title must not be nil, use empty pstring
		macWin = ::NewCWindow(nil, &r, "\p", false, _WNStyleToProcNum(inLayer, inFlags, inStyle), _WNFindPlaneForNewWindow(inLayer), inFlags & windowOption_CloseBox, (Int32)win);
		if (macWin == nil) 
			Fail(errorType_Memory, memError_NotEnough);
	
		win->macWin = macWin;
	#if TARGET_API_MAC_CARBON
		// check if we are running under MacOS X
		if (_GetSystemVersion() >= 0x0A00)
		{
			if (inLayer == windowLayer_Float || (inLayer == windowLayer_Popup && inStyle == 1))
				::SetWindowClass(macWin, kFloatingWindowClass);
		}
	#endif
	
		// set the font correctly
		::GetPort(&savePort);
		::SetPortWindowPort(macWin);
		::TextFont(0);
		::SetPort(savePort);
		
		// set gray background
		if ((inFlags & windowOption_WhiteBkgnd) == 0)	// if not white bkgnd
			SetBackColor((TWindow)win, color_GrayD);
			
		// add in the parent list
		if (inParent)
			((SWindow *)inParent)->childrenList.AddItem(win);
			
	#if TARGET_API_MAC_CARBON
		// install carbon event handler
		_InstallEventHandler(win);
	#endif
	}
	catch (...)
	{
		// clean up
		if (macWin) ::DisposeWindow(macWin);
		if (win) UMemory::Dispose((TPtr)win);
		throw;
	}
	
	// done
	return (TWindow)win;
}

// does not throw exceptions
void UWindow::Dispose(TWindow inWin)
{
	if (inWin)
	{
		WindowRef macWin = WIN->macWin;
		
		// kill any messages for this window
		if (WIN->msgProc)
			UApplication::FlushMessages(WIN->msgProc, WIN->msgProcContext, inWin);
		
		// make sure window isn't stored anyway
		if (WIN == _WNData.mouseDownWin)
			_WNData.mouseDownWin = nil;
		if (WIN == _WNData.dragWithinWin)
			_WNData.dragWithinWin = nil;
		if (WIN == _WNData.mainWindow)
			_WNData.mainWindow = nil;
		if (WIN == _WNData.focusWin)
			_WNData.focusWin = nil;
		if (WIN == _WNData.mouseCaptureWin)
			_WNData.mouseCaptureWin = nil;
		
		// if a visible modal/popup window, decrement modal/popup count
		if (IsWindowVisible(macWin))
		{
			switch (WIN->layer)
			{
				case windowLayer_Modal:
					_WNData.visModalCount--;
					break;
				case windowLayer_Popup:
					_WNData.visPopupCount--;
					break;
			}
		}

		WIN->childrenList.Clear();				// clear children list
		_DeleteFromParentList(WIN);				// delete from parent list
		
		// dispose the window
		::DisposeWindow(macWin);				// dispose mac window
		UMemory::Dispose((TPtr)inWin);			// dispose extended data
		
		// adjust hiliting
		_WNCheckWinStatesFlag = true;
		
		// if this window was the mouse-within window, it is no longer
		if (WIN == _WNData.mouseWithinWin)
		{
			_WNData.mouseWithinWin = nil;
			_WNCheckMouseWithinWinFlag = true;
			UMouse::SetImage(mouseImage_Standard);
		}
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetTitle(TWindow inWin, const Uint8 *inTitle)
{
	if (inWin)
	{
		if (inTitle)
			::SetWTitle(WIN->macWin, inTitle);
		else
			::SetWTitle(WIN->macWin, "\p");
	}
}

void UWindow::SetNoTitle(TWindow inWin)
{
	if (inWin) ::SetWTitle(WIN->macWin, "\p");		// do NOT use nil for the title
}

Uint32 UWindow::GetTitle(TWindow inWin, void *outText, Uint32 inMaxSize)
{
	if (inWin)
	{
		Str255 str;
		::GetWTitle(WIN->macWin, str);

		if (str[0] > inMaxSize) str[0] = inMaxSize;
		::BlockMoveData(str+1, outText, str[0]);
		return str[0];
	}
	return 0;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetRef(TWindow inWin, void *ref)
{
	Require(inWin);
	WIN->userRef = ref;
}

void *UWindow::GetRef(TWindow inWin)
{
	if (inWin) return WIN->userRef;
	return nil;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetVisible(TWindow inWin, Uint16 inVisible)
{
	Require(inWin);
	WindowRef macWin = WIN->macWin;
	
	if (inVisible == kHideWindow)
	{
		// hide window if not already hidden
		if (::IsWindowVisible(macWin))
		{
			// hide the window
			_WNSetVisible(inWin, false);
			_WNCheckWinStatesFlag = _WNCheckMouseWithinWinFlag = true;
			
			// if the window was the focus, it can't be anymore (can't have hidden window as the focus)
			if (_WNData.focusWin == WIN)
				_WNData.focusWin = nil;
			
			// set windows state to Hidden
			WIN->state = windowState_Hidden;
			if (WIN->msgProc)
				UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, WIN->msgProc, WIN->msgProcContext, inWin);
		}
	}
	else if (!::IsWindowVisible(macWin))	// if hidden, then show
	{
		// make sure there's enough memory
		if (!UMemory::IsAvailable(1024))
			Fail(errorType_Memory, memError_NotEnough);
		
		// need to check the win states
		_WNCheckWinStatesFlag = _WNCheckMouseWithinWinFlag = true;
		_WNData.focusWin = nil;		// find a new focus window
		
		if (inVisible == kShowWinAtBack)
		{
			_WNSendToBack(inWin);
			if (::IsWindowHilited(macWin)) ::HiliteWindow(macWin, false);
		}
		else	// kShowWinAtFront
		{
			_WNBringToFront(inWin);
			if (!::IsWindowHilited(macWin)) ::HiliteWindow(macWin, true);
		}
		
		_WNSetVisible(inWin, true);
	}
}

bool UWindow::IsVisible(TWindow inWin)
{
	return inWin && ::IsWindowVisible(WIN->macWin);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool UWindow::IsChild(TWindow inParent, TWindow inChild)
{
	Require(inParent && inChild);

	return ((SWindow *)inParent)->childrenList.IsInList((SWindow *)inChild);
}

bool UWindow::GetChildrenList(TWindow inParent, CPtrList<CWindow>& outChildrenList)
{
	Require(inParent);

	Uint32 i = 0;
	SWindow *pWin;
	
	while (((SWindow *)inParent)->childrenList.GetNext(pWin, i))
		outChildrenList.AddItem((CWindow *)pWin->userRef);
	
	if (!outChildrenList.GetItemCount())
		return false;
		
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetBackColor(TWindow inWin, const SColor& rgb)
{
	// set back color
	GrafPtr savePort;
	::GetPort(&savePort);
	::SetPortWindowPort(WIN->macWin);
	::RGBBackColor((RGBColor *)&rgb);
	::SetPort(savePort);
	
#if TARGET_API_MAC_CARBON
	::SetWindowContentColor((WindowPtr)WIN->macWin, (RGBColor *)&rgb);
#else
	// alloc color table
	WCTabHandle wctb = (WCTabHandle)NewHandleClear(sizeof(WinCTab));
	if (wctb == nil) Fail(errorType_Memory, memError_NotEnough);
	
	// build color table
	(**wctb).ctSize = 4;
	(**wctb).ctTable[0].value = 0;
	(**wctb).ctTable[1].value = 1;
	(**wctb).ctTable[2].value = 2;
	(**wctb).ctTable[3].value = 3;
	(**wctb).ctTable[4].value = 4;
	(**wctb).ctTable[4].rgb.red = (**wctb).ctTable[4].rgb.green = (**wctb).ctTable[4].rgb.blue = 0xFFFF;
	(**wctb).ctTable[0].rgb = *(RGBColor *)&rgb;
	
	// install color table
	::SetWinColor((WindowPtr)WIN->macWin, wctb);
#endif
}

void UWindow::GetBackColor(TWindow inWin, SColor& outColor)
{
	if (inWin)
	{
		GrafPtr savePort;
		GetPort(&savePort);
		SetPortWindowPort(WIN->macWin);
		::GetBackColor((RGBColor *)&outColor);
		SetPort(savePort);
	}
	else
		outColor.Set(0xFFFF);
}

void UWindow::SetBackPattern(TWindow inWindow, Int16 inID)
{
	#pragma unused(inWindow, inID)
	// this doesn't work with BeginDraw/EndDraw for some reason
/*
	const RGBColor whitergb = { 0xFFFF, 0xFFFF, 0xFFFF };
	PixPatHandle pat;
	
	Require(inWindow);
	
	pat = GetPixPat(inID);
	if (pat)
	{
		SetPort(inWindow->macWin);
		RGBBackColor(&whitergb);
		BackPixPat(pat);
	}*/
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetLayer(TWindow /* inWindow */, Uint16 /* inLayer */)
{
	DebugBreak("UWindow - SetLayer() is not yet implemented");
	
	/*
	Some modification of _WNFindPlaneForNewWindow could be used for this.
	I think it'd have to ignore inWindow when determining what window to
	place inWindow behind.
	*/
}

Uint16 UWindow::GetLayer(TWindow inWin)
{
	Require(inWin);
	return WIN->layer;
}
		
/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetBounds(TWindow inWin, const SRect& inBounds)
{
	Int16 newWidth, newHeight;
	bool changed = false;

	// sanity checks
	Require(inWin);
	if (!UMemory::IsAvailable(1024))
		Fail(errorType_Memory, memError_NotEnough);
	
	// get current bounds
	WindowPtr macWin = WIN->macWin;
	
	GrafPtr savePort;
	::GetPort(&savePort);
	::SetPortWindowPort(macWin);

#if TARGET_API_MAC_CARBON
	Rect old;
	::GetPortBounds(::GetWindowPort(macWin), &old);
#else	
	Rect old = macWin->portRect;
#endif
	
	::LocalToGlobal((Point *)&old);
	::LocalToGlobal(((Point *)&old)+1);
	::SetPort(savePort);
	
	// move window if location changed
	if ((old.left != inBounds.left) || (old.top != inBounds.top))
	{
		::MoveWindow(macWin, inBounds.left, inBounds.top, false);
		changed = true;
	}
	
	// resize window if height or width changed
	newWidth = inBounds.GetWidth();
	newHeight = inBounds.GetHeight();
	if ((old.right - old.left) != newWidth || (old.bottom - old.top) != newHeight)
	{
		::SizeWindow(macWin, newWidth, newHeight, false);
		_WNDrawGrowBox(inWin);
		changed = true;
	}

	// if bounds changed, update mouse-within-win and post bounds-changed message
	if (changed)
	{
		_WNCheckMouseWithinWinFlag = true;
	
		if (WIN->msgProc)
			UApplication::SendMessage(msg_BoundsChanged, nil, 0, priority_MouseDown, WIN->msgProc, WIN->msgProcContext, inWin);
	}
}

void UWindow::GetBounds(TWindow inWin, SRect& outBounds)
{
	Require(inWin);
	
	GrafPtr savePort;
	::GetPort(&savePort);
	::SetPortWindowPort(WIN->macWin);
	
#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(::GetWindowPort(WIN->macWin), &portRect);
#else	
	Rect portRect = WIN->macWin->portRect;
#endif

	::LocalToGlobal((Point *)&portRect);
	::LocalToGlobal(((Point *)&portRect) + 1);

	::SetPort(savePort);
	
	outBounds.Set(portRect.left, portRect.top, portRect.right, portRect.bottom);
}

void UWindow::SetLocation(TWindow inWin, SPoint& inTopLeft)
{
	Require(inWin);
	
#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(::GetWindowPort(WIN->macWin), &portRect);
#else	
	Rect portRect = WIN->macWin->portRect;
#endif

	SRect bounds;
	bounds.left = inTopLeft.x;
	bounds.top = inTopLeft.y;
	bounds.right = bounds.left + (portRect.right - portRect.left);
	bounds.bottom = bounds.top + (portRect.bottom - portRect.top);
	
	SetBounds(inWin, bounds);
}

void UWindow::SetLimits(TWindow inWin, Int32 minWidth, Int32 minHeight, Int32 maxWidth, Int32 maxHeight)
{
	Require(inWin);
	
	// we limit the minimums to 15 to avoid drawing problems
	WIN->minWidth = minWidth < 15 ? 15 : minWidth;
	WIN->minHeight = minHeight < 15 ? 15 : minHeight;
	
	// we limit the maximums to 30K to avoid potential Int16 integer overflows
	WIN->maxWidth = maxWidth > 30000 ? 30000 : maxWidth;
	WIN->maxHeight = maxHeight > 30000 ? 30000 : maxHeight;

	if (WIN->maxWidth < WIN->minWidth)
		WIN->maxWidth = WIN->minWidth;
		
	if (WIN->maxHeight < WIN->minHeight)
		WIN->maxHeight = WIN->minHeight;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool UWindow::GetAutoBounds(TWindow inWin, Uint16 inPos, const SRect& inEnclosure, SRect& outBounds)
{
	SRect oldBounds;
	Rect r;
	Int32 n;
	
	UWindow::GetBounds(inWin, oldBounds);

	switch (inPos)
	{
		case windowPos_Center:
			outBounds = oldBounds;
			outBounds.Center(inEnclosure);
			break;
			
		case windowPos_CenterVert:
			outBounds = oldBounds;
			outBounds.CenterVert(inEnclosure);
			break;
			
		case windowPos_CenterHoriz:
			outBounds = oldBounds;
			outBounds.CenterHoriz(inEnclosure);
			break;
			
		case windowPos_Alert:
			outBounds = oldBounds;
			outBounds.CenterHoriz(inEnclosure);
			n = outBounds.bottom - outBounds.top;
			outBounds.top = (inEnclosure.bottom + inEnclosure.top - n) / 3;
			outBounds.bottom = outBounds.top + n;
			break;
		
		case windowPos_TopLeft:
			n = oldBounds.bottom - oldBounds.top;
			outBounds.top = inEnclosure.top + 10 + _WNGetTitleBarHeight(inWin);
			outBounds.bottom = outBounds.top + n;
			n = oldBounds.right - oldBounds.left;
			outBounds.left = inEnclosure.left + 10;
			outBounds.right = outBounds.left + n;
			break;
		
		case windowPos_TopRight:
			n = oldBounds.bottom - oldBounds.top;
			outBounds.top = inEnclosure.top + 10 + _WNGetTitleBarHeight(inWin);
			outBounds.bottom = outBounds.top + n;
			n = oldBounds.right - oldBounds.left;
			outBounds.right = inEnclosure.right - 10;
			outBounds.left = outBounds.right - n;
			break;
		
		case windowPos_BottomLeft:
			n = oldBounds.bottom - oldBounds.top;
			outBounds.bottom = inEnclosure.bottom - 10;
			outBounds.top = outBounds.bottom - n;
			n = oldBounds.right - oldBounds.left;
			outBounds.left = inEnclosure.left + 10;
			outBounds.right = outBounds.left + n;
			break;
		
		case windowPos_BottomRight:
			n = oldBounds.bottom - oldBounds.top;
			outBounds.bottom = inEnclosure.bottom - 10;
			outBounds.top = outBounds.bottom - n;
			n = oldBounds.right - oldBounds.left;
			outBounds.right = inEnclosure.right - 10;
			outBounds.left = outBounds.right - n;
			break;
		
		case windowPos_Stagger:
			r.top = oldBounds.top;
			r.left = oldBounds.left;
			r.bottom = oldBounds.bottom;
			r.right = oldBounds.right;
			_WNCalcStaggeredBounds(inWin, &r);
			outBounds.Set(r.left, r.top, r.right, r.bottom);
			break;	
		
		case windowPos_Unobstructed:
			r.top = oldBounds.top;
			r.left = oldBounds.left;
			r.bottom = oldBounds.bottom;
			r.right = oldBounds.right;
			_WNCalcUnobstructedBounds(inWin, &r);
			outBounds.Set(r.left, r.top, r.right, r.bottom);
			break;
				
		default:	// windowPos_Best
			r.top = oldBounds.top;
			r.left = oldBounds.left;
			r.bottom = oldBounds.bottom;
			r.right = oldBounds.right;
			_WNCalcAdjustedBounds(inWin, &r);
			outBounds.Set(r.left, r.top, r.right, r.bottom);
			break;
	}
	
	// return whether or not bounds changed
	return (oldBounds != outBounds);
}

bool UWindow::GetAutoBounds(TWindow inWin, Uint16 inPos, Uint16 inPosOn, SRect& outBounds)
{
	SRect enc;
	Rect r;
	GDHandle gd;
	GDHandle mainDev = ::GetMainDevice();
	Int16 menuBarHeight = ::GetMBarHeight();
	
	switch (inPosOn)
	{
		case windowPosOn_WinScreen:
			if (_WNData.mainWindow)
			{
				gd = _GetWindowDevice(((SWindow *)_WNData.mainWindow)->macWin);
				r = (**gd).gdRect;
				if (gd == mainDev) r.top += menuBarHeight;
			}
			else
			{
				r = (**mainDev).gdRect;
				r.top += menuBarHeight;
			}
			break;
			
		case windowPosOn_DeepestScreen:
		#if TARGET_API_MAC_CARBON
			::GetRegionBounds(GetGrayRgn(), &r);
		#else
			r = (**GetGrayRgn()).rgnBBox;
		#endif
		
			gd = ::GetMaxDevice(&r);
			r = (**gd).gdRect;
			if (gd == mainDev) r.top += menuBarHeight;
			break;
			
		case windowPosOn_MainScreen:
			r = (**mainDev).gdRect;
			r.top += menuBarHeight;
			break;
			
		default:	// windowPosOn_SameScreen
			gd = _GetWindowDevice(WIN->macWin);
			r = (**gd).gdRect;
			if (gd == mainDev) r.top += menuBarHeight;
			break;
	}
	
	enc.Set(r.left, r.top, r.right, r.bottom);
	return GetAutoBounds(inWin, inPos, enc, outBounds);
}

void UWindow::SetMainWindow(TWindow inWindow)
{
	_WNData.mainWindow = (SWindow *)inWindow;
}

TWindow UWindow::GetMainWindow()
{
	return (TWindow)_WNData.mainWindow;
}

bool UWindow::GetZoomBounds(TWindow inWin, const SRect& inIdealSize, SRect& outBounds)
{
	Rect r, tr;
	SRect oldBounds;
	Int32 w, h;
	GetBounds(inWin, oldBounds);

	// convert inIdealSize to mac rect
	tr.top = inIdealSize.top;
	tr.left = inIdealSize.left;
	tr.bottom = inIdealSize.bottom;
	tr.right = inIdealSize.right;

	// calc best size
	_WNCalcZoomedBounds(inWin, &tr, r);
	
	// make sure it fits within the limits
	w = r.right - r.left;
	h = r.bottom - r.top;
	if (w < WIN->minWidth)		r.right = r.left + WIN->minWidth;
	if (w > WIN->maxWidth)		r.right = r.left + WIN->maxWidth;
	if (h < WIN->minHeight)		r.bottom = r.top + WIN->minHeight;
	if (h > WIN->maxHeight)		r.bottom = r.top + WIN->maxHeight;
	
	// output the bounds
	outBounds.top = r.top;
	outBounds.left = r.left;
	outBounds.bottom = r.bottom;
	outBounds.right = r.right;
	
	// return whether or not bounds changed
	return (oldBounds != outBounds);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

Uint16 UWindow::GetStyle(TWindow inWin)
{
	Require(inWin);
	return WIN->style;
}

Uint16 UWindow::GetOptions(TWindow inWin)
{
	Require(inWin);
	return WIN->flags;
}

Uint16 UWindow::GetState(TWindow inWin)
{
	Require(inWin);
	
	// it's important that we check the states here otherwise we could be returning a not-very-correct value
	_WNCheckWinStates();
	
	return WIN->state;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetMessageHandler(TWindow inWin, TMessageProc inProc, void *inContext)
{
	Require(inWin);
	
	if (WIN->msgProc)
		UApplication::FlushMessages(WIN->msgProc, WIN->msgProcContext, inWin);
	
	WIN->msgProc = inProc;
	WIN->msgProcContext = inContext;
}

void UWindow::PostMessage(TWindow inWin, Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority)
{
	Require(inWin);
	
	if (WIN->msgProc)
		UApplication::PostMessage(inMsg, inData, inDataSize, inPriority, WIN->msgProc, WIN->msgProcContext, inWin);
}

void UWindow::InstallKeyCommands(const SKeyCmd *inCmds, Uint32 inCount, TMessageProc inProc, void *inContext)
{
	TPtr p = UMemory::New(inCmds, sizeof(SKeyCmd) * inCount);
	UMemory::Dispose((TPtr)_WNData.keyCmds);
	
	_WNData.keyCmds = (SKeyCmd *)p;
	_WNData.keyCmdCount = inCount;
	_WNData.keyCmdMsgProc = inProc;
	_WNData.keyCmdMsgProcContext = inContext;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// returns nil if no front window for the specified layer
// invisible windows are not considered in the search
TWindow UWindow::GetFront(Uint16 inLayer)
{
	WindowRef macWin = ::FrontWindow();
	
	while (macWin)
	{
		SWindow *win = (SWindow *)::GetWRefCon(macWin);
		if (win && win->layer == inLayer && ::IsWindowVisible(macWin))
			return (TWindow)win;
		
		macWin = ::GetNextWindow(macWin);
	}
	
	return nil;
}

bool UWindow::IsFront(TWindow inWin)
{
	return inWin && (inWin == GetFront(WIN->layer));
}

// brings a window to the front of it's layer, and returns true if the window was brought to the front (false if already front or hidden)
bool UWindow::BringToFront(TWindow inWin, bool inActivate)
{
	#pragma unused(inActivate)

	if (inWin && ::IsWindowVisible(WIN->macWin) && _WNBringToFront(inWin))	// if was brought to the front
	{
		_WNCheckWinStatesFlag = _WNCheckMouseWithinWinFlag = true;
		_WNData.focusWin = nil;		// find a new focus window
		return true;
	}
	
	return false;
}

bool UWindow::InsertAfter(TWindow inWin, TWindow inInsertAfter, bool inActivate)
{
	#pragma unused(inWin, inInsertAfter, inActivate)

	DebugBreak("UWindow - InsertAfter() is not yet implemented");
	
	return false;
}

// sends a window to the back of it's layer, and returns true if the window was sent to the back
bool UWindow::SendToBack(TWindow inWin)
{
	if (inWin && ::IsWindowVisible(WIN->macWin) && _WNSendToBack(inWin))	// if was sent to the back
	{
		_WNCheckWinStatesFlag = true;
		_WNData.focusWin = nil;		// find a new focus window
		return true;
	}
	
	return false;
}

// returns the next visible window lower down
TWindow UWindow::GetNextVisible(TWindow inWin)
{
	WindowRef next;
	TWindow w;
	
	if (inWin)
		next = ::GetNextWindow(WIN->macWin);
	else
		next = ::FrontWindow();

	for(;;)
	{
		if (next == nil) break;
		if (IsWindowVisible(next))
		{
			w = (TWindow)::GetWRefCon(next);
			if (w) return w;
		}
		next = ::GetNextWindow(next);
	}
	return nil;
}

// does nothing if <inWin> is already has the focus
// use nil for <inWin> to remove the focus
// does nothing if <inWin> is hidden (hidden windows can't have the focus)
// does not bring <inWin> to the front or anything (yes, the focus win doesn't have to be at the front)
void UWindow::SetFocus(TWindow inWin)
{
	// important to check the states here or we might be looking at no-longer-valid states
	_WNCheckWinStates();
	
	// find which window was previously the focus
	SWindow *oldFocus = (SWindow *)_WNData.focusWin;
	
	if (oldFocus != WIN)	// if change
	{
		// disallow changing to focus to a hidden window
		if (inWin && WIN->state == windowState_Hidden) 
			return;
	
		// remove the focus from the old window (if any)
		if (oldFocus)
		{
			// mark no window as the focus
			_WNData.focusWin = nil;
			
			// if the win was in the Focus state, it is no more (the state may be Inactive if the app is in the bkgnd)
			if (oldFocus->state == windowState_Focus)
			{
				oldFocus->state = windowState_Active;
				if (oldFocus->msgProc)	// we don't include the new state with the message as it may have changed by the time the message is processed
					UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, oldFocus->msgProc, oldFocus->msgProcContext, oldFocus);
			}
			
			// unhilite the win except if it's a floater or popup
			if (::IsWindowHilited(oldFocus->macWin) && (oldFocus->layer != windowLayer_Float) && (oldFocus->layer != windowLayer_Popup))
				::HiliteWindow(oldFocus->macWin, false);
		}
		
		// give the focus to the new window
		if (inWin)
		{
			// store ptr to the new focus win
			_WNData.focusWin = WIN;
			
			// if the win is in the Active state, upgrade it to the Focus state
			if (WIN->state == windowState_Active)
			{
				WIN->state = windowState_Focus;
				if (WIN->msgProc)
					UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, WIN->msgProc, WIN->msgProcContext, inWin);
			}
		}
	}
}

// returns nil if no window has the focus
TWindow UWindow::GetFocus()
{
	_WNCheckWinStates();
	
	return (_WNData.focusWin && ((SWindow *)_WNData.focusWin)->state == windowState_Focus) ? (TWindow)_WNData.focusWin : nil;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetMouseCaptureWindow(TWindow inWin)
{
	// remove capture from prev windows
	if (_WNData.mouseCaptureWin && _WNData.mouseCaptureWin->msgProc)
		UApplication::SendMessage(msg_CaptureReleased, nil, 0, priority_High, _WNData.mouseCaptureWin->msgProc, _WNData.mouseCaptureWin->msgProcContext, _WNData.mouseCaptureWin);

#if 0
	if (_WNData.mouseWithinWin)
	{
		SMouseMsgData info;
		
		// set info
		info.time = UDateTime::GetMilliseconds();
		info.button = 0;
		UMouse::GetLocation(info.loc);
		UWindow::GlobalToLocal((TWindow)_WNData.mouseWithinWin, info.loc);

		info.mods = 0;
		info.misc = 0;

		if (WIN == _WNData.mouseWithinWin)
		{
			// send message		
			if (_WNData.mouseWithinWin->msgProc)
				UApplication::SendMessage(msg_MouseMove, &info, sizeof(info), priority_MouseLeave, _WNData.mouseWithinWin->msgProc, _WNData.mouseWithinWin->msgProcContext, _WNData.mouseWithinWin);
		}
		else
		{
			// some sort of updating needs to be done
			// send a mouseleave to _WNData.mouseWithinWin
			
			// send message		
			if (_WNData.mouseWithinWin->msgProc)
				UApplication::SendMessage(msg_MouseLeave, &info, sizeof(info), priority_MouseLeave, _WNData.mouseWithinWin->msgProc, _WNData.mouseWithinWin->msgProcContext, _WNData.mouseWithinWin);
		
			_WNData.mouseWithinWin = nil;
		}
	}
#endif

	_WNData.mouseCaptureWin = WIN;
}

void UWindow::ReleaseMouseCaptureWindow(TWindow inWin)
{
	// if inWin is not the captured win, this should not affect the currenet capture.
	if (WIN == _WNData.mouseCaptureWin)
	{
		if (_WNData.mouseCaptureWin && _WNData.mouseCaptureWin->msgProc)
			UApplication::SendMessage(msg_CaptureReleased, nil, 0, priority_High, _WNData.mouseCaptureWin->msgProc, _WNData.mouseCaptureWin->msgProcContext, _WNData.mouseCaptureWin);

		_WNData.mouseCaptureWin = nil;
		// if the mouse is in another window, call mouse enter
		// else, if is in the same window, call mouse move
		// or make life simple and dispatch mouse
		SMouseMsgData info;
		
		// set info
		info.time = UDateTime::GetMilliseconds();
		info.button = 0;
		UMouse::GetLocation(info.loc);
		info.mods = 0;
		info.misc = 0;
		
		//_WNDispatchMouse(msg_MouseMove, info);
	}
	else
	{
#if DEBUGGING_MOUSECAPTURE		
		DebugBreak("Trying to release capture for win not captured");
#endif
	}
}

TWindow UWindow::GetMouseCaptureWindow()
{
	return (TWindow)_WNData.mouseCaptureWin;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// returns true if there are any visible modal windows
bool UWindow::InModal()
{
	return _WNData.visModalCount != 0;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::Collapse(TWindow inWin)
{
	Require(inWin);
	
	// in order to call CollapseWindow we need Mac OS 8.0 or later
	if (_GetSystemVersion() < 0x0800)
		return;

	if (::CollapseWindow(WIN->macWin, true) == noErr)
		_WNCheckCollapseState(inWin);
}

void UWindow::Expand(TWindow inWin)
{
	Require(inWin);
	
	// in order to call CollapseWindow we need Mac OS 8.0 or later
	if (_GetSystemVersion() < 0x0800)
		return;

	if (::CollapseWindow(WIN->macWin, false) == noErr)
		_WNCheckCollapseState(inWin);
}

bool UWindow::IsCollapsed(TWindow inWin)
{
	Require(inWin);

	// in order to call IsWindowCollapsed we need Mac OS 8.0 or later
	if (_GetSystemVersion() < 0x0800)
		return false;

	return ::IsWindowCollapsed(WIN->macWin);
}

bool UWindow::IsZoomed(TWindow inWin)
{
	Require(inWin);

	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::GlobalToLocal(TWindow inWin, SPoint& ioPoint)
{
	Require(inWin);
	Point pt;
	pt.h = ioPoint.h;
	pt.v = ioPoint.v;

	SetPortWindowPort(WIN->macWin);
	::GlobalToLocal(&pt);
	
	ioPoint.h = pt.h;
	ioPoint.v = pt.v;
}

void UWindow::GlobalToLocal(TWindow inWin, SRect& ioRect)
{
	Require(inWin);
	Point tl = { ioRect.top, ioRect.left };
	Point br = { ioRect.bottom, ioRect.right };
	
	SetPortWindowPort(WIN->macWin);
	::GlobalToLocal(&tl);
	::GlobalToLocal(&br);
	
	ioRect.top = tl.v;
	ioRect.left = tl.h;
	ioRect.bottom = br.v;
	ioRect.right = br.h;
}

void UWindow::LocalToGlobal(TWindow inWin, SPoint& ioPoint)
{
	Require(inWin);
	Point pt;
	pt.h = ioPoint.h;
	pt.v = ioPoint.v;

	SetPortWindowPort(WIN->macWin);
	::LocalToGlobal(&pt);
	
	ioPoint.h = pt.h;
	ioPoint.v = pt.v;
}

void UWindow::LocalToGlobal(TWindow inWin, SRect& ioRect)
{
	Require(inWin);
	Point tl = { ioRect.top, ioRect.left };
	Point br = { ioRect.bottom, ioRect.right };
	
	SetPortWindowPort(WIN->macWin);
	::LocalToGlobal(&tl);
	::LocalToGlobal(&br);
	
	ioRect.top = tl.v;
	ioRect.left = tl.h;
	ioRect.bottom = br.v;
	ioRect.right = br.h;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// returns false if nothing to draw.  Should be called in a loop (there may be more than one rect to draw)
bool UWindow::GetRedrawRect(TWindow inWin, SRect& outRect)
{
	if (inWin && WIN->needsRedraw)
	{
		WIN->needsRedraw = false;
		
		if (!::IsWindowVisible(WIN->macWin)) 
			return false;
		
		outRect = WIN->redrawRect;
		return true;
	}
	
	return false;
}

void UWindow::Refresh(TWindow inWin)
{
	if (inWin)
	{
	#if TARGET_API_MAC_CARBON
		Rect portRect;
		::GetPortBounds(::GetWindowPort(WIN->macWin), &portRect);
	#else	
		Rect portRect = WIN->macWin->portRect;
	#endif
		
		SRect r;
		r.top = r.left = 0;
		r.bottom = portRect.bottom - portRect.top;
		r.right = portRect.right - portRect.left;
		
		Refresh(inWin, r);
	}
}

void UWindow::Refresh(TWindow inWin, const SRect& inRect)
{
	if (inWin && ::IsWindowVisible(WIN->macWin) && !inRect.IsEmpty())
	{
		try
		{
			if (WIN->needsRedraw)
			{
				WIN->redrawRect |= inRect;
			}
			else
			{
				WIN->needsRedraw = true;
				WIN->redrawRect = inRect;
				if (WIN->msgProc)
					UApplication::ReplaceMessage(msg_Draw, nil, 0, priority_Draw, WIN->msgProc, WIN->msgProcContext, inWin);
			}
		}
		catch(...)
		{
			DebugBreak("UWindow - unable to refresh");
			// don't throw
		}
	}
}

bool UWindow::IsColor(TWindow inWin)
{
	if (inWin)
	{
		GDHandle gd = _GetWindowDevice(WIN->macWin);
		if (gd) return ((**(**gd).gdPMap).pixelSize >= 4);
	}
	return false;
}

Uint16 UWindow::GetDepth(TWindow inWin)
{
	if (inWin)
	{
		GDHandle gd = _GetWindowDevice(WIN->macWin);
		if (gd) return (**(**gd).gdPMap).pixelSize;
	}
	return 1;
}

extern CGrafPtr _gStartupPort;
extern GDHandle _gStartupDevice;

#define DRAW_OFFSCREEN		1

TImage UWindow::BeginDraw(TWindow inWin, const SRect& inUpdateRect)
{
	Require(inWin);
		
#if DRAW_OFFSCREEN

	if (WIN->gworld)
	{
		DebugBreak("UWindow - BeginDraw() has already been called without matching EndDraw()");
		Fail(errorType_Misc, error_Protocol);
	}

	GWorldPtr gw = nil;
	RGBColor backColor;
	CGrafPtr winPort;
	Rect updateRect, globalUpdateRect, screenRect;
	OSErr err;
		
	// get settings from windows port
	winPort = ::GetWindowPort(WIN->macWin);

#if TARGET_API_MAC_CARBON
	::SetPortWindowPort(::GetWindowFromPort(winPort));
#else
	::SetPortWindowPort((GrafPtr)winPort);
#endif

	::GetBackColor(&backColor);
	
	// convert inUpdateRect to mac rect
	updateRect.top = inUpdateRect.top;
	updateRect.left = inUpdateRect.left;
	updateRect.bottom = inUpdateRect.bottom;
	updateRect.right = inUpdateRect.right;
	
	// return dummy image if update rect not on screen
	globalUpdateRect = updateRect;
	::LocalToGlobal(&topLeft(globalUpdateRect));
	::LocalToGlobal(&botRight(globalUpdateRect));

#if TARGET_API_MAC_CARBON
	::GetRegionBounds(GetGrayRgn(), &screenRect);
#else
	screenRect = (**GetGrayRgn()).rgnBBox;
#endif

	if (!SectRect(&globalUpdateRect, &screenRect, &globalUpdateRect))
		return UGraphics::GetDummyImage();
	
	updateRect = globalUpdateRect;
	//OffsetRect(&updateRect, -inWindow->bounds.left, -inWindow->bounds.top);
	::GlobalToLocal((Point *)&updateRect);
	::GlobalToLocal(((Point *)&updateRect)+1);
	
	// create new offscreen graphics world
	err = ::NewGWorld(&gw, 0, &globalUpdateRect, nil, nil, noNewDevice | useTempMem);
	if (err == paramErr)	// if update-rect is entirely not on any display
		return UGraphics::GetDummyImage();
	
	if (gw == nil)
		err = ::NewGWorld(&gw, 0, &globalUpdateRect, nil, nil, noNewDevice);
	
	CheckMacError(err);

	WIN->gworld = gw;
	WIN->updateRect = updateRect;
		
	try
	{
		// set gworld settings to be like windows port
		if (!LockPixels(GetGWorldPixMap(gw)))
			Fail(errorType_Misc, error_Unknown);
		
		::SetGWorld(gw, nil);		// DO NOT USE SetPort() FOR A GWORLD!!!
		RGBBackColor(&backColor);
		CheckMacError(QDError());
		::SetOrigin(updateRect.left, updateRect.top);
		
		// set gworld back pat to windows pat
		/*if (winPort->bkPixPat)
		{
			PixPatHandle ppat = ::NewPixPat();
			if (ppat == nil) Fail(errorType_Memory, memError_NotEnough);
			try
			{
				::CopyPixPat(winPort->bkPixPat, ppat);
				//CheckMacError(QDError());      reports nilHandleErr but works anyway????
				::BackPixPat(ppat);
			}
			catch(...)
			{
				::DisposePixPat(ppat);
				throw;
			}
		}*/
		
		// 	doing the above causes memory leak, this appears to work
		//	if (winPort->bkPixPat) ::BackPixPat(winPort->bkPixPat);
		// 	.... and this causes problems on MacOS 8
		
		// erase offscreen image
	#if TARGET_API_MAC_CARBON
		Rect portRect;
		::GetPortBounds(gw, &portRect);
	#else	
		Rect portRect = gw->portRect;
	#endif
		
		::EraseRect(&portRect);
	}
	catch(...)
	{
		// clean up
		::SetGWorld(_gStartupPort, _gStartupDevice);
		WIN->gworld = nil;
		::DisposeGWorld(gw);
		throw;
	}
	
	WIN->img = _NewGWImage(gw);
	return WIN->img;

#else

	Rect updateRect = { inUpdateRect.top, inUpdateRect.left, inUpdateRect.bottom, inUpdateRect.right };
	
	CGrafPtr winPort = ::GetWindowPort(WIN->macWin);
	::SetPortWindowPort((GrafPtr)winPort);
	::PenNormal();
	::SetOrigin(0,0);
	
	::ClipRect(&updateRect);
	::EraseRect(&updateRect);
	
	WIN->img = _NewGWImage((GWorldPtr)winPort);
	return WIN->img;
	
#endif
}

void UWindow::EndDraw(TWindow inWin)
{
	if (inWin) 
	{	
		if (WIN->img)
		{
			_DisposeGWImage(WIN->img);
			WIN->img = NULL;
		}

#if DRAW_OFFSCREEN

		GWorldPtr gw = WIN->gworld;
		WindowRef macWin = WIN->macWin;
		if (gw == nil) 
			return;

		if (::IsWindowVisible(macWin))
		{
			const RGBColor whiteRGB = { 0xFFFF, 0xFFFF, 0xFFFF };
			const RGBColor blackRGB = { 0x0000, 0x0000, 0x0000 };
			RGBColor saveBack;

			::SetGWorld(gw, nil);	// do not use SetPort()
			::SetOrigin(0,0);
			::RGBForeColor(&blackRGB);
			::RGBBackColor(&whiteRGB);

			GrafPtr winPort = (GrafPtr)::GetWindowPort(macWin);
			::SetPort(winPort);
			::SetOrigin(0,0);
		
			::GetBackColor(&saveBack);
			::RGBForeColor(&blackRGB);
			::RGBBackColor(&whiteRGB);
		
			Rect updateRect = WIN->updateRect;
		#if TARGET_API_MAC_CARBON
			const BitMap *bmp = ::GetPortBitMapForCopyBits(gw);
		#else	
			const BitMap *bmp = &((GrafPtr)gw)->portBits;
		#endif
				
			if (WIN->flags & windowOption_Sizeable)
			{
				Rect content, growRect, r;
				RgnHandle growRgn, clipRgn;
		
			#if TARGET_API_MAC_CARBON
				RgnHandle contRgn = ::NewRgn();
				
				::GetWindowRegion(macWin, kWindowContentRgn, contRgn);
				::GetRegionBounds(contRgn, &content);
				
				::DisposeRgn(contRgn);
			#else	
				content = (**((WindowPeek)macWin)->contRgn).rgnBBox;
			#endif
			
				growRect.bottom = content.bottom - content.top;
				growRect.right = content.right - content.left;
				growRect.top = growRect.bottom - 15;
				growRect.left = growRect.right - 15;
			
				if (::SectRect(&growRect, &updateRect, &r))
				{
					growRgn = ::NewRgn();
					clipRgn = ::NewRgn();
				
					if (growRgn == nil || clipRgn == nil)
					{
						if (growRgn) ::DisposeRgn(growRgn);
						if (clipRgn) ::DisposeRgn(clipRgn);
					}
					else
					{
						::RectRgn(growRgn, &growRect);
						
						::OffsetRect(&content, -content.left, -content.top);
						::RectRgn(clipRgn, &content);
					
						::DiffRgn(clipRgn, growRgn, clipRgn);
						::DisposeRgn(growRgn);
					
					#if TARGET_API_MAC_CARBON
						Rect portRect;
						::GetPortBounds(gw, &portRect);
	
						::CopyBits(bmp, ::GetPortBitMapForCopyBits(winPort), &portRect, &updateRect, srcCopy, clipRgn);
					#else	
						::CopyBits(bmp, &winPort->portBits, &gw->portRect, &updateRect, srcCopy, clipRgn);
					#endif
					
						::DisposeRgn(clipRgn);
					
						_WNDrawGrowBox(inWin);
					}
				}
				else
				{
				#if TARGET_API_MAC_CARBON
					Rect portRect;
					::GetPortBounds(gw, &portRect);

					RgnHandle visRgn = ::NewRgn();
					::GetPortVisibleRegion(winPort, visRgn);
	
					::CopyBits(bmp, ::GetPortBitMapForCopyBits(winPort), &portRect, &updateRect, srcCopy, visRgn);
				
					::DisposeRgn(visRgn);
				#else
					::CopyBits(bmp, &winPort->portBits, &gw->portRect, &updateRect, srcCopy, winPort->visRgn);
				#endif
				}
			}
			else
			{
			#if TARGET_API_MAC_CARBON
				Rect portRect;
				::GetPortBounds(gw, &portRect);
			
				RgnHandle visRgn = ::NewRgn();
				::GetPortVisibleRegion(winPort, visRgn);
					
				::CopyBits(bmp, ::GetPortBitMapForCopyBits(winPort), &portRect, &updateRect, srcCopy, visRgn);
			
				::DisposeRgn(visRgn);
			#else
				::CopyBits(bmp, &winPort->portBits, &gw->portRect, &updateRect, srcCopy, winPort->visRgn);
			#endif
			}
			
			::RGBBackColor(&saveBack);
		}
	
		::SetGWorld(_gStartupPort, _gStartupDevice);	// ESSENTIAL
		WIN->gworld = nil;
		::DisposeGWorld(gw);

#else
	
		// nothing to do here

#endif
	}
}

void UWindow::AbortDraw(TWindow inWin)
{
	if (inWin)
	{
		if (WIN->img)
		{
			_DisposeGWImage(WIN->img);
			WIN->img = NULL;
		}
		
		GWorldPtr gw = WIN->gworld;
		if (gw)
		{
			::SetGWorld(_gStartupPort, _gStartupDevice);
			WIN->gworld = nil;
			::DisposeGWorld(gw);
		}
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static void _WNCheckWinStates()
{
	WindowRef macWin;
	SWindow *win;
	
	if (_WNCheckWinStatesFlag)	// if window states need to be checked
	{
		_WNCheckWinStatesFlag = false;

		// get the first/frontmost window
		macWin = ::FrontWindow();

		// if app is in background, all windows should be hidden or inactive
		if (_APInBkgndFlag)
		{
			if (_WNData.focusWin)
				_WNData.focusWin = nil;
			
			// loop thru all the windows and adjust as necessary
			while (macWin)
			{
				// find our extra window data and ignore if not found
				win = (SWindow *)::GetWRefCon(macWin);
				if (win)
				{
					// if the window is visible, the correct state is Inactive, otherwise Hidden
					if (::IsWindowVisible(macWin))
					{
						// remove the hilite from the titlebar
						if (::IsWindowHilited(macWin))
							::HiliteWindow(macWin, false);
						
						// if necessary, change the state of the window to Inactive
						if (win->state != windowState_Inactive)
						{
							win->state = windowState_Inactive;
							if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
						}
					}
					else	// window is not visible
					{
						// if necessary, change the state of the window to Hidden
						if (win->state != windowState_Hidden)
						{
							win->state = windowState_Hidden;
							if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
						}
					}
					
					_WNCheckCollapseState((TWindow)win);
				}
				
				// get the next window
				macWin = ::GetNextWindow(macWin);
			}
		}
		else	// app is in the foreground
		{
			SWindow *focusWin = _WNData.focusWin;
				
			// if no focus win, use topmost modal window, or if no modal windows, topmost standard window
			if (focusWin == nil)
				focusWin = _WNData.focusWin = (SWindow *)UWindow::GetFront(_WNData.visModalCount ? windowLayer_Modal : windowLayer_Standard);
						
			if (_WNData.visModalCount)	// if have modal windows
			{
				bool newHilite;
				Uint8 newState;
				Uint8 layer = 0;
				Uint8 prevLayer;

				while (macWin)
				{
					// find our extra window data and ignore if not found
					win = (SWindow *)::GetWRefCon(macWin);
					if (win)
					{
						if (::IsWindowVisible(macWin))	// if the window is visible
						{
							// get the layer for the window and remember the previous layer
							prevLayer = layer;
							layer = win->layer;

							if (prevLayer == layer)	// window is NOT front of its layer
								newState = (layer == windowLayer_Popup) ? windowState_Active : windowState_Inactive;
							else					// window is the front of its layer
								newState = (layer == windowLayer_Popup || layer == windowLayer_Modal) ? windowState_Active : windowState_Inactive;
							
							// determine hilite status for window
							if (win == focusWin && newState == windowState_Active)
							{
								newState = windowState_Focus;	// upgrade the focusWin from Active to Focus
								newHilite = true;
							}
							else
							{
								newHilite = (layer == windowLayer_Popup);
							}
							
							// set the window's hilite correctly
							if (newHilite)
							{
								if (!::IsWindowHilited(macWin)) 
									::HiliteWindow(macWin, true);
							}
							else
							{
								if (::IsWindowHilited(macWin)) 
									::HiliteWindow(macWin, false);
							}
						}
						else	// window is not visible
						{
							// a hidden windows state is always Hidden
							newState = windowState_Hidden;
						}
						
						// if necessary, change the state of the window
						if (win->state != newState)
						{
							win->state = newState;
							if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
						}
					}
					
					// get the next window
					macWin = ::GetNextWindow(macWin);
				}
			}
			else	// no modal windows
			{
				while (macWin)
				{
					// find our extra window data and ignore if not found
					win = (SWindow *)::GetWRefCon(macWin);
					if (win)
					{
						if (::IsWindowVisible(macWin))	// if the window is visible
						{
							// determine new state and hilite
							if (win == focusWin)		// if the window is the focus
							{
								// if necessary, change state to Focus
								if (win->state != windowState_Focus)
								{
									win->state = windowState_Focus;
									if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
								}

								// if necessary, hilite the window
								if (!::IsWindowHilited(macWin)) 
									::HiliteWindow(macWin, true);
							}
							else	// window is not the focus
							{
								// if necessary, change state to Active
								if (win->state != windowState_Active)
								{
									win->state = windowState_Active;
									if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
								}

								// adjust the windows hilite
								if (win->layer == windowLayer_Popup || win->layer == windowLayer_Float)
								{
									if (!::IsWindowHilited(macWin)) 
										::HiliteWindow(macWin, true);
								}
								else
								{
									if (::IsWindowHilited(macWin)) 
										::HiliteWindow(macWin, false);
								}
							}
						}
						else	// window is not visible
						{
							// a hidden windows state is always Hidden
							if (win->state != windowState_Hidden)
							{
								win->state = windowState_Hidden;
								if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
							}
						}
						
						_WNCheckCollapseState((TWindow)win);
					}
					
					// get the next window
					macWin = ::GetNextWindow(macWin);
				}
			}
		}
	}
	
	if (_WNCheckMouseWithinWinFlag)	// if mouse-within-win needs to be checked
	{
		_WNCheckMouseWithinWinFlag = false;
		
		Point mouseLoc;
		SMouseMsgData info;
		
		::GetMouse(&mouseLoc);
		::LocalToGlobal(&mouseLoc);
		
		if (::FindWindow(mouseLoc, &macWin) == inContent)
			win = (SWindow *)::GetWRefCon(macWin);
		else
			win = nil;
		
		if ((SWindow *)_WNData.mouseWithinWin != win)
		{
			info.loc.h = mouseLoc.h;
			info.loc.v = mouseLoc.v;
			info.button = 0;
			info.mods = UKeyboard::GetModifiers();
			info.misc = 0;
		
			if (_WNData.mouseWithinWin)
			{
				if (_WNData.mouseWithinWin->msgProc)
					UApplication::PostMessage(msg_MouseLeave, &info, sizeof(info), priority_MouseLeave, _WNData.mouseWithinWin->msgProc, _WNData.mouseWithinWin->msgProcContext, _WNData.mouseWithinWin);

				_WNData.mouseWithinWin = nil;
			}
			
			if (win && (win->state == windowState_Active || win->state == windowState_Focus))
			{
				_WNData.mouseWithinWin = win;
				
				if (_WNData.mouseWithinWin->msgProc)
				{
					UWindow::GlobalToLocal((TWindow)_WNData.mouseWithinWin, info.loc);
					UApplication::PostMessage(msg_MouseEnter, &info, sizeof(info), priority_MouseEnter, _WNData.mouseWithinWin->msgProc, _WNData.mouseWithinWin->msgProcContext, _WNData.mouseWithinWin);
				}
			}
		}
	}
}

void _WNCheckCollapseState(TWindow inWin)
{
	// in order to call IsWindowCollapsed we need Mac OS 8.0 or later
	if (_GetSystemVersion() < 0x0800)
		return;

	if (::IsWindowCollapsed(WIN->macWin))
	{
		if (!WIN->isCollapsed)
		{
			WIN->isCollapsed = true;
			if (WIN->msgProc) 
				UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, WIN->msgProc, WIN->msgProcContext, inWin);
		}
	}
	else if (WIN->isCollapsed)
	{
		WIN->isCollapsed = false;
		if (WIN->msgProc) 
			UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, WIN->msgProc, WIN->msgProcContext, inWin);
	}
}

// returns the window that a new window of the specified layer should be placed behind
static WindowRef _WNFindPlaneForNewWindow(Uint16 layer)
{
	// if modal, place in front of all others
	if (layer == windowLayer_Modal) 
		return (WindowRef)-1;
	
	WindowRef macWin = ::FrontWindow(), prevWin = nil;
	if (macWin == nil) 
		return (WindowRef)-1;	// no windows

	// we want to place the new window in front of the topmost window of the same layer	
	for(;;)
	{
		// check if win is of the same layer or lower down
		if (IsWindowVisible(macWin))			// ignore invisible windows
		{
			SWindow *win = (SWindow *)::GetWRefCon(macWin);
			if (win != nil && win->layer >= layer)
			{
				if (prevWin)
					return prevWin;				// place behind the previous window
				else
					return (WindowRef)-1;		// place in front
			}
			
			prevWin = macWin;
		}
		
		// go down a window
		macWin = GetNextWindow(macWin);
		if (macWin == nil) 
			return nil;			// place at very back
	}
}

// returns the WDEF proc to use
static Int16 _WNStyleToProcNum(Uint16 layer, Uint16 flags, Uint16 style)
{
	Int16 wdefProc = 0;

	switch (layer)
	{
		case windowLayer_Popup:
			if (style == 1)		// tooltip
				wdefProc = plainDBox;
			else
				wdefProc = plainDBox;
			break;
				
		case windowLayer_Modal:
			if (style == 1)
			#if TARGET_API_MAC_CARBON
				wdefProc = dBoxProc;
			#else
				wdefProc = plainDBox;
			#endif
			else if (style == 2)
				wdefProc = dBoxProc;
			else if (flags & windowOption_ZoomBox)
				wdefProc = 13;
			else
				wdefProc = movableDBoxProc;
			break;
			
		case windowLayer_Float:
		#if TARGET_API_MAC_CARBON
			wdefProc = noGrowDocProc;
			if (flags & windowOption_Sizeable)	wdefProc = documentProc;
			if (flags & windowOption_ZoomBox)	wdefProc = zoomNoGrow;
			if ((flags & windowOption_Sizeable) && (flags & windowOption_ZoomBox))
				wdefProc = zoomDocProc;
		#else
			wdefProc = floatProc;
			if (flags & windowOption_Sizeable)	wdefProc = floatGrowProc;
			if (flags & windowOption_ZoomBox)	wdefProc = floatZoomProc;
			if ((flags & windowOption_Sizeable) && (flags & windowOption_ZoomBox))
				wdefProc = floatZoomGrowProc;
			
			if (style == 3) wdefProc += 8;
		#endif
			break;
			
		default:	// windowLayer_Standard
			wdefProc = noGrowDocProc;
			if (flags & windowOption_Sizeable)	wdefProc = documentProc;
			if (flags & windowOption_ZoomBox)	wdefProc = zoomNoGrow;
			if ((flags & windowOption_Sizeable) && (flags & windowOption_ZoomBox))
				wdefProc = zoomDocProc;
			break;
	};
	
	return wdefProc;
}

static void _WNSendBehind(WindowRef macWindow, WindowRef behindWindow)
{
	ASSERT(macWindow);
	ASSERT(behindWindow);
	
	if (::IsWindowVisible(macWindow))
	{
		RgnHandle	badRgn;		// Originally visible window parts AND newly exposed window parts
		GrafPtr		savePort;	// Current port
		Point		corner;		// Top left of visible region

		::GetPort(&savePort);
		::SetPortWindowPort(macWindow);

		// Save portion of window which is originally visible
		badRgn = ::NewRgn();
	
	#if TARGET_API_MAC_CARBON
		::GetPortVisibleRegion(::GetWindowPort(macWindow), badRgn);
	#else
		::CopyRgn(macWindow->visRgn, badRgn);
	#endif
		
		// Adjust the window's plane
		::SendBehind(macWindow, behindWindow);

		// We must draw the newly exposed portion of the window.  Find the
		// difference between the present structure region and what was
		// originally visible.  Before doing this, we must convert the
		// originally visible region to global coords.

	#if TARGET_API_MAC_CARBON
		Rect badRgnRect;
		::GetRegionBounds(badRgn, &badRgnRect);
	#else
		Rect badRgnRect = (**badRgn).rgnBBox;
	#endif
	
		corner = topLeft(badRgnRect);
		::LocalToGlobal(&corner);
		::OffsetRgn(badRgn, corner.h - badRgnRect.left, corner.v - badRgnRect.top);
	
	#if TARGET_API_MAC_CARBON
		RgnHandle strucRgn = ::NewRgn();		
		::GetWindowRegion(macWindow, kWindowStructureRgn, strucRgn);
		
		::DiffRgn(strucRgn, badRgn, badRgn);		
	#else	
		::DiffRgn(((WindowPeek)macWindow)->strucRgn, badRgn, badRgn);
	#endif

		// Draw newly exposed region
		::PaintOne(macWindow, badRgn);

		// Adjust the visible regions of this window and those behind it.
	#if TARGET_API_MAC_CARBON
		::CalcVisBehind(macWindow, strucRgn);
		
		::DisposeRgn(strucRgn);
	#else
		::CalcVisBehind(macWindow, ((WindowPeek)macWindow)->strucRgn);
	#endif
	
		::SetPort(savePort);
		::DisposeRgn(badRgn);
	}
	else
		::SendBehind(macWindow, behindWindow);
}

// moves the specified window to the back of its layer and returns true if the window was sent to the back
// doesn't adjust hiliting or anything
static bool _WNSendToBack(TWindow inWin)
{	
	WindowRef macWin = WIN->macWin;
	Uint8 layer = WIN->layer;
	WindowRef curMacWin = ::FrontWindow(), prevMacWin = nil;
	
	// we want to send the window behind the bottommost window of the same layer
	if (curMacWin == nil) 
		return false;
	
	for(;;)
	{
		if (::IsWindowVisible(curMacWin))		// ignore invisible windows
		{
			SWindow *curWin = (SWindow *)::GetWRefCon(curMacWin);
			if (curWin && (curWin->layer > layer))
			{
				if (prevMacWin)
				{
					_WNSendBehind(macWin, prevMacWin);
					return true;
				}
				
				return false;
			}
			
			prevMacWin = curMacWin;
		}
		
		// go down a window
		curMacWin = ::GetNextWindow(curMacWin);
		if (curMacWin == nil)
		{
			if (prevMacWin)
			{
				_WNSendBehind(macWin, prevMacWin);
				return true;
			}
			
			return false;
		}
	}
	
	return false;
}

/*
 * Bring the specified window to the front of its layer (doesn't adjust hiliting).
 * Returns true if the window was brought to the front (false if already front).
 */
static bool _WNBringToFront(TWindow inWin)
{
	WindowRef macWin = WIN->macWin;
	Uint8 layer = WIN->layer;
	WindowRef curWin = ::FrontWindow(), prevWin = nil;

	// if modal, place in front of all others
	if (layer == windowLayer_Modal)
	{
		if (curWin == macWin)
			return false;
		
		::BringToFront(macWin);
		return true;
	}
	
	// we want to place the window in front of the topmost window of the same layer
	if (curWin == nil || curWin == macWin) 
		return false;	// no windows or already in front
	
	for (;;)
	{
		// check if win is of the same layer or lower down
		if (::IsWindowVisible(curWin))					// ignore invisible windows
		{
			SWindow *win = (SWindow *)::GetWRefCon(curWin);
			if (win != nil && win->layer >= layer)
			{
				if (prevWin)
					_WNSendBehind(macWin, prevWin);	// place behind the previous window
				else
					::BringToFront(macWin);			// no previous, so must be going to the top			
					
				return true;
			}
			
			prevWin = curWin;
		}
		
		// go down a window
		curWin = ::GetNextWindow(curWin);
		if (curWin == nil || curWin == macWin) 
			return false;
	}
	
	return false;
}

// like UWindow::GetFront(), but includes invisible windows in the search
static WindowRef _WNGetFirstOfLayer(Int16 layer)
{
	WindowRef macWin = ::FrontWindow();
	SWindow *win;
	
	for(;;)
	{
		// get our info
		if (macWin == nil) return nil;
		win = (SWindow *)::GetWRefCon(macWin);
		if (win == nil) goto nextWin;		// not one of our windows
		
		// check if front of specified layer
		if (win->layer == layer)
			return macWin;
		
		// get next window
	nextWin:
		macWin = ::GetNextWindow(macWin);
	}
	
	return nil;
}

// stops a window from being redrawn at the next update event
static void _WNClearUpdate(WindowRef win)
{
#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(::GetWindowPort(win), &portRect);

	::ValidWindowRect(win, &portRect);
#else
	Rect portRect = ::GetWindowPort(win)->portRect;

	GrafPtr savePort;
	::GetPort(&savePort);
	
	::SetPortWindowPort(win);
	::ValidRect(&portRect);
	
	::SetPort(savePort);
#endif
}

// doesn't adjust hiliting/active
static void _WNSetVisible(TWindow inWin, bool inVis)
{
	WindowRef macWin = WIN->macWin;
	
	if (inVis)
	{
		if (!::IsWindowVisible(macWin))
		{
			::ShowHide(macWin, true);
			
			// increment modal/popup count
			switch (WIN->layer)
			{
				case windowLayer_Modal:
					_WNData.visModalCount++;
					break;
				case windowLayer_Popup:
					_WNData.visPopupCount++;
					break;
			}
		}
	}
	else
	{
		if (::IsWindowVisible(macWin))
		{
			// hide the window
			::HideWindow(macWin);
			WIN->needsRedraw = false;
			
			// decrement modal/popup count
			switch (WIN->layer)
			{
				case windowLayer_Modal:
					_WNData.visModalCount--;
					break;
				case windowLayer_Popup:
					_WNData.visPopupCount--;
					break;
			}
		}
	}
}

static void _WNDrawGrowBox(TWindow inWin)
{
	RgnHandle saveClip, clip;
	GrafPtr savePort;
	Rect r;
	WindowRef macWin = WIN->macWin;
	
	// only sizeable windows have a grow box
	if (!(WIN->flags & windowOption_Sizeable))
		return;
		
	if (!::IsWindowVisible(macWin))
		return;
	
	// save port and clipping
	::GetPort(&savePort);
	::SetPortWindowPort(macWin);
	saveClip = ::NewRgn();
	::GetClip(saveClip);
	clip = ::NewRgn();
	
	// set clip to grow box
	::SetOrigin(0,0);

#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(::GetWindowPort(macWin), &portRect);
#else
	Rect portRect = macWin->portRect;
#endif

	r.right = portRect.right - portRect.left;
	r.bottom = portRect.bottom - portRect.top;
	r.left = r.right-15;
	r.top = r.bottom-15;
	::RectRgn(clip, &r);
	
	// draw grox box
	::SetClip(clip);
	::DrawGrowIcon(macWin);
	
	// restore port and clipping
	::SetClip(saveClip);
	::DisposeRgn(clip);
	::DisposeRgn(saveClip);
	::SetPort(savePort);
}

static void _WNResetClip(TWindow inWin)
{
	GrafPtr savePort;
	Rect r;
	RgnHandle rgn;
	GrafPtr winPort = (GrafPtr)::GetWindowPort(WIN->macWin);

#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(winPort, &portRect);
#else
	Rect portRect = winPort->portRect;
#endif
	
	// make region of the entire window
	::RectRgn((RgnHandle)gWorkRgn, &portRect);
	
	// subtract the grow box from the region
	if (WIN->flags & windowOption_Sizeable)			// if has a grow box
	{
		rgn = ::NewRgn();
		if (rgn)
		{
			// set rgn to grox box
			r.right = portRect.right;
			r.bottom = portRect.bottom;
			r.left = r.right-15;
			r.top = r.bottom-15;
			::RectRgn(rgn, &r);
			
			// subtract rgn from gWorkRgn
			::DiffRgn((RgnHandle)gWorkRgn, rgn, (RgnHandle)gWorkRgn);
			::DisposeRgn(rgn);
		}
	}
	
	// set the clipping region
	::GetPort(&savePort);
	::SetPort(winPort);
	::SetOrigin(0,0);
	::SetClip((RgnHandle)gWorkRgn);
	::SetPort(savePort);
}

static Int16 _WNGetTitleBarHeight(TWindow inWin)
{
	WindowRef macWin = WIN->macWin;
	
	if (::IsWindowVisible(macWin))
	{
		GrafPtr savePort;
		Int16 headRoom;
		Point pt;
		
		::GetPort(&savePort);
		::SetPortWindowPort(macWin);
		
		// this technique only works when the window is visible dammit
	#if TARGET_API_MAC_CARBON
		Rect portRect;
		::GetPortBounds(::GetWindowPort(macWin), &portRect);
		
		pt.v = portRect.top;
	#else
		pt.v = ::GetWindowPort(macWin)->portRect.top;
	#endif
	
		::LocalToGlobal(&pt);

	#if TARGET_API_MAC_CARBON
		RgnHandle strucRgn = ::NewRgn();		
		::GetWindowRegion(macWin, kWindowStructureRgn, strucRgn);
		
		Rect strucRgnRect;
		::GetRegionBounds(strucRgn, &strucRgnRect);
		
		::DisposeRgn(strucRgn);
		
		headRoom = pt.v - 1 - strucRgnRect.top;
	#else
		headRoom = pt.v - 1 - (*((WindowPeek)macWin)->strucRgn)->rgnBBox.top;
	#endif
		
		::SetPort(savePort);
		return headRoom;
	}
	else
	{
		// useless macos mutter grumble we're going to have to fudge some values
		return (WIN->layer == windowLayer_Float) ? 16 : 20;
	}
}

/*
 * Calculates a rgn of the entire screen, minus the structure regions
 * of all the windows in layers above the specified window.
 */
static void _WNCalcLayersAboveRgn(TWindow inWin, RgnHandle rgn)
{
	WindowRef macWin;
	SWindow *curWin;
	Uint8 layer = WIN->layer;
	
	// make rgn the desktop
	if (rgn == nil) return;
	::CopyRgn(GetGrayRgn(), rgn);
	
	// loop thru the windows
	macWin = ::FrontWindow();
	while (macWin)
	{
		// get our info
		curWin = (SWindow *)::GetWRefCon(macWin);
		if (curWin == nil) goto nextWin;	// not one of our windows
	
		// subtract window from region
		if (IsWindowVisible(macWin))		// ignore invisible windows
		{
			if (curWin != WIN && curWin->layer != layer)
			{
			#if TARGET_API_MAC_CARBON
				RgnHandle strucRgn = ::NewRgn();		
				::GetWindowRegion(macWin, kWindowStructureRgn, strucRgn);
		
				::DiffRgn(rgn, strucRgn, rgn);
				::DisposeRgn(strucRgn);
			#else
				::DiffRgn(rgn, ((WindowPeek)macWin)->strucRgn, rgn);
			#endif
			}
			else
				break;						// don't subtract windows below
		}
		
		// go down a window
	nextWin:
		macWin = ::GetNextWindow(macWin);
	}
}

/*
 * Calculates a region of the entire screen minus the structure
 * regions of all the windows above the specified window.
 */
static void _WNCalcAboveRgn(WindowRef win, RgnHandle rgn)
{
	WindowRef curWin;
	
	// make rgn the desktop
	if (rgn == nil) return;
	::CopyRgn(GetGrayRgn(), rgn);
	
	// loop thru the windows
	curWin = ::FrontWindow();
	for(;;)
	{
		// check if reached end of window list
		if (curWin == nil) break;
	
		// subtract this window from the region
		if (win != curWin)
		{
		#if TARGET_API_MAC_CARBON
			RgnHandle strucRgn = ::NewRgn();		
			::GetWindowRegion(curWin, kWindowStructureRgn, strucRgn);
		
			::DiffRgn(rgn, strucRgn, rgn);
			::DisposeRgn(strucRgn);
		#else	
			::DiffRgn(rgn, ((WindowPeek)curWin)->strucRgn, rgn);
		#endif
		}
		else
			break;							// don't subtract windows below
		
		// go down a window
		curWin = ::GetNextWindow(curWin);
	}
}

#define USE_THICK_MOVE		1

// note that this function doesn't bring the window to the front
static bool _WNTrackMove(TWindow inWin, Uint16 inMods, Point inMouseLoc, Point& outNewLoc)
{
	// check if the mouse has already been released
	if (!StillDown()) 
		return false;
			
	Point pt;
	short hd, vd;
	bool bIsEnableSnapWindows = gSnapWindows.IsEnableSnapWindows();

	Point mouseLoc = inMouseLoc;
	WindowRef win = WIN->macWin;
	
	CWindow *pWindow = nil;	
	if (WIN->userRef)
	{
	#if !TARGET_API_MAC_CARBON	
		if (bIsEnableSnapWindows)
	#endif
			pWindow = (CWindow *)WIN->userRef;
	}
								
	if (gWindowList.IsInList(pWindow))
	{	
		short left, top, right, bottom;
		
		// get global rects of window
		::SetPortWindowPort(win);
	
	#if TARGET_API_MAC_CARBON
		Rect winRect;
		::GetPortBounds(::GetWindowPort(win), &winRect);
	#else	
		Rect winRect = ::GetWindowPort(win)->portRect;
	#endif
	
		::LocalToGlobal(&topLeft(winRect));
		::LocalToGlobal(&botRight(winRect));
	
	#if TARGET_API_MAC_CARBON
		RgnHandle strucRgn = ::NewRgn();		
		::GetWindowRegion(win, kWindowStructureRgn, strucRgn);
		
		Rect realRect;
		::GetRegionBounds(strucRgn, &realRect);
		
		::DisposeRgn(strucRgn);
	#else
		Rect realRect = (**((WindowPeek)win)->strucRgn).rgnBBox;
	#endif
	
		GrafPtr deskPort = _ImageToGrafPtr(UGraphics::GetDesktopImage());
		::SetPort(deskPort);
		
		if (bIsEnableSnapWindows)
		{
			if (inMods & modKey_Alt)
				gSnapWindows.CreateSnapSound(pWindow);
			else
				gSnapWindows.CreateSnapWinList(pWindow);
		}
		
		// mouse tracking loop
		while (::StillDown())
		{
			::GetMouse(&pt);
			if (pt.h != mouseLoc.h || pt.v != mouseLoc.v)
			{
				// relocate rect to new position
				hd = pt.h - mouseLoc.h;
				vd = pt.v - mouseLoc.v;
				mouseLoc = pt;
				
				OffsetRect(&winRect, hd, vd);
				OffsetRect(&realRect, hd, vd);
											
				if (bIsEnableSnapWindows)
				{
					SRect rSnapWinBounds(realRect.left, realRect.top, realRect.right, realRect.bottom);
					if (gSnapWindows.WinSnapTogether(pWindow, rSnapWinBounds, true))
					{
						left = rSnapWinBounds.left - realRect.left;
						top = rSnapWinBounds.top - realRect.top;
						right = rSnapWinBounds.right - realRect.right;
						bottom = rSnapWinBounds.bottom - realRect.bottom;
										
						mouseLoc.h += left;
						mouseLoc.v += top;
						SetRect(&winRect, winRect.left + left, winRect.top + top, winRect.right + right, winRect.bottom + bottom);
						SetRect(&realRect, realRect.left + left, realRect.top + top, realRect.right + right, realRect.bottom + bottom);
					}
				}
				
				SRect rScreenWinBounds(realRect.left, realRect.top, realRect.right, realRect.bottom);
				if (gSnapWindows.WinKeepOnScreen(pWindow, rScreenWinBounds) || _WNKeepUnderSystemMenu(rScreenWinBounds))
				{
					hd = rScreenWinBounds.left - realRect.left;
					vd =  rScreenWinBounds.top - realRect.top;
					
					mouseLoc.h += hd;
					mouseLoc.v += vd; 
					OffsetRect(&winRect, hd, vd);
					OffsetRect(&realRect, hd, vd);
				}
				
				// resize the window
				SRect rBounds(winRect.left, winRect.top, winRect.right, winRect.bottom);
				UWindow::SetBounds(inWin, rBounds);
									
				if (bIsEnableSnapWindows)
				{
					if (UKeyboard::IsOptionKeyDown())
						gSnapWindows.DestroySnapWinListSound(false);
					else
						gSnapWindows.WinMoveTogether(pWindow);
				}
			}
		}
		
		if (bIsEnableSnapWindows)
			gSnapWindows.DestroySnapWinListSound();
		
		return false;
	}
			
#if USE_THICK_MOVE

	Rect winRect, drawRect;
	RgnHandle saveClip, rgn;
	GrafPtr deskPort;
	bool isVisible = false;

	// get global rects of window
	::SetPortWindowPort(win);

#if TARGET_API_MAC_CARBON
	::GetPortBounds(::GetWindowPort(win), &winRect);
#else	
	winRect = ::GetWindowPort(win)->portRect;
#endif

	::LocalToGlobal(&topLeft(winRect));
	::LocalToGlobal(&botRight(winRect));

#if TARGET_API_MAC_CARBON
	RgnHandle strucRgn = ::NewRgn();		
	::GetWindowRegion(win, kWindowStructureRgn, strucRgn);
		
	::GetRegionBounds(strucRgn, &drawRect);
	::DisposeRgn(strucRgn);
#else
	drawRect = (**((WindowPeek)win)->strucRgn).rgnBBox;
#endif
	
	// setup port
	deskPort = _ImageToGrafPtr(UGraphics::GetDesktopImage());
	::SetPort(deskPort);
	::PenNormal();
	::PenMode(patXor);
	::PenSize(4, 4);
	::PenPat((PatPtr)&pattern_Gray);
	
	// setup clipping
	saveClip = ::NewRgn();
	if (saveClip == nil) return false;
	::GetClip(saveClip);
	rgn = ::NewRgn();
	if (rgn)
	{
		_WNCalcAboveRgn(win, rgn);
		::SetClip(rgn);
		::DisposeRgn(rgn);	// SetClip() copies
	}
	
	// mouse tracking loop
	while (::StillDown())
	{
		::GetMouse(&pt);
		if (pt.h != mouseLoc.h || pt.v != mouseLoc.v)
		{
			hd = pt.h - mouseLoc.h;
			vd = pt.v - mouseLoc.v;
			mouseLoc = pt;
			
			// erase frame
			if (isVisible)
				::FrameRect(&drawRect);
			
			// relocate rects to new position
			::OffsetRect(&winRect, hd, vd);
			::OffsetRect(&drawRect, hd, vd);
			
			// draw frame in new location
			::FrameRect(&drawRect);
			isVisible = true;
		}
	}
	
	// erase frame and restore clipping
	if (isVisible)
		::FrameRect(&drawRect);
		
	::SetClip(saveClip);
	::DisposeRgn(saveClip);
	::PenNormal();

	SRect rScreenWinBounds(drawRect.left, drawRect.top, drawRect.right, drawRect.bottom);
	if (gSnapWindows.WinKeepOnScreen((CWindow *)WIN->userRef, rScreenWinBounds) || _WNKeepUnderSystemMenu(rScreenWinBounds))
		OffsetRect(&winRect,  rScreenWinBounds.left - drawRect.left, rScreenWinBounds.top - drawRect.top);	
	
	// return new location and whether or not changed
	outNewLoc.h = winRect.left;
	outNewLoc.v = winRect.top;
	
	return (mouseLoc.h != inMouseLoc.h || mouseLoc.v != inMouseLoc.v);

#else

	GrafPtr savePort;
	RgnHandle saveClip, rgn;
	Rect winRect, drawRect;
	Rect limit, slop;
	Int32 distMoved;
	bool result = false;
	Point corner;
	
	// find global coords of the top left corner of the window
	::GetPort(&savePort);
	::SetPortWindowPort(win);
	winRect = ::GetWindowPort(win)->portRect;
	::LocalToGlobal(&topLeft(winRect));
	::LocalToGlobal(&botRight(winRect));
	drawRect = (**((WindowPeek)win)->strucRgn).rgnBBox;
	corner = topLeft(winRect);

	// setup port
	::SetPort(_ImageToGrafPtr(UGraphics::GetDesktopImage()));
	::PenNormal();
		
	// setup clipping
	saveClip = ::NewRgn();
	if (saveClip == nil) goto abort;
	::GetClip(saveClip);
	rgn = ::NewRgn();
	if (rgn != nil)
	{
		_WNCalcAboveRgn(win, rgn);
		::SetClip(rgn);
		::DisposeRgn(rgn);	// SetClip() copies
	}
	
	// do the drag
	slop = limit = (**GetGrayRgn()).rgnBBox;
	limit.top += 4;
	limit.left += 4;
	limit.bottom -= 4;
	limit.right -= 4;
	rgn = ::NewRgn();
	if (rgn == nil) goto abort;
	::GetWindowStructureRgn(win, rgn);
	distMoved = ::DragGrayRgn(rgn, inMouseLoc, &limit, &slop, 0, nil);
	::DisposeRgn(rgn);
	
	result = !((distMoved == 0L) || (distMoved == 0x80008000));
	outNewLoc.h = corner.h + LoWord(distMoved);
	outNewLoc.v = corner.v + HiWord(distMoved);

	SRect rScreenWinBounds(outNewLoc.h, outNewLoc.v, outNewLoc.h + drawRect.right - drawRect.left, outNewLoc.v + drawRect.bottom - drawRect.top);
	if (gSnapWindows.WinKeepOnScreen((CWindow *)WIN->userRef, rScreenWinBounds) || _WNKeepUnderSystemMenu(rScreenWinBounds))
	{
		outNewLoc.h = rScreenWinBounds.left + winRect.left - drawRect.left;
		outNewLoc.v = rScreenWinBounds.top + winRect.top - drawRect.top;
	}
	
	// clean up
abort:
	if (saveClip)
	{
		::SetClip(saveClip);
		::DisposeRgn(saveClip);
	}
	
	::SetPort(savePort);
	
	return result;
	
#endif
}

static bool _WNTrackGrow(TWindow inWin, Point startPt, Int32& newWidth, Int32& newHeight)
{
	if (UWindow::IsCollapsed(inWin))
		return false;
	
	bool bIsEnableSnapWindows = gSnapWindows.IsEnableSnapWindows();
	
	CWindow *pWindow = nil;	
	if (WIN->userRef)
	{
	#if !TARGET_API_MAC_CARBON
		if (bIsEnableSnapWindows)
	#endif
			pWindow = (CWindow *)WIN->userRef;
	}
	
	if (gWindowList.IsInList(pWindow))
	{	
		SRect rBounds;
		Point pt, oldpt;
		short hd, vd;
		
		Point mouseLoc = startPt;		
		WindowRef win = WIN->macWin;
	
		// set port
		::SetPortWindowPort(win);
	
		// get window bounds
	#if TARGET_API_MAC_CARBON
		Rect winRect;
		::GetPortBounds(::GetWindowPort(win), &winRect);
	#else	
		Rect winRect = ::GetWindowPort(win)->portRect;
	#endif
	
		::LocalToGlobal(&topLeft(winRect));
		::LocalToGlobal(&botRight(winRect));

		// get real window bounds
		Rect realRect;
		_GetRealWinBounds(inWin, realRect);
	
		Point saveDist;
		saveDist.h = winRect.left - realRect.left + realRect.right - winRect.right;
		saveDist.v = winRect.top - realRect.top + realRect.bottom - winRect.bottom;
		
		Point savedDepl; 
		savedDepl.h = winRect.right - mouseLoc.h; 
		savedDepl.v = winRect.bottom - mouseLoc.v;
	
		GrafPtr deskPort = _ImageToGrafPtr(UGraphics::GetDesktopImage());
		::SetPort(deskPort);
				
		if (bIsEnableSnapWindows)
			gSnapWindows.CreateSnapSound(pWindow);

		// mouse tracking loop
		oldpt.h = 0; oldpt.v = 0;
		while (::StillDown())
		{			
			// get mouse location
			::GetMouse(&pt);
			if (pt.h == oldpt.h && pt.v == oldpt.v)
				continue;
			
			oldpt.h = pt.h;
			oldpt.v = pt.v;
			
			// get window bounds
			UWindow::GetBounds(inWin, rBounds);
			winRect.left = rBounds.left;
			winRect.top = rBounds.top;
			winRect.right = rBounds.right;
			winRect.bottom = rBounds.bottom;
		
			// get real window bounds
			_GetRealWinBounds(inWin, realRect);
		
			mouseLoc.h = winRect.right - savedDepl.h; 
			mouseLoc.v = winRect.bottom - savedDepl.v;

			if (pt.h == mouseLoc.h && pt.v == mouseLoc.v)
				continue;
			
			hd = pt.h - mouseLoc.h;
			if (winRect.right - winRect.left + hd < WIN->minWidth)
				hd = WIN->minWidth - (winRect.right - winRect.left);
					
			vd = vd = pt.v - mouseLoc.v;
			if (winRect.bottom - winRect.top + vd < WIN->minHeight)
				vd = WIN->minHeight - (winRect.bottom - winRect.top);
								
			if (!hd && !vd)
				continue;
					
			winRect.right += hd;
			winRect.bottom += vd;
			realRect.right += hd;
			realRect.bottom += vd;
										
			if (bIsEnableSnapWindows)
			{
				SRect rSnapWinBounds(realRect.left, realRect.top, realRect.right, realRect.bottom);
				if (gSnapWindows.WinSnapTogether(pWindow, rSnapWinBounds, false))
				{
					hd = rSnapWinBounds.right - realRect.right;
					vd = rSnapWinBounds.bottom - realRect.bottom;
										
					mouseLoc.h += hd;
					mouseLoc.v += vd;
					
					winRect.right += hd;
					winRect.bottom += vd;
					realRect.right += hd;
					realRect.bottom += vd;
				}
			}
			
			SRect rScreenWinBounds(realRect.left, realRect.top, realRect.right, realRect.bottom);
			if (gSnapWindows.WinKeepOnScreen(pWindow, rScreenWinBounds) || _WNKeepUnderSystemMenu(rScreenWinBounds))
			{
				hd = rScreenWinBounds.right - realRect.right;
				vd =  rScreenWinBounds.bottom - realRect.bottom;
					
				mouseLoc.h += hd;
				mouseLoc.v += vd; 
					
				winRect.right += hd;
				winRect.bottom += vd;
				realRect.right += hd;
				realRect.bottom += vd;
			}
				
			if (winRect.right - winRect.left < WIN->minWidth)
			{
				winRect.right = winRect.left + WIN->minWidth;
				realRect.right = realRect.left + WIN->minWidth + saveDist.h;
			}
			else if (winRect.right - winRect.left > WIN->maxWidth)
			{
				winRect.right = winRect.left + WIN->maxWidth;
				realRect.right = realRect.left + WIN->maxWidth + saveDist.h;
			}

			if (winRect.bottom - winRect.top < WIN->minHeight)
			{
				winRect.bottom = winRect.top + WIN->minHeight;
				realRect.bottom = realRect.top + WIN->minHeight + saveDist.v;
			}
			else if (winRect.bottom - winRect.top > WIN->maxHeight)
			{
				winRect.bottom = winRect.top + WIN->maxHeight;
				realRect.bottom = realRect.top + WIN->maxHeight + saveDist.v;
			}
				
			// resize the window
			rBounds.Set(winRect.left, winRect.top, winRect.right, winRect.bottom);
			UWindow::SetBounds(inWin, rBounds);									
			
			// SetBounds() calls BoundsChanged() which can overwrite rBounds, so we must get the real window bounds here
			UWindow::GetBounds(inWin, rBounds);
			
			// force the window to be redrawn
			WIN->needsRedraw = true;
			WIN->redrawRect.left = WIN->redrawRect.top = 0;
			WIN->redrawRect.right = rBounds.GetWidth();
			WIN->redrawRect.bottom = rBounds.GetHeight();
			UApplication::SendMessage(msg_Draw, nil, 0, priority_Draw, WIN->msgProc, WIN->msgProcContext, inWin);
		}
		
		if (bIsEnableSnapWindows)
			gSnapWindows.DestroySnapWinListSound();
		
		return false;
	}

	Int32 newSize;
	Rect sizeRect;
		
	sizeRect.top = WIN->minHeight;
	sizeRect.left = WIN->minWidth;
	sizeRect.bottom = WIN->maxHeight;
	sizeRect.right = WIN->maxWidth;
	
	newSize = ::GrowWindow(WIN->macWin, startPt, &sizeRect);
	
	newHeight = HiWord(newSize);
	newWidth = LoWord(newSize);
	
	return (newSize != 0);
}

/*
 * Erases the grow box, and then invalidates that area so that it
 * will be redrawn through the event loop.
 */
static void _WNEraseGrowBox(WindowRef win)
{
#if TARGET_API_MAC_CARBON
	Rect winBounds;
	::GetPortBounds(::GetWindowPort(win), &winBounds);
#else	
	Rect winBounds = ::GetWindowPort(win)->portRect;
#endif

	Rect r;
	r.right = winBounds.right - winBounds.left;
	r.bottom = winBounds.bottom - winBounds.top;
	r.left = r.right - 15;
	r.top = r.bottom - 15;
	
	GrafPtr savePort;
	::GetPort(&savePort);
	::SetPortWindowPort(win);

#if TARGET_API_MAC_CARBON
	::InvalWindowRect(win, &r);
#else
	::InvalRect(&r);
#endif
	
	RgnHandle rgn = ::NewRgn();
	::RectRgn(rgn, &r);

#if TARGET_API_MAC_CARBON
	RgnHandle saveVis = ::NewRgn();
	::GetPortVisibleRegion(::GetWindowPort(win), saveVis);
#else					
	RgnHandle saveVis = ::GetWindowPort(win)->visRgn;
#endif
	
	::EraseRect(&r);
	
#if TARGET_API_MAC_CARBON
	::SetPortVisibleRegion(::GetWindowPort(win), saveVis);
	::DisposeRgn(saveVis);
#else	
	::GetWindowPort(win)->visRgn = saveVis;
#endif
	
	::DisposeRgn(rgn);
	::SetPort(savePort);
}

static void _WNRefreshGrowBox(WindowRef win)
{
#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(::GetWindowPort(win), &portRect);
#else
	Rect portRect = ::GetWindowPort(win)->portRect;
#endif

	Rect r;
	r.right = portRect.right;
	r.bottom = portRect.bottom;
	r.left = r.right - 15;
	r.top = r.bottom - 15;
	
#if TARGET_API_MAC_CARBON
	::InvalWindowRect(win, &r);
#else
	GrafPtr savePort;
	::GetPort(&savePort);
	::SetPortWindowPort(win);
	::InvalRect(&r);
	::SetPort(savePort);
#endif
}

#pragma mark -

// this function works only if the window is visible
void _GetRealWinBounds(TWindow inWindow, Rect& outWindowBounds)
{
	WindowRef win = ((SWindow *)inWindow)->macWin;
	
#if TARGET_API_MAC_CARBON
	RgnHandle strucRgn = ::NewRgn();		
	::GetWindowRegion(win, kWindowStructureRgn, strucRgn);
	::GetRegionBounds(strucRgn, &outWindowBounds);
		
	::DisposeRgn(strucRgn);
#else
	outWindowBounds = (**((WindowPeek)win)->strucRgn).rgnBBox;
#endif
}
	
// this function works only if the window is visible
void _GetRealWinBounds(TWindow inWindow, SRect& outWindowBounds)
{
	Rect stWindowBounds;
	_GetRealWinBounds(inWindow, stWindowBounds);
	
	outWindowBounds.Set(stWindowBounds.left, stWindowBounds.top, stWindowBounds.right, stWindowBounds.bottom);
}

// this function works only if the window is visible
void _GetRealWinBounds(CWindow *inWindow, SRect& outWindowBounds)
{
	_GetRealWinBounds(*inWindow, outWindowBounds);	
}

void _SetRealWinBounds(CWindow *inWindow, SRect inWindowBounds)
{
	GrafPtr savePort;
	::GetPort(&savePort);
	
	WindowRef win = ((SWindow *)(TWindow)*inWindow)->macWin;
	::SetPortWindowPort(win);
	
#if TARGET_API_MAC_CARBON
	Rect rWinBounds;
	::GetPortBounds(::GetWindowPort(win), &rWinBounds);
#else
	Rect rWinBounds = ::GetWindowPort(win)->portRect;
#endif	
	
	::LocalToGlobal(&topLeft(rWinBounds));
	::LocalToGlobal(&botRight(rWinBounds));

#if TARGET_API_MAC_CARBON
	RgnHandle strucRgn = ::NewRgn();		
	::GetWindowRegion(win, kWindowStructureRgn, strucRgn);
		
	Rect rSnapWinBounds;
	::GetRegionBounds(strucRgn, &rSnapWinBounds);
		
	::DisposeRgn(strucRgn);
#else
	Rect rSnapWinBounds = (**((WindowPeek)win)->strucRgn).rgnBBox;
#endif
	
	::SetPort(savePort);

	inWindowBounds.left += rWinBounds.left - rSnapWinBounds.left;
	inWindowBounds.top += rWinBounds.top - rSnapWinBounds.top;
	inWindowBounds.right -= rSnapWinBounds.right - rWinBounds.right;
	inWindowBounds.bottom -= rSnapWinBounds.bottom - rWinBounds.bottom;

	UWindow::SetBounds(*inWindow, inWindowBounds);
}

void _SetRealWinLocation(CWindow *inWindow, SPoint inWindowLocation, CWindow *inInsertAfter)
{
	GrafPtr savePort;
	::GetPort(&savePort);
	
	WindowRef win = ((SWindow *)(TWindow)*inWindow)->macWin;
	::SetPortWindowPort(win);
	
#if TARGET_API_MAC_CARBON
	Rect rWinBounds;
	::GetPortBounds(::GetWindowPort(win), &rWinBounds);
#else
	Rect rWinBounds = GetWindowPort(win)->portRect;
#endif

	::LocalToGlobal(&topLeft(rWinBounds));
	::LocalToGlobal(&botRight(rWinBounds));

#if TARGET_API_MAC_CARBON
	RgnHandle strucRgn = ::NewRgn();		
	::GetWindowRegion(win, kWindowStructureRgn, strucRgn);
		
	Rect rSnapWinBounds;
	::GetRegionBounds(strucRgn, &rSnapWinBounds);
		
	::DisposeRgn(strucRgn);
#else
	Rect rSnapWinBounds = (**((WindowPeek)win)->strucRgn).rgnBBox;
#endif
	
	::SetPort(savePort);

	SPoint ptLocation(inWindowLocation.x + rWinBounds.left - rSnapWinBounds.left,inWindowLocation.y + rWinBounds.top - rSnapWinBounds.top);
	UWindow::SetLocation(*inWindow, ptLocation);
		
	if (inInsertAfter)
		inWindow->BringToFront();
}

void _GetScreenBounds(SRect& outScreenBounds)
{
	GDHandle mainDev = ::GetMainDevice();
	Rect rScreenBounds = (**mainDev).gdRect;
	rScreenBounds.top += ::GetMBarHeight();
			
	outScreenBounds.Set(rScreenBounds.left, rScreenBounds.top, rScreenBounds.right, rScreenBounds.bottom);
}


#pragma mark -

// r is desired bounds on input, and adjusted bounds on output
static void _WNCalcAdjustedBounds(TWindow inWin, Rect *r)
{
	Int16 frameTopSize, frameLeftSize, frameRightSize, frameBottomSize;
	Rect contentRect, structureRect;
	WindowRef macWin = WIN->macWin;
	
	// determine the size of the window frame
#if TARGET_API_MAC_CARBON
	RgnHandle contRgn = ::NewRgn();		
	::GetWindowRegion(macWin, kWindowContentRgn, contRgn);
	
	::GetRegionBounds(contRgn, &contentRect);
	::DisposeRgn(contRgn);
	
	RgnHandle strucRgn = ::NewRgn();		
	::GetWindowRegion(macWin, kWindowStructureRgn, strucRgn);
	
	::GetRegionBounds(strucRgn, &structureRect);
	::DisposeRgn(strucRgn);
#else
	contentRect = (**((WindowPeek)macWin)->contRgn).rgnBBox;
	structureRect = (**((WindowPeek)macWin)->strucRgn).rgnBBox;
#endif

	frameTopSize = contentRect.top - structureRect.top;
	// TG - TO DO - fix the following two lines
	if(frameTopSize == 0)
		frameTopSize = 20;	// for some reason we have a bug where I don't see the titlebar

	frameLeftSize = contentRect.left - structureRect.left;
	frameRightSize = structureRect.right - contentRect.right;
	frameBottomSize = structureRect.bottom - contentRect.bottom;

	// calculate the new structure rect
	structureRect = *r;
	structureRect.top -= frameTopSize;
	structureRect.left -= frameLeftSize;
	structureRect.bottom += frameBottomSize;
	structureRect.right += frameRightSize;
	
	// make sure the structure rect fits nicely on the screen
	_WNAdjustBoundsToScreen(&structureRect, (WIN->flags & windowOption_Sizeable)!=0);
	
	// bounds doesn't include the window frame
	structureRect.top += frameTopSize;
	structureRect.left += frameLeftSize;
	structureRect.bottom -= frameBottomSize;
	structureRect.right -= frameRightSize;
	*r = structureRect;
}

static void _WNCalcZoomedBounds(TWindow inWin, Rect *idealBounds, Rect& bestBounds)
{
	WindowRef macWin = WIN->macWin;
	Rect ideal = *idealBounds;

#if TARGET_API_MAC_CARBON
	RgnHandle contRgn = ::NewRgn();		
	::GetWindowRegion(macWin, kWindowContentRgn, contRgn);
	
	Rect contentRect;
	::GetRegionBounds(contRgn, &contentRect);
	
	::DisposeRgn(contRgn);
#else
	Rect contentRect = (**((WindowPeek)macWin)->contRgn).rgnBBox;
#endif
	
	// if zoom box but no size box, we toggle between min and max
	if (WIN->flags & windowOption_ZoomBox && !(WIN->flags & windowOption_Sizeable))
	{
		// ideal is max
		ideal.top = contentRect.top;
		ideal.left = contentRect.left;
		ideal.bottom = ideal.top + WIN->maxHeight;
		ideal.right = ideal.left + WIN->maxWidth;
		
		// user size is min
		WIN->saveBounds.top = contentRect.top;
		WIN->saveBounds.left = contentRect.left;
		WIN->saveBounds.bottom = WIN->saveBounds.top + WIN->minHeight;
		WIN->saveBounds.right = WIN->saveBounds.left + WIN->minWidth;
	}
	
	// calc best size
	_WNCalcAdjustedBounds(inWin, &ideal);
	if ((ideal.right - ideal.left) == (contentRect.right - contentRect.left) && (ideal.bottom - ideal.top) == (contentRect.bottom - contentRect.top))
	{
		// we're already at the ideal size, so go back to the user size
		bestBounds.top = contentRect.top;
		bestBounds.left = contentRect.left;
		bestBounds.bottom = bestBounds.top + (WIN->saveBounds.bottom - WIN->saveBounds.top);
		bestBounds.right = bestBounds.left + (WIN->saveBounds.right - WIN->saveBounds.left);
		_WNCalcAdjustedBounds(inWin, &bestBounds);
	}
	else
	{
		// zoom to the ideal bounds
		bestBounds = ideal;
		WIN->saveBounds = contentRect;
	}
}

#if 0
static void _WNCalcStaggeredBounds(WindowRef macWin, Rect& bounds)
{
	Rect r;
	Int16 h, w;
	
	// move to top left
	Rect& portRect = macWin->portRect;
	h = portRect.bottom - portRect.top;
	w = portRect.right - portRect.left;
	r.top = GetMBarHeight() + 10;
	r.left = 10;
	r.bottom = r.top + h;
	r.right = r.left + w;
	
	/*
	This function is not finished. A proper version would actually
	stagger the windows by looking at the other visible windows
	with kPosStagger auto positioning.
	*/
	
	// bounds is now staggered
	bounds = r;
}
#endif

static void _WNCalcStaggeredBounds(TWindow inWin, Rect *r)
{
	Int16 frameTopSize, frameLeftSize, frameRightSize, frameBottomSize;
	Rect contentRect, structureRect;
	WindowRef macWin = WIN->macWin;
	
	// determine the size of the window frame
#if TARGET_API_MAC_CARBON
	RgnHandle contRgn = ::NewRgn();		
	::GetWindowRegion(macWin, kWindowContentRgn, contRgn);
	
	::GetRegionBounds(contRgn, &contentRect);
	::DisposeRgn(contRgn);
	
	RgnHandle strucRgn = ::NewRgn();		
	::GetWindowRegion(macWin, kWindowStructureRgn, strucRgn);
	
	::GetRegionBounds(strucRgn, &structureRect);
	::DisposeRgn(strucRgn);
#else
	contentRect = (**((WindowPeek)macWin)->contRgn).rgnBBox;
	structureRect = (**((WindowPeek)macWin)->strucRgn).rgnBBox;
#endif
	
	frameTopSize = contentRect.top - structureRect.top;
	frameLeftSize = contentRect.left - structureRect.left;
	frameRightSize = structureRect.right - contentRect.right;
	frameBottomSize = structureRect.bottom - contentRect.bottom;

	// calculate the new structure rect
	structureRect = *r;
	structureRect.left -= frameLeftSize - 20;
	structureRect.top -= frameTopSize - 20;
	structureRect.bottom += frameBottomSize + 20;
	structureRect.right += frameRightSize + 20;
	
	
	// get the screen's bounds
	Rect screenRect;
	GDHandle rectDevice, mainDevice;

	// get the screen with the largest portion of the Rect
	mainDevice = GetMainDevice();
	rectDevice = _GetRectDevice(&structureRect);
	if (rectDevice == nil) rectDevice = mainDevice;
	screenRect = (**rectDevice).gdRect;
	
	// don't put the Rect under the menubar
	if (rectDevice == mainDevice)
		screenRect.top += GetMBarHeight();
	
	// check if we're off the screen anywhere
	// dont' need to check top and left
	if (structureRect.right > screenRect.right)
	{
		Int16 dx = structureRect.right - screenRect.right;
		structureRect.left -= dx;
		structureRect.right -= dx;
		if (structureRect.left < screenRect.left)
		{
			if (!WIN->flags & windowOption_Sizeable)	// to do - need to check limits
				structureRect.right += screenRect.left - structureRect.left;
			
			structureRect.left = screenRect.left;	
		}
	}
	
	if (structureRect.bottom > screenRect.bottom)
	{
		Int16 dy = structureRect.bottom - screenRect.bottom;
		structureRect.top -= dy;
		structureRect.bottom -= dy;
		if (structureRect.top < screenRect.top)	// to do - check that a. we're sizable, and b. that this does't violate limits
		{
			if (!WIN->flags & windowOption_Sizeable)	// to do - need to check limits
				structureRect.bottom += screenRect.top - structureRect.top;
		
			structureRect.top = screenRect.top;
		}
	}
	
	// bounds doesn't include the window frame
	structureRect.top += frameTopSize;
	structureRect.left += frameLeftSize;
	structureRect.bottom -= frameBottomSize;
	structureRect.right -= frameRightSize;
	*r = structureRect;
}

static void _WNCalcUnobstructedBounds(TWindow inWin, Rect *r)
{	
	Require(inWin);

	SRect rScreenBounds;
	_GetScreenBounds(rScreenBounds);
	
	SRect rWindowBounds;
	_GetRealWinBounds(inWin, rWindowBounds);

	GrafPtr savePort;
	::GetPort(&savePort);
	
	WindowRef macWin = WIN->macWin;
	::SetPortWindowPort(macWin);
	
#if TARGET_API_MAC_CARBON
	Rect rBounds;
	::GetPortBounds(::GetWindowPort(macWin), &rBounds);
#else
	Rect rBounds = ::GetWindowPort(macWin)->portRect;
#endif

	::LocalToGlobal(&topLeft(rBounds));
	::LocalToGlobal(&botRight(rBounds));
	
	::SetPort(savePort);
	SRect rDepl(rBounds.left - rWindowBounds.left, rBounds.top - rWindowBounds.top, rWindowBounds.right - rBounds.right, rWindowBounds.bottom - rBounds.bottom);
	
	CWindow *win = nil;
	Uint32 i = 0;
 		
	while (gWindowList.GetNextWindow(win, i))
	{
		if ((TWindow)*win == inWin || win->GetLayer() != windowLayer_Float || !win->IsVisible())
			continue;
		
		SRect rWinBounds;
		_GetRealWinBounds(win, rWinBounds);	

		if (rWindowBounds.Intersects(rWinBounds))
		{
			if (rWinBounds.GetWidth() <= rWinBounds.GetHeight())
			{
				if (rWindowBounds.right - rWinBounds.left <= rWinBounds.right - rWindowBounds.left && rScreenBounds.left <= rWinBounds.left - rWindowBounds.GetWidth())
					rWindowBounds.MoveBack(rWindowBounds.right - rWinBounds.left, 0);
				else if (rScreenBounds.right >= rWinBounds.right + rWindowBounds.GetWidth())
					rWindowBounds.Move(rWinBounds.right - rWindowBounds.left, 0);
				else if (rScreenBounds.left <= rWinBounds.left - rWindowBounds.GetWidth())
					rWindowBounds.MoveBack(rWindowBounds.right - rWinBounds.left, 0);
				else if (rWindowBounds.bottom - rWinBounds.top <= rWinBounds.bottom - rWindowBounds.top && rScreenBounds.top <= rWinBounds.top - rWindowBounds.GetHeight())
					rWindowBounds.MoveBack(0, rWindowBounds.bottom - rWinBounds.top);
				else if (rScreenBounds.bottom >= rWinBounds.bottom + rWindowBounds.GetHeight())
					rWindowBounds.Move(0, rWinBounds.bottom - rWindowBounds.top);
				else if (rScreenBounds.top <= rWinBounds.top - rWindowBounds.GetHeight())
					rWindowBounds.MoveBack(0, rWindowBounds.bottom - rWinBounds.top);
			}
			else
			{
				if (rWindowBounds.bottom - rWinBounds.top <= rWinBounds.bottom - rWindowBounds.top && rScreenBounds.top <= rWinBounds.top - rWindowBounds.GetHeight())
					rWindowBounds.MoveBack(0, rWindowBounds.bottom - rWinBounds.top);
				else if (rScreenBounds.bottom >= rWinBounds.bottom + rWindowBounds.GetHeight())
					rWindowBounds.Move(0, rWinBounds.bottom - rWindowBounds.top);
				else if (rScreenBounds.top <= rWinBounds.top - rWindowBounds.GetHeight())
					rWindowBounds.MoveBack(0, rWindowBounds.bottom - rWinBounds.top);
				else if (rWindowBounds.right - rWinBounds.left <= rWinBounds.right - rWindowBounds.left && rScreenBounds.left <= rWinBounds.left - rWindowBounds.GetWidth())
					rWindowBounds.MoveBack(rWindowBounds.right - rWinBounds.left, 0);
				else if (rScreenBounds.right >= rWinBounds.right + rWindowBounds.GetWidth())
					rWindowBounds.Move(rWinBounds.right - rWindowBounds.left, 0);
				else if (rScreenBounds.left <= rWinBounds.left - rWindowBounds.GetWidth())
					rWindowBounds.MoveBack(rWindowBounds.right - rWinBounds.left, 0);
			}
		}
	}
	
	r->left = rWindowBounds.left + rDepl.left;
	r->top = rWindowBounds.top + rDepl.top;
	r->right = rWindowBounds.right - rDepl.right;
	r->bottom = rWindowBounds.bottom - rDepl.bottom;
}

enum {
	kNudgeSlop = 4, kIconSpace = 64
};

// moves and resizes the specified Rect such that it fits on the screen
static void _WNAdjustBoundsToScreen(Rect *theRect, bool shrink)
{
	Rect screenRect, tr;
	GDHandle rectDevice, mainDevice;
	Int16 horizPixOffscreen, vertPixOffscreen;
	Rect r = *theRect;

	// get the screen with the largest portion of the Rect
	mainDevice = ::GetMainDevice();
	rectDevice = _GetRectDevice(&r);
	if (rectDevice == nil) rectDevice = mainDevice;
	screenRect = (**rectDevice).gdRect;
	
	// don't put the Rect under the menubar
	if (rectDevice == mainDevice)
		screenRect.top += ::GetMBarHeight();
	
	// if the Rect falls off the edge of the screen, nudge it so that it's just on the screen
	SectRect(&r, &screenRect, &tr);
	if (r.top != tr.top || r.left != tr.left || r.right != tr.right || r.bottom != tr.bottom)
	{
		horizPixOffscreen = _WNCalculateOffsetAmount(r.left, r.right, tr.left, tr.right,
			screenRect.left, screenRect.right);
		
		vertPixOffscreen = _WNCalculateOffsetAmount(r.top, r.bottom, tr.top, tr.bottom,
			screenRect.top, screenRect.bottom);
		
		::OffsetRect(&r, horizPixOffscreen, vertPixOffscreen);
	}
	
	// if still falling off the edge of the screen, need to shrink the Rect
	if (shrink)
	{
		SectRect(&r, &screenRect, &tr);
		if (r.top != tr.top || r.left != tr.left || r.right != tr.right || r.bottom != tr.bottom)
		{
			// First shrink the width of the window. If the window is wider than the screen
			// it is zooming to, we can just pin the standard rectangle to the edges of the
			// screen, leaving some slop. If the window is narrower than the screen, we know
			// we just nudged it into position, so nothing needs to be done.
			if ((r.right - r.left) > (screenRect.right - screenRect.left))
			{
				r.left = screenRect.left + kNudgeSlop;
				r.right = screenRect.right - kNudgeSlop;
	
				if ((rectDevice == mainDevice) && (r.right > (screenRect.right - kIconSpace)))
					r.right = screenRect.right - kIconSpace;
			}
	
			// Move in the top. Like the width of the window, nothing needs to be done unless
			// the window is taller than the height of the screen.
			if ((r.bottom - r.top) > (screenRect.bottom - screenRect.top))
			{
				r.top = screenRect.top + kNudgeSlop;
				r.bottom = screenRect.bottom - kNudgeSlop;
			}
		}
	}
	
	// the Rect is now perfectly positioned on the screen
	*theRect = r;
}

/*
 * Figure out how much we need to move the window to get it entirely on the monitor.  If
 * the window wouldn�t fit completely on the monitor anyway, don�t move it at all; we'll
 * make it fit later on.
 */
static Int16 _WNCalculateOffsetAmount(Int16 idealStartPoint, Int16 idealEndPoint, Int16 idealOnScreenStartPoint,
							Int16 idealOnScreenEndPoint, Int16 screenEdge1, Int16 screenEdge2)
{
	Int16	offsetAmount;

	// check to see if the Rect fits on the screen in this dimension
	if ((idealStartPoint < screenEdge1) && (idealEndPoint > screenEdge2))
		offsetAmount = 0;
	else
	{
		/*
		 * Find out how much of the Rect lies off this screen by subtracting the amount of the Rect
		 * that is on the screen from the size of the entire Rect in this dimension. If the Rect
		 * is completely offscreen, the offset amount is going to be the distance from the ideal
		 * starting point to the first edge of the screen.
		 */
		
		if ((idealOnScreenStartPoint - idealOnScreenEndPoint) == 0)
		{
			if (idealEndPoint < screenEdge1)	// if lying to the left or above the screen
				offsetAmount = screenEdge1 - idealStartPoint + kNudgeSlop;
			else								// below or to the right of the screen
				offsetAmount = screenEdge2 - idealEndPoint - kNudgeSlop;
		}
		else
		{
			// already partially or completely on the screen
			offsetAmount = (idealEndPoint - idealStartPoint) - (idealOnScreenEndPoint - idealOnScreenStartPoint);
	
			// if offscreen a little, move the Rect in a few more pixels from the edge of the screen
			if (offsetAmount != 0)
				offsetAmount += kNudgeSlop;
			
			// make sure we nudge in the right direction
			if (idealEndPoint > screenEdge2)
				offsetAmount = -offsetAmount;
		}
	}
	
	return offsetAmount;
}

bool _WNKeepUnderSystemMenu(SRect& ioWindowBounds)
{	
	SRect rScreenBounds;
	_GetScreenBounds(rScreenBounds);
	
	if (rScreenBounds.top > ioWindowBounds.top)
	{
		ioWindowBounds.Move(0, rScreenBounds.top - ioWindowBounds.top);
		return true;
	}
	
	return false;
}

// returns the device with the largest portion of the specified Rect
GDHandle _GetRectDevice(Rect *theRect)
{
	GDHandle deviceList, dev;
	Rect theSect, gdRect;
	Int32 greatestArea, sectArea;
	
	deviceList = GetDeviceList();
	greatestArea = 0;
	dev = nil;
		
	while (deviceList != nil)
	{
		if (TestDeviceAttribute(deviceList, screenDevice) && TestDeviceAttribute(deviceList, screenActive))
		{
			gdRect = (**deviceList).gdRect;
			SectRect(theRect, &gdRect, &theSect);
			sectArea = (Int32)(((Int32)theSect.right - (Int32)theSect.left) * ((Int32)theSect.bottom - (Int32)theSect.top));
			
			if (sectArea > greatestArea)
			{
				greatestArea = sectArea;	// greatest area so far
				dev = deviceList;
			}
		}
		deviceList = GetNextDevice(deviceList);
	}
	
	return dev;
}

static GDHandle _GetWindowDevice(WindowRef macWin)
{
	// get structure region bounds
#if TARGET_API_MAC_CARBON
	RgnHandle strucRgn = ::NewRgn();		
	::GetWindowRegion(macWin, kWindowStructureRgn, strucRgn);
	
	Rect r;
	::GetRegionBounds(strucRgn, &r);
	
	::DisposeRgn(strucRgn);
#else
	Rect r = (**((WindowPeek)macWin)->strucRgn).rgnBBox;
#endif
	
	// structure and content regions are empty if window is invisible, hidden, etc, so we'll use the window bounds instead
	if (::EmptyRect(&r))
	{
		GrafPtr savePort;
		::GetPort(&savePort);
		::SetPortWindowPort(macWin);
		
	#if TARGET_API_MAC_CARBON
		::GetPortBounds(::GetWindowPort(macWin), &r);
	#else
		r = macWin->portRect;
	#endif
	
		::LocalToGlobal((Point *)&r);
		::LocalToGlobal(((Point *)&r)+1);

		::SetPort(savePort);
	}
	
	return _GetRectDevice(&r);
}

static void _CenterRect(Rect& theRect, Rect base)
{
	_CenterRectH(theRect, base);
	_CenterRectV(theRect, base);
}

static void _CenterRectH(Rect& theRect, Rect base)
{
	Int16 n = theRect.right - theRect.left;
	theRect.left = base.left + ((base.right - base.left - n) / 2);
	theRect.right = theRect.left + n;
}

static void _CenterRectV(Rect& theRect, Rect base)
{
	Int16 n = theRect.bottom - theRect.top;
	theRect.top = base.top + ((base.bottom - base.top - n) / 2);
	theRect.bottom = theRect.top + n;
}

static void _WNUpdateHandler(WindowRef inMacWin)
{
	TWindow win;
	SRect updateRect;

	// get mac update rect
#if TARGET_API_MAC_CARBON
	RgnHandle updateRgn = ::NewRgn();
	::GetWindowRegion(inMacWin, kWindowUpdateRgn, updateRgn);
	
	Rect updateRgnRect;
	::GetRegionBounds(updateRgn, &updateRgnRect);
	
	::DisposeRgn(updateRgn);
#else
	Rect updateRgnRect = (**(((WindowPeek)inMacWin)->updateRgn)).rgnBBox;
#endif

	::SetPortWindowPort(inMacWin);
	::GlobalToLocal(&topLeft(updateRgnRect));
	::GlobalToLocal(&botRight(updateRgnRect));
	
	// convert mac update rect to universal rect
	updateRect.top = updateRgnRect.top;
	updateRect.left = updateRgnRect.left;
	updateRect.bottom = updateRgnRect.bottom;
	updateRect.right = updateRgnRect.right;

	// clear update event
	::BeginUpdate(inMacWin);
	::EndUpdate(inMacWin);
	
	// post draw message
	win = (TWindow)::GetWRefCon(inMacWin);
	if (win) UWindow::Refresh(win, updateRect);
}

// call to dispatch screen msg_MouseDown, msg_MouseUp, msg_MouseMove
void _WNDispatchMouse(Uint32 inType, const SMouseMsgData& inInfo, bool inResumeClick)
{
	SMouseMsgData info = inInfo;
	Point mouseLoc, newLoc;
	WindowRef macWin;
	SWindow *win;
	Int16 part;
	SPoint pt;
	SRect r;
	
	_WNCheckMouseWithinWinFlag = false;		// this func checks it so don't need to check twice
	_WNCheckWinStates();

	mouseLoc.v = info.loc.v;
	mouseLoc.h = info.loc.h;
	
	if (_WNData.mouseCaptureWin)
	{
		win = _WNData.mouseCaptureWin;
		if (win) UWindow::GlobalToLocal((TWindow)win, info.loc);
	
		// update mouse-within window
		if (_WNData.mouseWithinWin != win)
		{
			// leave current window
			if (_WNData.mouseWithinWin)
			{
				if (_WNData.mouseWithinWin->msgProc)
					UApplication::PostMessage(msg_MouseLeave, &info, sizeof(info), priority_MouseLeave, _WNData.mouseWithinWin->msgProc, _WNData.mouseWithinWin->msgProcContext, _WNData.mouseWithinWin);
				
				_WNData.mouseWithinWin = nil;
			}
			
			// enter new window
			_WNData.mouseWithinWin = win;
				
			if (_WNData.mouseWithinWin->msgProc)
				UApplication::PostMessage(msg_MouseEnter, &info, sizeof(info), priority_MouseEnter, _WNData.mouseWithinWin->msgProc, _WNData.mouseWithinWin->msgProcContext, _WNData.mouseWithinWin);

			if (inType == msg_MouseMove) 
				inType = 0;
		}
		
		if (inType == msg_MouseUp)	// mouse ups go to the window that the mouse went down in, regardless of where it came up
		{
			if (_WNData.mouseDownWin)
			{
				if (_WNData.mouseDownWin != win)
				{
					info.loc = inInfo.loc;
					UWindow::GlobalToLocal((TWindow)_WNData.mouseDownWin, info.loc);
				}
				
				win = _WNData.mouseDownWin;
				_WNData.mouseDownWin = nil;
				
				if (win->msgProc)
					UApplication::PostMessage(msg_MouseUp, &info, sizeof(info), priority_MouseUp, win->msgProc, win->msgProcContext, win);
			}
		}
		else if (inType == msg_MouseDown)
		{
			if (win)
			{
				if (win->layer == windowLayer_Modal)
				{
					if (UWindow::IsFront((TWindow)win))
						goto doAcceptMouseDown;
				}
				else
				{	
					UWindow::BringToFront((TWindow)win);
					
					if (!inResumeClick)	// if the click did not bring the app to the front
					{
						_WNCheckWinStates();
						_WNData.mouseDownWin = win;
						
						if (win->msgProc)
							UApplication::PostMessage(msg_MouseDown, &info, sizeof(info), priority_MouseDown, win->msgProc, win->msgProcContext, win);
					}
				}
			}
		}
		else if (inType == msg_MouseMove)
		{
			if (win->msgProc)
				UApplication::ReplaceMessage(msg_MouseMove, &info, sizeof(info), priority_MouseMove, win->msgProc, win->msgProcContext, win);
		}
	}
	else
	{
		part = ::FindWindow(mouseLoc, &macWin);
		win = macWin ? (SWindow *)::GetWRefCon(macWin) : nil;
		
		if (part == inContent)
		{
			if (win) UWindow::GlobalToLocal((TWindow)win, info.loc);
		
			// update mouse-within window
			if (_WNData.mouseWithinWin != win)
			{
				// leave current window
				if (_WNData.mouseWithinWin)
				{
					if (_WNData.mouseWithinWin->msgProc)
						UApplication::PostMessage(msg_MouseLeave, &info, sizeof(info), priority_MouseLeave, _WNData.mouseWithinWin->msgProc, _WNData.mouseWithinWin->msgProcContext, _WNData.mouseWithinWin);
					
					_WNData.mouseWithinWin = nil;
				}
				
				// enter new window
				if (win && (win->state == windowState_Active || win->state == windowState_Focus))
				{
					_WNData.mouseWithinWin = win;
					
					if (_WNData.mouseWithinWin->msgProc)
						UApplication::PostMessage(msg_MouseEnter, &info, sizeof(info), priority_MouseEnter, _WNData.mouseWithinWin->msgProc, _WNData.mouseWithinWin->msgProcContext, _WNData.mouseWithinWin);

					if (inType == msg_MouseMove) inType = 0;
				}
			}
			
			if (inType == msg_MouseUp)	// mouse ups go to the window that the mouse went down in, regardless of where it came up
			{
	doMouseUp:
				if (_WNData.mouseDownWin)
				{
					if (_WNData.mouseDownWin != win)
					{
						info.loc = inInfo.loc;
						UWindow::GlobalToLocal((TWindow)_WNData.mouseDownWin, info.loc);
					}
					
					win = _WNData.mouseDownWin;
					_WNData.mouseDownWin = nil;
					
					if (win->msgProc)
						UApplication::PostMessage(msg_MouseUp, &info, sizeof(info), priority_MouseUp, win->msgProc, win->msgProcContext, win);
				}
			}
			else if (inType == msg_MouseDown)
			{
				if (win)
				{					
					if (win->layer == windowLayer_Modal)
					{
						if (UWindow::IsFront((TWindow)win))
							goto doAcceptMouseDown;
					}
					else
					{
						UWindow::BringToFront((TWindow)win);
						
						if (!inResumeClick)	// if the click did not bring the app to the front
						{
				doAcceptMouseDown:
							_WNCheckWinStates();
							if (win->state == windowState_Active || win->state == windowState_Focus)
							{
								_WNData.mouseDownWin = win;
								
								if (win->msgProc)
									UApplication::PostMessage(msg_MouseDown, &info, sizeof(info), priority_MouseDown, win->msgProc, win->msgProcContext, win);
							}
						}
					}
				}
			}
			else if (inType == msg_MouseMove)
			{
				if (win && (win->state == windowState_Active || win->state == windowState_Focus))
				{
					if (win->msgProc)
						UApplication::ReplaceMessage(msg_MouseMove, &info, sizeof(info), priority_MouseMove, win->msgProc, win->msgProcContext, win);
				}
			}
		}
		else
		{
			// mouse is no longer in the content of a window
			if (_WNData.mouseWithinWin)
			{
				if (_WNData.mouseWithinWin->msgProc)
					UApplication::PostMessage(msg_MouseLeave, &info, sizeof(info), priority_MouseLeave, _WNData.mouseWithinWin->msgProc, _WNData.mouseWithinWin->msgProcContext, _WNData.mouseWithinWin);
				
				_WNData.mouseWithinWin = nil;
			}
				
			if (inType == msg_MouseDown)
			{				
				if (inResumeClick)	// if the click brought the app to the front
				{
					switch (part)
					{
						case inGrow:
						case inGoAway:
						case inZoomIn:
						case inZoomOut:
							part = inDrag;
							break;
					}
				}
				
				switch (part)
				{
					// user is moving window
					case inDrag:
					{
						// bring window to front if not modal and if command key is not down
						if (!(info.mods & modKey_Command) && win->layer != windowLayer_Modal)
							UWindow::BringToFront((TWindow)win);
										
					#if TARGET_API_MAC_CARBON	//??
						// check if the click brought the app to the front and if we are running under MacOS X
						if (inResumeClick && _GetSystemVersion() >= 0x0A00)
							UApplication::Process();	// this helps to fix a MacOS X window activation issue (see _ForceWindowActivation)
					#endif
					
						mouseLoc.h = info.loc.h;
						mouseLoc.v = info.loc.v;
						
						if (_WNTrackMove((TWindow)win, info.mods, mouseLoc, newLoc))
						{
							pt.h = newLoc.h;
							pt.v = newLoc.v;
							UWindow::SetLocation((TWindow)win, pt);
						}
					}
					break;
					
					// user is resizing window
					case inGrow:
					{
						Int32 newWidth, newHeight;
						
						mouseLoc.h = info.loc.h;
						mouseLoc.v = info.loc.v;

						if (_WNTrackGrow((TWindow)win, mouseLoc, newWidth, newHeight))
						{
							UWindow::GetBounds((TWindow)win, r);
							
							r.bottom = r.top + newHeight;
							r.right = r.left + newWidth;
							
							UWindow::SetBounds((TWindow)win, r);
						}
					}
					break;
					
					// user is closing window
					case inGoAway:
					{
						if (::TrackGoAway(macWin, mouseLoc))
						{
							if (win->msgProc)
								UApplication::PostMessage(msg_UserClose, &info, sizeof(info), priority_Normal, win->msgProc, win->msgProcContext, win);
						}
					}
					break;
					
					// user is zooming window
					case inZoomIn:
					case inZoomOut:
					{
						if (::TrackBox(macWin, mouseLoc, part))
						{
							if (win->msgProc)
								UApplication::PostMessage(msg_UserZoom, &info, sizeof(info), priority_Normal, win->msgProc, win->msgProcContext, win);
						}
					}
					break;
					
					// user is clicking in menubar
					case inMenuBar:
					{
						_DoMacMenuBar(mouseLoc);
					}
					break;
					
					// user is collapsing window
					case inCollapseBox:
					{
						if (win->msgProc) 
							UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
					}
					break;
				}
			}
			else if (inType == msg_MouseUp)
			{
				goto doMouseUp;
			}
		}
	}
}

// call to dispatch screen msg_KeyDown, msg_KeyUp
void _WNDispatchKey(Uint32 inType, const SKeyMsgData& inInfo)
{
	bool bPostKeyMessage = true;

	if (_WNData.visModalCount == 0)		// don't post msg_KeyCommand if there is a top modal window
	{
		Uint32 cmd = (inType == msg_KeyDown) ? UKeyboard::FindKeyCommand(inInfo, _WNData.keyCmds, _WNData.keyCmdCount) : 0;
	
		if (cmd)
		{
			bPostKeyMessage = false;
			
			Uint8 buf[64];
			*(Uint32 *)buf = cmd;
			*(SKeyMsgData *)(buf+sizeof(Uint32)) = inInfo;
		
			UApplication::PostMessage(msg_KeyCommand, buf, sizeof(Uint32) + sizeof(SKeyMsgData), priority_KeyDown, _WNData.keyCmdMsgProc, _WNData.keyCmdMsgProcContext);
		}
	}
		
	if (bPostKeyMessage)
	{
		SWindow *win = (SWindow *)UWindow::GetFocus();
		
		if (win && win->msgProc)
			UApplication::PostMessage(inType, &inInfo, sizeof(SKeyMsgData), priority_KeyDown, win->msgProc, win->msgProcContext, win);
	}
}

void _WNDispatchDrag(Uint32 inType, const SDragMsgData& inInfo, WindowPtr inWin)
{
#if !TARGET_API_MAC_CARBON
	#pragma unused(inWin)
#endif
	
	SDragMsgData info = inInfo;
	WindowRef macWin = nil;
	SWindow *win = nil;
	
	Point mouseLoc;
	mouseLoc.v = info.loc.v;
	mouseLoc.h = info.loc.h;

	if (inType == msg_DragBegin)
		_WNData.dragWithinWin = nil;
	
	if (inType != msg_DragEnd)
	{
		if (::FindWindow(mouseLoc, &macWin) != inContent)
		{
		#if TARGET_API_MAC_CARBON
			// sometimes FindWindow() fails under MacOS X, so we use the window received from the drag & drop handler instead
			if (!macWin && inWin)
				macWin = inWin;
			else
		#endif
				macWin = nil;
		}
		
		win = macWin ? (SWindow *)::GetWRefCon(macWin) : nil;
	}
	
	if (win == _WNData.dragWithinWin)
	{
		// move within current window
		if (inType != msg_DragDropped && win && win->msgProc)	// don't do this if we just dropped
		{
			info.loc = inInfo.loc;
			UWindow::GlobalToLocal((TWindow)win, info.loc);
			UApplication::PostMessage(msg_DragMove, &info, sizeof(info), priority_High, win->msgProc, win->msgProcContext, win);
		}
	}
	else
	{
		// leave current window
		if (_WNData.dragWithinWin)
		{
			SWindow *tmpWin = _WNData.dragWithinWin;
			_WNData.dragWithinWin = nil;
			
			if (tmpWin->msgProc)
			{
				info.loc = inInfo.loc;
				UWindow::GlobalToLocal((TWindow)tmpWin, info.loc);
				UApplication::PostMessage(msg_DragLeave, &info, sizeof(info), priority_High, tmpWin->msgProc, tmpWin->msgProcContext, tmpWin);
			}
		}
		
		// enter new window
		if (win && inType != msg_DragDropped)	// if we've dropped, don't enter a new win
		{
			if (win->msgProc)
			{
				info.loc = inInfo.loc;
				UWindow::GlobalToLocal((TWindow)win, info.loc);
				UApplication::PostMessage(msg_DragEnter, &info, sizeof(info), priority_High, win->msgProc, win->msgProcContext, win);
			}
			
			_WNData.dragWithinWin = win;
		}
	}
	
	// check if drag has dropped
	if (inType == msg_DragDropped && _WNData.dragWithinWin && _WNData.dragWithinWin->msgProc)
	{
		info.loc = inInfo.loc;
		UWindow::GlobalToLocal((TWindow)_WNData.dragWithinWin, info.loc);
		UApplication::PostMessage(msg_DragDropped, &info, sizeof(info), priority_Normal, _WNData.dragWithinWin->msgProc, _WNData.dragWithinWin->msgProcContext, _WNData.dragWithinWin);
	}
}

static void _SetupMacMenuBar()
{
	// get the OS version
	Int16 nVersion = _GetSystemVersion();
	
	if (gMacMenuBarID == 0)
	{
		// check if we are not running under MacOS X
		if (nVersion < 0x0A00)
		{
			// set apple menu
			MenuHandle appleMenu = ::NewMenu(128, "\p\024");
			::AppendMenu(appleMenu, "\pAbout This Computer...");
		#if !TARGET_API_MAC_CARBON	
			// this is done automatically for us in Carbon
			::AppendResMenu(appleMenu, 'DRVR');
		#endif
			::InsertMenu(appleMenu, 0);
		
			// add "File" menu
			MenuHandle fileMenu = ::NewMenu(129, "\pFile");
			::AppendMenu(fileMenu, "\pQuit/Q");
			::InsertMenu(fileMenu, 0);
		}
		
		// add "Edit" menu
		MenuHandle editMenu = ::NewMenu(130, "\pEdit");
		::AppendMenu(editMenu, "\pUndo/Z;-;Cut/X;Copy/C;Paste/V;Clear;-;Select All/A");
		::InsertMenu(editMenu, 0);
	}
	else
	{
		// set menu bar
		MenuBarHandle theMenuBar = NULL;
	#if TARGET_API_MAC_CARBON
		// check if we are running under MacOS X
		if (nVersion >= 0x0A00)
			theMenuBar = ::GetNewMBar(gMacMenuBarID + 1);
		else
	#endif
			theMenuBar = ::GetNewMBar(gMacMenuBarID);
		
		if (!theMenuBar)
		{
			DebugBreak("Could not load MBAR resource!");
			Fail(errorType_Misc, error_Unknown);
		}
		
		::SetMenuBar(theMenuBar);
		::DisposeHandle(theMenuBar);
		
	#if !TARGET_API_MAC_CARBON	
		// set apple menu
		MenuHandle appleMenu = ::GetMenuHandle(128);
		if (appleMenu)
		{
			_WNMacAppleMenuSize = ::CountMItems(appleMenu);
		
			// this is done automatically for us in Carbon
			::AppendResMenu(appleMenu, 'DRVR');
		}
	#endif		
	}

#if TARGET_API_MAC_CARBON	
	// check if we are running under MacOS X
	if (nVersion >= 0x0A00)
		::EnableMenuCommand(NULL, 'pref');	// enable "Preferences" from the Application menu
#endif
	
	::DrawMenuBar();
}

void _BringProcessToFrontBySig(Uint32 inSignature);

void _DoAboutComputer()
{
	AEAddressDesc finderTarg;
	AppleEvent ae, aeReply;
	Uint32 finderSig = 'MACS';
				
	if (::AECreateDesc(typeApplSignature, &finderSig, sizeof(finderSig), &finderTarg) == 0)
	{
		if (::AECreateAppleEvent(kCoreEventClass, 'abou', &finderTarg, kAutoGenerateReturnID, kAnyTransactionID, &ae) == 0)
		{
			::AESend(&ae, &aeReply, kAENoReply | kAECanSwitchLayer, kAENormalPriority, 60, nil, nil);
			::AEDisposeDesc(&ae);
			
			_BringProcessToFrontBySig(finderSig);
		}
					
		::AEDisposeDesc(&finderTarg);
	}
}

static void _DoMacMenuBar(Point startPt)
{
	Uint32 menuAndItem = ::MenuSelect(startPt);
	Int16 menu = HiWord(menuAndItem);
	Int16 item = LoWord(menuAndItem);

#if !TARGET_API_MAC_CARBON	
	Str255 str;
#endif
	
	if (menu == 0 || item == 0) 
		return;
	
	if (gMacMenuBarID == 0)
	{
		if (menu == 128)
		{
			if (item == 1)
				_DoAboutComputer();
		#if !TARGET_API_MAC_CARBON
			else
			{
				// Desk accessories are not supported in Carbon
				::GetMenuItemText(GetMenuHandle(128), item, str);
				::OpenDeskAcc(str);
			}
		#endif
		}
		else if (menu == 129)
		{
			if (item == 1)
				UApplication::PostMessage(msg_Quit, nil, 0, priority_Quit);
		}
	}
	else
	{
	#if TARGET_API_MAC_CARBON
		UApplication::PostMessage(999, &menuAndItem, sizeof(menuAndItem));
	#else	
		if (menu != 128 || item <= _WNMacAppleMenuSize)
			UApplication::PostMessage(999, &menuAndItem, sizeof(menuAndItem));
		else
		{
			// Desk accessories are not supported in Carbon
			::GetMenuItemText(GetMenuHandle(128), item, str);
			::OpenDeskAcc(str);
		}
	#endif
	}
}

void _SearchAndRedrawWindow(WindowPtr inWindow)
{
	if (!inWindow)
		return;

	Uint32 i = 0;
	CWindow *win = nil;
	
	while (gWindowList.GetNextWindow(win, i))
	{
		SWindow *pWin = (SWindow*)((TWindow)*win);
		if (pWin->macWin == inWindow)
		{
			// get window bounds
			SRect rBounds;
			UWindow::GetBounds((TWindow)pWin, rBounds);									
			
			// force the window to be redrawn
			pWin->needsRedraw = true;
			pWin->redrawRect.left = pWin->redrawRect.top = 0;
			pWin->redrawRect.right = rBounds.GetWidth();
			pWin->redrawRect.bottom = rBounds.GetHeight();
			UApplication::SendMessage(msg_Draw, nil, 0, priority_Draw, pWin->msgProc, pWin->msgProcContext, pWin);
			
			break;
		}
	}
}

void _DeleteFromParentList(SWindow *inWindow)
{
	if (!inWindow)
		return;
	
	Uint32 i = 0;
	CWindow *win = nil;
	
	while (gWindowList.GetNextWindow(win, i))
		((SWindow*)((TWindow)*win))->childrenList.RemoveItem(inWindow);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

#if TARGET_API_MAC_CARBON
bool _ForceWindowActivation(SWindow *inWin)		//??
{
	if (!inWin || !inWin->macWin || !::IsWindowVisible(inWin->macWin))
		return false;
	
	WindowRef curWin = ::FrontWindow(), prevWin = nil;
	if (!curWin || curWin == inWin->macWin)
		return false;
						
	for (;;)
	{			
		if (::IsWindowVisible(curWin))	// ignore invisible windows
			prevWin = curWin;
		
		curWin = ::GetNextWindow(curWin);
		if (!curWin)
			break;
							
		if (curWin == inWin->macWin)
		{
			if (!prevWin)
				break;
			
			// this is a hack which fixes a MacOS X window activation issue
			::SendBehind(prevWin, inWin->macWin);
			::SendBehind(inWin->macWin, prevWin);
			
			return true;
		}
	}
	
	return false;
}

bool _InstallEventHandler(SWindow *inWin)
{
	if (!inWin || !inWin->macWin)
		return false;
	
	EventTypeSpec eventSpec[] = {{kEventClassWindow, kEventWindowActivated},
								 {kEventClassWindow, kEventWindowFocusAcquired}, 
								 {kEventClassWindow, kEventWindowCollapsed}, 
								 {kEventClassWindow, kEventWindowExpanded}};

	if (::InstallWindowEventHandler(inWin->macWin, _EventHandlerProcUPP, sizeof(eventSpec)/sizeof(EventTypeSpec), eventSpec, inWin, nil) != noErr)
		return false;
		
	return true;
}

pascal OSStatus _EventHandlerProc(EventHandlerCallRef inHandler, EventRef inEvent, void *inUserData)
{
	#pragma unused(inHandler)
	
	switch (::GetEventClass(inEvent))
	{
		case kEventClassWindow:
			switch (::GetEventKind(inEvent))
			{
				case kEventWindowCollapsed:
					// check if we are running under MacOS X
					if (_GetSystemVersion() >= 0x0A00)
					{
						if (_WNData.focusWin == (SWindow *)inUserData)
							_WNData.focusWin = nil;
					}
					// don't break
				case kEventWindowExpanded:
					_WNCheckCollapseState((TWindow)inUserData);
					break;
					
				case kEventWindowFocusAcquired:
					_WNCheckCollapseState((TWindow)inUserData);
					break;
					
				case kEventWindowActivated:
					// check if we are running under MacOS X
					if (_APInBkgndFlag && _GetSystemVersion() >= 0x0A00)	//??
						_ForceWindowActivation((SWindow *)inUserData);
					break;
			};
			break;
	};
	
	return eventNotHandledErr;
}
#endif

/* -------------------------------------------------------------------------- */
#pragma mark -

void _RegisterWithQuickTime(TWindow inWin)
{
	Require(inWin);
	
	if (!WIN->isQuickTimeView)
	{
		WIN->isQuickTimeView = true;
	
		// set the port
	#if TARGET_API_MAC_CARBON	
		::MacSetPort(::GetWindowPort(WIN->macWin));	
	#else
		::MacSetPort(WIN->macWin);	
	#endif
	}
}

void _InitQuickTime(TWindow inWin, Movie inMovie)
{
	Require(inWin);

	// resize the movie bounding rect and offset to 0,0
	Rect stBounds;	
	::GetMovieBox(inMovie, &stBounds);
	::MacOffsetRect(&stBounds, -stBounds.left, -stBounds.top);
	::SetMovieBox(inMovie, &stBounds);
	::AlignWindow(WIN->macWin, false, &stBounds, nil);

#if TARGET_API_MAC_CARBON
	::SetMovieGWorld(inMovie, ::GetWindowPort(WIN->macWin), ::GetGWorldDevice(::GetWindowPort(WIN->macWin)));
#else
	::SetMovieGWorld(inMovie, (CGrafPtr)WIN->macWin, ::GetGWorldDevice((CGrafPtr)WIN->macWin));
#endif
}

void _UpdateQuickTime(TWindow inWin, MovieController inController, Movie inMovie, bool inGrowBox)
{
	Require(inWin);

	// update grow box
	if (inGrowBox)
		_WNDrawGrowBox(inWin);

	// draw the movie controler and its movie
	::MCDoAction(inController, mcActionDraw, WIN->macWin);

	// force the movie to be redrawn
	::MoviesTask(inMovie, 0);
}

bool _SendToQuickTime(EventRecord& inEvent)
{
	bool bSentToQuickTime = false;
	
	WindowRef macWin = ::FrontWindow();
	
	while (macWin)
	{
		SWindow *win = (SWindow *)::GetWRefCon(macWin);
				
		// if this window contain a CQuickTimeView must pass all events to QuickTime
		if (win && win->isQuickTimeView && UWindow::IsVisible((TWindow)win) && !UWindow::IsCollapsed((TWindow)win))
		{
			UApplication::SendMessage(msg_SendToQuickTime, &inEvent, sizeof(inEvent), priority_Draw, win->msgProc, win->msgProcContext, win);
			bSentToQuickTime = true;
		}
		
		macWin = ::GetNextWindow(macWin);
	}	
	
	return bSentToQuickTime;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static bool _CydoorServiceShow(CWindow */*inWindow*/, const SRect& /*inBounds*/)
{
	return false;
}

bool _CydoorServiceSetBounds(CWindow */*inWindow*/, const SRect& /*inBounds*/)
{
	return false;
}

static bool _CydoorServiceClose(CWindow */*inWindow*/)
{
	return false;	
}

#endif /* MACINTOSH */
