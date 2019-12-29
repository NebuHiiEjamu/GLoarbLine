#if WIN32

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UUserInterface.h"

HDC _ImageToDC(TImage inImage);

extern HPEN _3DSHADOW_PEN, _3DHILIGHT_PEN, _ACTIVECAPTION_PEN;
extern HBRUSH _3DDKSHADOW_BRUSH, _ACTIVECAPTION_BRUSH, _3DSHADOW_BRUSH;

/* -------------------------------------------------------------------------- */

void UUserInterface_DrawStandardBox(TImage inImage, const SRect& inBounds, const SColor *inFillColor, bool inIsDisabled, bool inCanFocus, bool inIsFocus)
{
	RECT r;
	HPEN savedPen;
	HBRUSH brush;
	
	HDC dc = _ImageToDC(inImage);

	// grab a copy of the bounds
	r.top = inBounds.top;
	r.left = inBounds.left;
	r.bottom = inBounds.bottom;
	r.right = inBounds.right;
	
	if (inCanFocus && inIsFocus && !inIsDisabled)	// if has focus border
	{
		// draw focus border
		savedPen = (HPEN)SelectObject(dc, _ACTIVECAPTION_PEN);
		::MoveToEx(dc, r.left, r.top+1, NULL);
		::LineTo(dc, r.left, r.bottom-1);
		::MoveToEx(dc, r.left+1, r.top, NULL);
		::LineTo(dc, r.right-1, r.top);
		::MoveToEx(dc, r.right-1, r.top+1, NULL);
		::LineTo(dc, r.right-1, r.bottom-1);
		::MoveToEx(dc, r.left+1, r.bottom-1, NULL);
		::LineTo(dc, r.right-1, r.bottom-1);
		SelectObject(dc, savedPen);
		r.left++;
		r.top++;
		r.right--;
		r.bottom--;
		::FrameRect(dc, &r, _ACTIVECAPTION_BRUSH);
		
		// draw frame
		r.left++;
		r.top++;
		r.right--;
		r.bottom--;
		::FrameRect(dc, &r, _3DDKSHADOW_BRUSH);
	}
	else	// doesn't have focus border
	{
		// if can focus, make room for the focus border
		if (inCanFocus)
		{
			r.left++;
			r.top++;
			r.right--;
			r.bottom--;
		}
		
		// draw dark area
		savedPen = (HPEN)SelectObject(dc, _3DSHADOW_PEN);
		MoveToEx(dc, r.right-2, r.top, NULL);
		LineTo(dc, r.left, r.top);
		LineTo(dc, r.left, r.bottom-1);

		// draw light area
		SelectObject(dc, _3DHILIGHT_PEN);
		MoveToEx(dc, r.left+1, r.bottom-1, NULL);
		LineTo(dc, r.right-1, r.bottom-1);
		LineTo(dc, r.right-1, r.top);

		// restore pen
		SelectObject(dc, savedPen);
		
		// draw frame
		r.left++;
		r.top++;
		r.right--;
		r.bottom--;
		::FrameRect(dc, &r, _3DDKSHADOW_BRUSH);
	}
	
	// fill content if applicable
	if (inFillColor)
	{
		r.left++;
		r.top++;
		r.right--;
		r.bottom--;

		brush = CreateSolidBrush(RGB(inFillColor->red >> 8, inFillColor->green >> 8, inFillColor->blue >> 8));
		FillRect(dc, &r, brush);
		DeleteObject(brush);
	}
}

