/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "UGraphics.h"

enum {
	sbIndex_Up			= 0,
	sbIndex_Down		= 1,
	sbIndex_SecondUp	= 2,
	sbIndex_SecondDown	= 3,
	sbIndex_PageUp		= 4,
	sbIndex_PageDown	= 5,
	sbIndex_Thumb		= 6,
	
	sbIndex_Left		= sbIndex_Up,
	sbIndex_Right		= sbIndex_Down,
	sbIndex_SecondLeft	= sbIndex_SecondUp,
	sbIndex_SecondRight	= sbIndex_SecondDown,
	sbIndex_PageLeft	= sbIndex_PageUp,
	sbIndex_PageRight	= sbIndex_PageDown,
	
	sbPart_Up			= sbIndex_Up + 1,
	sbPart_Down			= sbIndex_Down + 1,
	sbPart_SecondUp		= sbIndex_SecondUp + 1,
	sbPart_SecondDown	= sbIndex_SecondDown + 1,
	sbPart_PageUp		= sbIndex_PageUp + 1,
	sbPart_PageDown		= sbIndex_PageDown + 1,
	sbPart_Thumb		= sbIndex_Thumb + 1,
	
	sbPart_Left			= sbPart_Up,
	sbPart_Right		= sbPart_Down,
	sbPart_SecondLeft	= sbPart_SecondUp,
	sbPart_SecondRight	= sbPart_SecondDown,
	sbPart_PageLeft		= sbPart_PageUp,
	sbPart_PageRight	= sbPart_PageDown,

	kScrollBarPartCount	= 7,
	kScrollBarWidth		= 16
};

struct SScrollBarInfo {
	Uint32 val, max;
	Uint32 visVal, visMax;
	Uint32 options;
};

class UScrollBar
{
	public:
		// drawing
		static void Draw(TImage inImage, const SRect& inBounds, const SScrollBarInfo& inInfo, Uint32 inHilitedPart = 0);
		static void DrawDisabled(TImage inImage, const SRect& inBounds, const SScrollBarInfo& inInfo);

		// parts
		static Uint32 PointToPart(const SRect& inBounds, const SScrollBarInfo& inInfo, const SPoint& inPoint);
		static void GetPartRect(const SRect& inBounds, const SScrollBarInfo& inInfo, Uint32 inPart, SRect& outRect);
		static Int32 GetThumbDelta(const SRect& inBounds, const SScrollBarInfo& inInfo, const SPoint& inMouseLoc);
		static Uint32 GetThumbValue(const SRect& inBounds, const SScrollBarInfo& inInfo, const SPoint& inMouseLoc, Int32 inThumbDelta);

		// calculate rects
		static Uint32 CalcHorizRects(const SRect& inBounds, Uint32 inWidth, Uint32 inButtonSize, Uint32 inMinThumbSize, Uint32 inVal, Uint32 inMax, Uint32 inVisVal, Uint32 inVisMax, SRect *outRects, Uint32 inMaxRects);
		static Uint32 CalcVertRects(const SRect& inBounds, Uint32 inWidth, Uint32 inButtonSize, Uint32 inMinThumbSize, Uint32 inVal, Uint32 inMax, Uint32 inVisVal, Uint32 inVisMax, SRect *outRects, Uint32 inMaxRects);
};



