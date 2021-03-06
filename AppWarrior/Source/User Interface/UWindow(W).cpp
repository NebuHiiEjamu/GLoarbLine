#if WIN32



/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UWindow.h"
#include "UMemory.h"
#include "Resource.h"


#define DRAW_OFFSCREEN		1
#define RGBColor(c)		PALETTERGB((c).red >> 8, (c).green >> 8, (c).blue >> 8)

struct SWindow {
	HWND hwnd;
	SWindow *next;
	
	void *userRef;
	TMessageProc msgProc;
	void *msgProcContext;
	SRect redrawRect;
	
	TImage img;
#if DRAW_OFFSCREEN
	HDC onscreenDC, offscreenDC;
	HBITMAP offscreenBM;
	SRect beginDrawRect;
	HBRUSH backColBrush;
#endif

	Int16 minHeight, minWidth, maxHeight, maxWidth;

	RECT beforeMyMaximizeRect;
	Uint8 frameSizeLeft, frameSizeTop, frameSizeRight, frameSizeBottom;
	Uint16 options, style;
	Uint8 layer, state;
	Uint8 needsRedraw;
	Uint8 isMyMaximized;
	Uint8 isQuickTimeView;
};

#define REF		((SWindow *)inRef)

extern HINSTANCE _gProgramInstance;

static LRESULT CALLBACK _WNWndProc(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam);
static LRESULT CALLBACK _WNFrameWndProc(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam);

HDC _ImageToDC(TImage inImage);
TImage _NewDCImage(HDC inDC, Uint32 inWidth, Uint32 inHeight);
void _DisposeDCImage(TImage inImage);
Uint16 _WinKeyCodeToStd(Uint16 inWinKC);

void _FailLastWinError(const Int8 *inFile, Uint32 inLine);
void _FailWinError(Int32 inWinError, const Int8 *inFile, Uint32 inLine);

void _RemoveFromLayer(SWindow *inRef);
void _PutInLayer(SWindow *inRef);
HWND _AddToLayerAndGetInsertAfter(SWindow *inRef, HWND inInsertAfterWin);
SWindow *_GetTopVisibleModalWindow();
HWND _GetPrevVisibleWindow(SWindow *inRef, HWND inDefault);
HWND _GetBottomWindow(SWindow *inRef);
bool _IsTopVisibleModalWindow();
bool _IsInWindowList(HWND inHwnd);
void _CheckWindowStates();
bool _StandardDialogBox(Uint8 inActive = 0);

void _DDRegisterWinForDrop(HWND inWin, void *inRef);
static void _WNCalcUnobstructedBounds(TWindow inRef, SRect& rBounds);

static bool _SendToQuickTime(TWindow inRef, UINT inMsg, WPARAM inWParam, LPARAM inLParam);
static void _UnregisterWithQuickTime(TWindow inRef);


#if DEBUG
	#define FailLastWinError()		_FailLastWinError(__FILE__, __LINE__)
	#define FailWinError(id)		_FailWinError(id, __FILE__, __LINE__)
#else
	#define FailLastWinError()		_FailLastWinError(nil, 0)
	#define FailWinError(id)		_FailWinError(id, nil, 0)
#endif

static ATOM _gWinClassAtom = 0;
static SWindow *_gMainWindow = nil;
static SWindow *_gMouseWithinWin = nil;
static SWindow *_gMouseCaptureWin = nil;
static HWND _gModalParent = 0;
static HWND _gMDIFrame = 0;
static HWND _gMDIClient = 0;
static HCURSOR _gArrowCursor = 0;

Uint8 _gUseMDIWindow = 0;
Uint8 _gMDIWindowMenu = 0;
Uint8 _APInBkgndFlag = false;

extern HPALETTE _MAC_PALETTE;
extern int _gStartupShowCmd;

static struct {
	SKeyCmd *keyCmds;
	Uint32 keyCmdCount;
	TMessageProc keyCmdMsgProc;
	void *keyCmdMsgProcContext;
} _WNData = { 0 };

extern TWindow _TTWin;
static SWindow *_gTopWin = nil;
static SWindow *_gWinToBeActivated = nil;
static bool _gAppDeactivated = false;	// whether this app was just deactivated
static bool _gAppPosChanging = false;

static TTimer _gStateTimer = nil;
static void _StateTimerProc(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);

static TTimer _gFlashTimer = nil;
static void _FlashTimerProc(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);


enum
{
	windowState_Deleting		= 0
};

/* -------------------------------------------------------------------------- */

static void _UninitWindow()
{
	UTimer::Dispose(_gStateTimer);
	_gStateTimer = nil;
	
	UTimer::Dispose(_gFlashTimer);
	_gFlashTimer = nil;
}

void UWindow::Init()
{
	if (_gArrowCursor == 0)
		_gArrowCursor = ::LoadCursor(NULL, IDC_ARROW);
	
	if (_gUseMDIWindow)
	{
		WNDCLASSEX wndclass;
		ATOM frameClassAtom;
		
		// register frame window class
		wndclass.cbSize = sizeof(WNDCLASSEX);
		wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// CS_OWNDC must be on for color palettes to work 
		wndclass.lpfnWndProc = _WNFrameWndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = _gProgramInstance;
		wndclass.hIcon = ::LoadIcon(_gProgramInstance, (LPCTSTR)IDI_MDIICON);
		if (wndclass.hIcon == NULL) wndclass.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "UWindow-MDIFrame";
		wndclass.hIconSm = wndclass.hIcon;
		frameClassAtom = RegisterClassEx(&wndclass);
		if (frameClassAtom == 0) FailLastWinError();
		
		// load string for frame window title
		char str[256];
		if (::LoadString(_gProgramInstance, 4096, str, sizeof(str)) == 0)
			str[0] = 0;
		
		// create the frame window
		HMENU menuBar = ::LoadMenu(_gProgramInstance, "MDIMenu");
		_gMDIFrame = ::CreateWindow((char *)frameClassAtom, str, WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, menuBar, _gProgramInstance, NULL);
		if (_gMDIFrame == NULL) FailLastWinError();
		
		// create the client window
		CLIENTCREATESTRUCT mdiData = { 0, 0 };
		_gMDIClient = ::CreateWindowEx(WS_EX_CLIENTEDGE, "MDICLIENT", NULL, WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_HSCROLL|WS_VSCROLL|WS_VISIBLE|MDIS_ALLCHILDSTYLES, 0, 0, 0, 0, _gMDIFrame, NULL, _gProgramInstance, &mdiData);
		if (_gMDIClient == NULL) FailLastWinError();		
		
		// tell MDI window which is the "Window" menu
		if (menuBar && _gMDIWindowMenu)
		{
			HMENU windowMenu = ::GetSubMenu(menuBar, _gMDIWindowMenu-1);
			if (windowMenu)
				::SendMessageA(_gMDIClient, WM_MDISETMENU, NULL, (LPARAM)windowMenu);
			else
				_gMDIWindowMenu = 0;
		}
		
		// show the frame window
		::ShowWindow(_gMDIFrame, _gStartupShowCmd/*SW_SHOW*/);
		::UpdateWindow(_gMDIFrame);
	}
	else
		_gModalParent = ::CreateWindowEx(0, "STATIC", "", WS_EX_DLGMODALFRAME, 0, 0, 0, 0, NULL, NULL, _gProgramInstance, NULL);
	
	_gStateTimer = UTimer::New(_StateTimerProc, nil);
	_gFlashTimer = UTimer::New(_FlashTimerProc, nil);


	UProgramCleanup::InstallAppl(_UninitWindow);
}

TWindow UWindow::New(const SRect& inBounds, Uint16 inLayer, Uint16 inOptions, Uint16 inStyle, TWindow inParent)
{
	HWND hwnd = 0;
	SWindow *ref = 0;
	RECT adjustedRect;
	DWORD dwExStyle;
	DWORD dwWStyle;
		
	switch (inLayer)
	{
		case windowLayer_Popup:
			if (_gMDIClient)
			{
				dwWStyle = WS_DLGFRAME;
				dwExStyle = 0;
			}
			else
			{
				if (inStyle == 1)	// tooltip
					dwWStyle = WS_POPUP;
				else
					dwWStyle = WS_POPUP | WS_BORDER | WS_THICKFRAME;	

				dwExStyle = 0;
			}
			break;
			
		case windowLayer_Modal:
			if (inOptions & windowOption_Sizeable)
			{
				dwWStyle = WS_OVERLAPPEDWINDOW;
				dwExStyle = 0;
			}
			else
			{
				dwWStyle = WS_CAPTION;
				dwExStyle = WS_EX_DLGMODALFRAME;
			}
			break;
			
		case windowLayer_Float:
			if (inOptions & windowOption_Sizeable)
			{
				if (inStyle == 1)
				{
					dwWStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX;
					dwExStyle = WS_EX_DLGMODALFRAME;
				}
				else
				{
					dwWStyle = WS_OVERLAPPEDWINDOW;
					dwExStyle = WS_EX_TOOLWINDOW;
				}
			}
			else
			{
				if (inStyle == 1)
				{
					dwWStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
					dwExStyle = WS_EX_DLGMODALFRAME;
				}
				else
				{
					dwWStyle = WS_CAPTION;
					dwExStyle = WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME;
				}
			}
			break;
			
		default:	// windowLayer_Standard
			if (inOptions & windowOption_Sizeable)
			{
				dwWStyle = WS_OVERLAPPEDWINDOW;
				dwExStyle = WS_EX_OVERLAPPEDWINDOW;
			}
			else
			{
				dwWStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
				dwExStyle = WS_EX_OVERLAPPEDWINDOW | WS_EX_DLGMODALFRAME;
			}
			break;
	};
	
	// if not already created, create window class for UWindow windows
	if (_gWinClassAtom == 0)
	{
		WNDCLASSEX wndclass;		
		
		// fill in window class info
		wndclass.cbSize = sizeof(WNDCLASSEX);
		wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;	// CS_OWNDC must be on for color palettes to work 
		wndclass.lpfnWndProc = _WNWndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = _gProgramInstance;
		wndclass.hIcon = ::LoadIcon(_gProgramInstance, (LPCTSTR)IDI_APPICON);
		if (wndclass.hIcon == NULL) wndclass.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);	has to be NULL otherwise windoze continually changes cursor back to arrow
	#if DRAW_OFFSCREEN
		wndclass.hbrBackground = NULL;
	#else
		wndclass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	#endif
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "UWindow";
		wndclass.hIconSm = wndclass.hIcon;
	
		// register window class
		_gWinClassAtom = RegisterClassEx(&wndclass);
		if (_gWinClassAtom == 0) FailLastWinError();
	}
	
	try
	{
		// allocate struct to store window info
		ref = (SWindow *)UMemory::NewClear(sizeof(SWindow));
		ref->options = inOptions;
		ref->style = inStyle;
		ref->layer = inLayer;
		ref->state = windowState_Hidden;
		
		// calculate rectangle for window
		adjustedRect.left = inBounds.left;
		adjustedRect.top = inBounds.top;
		adjustedRect.right = inBounds.right;
		adjustedRect.bottom = inBounds.bottom;
		if (inLayer == windowLayer_Float)
		{
			// work around bug (?) in windoze
			if (!::AdjustWindowRectEx(&adjustedRect, WS_OVERLAPPEDWINDOW, false, WS_EX_TOOLWINDOW))
				FailLastWinError();
		}
		else
		{
			if (!::AdjustWindowRectEx(&adjustedRect, dwWStyle, false, dwExStyle))
				FailLastWinError();
		}
		
		// save the size of the frame for future bounds calculations
		ref->frameSizeLeft = inBounds.left - adjustedRect.left;
		ref->frameSizeTop = inBounds.top - adjustedRect.top;
		ref->frameSizeRight = adjustedRect.right - inBounds.right;
		ref->frameSizeBottom = adjustedRect.bottom - inBounds.bottom;

		ref->minHeight = ref->minWidth = 100;
		ref->maxHeight = ref->maxWidth = 30000;

		// create the window
		if (_gMDIClient)
		{/*
			MDICREATESTRUCT mdicreate;
			
			mdicreate.szClass = (char *)_gWinClassAtom;
			mdicreate.szTitle = "";
			mdicreate.hOwner = _gProgramInstance;
			mdicreate.x = adjustedRect.left;
			mdicreate.y = adjustedRect.top;
			mdicreate.cx = adjustedRect.right - adjustedRect.left;
			mdicreate.cy = adjustedRect.bottom - adjustedRect.top;
			mdicreate.style = 0;	// dwWStyle|WS_CHILD
			mdicreate.lParam = 0;
			
			hwnd = (HWND)SendMessageA(_gMDIClient, WM_MDICREATE, 0, (LPARAM)&mdicreate);*/
			
			hwnd = ::CreateWindowEx(dwExStyle | WS_EX_MDICHILD, (char *)_gWinClassAtom, "", dwWStyle | WS_CHILD, adjustedRect.left, adjustedRect.top, adjustedRect.right - adjustedRect.left, adjustedRect.bottom - adjustedRect.top, _gMDIClient, NULL, _gProgramInstance, NULL);
		}
		else if (inParent)
			hwnd = ::CreateWindowEx(dwExStyle, (char *)_gWinClassAtom, "", dwWStyle, adjustedRect.left, adjustedRect.top, adjustedRect.right - adjustedRect.left, adjustedRect.bottom - adjustedRect.top, ((SWindow *)inParent)->hwnd, NULL, _gProgramInstance, NULL);
		else if (inLayer == windowLayer_Modal || (inLayer == windowLayer_Popup && inStyle == 1))	// don't display modal windows and tooltips in the taskbar
			hwnd = ::CreateWindowEx(dwExStyle, (char *)_gWinClassAtom, "", dwWStyle, adjustedRect.left, adjustedRect.top, adjustedRect.right - adjustedRect.left, adjustedRect.bottom - adjustedRect.top, _gModalParent, NULL, _gProgramInstance, NULL);
		else
			hwnd = ::CreateWindowEx(dwExStyle, (char *)_gWinClassAtom, "", dwWStyle, adjustedRect.left, adjustedRect.top, adjustedRect.right - adjustedRect.left, adjustedRect.bottom - adjustedRect.top, NULL, NULL, _gProgramInstance, NULL);

		if (hwnd == 0) FailLastWinError();
		ref->hwnd = hwnd;
		::SetWindowLong(hwnd, GWL_USERDATA, (LONG)ref);
		
		if (ref->layer != windowLayer_Popup)
			_DDRegisterWinForDrop(hwnd, ref);
				
		// put this in the linked list
		_PutInLayer(ref);
	}
	catch(...)
	{
		if (hwnd) ::DestroyWindow(hwnd);
		UMemory::Dispose((TPtr)ref);
		throw;
	}
	
	return (TWindow)ref;
}

