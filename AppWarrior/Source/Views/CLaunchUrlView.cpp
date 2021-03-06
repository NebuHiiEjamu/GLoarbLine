/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CLaunchUrlView.h"


CLaunchUrlView::CLaunchUrlView(CViewHandler *inHandler, const SRect& inBounds)
	: CView(inHandler, inBounds)
{
	mURL = nil;
	mComment = nil;
}
 
CLaunchUrlView::~CLaunchUrlView()
{
	if (mURL)
		UMemory::Dispose(mURL);
		
	if (mComment)
		UMemory::Dispose(mComment);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CLaunchUrlView::MouseUp(const SMouseMsgData& inInfo)
{
	if (inInfo.button == mouseButton_Left && mBounds.Contains(inInfo.loc))
		LaunchURL();
	
	CView::MouseUp(inInfo);
}

void CLaunchUrlView::MouseEnter(const SMouseMsgData& inInfo)
{
	SetMouseLaunch();

	CView::MouseEnter(inInfo);
}

void CLaunchUrlView::MouseMove(const SMouseMsgData& inInfo)
{
	SetMouseLaunch();

	CView::MouseMove(inInfo);
}

void CLaunchUrlView::MouseLeave(const SMouseMsgData& inInfo)
{
	SetMouseStandard();

	CView::MouseLeave(inInfo);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CLaunchUrlView::ChangeState(Uint16 inState)
{
	if (inState == viewState_Hidden || inState == viewState_Inactive)
		SetMouseStandard();
	
	return CView::ChangeState(inState);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CLaunchUrlView::SetURL(const Uint8 *inURL, const Uint8 *inComment)
{
	if (inURL != nil && inURL[0] != 0)
	{
		if (mURL == nil)
			mURL = UMemory::NewHandle(inURL[0]);
		else if (mURL->GetSize() != inURL[0])
			mURL->Reallocate(inURL[0]);
		
		mURL->Set(inURL+1, inURL[0]);
	}

	if (inComment != nil && inComment[0] != 0)
	{
		if (mComment == nil)
			mComment = UMemory::NewHandle(inComment[0]);
		else if (mComment->GetSize() != inComment[0])
			mComment->Reallocate(inComment[0]);
		
		mComment->Set(inComment+1, inComment[0]);
		SetTooltipMsg(inComment);
	}
	else
		SetTooltipMsg(inURL);	
}

Uint32 CLaunchUrlView::GetURL(void *outURL, Uint32 inMaxSize) 
{	
	if (mURL == nil || outURL == nil)
		return 0;

	return UMemory::Read(mURL, 0, outURL, inMaxSize);	
}

Uint32 CLaunchUrlView::GetComment(void *outComment, Uint32 inMaxSize) 
{	
	if (mComment == nil || outComment == nil)
		return 0;
	
	return UMemory::Read(mComment, 0, outComment, inMaxSize);	
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CLaunchUrlView::LaunchURL()
{
	if (mURL == nil)
		return false;

	Uint32 nSize= UMemory::GetSize(mURL);
	Uint8 *pURL	= UMemory::Lock(mURL);
	bool bRet = UTransport::LaunchURL(pURL, nSize);
	UMemory::Unlock(mURL);
	
	return bRet;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CLaunchUrlView::SetMouseLaunch()
{
	if (mURL != nil && UMouse::GetImage() != mouseImage_HandPoint)
		UMouse::SetImage(mouseImage_HandPoint);
}

void CLaunchUrlView::SetMouseStandard()
{
	if (mURL != nil && UMouse::GetImage() != mouseImage_Standard)
		UMouse::SetImage(mouseImage_Standard);
}

