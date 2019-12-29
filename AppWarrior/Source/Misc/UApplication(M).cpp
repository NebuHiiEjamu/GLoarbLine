#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UApplication.h"

#include <Events.h>
#include <MixedMode.h>
#include <AppleEvents.h>
#include <LowMem.h>
#include <Threads.h>
#include <Icons.h>
#include <Sound.h>

/*
 * Constants
 */

enum {
	kDefaultLowMemoryThresh		= 32768,
	kLowMemorySlop				= 4096
};

/*
 * Functions Prototypes
 */

static bool _APGetMacEvent(EventRecord& outEvent);
static void _APWaitMacEvent(EventRecord& outEvent);
static void _APDispatchMacEvent(EventRecord& inEvent);
static void _APProcess(const Uint32 *inMsgList = nil);
void _APProcessTimers();
static void _APDoMouse(EventRecord& evt);
static void _APDoKey(EventRecord& evt);
static void _APDoUpdate(EventRecord& evt);
#if !TARGET_API_MAC_CARBON
static void _APDoDisk(EventRecord& evt);
#endif
static void _APDoSuspend(EventRecord& evt);
static void _APDoIdle();

static pascal OSErr _APAppleEventQuit(const AppleEvent *inEvent, AppleEvent *inReply, long inRefCon);
static pascal OSErr _APAppleEventOpenDocs(const AppleEvent *inEvent, AppleEvent *inReply, long inRefCon);
#if TARGET_API_MAC_CARBON
static pascal OSErr _APAppleEventShowPrefs(const AppleEvent *inEvent, AppleEvent *inReply, long inRefCon);
#endif

static DialogPtr _APMakeLowMemAlert();
static void _APCheckLowMemory();
bool _APIsInForeground();
Uint16 _MacModsToStd(Uint16 mods);
Int16 _GetSystemVersion();

void _FailMacError(Int16 inMacError, const Int8 *inFile, Uint32 inLine);
inline void _CheckMacError(Int16 inMacError, const Int8 *inFile, Uint32 inLine) { if (inMacError) _FailMacError(inMacError, inFile, inLine); }
#if DEBUG
	#define CheckMacError(id)		_CheckMacError(id, __FILE__, __LINE__)
#else
	#define CheckMacError(id)		_CheckMacError(id, nil, 0)
#endif

/*
 * Global Variables
 */

#if TARGET_API_MAC_CARBON
	static AEEventHandlerUPP _APAppleEventQuitUPP = ::NewAEEventHandlerUPP(_APAppleEventQuit);
	static AEEventHandlerUPP _APAppleEventOpenDocsUPP = ::NewAEEventHandlerUPP(_APAppleEventOpenDocs);
	static AEEventHandlerUPP _APAppleEventShowPrefsUPP = ::NewAEEventHandlerUPP(_APAppleEventShowPrefs);
#else
	static RoutineDescriptor _APAppleEventQuitRD = BUILD_ROUTINE_DESCRIPTOR(uppAEEventHandlerProcInfo, _APAppleEventQuit);
	#define _APAppleEventQuitUPP		(&_APAppleEventQuitRD)

	static RoutineDescriptor _APAppleEventOpenDocsRD = BUILD_ROUTINE_DESCRIPTOR(uppAEEventHandlerProcInfo, _APAppleEventOpenDocs);
	#define _APAppleEventOpenDocsUPP	(&_APAppleEventOpenDocsRD)
#endif

static struct {
	TMessageSys msgSys;
	RgnHandle mouseMoveRgn;
	Point mouseLoc, lastMouseDownLoc, prevMouseDownLoc;
	Uint32 lastMouseDownTicks, prevMouseDownTicks;
	DialogPtr lowMemDialog;
	Uint32 lowMemoryThresh;
	Handle lowMemHdl;
	Handle openDocsHdl;
	NMRecPtr curNotif;
	Uint32 doubleClickTicks;
	Uint16 mouseIsDown		: 1;
	Uint16 isInitted		: 1;
	Uint16 memoryIsLow		: 1;
	Uint16 isQuit			: 1;
	Uint16 canOpenDocs		: 1;
	Uint16 isResumeClick	: 1;
} _APData = { 0 };

