#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UUserInterface.h"

#include <QuickDraw.h>
#include <QDOffscreen.h>
#include <LowMem.h>

GrafPtr _ImageToGrafPtr(TImage inImage);

// don't use SetPort() because it could easily be a GWorld we're drawing into
#if TARGET_API_MAC_CARBON
#define SET_PORT(inImage)												\
	if (::GetQDGlobalsThePort() != _ImageToGrafPtr(inImage))			\
		::SetGWorld((CGrafPtr)_ImageToGrafPtr(inImage), nil);
#else
#define SET_PORT(inImage)												\
	if (qd.thePort != _ImageToGrafPtr(inImage))							\
		::SetGWorld((CGrafPtr)_ImageToGrafPtr(inImage), nil);
#endif

/* -------------------------------------------------------------------------- */

void UUserInterface_DrawStandardBox(TImage inImage, const SRect& inBounds, const SColor *inFillColor, bool inIsDisabled, bool inCanFocus, bool inIsFocus)
{
	Rect r;
	RGBColor rgb;
	
	SET_PORT(inImage);
	
	// grab a copy of the bounds
	r.top = inBounds.top;
	r.left = inBounds.left;
	r.bottom = inBounds.bottom;
	r.right = inBounds.right;

	if (inCanFocus && inIsFocus && !inIsDisabled)	// if has focus border
	{
		// set focus border color
		rgb.red = rgb.green = 0x6666;
		rgb.blue = 0xCCCC;
		::RGBForeColor(&rgb);

		// draw focus border
		::MoveTo(r.left, r.top+1);
		::LineTo(r.left, r.bottom-2);
		::MoveTo(r.left+1, r.top);
		::LineTo(r.right-2, r.top);
		::MoveTo(r.right-1, r.top+1);
		::LineTo(r.right-1, r.bottom-2);
		::MoveTo(r.left+1, r.bottom-1);
		::LineTo(r.right-2, r.bottom-1);
		r.top++;
		r.left++;
		r.bottom--;
		r.right--;
		::FrameRect(&r);
		
		// draw frame
		r.top++;
		r.left++;
		r.bottom--;
		r.right--;
		rgb.red = rgb.green = rgb.blue = 0;
		::RGBForeColor(&rgb);
		::FrameRect(&r);
	}
	else	// doesn't have focus border
	{
		// if can focus, make room for the focus border
		if (inCanFocus)
		{
			r.top++;
			r.left++;
			r.bottom--;
			r.right--;
		}
		
		// set degree of lightening/darkening
		rgb.red = rgb.green = rgb.blue = 0x3333;
		::RGBForeColor(&rgb);
		
		// set darken mode
		::PenMode(subPin);
		rgb.red = rgb.green = rgb.blue = 0;
		::OpColor(&rgb);
		
		// draw dark area
		::PenSize(1, 1);
		::MoveTo(r.left, r.top+1);
		::LineTo(r.left, r.bottom-2);
		::MoveTo(r.left, r.top);			// must have otherwise get double-dark corner
		::LineTo(r.right-2, r.top);
		
		// set lighten mode
		::PenMode(addPin);
		rgb.red = rgb.green = rgb.blue = 0xFFFF;
		::OpColor(&rgb);
		
		// draw light area
		::MoveTo(r.left+1, r.bottom-1);
		::LineTo(r.right-2, r.bottom-1);
		::MoveTo(r.right-1, r.top+1);		// must have otherwise get double-light corner
		::LineTo(r.right-1, r.bottom-1);

		// reset drawing mode
		::PenMode(patCopy);
		
		// draw frame
		r.top++;
		r.left++;
		r.bottom--;
		r.right--;
		rgb.red = rgb.green = rgb.blue = inIsDisabled ? 0x5555 : 0;
		::RGBForeColor(&rgb);
		::FrameRect(&r);
	}
	
	// fill content if applicable
	if (inFillColor)
	{
		r.top++;
		r.left++;
		r.bottom--;
		r.right--;
		::RGBForeColor((RGBColor *)inFillColor);
		::PaintRect(&r);
	}
}

