/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CSeparatorView.h"

#pragma options align=packed

typedef struct {
	Int16 style;
} SSeparatorView;

#pragma options align=reset

/* -------------------------------------------------------------------------- */

CSeparatorView::CSeparatorView(CViewHandler *inHandler, const SRect& inBounds)
	: CView(inHandler, inBounds)
{
	// nothing more to do
}

void CSeparatorView::Draw(TImage inImage, const SRect& /* inUpdateRect */, Uint32 /* inDepth */)
{
	SRect r = mBounds;
	
	inImage->SetPenSize(1);
	
	if (r.GetHeight() > r.GetWidth())
	{
		inImage->SetInkColor(color_Gray8);
		inImage->DrawLine(SLine(r.left, r.top, r.left, r.bottom-2));
		inImage->SetInkColor(color_White);
		inImage->DrawLine(SLine(r.left+1, r.top+1, r.left+1, r.bottom-1));
	}
	else
	{
		inImage->SetInkColor(color_Gray8);
		inImage->DrawLine(SLine(r.left, r.top, r.right-2, r.top));
		inImage->SetInkColor(color_White);
		inImage->DrawLine(SLine(r.left+1, r.top+1, r.right-1, r.top+1));
	}
}


