/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "UGraphics.h"

struct SProgressBarInfo {
	Uint32 val, max;
	Uint32 options;
};

class UProgressBar
{
	public:
		static void Draw(TImage inImage, const SRect& inBounds, const SProgressBarInfo& inInfo);
};



