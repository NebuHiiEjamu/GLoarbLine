/*******************************************************************


UTextEngine - styled text.  Keep UEditText as well, it's good
for little text boxes etc, places where you don't need nor want
styled text.

UTextEngine is the almightly styled text engine supporting embedded
objects (eg pictures, sounds etc).  Should also support tabs.

For pages => UTextEngine does NOT need to support.  Pages can be 
implemented on top of UTextEngine, eg if you have two pages on screen,
that's two TTextEngine's (two instances) and when you type text and
that pushes text down onto the next page, remove lines from end of
first TTextEngine and insert at start of next.  Woohoo!

Style - effects (bold etc), font name, fontstyle, fontsize, color,
misc options, alignment (per line or paragraph or what?).
Style sheets - important.  Just needs an ID number to be stored.
Reserved space.   Bkgnd color - good for netscrawl multi-user
locked text color.

Tabs - can include alignment remember.  Look at clarisworks. 
Per line or paragraph or what?

Embedded objects - eg picture, sound etc.  Use callback procs to
draw, mousedown, etc.  Also note special wrapping - see separate
notes.

Discontiguous selection?

*************************************************************************/





/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UEditText.h"

/*
 * Structures
 */

struct SEditText {
	SRect bounds, caretRect, dragCaretRect;
	TFontDesc font;
	Uint16 lineHeight;
	Uint16 lineStartsValid	: 1;
	Uint16 selectRgnValid	: 1;
	Uint16 caretRectValid	: 1;
	Uint16 isActive			: 1;
	Uint16 isMouseDown		: 1;
	Uint16 isMouseWithin	: 1;
	Uint16 isCaretBlinkVis	: 1;
	Uint16 isEditable		: 1;
	Uint16 isSelectable		: 1;
	Uint16 isPossibleDrag	: 1;
	Uint16 isDragWithin		: 1;
	Uint16 showDragCaret	: 1;
	Int16 origMouseImageID;
	Uint32 caretTime;
	Uint32 selectStart, selectEnd, dragInsertOffset;
	Uint32 anchor, anchorStart, anchorEnd;
	Uint32 clickTime, clickOffset, clickCount;
	TRegion selectRgn;
	SPoint dragStartPt;
	TDrag curDrag;
	TEditTextDrawProc drawProc;
	TEditTextScreenDeltaProc screenDeltaProc;
	void *userRef;
	THdl textHandle;
	Uint32 textLength;
	Uint32 lineCount;
	THdl lineStartsHdl;
	Uint16 margin;
};

#define REF		inRef

/*
 * Function Prototypes
 */

static void _ETCalcLineStarts(TEditText inRef);
static void _ETCalcLineHeight(TEditText inRef);
static void _ETCalcSelectRgn(TEditText inRef);
static void _ETCalcCaretRect(TEditText inRef);
static void _ETVerifySelect(TEditText inRef);
static Uint32 _ETOffsetToLine(TEditText inRef, Uint32 offset);
static void _ETOffsetToPoint(TEditText inRef, Uint32 offset, SPoint& outPoint);
static Uint32 _ETPointToOffset(TEditText inRef, const SPoint& inPt);
static void _ETGetRangeRgn(TEditText inRef, Uint32 start, Uint32 end, TRegion rgn);
static void _ETGetRangeRect(TEditText inRef, Uint32 inStart, Uint32 inEnd, SRect& outRect);
static void _ETReorder(Uint32& a, Uint32& b);
static Int32 _ETGetLineHoriz(TImage inImage, const void *inText, Uint32 inLength, const SRect& inBounds, Int16 inAlign);
static void _ETSetDragInsertPos(TEditText inRef, Uint32 inOffset);
static bool _ETSetSelect(TEditText inRef, Uint32 inSelectStart, Uint32 inSelectEnd, SRect *outUpdateRect = nil);
static void _ETRedrawAll(TEditText inRef);
static void _ETFindWord(TEditText inRef, Uint32 inOffset, Uint32& outStart, Uint32& outEnd);
static void _ETFindLine(TEditText inRef, Uint32 inOffset, Uint32& outStart, Uint32& outEnd);
static void _ETSetCaretBlinkVis(TEditText inRef, bool inVis);
static void _ETDrag(TEditText inRef, const SPoint& inMouseLoc);

/* -------------------------------------------------------------------------- */

TEditText UEditText::New(const SRect& inBounds, Uint16 inMargin)
{
	TEditText ref = (TEditText)UMemory::NewClear(sizeof(SEditText));
	
	try
	{
		ref->margin = inMargin;
		ref->bounds = inBounds;
		ref->bounds.Enlarge(-inMargin, -inMargin);
		
		ref->isActive = true;
		ref->selectRgn = URegion::New();
		ref->selectRgnValid = true;
		ref->lineStartsHdl = UMemory::NewHandle();
		ref->textHandle = UMemory::NewHandle();
		ref->isEditable = ref->isSelectable = true;
		_ETCalcLineHeight(ref);
	}
	catch(...)
	{
		URegion::Dispose(ref->selectRgn);
		UMemory::Dispose(ref->textHandle);
		UMemory::Dispose((THdl)ref->lineStartsHdl);
		UMemory::Dispose((TPtr)ref);
		throw;
	}
	
	return ref;
}

