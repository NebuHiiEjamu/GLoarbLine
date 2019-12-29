/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CQuickTimeView.h"

void _RegisterWithQuickTime(TWindow inWin);
void _InitQuickTime(TWindow inWin, Movie inMovie);
void _UpdateQuickTime(TWindow inWin, MovieController inController, Movie inMovie, bool inGrowBox);
void _GetQuickTimeFSSpec(TFSRefObj* inRef, FSSpec& outSpec);

#if TARGET_API_MAC_CARBON
	static MCActionFilterWithRefConUPP gActionFilterWithRefConUPP = ::NewMCActionFilterWithRefConUPP(_QuickTimeFilterProc);
#else
	#define gActionFilterWithRefConUPP	NewMCActionFilterWithRefConProc(_QuickTimeFilterProc)
#endif


Uint32 CQuickTimeView::sMovieCount = 0L;

CQuickTimeView::CQuickTimeView(CViewHandler *inHandler, const SRect& inBounds, Uint16 inResizeOptions, Uint16 inOptions)
	: CLaunchUrlView(inHandler, inBounds)
{
	sMovieCount++;
	
	mMovie = nil;
	mController = nil;
	mInstance = nil;

	mIsVideoTrack = false;
	mTempFile = nil;

	mResizeOptions = inResizeOptions;
	mOptions = inOptions;
	mMaxHorizSize = 0;
	mMaxVertSize = 0;

	if (mOptions & qtOption_ResolveGrowBox)	
	{
#if WIN32
		mOptions &= ~qtOption_ResolveGrowBox;	// ignore qtOption_ResolveGrowBox flag on Windows
#else
		if (mOptions & qtOption_ShowGrowBox)		
			mOptions &= ~qtOption_ShowGrowBox;	// ignore qtOption_ShowGrowBox flag on Macintosh
#endif
	}

	// register this window with QuickTime
	if (UOperatingSystem::IsQuickTimeAvailable())
	{
		CWindow *pWindow = GetParentWindow();
		if (pWindow) 
			_RegisterWithQuickTime(*pWindow);
	}
}
 
