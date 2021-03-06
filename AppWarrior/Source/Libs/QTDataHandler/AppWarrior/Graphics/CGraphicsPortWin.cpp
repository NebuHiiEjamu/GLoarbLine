/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CModuleWin.h"
#include "CGraphicsPortWin.h"
#include "CImageImp_W.h"
#include "PixelWin.h"
#include "UGraphicsWin.h"
#include <memory>
#include "CGraphicsScopeWin.h"
#include "StString.h"
#include "CFontWin.h"

HL_Begin_Namespace_BigRedH

#define NULL_CHECK(inValue, inTask) \
if (inValue == NULL) \
	THROW_GRAPHICS_ (inTask, eTranslate, ::GetLastError())

// ---------------------------------------------------------------------
//  CGraphicsPortWin                                            [public]
// ---------------------------------------------------------------------
// Constructor

CGraphicsPortWin::CGraphicsPortWin(HDC inDC)
: mHwnd(0), mFromPaint(false), mOwned(false), mOldHandleBMP(0)
, mBitmapBits(0)
{
	mPS.hdc = inDC;
}

CGraphicsPortWin::CGraphicsPortWin(HDC inDC, HBITMAP inBitmap, 
		BITMAPINFOHEADER* inInfo, UInt8* inBits)
: mHwnd(0), mFromPaint(false), mOwned(true), mOldHandleBMP(0)
	, mBitmapInfo(*inInfo), mBitmapBits(inBits)
{
	ASSERT(inBitmap != 0); 
	mOldHandleBMP = (HBITMAP)::SelectObject(inDC, inBitmap);
	NULL_CHECK(mOldHandleBMP, eGraphicsPortCreate);
		
	// Rest of PaintStruct is undefined; we're not mFromPaint
	mPS.hdc = inDC;	

	if (mBitmapInfo.biBitCount == 8) {
		// If we're 8 bit, force a grayscale palette
		InitGrayscale();
		NULL_CHECK(::SetDIBColorTable(inDC, 0, 256, sGrayScalePalette), 
				eGraphicsPortCreate);
	}
}

CGraphicsPortWin::CGraphicsPortWin(HWND inHwnd)
: mHwnd(inHwnd), mFromPaint(false), mOwned(true), mOldHandleBMP(0), mBitmapBits(0)
{
	// Rest of PaintStruct is undefined; we're not mFromPaint
	NULL_CHECK((mPS.hdc = ::GetDC(inHwnd)), eGraphicsPortCreate);
}

CGraphicsPortWin::CGraphicsPortWin(PAINTSTRUCT* inPS, HWND inHwnd)
: mPS(*inPS), mHwnd(inHwnd), mFromPaint(true), mOwned(false), mOldHandleBMP(0)
, mBitmapBits(0)
{	
}

// ---------------------------------------------------------------------
//  ~CGraphicsPort                                     [public][virtual]
// ---------------------------------------------------------------------
// Destructor

CGraphicsPortWin::~CGraphicsPortWin()
{
	if (mOldHandleBMP) {
		::SelectObject(mPS.hdc, mOldHandleBMP);
		mOldHandleBMP = 0;
	}
	if (mOwned) {
		if (mHwnd != 0) {
			if (mFromPaint)
				::EndPaint(mHwnd, &mPS);
			else
				::ReleaseDC(mHwnd, mPS.hdc);
		} else
			::DeleteDC(mPS.hdc);
	}
}


// ---------------------------------------------------------------------
//  IsOffscreen                                        [virtual][public]
// ---------------------------------------------------------------------
// 
// Is this graphics port drawing on an offscreen bitmap?

bool
CGraphicsPortWin::IsOffscreen() const
{
	return (mBitmapBits != 0);	
}


// ---------------------------------------------------------------------
//  GetPortBits		                                    	   [private]
// ---------------------------------------------------------------------
// Returns a pointer to the top-left corner of the image bitmap

unsigned char*
CGraphicsPortWin::GetPortBits()
{
	// Returns a pointer to the TOP of the bitmap
	ASSERT(mBitmapBits);
	long offset = -GetPortRowBytes() * (mBitmapInfo.biHeight - 1);
	return mBitmapBits + offset;
}

