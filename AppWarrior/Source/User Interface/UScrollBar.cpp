/***********************************************

enum {
	sdOption_HorizBar	= 0x01,
	sdOption_VertBar	= 0x02,
	sdOption_GrowSpace	= 0x04,
	sdOption_LeftHeader	= 0x08,
	sdOption_TopHeader	= 0x10
};

enum {
	sdIndex_Content		= 0,
	sdIndex_HorizBar	= 1,
	sdIndex_VertBar		= 2,
	sdIndex_LeftHeader	= 3,
	sdIndex_TopHeader	= 4
};

******** maybe make it scOption and CalcScrollerRects ?

Uint32 CalcDocRects(const SRect& inBounds, Uint32 inOptions, Uint32 inBorderSize, SRect *outRects, Uint32 inMaxRects);

Also, portable func for calculating CScrollerView rects. Including
support for headers like in spreadsheet (on top, left, bottom or right
- up to 4) or like in Finder windows.  These should scroll so vis rect
and dest rect are needed.  Support for menu scroller or do it the 
windoze way by using a scroll bar ?

Do it the windoze way!!

*** support for hiding the scrollbars if not needed?
(if the content fits inside fully, optionally don't show scrollbars)
(note this is separate for each bar - eg vert can be hidden and horiz shown)

******************************************************/

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UScrollBar.h"

void UScrollBar_Draw(TImage inImage, const SRect& inBounds, const SScrollBarInfo& inInfo, Uint32 inHilitedPart);
void UScrollBar_DrawDisabled(TImage inImage, const SRect& inBounds, const SScrollBarInfo& inInfo);

void _SetVirtualOrigin(TImage inImage, const SPoint& inVirtualOrigin);
void _GetVirtualOrigin(TImage inImage, SPoint& outVirtualOrigin);
void _ResetVirtualOrigin(TImage inImage);

/* -------------------------------------------------------------------------- */

void UScrollBar::Draw(TImage inImage, const SRect& inBounds, const SScrollBarInfo& inInfo, Uint32 inHilitedPart)
{
	// get virtual origin
	SPoint stVirtualOrigin;
	_GetVirtualOrigin(inImage, stVirtualOrigin);

	if (stVirtualOrigin.IsNull())
		UScrollBar_Draw(inImage, inBounds, inInfo, inHilitedPart);
	else
	{
		// reset virtual origin
		_ResetVirtualOrigin(inImage);

		// recalculate rect
		SRect stBounds = inBounds;
		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);
	
		// draw
		UScrollBar_Draw(inImage, stBounds, inInfo, inHilitedPart);

		// restore virtual origin
		_SetVirtualOrigin(inImage, stVirtualOrigin);
	}
}