void (*_APUpdateProc)(WindowRef inWin) = nil;
void (*_APIdleProc)() = nil;
void (*_APWindowProcess)() = nil;
void (*_APTransportProcess)() = nil;
void (*_APSoundProcess)() = nil;
void (*_APSerialProcess)() = nil;

Uint32 _APTaskCount = 0;
ProcessSerialNumber _APProcSerialNum = { 0, kCurrentProcess };
Uint8 _APWakeupAppFlag = false;
Uint8 _APInBkgndFlag = false;

extern Int16 gMacMenuBarID;
extern Uint8 _WNCheckWinStatesFlag;



/* -------------------------------------------------------------------------- */

void UApplication::Init()
{
	if (!_APData.isInitted)
	{
		try
		{
			_APData.doubleClickTicks = ::GetDblTime();
			
			_APData.msgSys = UMessageSys::New();
			
			_APData.lowMemDialog = _APMakeLowMemAlert();
			if (_APData.lowMemDialog == nil) Fail(errorType_Memory, memError_NotEnough);
			_APData.lowMemoryThresh = kDefaultLowMemoryThresh;
			
			// make region for mouse-moved events
			if (_APData.mouseMoveRgn == nil)
			{
				_APData.mouseMoveRgn = NewRgn();
				if (_APData.mouseMoveRgn == nil) Fail(errorType_Memory, memError_NotEnough);
				Rect r = { -30001,-30001,-30000,-30000 };
				RectRgn(_APData.mouseMoveRgn, &r);
			}
		
			// install AppleEvent handlers
			::AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, _APAppleEventQuitUPP, 0, false);
			::AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, _APAppleEventOpenDocsUPP, 0, false);
		#if TARGET_API_MAC_CARBON
			::AEInstallEventHandler(kCoreEventClass, kAEShowPreferences, _APAppleEventShowPrefsUPP, 0, false);
		#endif
		}
		catch(...)
		{
			// clean up
			if (_APData.mouseMoveRgn)
			{
				DisposeRgn(_APData.mouseMoveRgn);
				_APData.mouseMoveRgn = nil;
			}
			
			UMessageSys::Dispose(_APData.msgSys);
			_APData.msgSys = nil;
			throw;
		}
		
		_APData.isInitted = true;
	}
}

// processes all messages and returns straight away
void UApplication::Process()
{
	Init();
	
	try
	{
		_APProcess(nil);
		_APProcess(nil);
	}
	catch(SError& err)
	{
		Error(err);
		// we don't throw so that the application keeps running
	}
	catch(...)
	{
		// don't throw
	}
}

// processes all messages and doesn't return until there are new messages
void UApplication::ProcessAndSleep()
{
	Init();
	
	try
	{
		_APProcess(nil);
		_APProcess(nil);
		
		// bring up warning alert if memory is running low
		_APCheckLowMemory();
		
		// nothing to do, so idle once
		_APDoIdle();
		
		// wait forever or until an event occurs
		EventRecord macEvt;
		_APWaitMacEvent(macEvt);
		_APDispatchMacEvent(macEvt);
	}
	catch(SError& err)
	{
		Error(err);
		// we don't throw so that the application keeps running
	}
	catch(...)
	{
		// don't throw
	}
}

// processes all messages of types in the specified list (zero-terminated) and returns straight away
void UApplication::ProcessOnly(const Uint32 inMsgList[])
{
	Init();
	
	try
	{
		_APProcess(inMsgList);
	}
	catch(SError& err)
	{
		Error(err);
		// we don't throw so that the application keeps running
	}
	catch(...)
	{
		// don't throw
	}
}

void UApplication::ProcessOnly(Uint32 inMsg)
{
	const Uint32 msgList[2] = { inMsg, 0 };
	ProcessOnly(msgList);
}

#pragma mark -

// continually processes messages and doesn't return until Quit() is called
void UApplication::Run()
{
	Init();
	
	while (!_APData.isQuit)
	{
		Process();
		if (_APData.isQuit) break;
		ProcessAndSleep();
	}
}