// ---------------------------------------------------------------------
//  GetPortBits		                                    	   [private]
// ---------------------------------------------------------------------
// Returns a pointer to the given pixel

unsigned char*
CGraphicsPortWin::GetPortBits(const CPoint& inPixelPos)
{
	ASSERT(GetPortBounds().ContainsPoint(inPixelPos));
	unsigned char* basePtr = GetPortBits();
	basePtr += inPixelPos.GetY() * GetPortRowBytes();
	basePtr += inPixelPos.GetX() * GetPortBytesPerPixel();
	return basePtr;
}

// ---------------------------------------------------------------------
//  GetPortRowBytes	                                    	   [private]
// ---------------------------------------------------------------------
// Returns the number of bytes from the top-left corner to one row below

long
CGraphicsPortWin::GetPortRowBytes() const
{
	long retVal = mBitmapInfo.biWidth * GetPortBytesPerPixel();
	// It's always quadbyte aligned
	retVal = ((retVal + 3) & ~3L);
	// This may seem silly, but windows bitmaps are backwards!
	return -retVal;
}

// ---------------------------------------------------------------------
//  GetPortBitsPerPixel                                    	   [private]
// ---------------------------------------------------------------------
// Note: Only works on ports allocated by CImages.

int
CGraphicsPortWin::GetPortBitsPerPixel() const
{
	return mBitmapInfo.biBitCount;
}

// ---------------------------------------------------------------------
//  GetPortBytesPerPixel                                   	   [private]
// ---------------------------------------------------------------------
// Little helper to compute the Bytes per pixel, for blends
// Note: Only works on ports allocated by CImages.

int
CGraphicsPortWin::GetPortBytesPerPixel() const
{
	return GetPortBitsPerPixel() / 8;
}


// ---------------------------------------------------------------------
//  SetClipRect                                                 [public]
// ---------------------------------------------------------------------
// Reset the current clipping region to the specified rect

void
CGraphicsPortWin::SetClipRect( const CRect &inRect )
{
	CRect clipRect = Transform(inRect);

	::SelectClipRgn(mPS.hdc, ::CreateRectRgn(clipRect.GetLeft(), clipRect.GetTop(), clipRect.GetRight(), clipRect.GetBottom()));
	//::IntersectClipRect(mPS.hdc, clipRect.GetLeft(), clipRect.GetTop(), clipRect.GetRight(), clipRect.GetBottom());
}

// ---------------------------------------------------------------------
//  GetClipRect                                                 [public]
// ---------------------------------------------------------------------
// Get the current clipping rect

CRect
CGraphicsPortWin::GetClipRect() const
{
	::RECT winRect;
	::GetClipBox(mPS.hdc, &winRect);
	CRect retVal;
	UGraphicsWin::RectToCRect(winRect, &retVal);
	return InvertTransform(retVal);
}


// ---------------------------------------------------------------------
//  GetPen                                                      [public]
// ---------------------------------------------------------------------
// Get the current pen

CPen
CGraphicsPortWin::GetPen() const
{	
	return CPen();
}


// ---------------------------------------------------------------------
//  SetPen                                                      [public]
// ---------------------------------------------------------------------
// Set the current pen

void
CGraphicsPortWin::SetPen( const CPen &inPen )
{
}


// ---------------------------------------------------------------------
//  GetBrush                                                    [public]
// ---------------------------------------------------------------------
// Get the current brush

CBrush
CGraphicsPortWin::GetBrush() const
{
	return CBrush();
}

// ---------------------------------------------------------------------
//  SetBrush                                                    [public]
// ---------------------------------------------------------------------
// Set the current brush

void
CGraphicsPortWin::SetBrush( const CBrush &inBrush )
{
}
/*
// ---------------------------------------------------------------------
//  GetFont                                                     [public]
// ---------------------------------------------------------------------
// Get the current font

CFont
CGraphicsPortWin::GetFont() const
{
	return CFont();
}

// ---------------------------------------------------------------------
//  SetFont                                                     [public]
// ---------------------------------------------------------------------
// Set the current font

void
CGraphicsPortWin::SetFont( const CFont &inFont )
{}
*/
CRect
CGraphicsPortWin::GetPortBounds() const
{
	return CRect(CPoint(0,0), CSize(mBitmapInfo.biWidth, mBitmapInfo.biHeight));
}

