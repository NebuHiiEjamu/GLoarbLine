/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "UGraphics.h"

struct SButtonInfo {
	void *title;
	Uint32 titleSize;
	Uint32 encoding;
	Uint32 options;		// 1 = default
};

class UButton
{
	public:
		static void Draw(TImage inImage, const SRect& inBounds, const SButtonInfo& inInfo);
		static void DrawFocused(TImage inImage, const SRect& inBounds, const SButtonInfo& inInfo);
		static void DrawHilited(TImage inImage, const SRect& inBounds, const SButtonInfo& inInfo);
		static void DrawDisabled(TImage inImage, const SRect& inBounds, const SButtonInfo& inInfo);
};