void UEditText::Dispose(TEditText inRef)
{
	if (inRef)
	{
		UFontDesc::Dispose(REF->font);
		URegion::Dispose(REF->selectRgn);
		UMemory::Dispose((THdl)REF->lineStartsHdl);
		UMemory::Dispose((THdl)REF->textHandle);
		UMemory::Dispose((TPtr)inRef);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UEditText::SetBounds(TEditText inRef, const SRect& inBounds)
{
	Require(inRef);
	
	SRect newBounds = inBounds;
	newBounds.Enlarge(-REF->margin, -REF->margin);

	SRect oldBounds = REF->bounds;
	REF->bounds = newBounds;
	
	if (oldBounds.GetWidth() != newBounds.GetWidth())		// if width changed
	{
		REF->lineStartsValid = false;
		REF->selectRgnValid = false;
		REF->caretRectValid = false;
	}
	else if (oldBounds.TopLeft() != newBounds.TopLeft())		// if top left changed
	{
		REF->selectRgnValid = false;
		REF->caretRectValid = false;
	}
}

void UEditText::GetBounds(TEditText inRef, SRect& outBounds)
{
	Require(inRef);
	outBounds = REF->bounds;
	outBounds.Enlarge(REF->margin, REF->margin);
}

Uint32 UEditText::GetFullHeight(TEditText inRef)
{
	if (inRef)
	{
		_ETCalcLineStarts(inRef);
		Uint32 lineCount = REF->lineCount;
		if (lineCount == 0) lineCount = 1;
		return (lineCount * REF->lineHeight) + REF->margin + REF->margin;
	}
	return 0;
}

Uint16 UEditText::GetMargin(TEditText inRef)
{
	if (inRef) return REF->margin;
	return 0;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// takes ownership of font
void UEditText::SetFont(TEditText inRef, TFontDesc inFont)
{
	Require(inRef);

	UFontDesc::Dispose(REF->font);
	REF->font = inFont;

	REF->lineStartsValid = REF->caretRectValid = REF->selectRgnValid = false;

	_ETCalcLineHeight(inRef);
	_ETRedrawAll(inRef);
}

void UEditText::SetFont(TEditText inRef, const Uint8 *inName, const Uint8 *inStyle, Uint32 inSize, Uint32 inEffect)
{
	Require(inRef);
	
	TFontDesc fd = UFontDesc::New(inName, inStyle, inSize, inEffect);
	
	UFontDesc::Dispose(REF->font);
	REF->font = fd;

	REF->lineStartsValid = REF->caretRectValid = REF->selectRgnValid = false;

	_ETCalcLineHeight(inRef);
	_ETRedrawAll(inRef);
}

// you shouldn't modify the returned TFontDesc
TFontDesc UEditText::GetFont(TEditText inRef)
{
	Require(inRef);
	return REF->font;
}

void UEditText::SetFontSize(TEditText inRef, Uint32 inSize)
{
	Require(inRef);
	
	if (UFontDesc::SetFontSize(REF->font, inSize))
	{
		REF->lineStartsValid = REF->caretRectValid = REF->selectRgnValid = false;

		_ETCalcLineHeight(inRef);
		_ETRedrawAll(inRef);
	}
}

Uint32 UEditText::GetFontSize(TEditText inRef)
{
	Require(inRef);
	return UFontDesc::GetFontSize(REF->font);
}

void UEditText::SetColor(TEditText inRef, const SColor& inColor)
{
	Require(inRef);
	UFontDesc::SetColor(REF->font, inColor);
	_ETRedrawAll(inRef);
}

void UEditText::GetColor(TEditText inRef, SColor& outColor)
{
	Require(inRef);
	UFontDesc::GetColor(REF->font, outColor);
}

void UEditText::SetAlign(TEditText inRef, Uint32 inAlign)
{
	Require(inRef);
	UFontDesc::SetAlign(REF->font, inAlign);
	_ETRedrawAll(inRef);
}

Uint32 UEditText::GetAlign(TEditText inRef)
{
	Require(inRef);
	return UFontDesc::GetAlign(REF->font);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UEditText::SetSelect(TEditText inRef, Uint32 inSelectStart, Uint32 inSelectEnd)
{
	Require(inRef);
	
	if (REF->drawProc == nil)
		_ETSetSelect(inRef, inSelectStart, inSelectEnd, nil);
	else
	{
		SRect updateRect;
		if (_ETSetSelect(inRef, inSelectStart, inSelectEnd, &updateRect))
			REF->drawProc(inRef, updateRect);
	}
}

void UEditText::GetSelect(TEditText inRef, Uint32& outSelectStart, Uint32& outSelectEnd)
{
	Require(inRef);
	outSelectStart = REF->selectStart;
	outSelectEnd = REF->selectEnd;
}

void UEditText::GetSelectRegion(TEditText inRef, TRegion outRgn)
{
	Require(inRef);
	_ETCalcSelectRgn(inRef);
	URegion::Set(outRgn, REF->selectRgn);
}

void UEditText::GetSelectRect(TEditText inRef, SRect& outRect)
{
	Require(inRef);
	_ETCalcSelectRgn(inRef);
	URegion::GetBounds(REF->selectRgn, outRect);
}

// returns true if changed
bool UEditText::SetActive(TEditText inRef, bool inActive)
{
	Require(inRef);
	
	if (REF->isActive != (inActive!=0))			// if change
	{
		_ETCalcCaretRect(inRef);
		_ETCalcSelectRgn(inRef);
		SRect r;
	
		REF->isActive = (inActive != 0);
		
		// draw the change
		if (REF->selectStart == REF->selectEnd)			// if have a caret
		{
			_ETSetCaretBlinkVis(inRef, REF->isEditable);
		}
		else											// have a selection
		{
			URegion::GetBounds(REF->selectRgn, r);
			if (REF->drawProc)
				REF->drawProc(inRef, r);
		}
		
		return true;
	}
	
	return false;
}

/*
 * GetActiveRect() sets <outRect> to a rectangle indicating the area
 * of the text that will need to be redrawn if the active status of
 * the text is changed.
 */
void UEditText::GetActiveRect(TEditText inRef, SRect& outRect)
{
	Require(inRef);
	
	_ETCalcSelectRgn(inRef);
	_ETCalcCaretRect(inRef);
	
	if (REF->selectStart == REF->selectEnd)
		outRect = REF->caretRect;
	else
		URegion::GetBounds(REF->selectRgn, outRect);
	
	outRect.Constrain(REF->bounds);
}

bool UEditText::IsActive(TEditText inRef)
{
	Require(inRef);
	return REF->isActive;
}

void UEditText::SetRef(TEditText inRef, void *inValue)
{
	Require(inRef);
	REF->userRef = inValue;
}

void *UEditText::GetRef(TEditText inRef)
{
	Require(inRef);
	return REF->userRef;
}

/*
 * SetEditable() sets whether or not the USER can edit the text (InsertText() etc
 * will function regardless of this setting).  Since the user cannot edit the text
 * without being able to select it, SetEditable() will turn the selectable status
 * ON if the text is being made editable.
 */
void UEditText::SetEditable(TEditText inRef, bool inEditable)
{
	Require(inRef);
	
	if (inEditable)
		REF->isEditable = REF->isSelectable = true;	// must be selectable if editable
	else
	{
		REF->isEditable = false;
		_ETSetCaretBlinkVis(inRef, false);
	}
}

bool UEditText::IsEditable(TEditText inRef)
{
	Require(inRef);
	return REF->isEditable;
}

void UEditText::SetSelectable(TEditText inRef, bool inSelectable)
{
	Require(inRef);
	
	if (inSelectable)
		REF->isSelectable = true;
	else
		REF->isSelectable = REF->isEditable = false;	// cannot be editable if not selectable
}

bool UEditText::IsSelectable(TEditText inRef)
{
	Require(inRef);
	return REF->isSelectable;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UEditText::SetText(TEditText inRef, const void *inText, Uint32 inLength)
{
	Require(inRef);
	
	if (inText == nil || inLength == 0)
	{
		if (REF->textLength != 0)
		{
			UMemory::Reallocate(REF->textHandle, 0);
			REF->textLength = 0;
			REF->selectStart = REF->selectEnd = 0;
			REF->lineStartsValid = false;
			REF->caretRectValid = false;
			REF->selectRgnValid = false;
			_ETRedrawAll(inRef);
		}
	}
	else
	{
		if (REF->textHandle)
			UMemory::Set(REF->textHandle, inText, inLength);
		else
			REF->textHandle = UMemory::NewHandle(inText, inLength);
		
		REF->textLength = inLength;
		REF->lineStartsValid = false;
		REF->caretRectValid = false;
		REF->selectRgnValid = false;
		
		_ETVerifySelect(inRef);
		_ETRedrawAll(inRef);
	}
}

Uint32 UEditText::GetText(TEditText inRef, void *outText, Uint32 inMaxLength)
{
	if (inRef) return UMemory::Read(REF->textHandle, 0, outText, inMaxLength);
	return 0;
}

Uint32 UEditText::GetText(TEditText inRef, Uint32 inOffset, Uint32 inSourceSize, void *outText, Uint32 inMaxSize)
{
	if (inRef)
	{
		THdl textHandle = REF->textHandle;
		Uint32 textLength = REF->textLength;
		
		if (textHandle && textLength)
		{
			Require(inOffset < textLength);
			if (inOffset + inSourceSize > textLength)
				inSourceSize = textLength - inOffset;
			
			if (inSourceSize > inMaxSize)
				inSourceSize = inMaxSize;

			Uint8 *textp;
			StHandleLocker locker(textHandle, (void*&)textp);
			UMemory::Copy(outText, textp + inOffset, inSourceSize);
			
			return inSourceSize;
		}
	}
	return 0;
}

void UEditText::SetTextHandle(TEditText inRef, THdl inHdl)
{
	Require(inRef && inHdl);
	
	if (REF->textHandle != inHdl)
	{
		UMemory::Dispose(REF->textHandle);
		REF->textHandle = inHdl;
	}
	
	// recalc line starts even if is the same handle (content probably changed)
	REF->textLength = UMemory::GetSize(inHdl);
	REF->lineStartsValid = REF->selectRgnValid = REF->caretRectValid = false;
	
	_ETVerifySelect(inRef);
	_ETRedrawAll(inRef);
}

THdl UEditText::GetTextHandle(TEditText inRef)
{
	Require(inRef);
	return REF->textHandle;
}

THdl UEditText::DetachTextHandle(TEditText inRef)
{
	Require(inRef);
	
	THdl h = REF->textHandle;
	if (h)
	{
		REF->textHandle = UMemory::NewHandle();
		REF->textLength = 0;
		REF->lineStartsValid = false;
		REF->caretRectValid = false;
		REF->selectStart = REF->selectEnd = 0;
		_ETRedrawAll(inRef);
	}
	
	return h;
}

// if <inUpdateSelect> is true, the selection will be condensed to a caret and positioned at the end of the inserted text
void UEditText::InsertText(TEditText inRef, Uint32 inOffset, const void *inText, Uint32 inTextLength, bool inUpdateSelect)
{
	Uint32 drawStart, drawEnd;
	SRect updateRect, selectRect;
	
	Require(inRef);
	_ETCalcLineStarts(inRef);
	
	try
	{
		TImage img = UGraphics::GetDummyImage();
		img->SetFont(REF->font);
		
		if (LowInsert(img, REF->textHandle, (THdl)REF->lineStartsHdl, REF->bounds.GetWidth(), REF->lineCount, inOffset, inText, inTextLength, &drawStart, &drawEnd))
		{
			updateRect.left = REF->bounds.left;
			updateRect.right = REF->bounds.right;
			updateRect.top = REF->bounds.top + (drawStart * REF->lineHeight);
			updateRect.bottom = REF->bounds.top + ((drawEnd+1) * REF->lineHeight);
		}
		else
			updateRect.SetEmpty();
		
		REF->textLength = UMemory::GetSize(REF->textHandle);
	}
	catch(...)
	{
		// if anything fails, fall back to no lines
		REF->lineCount = 0;
		REF->lineStartsValid = false;
		throw;
	}

	// if applicable, set selection to caret at end of inserted text
	if (inUpdateSelect)
	{
		_ETSetSelect(inRef, inOffset + inTextLength, inOffset + inTextLength, &selectRect);
		updateRect |= selectRect;
	}
	
	// redraw
	if (REF->drawProc && !updateRect.IsEmpty())
		REF->drawProc(inRef, updateRect);
}

void UEditText::ReplaceText(TEditText inRef, Uint32 inOffset, Uint32 inExistingLength, const void *inText, Uint32 inTextLength, bool inUpdateSelect)
{
	Uint32 drawStart, drawEnd, oldLineCount;
	SRect updateRect, selectRect;
	
	Require(inRef);
	_ETCalcLineStarts(inRef);
	
	try
	{
		TImage img = UGraphics::GetDummyImage();
		img->SetFont(REF->font);

		oldLineCount = REF->lineCount;
		if (LowReplace(img, REF->textHandle, (THdl)REF->lineStartsHdl, REF->bounds.GetWidth(), REF->lineCount, inOffset, inExistingLength, inText, inTextLength, &drawStart, &drawEnd))
		{
			updateRect.left = REF->bounds.left;
			updateRect.right = REF->bounds.right;
			updateRect.top = REF->bounds.top + (drawStart * REF->lineHeight);

			if (REF->lineCount < oldLineCount)
				drawEnd = oldLineCount - 1;

			updateRect.bottom = REF->bounds.top + ((drawEnd+1) * REF->lineHeight);
		}
		else
			updateRect.SetEmpty();
		
		REF->textLength = UMemory::GetSize(REF->textHandle);
	}
	catch(...)
	{
		// if anything fails, fall back to no lines
		REF->lineCount = 0;
		REF->lineStartsValid = false;
		throw;
	}

	// if applicable, set selection to caret at end of inserted text
	if (inUpdateSelect)
	{
		_ETSetSelect(inRef, inOffset + inTextLength, inOffset + inTextLength, &selectRect);
		updateRect |= selectRect;
	}
	else
		_ETVerifySelect(inRef);

	// redraw
	if (REF->drawProc && !updateRect.IsEmpty())
		REF->drawProc(inRef, updateRect);
}

void UEditText::DeleteText(TEditText inRef, Uint32 inOffset, Uint32 inTextLength, bool inUpdateSelect)
{
	Uint32 drawStart, drawEnd, oldLineCount;
	SRect updateRect, selectRect;
	
	Require(inRef);
	_ETCalcLineStarts(inRef);
	
	try
	{
		TImage img = UGraphics::GetDummyImage();
		img->SetFont(REF->font);

		oldLineCount = REF->lineCount;
		if (LowDelete(img, REF->textHandle, (THdl)REF->lineStartsHdl, REF->bounds.GetWidth(), REF->lineCount, inOffset, inTextLength, &drawStart, &drawEnd))
		{
			updateRect.left = REF->bounds.left;
			updateRect.right = REF->bounds.right;
			updateRect.top = REF->bounds.top + (drawStart * REF->lineHeight);
			
			if (REF->lineCount < oldLineCount)
				drawEnd = oldLineCount - 1;
			
			updateRect.bottom = REF->bounds.top + ((drawEnd+1) * REF->lineHeight);
		}
		else
			updateRect.SetEmpty();
		
		REF->textLength = UMemory::GetSize(REF->textHandle);
	}
	catch(...)
	{
		// if anything fails, fall back to no lines
		REF->lineCount = 0;
		REF->lineStartsValid = false;
		throw;
	}

	// if applicable, set selection to caret at end of inserted text
	if (inUpdateSelect)
	{
		_ETSetSelect(inRef, inOffset, inOffset, &selectRect);
		updateRect |= selectRect;
	}
	else
		_ETVerifySelect(inRef);

	// redraw
	if (REF->drawProc && !updateRect.IsEmpty())
		REF->drawProc(inRef, updateRect);
}

// inNewOffset is in terms of the text before it has been moved, returns the offset that the text is now located at
Uint32 UEditText::MoveText(TEditText inRef, Uint32 inOffset, Uint32 inTextSize, Uint32 inNewOffset, bool inUpdateSelect)
{
	Require(inRef);
	
	// bring values into range
	Uint32 n = REF->textLength;
	if (inOffset > n) inOffset = n;
	if (inNewOffset > n) inNewOffset = n;
	if (inOffset + inTextSize > n) inTextSize = n - inOffset;
	
	// this function could definitely be written more efficiently
	if (inTextSize && inOffset != inNewOffset)
	{
		// abort if move destination is inside the source
		if ((inNewOffset > inOffset) && (inNewOffset < (inOffset + inTextSize)))
			return inOffset;
		
		StPtr buf(inTextSize);
		
		UEditText::GetText(inRef, inOffset, inTextSize, buf, inTextSize);
		
		UEditText::DeleteText(inRef, inOffset, inTextSize, false);
		
		if (inNewOffset > inOffset)
			inNewOffset -= inTextSize;
		
		UEditText::InsertText(inRef, inNewOffset, buf, inTextSize, inUpdateSelect);
	}
	
	return inNewOffset;
}

bool UEditText::IsEmpty(TEditText inRef)
{
	Require(inRef);
	return REF->textLength == 0 || REF->textHandle == nil;
}

Uint32 UEditText::GetTextSize(TEditText inRef)
{
	Require(inRef);
	return REF->textLength;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UEditText::ClipCut(TEditText inRef)
{
	ClipCopy(inRef);
	DeleteText(inRef, REF->selectStart, REF->selectEnd - REF->selectStart, true);
}

void UEditText::ClipCopy(TEditText inRef)
{
	Require(inRef);
	
	Uint32 ofs = REF->selectStart;
	Uint32 s = REF->selectEnd - ofs;
	THdl textHandle = REF->textHandle;
	Uint32 textLength = REF->textLength;
		
	if (s && textHandle && textLength)
	{
		if (ofs >= textLength) return;
		if (ofs + s > textLength)
			s = textLength - ofs;

		Uint8 *textp;
		StHandleLocker locker(textHandle, (void*&)textp);
		UClipboard::SetData("text/plain", textp + ofs, s);
	}
}

void UEditText::ClipPaste(TEditText inRef)
{
	Require(inRef);
	
	THdl h = UClipboard::GetData("text/plain");
	
	if (h)
	{
		void *p = UMemory::Lock(h);
		try
		{
			ReplaceText(inRef, REF->selectStart, REF->selectEnd - REF->selectStart, p, UMemory::GetSize(h), true);
		}
		catch(...)
		{
			UMemory::Unlock(h);
			UMemory::Dispose(h);
			throw;
		}
		UMemory::Unlock(h);
		UMemory::Dispose(h);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UEditText::SetDrawHandler(TEditText inRef, TEditTextDrawProc inProc)
{
	Require(inRef);
	REF->drawProc = inProc;
}

TEditTextDrawProc UEditText::GetDrawHandler(TEditText inRef)
{
	Require(inRef);
	return REF->drawProc;
}

#if MACINTOSH

void UEditText::Draw(TEditText inRef, TImage inImage, const SRect& inUpdateRect, Uint16 /*inDepth*/)
{
	if (inRef == nil) return;
	
	// calculate everything
	_ETCalcLineStarts(inRef);
	_ETCalcSelectRgn(inRef);
	_ETCalcCaretRect(inRef);
		
	// draw
	UGraphics::SetInkMode(inImage, mode_Copy);
	if (REF->textHandle)
	{
		// lock everything else down
		Uint8 *textPtr;
		StHandleLocker lockText(REF->textHandle, (void*&)textPtr);
		Uint32 *lineStarts;
		StHandleLocker lockC(REF->lineStartsHdl, (void*&)lineStarts);
		
		Uint32 lineCount = REF->lineCount;
		SRect& bounds = REF->bounds;
		//Int32 top = bounds.top;
		Int32 lineHeight = REF->lineHeight;
		Uint32 i, endLine;
				
		// calc starting line - 1
		if (inUpdateRect.top > bounds.top)
		{
			i = (inUpdateRect.top - bounds.top) / lineHeight;
			if (i >= lineCount) return;
			//top += (lineHeight * i);
		}
		else
			i = 0;
		
		// calc ending line
		endLine = (((inUpdateRect.bottom <= bounds.bottom ? inUpdateRect.bottom : bounds.bottom) - bounds.top) / lineHeight) + 1;
		if (endLine > lineCount) endLine = lineCount;
		
		// set correct font etc
		UGraphics::SetPenSize(inImage, 1);
#if MACINTOSH
		UGraphics::SetInkMode(inImage, mode_Or);
#endif

		// draw selection
		if (REF->selectStart != REF->selectEnd)
		{
			TRegion selectRgn = REF->selectRgn;
			SRect selectRect;
			SColor hiliteCol;
			URegion::GetBounds(selectRgn, selectRect);
			if (selectRect.Intersects(inUpdateRect))
			{
				UUserInterface::GetHighlightColor(&hiliteCol);
				UGraphics::SetInkColor(inImage, hiliteCol);
				
				UGraphics::DrawRegion(inImage, 
									selectRgn, 
									REF->isActive ? fill_Solid : fill_Frame);
				// if windoze, find the lines intersecting with the region and 
				// draw them with reverse hilite color if isActive
				// and don't draw them later since the selection could be huge
			}
		}
		
		// draw each line
		//while (i < endLine)
		//{
		//	UGraphics::DrawTextLine(inImage, textPtr + lineStarts[i], lineStarts[i+1] - lineStarts[i], bounds, top, align);
		//	top += lineHeight;
		//	i++;
		//}
		UGraphics::SetFont(inImage, REF->font);
		UGraphics::DrawTextLines(inImage, bounds, textPtr, 0, lineStarts, i, endLine, lineHeight, REF->font->GetAlign());
		
		// draw caret
		if (REF->selectStart == REF->selectEnd)
		{
			if (REF->isCaretBlinkVis && REF->isActive)
			{
				UGraphics::SetInkMode(inImage, mode_Xor);
				UGraphics::SetInkColor(inImage, color_Black);
			#if WIN32	// stupid windoze won't draw filled shapes in Xor mode so we'll draw a line instead
				UGraphics::DrawLine(inImage, 
					SLine( REF->caretRect.left, 
						   REF->caretRect.top, 
						   REF->caretRect.left, 
						   REF->caretRect.bottom));
			#else
				UGraphics::FillRect(inImage, REF->caretRect);
			#endif
			}
		}
	}
	else
	{
		// no text, but can draw caret anyway
		if (REF->selectStart == REF->selectEnd && REF->isCaretBlinkVis && REF->isActive)
		{
			UGraphics::SetInkMode(inImage, mode_Xor);
			UGraphics::SetInkColor(inImage, color_Black);
		#if WIN32	// stupid windoze won't draw filled shapes in Xor mode so we'll draw a line instead
			UGraphics::DrawLine(inImage, SLine(REF->caretRect.left, REF->caretRect.top, REF->caretRect.left, REF->caretRect.bottom));
		#else
			UGraphics::FillRect(inImage, REF->caretRect);
		#endif
		}
	}
	
	// draw drag caret
	if (REF->showDragCaret)
	{
		UGraphics::SetInkMode(inImage, mode_Xor);
		UGraphics::SetInkColor(inImage, color_Black);
	#if WIN32	// stupid windoze won't draw filled shapes in Xor mode so we'll draw a line instead
		UGraphics::DrawLine(inImage, SLine(REF->dragCaretRect.left, REF->dragCaretRect.top, REF->dragCaretRect.left, REF->dragCaretRect.bottom));
	#else
		UGraphics::FillRect(inImage, REF->dragCaretRect);
	#endif
	}
	
	UGraphics::SetInkMode(inImage, mode_Copy);
}

#endif

#if WIN32

void UEditText::Draw(TEditText inRef, TImage inImage, const SRect& inUpdateRect, Uint16 /*inDepth*/)
{
	if (inRef == nil) return;
	
	// calculate everything
	_ETCalcLineStarts(inRef);
	_ETCalcSelectRgn(inRef);
	_ETCalcCaretRect(inRef);
		
	// draw
	UGraphics::SetInkMode(inImage, mode_Copy);
	if (REF->textHandle)
	{
		// lock everything else down
		Uint8 *textPtr;
		StHandleLocker lockText(REF->textHandle, (void*&)textPtr);
		Uint32 *lineStarts;
		StHandleLocker lockC(REF->lineStartsHdl, (void*&)lineStarts);
		
		Uint32 lineCount = REF->lineCount;
		SRect& bounds = REF->bounds;
		//Int32 top = bounds.top;
		Int32 lineHeight = REF->lineHeight;
		Uint32 i, endLine;
				
		// calc starting line - 1
		if (inUpdateRect.top > bounds.top)
		{
			i = (inUpdateRect.top - bounds.top) / lineHeight;
			if (i >= lineCount) return;
			//top += (lineHeight * i);
		}
		else
			i = 0;
		
		// calc ending line
		endLine = (((inUpdateRect.bottom <= bounds.bottom ? inUpdateRect.bottom : bounds.bottom) - bounds.top) / lineHeight) + 1;
		if (endLine > lineCount) endLine = lineCount;
		
		// set correct font etc
		UGraphics::SetPenSize(inImage, 1);
#if MACINTOSH
		UGraphics::SetInkMode(inImage, mode_Or);
#endif

		SColor inverseHilite;
		
		// draw selection
		if (REF->selectStart != REF->selectEnd)
		{
			TRegion selectRgn = REF->selectRgn;
			SRect selectRect;
			SColor hiliteCol;
			URegion::GetBounds(selectRgn, selectRect);
			if (selectRect.Intersects(inUpdateRect))
			{
				UUserInterface::GetHighlightColor(&hiliteCol, &inverseHilite);
				UGraphics::SetInkColor(inImage, hiliteCol);
				
				UGraphics::DrawRegion(inImage, selectRgn, REF->isActive ? fill_Solid : fill_Frame);
				// if windoze, find the lines intersecting with the region and draw them with reverse hilite color if isActive
				// and don't draw them later since the selection could be huge
			}
		}
		
		// draw each line
		//while (i < endLine)
		//{
		//	UGraphics::DrawTextLine(inImage, textPtr + lineStarts[i], lineStarts[i+1] - lineStarts[i], bounds, top, align);
		//	top += lineHeight;
		//	i++;
		//}
		UGraphics::SetFont(inImage, REF->font);
		
		// don't bother drawing in 5 steps unless necessary
		if (!REF->isActive || REF->selectStart == REF->selectEnd)
		{
			UGraphics::DrawTextLines(inImage, bounds, textPtr, 0, lineStarts, i, endLine, lineHeight, REF->font->GetAlign());
			
			// draw caret
			if (REF->isCaretBlinkVis && REF->isActive)
			{
				UGraphics::SetInkMode(inImage, mode_Xor);
				UGraphics::SetInkColor(inImage, color_Black);
			#if WIN32	// stupid windoze won't draw filled shapes in Xor mode so we'll draw a line instead
				UGraphics::DrawLine(inImage, SLine(REF->caretRect.left, REF->caretRect.top, REF->caretRect.left, REF->caretRect.bottom));
			#else
				UGraphics::FillRect(inImage, REF->caretRect);
			#endif
			}
		}
		else
		{
			/*  I could do this in 7 steps so I don't have overlap, but that might be slower...
			
					------------	1
					------------	1
					---=========	1-2
					============	3
					======------	3-4
					------------	5
					------------	5
					
			*/
		
			Uint32 selectStart, selectEnd;
			if (REF->selectStart > REF->selectEnd)
			{
				selectEnd = REF->selectStart;
				selectStart = REF->selectEnd;
			}
			else
			{
				selectStart = REF->selectStart;
				selectEnd = REF->selectEnd;
			}
			
			// draw the first chunk
			Uint32 lineA = _ETOffsetToLine(inRef, selectStart);
			
			if (lineA > endLine)
				lineA = endLine;
			
			UGraphics::DrawTextLines(inImage, bounds, textPtr, 0, lineStarts, i, lineA + 1, lineHeight, REF->font->GetAlign());
			
			// draw the second chunk
			SRect r(bounds);
			SPoint pt;
			
			SColor oldColor;
			inImage->GetInkColor(oldColor);
			
			inImage->SetInkColor(inverseHilite);
			
			
			Int32 length = lineStarts[lineA + 1] - selectStart;
			
			// if we're at the end of a line we need to strip out the CR
			if (*(textPtr + selectStart + length - 1) == '\r')
				length--;

			if (length > 0)
			{
				_ETOffsetToPoint(inRef, selectStart, pt);

				r.left += pt.h;
				r.top = pt.v;
				
				
				UGraphics::DrawText(inImage, r, textPtr + selectStart, length, nil, REF->font->GetAlign());
			}
			// region 3
			lineA++;
			
			Uint32 lineB = _ETOffsetToLine(inRef, selectEnd);
			
			if (lineB >= endLine)
				lineB = endLine - 1;
			
			if (lineA <= lineB)
				UGraphics::DrawTextLines(inImage, bounds, textPtr, 0, lineStarts, lineA, lineB + 1, lineHeight, REF->font->GetAlign());
			
			
			// region 4
			inImage->SetInkColor(oldColor);
			
			length = lineStarts[lineB + 1] - selectEnd;
			
			// if we're at the end of a line we need to strip out the CR
			if (*(textPtr + selectEnd + length - 1) == '\r')
				length--;
			
			if (length > 0)
			{
				_ETOffsetToPoint(inRef, selectEnd, pt);
				
				r.left = bounds.left + pt.h;
				r.top = pt.v;
				UGraphics::DrawText(inImage, r, textPtr + selectEnd, length, nil, REF->font->GetAlign());
			}
			
			// region 5
			lineB++;
			
			if (lineB < endLine)
				UGraphics::DrawTextLines(inImage, bounds, textPtr, 0, lineStarts, lineB, endLine, lineHeight, REF->font->GetAlign());
		}
	}
	else
	{
		// no text, but can draw caret anyway
		if (REF->selectStart == REF->selectEnd && REF->isCaretBlinkVis && REF->isActive)
		{
			UGraphics::SetInkMode(inImage, mode_Xor);
			UGraphics::SetInkColor(inImage, color_Black);
		#if WIN32	// stupid windoze won't draw filled shapes in Xor mode so we'll draw a line instead
			UGraphics::DrawLine(inImage, SLine(REF->caretRect.left, REF->caretRect.top, REF->caretRect.left, REF->caretRect.bottom));
		#else
			UGraphics::FillRect(inImage, REF->caretRect);
		#endif
		}
	}
	
	// draw drag caret
	if (REF->showDragCaret)
	{
		UGraphics::SetInkMode(inImage, mode_Xor);
		UGraphics::SetInkColor(inImage, color_Black);
	#if WIN32	// stupid windoze won't draw filled shapes in Xor mode so we'll draw a line instead
		UGraphics::DrawLine(inImage, SLine(REF->dragCaretRect.left, REF->dragCaretRect.top, REF->dragCaretRect.left, REF->dragCaretRect.bottom));
	#else
		UGraphics::FillRect(inImage, REF->dragCaretRect);
	#endif
	}
	
	UGraphics::SetInkMode(inImage, mode_Copy);
}
#endif



void UEditText::SetScreenDeltaHandler(TEditText inRef, TEditTextScreenDeltaProc inProc)
{
	Require(inRef);
	REF->screenDeltaProc = inProc;
}

TEditTextScreenDeltaProc UEditText::GetScreenDeltaHandler(TEditText inRef)
{
	Require(inRef);
	return REF->screenDeltaProc;
}

/* -------------------------------------------------------------------------- */
#pragma mark -


void UEditText::MouseDown(TEditText inRef, const SMouseMsgData& inInfo)
{
	Require(inRef);
	if (REF->textHandle == nil || REF->textLength == 0) return;
	
	if (REF->isSelectable)
	{
		Uint32 offset, clickTime;
		bool isMultipleClick;
		
		REF->isPossibleDrag = false;
		
		offset = _ETPointToOffset(inRef, inInfo.loc);

		if (inInfo.mods & modKey_Shortcut)
		{
			bool urlLaunched;
			Uint32 start, end;
			{
				void *textp;
				StHandleLocker lock(REF->textHandle, textp);
				start = REF->selectStart;
				end = REF->selectEnd;
				if ((offset < start) || (offset >= end))
					start = end = offset;
				urlLaunched = UTransport::LaunchURL(textp, REF->textLength, &start, &end);
			}
			if (urlLaunched) SetSelect(inRef, start, end);
			return;
		}

		// determine if multiple click
		clickTime = UDateTime::GetMilliseconds();
		isMultipleClick = ((clickTime < REF->clickTime + UMouse::GetDoubleClickTime()) && (offset == REF->clickOffset));
		REF->clickTime = clickTime;
		REF->clickOffset = offset;
		
		if (inInfo.mods & modKey_Shift)
		{
			if (offset > REF->selectStart)
				REF->anchor = REF->selectStart;
			else
				REF->anchor = REF->selectEnd;
		}
		else
		{
			if (isMultipleClick)
			{
				REF->clickCount++;

				// a double/triple click creates an anchor-word/line
				if (REF->clickCount > 1)
					_ETFindLine(inRef, offset, REF->anchorStart, REF->anchorEnd);
				else
					_ETFindWord(inRef, offset, REF->anchorStart, REF->anchorEnd);

				offset = REF->anchorStart;
			}
			else
			{
				if (UDragAndDrop::IsAvailable() && ((offset >= REF->selectStart) && (offset < REF->selectEnd)))
				{
					REF->dragStartPt = inInfo.loc;
					REF->isPossibleDrag = REF->isMouseDown = true;
					return;
				}
				
				REF->clickCount = 0;
				REF->anchor = offset;
			}
		}

		REF->isMouseDown = true;
		UEditText::MouseMove(inRef, inInfo);
	}
}

void UEditText::MouseUp(TEditText inRef, const SMouseMsgData& inInfo)
{
	Require(inRef);
	REF->isMouseDown = false;
	
	if (!REF->isEditable) _ETSetCaretBlinkVis(inRef, false);
	
	// if clicked in selected text but didn't move (drag), then set selection to caret at mouseloc
	if (REF->isPossibleDrag)
	{
		REF->isPossibleDrag = false;
		Uint32 offset = _ETPointToOffset(inRef, inInfo.loc);
		SetSelect(inRef, offset, offset);
	}
}

void UEditText::MouseEnter(TEditText inRef, const SMouseMsgData& inInfo)
{
	Require(inRef);
	REF->origMouseImageID = UMouse::GetImage();
	UMouse::SetImage(mouseImage_Text);
	REF->isMouseWithin = true;
	try
	{
		MouseMove(inRef, inInfo);
	}
	catch(...)
	{
		REF->isMouseWithin = false;
		throw;
	}
}

void UEditText::MouseLeave(TEditText inRef, const SMouseMsgData& inInfo)
{
	Require(inRef);
	REF->isMouseWithin = false;
	UMouse::SetImage(REF->origMouseImageID);
	MouseMove(inRef, inInfo);
}

void UEditText::MouseMove(TEditText inRef, const SMouseMsgData& inInfo)
{
	Require(inRef);
	_ETCalcSelectRgn(inRef);
	
	if (REF->isMouseWithin)
	{
		if (!REF->isMouseDown && REF->selectStart != REF->selectEnd && REF->selectRgn->IsPointWithin(inInfo.loc))
			UMouse::SetImage(mouseImage_Arrow);
		else
			UMouse::SetImage(mouseImage_Text);
	}
	
	if (REF->isMouseDown)
	{
		Uint32 offset, rangeStart, rangeEnd;
		
		if (REF->isPossibleDrag)
		{
			REF->isPossibleDrag = REF->isMouseDown = false;
			_ETDrag(inRef, REF->dragStartPt);
			return;
		}
		
		offset = _ETPointToOffset(inRef, inInfo.loc);

		// if we're selecting words or lines, pin offset to a word or line boundary
		if (REF->clickCount > 0)
		{
			if (REF->clickCount > 1)
				_ETFindLine(inRef, offset, rangeStart, rangeEnd);
			else
				_ETFindWord(inRef, offset, rangeStart, rangeEnd);

			// choose the word/line boundary and the anchor that are farthest away from each other
			if (offset > REF->anchorStart)
			{
				REF->anchor = REF->anchorStart;
				offset = rangeEnd;
			}
			else
			{
				offset = rangeStart;
				REF->anchor = REF->anchorEnd;
			}
		}

		// set the selection range from anchor point to current offset
		SetSelect(inRef, REF->anchor, offset);
	}
}

bool UEditText::KeyChar(TEditText inRef, const SKeyMsgData& inInfo)
{
	Require(inRef);

	Uint8 keyChar = (Uint8)inInfo.keyChar;
	Uint8 keyCode = (Uint8)inInfo.keyCode;
	Uint32 start = REF->selectStart;
	Uint32 end = REF->selectEnd;
	Uint32 textLength = REF->textLength;
	bool isEditable = REF->isEditable;
	bool isSelectable = REF->isSelectable;
	SPoint pt;
	Uint32 n;
		
	switch (keyCode)
	{
		// delete key
		case key_Delete:
		{
			if (isEditable)
			{
				if (start == end)
				{
					// backspace
					if (start != 0)
						DeleteText(inRef, start-1, 1, true);
				}
				else
					DeleteText(inRef, start, end - start, true);
			}
		}
		break;
		
		// clear key
		case key_nClear:
		{
			if (isEditable && start != end)
				DeleteText(inRef, start, end - start, true);
		}
		break;
		
		// forward delete key
		case key_Del:
		{
			if (isEditable)
			{
				if (start == end)
					DeleteText(inRef, start, 1, true);
				else
					DeleteText(inRef, start, end - start, true);
			}
		}
		break;
		
		// move left key
		case key_Left:
		{
			if (isSelectable)
			{
				if (inInfo.mods & (modKey_Command|modKey_Control))
				{
					Uint32 *lsp;
					StHandleLocker lsLock(REF->lineStartsHdl, (void*&)lsp);
					start = lsp[_ETOffsetToLine(inRef, start)];
				}
				else
				{
					if (start != 0 && (start == end || (inInfo.mods & modKey_Shift)))
						start--;
				}
				
				if (!(inInfo.mods & modKey_Shift))
					end = start;
				
				SetSelect(inRef, start, end);
			}
		}
		break;
		
		// move right key
		case key_Right:
		{
			if (isSelectable)
			{
				if (inInfo.mods & (modKey_Command|modKey_Control))
				{
					pt.h = REF->bounds.right + 100;
					pt.v = REF->bounds.top + (_ETOffsetToLine(inRef, end) * REF->lineHeight);
					end = _ETPointToOffset(inRef, pt);
				}
				else
				{
					if (start == end || inInfo.mods & modKey_Shift)
						end++;
				}
				
				if (!(inInfo.mods & modKey_Shift))
					start = end;
				
				SetSelect(inRef, start, end);
			}
		}
		break;
		
		// move up key
		case key_Up:
		{
			if (isSelectable)
			{
				if (inInfo.mods & (modKey_Command|modKey_Control))
					start = 0;
				else
				{
					if (start == end || (inInfo.mods & modKey_Shift))
					{
						_ETOffsetToPoint(inRef, start, pt);
						pt.v--;
						n = _ETPointToOffset(inRef, pt);;
						if (start == n) start = 0;
						else start = n;
					}
				}
				
				if (!(inInfo.mods & modKey_Shift))
					end = start;
				
				SetSelect(inRef, start, end);
			}
		}
		break;
		
		// move down key
		case key_Down:
		{
			if (isSelectable)
			{
				if (inInfo.mods & (modKey_Command|modKey_Control))
					end = max_Uint32;
				else
				{
					if (start == end || (inInfo.mods & modKey_Shift))
					{
						_ETOffsetToPoint(inRef, end, pt);
						pt.v += REF->lineHeight;
						n = _ETPointToOffset(inRef, pt);
						if (end == n) end = max_Uint32;
						else end = n;
					}
				}
				
				if (!(inInfo.mods & modKey_Shift))
					start = end;
				
				SetSelect(inRef, start, end);
			}
		}
		break;
		
		// home key
		case key_Home:
		{
			if (isSelectable)
				SetSelect(inRef, 0, 0);
		}
		break;
		
		// end key
		case key_End:
		{
			if (isSelectable)
				SetSelect(inRef, max_Uint32, max_Uint32);
		}
		break;
		
		// any other key
		default:
		{
			if (inInfo.mods & (modKey_Command|modKey_Control))
			{
				if (isSelectable && !isEditable)
				{
					if (keyChar == 'c' || keyChar == 'C')
					{
						ClipCopy(inRef);
						return true;
					}
					
					if (keyChar == 'a' || keyChar == 'A')
					{
						SetSelect(inRef, 0, max_Uint32);
						return true;
					}
				}
				else if (isEditable)
				{
					if (keyChar == 'c' || keyChar == 'C')
					{
						ClipCopy(inRef);
						return true;
					}
					
					if (keyChar == 'x' || keyChar == 'X')
					{
						ClipCut(inRef);
						return true;
					}
					
					if (keyChar == 'v' || keyChar == 'V')
					{
						ClipPaste(inRef);
						return true;
					}
					
					if (keyChar == 'a' || keyChar == 'A')
					{
						SetSelect(inRef, 0, max_Uint32);
						return true;
					}
				}
			}
			else if (isEditable)
			{
				if (UText::IsPrinting(keyChar) || keyChar == '\r')
				{
					if (start == end)
						InsertText(inRef, start, &keyChar, sizeof(keyChar), true);
					else
						ReplaceText(inRef, start, end - start, &keyChar, sizeof(keyChar), true);
						
					return true;
				}
			}
		}
		
		return false;
	}
	
	return true;
}

void UEditText::DragEnter(TEditText inRef, const SDragMsgData& inInfo)
{
	Require(inRef);
	REF->isDragWithin = true;

	_ETSetCaretBlinkVis(inRef, false);
	
	UEditText::DragMove(inRef, inInfo);
	
}

void UEditText::DragLeave(TEditText inRef, const SDragMsgData& /* inInfo */)
{
	Require(inRef);
	REF->isDragWithin = false;
	
	// remove the drag caret if it is shown
	if (REF->showDragCaret)
	{
		SRect r = REF->dragCaretRect;
		REF->showDragCaret = false;
		if (REF->drawProc) REF->drawProc(inRef, r);
	}
}

void UEditText::DragMove(TEditText inRef, const SDragMsgData& inInfo)
{
	Require(inRef);
	
	if (REF->isEditable && inInfo.drag->HasFlavor('TEXT'))
	{
		Uint32 ofs = _ETPointToOffset(inRef, inInfo.loc);
	
		if (IsSelfDrag(inRef, inInfo.drag))
		{
			inInfo.drag->SetDragAction(inInfo.mods & modKey_Option ? dragAction_Copy : dragAction_Move);
			if (ofs >= REF->selectStart && ofs <= REF->selectEnd)		// if is self drag and drop destination is in source
			{
				// remove the drag caret if it is shown
				if (REF->showDragCaret)
				{
					SRect r = REF->dragCaretRect;
					REF->showDragCaret = false;
					if (REF->drawProc) REF->drawProc(inRef, r);
				}
			}
		}
		else
			inInfo.drag->SetDragAction(dragAction_Copy);
		
		_ETSetDragInsertPos(inRef, ofs);
	}
	else
		inInfo.drag->SetDragAction(dragAction_None);
}

bool UEditText::Drop(TEditText inRef, const SDragMsgData& inInfo)
{
	Uint32 i, c, s, ofs, item;
	TDrag drag;
	void *p;
	
	Require(inRef);
	
	if (REF->showDragCaret)
	{
		if (IsSelfDrag(inRef, inInfo.drag) && ((inInfo.mods & modKey_Option) == 0))	// if is self drag and option key NOT down
		{
			ofs = REF->dragInsertOffset;
			if (ofs > REF->textLength) ofs = REF->textLength;

			s = REF->selectEnd - REF->selectStart;
			ofs = UEditText::MoveText(inRef, REF->selectStart, s, ofs, false);
			
			UEditText::SetSelect(inRef, ofs, ofs + s);
			
			return true;
		}
		else
		{
			// allocate handle we'll use to munge the text items together
			THdl h = UMemory::NewHandle();
			scopekill(THdlObj, h);
			ofs = 0;
			
			// extract and munge text items together
			drag = inInfo.drag;
			c = drag->GetItemCount();
			for (i=1; i<=c; i++)
			{
				item = drag->GetItem(i);
				s = drag->GetFlavorDataSize(item, 'TEXT');
				if (s)
				{
					UMemory::SetSize(h, ofs + s);
					
					StHandleLocker lock(h, p);
					
					drag->GetFlavorData(item, 'TEXT', BPTR(p) + ofs, s);
					ofs += s;
				}
			}
			
			// insert the munged text into the TEditText
			s = UMemory::GetSize(h);
			if (s)
			{
				ofs = REF->dragInsertOffset;
				if (ofs > REF->textLength) ofs = REF->textLength;
				
				StHandleLocker lock(h, p);
				
				UEditText::InsertText(inRef, ofs, p, s, false);
				UEditText::SetSelect(inRef, ofs, ofs + s);
				
				return true;
			}
		}
	}
	
	return false;
}

/*
bool UEditText::CanAcceptDrop(TEditText inRef, TDrag inDrag)
{
	if (inRef) return REF->isEditable && inDrag->HasFlavor('TEXT');
	return false;
}
*/

bool UEditText::IsSelfDrag(TEditText inRef, TDrag inDrag)
{	
	if (inRef && inDrag && REF->curDrag) 
		return inDrag->Equals(REF->curDrag);
	
	return false;
}

void UEditText::BlinkCaret(TEditText inRef)
{
	Require(inRef);
	
	_ETCalcLineStarts(inRef);
	_ETCalcCaretRect(inRef);
	
	if (REF->isActive && REF->selectStart == REF->selectEnd && REF->isEditable)		// if have caret
	{
		// toggle caret visible
		REF->isCaretBlinkVis = !REF->isCaretBlinkVis;
		
		// draw caret
		if (REF->drawProc)
		{
			SRect r = REF->caretRect;
			REF->drawProc(inRef, r);
		}
	}
}

// returns false is no caret
bool UEditText::GetCaretRect(TEditText inRef, SRect& outRect)
{
	Require(inRef);
	
	if (REF->selectStart == REF->selectEnd)
	{
		_ETCalcLineStarts(inRef);
		_ETCalcCaretRect(inRef);
		
		outRect = REF->caretRect;
		return true;
	}
	
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UEditText::LowCalcLines(TImage inImage, THdl inText, THdl outOffsets, Uint32 inMaxWidth, Uint32& outLineCount)
{
	Uint8 *textPtr, *linePtr, *endPtr;
	Uint32 *lineStarts;
	Uint32 textSize, linesAllocated, lineCount;
	
	// sanity check
	Require(inText && outOffsets && inMaxWidth);
	
	// if no text, we want just one line
	textSize = UMemory::GetSize(inText);
	if (textSize == 0)
	{
		UMemory::ReallocateClear(outOffsets, 16);
		outLineCount = 1;
		return;
	}
	
	// preallocate space for line starts
	linesAllocated = textSize / 50;						// guess how many lines there will be
	if (linesAllocated < 64) linesAllocated = 64;		// preallocate at least 64 lines
	UMemory::Reallocate(outOffsets, linesAllocated * sizeof(Uint32));

	// get ptrs to text and offsets
	StHandleLocker lockA(inText, (void*&)textPtr);
	lineStarts = (Uint32 *)UMemory::Lock(outOffsets);
	
	try
	{
		// prepare to calculate
		lineStarts[0] = 0;
		lineCount = 1;
		linePtr = textPtr;
		endPtr = textPtr + textSize;
		
		// store end of each line (start of the next)
		while (linePtr < endPtr)
		{
			if (lineCount >= linesAllocated)
			{
				linesAllocated += 256;
				UMemory::Unlock(outOffsets);
				lineStarts = nil;
				
				UMemory::SetSize(outOffsets, linesAllocated * sizeof(Uint32));

				lineStarts = (Uint32 *)UMemory::Lock(outOffsets);
			}

			linePtr += UGraphics::GetTextLineBreak(inImage, linePtr, endPtr - linePtr, inMaxWidth);
			
			lineStarts[lineCount] = linePtr - textPtr;
			lineCount++;
		}
		
		// if text ends with return, add an extra blank line
		if (endPtr[-1] == '\r')
		{
			if (lineCount >= linesAllocated)
			{
				linesAllocated += 4;
				UMemory::Unlock(outOffsets);
				lineStarts = nil;
				
				UMemory::SetSize(outOffsets, linesAllocated * sizeof(Uint32));
				
				lineStarts = (Uint32 *)UMemory::Lock(outOffsets);
			}
			
			lineStarts[lineCount] = lineStarts[lineCount-1];
			lineCount++;
		}
		
		// remove unused space from offsets
		UMemory::Unlock(outOffsets);
		lineStarts = nil;
		UMemory::SetSize(outOffsets, (lineCount+16) * sizeof(Uint32));

		// determine number of lines
		outLineCount = lineCount - 1;
	}
	catch(...)
	{
		if (lineStarts) UMemory::Unlock(outOffsets);
		throw;
	}
}

// returns true if need to redraw lines outDrawStart to outDrawEnd inclusive (0-based)
bool UEditText::LowInsert(TImage inImage, THdl ioText, THdl ioOffsets, Uint32 inMaxWidth, Uint32& ioLineCount, Uint32 inOffset, const void *inNewText, Uint32 inNewTextSize, Uint32 *outDrawStart, Uint32 *outDrawEnd)
{
	Uint32 textSize, line, linesAllocated, minIndex, maxIndex, newStart, lineCount;
	Uint32 *lineStarts, *lineStartPtr, *endLineStartPtr;
	Uint8 *textPtr, *linePtr, *endPtr;
	bool dontDrawLineAbove = false;
	
	// sanity checks
	Require(ioText && ioOffsets && inMaxWidth && ioLineCount);
	if (inNewTextSize == 0) return false;
	
	// verify line count
	lineCount = ioLineCount;
	linesAllocated = UMemory::GetSize(ioOffsets) / sizeof(Uint32);
	if (lineCount > linesAllocated)
	{
		DebugBreak("LowInsert - line count exceeds allocated count");
		Fail(errorType_Misc, error_Param);
	}
	
	// bring offset into range
	textSize = UMemory::GetSize(ioText);
	if (inOffset > textSize)
		inOffset = textSize;
	
	// get ptr to line starts
	StHandleLocker lockA(ioOffsets, (void*&)lineStarts);
	
	// determine starting line *before* changing text length
	if (inOffset >= textSize)
		line = lineCount;
	else
	{
		minIndex = 0;
		maxIndex = lineCount;
		
		// do a fast binary search through the line array
		while (minIndex < maxIndex)
		{
			line = (minIndex + maxIndex) >> 1;
			
			if (inOffset >= lineStarts[line])
			{
				if (inOffset < lineStarts[line + 1])
					break;
				else
					minIndex = line + 1;
			}
			else
			{
				maxIndex = line;
			}
		}
	}
	
	// insert the text
	UMemory::Insert(ioText, inOffset, inNewText, inNewTextSize);
	textSize += inNewTextSize;
	
	// bump line starts
	lineStartPtr = lineStarts + line + 1;
	endLineStartPtr = lineStarts + lineCount + 1;
	if (lineStartPtr < endLineStartPtr)
	{
		while (lineStartPtr != endLineStartPtr)
		{
			*lineStartPtr += inNewTextSize;
			lineStartPtr++;
		}
	}
	
	// get ptr to the text
	StHandleLocker lockB(ioText, (void*&)textPtr);
	
	// prepare to update line offsets
	minIndex = lineCount = line ? line-1 : line;	// line above may have been affected
	maxIndex = -1;									// going to be the line to draw down to
	linePtr = textPtr + lineStarts[lineCount];
	endPtr = textPtr + textSize;
	lineCount++;
	
	// update each line
	while (linePtr < endPtr)
	{
		if (lineCount >= linesAllocated)
		{
			if (inNewTextSize < 256)
				linesAllocated += 32;
			else if (inNewTextSize < 2048)
				linesAllocated += 128;
			else
				linesAllocated += 256;
			
			UMemory::Unlock(ioOffsets);
			try
			{
				UMemory::SetSize(ioOffsets, linesAllocated * sizeof(Uint32));
			}
			catch(...)
			{
				UMemory::Lock(ioOffsets);
				throw;
			}
			lineStarts = (Uint32 *)UMemory::Lock(ioOffsets);
		}

		linePtr += UGraphics::GetTextLineBreak(inImage, linePtr, endPtr - linePtr, inMaxWidth);
		
		newStart = linePtr - textPtr;
		
		if (newStart == lineStarts[lineCount])		// if the line start we calc'd matches the existing one
		{
			if (line && lineCount == minIndex+1)	// if line above and we just calc'd for line above
				dontDrawLineAbove = true;			// don't draw the line above b/c it is the same
			else
			{
				maxIndex = lineCount - 1;		// draw only to this point
				lineCount = ioLineCount + 1;	// we're stopping the recalc, so same number of lines
				break;
			}
		}
		
		lineStarts[lineCount] = newStart;
		lineCount++;
	}
		
	// if text ends in CR and we recalculated right to the end, add extra blank line
	if (endPtr[-1] == '\r' && maxIndex == -1)
	{
		if (lineCount >= linesAllocated)
		{
			linesAllocated += 4;
			UMemory::Unlock(ioOffsets);
			try
			{
				UMemory::SetSize(ioOffsets, linesAllocated * sizeof(Uint32));
			}
			catch(...)
			{
				UMemory::Lock(ioOffsets);
				throw;
			}
			lineStarts = (Uint32 *)UMemory::Lock(ioOffsets);
		}
		
		lineStarts[lineCount] = lineStarts[lineCount-1];
		lineCount++;
	}
	
	// output number of lines
	ioLineCount = lineCount - 1;
	
	// determine line to draw down to
	if (maxIndex == -1)					// if we recalculated right to the end
		maxIndex = ioLineCount - 1;		// then draw right to the end

	// output lines to draw
	if (outDrawStart)
		*outDrawStart = dontDrawLineAbove ? line : minIndex;
	if (outDrawEnd)
		*outDrawEnd = maxIndex;
	
	// need redraw, so return true
	return true;
}


bool UEditText::LowDelete(TImage inImage, THdl ioText, THdl ioOffsets, Uint32 inMaxWidth, Uint32& ioLineCount, Uint32 inOffset, Uint32 inDeleteSize, Uint32 *outDrawStart, Uint32 *outDrawEnd)
{
	Uint32 lineCount, linesAllocated, textSize, startLine, endLine, minIndex, maxIndex, n;
	Uint32 *lineStarts, *lineStartPtr, *endLineStartPtr;
	Uint8 *textPtr, *linePtr, *endPtr;
	bool dontDrawLineAbove = false;

	// sanity checks
	Require(ioText && ioOffsets && inMaxWidth);
	if (inDeleteSize == 0 || ioLineCount == 0) return false;
	
	// bring offset and size into range
	textSize = UMemory::GetSize(ioText);
	if (inOffset > textSize) return false;
	if (inOffset + inDeleteSize > textSize)
		inDeleteSize = textSize - inOffset;		// remove to end
	if (textSize == 0 || inDeleteSize == 0)  return false;

	// check if there would be no text left
	if (textSize == inDeleteSize)
	{
		UMemory::ReallocateClear(ioOffsets, 16);
		UMemory::Reallocate(ioText, 0);
		ioLineCount = 1;
		if (outDrawStart) *outDrawStart = 0;
		if (outDrawEnd) *outDrawEnd = 0;
		return true;
	}

	// verify line count
	lineCount = ioLineCount;
	linesAllocated = UMemory::GetSize(ioOffsets) / sizeof(Uint32);
	if (lineCount > linesAllocated)
	{
		DebugBreak("LowDelete - line count exceeds allocated count");
		Fail(errorType_Misc, error_Param);
	}
	
	// get ptr to line starts
	StHandleLocker lockA(ioOffsets, (void*&)lineStarts);
	
	// determine starting line *before* changing text length
	if (inOffset >= textSize)
		startLine = lineCount;
	else
	{
		minIndex = 0;
		maxIndex = lineCount;
		
		// do a fast binary search through the line array
		while (minIndex < maxIndex)
		{
			startLine = (minIndex + maxIndex) >> 1;
			
			if (inOffset >= lineStarts[startLine])
			{
				if (inOffset < lineStarts[startLine + 1])
					break;
				else
					minIndex = startLine + 1;
			}
			else
			{
				maxIndex = startLine;
			}
		}
	}
	
	// determine ending line
	n = inOffset + inDeleteSize;
	if (n >= textSize)
		endLine = lineCount;
	else
	{
		minIndex = 0;
		maxIndex = lineCount;
		
		// do a fast binary search through the line array
		while (minIndex < maxIndex)
		{
			endLine = (minIndex + maxIndex) >> 1;
			
			if (n >= lineStarts[endLine])
			{
				if (n < lineStarts[endLine + 1])
					break;
				else
					minIndex = endLine + 1;
			}
			else
			{
				maxIndex = endLine;
			}
		}
	}
	
	// delete the text
	UMemory::Remove(ioText, inOffset, inDeleteSize);
	textSize -= inDeleteSize;
	
	// adjust line starts such that remaining text stays on the same lines
	lineStartPtr = lineStarts + startLine + 1;
	endLineStartPtr = lineStarts + endLine + 1;
	if (lineStartPtr < endLineStartPtr)
	{
		while (lineStartPtr != endLineStartPtr)
		{
			*lineStartPtr -= (*lineStartPtr - inOffset);
			lineStartPtr++;
		}
	}
	lineStartPtr = lineStarts + endLine + 1;
	endLineStartPtr = lineStarts + lineCount + 1;
	if (lineStartPtr < endLineStartPtr)
	{
		while (lineStartPtr != endLineStartPtr)
		{
			*lineStartPtr -= inDeleteSize;
			lineStartPtr++;
		}
	}
	
	// get ptr to the text
	StHandleLocker lockB(ioText, (void*&)textPtr);
	
	// prepare to update line offsets
	minIndex = lineCount = startLine ? startLine-1 : startLine;	// line above may have been affected
	maxIndex = -1;												// going to be the line to draw down to
	linePtr = textPtr + lineStarts[lineCount];
	endPtr = textPtr + textSize;
	lineCount++;
	
	// update each line
	while (linePtr < endPtr)
	{
		if (lineCount >= linesAllocated)
		{
			linesAllocated += 8;
			UMemory::Unlock(ioOffsets);
			try
			{
				UMemory::SetSize(ioOffsets, linesAllocated * sizeof(Uint32));
			}
			catch(...)
			{
				UMemory::Lock(ioOffsets);
				throw;
			}
			lineStarts = (Uint32 *)UMemory::Lock(ioOffsets);
		}

		linePtr += UGraphics::GetTextLineBreak(inImage, linePtr, endPtr - linePtr, inMaxWidth);
		
		n = linePtr - textPtr;
		
		if (n == lineStarts[lineCount])		// if the line start we calc'd matches the existing one
		{
			if (startLine && lineCount == minIndex+1)	// if line above and we just calc'd for line above
				dontDrawLineAbove = true;				// don't draw the line above b/c it is the same
			else
			{
				maxIndex = lineCount - 1;				// draw only to this point
				lineCount = ioLineCount + 1;			// we're stopping the recalc, so same number of lines
				break;
			}
		}
		
		lineStarts[lineCount] = n;
		lineCount++;
	}
	
	// handle special case of text ending in CR
	if (endPtr[-1] == '\r')
	{
		if (maxIndex == -1)
		{
			// we recalculated right to the end, so add extra blank line
			if (lineCount >= linesAllocated)
			{
				linesAllocated += 4;
				UMemory::Unlock(ioOffsets);
				try
				{
					UMemory::SetSize(ioOffsets, linesAllocated * sizeof(Uint32));
				}
				catch(...)
				{
					UMemory::Lock(ioOffsets);
					throw;
				}
				lineStarts = (Uint32 *)UMemory::Lock(ioOffsets);
			}
			lineStarts[lineCount] = lineStarts[lineCount-1];
			lineCount++;
		}
	}
	else
	{
		// remove any incorrect zero-length line
		if (lineCount > 1 && lineStarts[lineCount-1] == lineStarts[lineCount-2])
			lineCount--;
	}

	// output number of lines
	ioLineCount = lineCount - 1;
	
	// determine line to draw down to
	if (maxIndex == -1)					// if we recalculated right to the end
		maxIndex = ioLineCount - 1;		// then draw right to the end

	// output lines to draw
	if (outDrawStart)
		*outDrawStart = dontDrawLineAbove ? startLine : minIndex;
	if (outDrawEnd)
		*outDrawEnd = maxIndex;
	
	// shrink offsets hdl if necessary
	if (linesAllocated - lineCount > 256)
	{
		UMemory::Unlock(ioOffsets);
		try
		{
			UMemory::SetSize(ioOffsets, (lineCount+32) * sizeof(Uint32));
		}
		catch(...)
		{
			UMemory::Lock(ioOffsets);
			throw;
		}
		UMemory::Lock(ioOffsets);
	}
	
	// need redraw, so return true
	return true;
}

bool UEditText::LowReplace(TImage inImage, THdl ioText, THdl ioOffsets, Uint32 inMaxWidth, Uint32& ioLineCount, Uint32 inOffset, Uint32 inExistingSize, const void *inNewText, Uint32 inNewTextSize, Uint32 *outDrawStart, Uint32 *outDrawEnd)
{
	Uint32 textSize, line, linesAllocated, minIndex, maxIndex, newStart, lineCount, endLine, startLine, n;
	Uint32 *lineStarts, *lineStartPtr, *endLineStartPtr;
	Uint8 *textPtr, *linePtr, *endPtr;
	bool dontDrawLineAbove = false;
	
	// sanity checks
	Require(ioText && ioOffsets && inMaxWidth && ioLineCount);
	
	// bring offset and sizes into range
	textSize = UMemory::GetSize(ioText);
	if (inOffset > textSize)
		inOffset = textSize;
	if (inOffset + inExistingSize > textSize)
		inExistingSize = textSize - inOffset;

	// check if not really a replace operation
	if (inExistingSize == 0)
		return LowInsert(inImage, ioText, ioOffsets, inMaxWidth, ioLineCount, inOffset, inNewText, inNewTextSize, outDrawStart, outDrawEnd);
	if (inNewTextSize == 0)
		return LowDelete(inImage, ioText, ioOffsets, inMaxWidth, ioLineCount, inOffset, inExistingSize, outDrawStart, outDrawEnd);

	// verify line count
	lineCount = ioLineCount;
	linesAllocated = UMemory::GetSize(ioOffsets) / sizeof(Uint32);
	if (lineCount > linesAllocated)
	{
		DebugBreak("LowReplace - line count exceeds allocated count");
		Fail(errorType_Misc, error_Param);
	}
	
	// get ptr to line starts
	StHandleLocker lockA(ioOffsets, (void*&)lineStarts);

	// determine starting line
	if (inOffset >= textSize)
		startLine = lineCount;
	else
	{
		minIndex = 0;
		maxIndex = lineCount;
		
		// do a fast binary search through the line array
		while (minIndex < maxIndex)
		{
			startLine = (minIndex + maxIndex) >> 1;
			
			if (inOffset >= lineStarts[startLine])
			{
				if (inOffset < lineStarts[startLine + 1])
					break;
				else
					minIndex = startLine + 1;
			}
			else
			{
				maxIndex = startLine;
			}
		}
	}

	// update line starts
	if (inNewTextSize != inExistingSize)
	{
		// determine offset where the insert or delete starts
		if (inNewTextSize > inExistingSize)
			n = inOffset + inExistingSize;		// insert
		else
			n = inOffset + inNewTextSize;		// delete
		
		// determine line where the insert or delete starts
		if (n >= textSize)
			line = lineCount;
		else
		{
			minIndex = 0;
			maxIndex = lineCount;
			
			// do a fast binary search through the line array
			while (minIndex < maxIndex)
			{
				line = (minIndex + maxIndex) >> 1;
				
				if (n >= lineStarts[line])
				{
					if (n < lineStarts[line + 1])
						break;
					else
						minIndex = line + 1;
				}
				else
				{
					maxIndex = line;
				}
			}
		}

		// update the line starts
		if (inNewTextSize > inExistingSize)
		{
			// insert, so bump line starts
			n = inNewTextSize - inExistingSize;
			lineStartPtr = lineStarts + line + 1;
			endLineStartPtr = lineStarts + lineCount + 1;
			if (lineStartPtr < endLineStartPtr)
			{
				while (lineStartPtr != endLineStartPtr)
				{
					*lineStartPtr += n;
					lineStartPtr++;
				}
			}
		}
		else
		{
			// determine ending line
			n = inOffset + (inExistingSize - inNewTextSize);
			if (n >= textSize)
				endLine = lineCount;
			else
			{
				minIndex = 0;
				maxIndex = lineCount;
				
				// do a fast binary search through the line array
				while (minIndex < maxIndex)
				{
					endLine = (minIndex + maxIndex) >> 1;
					
					if (n >= lineStarts[endLine])
					{
						if (n < lineStarts[endLine + 1])
							break;
						else
							minIndex = endLine + 1;
					}
					else
					{
						maxIndex = endLine;
					}
				}
			}

			// delete, keep remaining text stays on the same lines
			lineStartPtr = lineStarts + line + 1;
			endLineStartPtr = lineStarts + endLine + 1;
			if (lineStartPtr < endLineStartPtr)
			{
				while (lineStartPtr != endLineStartPtr)
				{
					*lineStartPtr -= (*lineStartPtr - inOffset);
					lineStartPtr++;
				}
			}
			n = inExistingSize - inNewTextSize;
			lineStartPtr = lineStarts + endLine + 1;
			endLineStartPtr = lineStarts + lineCount + 1;
			if (lineStartPtr < endLineStartPtr)
			{
				while (lineStartPtr != endLineStartPtr)
				{
					*lineStartPtr -= n;
					lineStartPtr++;
				}
			}
		}
	}
	
	// replace the text
	UMemory::Replace(ioText, inOffset, inExistingSize, inNewText, inNewTextSize);
	textSize = UMemory::GetSize(ioText);
	
	// get ptr to the text
	StHandleLocker lockB(ioText, (void*&)textPtr);
	
	// prepare to update line offsets
	minIndex = lineCount = startLine ? startLine-1 : startLine;	// line above may have been affected
	maxIndex = -1;									// going to be the line to draw down to
	linePtr = textPtr + lineStarts[lineCount];
	endPtr = textPtr + textSize;
	lineCount++;
	
	// update each line
	while (linePtr < endPtr)
	{
		if (lineCount >= linesAllocated)
		{
			if (inNewTextSize < 256)
				linesAllocated += 32;
			else if (inNewTextSize < 2048)
				linesAllocated += 128;
			else
				linesAllocated += 256;
			
			UMemory::Unlock(ioOffsets);
			try
			{
				UMemory::SetSize(ioOffsets, linesAllocated * sizeof(Uint32));
			}
			catch(...)
			{
				UMemory::Lock(ioOffsets);
				throw;
			}
			lineStarts = (Uint32 *)UMemory::Lock(ioOffsets);
		}

		linePtr += UGraphics::GetTextLineBreak(inImage, linePtr, endPtr - linePtr, inMaxWidth);
		
		newStart = linePtr - textPtr;
		
		if (newStart == lineStarts[lineCount])			// if the line start we calc'd matches the existing one
		{
			if (startLine && lineCount == minIndex+1)	// if line above and we just calc'd for line above
				dontDrawLineAbove = true;				// don't draw the line above b/c it is the same
			else
			{
				maxIndex = lineCount - 1;				// draw only to this point
				lineCount = ioLineCount + 1;			// we're stopping the recalc, so same number of lines
				break;
			}
		}
		
		lineStarts[lineCount] = newStart;
		lineCount++;
	}
		
	// handle special case of text ending in CR
	if (endPtr[-1] == '\r')
	{
		if (maxIndex == -1)
		{
			// we recalculated right to the end, so add extra blank line
			if (lineCount >= linesAllocated)
			{
				linesAllocated += 4;
				UMemory::Unlock(ioOffsets);
				try
				{
					UMemory::SetSize(ioOffsets, linesAllocated * sizeof(Uint32));
				}
				catch(...)
				{
					UMemory::Lock(ioOffsets);
					throw;
				}
				lineStarts = (Uint32 *)UMemory::Lock(ioOffsets);
			}
			lineStarts[lineCount] = lineStarts[lineCount-1];
			lineCount++;
		}
	}
	else
	{
		// remove any incorrect zero-length line
		if (lineCount > 1 && lineStarts[lineCount-1] == lineStarts[lineCount-2])
			lineCount--;
	}

	// output number of lines
	ioLineCount = lineCount - 1;
	
	// determine line to draw down to
	if (maxIndex == -1)					// if we recalculated right to the end
		maxIndex = ioLineCount - 1;		// then draw right to the end

	// output lines to draw
	if (outDrawStart)
		*outDrawStart = dontDrawLineAbove ? startLine : minIndex;
	if (outDrawEnd)
		*outDrawEnd = maxIndex;
	
	// shrink offsets hdl if necessary
	if (linesAllocated - lineCount > 256)
	{
		UMemory::Unlock(ioOffsets);
		UMemory::SetSize(ioOffsets, (lineCount+32) * sizeof(Uint32));
	}
	
	// need redraw, so return true
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static void _ETCalcLineStarts(TEditText inRef)
{
	if (REF->lineStartsValid)
		return;
	
	try
	{
		TImage img = UGraphics::GetDummyImage();
		img->SetFont(REF->font);
		
		REF->textLength = UMemory::GetSize(REF->textHandle);
		UEditText::LowCalcLines(img, REF->textHandle, (THdl)REF->lineStartsHdl, REF->bounds.GetWidth(), REF->lineCount);
	}
	catch(...)
	{
		// if anything fails, fall back to no lines
		REF->lineCount = 0;
		REF->lineStartsValid = false;
		throw;
	}
	
	REF->lineStartsValid = true;
}

static void _ETCalcLineHeight(TEditText inRef)
{
	TImage img = UGraphics::GetDummyImage();
	UGraphics::SetFont(img, REF->font);
	REF->lineHeight = UGraphics::GetFontLineHeight(img);
}

static void _ETCalcSelectRgn(TEditText inRef)
{
	if (!REF->selectRgnValid)
	{
		_ETGetRangeRgn(inRef, REF->selectStart, REF->selectEnd, REF->selectRgn);
		REF->selectRgnValid = true;
	}
}

static void _ETCalcCaretRect(TEditText inRef)
{
	if (!REF->caretRectValid)
	{
		if (REF->selectStart == REF->selectEnd)
		{
			SRect& caretRect = REF->caretRect;
			
			if (REF->textHandle && REF->textLength)
				_ETOffsetToPoint(inRef, REF->selectStart, caretRect.TopLeft());
			else
				caretRect.TopLeft() = REF->bounds.TopLeft();
			
			caretRect.bottom = caretRect.top + REF->lineHeight;
			caretRect.right = caretRect.left + 1;
		}
		
		REF->caretRectValid = true;
	}
}

static Int32 _ETGetLineHoriz(TImage inImage, const void *inText, Uint32 inLength, const SRect& inBounds, Int16 inAlign)
{
	Int32 h;

	switch (inAlign)
	{
		case textAlign_Right:
			inLength = UText::GetVisibleLength(inText, inLength);
			h = inBounds.right - UGraphics::GetTextWidth(inImage, inText, inLength);
			break;
		case textAlign_Center:
			inLength = UText::GetVisibleLength(inText, inLength);
			h = inBounds.left + ((inBounds.GetWidth() - UGraphics::GetTextWidth(inImage, inText, inLength)) / 2);
			break;
		default:	// textAlign_Left
			h = inBounds.left;
			break;
	}

	return h;
}
 
// given a byte offset into the text, returns the corresponding line index (0-based)
static Uint32 _ETOffsetToLine(TEditText inRef, Uint32 offset)
{
	_ETCalcLineStarts(inRef);
	
	if (offset >= REF->textLength)
		return REF->lineCount;
	
	Uint32 *lineStarts;
	StHandleLocker lock(REF->lineStartsHdl, (void*&)lineStarts);
	
	Uint32 minIndex, maxIndex, index;

	minIndex = 0;
	maxIndex = REF->lineCount;
	
	// do a fast binary search through the line array
	while (minIndex < maxIndex)
	{
		index = (minIndex + maxIndex) >> 1;
		
		if (offset >= lineStarts[index])
		{
			if (offset < lineStarts[index + 1])
				break;
			else
				minIndex = index + 1;
		}
		else
		{
			maxIndex = index;
		}
	}

	return index;
}

// given a byte offset into the text, find the top-left point of the glyph
static void _ETOffsetToPoint(TEditText inRef, Uint32 offset, SPoint& outPoint)
{
	_ETCalcLineStarts(inRef);

	if (REF->lineCount == 0 || REF->textLength == 0)
	{
		outPoint.h = REF->bounds.left;
		outPoint.v = REF->bounds.top;
		return;
	}

	// get ptrs to data
	Uint8 *textPtr;
	StHandleLocker lockB(REF->textHandle, (void*&)textPtr);
	Uint32 *lineStarts;
	StHandleLocker lockC(REF->lineStartsHdl, (void*&)lineStarts);

	// set dummy image to the correct font
	TImage img = UGraphics::GetDummyImage();
	img->SetFont(REF->font);
	
	// find the line on which the glyph lies
	Uint32 line;
	if (offset >= REF->textLength)
	{
		line = REF->lineCount - 1;
		offset = REF->textLength;
	}
	else
		line = _ETOffsetToLine(inRef, offset);
	
	// get info about the line
	Uint32 lineStart = lineStarts[line];
	Uint8 *linePtr = textPtr + lineStart;
	Uint32 lineSize = lineStarts[line+1] - lineStart;

	// calculate the vertical coordinate
	outPoint.v = REF->bounds.top + (line * REF->lineHeight);

	// calculate the horizontal coordinate
	outPoint.h = _ETGetLineHoriz(img, linePtr, lineSize, REF->bounds, REF->font->GetAlign());
	outPoint.h += UGraphics::GetTextWidth(img, linePtr, offset - lineStart);
}

// given a point, returns the corresponding line index (0-based)
static Uint32 _ETPointToLine(TEditText inRef, const SPoint& inPt)
{
	_ETCalcLineStarts(inRef);
		
	Int32 vert = inPt.v - REF->bounds.top;
	if (vert <= 0) return 0;
	
	Uint32 line = (Uint32)vert / REF->lineHeight;
	
	if (line >= REF->lineCount)
		line = REF->lineCount - 1;
	
	return line;
}

static Uint32 _ETPointToOffset(TEditText inRef, const SPoint& inPt)
{
	if (REF->textHandle == nil) return 0;
	_ETCalcLineStarts(inRef);
	
	// get ptrs to data
	Uint8 *textPtr;
	StHandleLocker lockB(REF->textHandle, (void*&)textPtr);
	Uint32 *lineStarts;
	StHandleLocker lockC(REF->lineStartsHdl, (void*&)lineStarts);
	
	// determine what line the point is in
	Uint32 line = _ETPointToLine(inRef, inPt);
	Uint32 lineStart = lineStarts[line];
	Uint8 *linePtr = textPtr + lineStart;
	Uint32 lineSize = lineStarts[line+1] - lineStart;

	// if point if outside of bounds to left, offset is start of line
	if (inPt.h < REF->bounds.left)
		return lineStart;

	// set dummy image to the correct font
	TImage img = UGraphics::GetDummyImage();
	img->SetFont(REF->font);
	
	// calc offset
	bool isLeftSide;
	Uint32 offset = lineStart + UGraphics::WidthToChar(img, linePtr, lineSize, inPt.h - REF->bounds.left, &isLeftSide);

	// don't let offset go onto the next line
	if (offset >= lineStart + lineSize)
	{
		if (isLeftSide && offset != lineStart + UText::GetVisibleLength(linePtr, lineSize))
			offset--;
	}
	
	return offset;
}

static void _ETVerifySelect(TEditText inRef)
{
	Uint32 len = REF->textLength;
	
	if (REF->selectStart > len)
	{
		REF->selectStart = len;
		REF->selectRgnValid = REF->caretRectValid = false;
	}
	
	if (REF->selectEnd > len)
	{
		REF->selectEnd = len;
		REF->selectRgnValid = REF->caretRectValid = false;
	}
}

#if 0
static Char16 _ETGetChar(TEditText inRef, Uint32 inOffset)
{
	if (inOffset >= REF->textLength)
		return 0;

	#error
	return *(((Uint8 *)*REF->textHandle) + inOffset);
}
#endif

// calculates a region that includes the characters at <inStart>, <inEnd> and all chars in-between
static void _ETGetRangeRgn(TEditText inRef, Uint32 inStart, Uint32 inEnd, TRegion outRgn)
{
	_ETCalcLineStarts(inRef);

	if (inStart == inEnd)
	{
		URegion::SetEmpty(outRgn);
		return;
	}
	else if (inStart > inEnd)
	{
		Uint32 tmp = inEnd;
		inEnd = inStart;
		inStart = tmp;
	}
	
	Uint32 textLength = REF->textLength;
	if (inEnd > textLength)
		inEnd = textLength;
	
	SRect bounds = REF->bounds;
	SPoint firstPoint, lastPoint;
	Int32 firstLineHeight, lastLineHeight;
	SRect r;
	
	// get pixel locations corresponding to start and end characters
	_ETOffsetToPoint(inRef, inStart, firstPoint);
	_ETOffsetToPoint(inRef, inEnd, lastPoint);
	firstLineHeight = lastLineHeight = REF->lineHeight;
	
	// build the region
	if (firstPoint.v == lastPoint.v)	// if selection range encompasses only one line
	{
		r.left = firstPoint.h;
		r.top = firstPoint.v;
		r.right = lastPoint.h;
		r.bottom = lastPoint.v + lastLineHeight;
		URegion::SetRect(outRgn, r);
	}
	else								// selection range encompasses more than one line
	{
		// add the first line
		r.left = firstPoint.h;
		r.top = firstPoint.v;
		r.right = bounds.right;
		r.bottom = firstPoint.v + firstLineHeight;
		URegion::SetRect(outRgn, r);

		// add lines between the first and the last line
		if (firstPoint.v + firstLineHeight < lastPoint.v)		// if any lines between
		{
			// add all the lines in-between
			r.left = bounds.left;
			r.top = firstPoint.v + firstLineHeight;
			r.right = bounds.right;
			r.bottom = lastPoint.v;
			URegion::AddRect(outRgn, r);
		}

		// add the last line
		r.left = bounds.left;
		r.top = lastPoint.v;
		r.right = lastPoint.h;
		r.bottom = lastPoint.v + lastLineHeight;
		URegion::AddRect(outRgn, r);
	}
}

static void _ETGetRangeRect(TEditText inRef, Uint32 inStart, Uint32 inEnd, SRect& outRect)
{
	_ETCalcLineStarts(inRef);

	if (inStart == inEnd)
	{
		outRect.SetEmpty();
		return;
	}
	else if (inStart > inEnd)
	{
		Uint32 tmp = inEnd;
		inEnd = inStart;
		inStart = tmp;
	}
	
	Uint32 textLength = REF->textLength;
	if (inEnd > textLength)
		inEnd = textLength;
	
	SRect bounds = REF->bounds;
	SPoint firstPoint, lastPoint;
	Int32 firstLineHeight, lastLineHeight;
	
	// get pixel locations corresponding to start and end characters
	_ETOffsetToPoint(inRef, inStart, firstPoint);
	_ETOffsetToPoint(inRef, inEnd, lastPoint);
	firstLineHeight = lastLineHeight = REF->lineHeight;
	
	if (firstPoint.v == lastPoint.v)	// if selection range encompasses only one line
	{
		outRect.left = firstPoint.h;
		outRect.top = firstPoint.v;
		outRect.right = lastPoint.h;
		outRect.bottom = lastPoint.v + lastLineHeight;
	}
	else								// selection range encompasses more than one line
	{
		outRect.right = bounds.right;
		outRect.top = firstPoint.v;
			
		if (lastPoint.h == bounds.left && lastPoint.v == firstPoint.v + firstLineHeight)
			outRect.left = firstPoint.h;
		else
			outRect.left = bounds.left;
					
		if (lastPoint.h == bounds.left)
			outRect.bottom = lastPoint.v;
		else
			outRect.bottom = lastPoint.v + lastLineHeight;
	}
}

static void _ETReorder(Uint32& a, Uint32& b)
{
	if (a > b)
	{
		Uint32 temp = a;
		a = b;
		b = temp;
	}
}

static void _ETSetDragInsertPos(TEditText inRef, Uint32 inOffset)
{
	/*******************************************************************
	This function could be made more perfect by handling the situation
	where the text changes (eg font, size, style, or just the actual
	text content) and the drag caret rect needs to be recalculated.
	Currently it's not recalculated.  It really should be.
	********************************************************************/
	
	SRect newCaretRect, r;
	
	// calculate line starts if necessary
	_ETCalcLineStarts(inRef);
	
	// bring into range
	if (inOffset > REF->textLength)
		inOffset = REF->textLength;

	// check if no change
	if (REF->showDragCaret && REF->dragInsertOffset == inOffset)
		return;

	// calculate new drag caret rect
	if (REF->textHandle && REF->textLength)
		_ETOffsetToPoint(inRef, inOffset, newCaretRect.TopLeft());
	else
		newCaretRect.TopLeft() = REF->bounds.TopLeft();
	newCaretRect.bottom = newCaretRect.top + REF->lineHeight;
	newCaretRect.right = newCaretRect.left + 2;

	// store new drag insert offset
	REF->dragInsertOffset = inOffset;

	// calc update rect
	if (REF->showDragCaret)
	{
		// we have an existing caret, so the redraw rect is the union of the old and new caret rects
		r = REF->dragCaretRect;
		r |= newCaretRect;
		REF->dragCaretRect = newCaretRect;
		if (REF->drawProc) REF->drawProc(inRef, r);
	}
	else
	{
		// no existing caret, so the redraw rect is the rect of the new caret
		REF->showDragCaret = true;
		REF->dragCaretRect = newCaretRect;
		if (REF->drawProc) REF->drawProc(inRef, newCaretRect);
	}
}

// returns whether or not outUpdateRect needs to be drawn
static bool _ETSetSelect(TEditText inRef, Uint32 inSelectStart, Uint32 inSelectEnd, SRect *outUpdateRect)
{
	Uint32 n, oldSelectStart, oldSelectEnd;
	
	// get ptr to data
	_ETCalcLineStarts(inRef);
	
	// ensure that end is greater than or equal to start
	if (inSelectStart > inSelectEnd)
	{
		n = inSelectStart;
		inSelectStart = inSelectEnd;
		inSelectEnd = n;
	}
	
	// check if no change
	if (inSelectStart == REF->selectStart && inSelectEnd == REF->selectEnd)
	{
		if (outUpdateRect) outUpdateRect->SetEmpty();
		return false;
	}
	
	// bring into range
	n = REF->textLength;
	if (inSelectStart > n)
		inSelectStart = n;
	if (inSelectEnd > n)
		inSelectEnd = n;
	
	oldSelectStart = REF->selectStart;
	oldSelectEnd = REF->selectEnd;
	if (oldSelectStart == oldSelectEnd)
		_ETCalcCaretRect(inRef);		// we will need the old caret rect later on
	
	REF->selectStart = inSelectStart;
	REF->selectEnd = inSelectEnd;
	REF->selectRgnValid = false;
	
	if (inSelectStart == inSelectEnd)
		REF->caretRectValid = false;
	
	// call draw callback
	if (outUpdateRect)
	{
		SRect updateRect, r;
		
		if (inSelectStart == inSelectEnd || oldSelectStart == oldSelectEnd)
		{
			if (oldSelectStart == oldSelectEnd)
			{
				if (REF->isCaretBlinkVis)			// don't need to redraw old caret if not visible
					updateRect = REF->caretRect;
				else
					updateRect.SetEmpty();
			}
			else
				_ETGetRangeRect(inRef, oldSelectStart, oldSelectEnd, updateRect);
			
			if (inSelectStart == inSelectEnd)
			{
				// we're setting the selection to a caret, so immediately show it
				if (REF->isEditable || REF->isMouseDown)
				{
					REF->isCaretBlinkVis = true;
					REF->caretTime = UDateTime::GetMilliseconds() + UText::GetCaretTime();
					
					_ETCalcCaretRect(inRef);
					r = REF->caretRect;
				}
			}
			else
				_ETGetRangeRect(inRef, inSelectStart, inSelectEnd, r);
			
			updateRect |= r;
		}
		else
		{ 
			// if we're active (selection is not outline), we can avoid redrawing already selected areas
			if (REF->isActive)
			{
				// invert the exclusive-OR between the old range and the new range
				_ETReorder(oldSelectStart, inSelectStart);
				_ETReorder(oldSelectEnd, inSelectEnd);
				_ETReorder(oldSelectEnd, inSelectStart);
			}
			
			// calc union of old and new selection rects
			_ETGetRangeRect(inRef, oldSelectStart, oldSelectEnd, updateRect);
			_ETGetRangeRect(inRef, inSelectStart, inSelectEnd, r);
			updateRect |= r;
		}
		
		if (updateRect.bottom > REF->bounds.bottom)
			updateRect.bottom = REF->bounds.bottom;

		*outUpdateRect = updateRect;
	}
	
	return true;
}

static void _ETRedrawAll(TEditText inRef)
{
	if (REF->drawProc)
	{
		SRect bounds = REF->bounds;
		REF->drawProc(inRef, bounds);
	}
}

static void _ETFindWord(TEditText inRef, Uint32 inOffset, Uint32& outStart, Uint32& outEnd)
{
	void *textp;
	StHandleLocker locker(REF->textHandle, textp);
	UText::FindWord(textp, REF->textLength, 0, inOffset, outStart, outEnd);
	outEnd += outStart;
}

static void _ETFindLine(TEditText inRef, Uint32 inOffset, Uint32& outStart, Uint32& outEnd)
{
	Uint32 line;
	
	_ETCalcLineStarts(inRef);

	if (inOffset >= REF->textLength)
		line = REF->lineCount - 1;
	else
		line = _ETOffsetToLine(inRef, inOffset);
	
	
	Uint32 *lineStarts;
	StHandleLocker lock(REF->lineStartsHdl, (void*&)lineStarts);
	
	outStart = lineStarts[line];
	outEnd = lineStarts[line+1];
}

static void _ETSetCaretBlinkVis(TEditText inRef, bool inVis)
{
	if (REF->isCaretBlinkVis != (inVis != 0))
	{
		if (inVis && !REF->isEditable && !REF->isMouseDown) return;

		REF->isCaretBlinkVis = (inVis != 0);
		if (REF->drawProc)
		{
			_ETCalcCaretRect(inRef);
			_ETCalcSelectRgn(inRef);
			SRect r = REF->caretRect;
			REF->drawProc(inRef, r);
		}
	}
}

static TImage _ETGetImage(TEditText inRef, SPoint& outScreenDelta)
{
	TImage img;
	SRect localBounds, globalBounds, r;
	Int32 hd, vd;

	if (REF->selectStart == REF->selectEnd) return nil;
	
	UEditText::GetSelectRect(inRef, localBounds);
	REF->screenDeltaProc(inRef, hd, vd);
	globalBounds = localBounds;
	globalBounds.Move(hd, vd);
	
	img = UGraphics::NewImage(globalBounds);
	
	try
	{
		UGraphics::LockImage(img);
		
		r = globalBounds;
		r.Move(-r.left, -r.top);
		UGraphics::SetInkColor(img, color_White);
		UGraphics::FillRect(img, r);

		UGraphics::SetOrigin(img, SPoint(-localBounds.left, -localBounds.top));
		UEditText::Draw(inRef, img, localBounds, 8);
		UGraphics::ResetOrigin(img);
		
		UGraphics::UnlockImage(img);
	}
	catch(...)
	{
		UGraphics::DisposeImage(img);
		throw;
	}
	
	outScreenDelta.h = globalBounds.left;
	outScreenDelta.v = globalBounds.top;
	return img;
}

static void _ETDrag(TEditText inRef, const SPoint& inMouseLoc)
{
	TDrag drag;
	TRegion rgn, imgRgn;
	TImage img;
	SPoint mouseLoc, imgPt;
	SRect r;
	Int32 hd, vd;
	Uint32 dragArea;
	
	if ((REF->selectStart == REF->selectEnd) || (REF->screenDeltaProc == nil)) return;
	_ETCalcSelectRgn(inRef);
	
	// get global mouse loc
	REF->screenDeltaProc(inRef, hd, vd);
	mouseLoc.h = inMouseLoc.h + hd;
	mouseLoc.v = inMouseLoc.v + vd;

	// create new drag object
	drag = UDragAndDrop::New();
	scopekill(TDragObj, drag);
	
	// add text to drag object
	{
		void *textp;
		StHandleLocker locker(REF->textHandle, textp);
		drag->AddTextFlavor(1, BPTR(textp) + REF->selectStart, REF->selectEnd - REF->selectStart);
	}
	
	UMouse::SetImage(mouseImage_Arrow);
	
	rgn = imgRgn = nil;
	img = nil;
	
	try
	{
		rgn = URegion::New(REF->selectRgn);
		rgn->Move(hd, vd);
		rgn->SetOutline(2, 2);
		
		REF->selectRgn->GetBounds(r);
		dragArea = (Uint32)r.GetWidth() * (Uint32)r.GetHeight();
		
#if MACINTOSH
		if ((dragArea <= 65536) && UDragAndDrop::HasTranslucentDrag())
		{
			imgRgn = URegion::New(REF->selectRgn);
			imgRgn->GetBounds(r);
			imgRgn->Move(-r.left, -r.top);
			
			img = _ETGetImage(inRef, imgPt);
			drag->SetImage(img, imgRgn, imgPt, dragImageStyle_Standard);
		}
#endif

		REF->curDrag = drag;
		drag->Track(mouseLoc, rgn);
		REF->curDrag = nil;
	}
	catch(...)
	{
		REF->curDrag = nil;
		URegion::Dispose(rgn);
		URegion::Dispose(imgRgn);
#if MACINTOSH
		UGraphics::DisposeImage(img);
#endif
		throw;
	}

	URegion::Dispose(rgn);
	URegion::Dispose(imgRgn);
#if MACINTOSH
	UGraphics::DisposeImage(img);	
#endif

	if (REF->isEditable && drag->DropLocationIsTrash())
	{
		UEditText::DeleteText(inRef, REF->selectStart, REF->selectEnd - REF->selectStart, true);
	}
}