// ---------------------------------------------------------------------
//  DrawImage                                          [virtual][public]
// ---------------------------------------------------------------------
// Blasts an image to this graphics port

void
CGraphicsPortWin::DrawImage ( CImage& inImage
								  , const CRect& inSourceRect
								  , const CRect& inDestRect )
{	
	if (inImage.HasAlpha()) {
		// Clip the source and dest rects
		CRect clippedSourceRect = inSourceRect;
		CRect clippedDestRect = Transform(inDestRect);
		CImage* sourceImage = ClipImageBlast(inImage, clippedSourceRect
								, clippedDestRect, GetPortBounds());
		if (clippedSourceRect.IsEmpty() || clippedDestRect.IsEmpty()) return;
		
		std::auto_ptr<CImage> bufferScope;
		if (sourceImage != &inImage)
			bufferScope.reset(sourceImage);
		
		TProxy<CGraphicsPort> sourcePortPlatformIndependant 
				= sourceImage->GetGraphicsPort();
		CGraphicsPortWin* sourcePort = dynamic_cast<CGraphicsPortWin*>
				(sourcePortPlatformIndependant.GetPtr());
						
		UGraphicsWin::InlineBlendBitmap (
			sourcePort->GetPortBits(clippedSourceRect.GetTopLeft())
			,sourcePort->GetPortRowBytes()
			,GetPortBits(clippedDestRect.GetTopLeft())
			,GetPortRowBytes()
			,GetPortBits(clippedDestRect.GetTopLeft())
			,GetPortRowBytes()
			,static_cast<unsigned long>(clippedDestRect.GetWidth())
			,static_cast<unsigned long>(clippedDestRect.GetHeight())
			,sourcePort->GetPortBitsPerPixel()
			,GetPortBitsPerPixel()
		 	,GetPortBitsPerPixel());
	} else {
		TProxy<CGraphicsPort> sourcePortPlatformIndependant = inImage.GetGraphicsPort();
		CGraphicsPortWin* sourcePort = dynamic_cast<CGraphicsPortWin*>
				(sourcePortPlatformIndependant.GetPtr());

		CRect destRect = Transform(inDestRect);
		// Are we doing a resize?  If so, we must use StretchBlt
		if (inSourceRect.GetSize() != inDestRect.GetSize()) {
			::SetStretchBltMode(mPS.hdc, COLORONCOLOR);
			NULL_CHECK(	::StretchBlt( mPS.hdc
						, destRect.GetLeft(), destRect.GetTop()
						, destRect.GetWidth(), destRect.GetHeight()
						, sourcePort->mPS.hdc
						, inSourceRect.GetLeft(), inSourceRect.GetTop()
						, inSourceRect.GetWidth(), inSourceRect.GetHeight()
						, SRCCOPY ), eDrawImage);
		} else {
			// Non-resize case
			NULL_CHECK(	::BitBlt( mPS.hdc
						 , destRect.GetLeft(), destRect.GetTop()
						 , destRect.GetWidth(), destRect.GetHeight()
						 , sourcePort->mPS.hdc
						 , inSourceRect.GetLeft(), inSourceRect.GetTop()
						 , SRCCOPY), eDrawImage );
		}
	}
}

// ---------------------------------------------------------------------
//  AlphaMaskImage                                     [virtual][public]
// ---------------------------------------------------------------------
// Blasts an image to this graphics port, with the specified alpha mask.

void
CGraphicsPortWin::AlphaMaskImage ( CImage& inSrcImage
								  , const CRect& inSourceRect
								  , const CRect& inDestRect
								  , CImage* inAlphaMask )