// causes Run() to return when convenient
void UApplication::Quit()
{
	UProgramCleanup::CleanupAppl();
	_APData.isQuit = true;
}

// terminates the program immediately (does not return)
void UApplication::Abort()
{
	::ExitToShell();
}

void UApplication::Error(const SError& inError)
{
	/*
	 * IMPORTANT: This function may get called while a drag operation is running.
	 * Thus it's important that it be compatible with the drag operation.  For
	 * example, going into a modal loop to display an error box in the middle of
	 * a drag is not very nice.  Thus, Error() posts an error message.  While the
	 * drag is running, only drag messages get processed.  Thus, any errors
	 * that occured during the drag will nicely show their error boxes when the
	 * drag has completed.
	 */
	
	if (inError.fatal)
		throw inError;

	try
	{
		PostMessage(msg_Error, &inError, sizeof(inError), priority_Draw-1);
	}
	catch(...)
	{
		// don't throw because we've probably been called from another "catch" handler
	}
}

bool UApplication::IsQuit()
{
	return _APData.isQuit;
}

#pragma mark -

void UApplication::SetMessageHandler(TMessageProc inProc, void *inContext)
{
	Init();
	_APData.msgSys->SetDefaultProc(inProc, inContext);
}

void UApplication::SendMessage(Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 /* inPriority */, TMessageProc inProc, void *inContext, void *inObject)
{
	Init();
	try
	{
		if (inProc == nil)
			_APData.msgSys->GetDefaultProc(inProc, inContext);

		inProc(inContext, inObject, inMsg, inData, inDataSize);
	}
	catch(SError& err)
	{
		if (inMsg != msg_Error)	// avoid infinite loop of never-ending error messages
		{
			try { _APData.msgSys->Post(msg_Error, &err, sizeof(err), priority_Draw-1); } catch(...) {}
		}
		// don't throw
	}
	catch(...)
	{
		// don't throw
	}
}

void UApplication::PostMessage(Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority, TMessageProc inProc, void *inContext, void *inObject)
{
	Init();
	_APData.msgSys->Post(inMsg, inData, inDataSize, inPriority, inProc, inContext, inObject);
}

void UApplication::ReplaceMessage(Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority, TMessageProc inProc, void *inContext, void *inObject)
{
	Init();
	_APData.msgSys->Replace(inMsg, inData, inDataSize, inPriority, inProc, inContext, inObject);
}

void UApplication::FlushMessages(TMessageProc inProc, void *inContext, void *inObject)
{
	if (_APData.msgSys)
		_APData.msgSys->Flush(inProc, inContext, inObject);
}

bool UApplication::PeekMessage(Uint32 inMsg, void *outData, Uint32& ioDataSize, TMessageProc inProc, void *inContext, void *inObject)
{
	if (_APData.msgSys)
		return _APData.msgSys->Peek(inMsg, outData, ioDataSize, inProc, inContext, inObject);
		
	return false;
}

#pragma mark -

void UApplication::SetCanOpenDocuments(bool inEnable)
{
	_APData.canOpenDocs = (inEnable != 0);
}

TFSRefObj* _FSSpecToRef(const FSSpec& inSpec);

// returns nil if no document
TFSRefObj* UApplication::GetDocumentToOpen()
{
	FSSpec spec;
	Handle h;
	
	h = _APData.openDocsHdl;
	if (h == nil) 
		return nil;
	
	if (::GetHandleSize(h) < sizeof(FSSpec))
		return nil;
	
	::BlockMoveData(*h, &spec, sizeof(spec));
	::Munger(h, 0, nil, sizeof(FSSpec), "\p", 0);
	
	if (::GetHandleSize(h) == 0)
	{
		::DisposeHandle(h);
		_APData.openDocsHdl = nil;
	}
	
	return _FSSpecToRef(spec);
}