void UUserInterface_DrawEtchedBox(TImage inImage, const SRect& inBounds, const void *inTitle, Uint32 inTitleSize)
{
	Rect r;
	RGBColor rgb;
	
	SET_PORT(inImage);

	r.top = inBounds.top;
	r.left = inBounds.left;
	r.bottom = inBounds.bottom;
	r.right = inBounds.right;

	::PenSize(1, 1);
	
	if (inTitle == nil || inTitleSize == 0 || inBounds.GetWidth() <= 54)
	{
		r.right--;
		r.bottom--;
		
		rgb.red = rgb.green = rgb.blue = 0x8888;
		::RGBForeColor(&rgb);
		::FrameRect(&r);
		
		r.right++;
		r.bottom++;
		rgb.red = rgb.green = rgb.blue = 0xFFFF;
		::RGBForeColor(&rgb);
		
		::MoveTo(r.left+1, r.top+1);
		::LineTo(r.right-3, r.top+1);
		::MoveTo(r.left+1, r.top+1);
		::LineTo(r.left+1, r.bottom-3);
		::MoveTo(r.left, r.bottom-1);
		::LineTo(r.right-1, r.bottom-1);
		::MoveTo(r.right-1, r.top);
		::LineTo(r.right-1, r.bottom-1);
	}
	else
	{
		Uint32 titleWidth, titleHeight, n;
		FontInfo fi;
		
		::GetFontInfo(&fi);
		titleHeight = fi.ascent;
		titleWidth = ::TextWidth((Ptr)inTitle, 0, inTitleSize);

		n = inBounds.GetWidth() - 32;
		if (titleWidth > n) titleWidth = n;

		r.top += (titleHeight / 2) - 1;
		rgb.red = rgb.green = rgb.blue = 0x8888;
		::RGBForeColor(&rgb);
		
		::MoveTo(r.left, r.top);
		::LineTo(r.left, r.bottom-2);
		::MoveTo(r.left, r.bottom-2);
		::LineTo(r.right-2, r.bottom-2);
		::MoveTo(r.right-2, r.top);
		::LineTo(r.right-2, r.bottom-2);
		::MoveTo(r.left, r.top);
		::LineTo(r.left + 12, r.top);
		::MoveTo(r.left + 12 + 4 + titleWidth + 4, r.top);
		::LineTo(r.right-2, r.top);

		rgb.red = rgb.green = rgb.blue = 0xFFFF;
		::RGBForeColor(&rgb);
		
		::MoveTo(r.left+1, r.top+1);
		::LineTo(r.left+1, r.bottom-3);
		::MoveTo(r.left, r.bottom-1);
		::LineTo(r.right-1, r.bottom-1);
		::MoveTo(r.right-1, r.top);
		::LineTo(r.right-1, r.bottom-1);
		::MoveTo(r.left+2, r.top+1);
		::LineTo(r.left + 12, r.top+1);
		::MoveTo(r.left + 12 + 4 + titleWidth + 4, r.top+1);
		::LineTo(r.right-3, r.top+1);
		
		r.top = inBounds.top;
		r.left += 12 + 4;
		r.right = r.left + titleWidth;
		r.bottom = r.top + titleHeight;
		rgb.red = rgb.green = rgb.blue = 0;
		::RGBForeColor(&rgb);
		
		Uint8 buf[256];
		if (inTitleSize > sizeof(buf)) inTitleSize = sizeof(buf);
		Int16 length = inTitleSize;
		
		::BlockMoveData(inTitle, buf, inTitleSize);
		
		if (::TruncText(titleWidth, (Ptr)buf, &length, smTruncEnd) != -1)
		{
			::MoveTo((r.left + r.right - titleWidth) / 2, (fi.ascent - fi.descent + r.bottom + r.top) / 2);
			::DrawText(buf, 0, length);
		}
	}
}