{
	try {
		// If the alpha mask is null, never use alpha, just blast
		if (inAlphaMask == 0) {
			// Cut + Paste from DrawImage, but can't call DrawImage, to avoid
			// default alpha handling on images with alpha.
			TProxy<CGraphicsPort> sourcePortPlatformIndependant 
					= inSrcImage.GetGraphicsPort();
			CGraphicsPortWin* sourcePort = dynamic_cast<CGraphicsPortWin*>
					(sourcePortPlatformIndependant.GetPtr());
		
			CRect destRect = Transform(inDestRect);
			// Are we doing a resize?  If so, we must use StretchBlt
			if (inSourceRect.GetSize() != destRect.GetSize()) {
				::SetStretchBltMode(mPS.hdc, COLORONCOLOR);
				NULL_CHECK(	::StretchBlt( mPS.hdc
							, destRect.GetLeft(), destRect.GetTop()
							, destRect.GetWidth(), destRect.GetHeight()
							, sourcePort->mPS.hdc
							, inSourceRect.GetLeft(), inSourceRect.GetTop()
							, inSourceRect.GetWidth(), inSourceRect.GetHeight()
							, SRCCOPY ), eAlphaMaskImage);
			} else {
				// Non-resize case
				NULL_CHECK(	::BitBlt( mPS.hdc
							 , destRect.GetLeft(), destRect.GetTop()
							 , destRect.GetWidth(), destRect.GetHeight()
							 , sourcePort->mPS.hdc
							 , inSourceRect.GetLeft(), inSourceRect.GetTop()
							 , SRCCOPY), eAlphaMaskImage);
			}
		} else {
			// Clip the source and dest rects
			CRect clippedSourceRect = inSourceRect;
			CRect clippedDestRect = Transform(inDestRect);
			CRect clippedMaskRect = inSourceRect;
			
			CImage* bufferSourceImage = ClipImageBlast(inSrcImage, clippedSourceRect
											, clippedDestRect, GetPortBounds());
			if (clippedSourceRect.IsEmpty() || clippedDestRect.IsEmpty()) 
				return;

			std::auto_ptr<CImage> scopeSource;
			if (bufferSourceImage != &inSrcImage)
				scopeSource.reset(bufferSourceImage);
				
			// Reset the dest rect, so it works properly
			clippedDestRect = inDestRect;
			CImage* bufferMaskImage = ClipImageBlast(*inAlphaMask, clippedMaskRect
											, clippedDestRect, GetPortBounds());
			std::auto_ptr<CImage> scopeMask;
			if (bufferMaskImage != inAlphaMask)
				scopeMask.reset(bufferMaskImage);
				
			TProxy<CGraphicsPort> sourcePortPlatformIndependant 
					= bufferSourceImage->GetGraphicsPort();
			CGraphicsPortWin* sourcePort = dynamic_cast<CGraphicsPortWin*>
					(sourcePortPlatformIndependant.GetPtr());
			TProxy<CGraphicsPort> maskPortPlatformIndependant 
					= bufferMaskImage->GetGraphicsPort();
			CGraphicsPortWin* maskPort = dynamic_cast<CGraphicsPortWin*>
					(maskPortPlatformIndependant.GetPtr());

			// Assert that things aren't stupid
			ASSERT(mBitmapBits);
			ASSERT(sourcePort->mBitmapBits);
			ASSERT(maskPort->mBitmapBits);
			
			UGraphicsWin::BlendBitmap ( sourcePort->GetPortBits(clippedSourceRect.GetTopLeft())
										,sourcePort->GetPortRowBytes()
										,GetPortBits(clippedDestRect.GetTopLeft())
										,GetPortRowBytes()
										,maskPort->GetPortBits(clippedSourceRect.GetTopLeft())
										,maskPort->GetPortRowBytes()
										,GetPortBits(clippedDestRect.GetTopLeft())
										,GetPortRowBytes()
										,static_cast<unsigned long>(clippedDestRect.GetWidth())
										,static_cast<unsigned long>(clippedDestRect.GetHeight())
										,sourcePort->GetPortBitsPerPixel()
										,GetPortBitsPerPixel()
										,GetPortBitsPerPixel());
		}
	} catch (...) {
		RETHROW_GRAPHICS_(eAlphaMaskImage);
	}
}

// ---------------------------------------------------------------------
//  AlphaDrawImage                                     [virtual][public]
// ---------------------------------------------------------------------
// Blasts an image to this graphics port, blended at the specified alpha

