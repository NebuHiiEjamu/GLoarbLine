/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CLabelView.h"

#pragma options align=packed

typedef struct {
	Int16 textStyle;
	Int16 script;
	Uint16 titleLen;
	Uint8 titleData[];
} SLabelView;

#pragma options align=reset

/* -------------------------------------------------------------------------- */

CLabelView::CLabelView(CViewHandler *inHandler, const SRect& inBounds)
	: CView(inHandler, inBounds)
{
	mText = nil;
	mFont = nil;
}

CLabelView::CLabelView(CViewHandler *inHandler, const SRect& inBounds, const Uint8 *inText, TFontDesc inFont)
	: CView(inHandler, inBounds)
{
	mText = inText ? UMemory::NewHandle(inText+1, inText[0]) : nil;
	mFont = inFont;
}

CLabelView::~CLabelView()
{
	delete mFont;
	UMemory::Dispose(mText);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CLabelView::SetText(const void *inText, Uint32 inSize)
{
	if (mText == nil)
		mText = UMemory::NewHandle(inText, inSize);
	else
		UMemory::Set(mText, inText, inSize);
	
	Refresh();
}

void CLabelView::AppendText(const void *inText, Uint32 inSize)
{
	if (inSize)
	{
		if (mText == nil)
			mText = UMemory::NewHandle(inText, inSize);
		else
			UMemory::Append(mText, inText, inSize);
		
		Refresh();
	}
}

// takes ownership of the TFontDesc
void CLabelView::SetFont(TFontDesc inFont)
{
	delete mFont;
	mFont = inFont;
	Refresh();
}

void CLabelView::SetFont(const Uint8 *inName, const Uint8 *inStyle, Uint32 inSize, Uint32 inEffect)
{
	TFontDesc fd = UFontDesc::New(inName, inStyle, inSize, inEffect);
	delete mFont;
	mFont = fd;
	Refresh();
}

Uint32 CLabelView::GetTextHeight() const
{
	if (mText == nil) return 0;
	TImage img = UGraphics::GetDummyImage();
	img->SetFont(mFont);
	void *p;
	StHandleLocker locker(mText, p);
	return img->GetTextBoxHeight(mBounds, p, UMemory::GetSize(mText), 0, mFont->GetAlign());
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CLabelView::Draw(TImage inImage, const SRect& inUpdateRect, Uint32 /* inDepth */)
{
	if (mText)
	{
		inImage->SetFont(mFont);
		
		void *p;
		StHandleLocker locker(mText, p);
		inImage->DrawTextBox(mBounds, inUpdateRect, p, UMemory::GetSize(mText), 0, mFont->GetAlign());
	}
}