void UScrollBar::DrawDisabled(TImage inImage, const SRect& inBounds, const SScrollBarInfo& inInfo)
{
	// get virtual origin
	SPoint stVirtualOrigin;
	_GetVirtualOrigin(inImage, stVirtualOrigin);

	if (stVirtualOrigin.IsNull())
		UScrollBar_DrawDisabled(inImage, inBounds, inInfo);
	else
	{
		// reset virtual origin
		_ResetVirtualOrigin(inImage);

		// recalculate rect
		SRect stBounds = inBounds;
		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);
	
		// draw
		UScrollBar_DrawDisabled(inImage, stBounds, inInfo);

		// restore virtual origin
		_SetVirtualOrigin(inImage, stVirtualOrigin);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

Uint32 UScrollBar::CalcVertRects(const SRect& inBounds, Uint32 inWidth, Uint32 inButtonSize, Uint32 inMinThumbSize, Uint32 inVal, Uint32 inMax, Uint32 inVisVal, Uint32 inVisMax, SRect *outRects, Uint32 inMaxRects)
{
	Uint32 maxThumbSize, thumbSize, thumbLoc, contentTop, contentBottom, n;
	Int32 li;
	
	Int32 bl = inBounds.left;
	Int32 bt = inBounds.top;
	Int32 br = bl + inWidth;
	Int32 bb = inBounds.bottom;

	if ((bt > bb) || (bb - bt < (inButtonSize + inButtonSize - 1)))
	{
		n = 7;
		if (n > inMaxRects) n = inMaxRects;
		while (n--)
		{
			outRects->left = 0;
			outRects->top = 0;
			outRects->right = 0;
			outRects->bottom = 0;
			outRects++;
		}
		return n;
	}
	
	// calc content top and bottom
	if (bb - bt < (inButtonSize * 7) + inMinThumbSize)
	{
		// not enough room for second buttons
		contentTop = bt + inButtonSize - 1;
		contentBottom = bb - inButtonSize + 1;
		
		// no second buttons
		if (sbIndex_SecondDown < inMaxRects) outRects[sbIndex_SecondDown].SetEmpty();
		if (sbIndex_SecondUp < inMaxRects) outRects[sbIndex_SecondUp].SetEmpty();
	}
	else
	{
		// allow space for second buttons
		contentTop = bt + inButtonSize + inButtonSize - 2;
		contentBottom = bb - inButtonSize - inButtonSize + 2;

		// calc part SecondDown
		if (sbIndex_SecondDown < inMaxRects)
		{
			li = bt + inButtonSize - 1;
			outRects[sbIndex_SecondDown].Set(bl, li, br, li + inButtonSize);
		}
		
		// calc part SecondUp
		if (sbIndex_SecondUp < inMaxRects)
		{
			li = bb - inButtonSize - inButtonSize + 1;
			outRects[sbIndex_SecondUp].Set(bl, li, br, li + inButtonSize);
		}
	}
	
	// calc part Up
	if (sbIndex_Up < inMaxRects)
		outRects[sbIndex_Up].Set(bl, bt, br, bt + inButtonSize);
	
	// calc part Down
	if (sbIndex_Down < inMaxRects)
		outRects[sbIndex_Down].Set(bl, bb - inButtonSize, br, bb);
	
	if (contentBottom - contentTop < (inMinThumbSize * 2))
	{
		// not enough room for thumb
		if (sbIndex_PageUp < inMaxRects) outRects[sbIndex_PageUp].Set(bl, contentTop, br, contentBottom);
		if (sbIndex_PageDown < inMaxRects) outRects[sbIndex_PageDown].SetEmpty();
		if (sbIndex_Thumb < inMaxRects) outRects[sbIndex_Thumb].SetEmpty();
	}
	else
	{
		// calculate thumb size and location size
		maxThumbSize = contentBottom - contentTop;
		if (inVisVal < inVisMax)
			thumbSize = maxThumbSize * ((fast_float)inVisVal / (fast_float)inVisMax);
		else
			thumbSize = maxThumbSize;
		if (thumbSize < inMinThumbSize)
			thumbSize = inMinThumbSize;
		if (inVal < inMax)
			thumbLoc = (maxThumbSize - thumbSize) * ((fast_float)inVal / (fast_float)inMax);
		else
			thumbLoc = maxThumbSize - thumbSize;
		
		// calc part PageUp
		if (sbIndex_PageUp < inMaxRects)
			outRects[sbIndex_PageUp].Set(bl, contentTop, br, contentTop + thumbLoc + 1);
		
		// calc part PageDown
		if (sbIndex_PageDown < inMaxRects)
			outRects[sbIndex_PageDown].Set(bl, contentTop + thumbLoc + thumbSize - 1, br, contentBottom);
		
		// calc part Thumb
		if (sbIndex_Thumb < inMaxRects)
			outRects[sbIndex_Thumb].Set(bl, contentTop + thumbLoc, br, contentTop + thumbLoc + thumbSize);
	}
	
	return inMaxRects > kScrollBarPartCount ? kScrollBarPartCount : inMaxRects;
}

Uint32 UScrollBar::CalcHorizRects(const SRect& inBounds, Uint32 inWidth, Uint32 inButtonSize, Uint32 inMinThumbSize, Uint32 inVal, Uint32 inMax, Uint32 inVisVal, Uint32 inVisMax, SRect *outRects, Uint32 inMaxRects)
{
	Uint32 maxThumbSize, thumbSize, thumbLoc, contentLeft, contentRight, n;
	Int32 li;
	
	Int32 bl = inBounds.left;
	Int32 bt = inBounds.top;
	Int32 br = inBounds.right;
	Int32 bb = bt + inWidth;

	if ((bl > br) || (br - bl < (inButtonSize + inButtonSize - 1)))
	{
		n = 7;
		if (n > inMaxRects) n = inMaxRects;
		while (n--)
		{
			outRects->left = 0;
			outRects->top = 0;
			outRects->right = 0;
			outRects->bottom = 0;
			outRects++;
		}
		return n;
	}
	
	// calc content left and right
	if (br - bl < (inButtonSize * 7) + inMinThumbSize)
	{
		// not enough room for second buttons
		contentLeft = bl + inButtonSize - 1;
		contentRight = br - inButtonSize + 1;
		
		// no second buttons
		if (sbIndex_SecondDown < inMaxRects) outRects[sbIndex_SecondDown].SetEmpty();
		if (sbIndex_SecondUp < inMaxRects) outRects[sbIndex_SecondUp].SetEmpty();
	}
	else
	{
		// allow space for second buttons
		contentLeft = bl + inButtonSize + inButtonSize - 2;
		contentRight = br - inButtonSize - inButtonSize + 2;

		// calc part SecondRight
		if (sbIndex_SecondDown < inMaxRects)
		{
			li = bl + inButtonSize - 1;
			outRects[sbIndex_SecondDown].Set(li, bt, li + inButtonSize, bb);
		}
		
		// calc part SecondLeft
		if (sbIndex_SecondUp < inMaxRects)
		{
			li = br - inButtonSize - inButtonSize + 1;
			outRects[sbIndex_SecondUp].Set(li, bt, li + inButtonSize, bb);
		}
	}
	
	// calc part Left
	if (sbIndex_Up < inMaxRects)
		outRects[sbIndex_Up].Set(bl, bt, bl + inButtonSize, bb);
	
	// calc part Right
	if (sbIndex_Down < inMaxRects)
		outRects[sbIndex_Down].Set(br - inButtonSize, bt, br, bb);
	
	if (contentRight - contentLeft < (inMinThumbSize * 2))
	{
		// not enough room for thumb
		if (sbIndex_PageUp < inMaxRects) outRects[sbIndex_PageUp].Set(contentLeft, bt, contentRight, bb);
		if (sbIndex_PageDown < inMaxRects) outRects[sbIndex_PageDown].SetEmpty();
		if (sbIndex_Thumb < inMaxRects) outRects[sbIndex_Thumb].SetEmpty();
	}
	else
	{
		// calculate thumb size and location size
		maxThumbSize = contentRight - contentLeft;
		if (inVisVal < inVisMax)
			thumbSize = maxThumbSize * ((fast_float)inVisVal / (fast_float)inVisMax);
		else
			thumbSize = maxThumbSize;
		if (thumbSize < inMinThumbSize)
			thumbSize = inMinThumbSize;
		if (inVal < inMax)
			thumbLoc = (maxThumbSize - thumbSize) * ((fast_float)inVal / (fast_float)inMax);
		else
			thumbLoc = maxThumbSize - thumbSize;
		
		// calc part PageLeft
		if (sbIndex_PageUp < inMaxRects)
			outRects[sbIndex_PageUp].Set(contentLeft, bt, contentLeft + thumbLoc + 1, bb);
		
		// calc part PageRight
		if (sbIndex_PageDown < inMaxRects)
			outRects[sbIndex_PageDown].Set(contentLeft + thumbLoc + thumbSize - 1, bt, contentRight, bb);
		
		// calc part Thumb
		if (sbIndex_Thumb < inMaxRects)
			outRects[sbIndex_Thumb].Set(contentLeft + thumbLoc, bt, contentLeft + thumbLoc + thumbSize, bb);
	}
	
	return inMaxRects > kScrollBarPartCount ? kScrollBarPartCount : inMaxRects;
}

#if !MACINTOSH
Int32 UScrollBar::GetThumbDelta(const SRect& inBounds, const SScrollBarInfo& inInfo, const SPoint& inMouseLoc)
{
	SRect partRects[kScrollBarPartCount];
	
	if (inBounds.GetWidth() > inBounds.GetHeight())
	{
		UScrollBar::CalcHorizRects(inBounds, 16, 16, 16, inInfo.val, inInfo.max, inInfo.visVal, inInfo.visMax, partRects, kScrollBarPartCount);
		return inMouseLoc.h - partRects[sbIndex_Thumb].left;
	}
	else
	{
		UScrollBar::CalcVertRects(inBounds, 16, 16, 16, inInfo.val, inInfo.max, inInfo.visVal, inInfo.visMax, partRects, kScrollBarPartCount);
		return inMouseLoc.v - partRects[sbIndex_Thumb].top;
	}
}

Uint32 UScrollBar::GetThumbValue(const SRect& inBounds, const SScrollBarInfo& inInfo, const SPoint& inMouseLoc, Int32 inThumbDelta)
{
	Int32 baseMin, baseMax, mouseVal;
	SRect partRects[kScrollBarPartCount];

	if (inBounds.GetWidth() > inBounds.GetHeight())
	{
		UScrollBar::CalcHorizRects(inBounds, 16, 16, 16, inInfo.val, inInfo.max, inInfo.visVal, inInfo.visMax, partRects, kScrollBarPartCount);
		baseMin = partRects[sbIndex_PageLeft].left;
		baseMax = partRects[sbIndex_PageRight].right - (partRects[sbIndex_Thumb].right - partRects[sbIndex_Thumb].left);
		mouseVal = inMouseLoc.h - inThumbDelta;
	}
	else
	{
		UScrollBar::CalcVertRects(inBounds, 16, 16, 16, inInfo.val, inInfo.max, inInfo.visVal, inInfo.visMax, partRects, kScrollBarPartCount);
		baseMin = partRects[sbIndex_PageUp].top;
		baseMax = partRects[sbIndex_PageDown].bottom - (partRects[sbIndex_Thumb].bottom - partRects[sbIndex_Thumb].top);
		mouseVal = inMouseLoc.v - inThumbDelta;
	}
	
	if (mouseVal < baseMin) mouseVal = baseMin;
	if (mouseVal > baseMax) mouseVal = baseMax;
	
	return (inInfo.max * (mouseVal - baseMin)) / (baseMax - baseMin);
}

Uint32 UScrollBar::PointToPart(const SRect& inBounds, const SScrollBarInfo& inInfo, const SPoint& inPoint)
{
	SRect partRects[kScrollBarPartCount];
	
	if (inBounds.GetWidth() > inBounds.GetHeight())
		UScrollBar::CalcHorizRects(inBounds, 16, 16, 16, inInfo.val, inInfo.max, inInfo.visVal, inInfo.visMax, partRects, kScrollBarPartCount);
	else
		UScrollBar::CalcVertRects(inBounds, 16, 16, 16, inInfo.val, inInfo.max, inInfo.visVal, inInfo.visMax, partRects, kScrollBarPartCount);

	if (!partRects[sbIndex_Up].IsEmpty())
	{
		if (partRects[sbIndex_Up].Contains(inPoint)) return sbPart_Up;
		if (partRects[sbIndex_Down].Contains(inPoint)) return sbPart_Down;
		if (partRects[sbIndex_SecondUp].Contains(inPoint)) return sbPart_SecondUp;
		if (partRects[sbIndex_SecondDown].Contains(inPoint)) return sbPart_SecondDown;
		
		if (!partRects[sbIndex_Thumb].IsEmpty())
		{
			if (partRects[sbIndex_PageUp].Contains(inPoint)) return sbPart_PageUp;
			if (partRects[sbIndex_PageDown].Contains(inPoint)) return sbPart_PageDown;
			if (partRects[sbIndex_Thumb].Contains(inPoint)) return sbPart_Thumb;
		}
	}
	
	return 0;
}

void UScrollBar::GetPartRect(const SRect& inBounds, const SScrollBarInfo& inInfo, Uint32 inPart, SRect& outRect)
{
	SRect partRects[kScrollBarPartCount];
	
	if (inBounds.GetWidth() > inBounds.GetHeight())
		UScrollBar::CalcHorizRects(inBounds, 16, 16, 16, inInfo.val, inInfo.max, inInfo.visVal, inInfo.visMax, partRects, kScrollBarPartCount);
	else
		UScrollBar::CalcVertRects(inBounds, 16, 16, 16, inInfo.val, inInfo.max, inInfo.visVal, inInfo.visMax, partRects, kScrollBarPartCount);

	switch (inPart)
	{
		case sbPart_Up:
			outRect = partRects[sbIndex_Up];
			break;
		case sbPart_Down:
			outRect = partRects[sbIndex_Down];
			break;
		case sbPart_SecondUp:
			outRect = partRects[sbIndex_SecondUp];
			break;
		case sbPart_SecondDown:
			outRect = partRects[sbIndex_SecondDown];
			break;
		case sbPart_PageUp:
			outRect = partRects[sbIndex_PageUp];
			break;
		case sbPart_PageDown:
			outRect = partRects[sbIndex_PageDown];
			break;
		case sbPart_Thumb:
			outRect = partRects[sbIndex_Thumb];
			break;
		default:
			outRect.SetEmpty();
			break;
	}
}

#endif	// MACINTOSH