void
CGraphicsPortWin::AlphaDrawImage ( CImage& inSrcImage
								  , const CRect& inSourceRect
								  , const CRect& inDestRect
								  , ColorChannel inAlpha )
{
	try {
		// Optimize out some degenerate cases...
		if (inAlpha == 0) return;
		if (inAlpha == 0xFF) {
			AlphaMaskImage(inSrcImage, inSourceRect, inDestRect, 0);
			return;
		}
	
		CRect clippedSourceRect = inSourceRect;
		CRect clippedDestRect = Transform(inDestRect);
		// Clip & resize
		CImage* bufferSourceImage = ClipImageBlast(inSrcImage, clippedSourceRect
										, clippedDestRect, GetPortBounds());
		if (clippedSourceRect.IsEmpty() || clippedDestRect.IsEmpty()) 
			return;

		std::auto_ptr<CImage> scopeSource;
		if (bufferSourceImage != &inSrcImage)
			scopeSource.reset(bufferSourceImage);
		TProxy<CGraphicsPort> sourcePlatformIndependant 
				= bufferSourceImage->GetGraphicsPort();
		CGraphicsPortWin* sourcePort = dynamic_cast<CGraphicsPortWin*>
				(sourcePlatformIndependant.GetPtr());
		
		UGraphicsWin::BlendConstantAlphaBitmap(	
				 sourcePort->GetPortBits(clippedSourceRect.GetTopLeft())
				 ,sourcePort->GetPortRowBytes()
				 ,GetPortBits(clippedDestRect.GetTopLeft())
				 ,GetPortRowBytes()
				 ,GetPortBits(clippedDestRect.GetTopLeft())
				 ,GetPortRowBytes()
				 ,static_cast<unsigned long>(clippedDestRect.GetWidth())
				 ,static_cast<unsigned long>(clippedDestRect.GetHeight())
				 ,inAlpha
				 ,sourcePort->GetPortBitsPerPixel()
				 ,GetPortBitsPerPixel()
				 ,GetPortBitsPerPixel());
	} catch (...) {
		RETHROW_GRAPHICS_(eAlphaDrawImage);
	}
}


// ---------------------------------------------------------------------
//  ColorKeyedDrawImage                                [virtual][public]
// ---------------------------------------------------------------------
// Blasts an image to this graphics port
// Ignores alpha; just doesn't copy pixels which are keyColor
void
CGraphicsPortWin::ColorKeyedDrawImage (CImage& inSrcImage
										, const CRect& inSourceRect
										, const CRect& inDestRect
										, const CColor& inKeyColor )
{
	try {
		CRect clippedSourceRect = inSourceRect;
		CRect clippedDestRect = Transform(inDestRect);
		// Clip & resize
		CImage* bufferSourceImage = ClipImageBlast(inSrcImage, clippedSourceRect
										, clippedDestRect, GetPortBounds());
		if (clippedSourceRect.IsEmpty() || clippedDestRect.IsEmpty()) 
			return;

		std::auto_ptr<CImage> scopeSource;
		if (bufferSourceImage != &inSrcImage)
			scopeSource.reset(bufferSourceImage);
		TProxy<CGraphicsPort> sourcePlatformIndependant 
				= bufferSourceImage->GetGraphicsPort();
		CGraphicsPortWin* sourcePort = dynamic_cast<CGraphicsPortWin*>
				(sourcePlatformIndependant.GetPtr());
		
		UGraphicsWin::ColorKeyBitmap(	
				 sourcePort->GetPortBits(clippedSourceRect.GetTopLeft())
				 ,sourcePort->GetPortRowBytes()
				 ,GetPortBits(clippedDestRect.GetTopLeft())
				 ,GetPortRowBytes()
				 ,GetPortBits(clippedDestRect.GetTopLeft())
				 ,GetPortRowBytes()
				 ,static_cast<unsigned long>(clippedDestRect.GetWidth())
				 ,static_cast<unsigned long>(clippedDestRect.GetHeight())
				 ,inKeyColor
				 ,sourcePort->GetPortBitsPerPixel()
				 ,GetPortBitsPerPixel()
				 ,GetPortBitsPerPixel());
	} catch (...) {
		RETHROW_GRAPHICS_(eAlphaDrawImage);
	}
}

