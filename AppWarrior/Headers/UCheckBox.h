/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "UGraphics.h"

struct SCheckBoxInfo {
	Uint16 style;			// 0 = square, 1 = round
	Uint16 mark;			// 0 = no mark, 1 = marked, 2 = alt marked
	void *title;
	Uint32 titleSize;
	Uint32 encoding;
	Uint32 options;
};

class UCheckBox
{
	public:
		static void Draw(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo);
		static void DrawFocused(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo);
		static void DrawHilited(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo);
		static void DrawDisabled(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo);
};