void UUserInterface_DrawSunkenBox(TImage inImage, const SRect& inBounds, const SColor *inColor)
{
	enum {
		changeVal	= 0x3333
	};
	
	RGBColor rgb;
	
	SET_PORT(inImage);
	
	::PenSize(1, 1);

	if (inColor)
	{
		Rect r;
				
		// grab a copy of the bounds
		r.top = inBounds.top;
		r.left = inBounds.left;
		r.bottom = inBounds.bottom;
		r.right = inBounds.right;
		
		// determine darker color
		rgb = *(RGBColor *)inColor;
		if (rgb.red < changeVal) rgb.red = 0; else rgb.red -= changeVal;
		if (rgb.green < changeVal) rgb.green = 0; else rgb.green -= changeVal;
		if (rgb.blue < changeVal) rgb.blue = 0; else rgb.blue -= changeVal;
		
		// draw dark
		::PenMode(patCopy);
		::RGBForeColor(&rgb);
		::MoveTo(r.right-2, r.top);
		::LineTo(r.left, r.top);
		::LineTo(r.left, r.bottom-2);
		
		// determine lighter color
		rgb = *(RGBColor *)inColor;
		if ((0xFFFF - rgb.red) < changeVal) rgb.red = 0xFFFF; else rgb.red += changeVal;
		if ((0xFFFF - rgb.green) < changeVal) rgb.green = 0xFFFF; else rgb.green += changeVal;
		if ((0xFFFF - rgb.blue) < changeVal) rgb.blue = 0xFFFF; else rgb.blue += changeVal;
		
		// draw light
		::RGBForeColor(&rgb);
		::MoveTo(r.left+1, r.bottom-1);
		::LineTo(r.right-1, r.bottom-1);
		::LineTo(r.right-1, r.top+1);
		
		// draw corners
		SetCPixel(r.left, r.bottom-1, (RGBColor *)inColor);
		SetCPixel(r.right-1, r.top, (RGBColor *)inColor);

		// fill inside
		r.top++;
		r.left++;
		r.bottom--;
		r.right--;
		::RGBForeColor((RGBColor *)inColor);
		::PaintRect(&r);
	}
	else
	{
		Int16 top, left, bottom, right;
		
		// extract the bounds
		top = inBounds.top;
		left = inBounds.left;
		bottom = inBounds.bottom;
		right = inBounds.right;

		// set degree of lightening/darkening
		rgb.red = rgb.green = rgb.blue = changeVal;
		::RGBForeColor(&rgb);
		
		// set darken mode
		::PenMode(subPin);
		rgb.red = rgb.green = rgb.blue = 0;
		::OpColor(&rgb);
		
		// draw dark area
		::MoveTo(left+1, top);
		::LineTo(right-2, top);
		::MoveTo(left, top);
		::LineTo(left, bottom-2);

		// set lighten mode
		::PenMode(addPin);
		rgb.red = rgb.green = rgb.blue = 0xFFFF;
		::OpColor(&rgb);

		// draw light area
		top++;
		left++;
		bottom--;
		right--;
		::MoveTo(right, top);
		::LineTo(right, bottom);
		::MoveTo(left, bottom);
		::LineTo(right-1, bottom);

		// reset drawing mode
		::PenMode(patCopy);
	}
}