void UUserInterface_DrawEtchedBox(TImage inImage, const SRect& inBounds, const void *inTitle, Uint32 inTitleSize)
{
	RECT r;
	HPEN savedPen;
	COLORREF savedColor;

	HDC dc = _ImageToDC(inImage);

	r.top = inBounds.top;
	r.left = inBounds.left;
	r.bottom = inBounds.bottom;
	r.right = inBounds.right;
	
	if (inTitle == nil || inTitleSize == 0 || (inBounds.right - inBounds.left) <= 54)
	{
		r.right--;
		r.bottom--;
		
		::FrameRect(dc, &r, _3DSHADOW_BRUSH);
		
		r.right++;
		r.bottom++;
		
		savedPen = (HPEN)SelectObject(dc, _3DHILIGHT_PEN);
		
		::MoveToEx(dc, r.left+1, r.top+1, NULL);
		::LineTo(dc, r.right-2, r.top+1);
		::MoveToEx(dc, r.left+1, r.top+1, NULL);
		::LineTo(dc, r.left+1, r.bottom-2);
		::MoveToEx(dc, r.left, r.bottom-1, NULL);
		::LineTo(dc, r.right, r.bottom-1);
		::MoveToEx(dc, r.right-1, r.top, NULL);
		::LineTo(dc, r.right-1, r.bottom-1);
		
		SelectObject(dc, savedPen);
	}
	else
	{
		Uint32 titleWidth, titleHeight, n;
		TEXTMETRIC tm;
		SIZE sx;
		
		GetTextMetrics(dc, &tm);
		GetTextExtentPoint32(dc, (char *)inTitle, inTitleSize, &sx);
		titleHeight = tm.tmAscent;
		titleWidth = sx.cx;

		n = (inBounds.right - inBounds.left) - 32;
		if (titleWidth > n) titleWidth = n;

		r.top += (titleHeight / 2) - 1;
		savedPen = (HPEN)SelectObject(dc, _3DSHADOW_PEN);
		
		::MoveToEx(dc, r.left, r.top, NULL);
		::LineTo(dc, r.left, r.bottom-2);
		::MoveToEx(dc, r.left, r.bottom-2, NULL);
		::LineTo(dc, r.right-1, r.bottom-2);
		::MoveToEx(dc, r.right-2, r.top, NULL);
		::LineTo(dc, r.right-2, r.bottom-2);
		::MoveToEx(dc, r.left, r.top, NULL);
		::LineTo(dc, r.left + 12, r.top);
		::MoveToEx(dc, r.left + 12 + 4 + titleWidth + 4, r.top, NULL);
		::LineTo(dc, r.right-2, r.top);
		
		SelectObject(dc, _3DHILIGHT_PEN);
		
		::MoveToEx(dc, r.left+1, r.top+1, NULL);
		::LineTo(dc, r.left+1, r.bottom-2);
		::MoveToEx(dc, r.left, r.bottom-1, NULL);
		::LineTo(dc, r.right, r.bottom-1);
		::MoveToEx(dc, r.right-1, r.top, NULL);
		::LineTo(dc, r.right-1, r.bottom-1);
		::MoveToEx(dc, r.left+2, r.top+1, NULL);
		::LineTo(dc, r.left + 12, r.top+1);
		::MoveToEx(dc, r.left + 12 + 4 + titleWidth + 4, r.top+1, NULL);
		::LineTo(dc, r.right-2, r.top+1);
		
		SelectObject(dc, savedPen);
		
		r.top = inBounds.top;
		r.left += 12 + 4;
		r.right = r.left + titleWidth;
		r.bottom = r.top + titleHeight;
		
		savedColor = ::SetTextColor(dc, ::GetSysColor(COLOR_BTNTEXT));
	
		DrawTextEx(dc, (char *)inTitle, inTitleSize, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_END_ELLIPSIS, NULL);
		
		SetTextColor(dc, savedColor);
	}
}

