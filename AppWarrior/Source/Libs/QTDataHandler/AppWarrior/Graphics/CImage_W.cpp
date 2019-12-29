/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CImage.h"
#include "CImageImp_W.h"
#include "CGraphicsPort.h"
#include "CGraphicsPortWin.h"

HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  Imp 														[public]
// ---------------------------------------------------------------------
// Constructor

CImage::Imp::Imp()
: mHandleBMP(NULL)
 ,mMemory(nil)
 ,mInfo(nil)
{
}

// ---------------------------------------------------------------------
//  ~Imp 														[public]
// ---------------------------------------------------------------------
// Destructor

CImage::Imp::~Imp()
{
	if (mInfo != nil)
	{
		delete mInfo;
		mInfo = nil;
	}	
	if (mHandleBMP != NULL)
	{
		if (0 == ::DeleteObject(mHandleBMP))
			UDebug::Message("Failed to delete bitmap\n");
		mHandleBMP = NULL;
		mMemory = nil;
	}	
}

// ---------------------------------------------------------------------
//  CreateDIBSection											[public]
// ---------------------------------------------------------------------
// A helper that allocates a backbuffer Win32's way

void 
CImage::Imp::CreateDIBSection(const CSize inSize, EImageDepth inBPP)
{
	mDepth = inBPP;
	mInfo = new BITMAPINFOHEADER;
	mInfo->biSize		    = sizeof(BITMAPINFOHEADER);
	mInfo->biClrImportant  	= 0;
	mInfo->biClrUsed		= 0;
	mInfo->biCompression	= BI_RGB; // or BI_BITFIELDS
	mInfo->biSizeImage	 	= 0; 	   // as long as BI_RGB
	mInfo->biPlanes		 	= 1;
	mInfo->biXPelsPerMeter 	= 0; 		// ignore ?
	mInfo->biYPelsPerMeter 	= 0;
	
	switch (inBPP) {
		case eMillionsAlpha:
		case eMillions:
			mInfo->biBitCount = 32;
			break;
		case eThousandsAlpha:
		case eThousands:
			mInfo->biBitCount = 16;
			break;
		case eByteAlphaMap:
			mInfo->biBitCount = 8;
			break;
	}
	mInfo->biHeight	 	 	= inSize.GetHeight();		
	mInfo->biWidth	 	 	= inSize.GetWidth();

	HDC tmpDC = ::GetDC(NULL);
	ASSERT(0 != tmpDC);
	if (inBPP == eByteAlphaMap) {
		mHandleBMP = ::CreateDIBSection( tmpDC
							, (BITMAPINFO*) mInfo
							, DIB_PAL_COLORS
							, (void**)&mMemory
							, NULL, 0);
	} else { 
		mHandleBMP = ::CreateDIBSection( tmpDC
							, (BITMAPINFO*) mInfo
							, DIB_RGB_COLORS
							, (void**)&mMemory
							, NULL, 0);
	}
	::DeleteDC(tmpDC);
	ASSERT(mHandleBMP != NULL);
	#if DEBUG	
		DIBSECTION dbSect; // now get the DIB section's information
		::GetObject(mHandleBMP,sizeof(dbSect),&dbSect);
		ASSERT(mMemory == (UInt8*) dbSect.dsBm.bmBits);
		ASSERT(mInfo->biBitCount == dbSect.dsBmih.biBitCount);
		ASSERT(mInfo->biHeight == dbSect.dsBmih.biHeight);
		ASSERT(mInfo->biWidth == dbSect.dsBmih.biWidth);
	#endif

}

CGraphicsPort*
CImage::Imp::CreateProxy ()
{
	return new CGraphicsPortWin (::CreateCompatibleDC(NULL), mHandleBMP, mInfo, mMemory);
}


/*
// ---------------------------------------------------------------------
//  CreateDIBSection											[public]
// ---------------------------------------------------------------------
// A helper that sets the alpha channel info for a specific region

void
CImage::Imp::SetAlpha(UInt8 inAlpha, const CRect inBounds)
{
	if (mInfo == nil || mMemory == nil || mInfo->biBitCount != 32)
	{
		ASSERT(0);
		return;
	}

	SInt32 l = inBounds.GetLeft();
	SInt32 r = inBounds.GetRight();
	if (l < 0) l=0;
	if (r > mInfo->biWidth) r = mInfo->biWidth;
	if (r < 0) r=0;
	if (l > r) l = r;

	SInt32 t = inBounds.GetTop();
	SInt32 b = inBounds.GetBottom();
	if (t < 0) t=0;
	if (b > mInfo->biHeight) b = mInfo->biHeight;
	if (b < 0) b=0;
	if (t > b) t = b;
	
	UInt8* ptr = 3+mMemory; // +3 to start with the alpha byte
	for(; t<b; t++)
	{
		for(; l<r; l++)	
		{
			*ptr = inAlpha;
			ptr += 4;
		}	
	}
}
*/

// ---------------------------------------------------------------------
//  CImage 												     [protected]
// ---------------------------------------------------------------------
// Constructor for deferred storage allocation; this is protected

CImage::CImage() : mImp(nil)
{}

// ---------------------------------------------------------------------
//  CImage 												        [public]
// ---------------------------------------------------------------------
// Constructor for deferred storage allocation; this is protected

CImage::CImage(const CSize inSize, EImageDepth inBPP)
	: mImp(nil)
{
	Create(inSize, inBPP);
}


// ---------------------------------------------------------------------
//  ~CImage											   [virtual][public]
// ---------------------------------------------------------------------
// Destructor

CImage::~CImage()
{ 
	if (mImp != nil) delete mImp;
	mImp = nil;
}

// ---------------------------------------------------------------------
//  Create 												     [protected]
// ---------------------------------------------------------------------
// Allocates backing store
void
CImage::Create (CSize inSize, EImageDepth inBPP) 
{
	mImp = new CImage::Imp();
	mImp->CreateDIBSection(inSize, inBPP);
}

// ---------------------------------------------------------------------
//  GetGraphicsPort									   [virtual][public]
// ---------------------------------------------------------------------
// Returns a graphics port to this offscreen image
TProxy<CGraphicsPort>
CImage::GetGraphicsPort ()
{
	return mImp->GetProxy();
}

// ---------------------------------------------------------------------
//  IsValid		 												[public]
// ---------------------------------------------------------------------
// is the image header already downloaded 
// can the image be used already

bool
CImage::IsValid() const
{
	return (mImp != nil) 
		&& (mImp->mInfo != nil)
		&& (mImp->mInfo->biWidth > 0);
}

// ---------------------------------------------------------------------
//  GetSize		 												[public]
// ---------------------------------------------------------------------
// returns the size of the image in pixels

CSize		
CImage::GetSize( ) const
{ 
	ASSERT(mImp != nil);
	BITMAPINFOHEADER* info = mImp->mInfo;
	ASSERT(info != nil);
	// make shure height is positive
	return CSize( info->biWidth
	            , info->biHeight >= 0 ? info->biHeight : -info->biHeight);
}


// ---------------------------------------------------------------------
//  GetColorDepth 												[public]
// ---------------------------------------------------------------------
// returns the number of bits pe pixel in the image

CImage::EImageDepth	
CImage::GetColorDepth() const
{ 
	ASSERT(mImp != nil);
	return mImp->mDepth;
}

HL_End_Namespace_BigRedH
