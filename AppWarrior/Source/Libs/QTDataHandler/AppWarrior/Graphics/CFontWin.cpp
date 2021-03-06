#include "AW.h"
#include "CFontWin.h"
#include "CGraphicsPortWin.h"
#include "StString.h"
#include "CGraphicsScopeWin.h"

#define NULL_CHECK(inValue, inTask) \
if (inValue == NULL) \
	THROW_GRAPHICS_ (inTask, eTranslate, ::GetLastError())

HL_Begin_Namespace_BigRedH

CFont* CFont::sSystemFont = new CFontWin();

// ---------------------------------------------------------------------
//  CFontWin                                                    [public]
// ---------------------------------------------------------------------
// Constructor; creates the system default font.

CFontWin::CFontWin ()
{
	::HFONT sysFont = static_cast<HFONT>(::GetStockObject(SYSTEM_FONT));
	NULL_CHECK(sysFont,eFontConstructor);
	
	HDC tempDC = ::GetDC(::GetDesktopWindow());
	NULL_CHECK(tempDC,eFontConstructor);
	StWindowDC tempDCScope(tempDC);
	
	COLORREF winTextColor;
	winTextColor = ::GetTextColor(tempDC);
	UGraphicsWin::COLORREFToCColor(winTextColor, &mColor);
	
	::TEXTMETRIC textInfo;
	
	{	StGDISelectWin fontSelection(tempDC, sysFont); 
		NULL_CHECK(::GetTextMetrics(tempDC, &textInfo), eFontConstructor);
	}
		
	mSize = textInfo.tmHeight;
	mItalic = textInfo.tmItalic;
	mUnderline = textInfo.tmUnderlined;
	mBold = textInfo.tmWeight >= FW_BOLD;
}

// ---------------------------------------------------------------------
//  CFontWin                                                    [public]
// ---------------------------------------------------------------------
// Copy constructor.

CFontWin::CFontWin (const CFontWin& other)
	: CFont(other)
{
	mMetrics = other.mMetrics;
}


// ---------------------------------------------------------------------
//  Clone                                              [virtual][public]
// ---------------------------------------------------------------------
// Virtual copy constructor, so fonts can be copied in platform
// independant ways.

CFont*
CFontWin::Clone () const
{
	return new CFontWin(*this);
}


// ---------------------------------------------------------------------
//  ~CFontWin                                          [virtual][public]
// ---------------------------------------------------------------------
// Destructor.

CFontWin::~CFontWin()
{
}

// ---------------------------------------------------------------------
//  GetMetrics                                         [virtual][public]
// ---------------------------------------------------------------------
// Get the CFontMetrics object for this font.

const CFontMetrics&
CFontWin::GetMetrics() const
{
	::TEXTMETRIC textInfo;
	HDC tempDC = ::GetDC(::GetDesktopWindow());

	NULL_CHECK(tempDC,eFontGetMetrics);
	{
		StWindowDC tempDCScope(tempDC);
		StFontScoper fontScope(tempDC, *this);
		NULL_CHECK(::GetTextMetrics(tempDC, &textInfo),eFontGetMetrics);
	}
	
	mMetrics.mAscent = textInfo.tmAscent;
	mMetrics.mDescent = textInfo.tmDescent;
	mMetrics.mMaxCharacterWidth = textInfo.tmMaxCharWidth;
	mMetrics.mLeading = textInfo.tmExternalLeading;
	
	return mMetrics;
}
	
// ---------------------------------------------------------------------
//  MeasureText                                        [virtual][public]
// ---------------------------------------------------------------------
// Measure the extents of a given one-line string.	
CRect
CFontWin::MeasureText(const CString& inText, const TProxy<CGraphicsPort>& inPort) const
{
	const CGraphicsPortWin* winPort = dynamic_cast<const CGraphicsPortWin*>(inPort.GetPtr());
	StCStyleString osString(inText);
	SIZE textSize;
	StFontScoper fontScope(winPort->mPS.hdc, *this);	
	NULL_CHECK( GetTextExtentPoint32(winPort->mPS.hdc, osString
		,static_cast<int>(inText.length()), &textSize), eFontMeasureText );
	return CRect(CPoint(0,-GetMetrics().GetAscent()), CSize(textSize.cx, textSize.cy));
}


