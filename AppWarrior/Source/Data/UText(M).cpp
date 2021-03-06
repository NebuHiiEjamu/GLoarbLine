#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
/*
Utilities for working with text.

Functions for determining the type of a character are:

	Function				Characters
	--------				----------
	
	IsAlphaNumeric()		a-z, A-Z, 0-9
	IsAlphabetic()			a-z, A-Z
	IsControl()				0x00-0x1F, 0x7F
	IsDigit()				0-9
	IsGraph()				0x21-0x7E
	IsLower()				a-z
	IsUpper()				A-Z
	IsPrinting()			0x20-0x7E except 0x7F
	IsPunctuation()			#$%*()"'\`~:;,<>/?\|[]{}-_=+
	IsSpace()				space, form feed, new-line, return, h and v tab
	IsHex()					0-9, A-F, a-f
*/

#include "UText.h"

#include <Script.h>
#include <TextUtils.h>

/* -------------------------------------------------------------------------- */

Uint32 UText::GetVisibleLength(const void *inText, Uint32 inLength, Uint32 /* inEncoding */)
{
	return ::VisibleLength((Ptr)inText, inLength);
}

/*
 * GetCaretTime() returns the suggested difference in milliseconds that should exist
 * between blinks of the caret (usually a vertical bar marking the insertion point)
 * in editable text.
 */
Uint32 UText::GetCaretTime()
{
	// ask OS for caret time in ticks, and then pull a swifty to convert to milliseconds :)
	return ((Uint32)::GetCaretTime() * 50) / 3;
}

bool UText::FindWord(const void *inText, Uint32 inTextSize, Uint32 /* inEncoding */, Uint32 inOffset, Uint32& outWordOffset, Uint32& outWordSize)
{
	OffsetTable wordBreaks;
	Uint32 start, end;
	
	start = 0;
	if (inTextSize == 0)
	{
		outWordOffset = outWordSize = 0;
		return false;
	}
	if (inOffset >= inTextSize)
	{
		outWordOffset = inTextSize - 1;
		outWordSize = 0;
		return false;
	}
	
	// adjust if too big for FindWordBreaks to handle
	if (inTextSize > max_Int16)
	{
		if (inOffset < 30000)
			inTextSize = max_Int16;
		else
		{
			start = inOffset - 250;
			end = inOffset + 250;
			if (end > inTextSize) end = inTextSize;
			inText = BPTR(inText) + start;
			inTextSize = end - start;
		}
	}
	
	::FindWordBreaks((Ptr)inText, inTextSize, inOffset - start, true, nil, wordBreaks, 0);

	outWordOffset = start + (Uint32)wordBreaks[0].offFirst;
	outWordSize = wordBreaks[0].offSecond - wordBreaks[0].offFirst;
	
	return true;
}



#endif