void UWindow::Dispose(TWindow inRef)
{
	// what happens if we destroy the window but there's still a message waiting in the message queue?
	// should we null-ize the msg proc and purge msgs from this win
	// FlushMessages should take care of that
	if (inRef)
	{
		REF->state = windowState_Deleting;
		
		// unregister this window with QuickTime
		if (UOperatingSystem::IsQuickTimeAvailable())
			_UnregisterWithQuickTime(inRef);

		if (REF->backColBrush)
			::DeleteObject(REF->backColBrush);

		::DestroyWindow(REF->hwnd);
		
		_RemoveFromLayer(REF);
		UApplication::FlushMessages(REF->msgProc, REF->msgProcContext, inRef);

		if (_gMainWindow == REF)
			_gMainWindow = nil;
		
		if (_gMouseWithinWin == REF)
			_gMouseWithinWin = nil;
		
		if (_gMouseCaptureWin == REF)
			_gMouseCaptureWin = nil;

		UMemory::Dispose((TPtr)inRef);
				
		if (_gMDIClient && _gMDIWindowMenu) 
			::SendMessageA(_gMDIClient, WM_MDIREFRESHMENU, 0, 0);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetTitle(TWindow inRef, const Uint8 *inTitle)
{
	Int8 str[256];
	
	Require(inRef);
	
	// convert to C-string
	::CopyMemory(str, inTitle+1, inTitle[0]);
	str[inTitle[0]] = 0;
	
	if (!SetWindowText(REF->hwnd, str))
		FailLastWinError();
}

void UWindow::SetNoTitle(TWindow inRef)
{
	Require(inRef);
	
	if (!SetWindowText(REF->hwnd, ""))
		FailLastWinError();
}

Uint32 UWindow::GetTitle(TWindow inRef, void *outText, Uint32 inMaxSize)
{
	Require(inRef);
	return GetWindowText(REF->hwnd, (LPTSTR)outText, inMaxSize);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetRef(TWindow inRef, void *inUserRef)
{
	Require(inRef);
	REF->userRef = inUserRef;
}

void *UWindow::GetRef(TWindow inRef)
{
	Require(inRef);
	return REF->userRef;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetVisible(TWindow inRef, Uint16 inVisible)
{
	Require(inRef);
	
	if (inVisible == kShowWinAtBack)
		::SetWindowPos(REF->hwnd, _GetBottomWindow(REF), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	else if (inVisible == kHideWindow)
		::ShowWindow(REF->hwnd, SW_HIDE);
	else if (_APInBkgndFlag)
		::ShowWindow(REF->hwnd, SW_SHOWNA);
	else	// kShowWinAtFront
	{
		Uint32 nActive = (inRef == _TTWin ? SWP_NOACTIVATE : nil);
		HWND hwndInsertAfter = HWND_TOP;
		
		if (REF->layer == windowLayer_Float || REF->layer == windowLayer_Standard)
		{
			HWND hwndTopWin = ::GetActiveWindow();
			
			SWindow *pRef = (SWindow *)::GetWindowLong(hwndTopWin, GWL_USERDATA);
			if (pRef && pRef->layer == windowLayer_Modal)
			{
				hwndInsertAfter = hwndTopWin;
				nActive = SWP_NOACTIVATE;
			}		
		}
		
		::SetWindowPos(REF->hwnd, hwndInsertAfter, 0, 0, 0, 0, nActive | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	}

	_CheckWindowStates();

	if (_gMDIClient && _gMDIWindowMenu) 
		::SendMessageA(_gMDIClient, WM_MDIREFRESHMENU, 0, 0);
}

bool UWindow::IsVisible(TWindow inRef)
{
	Require(inRef);
	return ::IsWindowVisible(REF->hwnd);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool UWindow::IsChild(TWindow inParent, TWindow inChild)
{
	Require(inParent && inChild);
	
	return (::GetWindow(((SWindow *)inChild)->hwnd, GW_OWNER) == ((SWindow *)inParent)->hwnd);
}

// the list will have front to back order
bool UWindow::GetChildrenList(TWindow inParent, CPtrList<CWindow>& outChildrenList)
{
	Require(inParent);

	SWindow *win = _gTopWin;

	while (win)
	{
		if (IsChild(inParent, (TWindow)win))
			outChildrenList.AddItem((CWindow *)win->userRef);
						
		win = win->next;
	}

	if (!outChildrenList.GetItemCount())
		return false;
		
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetBackColor(TWindow inRef, const SColor& inColor)
{
	Require(inRef);
	if (REF->backColBrush)
		::DeleteObject(REF->backColBrush);
		
	COLORREF color = RGBColor(inColor);
	HBRUSH brush = CreateSolidBrush(color);
	REF->backColBrush = brush;
}

void UWindow::GetBackColor(TWindow inRef, SColor& outColor)
{
	Require(inRef);
	outColor.Set(0xFFFF);
}

void UWindow::SetBackPattern(TWindow inRef, Int16 inID)
{
	#pragma unused(inID)
	Require(inRef);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetLayer(TWindow inRef, Uint16 inLayer)
{
	#pragma unused(inRef, inLayer)
	Fail(errorType_Misc, error_Unimplemented);
}

Uint16 UWindow::GetLayer(TWindow inRef)
{
	Require(inRef);
	return REF->layer;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// set the size of the content area of the window
void UWindow::SetBounds(TWindow inRef, const SRect& inBounds)
{
	Require(inRef);
	_gAppPosChanging = true;
	bool bRet = ::MoveWindow(REF->hwnd, 
					inBounds.left - REF->frameSizeLeft, 
					inBounds.top - REF->frameSizeTop, 
					inBounds.GetWidth() + REF->frameSizeLeft + REF->frameSizeRight, 
					inBounds.GetHeight() + REF->frameSizeTop + REF->frameSizeBottom, true);
	_gAppPosChanging = false;
	
	if (!bRet)
		FailLastWinError();
}

// gets the bounds of the content area of the window
void UWindow::GetBounds(TWindow inRef, SRect& outBounds)
{
	Require(inRef);

	WINDOWPLACEMENT pm;
	pm.length = sizeof(pm);
	
	// don't use GetWindowRect, it gives wrong rect when window is minimized, also non-local coords in MDI window
	if (!::GetWindowPlacement(REF->hwnd, &pm))
		FailLastWinError();
	
	if (pm.showCmd == SW_SHOWMAXIMIZED)
	{
		::GetClientRect(REF->hwnd, (RECT *)&outBounds);
		outBounds.Move(pm.ptMaxPosition.x + REF->frameSizeLeft, pm.ptMaxPosition.y + REF->frameSizeTop);
	}
	else
	{
		outBounds.left = pm.rcNormalPosition.left + REF->frameSizeLeft;
		outBounds.top = pm.rcNormalPosition.top + REF->frameSizeTop;
		outBounds.right = pm.rcNormalPosition.right - REF->frameSizeRight;
		outBounds.bottom = pm.rcNormalPosition.bottom - REF->frameSizeBottom;
	}
}

// sets the topLeft corner of the content area of the window
void UWindow::SetLocation(TWindow inRef, SPoint& inTopLeft)
{
	Require(inRef);

	RECT r;
	if (!::GetWindowRect(REF->hwnd, &r))
		FailLastWinError();
	
	_gAppPosChanging = true;
	bool bRet = ::MoveWindow(REF->hwnd, inTopLeft.x - REF->frameSizeTop, inTopLeft.y - REF->frameSizeLeft, r.right - r.left, r.bottom - r.top, true);
	_gAppPosChanging = false;

	if (!bRet)
		FailLastWinError();
}

void UWindow::SetLimits(TWindow inRef, Int32 inMinWidth, Int32 inMinHeight, Int32 inMaxWidth, Int32 inMaxHeight)
{
	Require(inRef);
	
	// we limit the minimums to 15 to avoid drawing problems
	REF->minWidth = max(inMinWidth, (Int32)15);
	REF->minHeight = max(inMinHeight, (Int32)15);
	
	// we limit the maximums to 30K to avoid potential Int16 integer overflows
	REF->maxWidth = min(inMaxWidth, (Int32)30000);
	REF->maxHeight = min(inMaxHeight, (Int32)30000);
	
	if (REF->maxWidth < REF->minWidth)
		REF->maxWidth = REF->minWidth;
		
	if (REF->maxHeight < REF->minHeight)
		REF->maxHeight = REF->minHeight;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool UWindow::GetAutoBounds(TWindow inRef, Uint16 inPos, const SRect& inEnclosure, SRect& outBounds)
{
	SRect oldBounds;
	Int32 n;
	GetBounds(inRef, oldBounds);

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
positionAtTopLeft:
			n = oldBounds.bottom - oldBounds.top;
			outBounds.top = inEnclosure.top + 10 + REF->frameSizeTop;
			outBounds.bottom = outBounds.top + n;
			n = oldBounds.right - oldBounds.left;
			outBounds.left = inEnclosure.left + 10;
			outBounds.right = outBounds.left + n;
			break;
		
		case windowPos_TopRight:
			n = oldBounds.bottom - oldBounds.top;
			outBounds.top = inEnclosure.top + 10 + REF->frameSizeTop;
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
			outBounds = oldBounds;
			outBounds.Move(20, 20);

			if (inEnclosure.right < outBounds.right + REF->frameSizeRight)
			{
				if (REF->options & windowOption_Sizeable)
				{
					if (inEnclosure.right - REF->frameSizeRight - outBounds.left < REF->minWidth)
						goto positionAtTopLeft;
						
					outBounds.right = inEnclosure.right - REF->frameSizeRight;
				}
				else
					goto positionAtTopLeft;
			}

			if (inEnclosure.bottom < outBounds.bottom + REF->frameSizeBottom)
			{
				if (REF->options & windowOption_Sizeable)
				{
					if (inEnclosure.bottom - REF->frameSizeBottom - outBounds.top < REF->minHeight)
						goto positionAtTopLeft;
						
					outBounds.bottom = inEnclosure.bottom - REF->frameSizeBottom;
				}
				else
					goto positionAtTopLeft;
			}
			break;
		
		case windowPos_Unobstructed:
			_WNCalcUnobstructedBounds(inRef, outBounds);
			break;

		default:	// windowPos_Best
			
			// ************ this needs to make sure the window is onscreen
			
			// if left needs to be adjusted, move left + right
			// if right needs to be adjusted, move right
			// else if right needs to be adjusted, move left and right
			// if left needs to be adjusted, move left

			// if top, move top + bottom
			// if bottom move bottom
			// else if bottom move top + bottom
			// if top move top
			
			outBounds = oldBounds;
			n = inEnclosure.left - (outBounds.left - REF->frameSizeLeft);
			if (n > 0)
			{
				outBounds.left += n;
				outBounds.right += n;
				n = outBounds.right + REF->frameSizeRight - inEnclosure.right;
				
				if (n > 0)
					outBounds.right -= n;
			}
			else
			{
				n = outBounds.right + REF->frameSizeRight - inEnclosure.right;
				
				if (n > 0)
				{
					outBounds.right -= n;
					outBounds.left -= n;
					
					n = inEnclosure.left - (outBounds.left - REF->frameSizeLeft);
					if (n > 0)
						outBounds.left += n;
				}
			}
			
			n = inEnclosure.top - (outBounds.top - REF->frameSizeTop);
			if (n > 0)
			{
				outBounds.top += n;
				outBounds.bottom += n;
				n = outBounds.bottom + REF->frameSizeBottom - inEnclosure.bottom;
				
				if (n > 0)
					outBounds.bottom -= n;
			}
			else
			{
				n = outBounds.bottom + REF->frameSizeBottom - inEnclosure.bottom;
				
				if (n > 0)
				{
					outBounds.bottom -= n;
					outBounds.top -= n;
					
					n = inEnclosure.top - (outBounds.top - REF->frameSizeTop);
					if (n > 0)
						outBounds.top += n;
				}
			}
			break;
	}
	
	// return whether or not bounds changed
	return (oldBounds != outBounds);
}

bool UWindow::GetAutoBounds(TWindow inRef, Uint16 inPos, Uint16 /* inPosOn */, SRect& outBounds)
{
	SRect enc;
	HWND parent;
	
	Require(inRef);
	
	parent = ::GetParent(REF->hwnd);
	if (parent == NULL)
	{
		::GetClientRect(::GetDesktopWindow(), (RECT *)&enc);
	
		APPBARDATA abd;
		UMemory::Clear(&abd, sizeof(abd));
		abd.cbSize = sizeof(APPBARDATA);
		abd.hWnd = nil;
		::SHAppBarMessage(ABM_GETSTATE, &abd);
		if (abd.lParam & ABS_AUTOHIDE == 0)	 // not autohide => need to avoid the task bar
		{
			UMemory::Clear(&abd, sizeof(abd));
			abd.cbSize = sizeof(APPBARDATA);
			abd.hWnd = nil;
			::SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
		}
		
		if (enc.left < abd.rc.left)
			enc.right = abd.rc.left;
		else if (enc.top < abd.rc.top)
			enc.bottom = abd.rc.top;
		else if (enc.right > abd.rc.right)
			enc.left = abd.rc.right;
		else if (enc.bottom > abd.rc.bottom)
			enc.top = abd.rc.bottom;
	}
	else
		::GetClientRect(parent, (RECT *)&enc);
	
	return GetAutoBounds(inRef, inPos, enc, outBounds);
}

void UWindow::SetMainWindow(TWindow inRef)
{
	_gMainWindow = (SWindow *)inRef;
}

TWindow UWindow::GetMainWindow()
{
	return (TWindow)_gMainWindow;
}

bool UWindow::GetZoomBounds(TWindow inRef, const SRect& inIdealSize, SRect& outBounds)
{
	#pragma unused(inIdealSize)
	Require(inRef);
	
	UWindow::GetBounds(inRef, outBounds);
	
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

Uint16 UWindow::GetStyle(TWindow inRef)
{
	Require(inRef);
	return REF->style;
}

Uint16 UWindow::GetOptions(TWindow inRef)
{
	Require(inRef);
	return REF->options;
}

Uint16 UWindow::GetState(TWindow inRef)
{
	Require(inRef);

	return REF->state;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetMessageHandler(TWindow inRef, TMessageProc inProc, void *inContext)
{
	Require(inRef);
	
	if (REF->msgProc)
		UApplication::FlushMessages(REF->msgProc, REF->msgProcContext, inRef);
	
	REF->msgProc = inProc;
	REF->msgProcContext = inContext;
}

void UWindow::PostMessage(TWindow inRef, Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority)
{
	Require(inRef);
	
	if (REF->msgProc)
		UApplication::PostMessage(inMsg, inData, inDataSize, inPriority, REF->msgProc, REF->msgProcContext, inRef);
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

TWindow UWindow::GetFront(Uint16 inLayer)
{
	if (inLayer < windowLayer_Popup || inLayer >= windowLayer_Standard)
		inLayer = windowLayer_Standard;
			
	// loop thru our windows and return the frontmost of this layer
	SWindow *win = _gTopWin;
	while (win)
	{
		if (win->layer == inLayer)
			return (TWindow)win;
			
		win = win->next;
	}
	
	return nil;
}

bool UWindow::IsFront(TWindow inRef)
{
	Require(inRef);
	
	// TO DO!  ensure the top window isn't a windows window!  also, ensure that a top window of this layer is above all windoze wins
	return inRef == GetFront(REF->layer);
}

// brings a window to the front of it's layer, and returns true if the window was brought to the front (false if already front)
bool UWindow::BringToFront(TWindow inRef, bool inActivate)
{
	Require(inRef);
	
	HWND hTop = ::GetTopWindow(NULL);
	if (hTop == REF->hwnd) 
		return false;
	
	if (!::SetWindowPos(REF->hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | (inActivate ? 0 : SWP_NOACTIVATE)))
		return false;
		
	return true;
}

bool UWindow::InsertAfter(TWindow inRef, TWindow inInsertAfter, bool inActivate)
{
	Require(inRef);

	if (!inInsertAfter)
		return false;

	if (!::SetWindowPos(REF->hwnd, ((SWindow *)inInsertAfter)->hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | (inActivate ? 0 : SWP_NOACTIVATE)))
		return false;
	
	return true;
}

// sends a window to the back of it's layer, and returns true if the window was sent to the back
bool UWindow::SendToBack(TWindow inRef)
{
	Require(inRef);
	
	if (!::SetWindowPos(REF->hwnd, _GetBottomWindow(REF), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING))
		return false;
	
	return true;	// ****** this is bad, returns true even if the window was already at the back
}

TWindow UWindow::GetNextVisible(TWindow inRef)
{
	#pragma unused(inRef)
	
	Fail(errorType_Misc, error_Unimplemented);
	return nil;
}

bool UWindow::InModal()
{
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetFocus(TWindow inRef)
{
	::SetActiveWindow(REF->hwnd);
}

TWindow UWindow::GetFocus()
{
	HWND hwnd = ::GetActiveWindow();
	if (hwnd)
		return (TWindow)::GetWindowLong(hwnd, GWL_USERDATA);
		
	return nil;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::SetMouseCaptureWindow(TWindow inRef)
{
	if (_gMouseCaptureWin && _gMouseCaptureWin->msgProc)
		UApplication::SendMessage(msg_CaptureReleased, nil, 0, priority_High, _gMouseCaptureWin->msgProc, _gMouseCaptureWin->msgProcContext, _gMouseCaptureWin);
	
	::SetCapture(REF->hwnd);
	_gMouseCaptureWin = REF;
}

void UWindow::ReleaseMouseCaptureWindow(TWindow inRef)
{
	if (_gMouseCaptureWin == REF)
	{
		if (_gMouseCaptureWin && _gMouseCaptureWin->msgProc)
			UApplication::SendMessage(msg_CaptureReleased, nil, 0, priority_High, _gMouseCaptureWin->msgProc, _gMouseCaptureWin->msgProcContext, _gMouseCaptureWin);
		
		::ReleaseCapture();
		_gMouseCaptureWin = nil;
	}
}

TWindow UWindow::GetMouseCaptureWindow()
{
	return (TWindow)_gMouseCaptureWin;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::Collapse(TWindow inRef)
{
	Require(inRef);
	::ShowWindow(REF->hwnd, SW_MINIMIZE);
}

void UWindow::Expand(TWindow inRef)
{
	Require(inRef);
	::ShowWindow(REF->hwnd, SW_RESTORE);
}

bool UWindow::IsCollapsed(TWindow inRef)
{
	Require(inRef);
	return ::IsIconic(REF->hwnd);
}

bool UWindow::IsZoomed(TWindow inRef)
{
	Require(inRef);
	return ::IsZoomed(REF->hwnd);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UWindow::GlobalToLocal(TWindow inRef, SPoint& ioPoint)
{
	Require(inRef);
	::ScreenToClient(REF->hwnd, (POINT *)&ioPoint);

	//RECT r;
	//Require(inRef);
	//if (!GetWindowRect(REF->hwnd, &r))
	//	FailLastWinError();
	//ioPoint.x -= r.top;
	//ioPoint.x -= REF->frameSizeTop;
	//ioPoint.y -= r.left;
	//ioPoint.y -= REF->frameSizeLeft;
}

void UWindow::GlobalToLocal(TWindow inRef, SRect& ioRect)
{
	Require(inRef);
	::MapWindowPoints(NULL, REF->hwnd, (POINT *)&ioRect, 2);

	//RECT r;
	//Int32 n;
	//Require(inRef);
	//if (!GetWindowRect(REF->hwnd, &r))
	//	FailLastWinError();
	//n = r.top + REF->frameSizeTop;
	//ioRect.top -= n;
	//ioRect.bottom -= n;
	//n = r.left + REF->frameSizeLeft
	//ioRect.left -= n;
	//ioRect.right -= n;
}

void UWindow::LocalToGlobal(TWindow inRef, SPoint& ioPoint)
{
	Require(inRef);
	::ClientToScreen(REF->hwnd, (POINT *)&ioPoint);

	//RECT r;
	//Require(inRef);
	//if (!GetWindowRect(REF->hwnd, &r))
	//	FailLastWinError();
	//ioPoint.x += r.top;
	//ioPoint.x += REF->frameSizeTop;
	//ioPoint.y += r.left;
	//ioPoint.y += REF->frameSizeLeft;
}

void UWindow::LocalToGlobal(TWindow inRef, SRect& ioRect)
{
	Require(inRef);
	::MapWindowPoints(REF->hwnd, NULL, (POINT *)&ioRect, 2);

	//RECT r;
	//Int32 n;
	//Require(inRef);
	//if (!GetWindowRect(REF->hwnd, &r))
	//	FailLastWinError();
	//n = r.top + REF->frameSizeTop;
	//ioRect.top += n;
	//ioRect.bottom += n;
	//n = r.left + REF->frameSizeLeft
	//ioRect.left += n;
	//ioRect.right += n;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// returns false if nothing to draw.  Should be called in a loop (there may be more than one rect to draw)
bool UWindow::GetRedrawRect(TWindow inRef, SRect& outRect)
{
	if (inRef && REF->needsRedraw)
	{
		REF->needsRedraw = false;
		outRect = REF->redrawRect;
		return true;
	}
	
	return false;
}

void UWindow::Refresh(TWindow inRef)
{
	Require(inRef);
#if DRAW_OFFSCREEN
	::InvalidateRect(REF->hwnd, NULL, false);
#else
	::InvalidateRect(REF->hwnd, NULL, true);
#endif
}

void UWindow::Refresh(TWindow inRef, const SRect& inRect)
{
	Require(inRef);
	
	ASSERT(inRect.IsValid());
	
#if DRAW_OFFSCREEN
	::InvalidateRect(REF->hwnd, (RECT *)&inRect, false);
#else
	::InvalidateRect(REF->hwnd, (RECT *)&inRect, true);
#endif
}

bool UWindow::IsColor(TWindow /* inRef */)
{
	return true;
}

Uint16 UWindow::GetDepth(TWindow inRef)
{
	Require(inRef);
	return 8;
}

TImage UWindow::BeginDraw(TWindow inRef, const SRect& inUpdateRect)
{
#if DRAW_OFFSCREEN
	
	HWND hwnd;
	HDC onscreenDC = NULL;
	HDC offscreenDC = NULL;
	HBITMAP offscreenBM = NULL;
	TImage img = NULL;
	SRect beginDrawRect, r;
	
	// sanity checks
	Require(inRef);
	if (REF->img)
	{
		DebugBreak("UWindow - BeginDraw() has already been called, need matching EndDraw()/AbortDraw()");
		Fail(errorType_Misc, error_Protocol);
	}

	// if the window isn't visible, return dummy image
	hwnd = REF->hwnd;
	if (!::IsWindowVisible(hwnd) || !inUpdateRect.IsValid())
		return UGraphics::GetDummyImage();
	
	try
	{
		// create on-screen DC
		onscreenDC = ::GetDC(hwnd);
		if (onscreenDC == NULL) Fail(errorType_Memory, memError_NotEnough);

		// create off-screen DC
		offscreenDC = ::CreateCompatibleDC(onscreenDC);
		if (offscreenDC == NULL) Fail(errorType_Memory, memError_NotEnough);
		
		// install a much nicer palette into our offscreen DC
		if (::SelectPalette(offscreenDC, _MAC_PALETTE, false) == NULL) FailLastWinError();
		if (::RealizePalette(offscreenDC) == GDI_ERROR) FailLastWinError();
		
		// determine draw rect
		if (!GetClientRect(hwnd, (RECT *)&r))
			FailLastWinError();
		
		beginDrawRect = inUpdateRect;
		beginDrawRect.Constrain(r);

		// create bitmap for off-screen DC
		Uint32 beginDrawRectWidth = beginDrawRect.GetWidth();
		Uint32 beginDrawRectHeight = beginDrawRect.GetHeight();
		offscreenBM = ::CreateCompatibleBitmap(onscreenDC, beginDrawRectWidth, beginDrawRectHeight);
		if (offscreenBM == NULL) Fail(errorType_Memory, memError_NotEnough);
		::SelectObject(offscreenDC, offscreenBM);

		// erase off-screen DC (it's all black by default)
		r.left = r.top = 0;
		r.right = beginDrawRectWidth;
		r.bottom = beginDrawRectHeight;
		::FillRect(offscreenDC, (RECT *)&r, (REF->options & windowOption_WhiteBkgnd) ? (HBRUSH)GetStockObject(WHITE_BRUSH) : REF->backColBrush ? REF->backColBrush : (HBRUSH)COLOR_WINDOW);
		
		// adjust origin in off-screen DC to match on-screen
		::SetViewportOrgEx(offscreenDC, -beginDrawRect.left, -beginDrawRect.top, NULL);
		
		// create UGraphics-compatible TImage to use with off-screen DC
		img = _NewDCImage(offscreenDC, beginDrawRectWidth, beginDrawRectHeight);
	}
	catch(...)
	{
		// clean up
		if (onscreenDC) ::ReleaseDC(hwnd, onscreenDC);
		if (offscreenDC) ::DeleteDC(offscreenDC);
		if (offscreenBM) ::DeleteObject(offscreenBM);
		_DisposeDCImage(img);
		throw;
	}
	
	// all done!
	REF->img = img;
	REF->onscreenDC = onscreenDC;
	REF->offscreenDC = offscreenDC;
	REF->offscreenBM = offscreenBM;
	REF->beginDrawRect = beginDrawRect;
	return img;

#else

	HDC dc;
	HRGN rgn;
	TImage img;
		
	Require(inRef);
	
	// create DC
	dc = ::GetDC(REF->hwnd);
	if (dc == NULL) Fail(errorType_Memory, memError_NotEnough);

	// clip to update rect
	rgn = ::CreateRectRgnIndirect((RECT *)&inUpdateRect);
	if (rgn == nil)
	{
		::ReleaseDC(REF->hwnd, dc);
		Fail(errorType_Memory, memError_NotEnough);
	}
	else
	{
		::SelectClipRgn(dc, rgn);
		::DeleteObject(rgn);
	}

	// create UGraphics-compatible TImage
	try
	{
		img = _NewDCImage(dc);
	}
	catch(...)
	{
		::ReleaseDC(REF->hwnd, dc);
		throw;
	}
	
	// all done!
	REF->img = img;
	return img;

#endif
}

void UWindow::EndDraw(TWindow inRef)
{
	if (inRef)
	{
#if DRAW_OFFSCREEN

		if (REF->img)
		{
			::SetViewportOrgEx(REF->offscreenDC, 0, 0, NULL);
			::BitBlt(REF->onscreenDC, REF->beginDrawRect.left, REF->beginDrawRect.top, REF->beginDrawRect.GetWidth(), REF->beginDrawRect.GetHeight(), REF->offscreenDC, 0, 0, SRCCOPY);
			
			/*
			if ((REF->layer == windowLayer_Standard) && (REF->options & windowOption_Sizeable))
			{
				RECT boxRect;
				::GetClientRect(REF->hwnd, &boxRect);
				boxRect.left = boxRect.right - 16;
				boxRect.top = boxRect.bottom - 16;
				::DrawFrameControl(REF->onscreenDC, &boxRect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
			}
			*/
			
			_DisposeDCImage(REF->img);
			::ReleaseDC(REF->hwnd, REF->onscreenDC);
			::DeleteDC(REF->offscreenDC);
			::DeleteObject(REF->offscreenBM);
			REF->img = NULL;
			REF->onscreenDC = NULL;
			REF->offscreenDC = NULL;
			REF->offscreenBM = NULL;
		}

#else

		TImage img = REF->img;
		
		if (img)
		{
			REF->img = nil;
			::ReleaseDC(REF->hwnd, _ImageToDC(img));
			_DisposeDCImage(img);
		}

#endif
	}
}

void UWindow::AbortDraw(TWindow inRef)
{
	if (inRef)
	{
#if DRAW_OFFSCREEN
	
		if (REF->img)
		{
			_DisposeDCImage(REF->img);
			REF->img = NULL;
		}
		
		if (REF->onscreenDC)
		{
			::ReleaseDC(REF->hwnd, REF->onscreenDC);
			REF->onscreenDC = NULL;
		}
		
		if (REF->offscreenDC)
		{
			::DeleteDC(REF->offscreenDC);
			REF->offscreenDC = NULL;
		}
		
		if (REF->offscreenBM)
		{
			::DeleteObject(REF->offscreenBM);
			REF->offscreenBM = NULL;
		}
	
#else

		TImage img = REF->img;
		
		if (img)
		{
			REF->img = nil;
			::ReleaseDC(REF->hwnd, _ImageToDC(img));
			_DisposeDCImage(img);
		}

#endif
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static void _WNMyMaximize(TWindow inRef)
{
	if (::IsIconic(REF->hwnd))
		::ShowWindow(REF->hwnd, SW_RESTORE);
	else if (REF->options & windowOption_Sizeable)
	{
		RECT r;
		WINDOWPLACEMENT pm;
		pm.length = sizeof(pm);

		if (REF->isMyMaximized)
		{
			if (::GetWindowPlacement(REF->hwnd, &pm) && ::GetClientRect(_gMDIClient ? _gMDIClient : ::GetDesktopWindow(), &r))
			{
				if (pm.rcNormalPosition.left == r.left && pm.rcNormalPosition.top == r.top && pm.rcNormalPosition.right == r.right && pm.rcNormalPosition.bottom == r.bottom)
				{
					RECT& rp = REF->beforeMyMaximizeRect;
					if (::MoveWindow(REF->hwnd, rp.left, rp.top, rp.right - rp.left, rp.bottom - rp.top, true))
						REF->isMyMaximized = false;
				}
				else
				{
					::MoveWindow(REF->hwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, true);
				}
			}
		}
		else
		{
			if (::GetWindowPlacement(REF->hwnd, &pm))
			{
				REF->beforeMyMaximizeRect = pm.rcNormalPosition;

				if (::GetClientRect(_gMDIClient ? _gMDIClient : ::GetDesktopWindow(), &r))
				{
					if (::MoveWindow(REF->hwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, true))
						REF->isMyMaximized = true;
				}
			}
		}
	}
}

static LRESULT CALLBACK _WNWndProc(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam)
{	
#if 0
	const struct
	{
		WPARAM wparam;
		LPARAM lparam;
	
	} params = { inWParam, inLParam };	
#endif

	static Uint8 isUsingNicePal = false;
	static HWND lastWin;

	SWindow *ref = (SWindow *)::GetWindowLong(inWnd, GWL_USERDATA);
	SWindow *tmpRef;
	LRESULT result = 0;
	SMouseMsgData minfo;
	SKeyMsgData kinfo;
	Uint32 n;		

	if (ref == nil)
		return (_gMDIClient ? DefMDIChildProc : DefWindowProc)(inWnd, inMsg, inWParam, inLParam);
	
	// send all messages to QuickTime (if we pass WM_ERASEBKGND message sometime QuickTime give an unhandled exception when the window is resized)
	if (UOperatingSystem::IsQuickTimeAvailable() && inMsg != WM_ERASEBKGND)
		_SendToQuickTime((TWindow)ref, inMsg, inWParam, inLParam);
	
	try {

	switch (inMsg)
	{
		/*
		 * Draw
		 */
		case WM_PAINT:
		{			
			PAINTSTRUCT ps;
			::BeginPaint(inWnd, &ps);
			
			if (ref->needsRedraw)
			{
				ref->redrawRect |= *(SRect *)&ps.rcPaint;
				
				::EndPaint(inWnd, &ps);
			}
			else
			{
				ref->needsRedraw = true;
				ref->redrawRect = *(SRect *)&ps.rcPaint;
				
				::EndPaint(inWnd, &ps);
				
				if (ref->msgProc)
				{
					/*
					try
					{
						UApplication::ReplaceMessage(msg_Draw, nil, 0, priority_Draw, ref->msgProc, ref->msgProcContext, ref);
					}
					catch(...)
					{
						DebugBreak("UWindow - not enough memory to redraw!");
						// don't throw
					}
					*/
					
					UApplication::SendMessage(msg_Draw, nil, 0, priority_Draw, ref->msgProc, ref->msgProcContext, ref);			
				}
			}
		}
		break;
		
		case WM_NCPAINT:
		{
			if (_gMDIClient)
				result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			else
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
			
			// frame the content with a 1-pixel black border to suit our GUI elements better
			if (ref && ref->layer == windowLayer_Standard)
			{
				RECT r;
				::GetClientRect(inWnd, &r);
				r.left += ref->frameSizeLeft;
				r.top += ref->frameSizeTop;
				r.right += ref->frameSizeLeft;
				r.bottom += ref->frameSizeTop;
				r.left--;
				r.top--;
				r.right++;
				r.bottom++;
				
				HDC wdc = ::GetWindowDC(inWnd);
				::FrameRect(wdc, &r, (HBRUSH)::GetStockObject(BLACK_BRUSH));
				::ReleaseDC(inWnd, wdc);
			}
		}
		break;
		
		case WM_QUERYNEWPALETTE:
		{
			HDC dc = ::GetDC(inWnd);
			
			::SelectPalette(dc, _MAC_PALETTE, FALSE);	// it is necessary to select every time (can't just select once when window is created)
			::RealizePalette(dc);
			
			if (!isUsingNicePal)
			{
				::InvalidateRect(inWnd, NULL, FALSE);
				::UpdateWindow(inWnd);
			}
			
			::ReleaseDC(inWnd, dc);
			
			result = true;
			isUsingNicePal = true;
		}
		break;
		
		case WM_PALETTECHANGED:
		{
			isUsingNicePal = false;
			
			if ((HWND)inWParam != inWnd)
			{
				HDC dc = ::GetDC(inWnd);
				
				::SelectPalette(dc, _MAC_PALETTE, FALSE);	// it is necessary to select every time (can't just select once when window is created)
				::RealizePalette(dc);

				::InvalidateRect(inWnd, NULL, FALSE);
				::UpdateWindow(inWnd);
				
				::ReleaseDC(inWnd, dc);
			}
		}
		break;
		
		/*
		 * Mouse Move
		 */
		case WM_NCMOUSEMOVE:	// if a window has captured the mouse, this message is not posted
		{
			if (gSnapWindows.IsEnableSnapWindows())
				gSnapWindows.DestroySnapWinListSound();

			result = (_gMDIClient ? DefMDIChildProc : DefWindowProc)(inWnd, inMsg, inWParam, inLParam);
			
			if (ref->state == windowState_Hidden || ref->state == windowState_Inactive)
				break;
				
			/*
			 * In terms of msg_MouseMove/msg_MouseLeave, the mouse is "within" the window if it is inside
			 * the CONTENT area of the window, not the frame.  Thus if the mouse is in the frame it is in 
			 * NO window, so we'll post a leave to the current "within" window (if any).
			 */
			
			minfo.time = ::GetMessageTime();
			minfo.loc.x = (Int16)LOWORD(inLParam);
			minfo.loc.y = (Int16)HIWORD(inLParam);
			minfo.button = minfo.mods = 0;
			minfo.misc = 0;
						
			if (_gMouseWithinWin)
			{
				minfo.time = ::GetMessageTime();
				minfo.loc.x = (Int16)LOWORD(inLParam);
				minfo.loc.y = (Int16)HIWORD(inLParam);
				minfo.button = minfo.mods = 0;
				minfo.misc = 0;
				
				tmpRef = _gMouseWithinWin;
				_gMouseWithinWin = nil;
				
				if (tmpRef->msgProc)
					UApplication::SendMessage(msg_MouseLeave, &minfo, sizeof(minfo), priority_MouseLeave, tmpRef->msgProc, tmpRef->msgProcContext, tmpRef);
			}
		}
		break;

		case WM_MOUSEMOVE:
		{
			if (ref->state == windowState_Hidden || ref->state == windowState_Inactive)
				break;

			minfo.time = ::GetMessageTime();
			minfo.loc.x = (Int16)LOWORD(inLParam);
			minfo.loc.y = (Int16)HIWORD(inLParam);
			minfo.button = minfo.mods = 0;
			minfo.misc = 0;
			if (inWParam & MK_CONTROL) minfo.mods |= modKey_Control;
			if (inWParam & MK_SHIFT) minfo.mods |= modKey_Shift;
			if (::GetKeyState(VK_MENU) & 0x80) minfo.mods |= modKey_Alt;

			if (_gMouseWithinWin == ref || _gMouseCaptureWin == ref)
			{
				if (ref->msgProc)
					UApplication::SendMessage(msg_MouseMove, &minfo, sizeof(minfo), priority_MouseMove, ref->msgProc, ref->msgProcContext, ref);
			}
			else
			{
				::SetCursor(_gArrowCursor);
				
				if (_gMouseWithinWin)
				{
					if (_gMouseWithinWin->msgProc)
						UApplication::SendMessage(msg_MouseLeave, &minfo, sizeof(minfo), priority_MouseLeave, _gMouseWithinWin->msgProc, _gMouseWithinWin->msgProcContext, _gMouseWithinWin);
				}
								
				_gMouseWithinWin = ref;
				if (ref->msgProc)
					UApplication::SendMessage(msg_MouseEnter, &minfo, sizeof(minfo), priority_MouseEnter, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;

		/*
		 * Mouse Down
		 */
		case WM_NCLBUTTONDOWN:		// if a window has captured the mouse, this message is not posted
		{
			if (ref && ref->userRef && gSnapWindows.IsEnableSnapWindows())
			{
				CWindow *pWindow = (CWindow *)ref->userRef;
								
				if (gWindowList.IsInList(pWindow))
				{
					if (inWParam != HTCAPTION || UKeyboard::IsOptionKeyDown())
						gSnapWindows.CreateSnapSound(pWindow);
					else
						gSnapWindows.CreateSnapWinList(pWindow);
				}
			}

			if (_gMDIClient)
				result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			else
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
			break;
		}
		break;
				
		case WM_LBUTTONDOWN:
		{
			if (ref->state == windowState_Hidden || ref->state == windowState_Inactive)
				break;

			::SetCapture(inWnd);

			if (::GetActiveWindow() != inWnd)
				::SetActiveWindow(inWnd);
						
			if (ref->msgProc)
			{
				minfo.time = ::GetMessageTime();
				minfo.loc.x = (Int16)LOWORD(inLParam);
				minfo.loc.y = (Int16)HIWORD(inLParam);
				minfo.button = mouseButton_Left;
				minfo.mods = 0;
				minfo.misc = 0;
				if (inWParam & MK_CONTROL) minfo.mods |= modKey_Control;
				if (inWParam & MK_SHIFT) minfo.mods |= modKey_Shift;
				if (::GetKeyState(VK_MENU) & 0x80) minfo.mods |= modKey_Alt;
				UApplication::SendMessage(msg_MouseDown, &minfo, sizeof(minfo), priority_MouseDown, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;
		
		case WM_MBUTTONDOWN:
		{
			if (ref->state == windowState_Hidden || ref->state == windowState_Inactive)
				break;

			::SetCapture(inWnd);

			if (::GetActiveWindow() != inWnd)
				::SetActiveWindow(inWnd);
			
			if (ref->msgProc)
			{
				minfo.time = ::GetMessageTime();
				minfo.loc.x = (Int16)LOWORD(inLParam);
				minfo.loc.y = (Int16)HIWORD(inLParam);
				minfo.button = mouseButton_Middle;
				minfo.mods = 0;
				minfo.misc = 0;
				if (inWParam & MK_CONTROL) minfo.mods |= modKey_Control;
				if (inWParam & MK_SHIFT) minfo.mods |= modKey_Shift;
				if (::GetKeyState(VK_MENU) & 0x80) minfo.mods |= modKey_Alt;
				UApplication::SendMessage(msg_MouseDown, &minfo, sizeof(minfo), priority_MouseDown, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;
		
		case WM_RBUTTONDOWN:
		{
			if (ref->state == windowState_Hidden || ref->state == windowState_Inactive)
				break;

			::SetCapture(inWnd);
			
			if (::GetActiveWindow() != inWnd)
				::SetActiveWindow(inWnd);
			
			if (ref->msgProc)
			{
				minfo.time = ::GetMessageTime();
				minfo.loc.x = (Int16)LOWORD(inLParam);
				minfo.loc.y = (Int16)HIWORD(inLParam);
				minfo.button = mouseButton_Right;
				minfo.mods = 0;
				minfo.misc = 0;
				if (inWParam & MK_CONTROL) minfo.mods |= modKey_Control;
				if (inWParam & MK_SHIFT) minfo.mods |= modKey_Shift;
				if (::GetKeyState(VK_MENU) & 0x80) minfo.mods |= modKey_Alt;
				UApplication::SendMessage(msg_MouseDown, &minfo, sizeof(minfo), priority_MouseDown, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;
		
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		{
			if (ref->state == windowState_Hidden || ref->state == windowState_Inactive)
				break;

			::SetCapture(inWnd);
			
			if (ref->msgProc)
			{
				minfo.time = ::GetMessageTime();
				minfo.loc.x = (Int16)LOWORD(inLParam);
				minfo.loc.y = (Int16)HIWORD(inLParam);
				minfo.button = (inMsg == WM_RBUTTONDBLCLK) ? mouseButton_Right : ((inMsg == WM_MBUTTONDBLCLK) ? mouseButton_Middle : mouseButton_Left);
				minfo.mods = kIsDoubleClick;
				minfo.misc = 0;
				if (inWParam & MK_CONTROL) minfo.mods |= modKey_Control;
				if (inWParam & MK_SHIFT) minfo.mods |= modKey_Shift;
				if (::GetKeyState(VK_MENU) & 0x80) minfo.mods |= modKey_Alt;
				UApplication::SendMessage(msg_MouseDown, &minfo, sizeof(minfo), priority_MouseDown, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;
		
		/*
		 * Mouse Up
		 */
		case WM_NCLBUTTONUP:	// if a window has captured the mouse, this message is not posted
		{
			if (gSnapWindows.IsEnableSnapWindows())
				gSnapWindows.DestroySnapWinListSound();
		
			if (_gMDIClient)
				result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			else
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
			break;
		}
		break;
		
		case WM_LBUTTONUP:
		{
			if (!_gMouseCaptureWin)
				::ReleaseCapture();

			if (ref->msgProc)
			{
				minfo.time = ::GetMessageTime();
				minfo.loc.x = (Int16)LOWORD(inLParam);
				minfo.loc.y = (Int16)HIWORD(inLParam);
				minfo.button = mouseButton_Left;
				minfo.mods = 0;
				minfo.misc = 0;
				if (inWParam & MK_CONTROL) minfo.mods |= modKey_Control;
				if (inWParam & MK_SHIFT) minfo.mods |= modKey_Shift;
				if (::GetKeyState(VK_MENU) & 0x80) minfo.mods |= modKey_Alt;
				UApplication::SendMessage(msg_MouseUp, &minfo, sizeof(minfo), priority_MouseUp, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;
		
		case WM_MBUTTONUP:
		{
			if (!_gMouseCaptureWin)
				::ReleaseCapture();
			
			if (ref->msgProc)
			{
				minfo.time = ::GetMessageTime();
				minfo.loc.x = (Int16)LOWORD(inLParam);
				minfo.loc.y = (Int16)HIWORD(inLParam);
				minfo.button = mouseButton_Middle;
				minfo.mods = 0;
				minfo.misc = 0;
				if (inWParam & MK_CONTROL) minfo.mods |= modKey_Control;
				if (inWParam & MK_SHIFT) minfo.mods |= modKey_Shift;
				if (::GetKeyState(VK_MENU) & 0x80) minfo.mods |= modKey_Alt;
				UApplication::SendMessage(msg_MouseUp, &minfo, sizeof(minfo), priority_MouseUp, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;
		
		case WM_RBUTTONUP:
		{
			if (!_gMouseCaptureWin)
				::ReleaseCapture();
			
			if (ref->msgProc)
			{
				minfo.time = ::GetMessageTime();
				minfo.loc.x = (Int16)LOWORD(inLParam);
				minfo.loc.y = (Int16)HIWORD(inLParam);
				minfo.button = mouseButton_Right;
				minfo.mods = 0;
				minfo.misc = 0;
				if (inWParam & MK_CONTROL) minfo.mods |= modKey_Control;
				if (inWParam & MK_SHIFT) minfo.mods |= modKey_Shift;
				if (::GetKeyState(VK_MENU) & 0x80) minfo.mods |= modKey_Alt;
				UApplication::SendMessage(msg_MouseUp, &minfo, sizeof(minfo), priority_MouseUp, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;
		
		/*
		 * Keyboard
		 */
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		{
			if (ref->msgProc)
			{
				Uint8 kbState[256];
				WORD ascii;
				
				if (::GetKeyboardState(kbState))
				{
					kinfo.mods = 0;
					if (kbState[VK_SHIFT] & 0x80) kinfo.mods |= modKey_Shift;
					if (kbState[VK_MENU] & 0x80) kinfo.mods |= modKey_Alt;
					if (kbState[VK_CAPITAL] & 0x80) kinfo.mods |= modKey_CapsLock;
					
					if (kbState[VK_CONTROL] & 0x80)		// if control key is down
					{
						kbState[VK_CONTROL] &= ~0x80;	// turn control off so it doesn't modify the key char
						kbState[VK_LCONTROL] &= ~0x80;
						kbState[VK_RCONTROL] &= ~0x80;
						kinfo.mods |= modKey_Control;
					}

					if (::ToAscii(inWParam, inLParam, kbState, &ascii, 0))
						kinfo.keyChar = (Uint8)ascii;
					else
						kinfo.keyChar = 0;
					
					kinfo.time = ::GetMessageTime();
					kinfo.keyCode = _WinKeyCodeToStd(inWParam);
					
					// distinguish between main enter key and enter key on numeric keypad
					if (kinfo.keyCode == key_Enter && (inLParam & (1 << 24)))
						kinfo.keyCode = key_nEnter;
					
					if (inMsg == WM_KEYUP || inMsg == WM_SYSKEYUP)
						n = msg_KeyUp;
					else
						n = ((inLParam & 0xFFFF) > 1) ? msg_KeyRepeat : msg_KeyDown;
					
					bool bPostKeyMessage = true;
					if (!_IsTopVisibleModalWindow()) // don't post msg_KeyCommand if there is a modal visible window
					{
						Uint32 keyCmd = (n == msg_KeyDown) ? UKeyboard::FindKeyCommand(kinfo, _WNData.keyCmds, _WNData.keyCmdCount) : 0;
					
						if (keyCmd)
						{
							bPostKeyMessage	= false;

							*(Uint32 *)kbState = keyCmd;
							*(SKeyMsgData *)(kbState+sizeof(Uint32)) = kinfo;
							UApplication::PostMessage(msg_KeyCommand, kbState, sizeof(Uint32) + sizeof(SKeyMsgData), priority_KeyDown, _WNData.keyCmdMsgProc, _WNData.keyCmdMsgProcContext);						
						}
					}
					
					if (bPostKeyMessage)
						UApplication::SendMessage(n, &kinfo, sizeof(kinfo), priority_KeyDown, ref->msgProc, ref->msgProcContext, ref);
				}
			}
		}
		break;
		
		case WM_SHOWWINDOW:
		{
			if (_gMDIClient)
				result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			else
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
		
			if (!inWParam && _gStateTimer)
				_gStateTimer->Start(300, kOnceTimer);	// will post an msg_StateChanged message with windowState_Hidden state
		}
		break;
		
		/*
		 * User Close
		 */
		case WM_CLOSE:
		{
			if (ref->msgProc)
			{
				minfo.time = ::GetMessageTime();
				minfo.loc.x = minfo.loc.y = 0;
				minfo.button = 0;
				minfo.mods = 0;
				minfo.misc = 0;
				UApplication::SendMessage(msg_UserClose, &minfo, sizeof(minfo), priority_Normal, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;

		/*
		 * Bounds
		 */
		case WM_SIZING:
		{
			if (ref && ref->userRef && gSnapWindows.IsEnableSnapWindows())
			{
				CWindow *pWindow = (CWindow *)ref->userRef;
								
				if (gWindowList.IsInList(pWindow))
				{
					RECT *lprc = (RECT *)inLParam;
				
					SRect rSnapWinBounds(lprc->left, lprc->top, lprc->right, lprc->bottom);
					if (gSnapWindows.WinSnapTogether(pWindow, rSnapWinBounds, false))
					{
						lprc->left = rSnapWinBounds.left;
						lprc->top = rSnapWinBounds.top;
						lprc->right = rSnapWinBounds.right;
						lprc->bottom = rSnapWinBounds.bottom;
					}
				}			
			}
		}
		// don't break;

		case WM_MOVING:
		{
			if (gSnapWindows.IsEnableSnapWindows())
			{
				// I should do this only if Windows don't show window contents while dragging
				// but I didn't find the function which tell me this yet 
				RECT *lprc = (RECT *)inLParam;				
				::MoveWindow(inWnd, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top, TRUE);
				result = true;
			}
			else
			{
				if (_gMDIClient)
					result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
				else
					result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
			}
		}
		break;

		case WM_MOVE:
		{
			if (ref && ref->userRef && gSnapWindows.IsEnableSnapWindows())
			{
				CWindow *pWindow = (CWindow *)ref->userRef;
								
				if (gWindowList.IsInList(pWindow))
				{
					if (UKeyboard::IsOptionKeyDown())
						gSnapWindows.DestroySnapWinListSound(false);
					else
						gSnapWindows.WinMoveTogether(pWindow);
				}
			}
		}
		// don't break;
		
		case WM_SIZE:
		{
			// this is needed to adjust the scrollbars
			if (_gMDIClient)
				result = DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			
			if (ref)	// sometimes get message before SetWindowLong has installed the ref
			{
				// a WM_SIZE often follows a WM_MOVE, so to avoid doing stuff twice, we'll remove the extra messages
				MSG wmsg;
				while (::PeekMessageA(&wmsg, inWnd, WM_MOVE, WM_MOVE, PM_REMOVE)) {}
				while (::PeekMessageA(&wmsg, inWnd, WM_SIZE, WM_SIZE, PM_REMOVE)) {}
				
				if (ref->msgProc)
					UApplication::SendMessage(msg_BoundsChanged, nil, 0, priority_Draw+1, ref->msgProc, ref->msgProcContext, ref);
			}
		}
		break;
		
		/*
		 * Activate
		 */
		case WM_ACTIVATE:
		{

#if 0
			SWindow *mod = _GetTopVisibleModalWindow();
			/*
			if (mod)
			{
				HWND focusWin = ::GetFocus();
				if (mod->hwnd != focusWin)
				{
					if (focusWin)
					{
						inWParam = (inWParam & 0xFFFF0000) | WA_INACTIVE;
						inWnd = focusWin;
						inLParam = (long)mod->hwnd;
					}
					else
						return 0;
				}
				else
				{
					if (LOWORD(inWParam) == WA_INACTIVE)
						inWParam = (inWParam & 0xFFFF0000) | WA_ACTIVE;
					
					inWnd = mod->hwnd;
					inLParam = (long)focusWin;
				}
			}
			*/
			
			HWND focus = ::GetFocus();
			
			if (mod)
			{
				if (focus)
				{
					if (_gMDIClient)
						result = ::DefMDIChildProc(focus, inMsg, (inWParam & 0xFFFF0000) | WA_INACTIVE, (long)(mod->hwnd));
					else
						result = ::DefWindowProc(focus, inMsg, (inWParam & 0xFFFF0000) | WA_INACTIVE, (long)(mod->hwnd));
				}
				
				inWnd = mod->hwnd;
				inWParam = (inWParam & 0xFFFF0000) | WA_ACTIVE;
				inLParam = (long)focus;
			}
#endif
/*
			SWindow *mod = _GetTopVisibleModalWindow();
			if (mod) USound::Beep();
			if (mod && mod->hwnd != inWnd && LOWORD(inWParam) == WA_ACTIVE)
			{
				::SetActiveWindow(mod->hwnd);
			}
			else if (mod && mod->hwnd == inWnd && LOWORD(inWParam) == WA_INACTIVE)
			{
				::SetActiveWindow(mod->hwnd);
			}
			else*/
			{
				if (_gMDIClient)
					result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
				else
					result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
			}
			
			_CheckWindowStates();

			/*
			 * Fix bug where it doesn't draw until you release the mouse if you click and hold in
			 * the titlebar of an inactive window, and in doing so bring it to the front and expose
			 * part of the window thus needing a redraw.
			 */
			::UpdateWindow(inWnd);
		}
		break;
	
		case WM_MDIACTIVATE:
		{
			
			if (inWParam) ::SendMessageA(_gMDIFrame, WM_QUERYNEWPALETTE, NULL, NULL);

			result = DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			
			_CheckWindowStates();

			::UpdateWindow(inWnd);
		}
		break;
			
		case WM_MOUSEACTIVATE:
		{
			SWindow *mod = _GetTopVisibleModalWindow();
			if (mod && mod->hwnd != inWnd)// && ref->layer != windowLayer_Popup)
			{
				if (::GetActiveWindow() != mod->hwnd)
					::SetActiveWindow(mod->hwnd);
				
				result = MA_NOACTIVATEANDEAT;
			}
			else
			{
				if (_gMDIClient)
					result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
				else
					result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
			}
			
			/*
			USound::Beep();
			result = MA_NOACTIVATEANDEAT;
			*/
		}
		break;
		
		case WM_ACTIVATEAPP:
		{
			if (!inWParam)	// if this app is being deactivated
				_gAppDeactivated = true;
			
			if (_gMDIClient)
				result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			else
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
		}	
		break;

		// WM_WINDOWPOSCHANGING - window layers
		case WM_WINDOWPOSCHANGING:
			WINDOWPOS *lpwp = (WINDOWPOS *)inLParam;
			
			if (ref && ref->userRef)
			{
				CWindow *pWindow = (CWindow *)ref->userRef;
								
				if (gWindowList.IsInList(pWindow))
				{
					if (gSnapWindows.IsEnableSnapWindows() && !_gAppPosChanging)
					{				
						SRect rSnapWinBounds(lpwp->x, lpwp->y, lpwp->x + lpwp->cx, lpwp->y + lpwp->cy);
						if (gSnapWindows.WinSnapTogether(pWindow, rSnapWinBounds, true))
						{
							lpwp->x = rSnapWinBounds.left;
							lpwp->y = rSnapWinBounds.top;
							lpwp->cx = rSnapWinBounds.right - rSnapWinBounds.left;
							lpwp->cy = rSnapWinBounds.bottom - rSnapWinBounds.top;
						}
					}
			
					SRect rScreenWinBounds(lpwp->x, lpwp->y, lpwp->x + lpwp->cx, lpwp->y + lpwp->cy);
					if (gSnapWindows.WinKeepOnScreen(pWindow, rScreenWinBounds))
					{
						lpwp->x = rScreenWinBounds.left;
						lpwp->y = rScreenWinBounds.top;
						lpwp->cx = rScreenWinBounds.right - rScreenWinBounds.left;
						lpwp->cy = rScreenWinBounds.bottom - rScreenWinBounds.top;
					}
				}
			}

			if (!(lpwp->flags & SWP_NOACTIVATE))		// only find my location in the window layers if it's changed
			{
				_RemoveFromLayer(ref);
				lpwp->hwndInsertAfter = _AddToLayerAndGetInsertAfter(ref, lpwp->hwndInsertAfter);
			}
						
			if (_gMDIClient)
				result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			else
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);

			break;
		/*
		 * Misc
		 */
		case WM_INITMENUPOPUP:
		{
			result = (_gMDIClient ? DefMDIChildProc : DefWindowProc)(inWnd, inMsg, inWParam, inLParam);
			
			if (HIWORD(inLParam))	// if this is the system menu
			{
				if ((ref->options & windowOption_Sizeable) == 0)	// if not sizeable
				{
					// work around bug in win95
					::EnableMenuItem((HMENU)inWParam, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
					::EnableMenuItem((HMENU)inWParam, SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
				}
			}
		}
		break;
		
		case WM_SETFOCUS:
		{
			// calling DefMDIChildProc here likes to crash  
			if (_gMDIClient == nil) 
				::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
		}
		break;
		
		case WM_KILLFOCUS:
		{			
			if (_gMDIClient)
				result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			else
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
		}
		break;
		
		case WM_SYSCOMMAND:
		{
			Uint16 wParam  = inWParam & 0xFFF0;
			
			if (_gMDIClient)
			{
				// the std windoze maximize causes crashes and all sorts of problems when in an MDI window so we'll do our own maximize
				if (inWParam == SC_MAXIMIZE)
					_WNMyMaximize((TWindow)ref);
				else
					result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			}
			else
			{
				// don't process commands is there is a modal visible window
				if (_IsTopVisibleModalWindow() && (wParam == SC_MAXIMIZE || wParam == SC_MINIMIZE || wParam == SC_RESTORE || wParam == SC_CLOSE))
					return 0;
				
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
			}
							
			if (ref->msgProc && (wParam == SC_MAXIMIZE || wParam == SC_MINIMIZE || wParam == SC_RESTORE))
				UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, ref->msgProc, ref->msgProcContext, ref);
		}
		break;
				
		case WM_NCLBUTTONDBLCLK:
		{
			if (_gMDIClient)
			{
				// the WM_SYSCOMMAND above doesn't catch double-clicking a titlebar to maximize so we'll catch it here (damn windoze!!)
				if (inWParam == HTCAPTION)
					_WNMyMaximize((TWindow)ref);
				else
					result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			}
			else
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
		}
		break;
		
		case WM_GETMINMAXINFO:
		{
			result = (_gMDIClient ? DefMDIChildProc : DefWindowProc)(inWnd, inMsg, inWParam, inLParam);

			if (ref->options & windowOption_Sizeable)
			{
				((MINMAXINFO *)inLParam)->ptMinTrackSize.x = ref->minWidth + ref->frameSizeLeft + ref->frameSizeRight;
				((MINMAXINFO *)inLParam)->ptMinTrackSize.y = ref->minHeight + ref->frameSizeTop + ref->frameSizeBottom;
				((MINMAXINFO *)inLParam)->ptMaxTrackSize.x = ref->maxWidth + ref->frameSizeLeft + ref->frameSizeRight;
				((MINMAXINFO *)inLParam)->ptMaxTrackSize.y = ref->maxHeight + ref->frameSizeTop + ref->frameSizeBottom;
			}
		}
		break;		
				
		/*
		 * Default
		 */
		default:
			if (_gMDIClient)
				result = ::DefMDIChildProc(inWnd, inMsg, inWParam, inLParam);
			else
				result = ::DefWindowProc(inWnd, inMsg, inWParam, inLParam);
			break;
	}
	
	// must not continue exceptions out of this function
	} catch(...) {}
	
	return result;
}

static void _WNSendMessageToMDIKids(WORD inMsg, WPARAM inWParam, LPARAM inLParam)
{
	register HWND win = ::GetWindow(_gMDIClient, GW_CHILD);
	
	while (win)
	{
		::SendMessageA(win, inMsg, inWParam, inLParam);
		win = ::GetWindow(win, GW_HWNDNEXT);
	}
}

static LRESULT CALLBACK _WNFrameWndProc(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam)
{
	LRESULT result = 0;	
	
	try {
	
	switch (inMsg)
	{
		case WM_CLOSE:
			UApplication::PostMessage(msg_Quit, nil, 0, priority_Quit);
			break;
		
		case WM_QUERYNEWPALETTE:
		{
			HWND tmpWin = (HWND)(WORD)::SendMessageA(_gMDIClient, WM_MDIGETACTIVE, NULL, NULL); 

			// tell the active window to realize in foreground
			result = (WORD)::SendMessageA(tmpWin, WM_QUERYNEWPALETTE, NULL, NULL);

			// if mapping is unchanged, other windows could still change, so give them a change to realize
			if (!result) _WNSendMessageToMDIKids(WM_PALETTECHANGED, (WPARAM)tmpWin, NULL);
		}
		break;
		
		case WM_PALETTECHANGED:
		{
			_WNSendMessageToMDIKids(WM_PALETTECHANGED, inWParam, inLParam);
		}
		break;

		case WM_COMMAND:
			Int16 menuAndItem[2] = { 0, LOWORD(inWParam) };
			UApplication::PostMessage(999, menuAndItem, sizeof(menuAndItem));
			break;

		// the WM_SETFOCUS code in DefFrameProc just loves to crash so we're not going to let it run
		case WM_SETFOCUS:
			HWND topWin = ::GetTopWindow(_gMDIClient);
			if (topWin) ::SetFocus(topWin);
			break;
			
		case WM_NCMOUSEMOVE:
		case WM_MOUSEMOVE:
		{
			result = DefFrameProc(inWnd, _gMDIClient, inMsg, inWParam, inLParam);
			
			if (_gMouseWithinWin)
			{
				SMouseMsgData minfo;
				minfo.time = ::GetMessageTime();
				minfo.loc.x = (Int16)LOWORD(inLParam);
				minfo.loc.y = (Int16)HIWORD(inLParam);
				minfo.button = minfo.mods = 0;
				minfo.misc = 0;
				
				SWindow *tmpRef = _gMouseWithinWin;
				_gMouseWithinWin = nil;

				if (tmpRef->msgProc)
					UApplication::SendMessage(msg_MouseLeave, &minfo, sizeof(minfo), priority_MouseLeave, tmpRef->msgProc, tmpRef->msgProcContext, tmpRef);
			}
		}
		break;
				
		default:
			result = DefFrameProc(inWnd, _gMDIClient, inMsg, inWParam, inLParam);
			break;
	}
		
	// must not continue exceptions out of this function
	} catch(...) {}

	return result;
}

void _SetWinIcon(TWindow inRef, Int16 inID)
{
	Require(inRef && inID);
	
	HICON icon = ::LoadIcon(_gProgramInstance, MAKEINTRESOURCE(inID));	
	if (icon) ::SendMessageA(REF->hwnd, WM_SETICON, false, (LPARAM)icon);
}

void _RemoveFromLayer(SWindow *inRef)
{
	SWindow *win = _gTopWin;
	SWindow **prevPtr = &_gTopWin;
	
	// remove the window from the list
	while (win)
	{
		if (win == inRef)
		{
			*prevPtr = win->next;
			break;
		}
	
		prevPtr = &(win->next);
		win = win->next;
	}
}

HWND _AddToLayerAndGetInsertAfter(SWindow *inRef, HWND inInsertAfterWin)
{
	if (!_gTopWin)
	{
		_gTopWin = inRef;
		return inInsertAfterWin;
	}

	SWindow *win = _gTopWin;
	SWindow *prevWin = nil;
	
	if (_gAppDeactivated)	// if the app was just activated, we need to reposition all windows above this one too
	{
		_gAppDeactivated = false;

		while (win)
		{
			if (win->layer >= inRef->layer)
				break;
			
			if (prevWin)
				::SetWindowPos(win->hwnd, prevWin->hwnd, nil, nil, nil, nil, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			else
				::SetWindowPos(win->hwnd, inInsertAfterWin, nil, nil, nil, nil, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			
			prevWin = win;
			win = win->next;
		}
	}
	else
	{
		while (win)
		{
			if (win->layer >= inRef->layer)
				break;
			
			prevWin = win;
			win = win->next;
		}
	}
	
	inRef->next = win;

	if (prevWin)
	{
		prevWin->next = inRef;
		return _GetPrevVisibleWindow(prevWin, inInsertAfterWin);
	}
	else
	{
		_gTopWin = inRef;
		return inInsertAfterWin;
	}
}

// put at the top of the right layer and bring to front all windows that should be on top of me
void _PutInLayer(SWindow *inRef)
{
	if (!_gTopWin)
	{
		_gTopWin = inRef;
		return;
	}

	SWindow *win = _gTopWin;
	SWindow *prevWin = nil;	
	
	while (win)
	{
		if (win->layer >= inRef->layer)
			break;
			
		prevWin = win;
		win = win->next;
	}
	
	inRef->next = win;

	if (prevWin)
	{
		prevWin->next = inRef;
		::SetWindowPos(inRef->hwnd, _GetPrevVisibleWindow(prevWin, HWND_TOP), nil, nil, nil, nil, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	else
	{
		_gTopWin = inRef;
		::SetWindowPos(inRef->hwnd, HWND_TOP, nil, nil, nil, nil, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

SWindow *_GetTopVisibleModalWindow()
{
	SWindow *win = _gTopWin;
	
	while (win)
	{
		if ((win->layer == windowLayer_Modal || win->layer == windowLayer_Popup) && /*win->state != windowState_Hidden*/ ::IsWindowVisible(win->hwnd) && win->state != windowState_Deleting)
			return win;
		
		if (win->layer > windowLayer_Modal)
			return nil;
		
		win = win->next;
	}

	return nil;
}

HWND _GetPrevVisibleWindow(SWindow *inRef, HWND inDefault)
{
	if (!inRef)
		return inDefault;
		
	if (::IsWindowVisible(inRef->hwnd))
		return inRef->hwnd;
	
	SWindow *win = _gTopWin;
	SWindow *prevWin = nil;	
	
	while (win)
	{
		if (win == inRef)
		{
			if (!prevWin)
				return inDefault;
			else if (::IsWindowVisible(prevWin->hwnd))
				return prevWin->hwnd;
			else
				return _GetPrevVisibleWindow(prevWin, inDefault);
		}
		
		prevWin = win;
		win = win->next;
	}

	return inDefault;
}

HWND _GetBottomWindow(SWindow *inRef)
{
	HWND hBottomWin = HWND_BOTTOM;

	if (inRef)
	{	
		SWindow	*win = _gTopWin;
		while (win)
		{
			if (win->layer != windowLayer_Modal && ::IsWindowVisible(win->hwnd) && !UWindow::IsChild((TWindow)win, (TWindow)inRef) && !UWindow::IsChild((TWindow)inRef, (TWindow)win))
				hBottomWin = win->hwnd;
	
			win = win->next;
		}
	}
	
	return hBottomWin;
}

HWND _GetModalParentWindow()
{
	SWindow *win = _GetTopVisibleModalWindow();
	if (win)
		return win->hwnd;
	
	if (_gMainWindow)
		return _gMainWindow->hwnd;

	return _gModalParent;
}

bool _IsTopVisibleModalWindow()
{
	if (_StandardDialogBox())
		return true;
	
	SWindow *win = _gTopWin;

	while (win)
	{
		if (win->layer == windowLayer_Modal && ::IsWindowVisible(win->hwnd) && win->state != windowState_Deleting)
			return true;
				
		win = win->next;
	}

	return false;
}

bool _IsInWindowList(HWND inHwnd)
{
	if (!inHwnd)
		return false;

	SWindow	*win = _gTopWin;
	while (win)
	{
		if (win->hwnd == inHwnd)
			return true;
	
		win = win->next;
	}
	
	return false;
}

static void _StateTimerProc(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	#pragma unused(inContext, inObject, inData, inDataSize)
	
	if (inMsg == msg_Timer)
		_CheckWindowStates();
}

static void _FlashTimerProc(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	#pragma unused(inContext, inObject, inData, inDataSize)
	
	if (inMsg == msg_Timer && _gMainWindow)
	{
		if (_APInBkgndFlag)		// continue flashing
		{
			::FlashWindow(_gMainWindow->hwnd, true);
			
			if (_gFlashTimer)
				_gFlashTimer->Start(700, kOnceTimer);	
		}
		else					// stop flashing
			::FlashWindow(_gMainWindow->hwnd, false);
	}
}

void _CheckWindowStates()
{
	if (_StandardDialogBox())
		return;
			
	// get foreground window
	HWND hForeWin = ::GetForegroundWindow();

	// check the app status
	if (_APInBkgndFlag == _IsInWindowList(hForeWin))
	{
		_APInBkgndFlag = !_APInBkgndFlag;
	
		if (_APInBkgndFlag)
			UApplication::PostMessage(msg_AppInBackground);
		else
		{
			UApplication::PostMessage(msg_AppInForeground);
		
			// stop flashing
			if (_gFlashTimer && _gFlashTimer->WasStarted())
			{
				_gFlashTimer->Stop();
				
				if (_gMainWindow)
					::FlashWindow(_gMainWindow->hwnd, false);
			}
		}
	}

	// check modal windows
	SWindow	*win = _GetTopVisibleModalWindow();
	if (win)
	{
		while (win->layer == windowLayer_Popup)
		{
			if (::IsWindowVisible(win->hwnd))
			{
				if (hForeWin == win->hwnd)
				{
					if (win->state != windowState_Focus)
					{
						win->state = windowState_Focus;
						if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
					}
				}
				else
				{
					if (win->state == windowState_Hidden || win->state == windowState_Inactive || win->state == windowState_Focus)
					{
						win->state = windowState_Active;
						if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
					}
				}
			}
			else
			{
				if (win->state != windowState_Hidden)
				{
					win->state = windowState_Hidden;
					if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
				}
			}
				
			win = win->next;	
			
			if (!win)
				return;
		}

yesModalWins:

		if (win->layer != windowLayer_Modal)
			goto noModalWins;

		// check if the front-most modal is focused
		// if not, post a focus msg
		if (::IsWindowVisible(win->hwnd))
		{
			if (win->hwnd == hForeWin)
			{
				if (win->state != windowState_Focus)
				{
					win->state = windowState_Focus;
					if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
				}
			}
			else
			{
				if (win->state == windowState_Hidden || win->state == windowState_Inactive || win->state == windowState_Focus)
				{
					win->state = windowState_Active;
					if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
				}
			}			
		}
		else
		{
			if (win->state != windowState_Hidden)
			{
				win->state = windowState_Hidden;
				if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
			}
			
			win = win->next;
			
			if (!win)
				return;
				
			goto yesModalWins;
		}
		// check that all floats and stds are disabled
		// if not, post a disable		
		
		win = win->next;

		while (win)
		{
			if (win->layer != windowLayer_Modal)	// but wait...should the top modal prevent access to lower ones? eg, options window is open, then agreement shows up...
			{
				if (win->state == windowState_Active || win->state == windowState_Focus)
				{
					win->state = windowState_Inactive;
					if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
				}
			}
		
			win = win->next;
		}
	}
	else
	{
noModalWins:
		// get the windows focused window

		//HWND activeWin = ::GetActiveWindow();
		win = _gTopWin;
		
		while (win)
		{
			if (win->state == windowState_Deleting)
			{
				win = win->next;
				continue;
			}
			
			if (::IsWindowVisible(win->hwnd))
			{
				// if this window == activeWin, check if the state is already focused and if not, post a focus msg
				if (win->hwnd == hForeWin)
				{
					// ahh, but if we're going to delete this window, we can't have a msg being posted...
					// use 0 as BeingDeleted
					if (win->state != windowState_Focus && win->state != windowState_Deleting)
					{
						win->state = windowState_Focus;
						if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
					}
				}
				else
				{
					// else check if we're focused if so, post an unfocus msg or if inactive, time to activate (since no modal)
					if (win->state == windowState_Hidden || win->state == windowState_Inactive || win->state == windowState_Focus)
					{
						win->state = windowState_Active;
						if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
					}
				}
			}
			else
			{
				if (win->state != windowState_Hidden)
				{
					win->state = windowState_Hidden;
					if (win->msgProc) UApplication::ReplaceMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
				}
			}
		
			win = win->next;
		}
	}
}

bool _StandardDialogBox(Uint8 inActive)
{
	static bool bActive = false;
	
	if (inActive == 1)
		bActive = true;
	else if (inActive == 2)
	{
		bActive = false;
		_CheckWindowStates();
	}
		
	if (bActive)
	{
		SWindow *win = _gTopWin;

		while (win)
		{
			if (win->state == windowState_Active || win->state == windowState_Focus)
			{
				win->state = windowState_Inactive;
				if (win->msgProc) UApplication::SendMessage(msg_StateChanged, nil, 0, priority_Draw+1, win->msgProc, win->msgProcContext, win);
			}
			
			win = win->next;
		}
	}
	
	return bActive;
}

void _ShowWantsAttention()
{
	if (_gMainWindow && _APInBkgndFlag)
	{
		// start flashing
		if (!_gFlashTimer)
			::FlashWindow(_gMainWindow->hwnd, true);
		else if(!_gFlashTimer->WasStarted())
		{
			::FlashWindow(_gMainWindow->hwnd, true);
			_gFlashTimer->Start(700, kOnceTimer);	
		}	
	}
}

bool _ActivateNextWindow()
{
	// if is a modal window return
	if (_IsTopVisibleModalWindow())
		return false;

	// get active window
	HWND hWin = ::GetActiveWindow();
	if (!hWin)
		return false;

	Uint32 i = 0;
	CWindow *pWin;
	SWindow	*pSWin;
		
	// search active window
	bool bFound = false;
	while (gWindowList.GetNextWindow(pWin, i))
	{
		pSWin = (SWindow *)((TWindow)*pWin);
		if (pSWin->hwnd == hWin)
		{
			bFound = true;
			break;
		}
	}
	
	if (!bFound)
		return false;
		
	// search next window
	bFound = false;
	while (gWindowList.GetNextWindow(pWin, i))
	{
		pSWin = (SWindow *)((TWindow)*pWin);
		if (pSWin->hwnd == hWin)
			return false;
		
		if (::IsWindowVisible(pSWin->hwnd))
		{
			bFound = true;
			break;
		}
	}
	
	// start from begining of the list
	if (!bFound)
	{
		i = 0;
		
		// search next window
		bFound = false;
		while (gWindowList.GetNextWindow(pWin, i))
		{
			pSWin = (SWindow *)((TWindow)*pWin);
			if (pSWin->hwnd == hWin)
				return false;
		
			if (::IsWindowVisible(pSWin->hwnd))
			{
				bFound = true;
				break;
			}
		}
		
		if (!bFound)
			return false;
	}
	
	pSWin = (SWindow *)((TWindow)*pWin);
	::SetActiveWindow(pSWin->hwnd);

	return true;
}

void _ActivateNextApp()
{
	if (_gMainWindow && _gMainWindow->hwnd == ::GetForegroundWindow())
	{
		HWND hWin = _gMainWindow->hwnd;

		do
		{
			hWin = ::GetWindow(hWin, GW_HWNDNEXT);
			if (!hWin)
				hWin = ::GetDesktopWindow();
		
		} while (!::IsWindowVisible(hWin) || _IsInWindowList(hWin));
				
		::BringWindowToTop(hWin);
	}
}

void _WNDispatchDrag(TWindow inWin, TDrag inDrag, Uint32 inMsg, DWORD grfKeyState, POINTL pt)
{
	const Uint32 dragMsgs[] = { msg_Draw, msg_DragEnter, msg_DragMove, msg_DragLeave, msg_DragDropped, 0 };

	SWindow *win = (SWindow *)inWin;
	
	SDragMsgData info;
	info.mods = 0;
	info.button = 0;
	info.time = UDateTime::GetMilliseconds();
	info.loc.Set(pt.x, pt.y);
	UWindow::GlobalToLocal(inWin, info.loc);
	info.drag = inDrag;

	if (grfKeyState & MK_CONTROL) info.mods |= modKey_Control;
	if (grfKeyState & MK_SHIFT) info.mods |= modKey_Shift;
	if (grfKeyState & MK_ALT) info.mods |= modKey_Alt;
	
	if (grfKeyState & MK_LBUTTON)	info.button != mouseButton_Left;
	if (grfKeyState & MK_MBUTTON)	info.button != mouseButton_Middle;
	if (grfKeyState & MK_RBUTTON)	info.button != mouseButton_Right;
	
	UApplication::PostMessage(inMsg, &info, sizeof(info), priority_High, win->msgProc, win->msgProcContext, win);
	
	// it is essential that we process ALL drag messages because info is invalid when we return
	UApplication::ProcessOnly(dragMsgs);				
}


// this is ugly as heck!  TO DO  fix it up
// first of all, it does not take care of middle+right clicks
// also, I'm not guaranteed I need a mouse-up when DoDragDrop is called
// and finally, I don't know for certain that the captured window is the one with the mouse down
// although 99% of the time it is.
	
// this function is called after a drag and drop is done, because the mouseup doesn't get posted to the win proc after a drag-and-drop released
void _WNPostMouseUp()
{
	HWND win = ::GetCapture();	
	if (!win)
		return;
		
	SPoint loc;
	UMouse::GetLocation(loc);
	_WNWndProc(win, WM_LBUTTONUP, 0, loc.x << 16 | loc.y);	
}


#pragma mark -

void _GetRealWinBounds(TWindow inWindow, SRect& outWindowBounds)
{
	WINDOWPLACEMENT pm;
	pm.length = sizeof(pm);

	if (!::GetWindowPlacement(((SWindow *)inWindow)->hwnd, &pm))
		FailLastWinError();

	outWindowBounds.Set(pm.rcNormalPosition.left, pm.rcNormalPosition.top, pm.rcNormalPosition.right, pm.rcNormalPosition.bottom);
}

void _GetRealWinBounds(CWindow *inWindow, SRect& outWindowBounds)
{
	_GetRealWinBounds(*inWindow, outWindowBounds);	
}

void _SetRealWinBounds(CWindow *inWindow, SRect inWindowBounds)
{
	::SetWindowPos(((SWindow *)(TWindow)*inWindow)->hwnd, NULL, inWindowBounds.left, inWindowBounds.top, inWindowBounds.GetWidth(), inWindowBounds.GetHeight(), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

void _SetRealWinLocation(CWindow *inWindow, SPoint inWindowLocation, CWindow *inInsertAfter)
{
	Uint32 nNoZOrder;
	HWND hWndInsertAfter;
	
	if (inInsertAfter)
	{
		nNoZOrder = 0;
		hWndInsertAfter = ((SWindow *)(TWindow)*inInsertAfter)->hwnd;
	}
	else
	{
		nNoZOrder = SWP_NOZORDER;
		hWndInsertAfter = NULL;
	}
	
	::SetWindowPos(((SWindow *)(TWindow)*inWindow)->hwnd, hWndInsertAfter, inWindowLocation.x, inWindowLocation.y, 0, 0, SWP_NOSIZE | nNoZOrder | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

void _GetScreenBounds(SRect& outScreenBounds)
{
	if (_gUseMDIWindow)
		::GetClientRect(_gMDIClient, (RECT *)&outScreenBounds);
	else
	{
		::GetClientRect(::GetDesktopWindow(), (RECT *)&outScreenBounds);
		
		APPBARDATA abd;
		UMemory::Clear(&abd, sizeof(abd));
		abd.cbSize = sizeof(APPBARDATA);
		abd.hWnd = nil;
		::SHAppBarMessage(ABM_GETSTATE, &abd);
		if (abd.lParam & ABS_AUTOHIDE == 0)	 // not autohide => need to avoid the task bar
		{
			UMemory::Clear(&abd, sizeof(abd));
			abd.cbSize = sizeof(APPBARDATA);
			abd.hWnd = nil;
			::SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
		}

		if (outScreenBounds.left < abd.rc.left)
			outScreenBounds.right = abd.rc.left;
		else if (outScreenBounds.top < abd.rc.top)
			outScreenBounds.bottom = abd.rc.top;
		else if (outScreenBounds.right > abd.rc.right)
			outScreenBounds.left = abd.rc.right;
		else if (outScreenBounds.bottom > abd.rc.bottom)
			outScreenBounds.top = abd.rc.bottom;
	}
}

static void _WNCalcUnobstructedBounds(TWindow inRef, SRect& rBounds)
{
	Require(inRef);

	WINDOWPLACEMENT pm;
	pm.length = sizeof(pm);

	if (!::GetWindowPlacement(REF->hwnd, &pm))
		FailLastWinError();
	
	if (pm.showCmd == SW_SHOWMAXIMIZED)
		return;

	SRect rScreenBounds;
	_GetScreenBounds(rScreenBounds);

	SRect rWindowBounds;
	_GetRealWinBounds(inRef, rWindowBounds);
	
	CWindow *win = nil;
	Uint32 i = 0;
		
	while (gWindowList.GetNextWindow(win, i))
	{
		if ((TWindow)*win == inRef || win->GetLayer() != windowLayer_Float || !win->IsVisible())
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
	
	rBounds.left = rWindowBounds.left + REF->frameSizeLeft;
	rBounds.top = rWindowBounds.top + REF->frameSizeTop;
	rBounds.right = rWindowBounds.right - REF->frameSizeRight;
	rBounds.bottom = rWindowBounds.bottom - REF->frameSizeBottom;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void _RegisterWithQuickTime(TWindow inRef)
{
	Require(inRef);
	
	if (!REF->isQuickTimeView)
	{
		REF->isQuickTimeView = true;
		
		// associate a GrafPort with this window and set the port
		::CreatePortAssociation(REF->hwnd, nil, 0L);
		::MacSetPort(::GetHWNDPort(REF->hwnd));
	}
}

void _InitQuickTime(TWindow inRef, Movie inMovie)
{
	Require(inRef);

	// resize the movie bounding rect and offset to 0,0
	Rect stBounds;	
	::GetMovieBox(inMovie, &stBounds);
	::MacOffsetRect(&stBounds, -stBounds.left, -stBounds.top);
	::SetMovieBox(inMovie, &stBounds);
	::AlignWindow(::GetHWNDPort(REF->hwnd), false, &stBounds, nil);

	::SetMovieGWorld(inMovie, (CGrafPtr)::GetHWNDPort(REF->hwnd), ::GetGWorldDevice((CGrafPtr)::GetHWNDPort(REF->hwnd)));
}

void _UpdateQuickTime(TWindow inRef, MovieController inController, Movie inMovie, bool inGrowBox)
{
	#pragma unused(inMovie, inGrowBox)
	Require(inRef);
	
	// draw the movie controler and its movie
	::MCDoAction(inController, mcActionDraw, ::GetHWNDPort(REF->hwnd));
}

static bool _SendToQuickTime(TWindow inRef, UINT inMsg, WPARAM inWParam, LPARAM inLParam)
{
	Require(inRef);

	// if this window contain a CQuickTimeView must pass all events to QuickTime
	if (REF->isQuickTimeView && UWindow::IsVisible(inRef) && !UWindow::IsCollapsed(inRef))
	{
		MSG stMsg;
		
		stMsg.hwnd = REF->hwnd;
		stMsg.message = inMsg;
		stMsg.wParam = inWParam;
		stMsg.lParam = inLParam;
		stMsg.time = ::GetMessageTime();
		Uint32 nPos = ::GetMessagePos();
		stMsg.pt.x = LOWORD(nPos);
		stMsg.pt.y = HIWORD(nPos);
		
		EventRecord stEvent;
		::WinEventToMacEvent(&stMsg, &stEvent);
		
		UApplication::SendMessage(msg_SendToQuickTime, &stEvent, sizeof(stEvent), priority_Draw, REF->msgProc, REF->msgProcContext, REF);
		return true;
	}
	
	return false;
}

static void _UnregisterWithQuickTime(TWindow inRef)
{
	Require(inRef);

	if (REF->isQuickTimeView)
		::DestroyPortAssociation((CGrafPtr)::GetHWNDPort(REF->hwnd));
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static HINSTANCE _hCydoorLib = nil;

typedef int (CALLBACK* LPPROCDLL_ServiceShow)(int, int, int, HWND, int, int, int, int, int, void*, void*);
typedef int (CALLBACK* LPPROCDLL_ServiceClose)(int, HWND, void*);

/*
#define CY_SET_CALLBACK		(unsigned)2<<9

void CALLBACK _CydoorCallback(HWND inAdWindow, void *inCommand, void *inArgument)
{
	// we get a callback from the show thread
	if (!UText::CompareInsensitive(inCommand,"ShowErrorLevel", 14))
	{
		if (!(BOOL)inArgument)
			UApplication::PostMessage(1535); // quit the application if we cannot display banners
	}
}
*/

bool _CydoorServiceShow(CWindow *inWindow, const SRect& inBounds)
{
	if (_hCydoorLib)
		return false;
	
	// load Cydoor library
	_hCydoorLib = ::LoadLibrary("CD_Clint.dll");
	if (!_hCydoorLib)
		return false;
	
	// get proc address
	LPPROCDLL_ServiceShow lpProc_ServiceShow = (LPPROCDLL_ServiceShow)::GetProcAddress(_hCydoorLib, "ServiceShow");
	if (!lpProc_ServiceShow)
		return false;

	// call ServiceShow (allows just one location for the banner)
//	if (!lpProc_ServiceShow(280, 1, 0, ((SWindow *)(TWindow)*inWindow)->hwnd, inBounds.left, inBounds.top, inBounds.GetWidth(), inBounds.GetHeight(), CY_SET_CALLBACK, _CydoorCallback, nil))
	if (!lpProc_ServiceShow(280, 1, 0, ((SWindow *)(TWindow)*inWindow)->hwnd, inBounds.left, inBounds.top, inBounds.GetWidth(), inBounds.GetHeight(), 0, nil, nil))
		return false;

	return true;
}

bool _CydoorServiceSetBounds(CWindow *inWindow, const SRect& inBounds)
{
	if (!_hCydoorLib)
		return false;

	// get proc address
	LPPROCDLL_ServiceClose lpProc_ServiceClose = (LPPROCDLL_ServiceClose)::GetProcAddress(_hCydoorLib, "ServiceClose");
	if (!lpProc_ServiceClose)
		return false;
	
	// call ServiceClose
	if (!lpProc_ServiceClose(0, ((SWindow *)(TWindow)*inWindow)->hwnd, nil))
		return false;

	// get proc address
	LPPROCDLL_ServiceShow lpProc_ServiceShow = (LPPROCDLL_ServiceShow)::GetProcAddress(_hCydoorLib, "ServiceShow");
	if (!lpProc_ServiceShow)
		return false;

	// call ServiceShow (allows just one location for the banner)
//	if (!lpProc_ServiceShow(280, 1, 0, ((SWindow *)(TWindow)*inWindow)->hwnd, inBounds.left, inBounds.top, inBounds.GetWidth(), inBounds.GetHeight(), CY_SET_CALLBACK, _CydoorCallback, nil))
	if (!lpProc_ServiceShow(280, 1, 0, ((SWindow *)(TWindow)*inWindow)->hwnd, inBounds.left, inBounds.top, inBounds.GetWidth(), inBounds.GetHeight(), 0, nil, nil))
		return false;
		
	return true;
}

bool _CydoorServiceClose(CWindow *inWindow)
{
	if (!_hCydoorLib)
		return false;
	
	// get proc address
	LPPROCDLL_ServiceClose lpProc_ServiceClose = (LPPROCDLL_ServiceClose)::GetProcAddress(_hCydoorLib, "ServiceClose");
	if (!lpProc_ServiceClose)
	{
		// unload Cydoor library
		::FreeLibrary(_hCydoorLib);
		_hCydoorLib = nil;
		
		return false;
	}
	
	// call ServiceClose
	if (!lpProc_ServiceClose(0, ((SWindow *)(TWindow)*inWindow)->hwnd, nil))
	{
		// unload Cydoor library
		::FreeLibrary(_hCydoorLib);
		_hCydoorLib = nil;
		
		return false;
	}
		
	// unload Cydoor library
	::FreeLibrary(_hCydoorLib);
	_hCydoorLib = nil;
	
	return true;
}

#endif /* WIN32 */
