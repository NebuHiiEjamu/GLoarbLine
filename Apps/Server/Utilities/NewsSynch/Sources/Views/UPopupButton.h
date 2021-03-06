/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "UGraphics.h"

struct SPopupButtonInfo {
	void *title;
	Uint32 titleSize;
	Uint32 encoding;
	Uint32 options;
};

class UPopupButton
{
	public:
		static void Draw(TImage inImage, const SRect& inBounds, const SPopupButtonInfo& inInfo);
		static void DrawFocused(TImage inImage, const SRect& inBounds, const SPopupButtonInfo& inInfo);
		static void DrawHilited(TImage inImage, const SRect& inBounds, const SPopupButtonInfo& inInfo);
		static void DrawDisabled(TImage inImage, const SRect& inBounds, const SPopupButtonInfo& inInfo);
		static bool GetTitleRect(const SRect& inBounds, SRect& outRect);
};