// ---------------------------------------------------------------------
//  SolidFill                                                   [public]
// ---------------------------------------------------------------------
// Fill the graphics port with a constant color.
void
CGraphicsPortWin::SolidFill(const CColor& inColor) 
{
	::RECT bounds;
	bounds.right = ::GetDeviceCaps(mPS.hdc, HORZRES);
	bounds.bottom = ::GetDeviceCaps(mPS.hdc, VERTRES);
	bounds.left = -bounds.right;
	bounds.top = -bounds.bottom;
	
	COLORREF windowsColor;
	UGraphicsWin::CColorToCOLORREF(inColor, &windowsColor);

	::HBRUSH windowsBrush;
	NULL_CHECK((windowsBrush = ::CreateSolidBrush(windowsColor)), eSolidFill);	
	NULL_CHECK(::FillRect(mPS.hdc, &bounds, windowsBrush), eSolidFill);
	NULL_CHECK(::DeleteObject(windowsBrush), eSolidFill);
}

typedef 
CColor (*PixelCrackFN) (unsigned char*);

CImage*
CGraphicsPortWin::GetAlphaMask ()
{
	try {
		std::auto_ptr<CImage> retVal (new CImage(GetPortBounds().GetSize(), CImage::eByteAlphaMap));
		TProxy<CGraphicsPort> proxyMaskPort = retVal->GetGraphicsPort();
		CGraphicsPortWin* maskPort = dynamic_cast<CGraphicsPortWin*>
				(proxyMaskPort.GetPtr());
		
		PixelCrackFN pixelCracker = 0;
		switch (GetPortBitsPerPixel()) {
			case 16:
				pixelCracker = PixelFormat16Win_::CrackPixelAlpha;
				break;
			case 32:
				pixelCracker = PixelFormat32Win_::CrackPixelAlpha;
				break;
			default:
				ASSERT(0);
		}

		unsigned char* destRowPtr = maskPort->GetPortBits();
		unsigned char* sourceRowPtr = GetPortBits();
		long width = GetPortBounds().GetSize().GetWidth();
		long height = GetPortBounds().GetSize().GetHeight();
		for (long row=0;row< height;++row) {
			unsigned char* destPixelPtr = destRowPtr;
			unsigned char* sourcePixelPtr = sourceRowPtr;
			for (long pixel=0;pixel<width;++pixel) {
				*destPixelPtr = pixelCracker(sourcePixelPtr).GetAlpha();
				destPixelPtr += maskPort->GetPortBytesPerPixel();
				sourcePixelPtr += GetPortBytesPerPixel();
			}
			destRowPtr += maskPort->GetPortRowBytes();
			sourceRowPtr += GetPortRowBytes();
		}
		return retVal.release();
	} catch (...) {
		RETHROW_GRAPHICS_(eGetAlphaMask);
	}
		// dummy return to silence compiler
	return 0;
}


// ---------------------------------------------------------------------
//  GetPixel                                                    [public]
// ---------------------------------------------------------------------
// Returns the color value of a single pixel, converted to 8bpc
CColor
CGraphicsPortWin::GetPixel (const CPoint& inPixel) const 
{	ASSERT(mBitmapBits);
	CPoint pixelPos = Transform(inPixel);
	if (!GetPortBounds().ContainsPoint(pixelPos))
		return CColor::Black;
		
	CGraphicsPortWin* me = const_cast<CGraphicsPortWin*>(this);
	switch (me->GetPortBitsPerPixel()) {
		case 32:
			return PixelFormat32Win_::CrackPixelAlpha(me->GetPortBits(pixelPos));
		case 16:
			return PixelFormat16Win_::CrackPixelAlpha(me->GetPortBits(pixelPos));
		default:
			ASSERT(0);
	}
	// dummy return to silence compiler
	return CColor::Black;
}