void UUserInterface_DrawSunkenBox(TImage inImage, const SRect& inBounds, const SColor *inColor)
{
	enum {
		changeVal	= 0x33
	};
	
	HDC dc = _ImageToDC(inImage);

	HPEN savedPen;
	HBRUSH brush;
	COLORREF col;
	RECT r;
	Uint8 red, green, blue, cred, cgreen, cblue;
	
	// grab a copy of the bounds
	r.top = inBounds.top;
	r.left = inBounds.left;
	r.bottom = inBounds.bottom;
	r.right = inBounds.right;

	if (inColor)
	{
		// convert 48-bit color to 24-bit
		cred = inColor->red >> 8;
		cgreen = inColor->green >> 8;
		cblue = inColor->blue >> 8;
				
		// determine darker color
		red = cred;
		green = cgreen;
		blue = cblue;
		if (red < changeVal) red = 0; else red -= changeVal;
		if (green < changeVal) green = 0; else green -= changeVal;
		if (blue < changeVal) blue = 0; else blue -= changeVal;
		
		// draw dark
		savedPen = (HPEN)SelectObject(dc, CreatePen(PS_SOLID, 1, RGB(red, green, blue)));
		::MoveToEx(dc, r.right-2, r.top, NULL);
		::LineTo(dc, r.left, r.top);
		::LineTo(dc, r.left, r.bottom-1);
		DeleteObject(SelectObject(dc, savedPen));
		
		// determine lighter color
		red = cred;
		green = cgreen;
		blue = cblue;
		if ((0xFF - red) < changeVal) red = 0xFF; else red += changeVal;
		if ((0xFF - green) < changeVal) green = 0xFF; else green += changeVal;
		if ((0xFF - blue) < changeVal) blue = 0xFF; else blue += changeVal;
		
		// draw light
		savedPen = (HPEN)SelectObject(dc, CreatePen(PS_SOLID, 1, RGB(red, green, blue)));
		::MoveToEx(dc, r.left+1, r.bottom-1, NULL);
		::LineTo(dc, r.right-1, r.bottom-1);
		::LineTo(dc, r.right-1, r.top);
		DeleteObject(SelectObject(dc, savedPen));

		// draw corners
		col = RGB(cred, cgreen, cblue);
		SetPixel(dc, r.left, r.bottom-1, col);
		SetPixel(dc, r.right-1, r.top, col);

		// fill inside
		r.top++;
		r.left++;
		r.bottom--;
		r.right--;
		brush = CreateSolidBrush(col);
		FillRect(dc, &r, brush);
		DeleteObject(brush);
	}
	else
	{
		// draw dark
		savedPen = (HPEN)SelectObject(dc, _3DSHADOW_PEN);
		::MoveToEx(dc, r.right-2, r.top, NULL);
		::LineTo(dc, r.left, r.top);
		::LineTo(dc, r.left, r.bottom-1);
	
		// draw light
		SelectObject(dc, _3DHILIGHT_PEN);
		::MoveToEx(dc, r.left+1, r.bottom-1, NULL);
		::LineTo(dc, r.right-1, r.bottom-1);
		::LineTo(dc, r.right-1, r.top);
		
		// restore pen
		SelectObject(dc, savedPen);
	}
}

void UUserInterface_DrawRaisedBox(TImage inImage, const SRect& inBounds, const SColor *inColor)
{
	enum {
		changeVal	= 0x33
	};
	
	HDC dc = _ImageToDC(inImage);

	HPEN savedPen;
	HBRUSH brush;
	COLORREF col;
	RECT r;
	Uint8 red, green, blue, cred, cgreen, cblue;
	
	// grab a copy of the bounds
	r.top = inBounds.top;
	r.left = inBounds.left;
	r.bottom = inBounds.bottom;
	r.right = inBounds.right;

	if (inColor)
	{
		// convert 48-bit color to 24-bit
		cred = inColor->red >> 8;
		cgreen = inColor->green >> 8;
		cblue = inColor->blue >> 8;
				
		// determine lighter color
		red = cred;
		green = cgreen;
		blue = cblue;
		if ((0xFF - red) < changeVal) red = 0xFF; else red += changeVal;
		if ((0xFF - green) < changeVal) green = 0xFF; else green += changeVal;
		if ((0xFF - blue) < changeVal) blue = 0xFF; else blue += changeVal;
		
		// draw light
		savedPen = (HPEN)SelectObject(dc, CreatePen(PS_SOLID, 1, RGB(red, green, blue)));
		::MoveToEx(dc, r.right-2, r.top, NULL);
		::LineTo(dc, r.left, r.top);
		::LineTo(dc, r.left, r.bottom-1);
		DeleteObject(SelectObject(dc, savedPen));
		
		// determine darker color
		red = cred;
		green = cgreen;
		blue = cblue;
		if (red < changeVal) red = 0; else red -= changeVal;
		if (green < changeVal) green = 0; else green -= changeVal;
		if (blue < changeVal) blue = 0; else blue -= changeVal;
		
		// draw dark
		savedPen = (HPEN)SelectObject(dc, CreatePen(PS_SOLID, 1, RGB(red, green, blue)));
		::MoveToEx(dc, r.left+1, r.bottom-1, NULL);
		::LineTo(dc, r.right-1, r.bottom-1);
		::LineTo(dc, r.right-1, r.top);
		DeleteObject(SelectObject(dc, savedPen));

		// draw corners
		col = RGB(cred, cgreen, cblue);
		SetPixel(dc, r.left, r.bottom-1, col);
		SetPixel(dc, r.right-1, r.top, col);

		// fill inside
		r.top++;
		r.left++;
		r.bottom--;
		r.right--;
		brush = CreateSolidBrush(col);
		FillRect(dc, &r, brush);
		DeleteObject(brush);
	}
	else
	{
		// draw light
		savedPen = (HPEN)SelectObject(dc, _3DHILIGHT_PEN);
		::MoveToEx(dc, r.right-2, r.top, NULL);
		::LineTo(dc, r.left, r.top);
		::LineTo(dc, r.left, r.bottom-1);
	
		// draw dark
		SelectObject(dc, _3DSHADOW_PEN);
		::MoveToEx(dc, r.left+1, r.bottom-1, NULL);
		::LineTo(dc, r.right-1, r.bottom-1);
		::LineTo(dc, r.right-1, r.top);
		
		// restore pen
		SelectObject(dc, savedPen);
	}
}