// app is marked as wanting attention, and will stay that way until brought to the front
// the icon displayed is from icon family ID 128, which must NOT be purgeable
void UApplication::ShowWantsAttention()
{
	static NMRec info = {0,0,0,0,0,0,0,0,0,0,0};
	static Handle iconFam = nil;
	
	if (!_APInBkgndFlag || _APData.curNotif) 
		return;
	
	if (iconFam == nil)
	{
	#if TARGET_API_MAC_CARBON
		// check if we are running under MacOS X
		if (_GetSystemVersion() >= 0x0A00)
		{
//			if (::GetIconSuite(&iconFam, 131, kSelectorAllAvailableData))
			if (::GetIconSuite(&iconFam, 128, kSelectorAllSmallData))	//??
				iconFam = nil;
		}
		else
		{
			if (::GetIconSuite(&iconFam, 128, kSelectorAllSmallData))
				iconFam = nil;
		}
	#else
		if (::GetIconSuite(&iconFam, 128, kSelectorAllSmallData))
			iconFam = nil;
	#endif
	}
	
	info.qLink = nil;
	info.qType = nmType;
	info.nmFlags = info.nmReserved = 0;
	info.nmPrivate = info.nmRefCon = 0;
	info.nmMark = 1;
	info.nmIcon = iconFam;
	info.nmSound = nil;
	info.nmStr = nil;
	info.nmResp = nil;
	
	if (::NMInstall(&info) == noErr)
		_APData.curNotif = &info;
}

