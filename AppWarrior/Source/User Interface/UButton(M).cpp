#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UButton.h"
#include <QuickDraw.h>

/*
 * Function Prototypes
 */

static void _DrawButton(Rect r, void *title, Uint32 titleLen);
static void _DrawButtonHilited(Rect r, void *title, Uint32 titleLen);
static void _DrawButtonDisabled(Rect r, void *title, Uint32 titleLen);
static void _DrawDefButton(Rect r, void *title, Uint32 titleLen);
static void _DrawDefButtonHilited(Rect r, void *title, Uint32 titleLen);
static void _DrawDefButtonDisabled(Rect r, void *title, Uint32 titleLen);
static void _DrawButtonBW(Rect r, void *title, Uint32 titleLen);
static void _DrawButtonDisabledBW(Rect r, void *title, Uint32 titleLen);
static void _DrawButtonHilitedBW(Rect r, void *title, Uint32 titleLen);
static void _DrawDefButtonBW(Rect r, void *title, Uint32 titleLen);
static void _DrawDefButtonDisabledBW(Rect r, void *title, Uint32 titleLen);
static void _DrawDefButtonHilitedBW(Rect r, void *title, Uint32 titleLen);
static void _CalcRgnButton(RgnHandle rgn, Rect r);
static void _CalcRgnDefButton(RgnHandle rgn, Rect r);

GrafPtr _ImageToGrafPtr(TImage inImage);

/* -------------------------------------------------------------------------- */

void UButton_Draw(TImage inImage, const SRect& inBounds, const SButtonInfo& inInfo)
{
	Rect bounds = { inBounds.top, inBounds.left, inBounds.bottom, inBounds.right };
	
	::SetPort(_ImageToGrafPtr(inImage));
	
	if (inInfo.options & 1)				// if default
	{
		if (inInfo.options & 0x8000)	// if b/w
			_DrawDefButtonBW(bounds, inInfo.title, inInfo.titleSize);
		else
			_DrawDefButton(bounds, inInfo.title, inInfo.titleSize);
	}
	else
	{
		if (inInfo.options & 0x8000)	// if b/w
			_DrawButtonBW(bounds, inInfo.title, inInfo.titleSize);
		else
			_DrawButton(bounds, inInfo.title, inInfo.titleSize);
	}
}

void UButton_DrawFocused(TImage inImage, const SRect& inBounds, const SButtonInfo& inInfo)
{
	UButton_Draw(inImage, inBounds, inInfo);
}

void UButton_DrawHilited(TImage inImage, const SRect& inBounds, const SButtonInfo& inInfo)
{
	Rect bounds = { inBounds.top, inBounds.left, inBounds.bottom, inBounds.right };
	
	::SetPort(_ImageToGrafPtr(inImage));
	
	if (inInfo.options & 1)	// if default
	{
		if (inInfo.options & 0x8000)	// if b/w
			_DrawDefButtonHilitedBW(bounds, inInfo.title, inInfo.titleSize);
		else
			_DrawDefButtonHilited(bounds, inInfo.title, inInfo.titleSize);
	}
	else
	{
		if (inInfo.options & 0x8000)	// if b/w
			_DrawButtonHilitedBW(bounds, inInfo.title, inInfo.titleSize);
		else
			_DrawButtonHilited(bounds, inInfo.title, inInfo.titleSize);
	}
}

