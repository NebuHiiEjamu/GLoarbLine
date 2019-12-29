/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CFont.h"


HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  CFont                                                       [public]
// ---------------------------------------------------------------------
// Copy Constructor

CFont::CFont (const CFont& other)
	: mColor(other.mColor), mSize(other.mSize)
		, mItalic(other.mItalic), mBold(other.mBold)
		, mUnderline(other.mUnderline)
{}

HL_End_Namespace_BigRedH