void UUserInterface_DrawBarBox(TImage inImage, const SRect& inBounds)
{
	enum {
		th = 2 + 2
	};
	Int32 top, left, bottom, right;
	HPEN savedPen;
	
	HDC dc = _ImageToDC(inImage);

	top = inBounds.top;
	left = inBounds.left;
	bottom = inBounds.bottom;
	right = inBounds.right;
	
	// draw light lines
	savedPen = (HPEN)SelectObject(dc, _3DHILIGHT_PEN);
	::MoveToEx(dc, left, top, NULL);
	::LineTo(dc, right-1, top);
	::MoveToEx(dc, left, top, NULL);
	::LineTo(dc, left, bottom-1);
	::MoveToEx(dc, left+th, bottom-th, NULL);
	::LineTo(dc, right-th+1, bottom-th);
	::MoveToEx(dc, right-th, top+th, NULL);
	::LineTo(dc, right-th, bottom-th);
	
	// draw dark lines
	SelectObject(dc, _3DSHADOW_PEN);
	::MoveToEx(dc, left+th-1, top+th-1, NULL);
	::LineTo(dc, right-th, top+th-1);
	::MoveToEx(dc, left+th-1, top+th, NULL);
	::LineTo(dc, left+th-1, bottom-th);
	::MoveToEx(dc, left+1, bottom-1, NULL);
	::LineTo(dc, right, bottom-1);
	::MoveToEx(dc, right-1, top+1, NULL);
	::LineTo(dc, right-1, bottom-1);

	// restore pen
	SelectObject(dc, savedPen);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UUserInterface::GetHighlightColor(SColor *outHilite, SColor *outInverse)
{
	COLORREF c;
	
	if (outHilite)
	{
		c = ::GetSysColor(COLOR_HIGHLIGHT);
		outHilite->Set(GetRValue(c) * 257, GetGValue(c) * 257, GetBValue(c) * 257);
	}
	
	if (outInverse)
	{
		c = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		outInverse->Set(GetRValue(c) * 257, GetGValue(c) * 257, GetBValue(c) * 257);
	}
}

// returns false if unknown color (outColor is set to black)
bool UUserInterface::GetSysColor(Uint32 inNum, SColor& outColor)
{
	COLORREF c;

	switch (inNum)
	{
		case sysColor_Background:
			c = ::GetSysColor(COLOR_WINDOW);
			break;
		case sysColor_Content:
			c = RGB(0xFF, 0xFF, 0xFF);
			break;
		case sysColor_Light:
			c = ::GetSysColor(COLOR_3DHILIGHT);
			break;
		case sysColor_Dark:
			c = ::GetSysColor(COLOR_3DSHADOW);
			break;
		case sysColor_Frame:
			c = ::GetSysColor(COLOR_3DDKSHADOW);
			break;
		case sysColor_Label:
			//c = ::GetSysColor(COLOR_CAPTIONTEXT);	// stupid thing gives white for this so we'll use COLOR_BTNTEXT instead
			//break;
		case sysColor_ButtonLabel:
			c = ::GetSysColor(COLOR_BTNTEXT);
			break;
		case sysColor_Highlight:
			c = ::GetSysColor(COLOR_HIGHLIGHT);
			break;
		case sysColor_InverseHighlight:
			c = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
			break;
		default:
			outColor.Set(0);
			return false;
	}

	outColor.Set(GetRValue(c) * 257, GetGValue(c) * 257, GetBValue(c) * 257);
	return true;
}








#endif /* WIN32 */