CQuickTimeView::~CQuickTimeView()
{
	CloseMovie();
	sMovieCount--;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CQuickTimeView::SelectMovie()
{
	if (!UOperatingSystem::IsQuickTimeAvailable())
		return false;
	
	TFSRefObj* pMovieFile = UFileSys::UserSelectFile();
	if (!pMovieFile)
		return false;
	
	scopekill(TFSRefObj, pMovieFile);
	CloseMovie();
		
	return MakeMovieFromFile(pMovieFile);
}

bool CQuickTimeView::StreamMovie(Int8 *inAddress)
{
	if (!UOperatingSystem::IsQuickTimeAvailable() || !inAddress)
		return false;

	CloseMovie();
	
	Handle hAddress = ::NewHandle(strlen(inAddress) + 1);
	if (!hAddress)
		return false;
	
	::BlockMoveData(inAddress, *hAddress, strlen(inAddress) + 1);

	bool bUseComponent = false;
	if (!UText::CompareInsensitive(inAddress, "hotline://", 10))
		bUseComponent = true;

	if (!MakeMovieFromUrlHandle(hAddress, bUseComponent))
	{
		::DisposeHandle(hAddress);
		return false;
	}

	::DisposeHandle(hAddress);
	return true;
}

bool CQuickTimeView::SetMovie(void *inData, Uint32 inDataSize, Uint32 inTypeCode)
{
	if (!UOperatingSystem::IsQuickTimeAvailable() || !inData || !inDataSize)
		return false;

	CloseMovie();
	
	mTempFile = new CTempFile();
	mTempFile->SetTempFile("\pMovie", inTypeCode, 'HTLC', false);
	
	try
	{
		if (!mTempFile->WriteFileData(inData, inDataSize))
		{
			delete mTempFile;
			mTempFile = nil;
		
			return false;		
		}
	}
	catch(...)
	{
		delete mTempFile;
		mTempFile = nil;
		
		return false;		
	}

	if (!MakeMovieFromFile(mTempFile->GetFile()))
	{
		delete mTempFile;
		mTempFile = nil;
		
		return false;
	}
			
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CQuickTimeView::StartMovie()
{
	if (!mMovie)
		return false;
		
	::StartMovie(mMovie);
	return true;
}

bool CQuickTimeView::StopMovie()
{
	if (!mMovie)
		return false;

	::StopMovie(mMovie);
	return true;
}

void CQuickTimeView::CloseMovie()
{
	if (mController)
	{
		::DisposeMovieController(mController);
		mController = nil;
	}
	
	if (mMovie)
	{
		::DisposeMovie(mMovie);
		mMovie = nil;
	}

	if (mInstance)
	{
		HL_HandlerClose(mInstance);
		mInstance = nil;
	}

	if (mTempFile)
	{
		delete mTempFile;
		mTempFile = nil;
	}
	
	mIsVideoTrack = false;
}

bool CQuickTimeView::StopStreamMovie()
{
	if (mInstance)
	{
		HL_HandlerStop(mInstance);
		return true;
	}
	
	return false;
}

void CQuickTimeView::SetMaxMovieSize(Uint32 inMaxHorizSize, Uint32 inMaxVertSize)
{
	mMaxHorizSize = inMaxHorizSize;
	mMaxVertSize = inMaxVertSize;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CQuickTimeView::SetBounds(const SRect& inBounds)
{
	if (!CLaunchUrlView::SetBounds(inBounds))
		return false;
	
	UpdateQuickTimeBounds();
	return true;
}

bool CQuickTimeView::SetVisible(bool inVisible)
{
	if (!CLaunchUrlView::SetVisible(inVisible))
		return false;

	if (mController && (mOptions & qtOption_ShowController))
		::MCSetVisible(mController, inVisible);	
	
	if (mMovie)
		::SetMovieActive(mMovie, inVisible);
	
	return true;
}

bool CQuickTimeView::SetEnable(bool inEnable)
{
	if (!CLaunchUrlView::SetEnable(inEnable))
		return false;
		
	if (mController)
		::MCDoAction(mController, inEnable ? mcActionActivate : mcActionDeactivate, nil);
	
	return true;
}

bool CQuickTimeView::ChangeState(Uint16 inState)
{
	if (!CLaunchUrlView::ChangeState(inState))
		return false;

	if (mController)
	{
		if (inState == viewState_Hidden || inState == viewState_Inactive)
			::MCDoAction(mController, mcActionDeactivate, nil);
		else if ((inState == viewState_Active || inState == viewState_Focus) && mIsEnabled)
			::MCDoAction(mController, mcActionActivate, nil);
	}
	
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CQuickTimeView::UpdateQuickTimeBounds()
{
	if (!mMovie || !mController)
		return false;

	Rect stOldBounds;
	GetMovieSize(stOldBounds);

	SPoint stOrigin;
	mHandler->HandleGetOrigin(stOrigin);

	Rect stMovieBounds;
	stMovieBounds.left = stOrigin.x + mBounds.left;
	stMovieBounds.top = stOrigin.y + mBounds.top;
		
	if (mResizeOptions == qtOption_ResizeMovie || mResizeOptions == qtOption_ResizeView || mResizeOptions == qtOption_ResizeWindow)
	{		
		stMovieBounds.right = stMovieBounds.left + mBounds.GetWidth();
		stMovieBounds.bottom = stMovieBounds.top + mBounds.GetHeight();
	}
	else
	{		
		stMovieBounds.right = stMovieBounds.left + (stOldBounds.right - stOldBounds.left);
		stMovieBounds.bottom = stMovieBounds.top + (stOldBounds.bottom - stOldBounds.top);
	}
	
	if (stMovieBounds.left == stOldBounds.left && stMovieBounds.top == stOldBounds.top && stMovieBounds.right == stOldBounds.right && stMovieBounds.bottom == stOldBounds.bottom)
		return false;
	
	// set movie size
	SetMovieSize(stMovieBounds);
	
	// check movie size
	if (mResizeOptions == qtOption_ResizeView || mResizeOptions == qtOption_ResizeWindow)
	{
		Rect stNewBounds;
		GetMovieSize(stNewBounds);
						 
		if (stNewBounds.right != stMovieBounds.right || stNewBounds.bottom != stMovieBounds.bottom)
		{
			if (mResizeOptions == qtOption_ResizeWindow)
			{
				CWindow *pWindow = GetParentWindow();
				if (pWindow)
				{
					SRect stWindowBounds;
					UWindow::GetBounds(*pWindow, stWindowBounds);

					stWindowBounds.right = stWindowBounds.left + stNewBounds.right;
					stWindowBounds.bottom = stWindowBounds.top + stNewBounds.bottom;

					UWindow::SetBounds(*pWindow, stWindowBounds);
				}
			}

			SRect stViewBounds;
			stViewBounds.left = mBounds.left;
			stViewBounds.top = mBounds.top;
			stViewBounds.right = stViewBounds.left + (stNewBounds.right - stNewBounds.left);
			stViewBounds.bottom = stViewBounds.top + (stNewBounds.bottom - stNewBounds.top);
				
			SetBounds(stViewBounds);
		}
	}

	return true;
}

void CQuickTimeView::UpdateQuickTime()
{
	if (mController && mMovie)
	{
		CWindow *pWindow = GetParentWindow();
		if (pWindow)
			_UpdateQuickTime(*pWindow, mController, mMovie, mResizeOptions == qtOption_ResizeWindow);
	}
}

void CQuickTimeView::SendToQuickTime(const EventRecord& inEvent)
{
	if (mController)
		::MCIsPlayerEvent(mController, &inEvent);
}

// if qtOption_ShowSaveAs is enabled this function must be overridden in order to receive the event
bool CQuickTimeView::SaveMovieAs()
{
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CQuickTimeView::IsSupported(Uint32 inTypeCode)
{
	//GGPP disable quick time for testing external app 	
	switch (TB(inTypeCode))
	{
// images
//		case 'GIFf': case 'GIF ':
//		case 'JPEG':
//		case 'BMP ': case 'BMPf':
//		case 'PICT':
//		case 'FPix':
//		case 'PNTG':
//		case '8BPS':
//		case 'PNGf': case 'PNG ':
//		case 'SGI ':
//		case 'TPIC':
//		case 'TIFF':
//		case 'SCRN':
//		case 'PICS':
//		case 'ILBM':
//		case 'clpp':
// video
		case 'MooV': case 'sooV':
		case 'MPEG': case 'Mpeg': case 'mpeg': case 'MPG ': 
		case 'MPGa': case 'MPGv': case 'MPGx': 
		case 'MPG2': case 'MPG3':
//		case 'AVI ': case 'VfW ':
//		case 'dvc!':
//		case 'RTSP':
//		case 'SDP ':
// 3D
//		case 'SWF ': case 'SWFL':
//		case 'FLI ':
//		case '3DMF':
// audio	
//		case 'MP3 ': case 'Mp3 ': case 'mp3 ': case 'MP3U': case 'MP3!': case 'mp3!': case 'SwaT':
//		case 'AIFF': case 'AIFC':
//		case 'ULAW':
//		case 'WAVE': case '.WAV':
//		case 'Sd2f': case 'SD2 ':
//		case 'Midi':
//		case 'trak': case 'CDTK':
//		case 'PLS ':
//		case 'EQST':
//		case 'PLST': case 'PLAY':
			return true;
	};
	
	return false;
}

/*bool CQuickTimeView::IsNotSupported(Uint32 inTypeCode)
{
	switch (TB(inTypeCode))
	{ 
		case 'APPL': case 'DEXE':
		case 'SITD': case 'ZIP ':
		case 'HTft': case 'HTbm': case 'HLNZ':
			return true;
	};
	
	return false;
}*/

// return false if the movie don't have a video track
bool CQuickTimeView::IsVideoTrack()
{
	return mIsVideoTrack;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CQuickTimeView::MouseDown(const SMouseMsgData& inInfo)
{
	if (!MovieContains(inInfo.loc))
		return;
		
	CLaunchUrlView::MouseDown(inInfo);
}

void CQuickTimeView::MouseUp(const SMouseMsgData& inInfo)
{
	if (!MovieContains(inInfo.loc))
		return;

	CLaunchUrlView::MouseUp(inInfo);
}

void CQuickTimeView::MouseEnter(const SMouseMsgData& inInfo)
{
	if (!MovieContains(inInfo.loc))
		return;
	
	CLaunchUrlView::MouseEnter(inInfo);
}

void CQuickTimeView::MouseMove(const SMouseMsgData& inInfo)
{
	if (!MovieContains(inInfo.loc))
	{
		if (mIsMouseWithin)
			CLaunchUrlView::MouseLeave(inInfo);
		
		return;
	}

	if (!mIsMouseWithin)
	{
		CLaunchUrlView::MouseEnter(inInfo);
		return;
	}
	
	CLaunchUrlView::MouseMove(inInfo);
}

void CQuickTimeView::MouseLeave(const SMouseMsgData& inInfo)
{
	if (mIsMouseWithin)
		CLaunchUrlView::MouseLeave(inInfo);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CQuickTimeView::MakeMovieFromFile(TFSRefObj* inMovieFile)
{
	if (!inMovieFile)
		return false;

	CWindow *pWindow = GetParentWindow();
	if (!pWindow)
		return false;

	FSSpec stFileSpec;
	_GetQuickTimeFSSpec(inMovieFile, stFileSpec);
	
	Int16 nFileRefNum;
	if (::OpenMovieFile(&stFileSpec, &nFileRefNum, fsRdPerm) != noErr)
		return false;
	
	if (::NewMovieFromFile(&mMovie, nFileRefNum, nil, nil, 0, nil) != noErr)
	{
		::CloseMovieFile(nFileRefNum);
		return false;
	}

	::CloseMovieFile(nFileRefNum);

	if (mIsVisible)
		::SetMovieActive(mMovie, true);
	
	if (!MakeController(*pWindow, mMovie))
	{
		::DisposeMovie(mMovie);
		mMovie = nil;
		
		return false;
	}
	
	return true;
}

bool CQuickTimeView::MakeMovieFromUrlHandle(Handle inAddress, bool inUseComponent)
{
	if (!inAddress)
		return false;
	
	CWindow *pWindow = GetParentWindow();
	if (!pWindow)
		return false;

	if (inUseComponent)
		mInstance = HL_HandlerOpen(inAddress);

	if (::NewMovieFromDataRef(&mMovie, 0, nil, inAddress, URLDataHandlerSubType) != noErr)
		return false;
	
	if (mIsVisible)
		::SetMovieActive(mMovie, true);

	if (!MakeController(*pWindow, mMovie))
	{
		::DisposeMovie(mMovie);
		mMovie = nil;

		return false;
	}
	
	return true;
}

void _GetScreenBounds(SRect& outScreenBounds);

bool CQuickTimeView::MakeController(TWindow inWindow, Movie inMovie)
{
	if (!inWindow || !inMovie)
		return false;

	_InitQuickTime(inWindow, inMovie);
	
	SPoint stOrigin;
	mHandler->HandleGetOrigin(stOrigin);

	Rect stMovieBounds;
	::GetMovieBox(inMovie, &stMovieBounds);
	Uint32 nMovieWidth = stMovieBounds.right - stMovieBounds.left;
	Uint32 nMovieHeight = stMovieBounds.bottom - stMovieBounds.top;

	if (nMovieWidth && nMovieHeight)
	{
		mIsVideoTrack = true;
		CalculateMovieSize(stMovieBounds, nMovieWidth, nMovieHeight);
	}
	
	stMovieBounds.left = stOrigin.x + mBounds.left;
	stMovieBounds.top = stOrigin.y + mBounds.top;
	stMovieBounds.right = stMovieBounds.left + nMovieWidth;
	stMovieBounds.bottom = stMovieBounds.top + nMovieHeight;

	mController = ::NewMovieController(inMovie, &stMovieBounds, mcTopLeftMovie | mcScaleMovieToFit | mcNotVisible);
	if (!mController)
		return false;
	
	if (mResizeOptions == qtOption_ResizeMovie)
	{
		stMovieBounds.right = stMovieBounds.left + mBounds.GetWidth();
		stMovieBounds.bottom = stMovieBounds.top + mBounds.GetHeight();
	
		// set movie size
		SetMovieSize(stMovieBounds);
	}
	else if (mResizeOptions == qtOption_ResizeView || mResizeOptions == qtOption_ResizeWindow)
	{
		GetMovieSize(stMovieBounds);
				 
		if (mResizeOptions == qtOption_ResizeWindow)
		{
			SRect stWindowBounds;
			UWindow::GetBounds(inWindow, stWindowBounds);

			stWindowBounds.right = stWindowBounds.left + stMovieBounds.right;
			stWindowBounds.bottom = stWindowBounds.top + stMovieBounds.bottom - 1;

			UWindow::SetBounds(inWindow, stWindowBounds);
		}

		SRect stViewBounds;
		stViewBounds.left = mBounds.left;
		stViewBounds.top = mBounds.top;
		stViewBounds.right = stViewBounds.left + (stMovieBounds.right - stMovieBounds.left);
		stViewBounds.bottom = stViewBounds.top + (stMovieBounds.bottom - stMovieBounds.top);
				
		SetBounds(stViewBounds);
	}
		
	::MCSetActionFilterWithRefCon(mController, gActionFilterWithRefConUPP, (Int32)this);
                                         
	if (!mIsEnabled)
		::MCDoAction(mController, mcActionDeactivate , nil);
	
	if (mOptions & qtOption_ShowGrowBox)
	{
		SRect stScreenBounds;
		_GetScreenBounds(stScreenBounds);

		Rect stMaxBounds;
		stMaxBounds.left = stScreenBounds.left;
		stMaxBounds.top = stScreenBounds.top;
		stMaxBounds.right = stScreenBounds.right;
		stMaxBounds.bottom = stScreenBounds.bottom;
		
		::MCDoAction(mController, mcActionSetGrowBoxBounds, (void *)&stMaxBounds);
	}
	
	if (mOptions & qtOption_ShowSaveAs)
	{
		Int32 mControllerFlags;
		::MCDoAction(mController, mcActionGetFlags, &mControllerFlags);
		::MCDoAction(mController, mcActionSetFlags, (void *)(mControllerFlags | mcFlagsUseCustomButton));
	}
	
	if (mOptions & qtOption_LoopMovie)
		::MCDoAction(mController, mcActionSetLooping, (void *)true);
	
	if (mOptions & qtOption_UseBadge)
		::MCDoAction(mController, mcActionSetUseBadge, (void *)true);

	if (mIsVisible && (mOptions & qtOption_ShowController))
		::MCSetVisible(mController, true);	
			
	return true;
}

bool CQuickTimeView::MakeCustomMenu(EventRecord *inEvent)
{
	static MenuHandle hCustomMenu = nil;
	if (!inEvent || hCustomMenu)
		return false;
		
	// create de the menu
	hCustomMenu = ::NewMenu(139, "\pSave As...");
	if (!hCustomMenu)
		return false;

	// add items to the menu
	::MacAppendMenu(hCustomMenu, "\pSave As...");
	
	// insert the menu into the menu list
	::MacInsertMenu(hCustomMenu, hierMenu);
	
	Point stPoint = inEvent->where;
	::LocalToGlobal(&stPoint);
	
	Int32 nItem = 0;
	nItem = ::PopUpMenuSelect(hCustomMenu, stPoint.v, stPoint.h, nItem);
	// returns the menu ID in high-order word and selected submenu ID in low-order word

	switch (nItem & 0x00FF)
	{
		case 1:
			try
			{
				SaveMovieAs();
			}
			catch(...)
			{
				// don't throw
			}
			break;
	};
	
	// remove the menu from the menu list
#if TARGET_API_MAC_CARBON
	::MacDeleteMenu(::GetMenuID(hCustomMenu));
#else	
	::MacDeleteMenu((**hCustomMenu).menuID);
#endif
	
	// dispose the menu
	::DisposeMenu(hCustomMenu);
	hCustomMenu = nil;
	
	return true;	
}

void CQuickTimeView::CalculateMovieSize(Rect &inBounds, Uint32& outWidth, Uint32& outHeight)
{
	outWidth = inBounds.right - inBounds.left;
	outHeight = inBounds.bottom - inBounds.top;

	if (!mMaxHorizSize && !mMaxVertSize)
		return;
		
	float fHorizRatio = 0;
	if (outWidth > mMaxHorizSize)
		fHorizRatio = (float)outWidth/mMaxHorizSize;
		
	float fVertRatio = 0;
	if (outHeight > mMaxVertSize)
		fVertRatio = (float)outHeight/mMaxVertSize;

	if (fHorizRatio || fVertRatio)
	{
		float fMaxRatio = fHorizRatio > fVertRatio ? fHorizRatio : fVertRatio;
		
		outWidth = (float)outWidth/fMaxRatio;
		outHeight = (float)outHeight/fMaxRatio;
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CQuickTimeView::SetWindowSize()
{
	if (!(mOptions & qtOption_ShowGrowBox))
		return;
	
	if (mResizeOptions != qtOption_ResizeView && mResizeOptions != qtOption_ResizeWindow)
		return;
		
	Rect stMovieBounds;
	GetMovieSize(stMovieBounds);
						 
	if (mResizeOptions == qtOption_ResizeWindow)
	{
		CWindow *pWindow = GetParentWindow();
		if (pWindow)
		{
			SRect stWindowBounds;
			UWindow::GetBounds(*pWindow, stWindowBounds);

			SRect stNewWindowBounds;
			stNewWindowBounds.left = stWindowBounds.left;
			stNewWindowBounds.top = stWindowBounds.top;
			stNewWindowBounds.right = stNewWindowBounds.left + stMovieBounds.right;
			stNewWindowBounds.bottom = stNewWindowBounds.top + stMovieBounds.bottom;

			if (stNewWindowBounds.right != stWindowBounds.right || stNewWindowBounds.bottom != stWindowBounds.bottom)
				UWindow::SetBounds(*pWindow, stNewWindowBounds);
		}
	}

	SRect stViewBounds;
	stViewBounds.left = mBounds.left;
	stViewBounds.top = mBounds.top;
	stViewBounds.right = stViewBounds.left + (stMovieBounds.right - stMovieBounds.left);
	stViewBounds.bottom = stViewBounds.top + (stMovieBounds.bottom - stMovieBounds.top);
				
	if (stViewBounds.right != mBounds.right || stViewBounds.bottom != mBounds.bottom)
		SetBounds(stViewBounds);	
}

void CQuickTimeView::SetMovieSize(const Rect &inBounds)
{
	if (mOptions & qtOption_ShowController)
	{
		if (mOptions & qtOption_ResolveGrowBox)
		{
			if (::MCIsControllerAttached(mController))
				::MCSetControllerAttached(mController, false);
				
			Rect stControllerBounds;
			::MCGetControllerBoundsRect(mController, &stControllerBounds);
			Uint32 nControllerHeight = stControllerBounds.bottom - stControllerBounds.top;
			
			if (mIsVideoTrack)
			{
				Rect stMovieBounds = inBounds;
				if (stMovieBounds.bottom > nControllerHeight)
					stMovieBounds.bottom -= nControllerHeight - 1;	// we lose one pixel from the movie here
				else
					stMovieBounds.top = stMovieBounds.bottom = 0;
				
				stControllerBounds.left = stMovieBounds.left;
				stControllerBounds.top = stMovieBounds.bottom;	
				stControllerBounds.right = stMovieBounds.right - 15;	
				stControllerBounds.bottom = stControllerBounds.top + nControllerHeight;
			
				::MCPositionController(mController, &stMovieBounds, &stControllerBounds, mcTopLeftMovie | mcScaleMovieToFit);
			}
			else
			{
				stControllerBounds.left = inBounds.left;
				stControllerBounds.top = inBounds.top;
				stControllerBounds.right = inBounds.right - 15;			
				stControllerBounds.bottom = stControllerBounds.top + nControllerHeight;
				::MCSetControllerBoundsRect(mController, &stControllerBounds);
			}
		}
		else
			::MCSetControllerBoundsRect(mController, &inBounds);
	}
	else
		::SetMovieBox(mMovie, &inBounds);	
}

void CQuickTimeView::GetMovieSize(Rect &outBounds)
{
	if (mOptions & qtOption_ShowController)
	{
		if (mOptions & qtOption_ResolveGrowBox)
		{
			if (::MCIsControllerAttached(mController))
				::MCSetControllerAttached(mController, false);

			Rect stControllerBounds;
			::MCGetControllerBoundsRect(mController, &stControllerBounds);
			Uint32 nControllerHeight = stControllerBounds.bottom - stControllerBounds.top;

			if (mIsVideoTrack)
			{
				::GetMovieBox(mMovie, &outBounds);
				outBounds.bottom += nControllerHeight - 1;	// we lose one line from the movie here
			}
			else
			{
				outBounds.left = stControllerBounds.left;
				outBounds.top = stControllerBounds.top;	
				outBounds.right = stControllerBounds.right + 15;
				outBounds.bottom = stControllerBounds.bottom - 1;
			}
		}
		else
		{
			::MCGetControllerBoundsRect(mController, &outBounds);
			
			if (!mIsVideoTrack)
				outBounds.bottom -= 1;
		}
	}
	else
		::GetMovieBox(mMovie, &outBounds);
}

bool CQuickTimeView::MovieContains(const SPoint& inLocation)
{
	if (mMovie && mIsVideoTrack)
	{
		Rect stMovieBounds;
		::GetMovieBox(mMovie, &stMovieBounds);
	
		if (inLocation.h >= stMovieBounds.left && inLocation.v >= stMovieBounds.top && inLocation.h < stMovieBounds.right && inLocation.v < stMovieBounds.bottom)
			return true;
	}
	
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

pascal Boolean _QuickTimeFilterProc(MovieController inController, Int16 inAction, void *inParam, Int32 inRef)
{
	#pragma unused(inController)
	
	CQuickTimeView *pQuickTimeView = (CQuickTimeView *)inRef;
	
	switch (inAction)
	{
		case mcActionControllerSizeChanged:
			pQuickTimeView->SetWindowSize();
			break;
	
		case mcActionBadgeClick:
			*(bool *)inParam = false;
			break;
			
		case mcActionCustomButtonClick:
			pQuickTimeView->MakeCustomMenu((EventRecord *)inParam);
			break;
	};

	return false;
}