// ---------------------------------------------------------------------
//  SetPixel                                                    [public]
// ---------------------------------------------------------------------
// Sets the color value of a single pixel, converting from 8bpc
void
CGraphicsPortWin::SetPixel (const CPoint& inPixel, const CColor& inColor) 
{	ASSERT(mBitmapBits);
	CPoint pixelPos = Transform(inPixel);
	if (!GetPortBounds().ContainsPoint(pixelPos))
		return;

	switch (GetPortBitsPerPixel()) {
		case 32:
			PixelFormat32Win_::MakePixelAlpha(GetPortBits(pixelPos), inColor);
			break;
		case 16:
			PixelFormat16Win_::MakePixelAlpha(GetPortBits(pixelPos), inColor);
			break;
		default:
			ASSERT(0);
	}
}


// ---------------------------------------------------------------------
//  DrawLine                                           [virtual][public]
// ---------------------------------------------------------------------
// Draws a line.
void
CGraphicsPortWin::DrawLine (const CPoint& inStart
							,const CPoint& inEnd
							,const CColor& inColor
							,int inThickness)
{
	try {
		ASSERT(inThickness >= 0);
		if (inThickness > 1) {
			// Compute polygon based on endpoints, thickness and normal
			CPoint deltaPoint = ComputeNormal(inStart, inEnd, inThickness);
			CPoint polyPoints[4];
			polyPoints[0] = inStart - deltaPoint;
			polyPoints[1] = inEnd - deltaPoint;
			polyPoints[2] = inEnd + deltaPoint;
			polyPoints[3] = inStart + deltaPoint;
			
			DrawPoints(4, polyPoints, inColor, CGraphicsPort::ePolygonFill);

			return;
		}
		
		CPoint endPointA = Transform(inStart);
		CPoint endPointB = Transform(inEnd);
		COLORREF winColor;
		UGraphicsWin::CColorToCOLORREF(inColor, &winColor);
		::HPEN thePen = ::CreatePen(PS_SOLID, 0, winColor);
		StGDIObjWin penScope(thePen);
		StGDISelectWin penSelectScope(mPS.hdc, thePen);
		::MoveToEx(mPS.hdc, endPointA.GetX(), endPointA.GetY(), NULL);
		::LineTo(mPS.hdc, endPointB.GetX(), endPointB.GetY());
		
		// Include the last pixel as well.
		SetPixel(inEnd, inColor);
	} catch (...) {
		RETHROW_GRAPHICS_(eDrawLine);
	}
}
									
// ---------------------------------------------------------------------
//  FillRect                                           [virtual][public]
// ---------------------------------------------------------------------
// Solid fills the given Rect.
void 		
CGraphicsPortWin::FillRect (const CRect& inRect
								, const CColor& inColor)
{
	::RECT winRect;
	UGraphicsWin::CRectToRect(Transform(inRect), &winRect);
	winRect.right;
	winRect.bottom;
	
	COLORREF windowsColor;
	UGraphicsWin::CColorToCOLORREF(inColor, &windowsColor);

	::HBRUSH windowsBrush;
	NULL_CHECK((windowsBrush = ::CreateSolidBrush(windowsColor)), eFillRect);	
	StGDIObjWin scopeBrush(windowsBrush);
	NULL_CHECK(::FillRect(mPS.hdc, &winRect, windowsBrush), eFillRect);
}
								
// ---------------------------------------------------------------------
//  DrawOval                                           [virtual][public]
// ---------------------------------------------------------------------
// Draws the outline of the oval that is bounded by the given Rect.
void		
CGraphicsPortWin::DrawOval (const CRect& inBounds
								, const CColor& inColor
								, int inThickness)
{
	CRect ovalRect = Transform(inBounds);
	
	COLORREF winColor;
	UGraphicsWin::CColorToCOLORREF(inColor, &winColor);

	::HPEN thePen;
	NULL_CHECK((thePen = ::CreatePen(PS_SOLID, inThickness, winColor))
			, eDrawOval);
			
	StGDIObjWin penScope(thePen);
	StGDISelectWin penSelectScope(mPS.hdc, thePen);

	NULL_CHECK((::Arc(mPS.hdc, ovalRect.GetLeft(), ovalRect.GetTop()
			,ovalRect.GetRight(), ovalRect.GetBottom()
			, 1, 1, 1, 1)), eDrawOval);
}
	
