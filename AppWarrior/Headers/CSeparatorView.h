/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "CView.h"

class CSeparatorView : public CView
{
	public:
		CSeparatorView(CViewHandler *inHandler, const SRect& inBounds);

		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);
};