void UUserInterface_DrawRaisedBox(TImage inImage, const SRect& inBounds, const SColor *inColor)
{
	enum {
		changeVal	= 0x3333
	};
	
	RGBColor rgb;
	
	SET_PORT(inImage);
	
	::PenSize(1, 1);

	if (inColor)
	{
		Rect r;
				
		// grab a copy of the bounds
		r.top = inBounds.top;
		r.left = inBounds.left;
		r.bottom = inBounds.bottom;
		r.right = inBounds.right;
		
		// determine lighter color
		rgb = *(RGBColor *)inColor;
		if ((0xFFFF - rgb.red) < changeVal) rgb.red = 0xFFFF; else rgb.red += changeVal;
		if ((0xFFFF - rgb.green) < changeVal) rgb.green = 0xFFFF; else rgb.green += changeVal;
		if ((0xFFFF - rgb.blue) < changeVal) rgb.blue = 0xFFFF; else rgb.blue += changeVal;

		// draw light
		::PenMode(patCopy);
		::RGBForeColor(&rgb);
		::MoveTo(r.right-2, r.top);
		::LineTo(r.left, r.top);
		::LineTo(r.left, r.bottom-2);
		
		// determine darker color
		rgb = *(RGBColor *)inColor;
		if (rgb.red < changeVal) rgb.red = 0; else rgb.red -= changeVal;
		if (rgb.green < changeVal) rgb.green = 0; else rgb.green -= changeVal;
		if (rgb.blue < changeVal) rgb.blue = 0; else rgb.blue -= changeVal;

		// draw dark
		::RGBForeColor(&rgb);
		::MoveTo(r.left+1, r.bottom-1);
		::LineTo(r.right-1, r.bottom-1);
		::LineTo(r.right-1, r.top+1);
		
		// draw corners
		SetCPixel(r.left, r.bottom-1, (RGBColor *)inColor);
		SetCPixel(r.right-1, r.top, (RGBColor *)inColor);

		// fill inside
		r.top++;
		r.left++;
		r.bottom--;
		r.right--;
		::RGBForeColor((RGBColor *)inColor);
		::PaintRect(&r);
	}
	else
	{
		Int16 top, left, bottom, right;
		
		// extract the bounds
		top = inBounds.top;
		left = inBounds.left;
		bottom = inBounds.bottom;
		right = inBounds.right;

		// set degree of lightening/darkening
		rgb.red = rgb.green = rgb.blue = changeVal;
		::RGBForeColor(&rgb);
		
		// set lighten mode
		::PenMode(addPin);
		rgb.red = rgb.green = rgb.blue = 0xFFFF;
		::OpColor(&rgb);
		
		// draw light area
		::MoveTo(left+1, top);
		::LineTo(right-2, top);
		::MoveTo(left, top);
		::LineTo(left, bottom-2);

		// set darken mode
		::PenMode(subPin);
		rgb.red = rgb.green = rgb.blue = 0;
		::OpColor(&rgb);

		// draw dark area
		top++;
		left++;
		bottom--;
		right--;
		::MoveTo(right, top);
		::LineTo(right, bottom);
		::MoveTo(left, bottom);
		::LineTo(right-1, bottom);

		// reset drawing mode
		::PenMode(patCopy);
	}
}