void UButton_DrawDisabled(TImage inImage, const SRect& inBounds, const SButtonInfo& inInfo)
{
	Rect bounds = { inBounds.top, inBounds.left, inBounds.bottom, inBounds.right };
	
	::SetPort(_ImageToGrafPtr(inImage));
	
	if (inInfo.options & 1)	// if default
	{
		if (inInfo.options & 0x8000)	// if b/w
			_DrawDefButtonDisabledBW(bounds, inInfo.title, inInfo.titleSize);
		else
			_DrawDefButtonDisabled(bounds, inInfo.title, inInfo.titleSize);
	}
	else
	{
		if (inInfo.options & 0x8000)	// if b/w
			_DrawButtonDisabledBW(bounds, inInfo.title, inInfo.titleSize);
		else
			_DrawButtonDisabled(bounds, inInfo.title, inInfo.titleSize);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static void _CalcRgnButton(RgnHandle rgn, Rect r)
{
	if (!rgn) return;

	OpenRgn();
	FrameRoundRect(&r, 8, 8);
	CloseRgn(rgn);
}

static void _CalcRgnDefButton(RgnHandle rgn, Rect r)
{
	if (!rgn) return;

	OpenRgn();

	// frame straights
	MoveTo(r.left+3, r.top);
	LineTo(r.right-4, r.top);
	MoveTo(r.left, r.top+3);
	LineTo(r.left, r.bottom-4);
	MoveTo(r.left+3, r.bottom-1);
	LineTo(r.right-4, r.bottom-1);
	MoveTo(r.right-1, r.top+3);
	LineTo(r.right-1, r.bottom-4);

	// frame corners
	MoveTo(r.left+3, r.top);
	LineTo(r.left, r.top+3);
	MoveTo(r.left+3, r.bottom-1);
	LineTo(r.left, r.bottom-4);
	MoveTo(r.right-4, r.top);
	LineTo(r.right-1, r.top+3);
	MoveTo(r.right-4, r.bottom-1);
	LineTo(r.right-1, r.bottom-4);

	CloseRgn(rgn);
}

void _DrawCenteredText(const void *inText, Uint32 inLength, Rect& r)
{
	if (inText && inLength > 0)
	{
		FontInfo fi;
		Int16 h, v;
	
		GetFontInfo(&fi);
		h = r.left + ((r.right - r.left - TextWidth(inText, 0, inLength)) / 2);
		v = (((r.top + r.bottom) - (fi.descent + fi.ascent)) / 2) + fi.ascent;
		MoveTo(h, v);
		DrawText((Ptr)inText, 0, inLength);
	}
}

/* -------------------------------------------------------------------------- */

// width 80, height 20
static void _DrawButton(Rect r, void *title, Uint32 titleLen)
{
	Rect tr;
	
	// frame and body
	PenNormal();
	tr = r;
	InsetRect(&tr, 3, 3);
	RGBForeColor((RGBColor *)&color_GrayD);
	PaintRoundRect(&tr, 4, 4);
	RGBForeColor((RGBColor *)&color_Black);
	FrameRoundRect(&r, 8, 8);
	
	// top shading
	RGBForeColor((RGBColor *)&color_GrayD);
	MoveTo(r.left+3, r.top+1);
	LineTo(r.right-4, r.top+1);
	RGBForeColor((RGBColor *)&color_White);
	MoveTo(r.left+2, r.top+2);
	LineTo(r.right-4, r.top+2);
	
	// left shading
	RGBForeColor((RGBColor *)&color_GrayD);
	MoveTo(r.left+1, r.top+3);
	LineTo(r.left+1, r.bottom-4);
	RGBForeColor((RGBColor *)&color_White);
	MoveTo(r.left+2, r.top+2);
	LineTo(r.left+2, r.bottom-4);
	
	// bottom shading
	RGBForeColor((RGBColor *)&color_Gray7);
	MoveTo(r.left+3, r.bottom-2);
	LineTo(r.right-3, r.bottom-2);
	RGBForeColor((RGBColor *)&color_GrayA);
	MoveTo(r.left+3, r.bottom-3);
	LineTo(r.right-4, r.bottom-3);
	
	// right shading
	RGBForeColor((RGBColor *)&color_Gray7);
	MoveTo(r.right-2, r.top+3);
	LineTo(r.right-2, r.bottom-3);
	RGBForeColor((RGBColor *)&color_GrayA);
	MoveTo(r.right-3, r.top+3);
	LineTo(r.right-3, r.bottom-4);
	
	// top left corner
	SetCPixel(r.left+2, r.top, (RGBColor *)&color_Gray2);
	SetCPixel(r.left, r.top+2, (RGBColor *)&color_Gray2);
	SetCPixel(r.left+2, r.top+1, (RGBColor *)&color_GrayB);
	SetCPixel(r.left+1, r.top+2, (RGBColor *)&color_GrayB);
	SetCPixel(r.left+3, r.top+3, (RGBColor *)&color_White);
	
	// bottom right corner
	SetCPixel(r.right-3, r.bottom-1, (RGBColor *)&color_Gray2);
	SetCPixel(r.right-1, r.bottom-3, (RGBColor *)&color_Gray2);
	SetCPixel(r.right-3, r.bottom-3, (RGBColor *)&color_Gray7);
	SetCPixel(r.right-4, r.bottom-4, (RGBColor *)&color_GrayA);
	
	// top right corner
	SetCPixel(r.right-3, r.top, (RGBColor *)&color_Gray2);
	SetCPixel(r.right-1, r.top+2, (RGBColor *)&color_Gray2);
	SetCPixel(r.right-3, r.top+1, (RGBColor *)&color_GrayB);
	SetCPixel(r.right-2, r.top+2, (RGBColor *)&color_GrayB);
	SetCPixel(r.right-3, r.top+2, (RGBColor *)&color_GrayD);
	SetCPixel(r.right-4, r.top+3, (RGBColor *)&color_GrayD);
	
	// bottom left corner
	SetCPixel(r.left+2, r.bottom-1, (RGBColor *)&color_Gray2);
	SetCPixel(r.left, r.bottom-3, (RGBColor *)&color_Gray2);
	SetCPixel(r.left+2, r.bottom-2, (RGBColor *)&color_GrayB);
	SetCPixel(r.left+1, r.bottom-3, (RGBColor *)&color_GrayB);
	SetCPixel(r.left+2, r.bottom-3, (RGBColor *)&color_GrayD);
	SetCPixel(r.left+3, r.bottom-4, (RGBColor *)&color_GrayD);
		
	// draw title
	if (titleLen > 0)
	{
		tr = r;
		tr.left++;		tr.right++;
		tr.bottom++;	tr.top++;
		TextMode(srcOr);
		RGBForeColor((RGBColor *)&color_White);
		_DrawCenteredText(title, titleLen, tr);
		RGBForeColor((RGBColor *)&color_Black);
		_DrawCenteredText(title, titleLen, r);
	}
}

static void _DrawDefault(Rect r)
{	
	// frame straights
	RGBForeColor((RGBColor *)&color_Black);
	MoveTo(r.left+3, r.top);
	LineTo(r.right-4, r.top);
	MoveTo(r.left, r.top+3);
	LineTo(r.left, r.bottom-4);
	MoveTo(r.left+3, r.bottom-1);
	LineTo(r.right-4, r.bottom-1);
	MoveTo(r.right-1, r.top+3);
	LineTo(r.right-1, r.bottom-4);

	// frame corners
	MoveTo(r.left+3, r.top);
	LineTo(r.left, r.top+3);
	MoveTo(r.left+3, r.bottom-1);
	LineTo(r.left, r.bottom-4);
	MoveTo(r.right-4, r.top);
	LineTo(r.right-1, r.top+3);
	MoveTo(r.right-4, r.bottom-1);
	LineTo(r.right-1, r.bottom-4);
	
	// top shading
	RGBForeColor((RGBColor *)&color_GrayD);
	MoveTo(r.left+3, r.top+1);
	LineTo(r.right-5, r.top+1);
	RGBForeColor((RGBColor *)&color_GrayA);
	MoveTo(r.left+4, r.top+2);
	LineTo(r.right-3, r.top+2);
	
	// bottom shading
	RGBForeColor((RGBColor *)&color_Gray7);
	MoveTo(r.left+3, r.bottom-2);
	LineTo(r.right-4, r.bottom-2);
	RGBForeColor((RGBColor *)&color_GrayA);
	MoveTo(r.left+2, r.bottom-3);
	LineTo(r.right-5, r.bottom-3);
	
	// left shading
	RGBForeColor((RGBColor *)&color_GrayD);
	MoveTo(r.left+1, r.top+3);
	LineTo(r.left+1, r.bottom-5);
	RGBForeColor((RGBColor *)&color_GrayA);
	MoveTo(r.left+2, r.top+4);
	LineTo(r.left+2, r.bottom-3);

	// right shading
	RGBForeColor((RGBColor *)&color_Gray7);
	MoveTo(r.right-2, r.top+3);
	LineTo(r.right-2, r.bottom-4);
	RGBForeColor((RGBColor *)&color_GrayA);
	MoveTo(r.right-3, r.top+2);
	LineTo(r.right-3, r.bottom-5);

	// top left corner
	SetCPixel(r.left+3, r.top, (RGBColor *)&color_Gray2);
	SetCPixel(r.left, r.top+3, (RGBColor *)&color_Gray2);
	SetCPixel(r.left+2, r.top+2, (RGBColor *)&color_GrayD);
	SetCPixel(r.left+2, r.top+3, (RGBColor *)&color_GrayD);
	SetCPixel(r.left+3, r.top+2, (RGBColor *)&color_GrayD);
	SetCPixel(r.left+3, r.top+3, (RGBColor *)&color_GrayA);
	SetCPixel(r.left+3, r.top+4, (RGBColor *)&color_Gray7);
	SetCPixel(r.left+4, r.top+3, (RGBColor *)&color_Gray7);
	
	// bottom left corner
	SetCPixel(r.left+3, r.bottom-1, (RGBColor *)&color_Gray2);
	SetCPixel(r.left, r.bottom-4, (RGBColor *)&color_Gray2);
	SetCPixel(r.left+1, r.bottom-4, (RGBColor *)&color_GrayA);
	SetCPixel(r.left+3, r.bottom-4, (RGBColor *)&color_GrayA);
	SetCPixel(r.left+4, r.bottom-4, (RGBColor *)&color_Gray7);
	SetCPixel(r.left+3, r.bottom-5, (RGBColor *)&color_Gray7);
	
	// top right corner
	SetCPixel(r.right-4, r.top, (RGBColor *)&color_Gray2);
	SetCPixel(r.right-1, r.top+3, (RGBColor *)&color_Gray2);
	SetCPixel(r.right-4, r.top+1, (RGBColor *)&color_GrayA);
	SetCPixel(r.right-4, r.top+3, (RGBColor *)&color_GrayA);
	SetCPixel(r.right-4, r.top+4, (RGBColor *)&color_Gray7);
	SetCPixel(r.right-5, r.top+3, (RGBColor *)&color_Gray7);
	
	// bottom right corner
	SetCPixel(r.right-4, r.bottom-1, (RGBColor *)&color_Gray2);
	SetCPixel(r.right-1, r.bottom-4, (RGBColor *)&color_Gray2);
	SetCPixel(r.right-3, r.bottom-3, (RGBColor *)&color_Gray7);
	SetCPixel(r.right-3, r.bottom-4, (RGBColor *)&color_Gray7);
	SetCPixel(r.right-4, r.bottom-3, (RGBColor *)&color_Gray7);
	SetCPixel(r.right-4, r.bottom-4, (RGBColor *)&color_GrayA);
	SetCPixel(r.right-4, r.bottom-5, (RGBColor *)&color_Gray7);
	SetCPixel(r.right-5, r.bottom-4, (RGBColor *)&color_Gray7);
}

// width 80, height 26
static void _DrawDefButton(Rect r, void *title, Uint32 titleLen)
{
	Rect tr = r;
	InsetRect(&tr, 3, 3);
	_DrawButton(tr, title, titleLen);
	_DrawDefault(r);
}

static void _DrawButtonHilited(Rect r, void *title, Uint32 titleLen)
{
	Rect tr;
	
	// frame
	PenNormal();
	RGBForeColor((RGBColor *)&color_Black);
	FrameRoundRect(&r, 8, 8);
	
	// body
	tr = r;
	InsetRect(&tr, 3, 3);
	RGBForeColor((RGBColor *)&color_Gray5);
	PaintRect(&tr);
	
	// top shading
	RGBForeColor((RGBColor *)&color_Gray2);
	MoveTo(r.left+2, r.top+1);
	LineTo(r.right-3, r.top+1);
	RGBForeColor((RGBColor *)&color_Gray4);
	MoveTo(r.left+2, r.top+2);
	LineTo(r.right-4, r.top+2);
	SetCPixel(r.right-3, r.top+2, (RGBColor *)&color_Gray5);
	SetCPixel(r.right-2, r.top+2, (RGBColor *)&color_Gray7);
	
	// left shading
	RGBForeColor((RGBColor *)&color_Gray2);
	MoveTo(r.left+1, r.top+2);
	LineTo(r.left+1, r.bottom-3);
	RGBForeColor((RGBColor *)&color_Gray4);
	MoveTo(r.left+2, r.top+2);
	LineTo(r.left+2, r.bottom-4);
	SetCPixel(r.left+2, r.top+2, (RGBColor *)&color_Gray2);
	SetCPixel(r.left+3, r.top+3, (RGBColor *)&color_Gray4);
	
	// bottom shading
	RGBForeColor((RGBColor *)&color_Gray7);
	MoveTo(r.left+2, r.bottom-2);
	LineTo(r.right-3, r.bottom-2);
	RGBForeColor((RGBColor *)&color_Gray6);
	MoveTo(r.left+3, r.bottom-3);
	LineTo(r.right-4, r.bottom-3);
	SetCPixel(r.left+2, r.bottom-3, (RGBColor *)&color_Gray5);

	// right shading
	RGBForeColor((RGBColor *)&color_Gray7);
	MoveTo(r.right-2, r.top+3);
	LineTo(r.right-2, r.bottom-3);
	RGBForeColor((RGBColor *)&color_Gray6);
	MoveTo(r.right-3, r.top+3);
	LineTo(r.right-3, r.bottom-4);
	SetCPixel(r.right-3, r.bottom-3, (RGBColor *)&color_Gray7);
	SetCPixel(r.right-4, r.bottom-4, (RGBColor *)&color_Gray6);
	
	// title
	if (titleLen > 0)
	{
		RGBForeColor((RGBColor *)&color_White);
		TextMode(srcOr);
		_DrawCenteredText(title, titleLen, r);
	}
}

static void _DrawDefButtonHilited(Rect r, void *title, Uint32 titleLen)
{
	Rect tr = r;
	InsetRect(&tr, 3, 3);
	_DrawButtonHilited(tr, title, titleLen);
	_DrawDefault(r);
}

static void _DrawButtonDisabled(Rect r, void *title, Uint32 titleLen)
{
	Rect tr;
	
	// frame
	PenNormal();
	RGBForeColor((RGBColor *)&color_Gray8);
	FrameRoundRect(&r, 8, 8);
	
	// body
	tr = r;
	InsetRect(&tr, 3, 3);
	RGBForeColor((RGBColor *)&color_GrayD);
	PaintRect(&tr);
	
	// top shading
	RGBForeColor((RGBColor *)&color_GrayD);
	MoveTo(r.left+2, r.top+1);
	LineTo(r.right-3, r.top+1);
	RGBForeColor((RGBColor *)&color_White);
	MoveTo(r.left+2, r.top+2);
	LineTo(r.right-4, r.top+2);
	SetCPixel(r.right-3, r.top+2, (RGBColor *)&color_GrayD);
	SetCPixel(r.right-2, r.top+2, (RGBColor *)&color_GrayB);
	
	// left shading
	RGBForeColor((RGBColor *)&color_GrayD);
	MoveTo(r.left+1, r.top+2);
	LineTo(r.left+1, r.bottom-3);
	RGBForeColor((RGBColor *)&color_White);
	MoveTo(r.left+2, r.top+2);
	LineTo(r.left+2, r.bottom-4);
	
	// bottom shading
	RGBForeColor((RGBColor *)&color_GrayA);
	MoveTo(r.left+2, r.bottom-2);
	LineTo(r.right-3, r.bottom-2);
	RGBForeColor((RGBColor *)&color_GrayB);
	MoveTo(r.left+3, r.bottom-3);
	LineTo(r.right-4, r.bottom-3);
	SetCPixel(r.left+2, r.bottom-3, (RGBColor *)&color_GrayD);

	// right shading
	RGBForeColor((RGBColor *)&color_GrayA);
	MoveTo(r.right-2, r.top+3);
	LineTo(r.right-2, r.bottom-3);
	RGBForeColor((RGBColor *)&color_GrayB);
	MoveTo(r.right-3, r.top+3);
	LineTo(r.right-3, r.bottom-4);
	SetCPixel(r.right-3, r.bottom-3, (RGBColor *)&color_GrayA);
	
	// corners
	SetCPixel(r.left+3, r.top+3, (RGBColor *)&color_White);
	SetCPixel(r.left+1, r.top+2, (RGBColor *)&color_GrayB);
	SetCPixel(r.left+2, r.top+1, (RGBColor *)&color_GrayB);
	SetCPixel(r.left+1, r.bottom-3, (RGBColor *)&color_GrayB);
	SetCPixel(r.left+2, r.bottom-2, (RGBColor *)&color_GrayB);
	SetCPixel(r.right-3, r.top+1, (RGBColor *)&color_GrayB);
	SetCPixel(r.right-4, r.bottom-4, (RGBColor *)&color_GrayB);

	// title
	if (titleLen > 0)
	{
		RGBForeColor((RGBColor *)&color_Gray7);
		TextMode(srcOr);
		_DrawCenteredText(title, titleLen, r);
	}
}

static void _DrawDefButtonDisabled(Rect r, void *title, Uint32 titleLen)
{
	Rect tr = r;
	InsetRect(&tr, 3, 3);
	_DrawButtonDisabled(tr, title, titleLen);
	//_DrawDefault(r);
}

/* -------------------------------------------------------------------------- */

static void _DrawDefaultBW(Rect r)
{
	PenSize(2, 2);
	RGBForeColor((RGBColor *)&color_Black);
	FrameRoundRect(&r, 12, 12);
}

static void _DrawButtonBW(Rect r, void *title, Uint32 titleLen)
{
	RGBColor saveBack;
	GetBackColor(&saveBack);
	RGBBackColor((RGBColor *)&color_White);
	
	// frame and body
	PenNormal();
	RGBForeColor((RGBColor *)&color_White);
	PaintRoundRect(&r, 8, 8);
	RGBForeColor((RGBColor *)&color_Black);
	FrameRoundRect(&r, 8, 8);
	
	// title
	if (titleLen > 0)
	{
		TextMode(srcOr);
		_DrawCenteredText(title, titleLen, r);
	}
	RGBBackColor(&saveBack);
}

static void _DrawDefButtonBW(Rect r, void *title, Uint32 titleLen)
{
	Rect tr = r;
	InsetRect(&tr, 3, 3);
	_DrawButtonBW(tr, title, titleLen);
	_DrawDefaultBW(r);
}

static void _DrawButtonDisabledBW(Rect r, void *title, Uint32 titleLen)
{
	RGBColor saveBack;
	GetBackColor(&saveBack);
	RGBBackColor((RGBColor *)&color_White);
	
	// frame and body
	PenNormal();
	RGBForeColor((RGBColor *)&color_White);
	PaintRoundRect(&r, 8, 8);
	RGBForeColor((RGBColor *)&color_Black);
	FrameRoundRect(&r, 8, 8);
	
	// title
	if (titleLen > 0)
	{
		TextMode(grayishTextOr);
		_DrawCenteredText(title, titleLen, r);
		TextMode(srcOr);
	}
	RGBBackColor(&saveBack);
}

static void _DrawDefButtonDisabledBW(Rect r, void *title, Uint32 titleLen)
{
	Rect tr = r;
	InsetRect(&tr, 3, 3);
	_DrawButtonDisabledBW(tr, title, titleLen);
	//_DrawDefaultBW(r);
}

static void _DrawButtonHilitedBW(Rect r, void *title, Uint32 titleLen)
{
	RGBColor saveBack;
	GetBackColor(&saveBack);
	RGBBackColor((RGBColor *)&color_White);
	
	// frame and body
	PenNormal();
	RGBForeColor((RGBColor *)&color_Black);
	PaintRoundRect(&r, 8, 8);
	
	// title
	if (titleLen > 0)
	{
		RGBForeColor((RGBColor *)&color_White);
		TextMode(srcOr);
		_DrawCenteredText(title, titleLen, r);
	}
	RGBBackColor(&saveBack);
}

static void _DrawDefButtonHilitedBW(Rect r, void *title, Uint32 titleLen)
{
	Rect tr = r;
	InsetRect(&tr, 3, 3);
	_DrawButtonHilitedBW(tr, title, titleLen);
	_DrawDefaultBW(r);
}

#endif /* MACINTOSH */