// ---------------------------------------------------------------------
//  FillOval                                           [virtual][public]
// ---------------------------------------------------------------------
// Solid fills the oval bounded by inBounds.
void 		
CGraphicsPortWin::FillOval (const CRect& inBounds
								, const CColor& inColor)
{
	CRect ovalRect = Transform(inBounds);
	
	COLORREF windowsColor;
	UGraphicsWin::CColorToCOLORREF(inColor, &windowsColor);
	::HPEN thePen;
	NULL_CHECK((thePen = ::CreatePen(PS_SOLID, 0, windowsColor)), eFillOval);
	StGDIObjWin penScope(thePen);
	StGDISelectWin penSelectScope(mPS.hdc, thePen);

	::HBRUSH windowsBrush;
	NULL_CHECK((windowsBrush = ::CreateSolidBrush(windowsColor)), eFillOval);	
	StGDIObjWin scopeBrush(windowsBrush);
	StGDISelectWin scopeSelectBrush(mPS.hdc, windowsBrush);
	NULL_CHECK(::Ellipse(mPS.hdc, ovalRect.GetLeft(), ovalRect.GetTop()
			,ovalRect.GetRight(), ovalRect.GetBottom()), eFillOval);
}

void
CGraphicsPortWin::FillPolygon (int inPointCount, const CPoint* inPoints, const CColor& inColor)
{
	// No more than 256 points in a polygon
	static POINT winPoints[256];
	for (int i=0;i<inPointCount;++i) {
		CPoint surfacePt( Transform(inPoints[i]) );
		winPoints[i].x = surfacePt.GetX();
		winPoints[i].y = surfacePt.GetY();
	}
	
	COLORREF windowsColor;
	UGraphicsWin::CColorToCOLORREF(inColor, &windowsColor);
	::HPEN thePen;
	NULL_CHECK((thePen = ::CreatePen(PS_SOLID, 0, windowsColor)), eFillPolygon);
	StGDIObjWin penScope(thePen);
	StGDISelectWin penSelectScope(mPS.hdc, thePen);

	::HBRUSH windowsBrush;
	NULL_CHECK((windowsBrush = ::CreateSolidBrush(windowsColor)), eFillPolygon);	
	StGDIObjWin scopeBrush(windowsBrush);
	StGDISelectWin scopeSelectBrush(mPS.hdc, windowsBrush);
	
	// For more complex winding values for self-intersecting polygons
	//::SetPolyFillMode();
	NULL_CHECK(::Polygon(mPS.hdc, winPoints, inPointCount), eFillPolygon);
}

bool CGraphicsPortWin::sIsGrayscaleInitialized = false;
::RGBQUAD CGraphicsPortWin::sGrayScalePalette [256];

// ---------------------------------------------------------------------
//  InitGrayscale                                      [static][private]
// ---------------------------------------------------------------------
// Initialize the grayscale palette, if need be
void
CGraphicsPortWin::InitGrayscale ()
{
	unsigned char* pBlast = reinterpret_cast<unsigned char*>(
			CGraphicsPortWin::sGrayScalePalette);
	// Black is highest index (like mac)
	for (int i=0;i<256;++i) {
		*pBlast++ = static_cast<unsigned char>(0xFF - i);
		*pBlast++ = static_cast<unsigned char>(0xFF - i);
		*pBlast++ = static_cast<unsigned char>(0xFF - i);
		*pBlast++ = 0;	// reserved
	}		
}

// ---------------------------------------------------------------------
//  DrawText	 									   [virtual][public]
// ---------------------------------------------------------------------
// draw some text into this GraphicsPort

void
CGraphicsPortWin::DrawText ( const CString& inText
							, const CFont& inFont
							, const CPoint& inWhere)
{
	StCStyleString osString(inText);
	StBkModeWin scopeMode(mPS.hdc, TRANSPARENT);
	CFontWin::StFontScoper fontScope(mPS.hdc, inFont);
	NULL_CHECK(::TextOut(mPS.hdc, inWhere.GetX(), inWhere.GetY()
			,osString
			,static_cast<int>(inText.length()))
		, eDrawText);
}

HL_End_Namespace_BigRedH