TFSRefObj* UApplication::GetAppRef()
{
	Uint8 buf[32];
	ProcessInfoRec pir;
	ProcessSerialNumber psn;
	
	psn.highLongOfPSN = psn.lowLongOfPSN = 0;

	pir.processInfoLength = sizeof(pir);
	pir.processName = buf;
	pir.processAppSpec = nil;
	
	CheckMacError(::GetCurrentProcess(&psn));
	CheckMacError(::GetProcessInformation(&psn, &pir));
	
	return UFileSys::New(kProgramFolder, nil, buf);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// returns true if there was an event
static bool _APGetMacEvent(EventRecord& outEvent)
{
	// if the mouse is down, we'll check for mouse-moved events ourself thank you very much
	if (_APData.mouseIsDown && ::Button())
	{
		Point pt;
		::GetMouse(&pt);
		::LocalToGlobal(&pt);
		
		if (pt.h != _APData.mouseLoc.h || pt.v != _APData.mouseLoc.v)	// if mouse moved
		{
			// synthesize mouse moved event
		#if TARGET_API_MAC_CARBON
			::EventAvail(0, &outEvent);
		#else
			::OSEventAvail(0, &outEvent);
		#endif
			
			outEvent.what = osEvt;
			outEvent.message = 0xFA000000;
			outEvent.where = pt;
			return true;
		}
	}

	// get event from the mac
	::GetNextEvent(everyEvent, &outEvent);	// WaitNextEvent() slows program down on some macs
	
	return outEvent.what != nullEvent;
}

static void _APWaitMacEvent(EventRecord& outEvent)
{
	Uint32 sleep;
	
	if (_APWakeupAppFlag)		// if need to wakeup, don't sleep
		sleep = 0;
	else if (_APTaskCount)		// if we have no continual tasks to perform (eg, threads)
		sleep = 10;
	else {						// nothing to do, sleep for awhile
		if (CQuickTimeView::AreMoviesOpen()){
			sleep = 5;
		} else {
			sleep = 100;			// don't sleep for long because it's possible there's something that needs to be done
		}
	}

	// wait for an event (we may awaken before the sleep time has expired if a timer goes off)
	::WaitNextEvent(everyEvent, &outEvent, sleep, _APData.mouseMoveRgn);
	
	_APWakeupAppFlag = false;
}

bool _SendToQuickTime(EventRecord& inEvent);

static void _APDispatchMacEvent(EventRecord& inEvent)
{
	bool bSentToQuickTime = false;
	if (UOperatingSystem::IsQuickTimeAvailable())	// send all messages to QuickTime
		bSentToQuickTime = _SendToQuickTime(inEvent);

	switch (inEvent.what)
	{
		case mouseDown:
		case mouseUp:
			_APDoMouse(inEvent);
			break;
		
		case keyDown:
		case keyUp:
		case autoKey:
			_APDoKey(inEvent);
			break;
		
		case updateEvt:
			_APDoUpdate(inEvent);
			break;
	
	#if !TARGET_API_MAC_CARBON	
		case diskEvt:
			_APDoDisk(inEvent);
			break;
	#endif
	
		case osEvt:
			if (inEvent.message & 0x01000000)						// suspend or resume
				_APDoSuspend(inEvent);
			else if ((inEvent.message & 0xFF000000)==0xFA000000)	// mouse moved
				_APDoMouse(inEvent);
			break;
	
		case kHighLevelEvent:
			::AEProcessAppleEvent(&inEvent);
			break;
	
		case nullEvent:
			_APDoIdle();
			break;
	}

	if (bSentToQuickTime)
		::MoviesTask(nil, 50);
}

// processes everything and returns straight away
static void _APProcess(const Uint32 *inMsgList)
{
	if (!UDragAndDrop::IsTracking())	// if dragging, don't bother processing this stuff
	{
		// process timers
		_APProcessTimers();
		
		// process special procs
		if (_APWindowProcess) _APWindowProcess();
		if (_APTransportProcess) _APTransportProcess();
		if (_APSoundProcess) _APSoundProcess();
		if (_APSerialProcess) _APSerialProcess();
		// process all system events
		EventRecord macEvt;
		while (_APGetMacEvent(macEvt))
			_APDispatchMacEvent(macEvt);
	}
	
	// process all messages
	while (_APData.msgSys->Execute(inMsgList)) {}
}

static void _APUpdateMouseMovedRgn()
{
	Rect r = { _APData.mouseLoc.v, _APData.mouseLoc.h, _APData.mouseLoc.v+1, _APData.mouseLoc.h+1 };
	RectRgn(_APData.mouseMoveRgn, &r);
}

void _WNDispatchMouse(Uint32 inType, const SMouseMsgData& inInfo, bool inResumeClick);

static void _APDoMouse(EventRecord& evt)
{
	SMouseMsgData info;
	
	// extract info
	info.time = (evt.when * 50) / 3;
	info.button = mouseButton_Left;
	info.loc.h = evt.where.h;
	info.loc.v = evt.where.v;
	info.mods = _MacModsToStd(evt.modifiers);
	info.misc = 0;
	
	// handle mouse movement
	if (evt.where.h != _APData.mouseLoc.h || evt.where.v != _APData.mouseLoc.v)	// if mouse moved
	{
		// update mouse location
		_APData.mouseLoc = evt.where;
		_APUpdateMouseMovedRgn();
		
		// post message
		//UApplication::ReplaceMessage(msg_MouseMove, &info, sizeof(info), priority_MouseMove);
		_WNDispatchMouse(msg_MouseMove, info, false);
	}
	
	// handle mouse buttons
	if (evt.what == mouseDown)
	{
		// mouse is now down
		_APData.mouseIsDown = true;
		bool isResumeClick = _APData.isResumeClick;
		_APData.isResumeClick = false;
		
		// store info for double-click checking
		_APData.prevMouseDownLoc = _APData.lastMouseDownLoc;
		_APData.prevMouseDownTicks = _APData.lastMouseDownTicks;
		_APData.lastMouseDownLoc = evt.where;
		_APData.lastMouseDownTicks = evt.when;
		
		if ( ((_APData.lastMouseDownTicks - _APData.prevMouseDownTicks) < _APData.doubleClickTicks) &&
			(abs(_APData.lastMouseDownLoc.h - _APData.prevMouseDownLoc.h) < 5) &&
			(abs(_APData.lastMouseDownLoc.v - _APData.prevMouseDownLoc.v) < 5) )
		{
			_APData.prevMouseDownTicks = 0;	// to ensure another click isn't counted as a double
			_APData.lastMouseDownLoc.h = _APData.lastMouseDownLoc.v = -30000;
			info.mods |= kIsDoubleClick;
		}
		
		// post mouse-down message
		_WNDispatchMouse(msg_MouseDown, info, isResumeClick);
	}
	else if (evt.what == mouseUp)
	{
		// mouse is no longer down
		_APData.mouseIsDown = false;
		
		// check if the window was collapsed or expanded
		_WNCheckWinStatesFlag = true;
		
		// post mouse-up message
		_WNDispatchMouse(msg_MouseUp, info, false);
	}
}

Uint16 _MacKeyCodeToStd(Uint16);
void _WNDispatchKey(Uint32 inType, const SKeyMsgData& inInfo);

static void _APDoKey(EventRecord& evt)
{
	SKeyMsgData info;
	
	// extract info
	info.time = (evt.when * 50) / 3;
	info.keyChar = evt.message & charCodeMask;
	info.keyCode = _MacKeyCodeToStd((evt.message & keyCodeMask) >> 8);
	info.mods = _MacModsToStd(evt.modifiers);
	
	// check for control-k kill combo
#if DEBUG
	if (info.mods & modKey_Control && info.keyCode == key_LeftControl)
		::ExitToShell();
#endif
	
	// mac menubar support
	if ((info.mods & modKey_Command) && gMacMenuBarID && (evt.what == keyDown))
	{
		Uint32 menuAndItem = ::MenuKey(info.keyChar);
		if (HiWord(menuAndItem))
		{
			UApplication::PostMessage(999, &menuAndItem, sizeof(menuAndItem));
			return;
		}
	}
	
	// post key down/up/repeat message
	switch (evt.what)
	{
		case keyDown:
			//UApplication::PostMessage(msg_KeyDown, &info, sizeof(info), priority_KeyDown);
			_WNDispatchKey(msg_KeyDown, info);
			break;
		case keyUp:
			//UApplication::PostMessage(msg_KeyUp, &info, sizeof(info), priority_KeyUp);
			_WNDispatchKey(msg_KeyUp, info);
			break;
		case autoKey:
			//UApplication::PostMessage(msg_KeyRepeat, &info, sizeof(info), priority_KeyRepeat);
			_WNDispatchKey(msg_KeyRepeat, info);
			break;
	}
}

static void _APDoUpdate(EventRecord& evt)
{
	if (_APUpdateProc)
		_APUpdateProc((WindowRef)evt.message);
}

// Carbon does not support the Disk Initialization Manager
// Disk Initialization is supported by the system
#if !TARGET_API_MAC_CARBON
static void _APDoDisk(EventRecord& evt)
{
	if ((evt.message & 0xFFFF0000) != 0)	// if err mounting
	{
		Point pt = { 100, 100 };
		::DILoad();
		::DIBadMount(pt, evt.message);
		::DIUnload();
	}
	
	// drive num is evt.message & 0x0000FFFF
}
#endif

static void _APDoSuspend(EventRecord& evt)
{
	if (evt.message & 0x00000001)	// if resume
	{
		if (_APData.curNotif)
		{
			::NMRemove(_APData.curNotif);
			_APData.curNotif = nil;
		}
				
		if (_APInBkgndFlag)
		{
			_APInBkgndFlag = false;
			UApplication::PostMessage(msg_AppInForeground);
		}
		
		// if the app was brought to the front as a result of a click in a window, remember that
		EventRecord tmp;
		if (::EventAvail(mDownMask, &tmp))
			_APData.isResumeClick = true;
	}
	else if (!_APInBkgndFlag)		// suspend
	{
		_APInBkgndFlag = true;
		UApplication::PostMessage(msg_AppInBackground);
	}
	
	_WNCheckWinStatesFlag = true;
	_APWakeupAppFlag = true;

#if TARGET_API_MAC_CARBON
	Cursor arrowCurs;
	::SetCursor(::GetQDGlobalsArrow(&arrowCurs));
#else		
	::SetCursor(&qd.arrow);
#endif
}

static void _APDoIdle()
{	
	// unhilite menu title from shortcut
	if (::LMGetTheMenu() != 0) 
		::HiliteMenu(0);
			
	// call idle proc
	if (_APIdleProc)
		_APIdleProc();
}

static pascal OSErr _APAppleEventQuit(const AppleEvent */* inEvent */, AppleEvent */* inReply */, long /* inRefCon */)
{
	UApplication::PostMessage(msg_Quit, nil, 0, priority_Quit);
	
	return noErr;
}

static pascal OSErr _APAppleEventOpenDocs(const AppleEvent *inEvent, AppleEvent */* inReply */, long /* inRefCon */)
{
	if (!_APData.canOpenDocs) 
		return noErr;
	
	FSSpec spec;
	AEDescList docList;
	AEKeyword keywd;
	DescType returnedType;
	Size actualSize;
	OSErr err;
	long i, n;
	Handle h;
	
	h = _APData.openDocsHdl;
	if (h == nil)
	{
		h = _APData.openDocsHdl = ::NewHandle(0);
		if (h == nil) return MemError();
	}
	
	err = ::AEGetParamDesc(inEvent, keyDirectObject, typeAEList, &docList);
	if (err) return err;

	err = ::AECountItems(&docList, &n);
	if (err) goto abort;
	
	for (i=1; i<=n; i++)
	{
		err = ::AEGetNthPtr(&docList, i, typeFSS, &keywd, &returnedType, &spec, sizeof(spec), &actualSize);
		if (err) goto abort;
		
		err = ::PtrAndHand(&spec, h, sizeof(spec));
		if (err) goto abort;
	}
	
	try {
		UApplication::PostMessage(msg_OpenDocuments);
	} catch(...) {}
	
abort:
	::AEDisposeDesc(&docList);
	return err;
}

#if TARGET_API_MAC_CARBON
static pascal OSErr _APAppleEventShowPrefs(const AppleEvent */* inEvent */, AppleEvent */* inReply */, long /* inRefCon */)
{
	UApplication::PostMessage(msg_ShowPreferences);
	
	return noErr;
}
#endif

// returns nil if couldn't create
static DialogPtr _APMakeLowMemAlert()
{
	const Uint32 itemListData[] = {
		0x00020000, 0x00000038, 0x00FA004C, 0x01340402, 0x4F4B0000, 0x0000000A, 0x003C0030,
		0x013A8840, 0x4D656D6F, 0x72792069, 0x73207275, 0x6E6E696E, 0x67206C6F, 0x772E2020,
		0x506C6561, 0x73652063, 0x6C6F7365, 0x2077696E, 0x646F7773, 0x20616E64, 0x20736176,
		0x6520646F, 0x63756D65, 0x6E74732E, 0x00000000, 0x000A000A, 0x002A002A, 0xA0020002
	};
	
	Handle itemList;
	Rect bounds = { 126, 96, 215, 418 };
	
	itemList = ::NewHandle(sizeof(itemListData));
	if (itemList == nil) return nil;
	::BlockMoveData(itemListData, *itemList, sizeof(itemListData));
	
	DialogPtr dlg = ::NewDialog(nil, &bounds, "\p", false, dBoxProc, (WindowPtr)-1, false, 0, itemList);
	if (dlg == nil)
	{
		::DisposeHandle(itemList);
		return nil;
	}
	
	return dlg;
}

static void _APShowLowMemAlert()
{
	short n;

	// dialog should be already made, but just in case
	if (_APData.lowMemDialog == nil)
	{
		_APData.lowMemDialog = _APMakeLowMemAlert();
		if (_APData.lowMemDialog == nil) return;
	}
	
	// make sure we have enough memory to display the low memory alert!!
	if (!UMemory::IsAvailable(1024))
	{
		DebugBreak("UApplication - not enough memory to display low memory alert");
		return;
	}

#if TARGET_API_MAC_CARBON
	Rect r;
	::GetPortBounds(::GetWindowPort(::GetDialogWindow(_APData.lowMemDialog)), &r);
#else
	Rect r = _APData.lowMemDialog->portRect;
#endif

#if TARGET_API_MAC_CARBON
	Rect base = ::GetQDGlobalsScreenBits(nil)->bounds;
#else	
	Rect base = qd.screenBits.bounds;
#endif
	
	n = r.right - r.left;
	r.left = base.left + ((base.right - base.left - n) / 2);
	r.right = r.left + n;

	n = r.bottom - r.top;
	r.top = base.top + ((base.bottom - base.top - n) / 3);
	r.bottom = r.top + n;

#if TARGET_API_MAC_CARBON
	::MoveWindow(::GetDialogWindow(_APData.lowMemDialog), r.left, r.top, true);
#else
	::MoveWindow(_APData.lowMemDialog, r.left, r.top, true);
#endif

	::SysBeep(20);
	
#if TARGET_API_MAC_CARBON
	::SelectWindow(::GetDialogWindow(_APData.lowMemDialog));
	::ShowWindow(::GetDialogWindow(_APData.lowMemDialog));
#else
	::SelectWindow(_APData.lowMemDialog);
	::ShowWindow(_APData.lowMemDialog);
#endif

	::ModalDialog(nil, &n);

#if TARGET_API_MAC_CARBON
	::HideWindow(::GetDialogWindow(_APData.lowMemDialog));
#else
	::HideWindow(_APData.lowMemDialog);
#endif

	// app can come from back to front while modal dialog is up
	if (_APIsInForeground())
	{
		if (_APInBkgndFlag)
		{
			_APInBkgndFlag = false;
			UApplication::PostMessage(msg_AppInForeground);
		}
	}
	else if (!_APInBkgndFlag)
	{
		_APInBkgndFlag = true;
		UApplication::PostMessage(msg_AppInBackground);
	}
	
	_WNCheckWinStatesFlag = true;
}

static void _APCheckLowMemory()
{
	if (_APData.lowMemHdl == nil)
	{
		_APData.memoryIsLow = true;
		_APData.lowMemHdl = ::NewEmptyHandle();
		if (_APData.lowMemHdl == nil) Fail(errorType_Memory, memError_NotEnough);
	}
	
	if (*_APData.lowMemHdl == nil)
	{
		::ReallocateHandle(_APData.lowMemHdl, _APData.lowMemoryThresh + kLowMemorySlop);
		
		if (*_APData.lowMemHdl == nil)
		{
			if (!_APData.memoryIsLow)
			{
				_APData.memoryIsLow = true;
				_APShowLowMemAlert();
			}
		}
		else
		{
			_APData.memoryIsLow = false;
			::ReallocateHandle(_APData.lowMemHdl, _APData.lowMemoryThresh);
			HPurge(_APData.lowMemHdl);
		}
	}
}

bool _GetPSNBySig(Uint32 inSignature, ProcessSerialNumber *outPSN)
{
	ProcessSerialNumber PSN = { 0, 0 };
	ProcessInfoRec info;
	
	info.processInfoLength = sizeof(info);
	info.processName = nil;
	info.processAppSpec = nil;

	while (::GetNextProcess(&PSN) == noErr)
	{
		if (::GetProcessInformation(&PSN, &info) == 0 && (Int32)info.processSignature == inSignature)
		{
			if (outPSN) *outPSN = PSN;
			return true;
		}
	}
	
	return false;
}

void _BringProcessToFrontBySig(Uint32 inSignature)
{
	ProcessSerialNumber psn;
	
	if (_GetPSNBySig(inSignature, &psn))
		::SetFrontProcess(&psn);
}

bool _APIsInForeground()
{
	//ProcessSerialNumber thisProcess = { 0, kCurrentProcess };
	ProcessSerialNumber frontProcess;
	Boolean isFront = true;
		
	if (::GetFrontProcess(&frontProcess) == noErr)
		::SameProcess(&_APProcSerialNum/*thisProcess*/, &frontProcess, &isFront);
	
	return isFront;
}

Uint16 _MacModsToStd(Uint16 mods)
{
	Uint16 m = 0;
	
	if (mods & 0x1000)	m |= modKey_Control;
	if (mods & 0x0800)	m |= modKey_Option;
	if (mods & 0x0400)	m |= modKey_CapsLock;
	if (mods & 0x0200)	m |= modKey_Shift;
	if (mods & 0x0100)	m |= modKey_Command;
	
	return m;
}

void _WakeupApp()
{
	_APWakeupAppFlag = true;
	::WakeUpProcess(&_APProcSerialNum);
}

#endif /* MACINTOSH */