void UUserInterface_DrawBarBox(TImage inImage, const SRect& inBounds)
{
	enum {
		th = 2 + 2
	};
	Int16 top, left, bottom, right;
	RGBColor rgb;
	
	SET_PORT(inImage);
	
	top = inBounds.top;
	left = inBounds.left;
	bottom = inBounds.bottom;
	right = inBounds.right;

	::PenSize(1, 1);
	
	// set degree of lightening/darkening
	rgb.red = rgb.green = rgb.blue = 0x3333;
	::RGBForeColor(&rgb);

	// set lighten mode
	::PenMode(addPin);
	rgb.red = rgb.green = rgb.blue = 0xFFFF;
	::OpColor(&rgb);

	// draw light lines
	::MoveTo(left, top);
	::LineTo(right-1, top);
	::MoveTo(left, top);
	::LineTo(left, bottom-1);
	::MoveTo(left+th, bottom-th);
	::LineTo(right-th-1, bottom-th);
	::MoveTo(right-th, top+th);
	::LineTo(right-th, bottom-th);
	
	// set darken mode
	::PenMode(subPin);
	rgb.red = rgb.green = rgb.blue = 0;
	::OpColor(&rgb);
	
	// draw dark lines
	::MoveTo(left+th-1, top+th-1);
	::LineTo(right-th, top+th-1);
	::MoveTo(left+th-1, top+th);
	::LineTo(left+th-1, bottom-th);
	::MoveTo(left+1, bottom-1);
	::LineTo(right-2, bottom-1);
	::MoveTo(right-1, top+1);
	::LineTo(right-1, bottom-1);

	// reset drawing mode
	::PenMode(patCopy);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UUserInterface::GetHighlightColor(SColor *outHilite, SColor *outInverse)
{
	enum {
		threshold	= 0x3333 * 3		// anything darker makes white inverse
	};
	
	RGBColor rgb;
	
	LMGetHiliteRGB(&rgb);
	
	if (outInverse)
	{
		if (((Uint32)rgb.red + (Uint32)rgb.green + (Uint32)rgb.blue) < threshold)
			outInverse->Set(0xFFFF);
		else
			outInverse->Set(0);
	}
	
	if (outHilite) *outHilite = *(SColor *)&rgb;
}

// returns false if unknown color (outColor is set to black)
bool UUserInterface::GetSysColor(Uint32 inNum, SColor& outColor)
{
	switch (inNum)
	{
		case sysColor_Background:
			outColor.Set(0xDDDD);
			break;
		case sysColor_Content:
			outColor.Set(0xFFFF);
			break;
		case sysColor_Light:
			outColor.Set(0xFFFF);
			break;
		case sysColor_Dark:
			outColor.Set(0xAAAA);
			break;
		case sysColor_Frame:
			outColor.Set(0x0000);
			break;
		case sysColor_Label:
			outColor.Set(0x0000);
			break;
		case sysColor_ButtonLabel:
			outColor.Set(0x0000);
			break;
		case sysColor_Highlight:
			LMGetHiliteRGB((RGBColor *)&outColor);
			break;
		case sysColor_InverseHighlight:
			GetHighlightColor(nil, &outColor);
			break;
		default:
			outColor.Set(0);
			return false;
	}
	
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

#if 0

enum {
	kMyZoomNoAcceleration = 0, kMyZoomAccelerate = 1, kMyZoomDecelerate = 2
};

static void _CalcZoomRect(Rect *theRect, Rect *smallRect, Rect *bigRect, fast_float stepValue)
{
	theRect->left = smallRect->left + (Int16)((fast_float)(bigRect->left-smallRect->left)*stepValue);
	theRect->top = smallRect->top + (Int16)((fast_float)(bigRect->top-smallRect->top)*stepValue);
	theRect->right = smallRect->right + (Int16)((fast_float)(bigRect->right-smallRect->right)*stepValue);
	theRect->bottom = smallRect->bottom + (Int16)((fast_float)(bigRect->bottom-smallRect->bottom)*stepValue);
}

static void _ZoomEffect(Rect smallRect, Rect bigRect, bool zoomLarger)
{
	#define	kNumSteps		14
	#define	kRectsVisible	4
	#define	kZoomRatio		.7
	#define	kDelayTicks		1

	fast_float firstStep, stepValue, trailer, zoomRatio;
	Int16 i, step;
	Rect curRect;
	Uint32 ticks;
	TImage desktop = UGraphics::GetDesktopImage();
	
	UGraphics::SetPattern(desktop, pattern_Gray);
	UGraphics::SetInkMode(desktop, mode_Xor);
	
	firstStep=kZoomRatio;
	for (i=0; i<kNumSteps; i++)
		firstStep *= kZoomRatio;

	if (zoomLarger)
		zoomRatio = kZoomRatio;
	else
	{
		curRect = smallRect;
		smallRect = bigRect;
		bigRect = curRect;
	
		zoomRatio = 1.0/kZoomRatio;
		firstStep = 1.0-firstStep;
	}
		
	trailer = firstStep;
	stepValue = firstStep;
	for (step=0; step < (kNumSteps+kRectsVisible); step++)
	{
		// draw new frame
		if (step<kNumSteps)
		{
			stepValue /= zoomRatio;
			_CalcZoomRect(&curRect,&smallRect,&bigRect,stepValue);
			FrameRect(&curRect);
		}
		
		// erase old frame
		if (step>=kRectsVisible)
		{
			trailer /= zoomRatio;
			_CalcZoomRect(&curRect,&smallRect,&bigRect,trailer);
			FrameRect(&curRect);
		}

		Delay(kDelayTicks, (Uint32 *)&ticks);
	}
}
#endif


#endif /* MACINTOSH */