// ---------------------------------------------------------------------
//  MakeFont                                                   [private]
// ---------------------------------------------------------------------
// Builds the windows HFONT object for this font.
::HFONT
CFontWin::MakeFont() const
{
	::HFONT theFont = ::CreateFont(static_cast<int>(mSize), 0, 0, 0
							, (IsBold())?FW_BOLD:FW_NORMAL
							, static_cast<unsigned long>((IsItalic())?TRUE:FALSE)
							, static_cast<unsigned long>((IsUnderline())?TRUE:FALSE)
							, FALSE
							, DEFAULT_CHARSET
							, OUT_DEFAULT_PRECIS
							, CLIP_DEFAULT_PRECIS
							, DEFAULT_QUALITY
							, DEFAULT_PITCH | FF_DONTCARE
							, NULL);
	NULL_CHECK(theFont, eFontRealization);
	return theFont;
}

// ---------------------------------------------------------------------
//  SetColor                                           [virtual][public]
// ---------------------------------------------------------------------
// Set the color of this font.
void
CFontWin::SetColor (const CColor& inColor)
{
	mColor = inColor;
}

// ---------------------------------------------------------------------
//  SetSize                                            [virtual][public]
// ---------------------------------------------------------------------
// Set the size of the font.
void
CFontWin::SetSize (int inSize)
{
	mSize = inSize;
}

// ---------------------------------------------------------------------
//  SetItalic                                          [virtual][public]
// ---------------------------------------------------------------------
// Set the italic state of the font.
void
CFontWin::SetItalic (bool inState)
{
	mItalic = inState;
}

// ---------------------------------------------------------------------
//  SetBold                                            [virtual][public]
// ---------------------------------------------------------------------
// Set the bold state of the font.
void
CFontWin::SetBold (bool inState)
{
	mBold = inState;
}

// ---------------------------------------------------------------------
//  SetUnderline                                       [virtual][public]
// ---------------------------------------------------------------------
// Set the underline state of the font.
void
CFontWin::SetUnderline (bool inState)
{
	mUnderline = inState;
}

// ---------------------------------------------------------------------
//  StFontScoper                                                [public]
// ---------------------------------------------------------------------
// Scope the realization of a font.

CFontWin::StFontScoper::StFontScoper (HDC inDC, const CFont& inFont)
							: mDC(inDC)
{
	const CFontWin& winFont = dynamic_cast<const CFontWin&>(inFont);
	mObj = ::SelectObject(mDC, winFont.MakeFont());
	mOldBkMode = ::GetBkMode(mDC);
	NULL_CHECK(::SetBkMode(mDC, TRANSPARENT), eFontRealization);
	mOldTextColor = ::GetTextColor(mDC);
	COLORREF winColor;
	UGraphicsWin::CColorToCOLORREF(inFont.GetColor(), &winColor);
	if (::SetTextColor(mDC, winColor) == CLR_INVALID)
		THROW_GRAPHICS_ (eFontRealization, eTranslate, ::GetLastError());
}

// ---------------------------------------------------------------------
//  ~StFontScoper                                               [public]
// ---------------------------------------------------------------------
// Scope the realization of a font.			
CFontWin::StFontScoper::~StFontScoper ()
{
	if (::SetTextColor(mDC, mOldTextColor) == CLR_INVALID)
		THROW_GRAPHICS_ (eFontRealization, eTranslate, ::GetLastError());
	NULL_CHECK(::SetBkMode(mDC, mOldBkMode), eFontRealization);
	::DeleteObject(::SelectObject(mDC, mObj));
}
				
HL_End_Namespace_BigRedH