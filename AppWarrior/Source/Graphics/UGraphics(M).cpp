#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
/*
Functions for drawing and working with graphics.

-- See Also --

GrafTypes		Data types especially for graphics, eg, rectangles and points
URegion			Mathematically defined shapes

*/

#include "UGraphics.h"
#include "UMemory.h"

#include <QuickDraw.h>
#include <QDOffscreen.h>
#include <Windows.h>
#include <TextUtils.h>
#include <Fonts.h>
#include <FixMath.h>
#include <Resources.h>
#include <Errors.h>
#include <Icons.h>
#include <ColorPicker.h>

struct SImage 
{
	GWorldPtr gw;
	SPoint virtualOrigin;
	Uint32 lockCount;
};

//#define REF		((SImage *)inImage)

// mac error handling
void _FailMacError(Int16 inMacError, const Int8 *inFile, Uint32 inLine);
inline void _CheckMacError(Int16 inMacError, const Int8 *inFile, Uint32 inLine) { if (inMacError) _FailMacError(inMacError, inFile, inLine); }
#if DEBUG
	#define FailMacError(id)		_FailMacError(id, __FILE__, __LINE__)
	#define CheckMacError(id)		_CheckMacError(id, __FILE__, __LINE__)
#else
	#define FailMacError(id)		_FailMacError(id, nil, 0)
	#define CheckMacError(id)		_CheckMacError(id, nil, 0)
#endif

/*
 * Structures
 */

struct SPicture 
{
	Picture macPict;
};

struct SPattern 
{
	PixPat macPat;
};

// IF CHANGE FORMAT OF THIS, need to update UFontDesc::New() and UFontDesc::Unflatten()
struct SFontDescObj 
{
	Int16 macNum;
	Uint16 macStyle;
	Uint32 size, effect, align, customVal;
	SColor color, customColor;
	Uint8 locked;
};

static SFontDescObj _gDefaultFontDesc;

#define FD		((SFontDescObj *)inFD)

/*
 * Function Prototypes
 */

static Int16 _FontNameToMacNum(const Uint8 *inName);

void _GRCopy32To8(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inDstColors, Uint32 inDstColCount);
void _GRCopy32To16(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount);
void _GRCopy16To32(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount);
void _GRCopy16To16(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount);
void _GRCopy16To8(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inDstColors, Uint32 inDstColCount);
void _GRCopy8To16(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inSrcColors, Uint32 inSrcColorCount);
void _GRCopy24To16(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount);

void _GRTransCopy16To16(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount);
void _GRTransCopy16To16(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount);

/*
 * Global Variables
 */

const SSimplePattern pattern_White		= { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const SSimplePattern pattern_LightGray	= { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 };
const SSimplePattern pattern_Gray		= { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
const SSimplePattern pattern_DarkGray	= { 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD };
const SSimplePattern pattern_Black		= { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

TRegion gWorkRgn = nil;

static Uint8 _GRInitted = false;

static struct 
{
#if TARGET_API_MAC_CARBON	
	CGrafPtr desktopGrafPortData;
	CGrafPtr dummyGrafPortData;
#else
	CGrafPort desktopGrafPortData;
	CGrafPort dummyGrafPortData;
#endif
} _GRData = { 0,0 };

static SImage _gDesktopImage;
static SImage _gDummyImage;

extern CGrafPtr _gStartupPort;
extern GDHandle _gStartupDevice;

extern Uint32 _gConv5to8TabR[];
extern Uint32 _gConv5to8TabG[];
extern Uint32 _gConv5to8TabB[];

// same as above but for MAC 32 (ARGB)
static const Uint32 _gConv5to8TabMR[] = { 0x00000000, 0x00080000, 0x00100000, 0x00190000, 0x00210000, 0x00290000, 0x00310000, 0x003A0000, 0x00420000, 0x004A0000, 0x00520000, 0x005A0000, 0x00630000, 0x006B0000, 0x00730000, 0x007B0000, 0x00840000, 0x008C0000, 0x00940000, 0x009C0000, 0x00A50000, 0x00AD0000, 0x00B50000, 0x00BD0000, 0x00C50000, 0x00CE0000, 0x00D60000, 0x00DE0000, 0x00E60000, 0x00EF0000, 0x00F70000, 0x00FF0000 };
static const Uint32 _gConv5to8TabMG[] = { 0x00000000, 0x00000800, 0x00001000, 0x00001900, 0x00002100, 0x00002900, 0x00003100, 0x00003A00, 0x00004200, 0x00004A00, 0x00005200, 0x00005A00, 0x00006300, 0x00006B00, 0x00007300, 0x00007B00, 0x00008400, 0x00008C00, 0x00009400, 0x00009C00, 0x0000A500, 0x0000AD00, 0x0000B500, 0x0000BD00, 0x0000C500, 0x0000CE00, 0x0000D600, 0x0000DE00, 0x0000E600, 0x0000EF00, 0x0000F700, 0x0000FF00 };
static const Uint32 _gConv5to8TabMB[] = { 0x00000000, 0x00000008, 0x00000010, 0x00000019, 0x00000021, 0x00000029, 0x00000031, 0x0000003A, 0x00000042, 0x0000004A, 0x00000052, 0x0000005A, 0x00000063, 0x0000006B, 0x00000073, 0x0000007B, 0x00000084, 0x0000008C, 0x00000094, 0x0000009C, 0x000000A5, 0x000000AD, 0x000000B5, 0x000000BD, 0x000000C5, 0x000000CE, 0x000000D6, 0x000000DE, 0x000000E6, 0x000000EF, 0x000000F7, 0x000000FF };

/*
 * Macros
 */

#define CheckQDError()		CheckMacError(::QDError())

#ifndef topLeft
#define topLeft(r)	(((Point *) &(r))[0])
#define botRight(r)	(((Point *) &(r))[1])
#endif

#if DEBUG

static const Int8 kInvalidPortMsg[]		= "WARNING: UGraphics - image is not valid";
static const Int8 kInvalidRegionMsg[]	= "WARNING: UGraphics - region is not valid";
static const Int8 kInvalidPolygonMsg[]	= "WARNING: UGraphics - polygon is not valid";
static const Int8 kInvalidPatternMsg[]	= "WARNING: UGraphics - pattern is not valid";
static const Int8 kInvalidPictureMsg[]	= "WARNING: UGraphics - picture is not valid";

#define FAIL_IF_INVALID_HDL(h, msg)				\
	if (!UMemory::IsValid((THdl)(h)))			\
	{											\
		DebugBreak(msg);						\
		Fail(errorType_Misc, error_Param);		\
	}

#define FAIL_IF_INVALID_PTR(h, msg)				\
	if (!UMemory::IsValid((TPtr)(h)))			\
	{											\
		DebugBreak(msg);						\
		Fail(errorType_Misc, error_Param);		\
	}

#define FAIL_IF_INVALID_PORT(p)					FAIL_IF_INVALID_PTR(p, kInvalidPortMsg)
#define FAIL_IF_INVALID_RGN(h)					FAIL_IF_INVALID_HDL(h, kInvalidRegionMsg)
#define FAIL_IF_INVALID_POLY(h)					FAIL_IF_INVALID_HDL(h, kInvalidPolygonMsg)
#define FAIL_IF_INVALID_PAT(h)					FAIL_IF_INVALID_HDL(h, kInvalidPatternMsg)
#define FAIL_IF_INVALID_PICT(h)					FAIL_IF_INVALID_HDL(h, kInvalidPictureMsg)

#else

#define FAIL_IF_INVALID_HDL(h, msg)
#define FAIL_IF_INVALID_PTR(h, msg)
#define FAIL_IF_INVALID_PORT(p)
#define FAIL_IF_INVALID_RGN(h)
#define FAIL_IF_INVALID_POLY(h)
#define FAIL_IF_INVALID_PAT(h)
#define FAIL_IF_INVALID_PICT(h)

#endif

// don't use SetPort() because it could easily be a GWorld we're drawing into
#if TARGET_API_MAC_CARBON
#define SET_PORT(inImage)												\
	if (::GetQDGlobalsThePort() != (GrafPtr)(((SImage *)inImage)->gw))	\
		::SetGWorld((GrafPtr)(((SImage *)inImage)->gw), nil);
#else
#define SET_PORT(inImage)												\
	if (qd.thePort != (GrafPtr)(((SImage *)inImage)->gw))				\
		::SetGWorld((CGrafPtr)(((SImage *)inImage)->gw), nil);
#endif

/* -------------------------------------------------------------------------- */

void UGraphics::Init()
{
	if (!_GRInitted)
	{
		ClearStruct(_gDefaultFontDesc);
		_gDefaultFontDesc.size = 12;
		_gDefaultFontDesc.align = textAlign_Left;
		
		CGrafPtr desktopGrafPort = nil;
		CGrafPtr dummyGrafPort = nil;
	
		try
		{
			// create work region
			URegion::Init();
			gWorkRgn = URegion::New();
			
			// create desktop graphics port
			{
				RgnHandle desktopRgn = ::GetGrayRgn();

				// create port
			#if TARGET_API_MAC_CARBON
				_GRData.desktopGrafPortData = ::CreateNewPort();
			#else
				::OpenCPort((CGrafPtr)&_GRData.desktopGrafPortData);
			#endif
			
				CheckQDError();
				
			#if TARGET_API_MAC_CARBON	
				desktopGrafPort = _GRData.desktopGrafPortData;
			#else
				desktopGrafPort = &_GRData.desktopGrafPortData;
			#endif
			
				::SetPort((GrafPtr)desktopGrafPort);
				
				// clip to desktop
			#if TARGET_API_MAC_CARBON
				::GetPortVisibleRegion(_GRData.desktopGrafPortData, desktopRgn);
			#else
				::CopyRgn(desktopRgn, _GRData.desktopGrafPortData.visRgn);
			#endif
			
				CheckQDError();
				::SetClip(desktopRgn);
				CheckQDError();
				
				ClearStruct(_gDesktopImage);
			#if TARGET_API_MAC_CARBON
				_gDesktopImage.gw = _GRData.desktopGrafPortData;
			#else
				_gDesktopImage.gw = &_GRData.desktopGrafPortData;
			#endif
			}
			
			// create dummy graphics port
			{
				// create port
			#if TARGET_API_MAC_CARBON
				_GRData.dummyGrafPortData = ::CreateNewPort();
			#else	
				::OpenCPort((CGrafPtr)&_GRData.dummyGrafPortData);
			#endif
			
				CheckQDError();
				
			#if TARGET_API_MAC_CARBON	
				dummyGrafPort = _GRData.dummyGrafPortData;
			#else	
				dummyGrafPort = &_GRData.dummyGrafPortData;
			#endif
			
				::SetPort((GrafPtr)dummyGrafPort);

				// move off the screen
				::MovePortTo(-30000, -30000);
				::PortSize(20, 20);
				
				// don't draw anything in the dummy port
				::HidePen();
				
				ClearStruct(_gDummyImage);
			#if TARGET_API_MAC_CARBON
				_gDummyImage.gw = _GRData.dummyGrafPortData;
			#else
				_gDummyImage.gw = &_GRData.dummyGrafPortData;
			#endif
			}
			
			// make the dummy port current
			::SetPort((GrafPtr)dummyGrafPort);
		}
		catch(...)
		{
			// clean up
			URegion::Dispose(gWorkRgn);
			gWorkRgn = nil;
			
		#if TARGET_API_MAC_CARBON
			if (desktopGrafPort) ::DisposePort(desktopGrafPort);
			if (dummyGrafPort) ::DisposePort(dummyGrafPort);
		#else
			if (desktopGrafPort) ::CloseCPort(desktopGrafPort);
			if (dummyGrafPort) ::CloseCPort(dummyGrafPort);
		#endif
		
			DebugBreak("UGraphics - failed to initialize");
			throw;
		}
		
		_GRInitted = true;
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
 * SetOrigin() makes the specified point the origin (0,0).  You can
 * think of it as adding the point to the coordinates of all
 * subsequent drawing commands.
 */
void UGraphics::SetOrigin(TImage inImage, const SPoint& inOrigin)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	// have to reverse sign because mac SUBTRACTS the point from subsequent drawing commands, whereas we want it to ADD
	::SetOrigin(-inOrigin.h, -inOrigin.v);
}

void UGraphics::GetOrigin(TImage inImage, SPoint& outOrigin)
{
#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(((SImage *)inImage)->gw, &portRect);
	
	outOrigin.h = -portRect.left;
	outOrigin.v = -portRect.top;
#else
	outOrigin.h = -((SImage *)inImage)->gw->portRect.left;
	outOrigin.v = -((SImage *)inImage)->gw->portRect.top;
#endif
}

void UGraphics::AddOrigin(TImage inImage, Int32 inHorizDelta, Int32 inVertDelta)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	// subtract not add because mac is opposite - see comment in UGraphics::SetOrigin()
#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(((SImage *)inImage)->gw, &portRect);
	
	::SetOrigin(portRect.left - inHorizDelta, portRect.top - inVertDelta);
#else
	::SetOrigin(((SImage *)inImage)->gw->portRect.left - inHorizDelta, ((SImage *)inImage)->gw->portRect.top - inVertDelta);
#endif
}

void UGraphics::ResetOrigin(TImage inImage)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::SetOrigin(0, 0);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UGraphics::SetClip(TImage inImage, TRegion inClip)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	FAIL_IF_INVALID_RGN(inClip);

	::SetClip((RgnHandle)inClip);
	CheckQDError();
}

void UGraphics::SetClip(TImage inImage, const SRect& inClip)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	Rect r = {inClip.top, inClip.left, inClip.bottom, inClip.right};
	::ClipRect(&r);
	CheckQDError();
}

void UGraphics::GetClip(TImage inImage, TRegion outClip)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	FAIL_IF_INVALID_RGN(outClip);

	::GetClip((RgnHandle)outClip);
	CheckQDError();
}

void UGraphics::GetClip(TImage inImage, SRect& outClip)
{
#if TARGET_API_MAC_CARBON
	RgnHandle clipRgn = ::NewRgn();
	::GetPortClipRegion(((SImage *)inImage)->gw, clipRgn);
	
	Rect clipRect;
	::GetRegionBounds(clipRgn, &clipRect);
	
	::DisposeRgn(clipRgn);
#else
	Rect clipRect = (**((SImage *)inImage)->gw->clipRgn).rgnBBox;
#endif
	
	outClip.top = clipRect.top;
	outClip.left = clipRect.left;
	outClip.bottom = clipRect.bottom;
	outClip.right = clipRect.right;
}

void UGraphics::SetNoClip(TImage inImage)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	const Rect noClipRect = {-32767, -32767, 32767, 32767};
	::ClipRect(&noClipRect);
	CheckQDError();
}

/*
 * IntersectClip() sets the clip region to the intersection of the existing
 * clip region and the specified Rect (the intersection is the area where
 * they overlap).
 */
void UGraphics::IntersectClip(TImage inImage, const SRect& inClip)
{
#if TARGET_API_MAC_CARBON
	RgnHandle clipRgn = ::NewRgn();	
	::GetPortClipRegion(((SImage *)inImage)->gw, clipRgn);
	
	TRegion existingClip = (TRegion)clipRgn;
#else
	TRegion existingClip = (TRegion)((SImage *)inImage)->gw->clipRgn;
#endif
	
	Init();
	URegion::SetRect(gWorkRgn, inClip);
	URegion::GetIntersection(existingClip, gWorkRgn, gWorkRgn);
	SetClip(inImage, gWorkRgn);

#if TARGET_API_MAC_CARBON
	::DisposeRgn(clipRgn);
#endif
}

void UGraphics::IntersectClip(TImage inImage, TRegion inClip)
{
#if TARGET_API_MAC_CARBON
	RgnHandle clipRgn = ::NewRgn();	
	::GetPortClipRegion(((SImage *)inImage)->gw, clipRgn);
	
	TRegion existingClip = (TRegion)clipRgn;
#else
	TRegion existingClip = (TRegion)((SImage *)inImage)->gw->clipRgn;
#endif
	
	Init();
	URegion::GetIntersection(existingClip, inClip, gWorkRgn);
	SetClip(inImage, gWorkRgn);

#if TARGET_API_MAC_CARBON
	::DisposeRgn(clipRgn);
#endif
}

// set the clip to the intersection of inRgn and inRect
void UGraphics::IntersectClip(TImage inImage, TRegion inRgn, const SRect& inRect)
{
	Require(inImage && inRgn);
	SET_PORT(inImage);

	Init();
	
	Rect r = {min(inRect.top, (Int32)max_Int16), min(inRect.left, (Int32)max_Int16), min(inRect.bottom, (Int32)max_Int16), min(inRect.right, (Int32)max_Int16)};
	::RectRgn((RgnHandle)gWorkRgn, &r);

	::SectRgn((RgnHandle)inRgn, (RgnHandle)gWorkRgn, (RgnHandle)gWorkRgn);
	CheckQDError();
	
	::SetClip((RgnHandle)gWorkRgn);
	CheckQDError();
}

void UGraphics::MoveClip(TImage inImage, Int32 inHorizDelta, Int32 inVertDelta)
{
	if (inHorizDelta || inVertDelta)
	{
		RgnHandle rgn;

		ASSERT(inImage);
		SET_PORT(inImage);
		
		rgn = ::NewRgn();
		if (rgn)
		{
			::GetClip(rgn);
			::OffsetRgn(rgn, inHorizDelta, inVertDelta);
			::SetClip(rgn);
			::DisposeRgn(rgn);
		}
	}
}
		
/* -------------------------------------------------------------------------- */
#pragma mark -

GDHandle _GetRectDevice(Rect *theRect);

Uint16 UGraphics::GetDepth(TImage inImage)
{
	Require(inImage);
	
	// if offscreen image
#if TARGET_API_MAC_CARBON
	if (::IsPortOffscreen(((SImage *)inImage)->gw))
#else	
	if (((SImage *)inImage)->gw->portVersion == 0xC001)
#endif
		return (**::GetGWorldPixMap(((SImage *)inImage)->gw)).pixelSize;

	// if onscreen image
#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(((SImage *)inImage)->gw, &portRect);
#else	
	Rect portRect = ((SImage *)inImage)->gw->portRect;
#endif
		
	SET_PORT(inImage);
	
	Point pt = { 0, 0 };
	::LocalToGlobal(&pt);
	::OffsetRect(&portRect, pt.h, pt.v);
	
	GDHandle gd = _GetRectDevice(&portRect);
	return gd == nil ? 1 : (**(**gd).gdPMap).pixelSize;
}

void UGraphics::Reset(TImage inImage)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::PenNormal();
	::TextMode(srcOr);
	::ForeColor(blackColor);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UGraphics::ResetPen(TImage inImage)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::PenSize(1, 1);
}

void UGraphics::SetPenSize(TImage inImage, Uint32 inSize)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::PenSize(inSize, inSize);
}

Uint32 UGraphics::GetPenSize(TImage inImage)
{
	ASSERT(inImage);

#if TARGET_API_MAC_CARBON
	Point pnSize;
	::GetPortPenSize(((SImage *)inImage)->gw, &pnSize);
	
	return pnSize.h;
#else	
	return ((SImage *)inImage)->gw->pnSize.h;
#endif
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UGraphics::ResetInk(TImage inImage)
{
	const RGBColor rgb = { 0, 0, 0 };
	
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::RGBForeColor(&rgb);
	::PenMode(patCopy);
	::TextMode(srcOr);
}

void UGraphics::SetInkAttributes(TImage /* inImage */, Uint32 /* inAttrib */)
{
	// can't do any of those fancy attributes with plain quickdraw
}

void UGraphics::SetInkColor(TImage inImage, const SColor& inColor)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::RGBForeColor((RGBColor *)&inColor);
}

void UGraphics::SetInkMode(TImage inImage, Uint32 inTransferMode, const SColor *inOperand)
{
	Int16 macMode;
	
	switch (inTransferMode)
	{
		case mode_Copy:
			macMode = srcCopy;
			break;
		case mode_Add:
			macMode = addPin;
			break;
		case mode_Blend:
			macMode = blend;
			break;
		case mode_Highlight:
			macMode = hilitetransfermode;
			break;
		case mode_And:
		case mode_RampAnd:
			macMode = srcBic;
			break;
		case mode_Or:
		case mode_RampOr:
			macMode = srcOr;
			break;
		case mode_Xor:
		case mode_RampXor:
			macMode = srcXor;
			break;
		case mode_Over:
			macMode = addOver;
			break;
		case mode_Fade:
			macMode = subPin;
			break;
		default:
			macMode = srcCopy;
			break;
	}
	
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::PenMode(macMode);
	::TextMode(macMode);

	if (inOperand) 
		::OpColor((RGBColor *)&inOperand);
}

Uint32 UGraphics::GetInkAttributes(TImage /* inImage */)
{
	// can't do any of those fancy attributes with plain quickdraw
	return 0;
}

void UGraphics::GetInkColor(TImage inImage, SColor& outColor)
{
	ASSERT(inImage);
	SET_PORT(inImage);

	::GetForeColor((RGBColor *)&outColor);
}

void UGraphics::GetInkMode(TImage inImage, Uint32& outTransferMode, SColor& outOperand)
{
	outTransferMode = UGraphics::GetInkMode(inImage);
	
#if TARGET_API_MAC_CARBON
	RGBColor rgbOpColor;
	::GetPortOpColor(((SImage *)inImage)->gw, &rgbOpColor);
	
	outOperand = *(SColor *) &rgbOpColor;
#else
	outOperand = *(SColor *) &(**(GVarHandle)(((SImage *)inImage)->gw->grafVars)).rgbOpColor;
#endif
}

Uint32 UGraphics::GetInkMode(TImage inImage)
{
	Uint32 mode;
	
#if TARGET_API_MAC_CARBON
	switch (::GetPortPenMode(((SImage *)inImage)->gw))
#else
	switch (((SImage *)inImage)->gw->pnMode)
#endif
	{
		case srcCopy:
			mode = mode_Copy;
			break;
		case srcOr:
			mode = mode_Or;
			break;
		case srcXor:
			mode = mode_Xor;
			break;
		case srcBic:
			mode = mode_And;
			break;
		case hilitetransfermode:
			mode = mode_Highlight;
			break;
		case blend:
			mode = mode_Blend;
			break;
		case addPin:
			mode = mode_Add;
			break;
		case addOver:
			mode = mode_Over;
			break;
		case subPin:
			mode = mode_Fade;
			break;
		default:
			mode = mode_Copy;
			break;
	}
	
	return mode;
}

// find the color in inImage that most closely matches ioColor
void UGraphics::GetNearestColor(TImage inImage, SColor& ioColor)
{
	ASSERT(inImage);
	
#if TARGET_API_MAC_CARBON
	Int16 pixelSize = (**::GetPortPixMap(((SImage *)inImage)->gw)).pixelSize;
#else
	Int16 pixelSize = (**((SImage *)inImage)->gw->portPixMap).pixelSize;
#endif
	
	if (pixelSize <= 8)
	{
		::Index2Color(::Color2Index((RGBColor *)&ioColor), (RGBColor *)&ioColor);
	}
	else if (pixelSize == 16)
	{
		ioColor.red = _gConv5to8TabMB[ioColor.red >> 11] * 257;
		ioColor.green = _gConv5to8TabMB[ioColor.green >> 11] * 257;
		ioColor.blue = _gConv5to8TabMB[ioColor.blue >> 11] * 257;
	}
	else if (pixelSize == 32)
	{
		ioColor.red = (ioColor.red >> 8) * 257;
		ioColor.green = (ioColor.green >> 8) * 257;
		ioColor.blue = (ioColor.blue >> 8) * 257;
	}
	else
		ioColor.Set(0);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UGraphics_DrawPixel(TImage inImage, const SPoint& inPixel)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	RGBColor rgb;
	::GetForeColor(&rgb);
	::SetCPixel(inPixel.h, inPixel.v, &rgb);
}

// see notes in UGraphics.cp regarding how lines are drawn
void UGraphics_DrawLine(TImage inImage, const SLine& inLine, Uint32 inOptions)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
#if TARGET_API_MAC_CARBON
	Point pnSize;
	::GetPortPenSize(((SImage *)inImage)->gw, &pnSize);
	
	Int32 pn = pnSize.h;
#else
	Int32 pn = ((SImage *)inImage)->gw->pnSize.h;
#endif

	if (inOptions & 1)	// if draw rounded overlapping corners
	{
		pn >>= 1;
		
		// adjust to make thick line centered around the points
		::MoveTo(inLine.start.x - pn, inLine.start.y - pn);
		::LineTo(inLine.end.x - pn, inLine.end.y - pn);
	}
	else if (pn == 1)
	{
		Int32 startX = inLine.start.x;
		Int32 startY = inLine.start.y;
		
		Int32 endX = inLine.end.x;
		Int32 endY = inLine.end.y;
		
		Int32 deltaX = endX - startX;
		Int32 deltaY = endY - startY;
		
		if (deltaX || deltaY)
		{
			// pull the line back so it doesn't include the ending point
			if (deltaX > 0)
				endX -= 1;
			else if (deltaX < 0)
				endX += 1;
			
			if (deltaY > 0)
				endY -= 1;
			else if (deltaY < 0)
				endY += 1;
						
			::MoveTo(startX, startY);
			::LineTo(endX, endY);
		}
	}
	else if (pn != 0)
	{
		SPoint pts[4];
		UGraphics::GetLinePoly(inLine.start, inLine.end, pn, pts);

		PolyHandle poly = ::OpenPoly();
		if (poly == nil) 
			return;

		::MoveTo(pts[0].h, pts[0].v);
		::LineTo(pts[1].h, pts[1].v);
		::LineTo(pts[2].h, pts[2].v);
		::LineTo(pts[3].h, pts[3].v);
		
		::ClosePoly();
		::PaintPoly(poly);
		::KillPoly(poly);
	}
}

// see notes in UGraphics.cp regarding how lines are drawn
void UGraphics_DrawLines(TImage inImage, const SLine *inLines, Uint32 inCount, Uint32 inOptions)
{
	if (!inLines)
		return;

	ASSERT(inImage);
	SET_PORT(inImage);

#if TARGET_API_MAC_CARBON
	Point pnSize;
	::GetPortPenSize(((SImage *)inImage)->gw, &pnSize);
	
	Int32 pn = pnSize.h;
#else
	Int32 pn = ((SImage *)inImage)->gw->pnSize.h;
#endif
	
	if (inOptions & 1)	// if draw rounded overlapping corners
	{
		if (pn == 1)
		{
			while (inCount--)
			{
				::MoveTo(inLines->start.x, inLines->start.y);
				::LineTo(inLines->end.x, inLines->end.y);
				inLines++;
			}
		}
		else
		{
			pn >>= 1;

			while (inCount--)
			{
				// adjust to make thick line centered around the points
				::MoveTo(inLines->start.x - pn, inLines->start.y - pn);
				::LineTo(inLines->end.x - pn, inLines->end.y - pn);
				
				inLines++;
			}
		}
	}
	else if (pn == 1)
	{
		while (inCount--)
		{
			Int32 startX = inLines->start.x;
			Int32 startY = inLines->start.y;
			
			Int32 endX = inLines->end.x;
			Int32 endY = inLines->end.y;
			
			Int32 deltaX = endX - startX;
			Int32 deltaY = endY - startY;
			
			if (deltaX || deltaY)
			{
				// pull the line back so it doesn't include the ending point
				if (deltaX > 0)
					endX -= 1;
				else if (deltaX < 0)
					endX += 1;
				
				if (deltaY > 0)
					endY -= 1;
				else if (deltaY < 0)
					endY += 1;

				::MoveTo(startX, startY);
				::LineTo(endX, endY);
			}
			
			inLines++;
		}
	}
	else if (pn != 0)
	{
		while (inCount--)
		{
			SPoint pts[4];
			UGraphics::GetLinePoly(inLines->start, inLines->end, pn, pts);

			PolyHandle poly = ::OpenPoly();
			if (poly == nil) 
				return;

			::MoveTo(pts[0].h, pts[0].v);
			::LineTo(pts[1].h, pts[1].v);
			::LineTo(pts[2].h, pts[2].v);
			::LineTo(pts[3].h, pts[3].v);
			
			::ClosePoly();
			::PaintPoly(poly);
			::KillPoly(poly);

			inLines++;
		}
	}
}

void UGraphics_DrawRect(TImage inImage, const SRect& inRect, Uint32 inFill)
{
	Rect r;

	ASSERT(inImage);
	SET_PORT(inImage);
	
	r.top = inRect.top;
	r.left = inRect.left;
	r.bottom = inRect.bottom;
	r.right = inRect.right;
	
	switch (inFill)
	{
		case fill_None:
			break;
		
		case fill_OpenFrame:
		case fill_ClosedFrame:
			::FrameRect(&r);
			break;
		
		default:
			::PaintRect(&r);
			break;
	}
}

void UGraphics_DrawOval(TImage inImage, const SRect& inRect, Uint32 inFill)
{
	Rect r;

	ASSERT(inImage);
	SET_PORT(inImage);
	
	r.top = inRect.top;
	r.left = inRect.left;
	r.bottom = inRect.bottom;
	r.right = inRect.right;
	
	switch (inFill)
	{
		case fill_None:
			break;
		
		case fill_OpenFrame:
		case fill_ClosedFrame:
			::FrameOval(&r);
			break;
		
		default:
			::PaintOval(&r);
			break;
	}
}

void UGraphics_DrawRoundRect(TImage inImage, const SRoundRect& inRect, Uint32 inFill)
{
	Rect r;

	ASSERT(inImage);
	SET_PORT(inImage);
	
	r.top = inRect.top;
	r.left = inRect.left;
	r.bottom = inRect.bottom;
	r.right = inRect.right;
	
	switch (inFill)
	{
		case fill_None:
			break;
		
		case fill_OpenFrame:
		case fill_ClosedFrame:
			::FrameRoundRect(&r, inRect.ovalWidth, inRect.ovalHeight);
			break;
		
		default:
			::PaintRoundRect(&r, inRect.ovalWidth, inRect.ovalHeight);
			break;
	}
}

void UGraphics_DrawPolygon(TImage inImage, const SPoint *inPointList, Uint32 inPointCount, Uint32 inFill, Uint32 /* inOptions */)
{
	if (!inPointList || inPointCount < 2 || inFill == fill_None) 
		return;
	
	ASSERT(inImage);
	SET_PORT(inImage);
	
	if (inFill == fill_OpenFrame || inFill == fill_ClosedFrame)
	{
		const SPoint *firstPt = inPointList;
		
	#if TARGET_API_MAC_CARBON
		Point pnSize;
		::GetPortPenSize(((SImage *)inImage)->gw, &pnSize);
	
		Uint32 pn = pnSize.h;
	#else
		Uint32 pn = ((SImage *)inImage)->gw->pnSize.h;
	#endif
		
		if (pn == 1)
		{
			::MoveTo(inPointList->h, inPointList->v);
			
			inPointCount--;
			inPointList++;
			
			while (inPointCount--)
			{
				::LineTo(inPointList->h, inPointList->v);
				
				inPointList++;
			}
			
			if (inFill == fill_ClosedFrame)
				::LineTo(firstPt->h, firstPt->v);
		}
		else
		{
			// we make adjustments to ensure that the lines are drawn centered around the points rather than hanging like the mac does
			pn >>= 1;
			
			::MoveTo(inPointList->h - pn, inPointList->v - pn);
			
			inPointCount--;
			inPointList++;
			
			while (inPointCount--)
			{
				::LineTo(inPointList->h - pn, inPointList->v - pn);
				
				inPointList++;
			}
			
			if (inFill == fill_ClosedFrame)
				::LineTo(firstPt->h - pn, firstPt->v - pn);
		}
	}
	else
	{
		PolyHandle poly = ::OpenPoly();
		if (poly == nil) return;

		::MoveTo(inPointList->h, inPointList->v);
		
		inPointCount--;
		inPointList++;
		
		while (inPointCount--)
		{
			::LineTo(inPointList->h, inPointList->v);
			
			inPointList++;
		}
		
		::ClosePoly();
		::PaintPoly(poly);
		::KillPoly(poly);
	}
}

void UGraphics_DrawRegion(TImage inImage, TRegion inRgn, Uint32 inFill)
{
	if (!inRgn)
		return;
	
	ASSERT(inImage);
	SET_PORT(inImage);
		
	switch (inFill)
	{
		case fill_None:
			break;
			
		case fill_OpenFrame:
		case fill_ClosedFrame:
			::FrameRgn((RgnHandle)inRgn);
			break;
			
		default:
			::PaintRgn((RgnHandle)inRgn);
			break;
	}
}

void UGraphics_FillRect(TImage inImage, const SRect& inRect)
{
	Rect r;

	ASSERT(inImage);
	SET_PORT(inImage);
	
	r.top = inRect.top;
	r.left = inRect.left;
	r.bottom = inRect.bottom;
	r.right = inRect.right;
	
	::PaintRect(&r);
}

void UGraphics_FrameRect(TImage inImage, const SRect& inRect)
{
	Rect r;

	ASSERT(inImage);
	SET_PORT(inImage);
	
	r.top = inRect.top;
	r.left = inRect.left;
	r.bottom = inRect.bottom;
	r.right = inRect.right;
	
	::FrameRect(&r);
}

void UGraphics_SetPixel(TImage inImage, const SPoint& inPixel, const SColor& inColor)
{
	ASSERT(inImage);
	SET_PORT(inImage);

	::SetCPixel(inPixel.h, inPixel.v, (RGBColor *)&inColor);
}

void UGraphics_SetPixels(TImage inImage, const SPoint *inPointList, Uint32 inPointCount, const SColor& inColor)
{
	if (!inPointList)
		return;

	ASSERT(inImage);
	SET_PORT(inImage);
	
	RGBColor *rgb = (RGBColor *)&inColor;
	
	while (inPointCount--)
	{
		::SetCPixel(inPointList->h, inPointList->v, rgb);
		inPointList++;
	}
}

void UGraphics_GetPixel(TImage inImage, const SPoint& inPixel, SColor& outColor)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::GetCPixel(inPixel.h, inPixel.v, (RGBColor *)&outColor);
}

void UGraphics_FloodFill(TImage inImage, const SPoint& inPt)
{
	ASSERT(inImage);

#if TARGET_API_MAC_CARBON
	Rect srcRect;
	::GetPortBounds(((SImage *)inImage)->gw, &srcRect);
	const BitMap *srcMap = ::GetPortBitMapForCopyBits(((SImage *)inImage)->gw);
#else	
	Rect srcRect = ((SImage *)inImage)->gw->portRect;
	const BitMap *srcMap = &((GrafPtr)((SImage *)inImage)->gw)->portBits;
#endif
	
	BitMap destMap;
	destMap.rowBytes = ((srcRect.right - srcRect.left + 15) / 16) * 2;
	destMap.bounds = srcRect;
	destMap.baseAddr = ::NewPtr((Uint32)destMap.rowBytes * (srcRect.bottom - srcRect.top));
	if (destMap.baseAddr == nil) 
		return;
	
	::SeedCFill(srcMap, &destMap, &srcRect, &destMap.bounds, inPt.h, inPt.v, nil, nil);
	::CopyBits(&destMap, srcMap, &destMap.bounds, &srcRect, srcOr, nil);
	::DisposePtr(destMap.baseAddr);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UGraphics::ResetFont(TImage inImage)
{
	ASSERT(inImage);
	SET_PORT(inImage);

	::TextFont(0);
	::TextFace(0);
	::TextSize(0);
}

void UGraphics::SetFontName(TImage inImage, const Uint8 *inName, const Uint8 */* inStyle */)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::TextFont(_FontNameToMacNum(inName));
}

void UGraphics::SetFontSize(TImage inImage, Uint32 inSize)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	::TextSize(inSize);
}

Uint32 UGraphics::GetFontSize(TImage inImage)
{
#if TARGET_API_MAC_CARBON
	Uint16 fontSize = ::GetPortTextSize(((SImage *)inImage)->gw);
#else
	Uint16 fontSize = ((SImage *)inImage)->gw->txSize;
#endif
	
	if (fontSize == 0)
	{
		// get system font size
		FontInfo fi;
		::GetFontInfo(&fi);
		fontSize = fi.ascent;
	}
	
	return fontSize;
}

void UGraphics::SetFontEffect(TImage inImage, Uint32 inFlags)
{
	ASSERT(inImage);
	SET_PORT(inImage);

	if (inFlags == fontEffect_Plain)
	{
		::TextFace(0);
		return;
	}
	else if (inFlags == fontEffect_Bold)
	{
		::TextFace(bold);
		return;
	}

	Uint16 face = 0;
	
	if (inFlags & fontEffect_Bold) face |= bold;
	if (inFlags & fontEffect_Italic) face |= italic;
	if (inFlags & fontEffect_Underline) face |= underline;
	if (inFlags & fontEffect_Outline) face |= outline;
	if (inFlags & fontEffect_Condense) face |= condense;
	if (inFlags & fontEffect_Extend) face |= extend;

	::TextFace(face);
}

Uint32 UGraphics::GetFontEffect(TImage inImage)
{
#if TARGET_API_MAC_CARBON
	Uint16 face = ::GetPortTextFace(((SImage *)inImage)->gw);
#else
	Uint16 face = ((SImage *)inImage)->gw->txFace;
#endif

	Uint32 effect = 0;
	
	if (face & bold) effect |= fontEffect_Bold;
	if (face & italic) effect |= fontEffect_Italic;
	if (face & underline) effect |= fontEffect_Underline;
	if (face & outline) effect |= fontEffect_Outline;
	if (face & condense) effect |= fontEffect_Condense;
	if (face & extend) effect |= fontEffect_Extend;

	return effect;
}

// resets the other font settings (not specified in params) to defaults
void UGraphics::SetFont(TImage inImage, const Uint8 *inName, const Uint8 */* inStyle */, Uint32 inSize, Uint32 inEffect)
{
	SetFontEffect(inImage, inEffect);

	::TextFont(_FontNameToMacNum(inName));
	::TextSize(inSize);
}

void UGraphics::SetFont(TImage inImage, const SFontDesc& inInfo)
{
	SetFontEffect(inImage, inInfo.effect);

	::TextFont(_FontNameToMacNum(inInfo.name));
	::TextSize(inInfo.size);
	
	if (inInfo.color) 
		::RGBForeColor((RGBColor *)inInfo.color);
}

// specify nil to use default settings
void UGraphics::SetFont(TImage inImage, TFontDesc inFD)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	if (inFD == nil)
	{
		::TextFont(_gDefaultFontDesc.macNum);
		::TextFace(_gDefaultFontDesc.macStyle);
		::TextSize(_gDefaultFontDesc.size);
		::RGBForeColor((RGBColor *)&_gDefaultFontDesc.color);
	}
	else
	{
		::TextFont(FD->macNum);
		::TextFace(FD->macStyle);
		::TextSize(FD->size);
		::RGBForeColor((RGBColor *)&FD->color);
	}
	
	::TextMode(srcOr);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UGraphics::GetFontMetrics(TImage inImage, SFontMetrics& outInfo)
{
	FontInfo fi;
	
	ASSERT(inImage);
	SET_PORT(inImage);

	::GetFontInfo(&fi);
	
	outInfo.ascent = fi.ascent;
	outInfo.descent = fi.descent;
	outInfo.lineSpace = fi.leading;
	outInfo.internal = outInfo.rsvd[0] = outInfo.rsvd[1] = outInfo.rsvd[2] = outInfo.rsvd[3] = 0;
}

Uint32 UGraphics::GetFontHeight(TImage inImage)
{
	FontInfo fi;
	
	ASSERT(inImage);
	SET_PORT(inImage);

	::GetFontInfo(&fi);
	
	return fi.ascent + fi.descent;
}

Uint32 UGraphics::GetFontLineHeight(TImage inImage)
{
	FontInfo fi;
	
	ASSERT(inImage);
	SET_PORT(inImage);

	::GetFontInfo(&fi);
	
	return fi.ascent + fi.descent + fi.leading;
}

Uint32 UGraphics::GetCharWidth(TImage inImage, Uint16 inChar, Uint32 /* inEncoding */)
{
	ASSERT(inImage);
	SET_PORT(inImage);
	
	return ::CharWidth(inChar);
}

Uint32 UGraphics::GetTextWidth(TImage inImage, const void *inText, Uint32 inTextSize, Uint32 /* inEncoding */)
{
	if (!inText || !inTextSize) 
		return 0;
	
	ASSERT(inImage);
	SET_PORT(inImage);
	
	return ::TextWidth((Ptr)inText, 0, inTextSize);
}

// returns offset into <inText> of the line break
Uint32 UGraphics::GetTextLineBreak(TImage inImage, const void *inText, Uint32 inLength, Uint32 inMaxWidth)
{
	Fixed maxWidth = (Fixed)(inMaxWidth << 16);
	Int32 lineBytes = 1;
	
	if (inLength > 1000) 
		inLength = 1000;	// necessary or StyledLineBreak stuffs!
	
	SET_PORT(inImage);
	::StyledLineBreak((Ptr)inText, inLength, 0, inLength, 0, &maxWidth, &lineBytes);
	
	return lineBytes;
}

// returns offset of character that <inWidth> is closest to
Uint32 UGraphics::WidthToChar(TImage inImage, const void *inText, Uint32 inLength, Uint32 inWidth, bool *outLeftSide)
{
	SET_PORT(inImage);	
	Boolean isLeftSide;
	
#if TARGET_API_MAC_CARBON
	Point numer, denom;
	numer.v = numer.h = denom.v = denom.h = 1;
	
	Int16 offset = ::PixelToChar((Ptr)inText, inLength, 0, ::BitShift(inWidth, 16), &isLeftSide, nil, onlyStyleRun, numer, denom);
#else
	Int16 offset = ::Pixel2Char((Ptr)inText, inLength, 0, inWidth, &isLeftSide);
#endif

	if (outLeftSide) 
		*outLeftSide = (isLeftSide != 0);
	
	return offset;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UGraphics_DrawText(TImage inImage, const SRect& inBounds, const void *inText, Uint32 inTextSize, Uint32 /* inEncoding */, Uint32 inAlignFlags)
{
	if (!inText || !inTextSize)
		return;

	FontInfo fi;
	Int16 h, v;

	ASSERT(inImage);
	SET_PORT(inImage);
		
	::GetFontInfo(&fi);
		
	// calc horizontal position
	if (inAlignFlags & align_Right)				// at right
		h = inBounds.right - ::TextWidth(inText, 0, ::VisibleLength((Ptr)inText, inTextSize));
	else if (inAlignFlags & align_CenterHoriz)	// centered horizontally
		h = (inBounds.left + inBounds.right - ::TextWidth(inText, 0, inTextSize)) / 2;
	else										// at left
		h = inBounds.left;
		
	// calc vertical position
	if (inAlignFlags & align_Bottom)			// at bottom
		v = inBounds.bottom - fi.descent;
	else if (inAlignFlags & align_CenterVert)	// centered vertically
		v = (fi.ascent - fi.descent + inBounds.bottom + inBounds.top) / 2;
	else										// at top
		v = inBounds.top + fi.ascent;
		
	// draw the text (point is left of baseline)
	::MoveTo(h, v);
	::DrawText(inText, 0, inTextSize);
}

void UGraphics_DrawTruncText(TImage inImage, const SRect& inBounds, const void *inText, Uint32 inTextSize, Uint32 /* inEncoding */, Uint32 inAlignFlags)
{
	if (!inText || !inTextSize)
		return;

	Uint8 buf[1024];
	Int16 length = (inTextSize > sizeof(buf)) ? sizeof(buf) : inTextSize;
		
	::BlockMoveData(inText, buf, length);

	ASSERT(inImage);
	SET_PORT(inImage);
	
	if (::TruncText(inBounds.GetWidth(), (Ptr)buf, &length, smTruncEnd) != -1)
		UGraphics::DrawText(inImage, inBounds, buf, length, 0, inAlignFlags);
}

void UGraphics_DrawTextBox(TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, const void *inText, Uint32 inTextSize, Uint32 /* inEncoding */, Uint32 inAlign)
{
	if (!inText || !inTextSize)
		return;

	StyledLineBreakCode	breakCode;
	Fixed boxWidthFixed, tempFixed;
	Int32 lineBytes, textLeft;
	Uint32 blackLen;
	Int16 boxWidth, curY, updateTop, updateBottom, updateCurY;
	Uint16 lineHeight, sysAlign;
	Uint8 *lineStart, *textEnd;
	Rect wrapBox;
	FontInfo fi;
	
	// set port and get font ascent, descent etc
	ASSERT(inImage);
	SET_PORT(inImage);
	::GetFontInfo(&fi);
	
	// grab a copy of bounds and adjust to draw in same spot as standard TextBox()
	wrapBox.top = inBounds.top - 1;
	wrapBox.left = inBounds.left + 1;
	wrapBox.bottom = inBounds.bottom - 1;
	wrapBox.right = inBounds.right + 1;

	// grab update top and bottom and expand just to be on the safe side
	updateTop = inUpdateRect.top - 4;
	updateBottom = inUpdateRect.bottom + 4;

	// check alignment
	sysAlign = (::GetSysDirection() == -1) ? textAlign_Right : textAlign_Left;
	switch (inAlign)
	{
		case textAlign_Left:
		case textAlign_Right:
		case textAlign_Center:
		case textAlign_Full:
			break;
		default:
			inAlign = sysAlign;
			break;
	}

	// calculate some values
	boxWidth = wrapBox.right - wrapBox.left;
	boxWidthFixed = Long2Fix(boxWidth);
	lineHeight = fi.ascent + fi.descent + fi.leading;
	curY = wrapBox.top + fi.ascent + fi.leading;
	updateCurY = wrapBox.top;
	lineStart = (Uint8 *)inText;
	textEnd = (Uint8 *)inText + inTextSize;
	textLeft = inTextSize;
	
	// main wrap-and-draw loop
	while (lineStart < textEnd)
	{
		updateCurY += lineHeight;
		
		// determine where to break the line
		lineBytes = 1;				// must do every time
		tempFixed = boxWidthFixed;	// must do every time
		breakCode = ::StyledLineBreak((Ptr)lineStart, textLeft, 0, textLeft, 0, &tempFixed, &lineBytes);
		
		// draw the line but only if overlaps with the update rect
		if (updateCurY >= updateTop)
		{
			if (inAlign == textAlign_Full)
			{
				blackLen = ::VisibleLength((Ptr)lineStart, lineBytes);
			
				if (breakCode == smBreakOverflow || *((Uint8 *)lineStart + (lineBytes - 1)) == 0x0D)
				{
					if (sysAlign == textAlign_Right)
						::MoveTo(wrapBox.right - ::TextWidth(lineStart, 0, blackLen), curY);
					else
						::MoveTo(wrapBox.left, curY);
					
					::DrawText(lineStart, 0, lineBytes);
				}
				else
				{
					::MoveTo(wrapBox.left, curY);
				
				#if TARGET_API_MAC_CARBON
					Point numer, denom;
					numer.v = numer.h = denom.v = denom.h = 1;
	
					::DrawJustified((Ptr)lineStart, blackLen, boxWidth - ::TextWidth(lineStart, 0, blackLen), onlyStyleRun, numer, denom);
				#else
					::DrawJust((Ptr)lineStart, blackLen, boxWidth - ::TextWidth(lineStart, 0, blackLen));
				#endif
				}
			}
			else
			{
				switch (inAlign)
				{
					case textAlign_Left:
						::MoveTo(wrapBox.left, curY);
						break;
					case textAlign_Right:
						blackLen = ::VisibleLength((Ptr)lineStart, lineBytes);
						::MoveTo(wrapBox.right - ::TextWidth(lineStart, 0, blackLen), curY);
						break;
					case textAlign_Center:
						blackLen = ::VisibleLength((Ptr)lineStart, lineBytes);
						::MoveTo(wrapBox.left + (boxWidth - ::TextWidth(lineStart, 0, blackLen)) / 2, curY);
						break;
				}
				
				::DrawText(lineStart, 0, lineBytes);
			}
		}
		
		// advance to next line
		curY += lineHeight;
		lineStart += lineBytes;
		textLeft -= lineBytes;
		
		// stop drawing if outside of update rect
		if (updateCurY > updateBottom) break;
	}
}

Uint32 UGraphics_GetTextBoxHeight(TImage inImage, const SRect& inBounds, const void *inText, Uint32 inTextSize, Uint32 /* inEncoding */, Uint32 /* inAlign */)
{
	if (!inText || !inTextSize)
		return 0;

	StyledLineBreakCode	breakCode;
	Fixed boxWidthFixed, tempFixed;
	Int32 lineBytes, textLeft;
	Uint32 lineCount;
	Uint16 lineHeight;
	Uint8 *lineStart, *textEnd;
	FontInfo fi;
	
	// set port and get font ascent, descent etc
	ASSERT(inImage);
	SET_PORT(inImage);
	::GetFontInfo(&fi);
	
	// calculate some values
	boxWidthFixed = Long2Fix(inBounds.GetWidth());
	lineHeight = fi.ascent + fi.descent + fi.leading;
	lineStart = (Uint8 *)inText;
	textEnd = (Uint8 *)inText + inTextSize;
	textLeft = inTextSize;
	lineCount = 0;
	
	// main wrap-and-draw loop
	while (lineStart < textEnd)
	{
		// determine where to break the line
		lineBytes = 1;				// must do every time
		tempFixed = boxWidthFixed;	// must do every time
		breakCode = ::StyledLineBreak((Ptr)lineStart, textLeft, 0, textLeft, 0, &tempFixed, &lineBytes);
		
		// advance to next line
		lineCount++;
		lineStart += lineBytes;
		textLeft -= lineBytes;
	}
	
	return lineCount * lineHeight;
}

// drawing includes inStartLine (0-based), does NOT include inEndLine
void UGraphics_DrawTextLines(TImage inImage, const SRect& inBounds, const void *inText, Uint32 /* inEncoding */, const Uint32 *inLineOffsets, Uint32 inStartLine, Uint32 inEndLine, Uint32 inLineHeight, Uint32 inAlign)
{
	if (!inText || inStartLine >= inEndLine)
		return;

	Uint8 *linePtr;
	Uint32 lineBytes, i;
	Int16 boundsLeft, boundsRight, boundsWidth, curY;
	FontInfo fi;

	ASSERT(inImage);
	SET_PORT(inImage);
	::GetFontInfo(&fi);
	
	i = inStartLine;
	boundsLeft = inBounds.left;
	boundsRight = inBounds.right;
	boundsWidth = inBounds.GetWidth();
	curY = inBounds.top + (i * inLineHeight) + fi.ascent;
	
	if (inAlign == textAlign_Right)
	{
		while (i != inEndLine)
		{
			linePtr = (Uint8 *)inText + inLineOffsets[i];
			lineBytes = inLineOffsets[i+1] - inLineOffsets[i];
			
			::MoveTo(boundsRight - ::TextWidth(linePtr, 0, ::VisibleLength((Ptr)linePtr, lineBytes)), curY);
			::DrawText(linePtr, 0, lineBytes);

			curY += inLineHeight;
			i++;
		}
	}
	else if (inAlign == textAlign_Center)
	{
		while (i != inEndLine)
		{
			linePtr = (Uint8 *)inText + inLineOffsets[i];
			lineBytes = inLineOffsets[i+1] - inLineOffsets[i];
			
			::MoveTo(boundsLeft + (boundsWidth - ::TextWidth(linePtr, 0, ::VisibleLength((Ptr)linePtr, lineBytes))) / 2, curY);
			::DrawText(linePtr, 0, lineBytes);

			curY += inLineHeight;
			i++;
		}
	}
	else
	{
		while (i != inEndLine)
		{
			linePtr = (Uint8 *)inText + inLineOffsets[i];
			lineBytes = inLineOffsets[i+1] - inLineOffsets[i];
			
			::MoveTo(boundsLeft, curY);
			::DrawText(linePtr, 0, lineBytes);

			curY += inLineHeight;
			i++;
		}
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// creates an image that is compatible with the screen
TImage UGraphics::NewCompatibleImage(Uint32 inWidth, Uint32 inHeight)
{
	GWorldPtr gw = nil;
	Rect r = {0, 0, inHeight, inWidth};
	
	CheckMacError(::NewGWorld(&gw, 0, &r, nil, ::GetMainDevice(), noNewDevice | useTempMem));
//	::SetGWorld((CGrafPtr)gw, nil);
	::SetGWorld(gw, nil);

	SImage *img = (SImage *)UMemory::NewClear(sizeof(SImage));
	img->gw = gw;
	img->lockCount=0;
#if 0
	PixMapHandle pm = ::GetGWorldPixMap(gw);
	if (::HGetState((Handle)pm) & 0x80)	// if already locked
		DebugBreak("UGraphics - TImage(%X) already locked!",img);
#endif	
	
	
	return (TImage)img;
}

void UGraphics::DisposeImage(TImage inImage)
{
	if (inImage)
	{
		::SetGWorld(_gStartupPort, _gStartupDevice);

		::DisposeGWorld(((SImage *)inImage)->gw);
		UMemory::Dispose((TPtr)inImage);
	}
}

void UGraphics::LockImage(TImage inImage)
{
	Require(inImage);
	if (((SImage *)inImage)->lockCount++ > 0)
	   return;
	   
	PixMapHandle pm = ::GetGWorldPixMap(((SImage *)inImage)->gw);
	
#if 0
	if (::HGetState((Handle)pm) & 0x80)	// if already locked
	{
		DebugBreak("UGraphics - TImage(%X) already locked!",inImage);
		Fail(errorType_Misc, error_Protocol);
	}
#endif
	
	if (!::LockPixels(pm))
	{
		CheckQDError();
		Fail(errorType_Misc, error_Unknown);
		DebugLog("UGraphics - TImage(%X) locked",inImage);
	}
}

void UGraphics::UnlockImage(TImage inImage)
{
	if (inImage) 
		if (--((SImage *)inImage)->lockCount == 0)
			::UnlockPixels(::GetGWorldPixMap(((SImage *)inImage)->gw));
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UGraphics_CopyPixels(TImage inDest, const SPoint& inDestPt, TImage inSource, const SPoint& inSourcePt, Uint32 inWidth, Uint32 inHeight, Uint32 /* inOptions */)
{
	const RGBColor whiteRGB = { 0xFFFF, 0xFFFF, 0xFFFF };
	const RGBColor blackRGB = { 0x0000, 0x0000, 0x0000 };
	Rect destRect;
	Rect srcRect;
	RGBColor saveBack, saveFore;

	Require(inDest && inSource);
	
	destRect.left = inDestPt.x;
	destRect.top = inDestPt.y;
	destRect.right = destRect.left + inWidth;
	destRect.bottom = destRect.top + inHeight;
	
	srcRect.left = inSourcePt.x;
	srcRect.top = inSourcePt.y;
	srcRect.right = srcRect.left + inWidth;
	srcRect.bottom = srcRect.top + inHeight;
	
	SET_PORT(inDest);
	
	::GetForeColor(&saveFore);
	::GetBackColor(&saveBack);
	::RGBForeColor(&blackRGB);
	::RGBBackColor(&whiteRGB);
	
#if TARGET_API_MAC_CARBON
	::CopyBits(::GetPortBitMapForCopyBits(((SImage *)inSource)->gw), ::GetPortBitMapForCopyBits(((SImage *)inDest)->gw), &srcRect, &destRect, srcCopy, nil);
#else	
	::CopyBits(&((GrafPtr)(((SImage *)inSource)->gw))->portBits, &((GrafPtr)(((SImage *)inDest)->gw))->portBits, &srcRect, &destRect, srcCopy, nil);
#endif

	::RGBForeColor(&saveFore);
	::RGBBackColor(&saveBack);
}

void UGraphics_StretchPixels(TImage inDest, const SRect& inDestRect, TImage inSource, const SRect& inSourceRect, Uint32 /* inOptions */)
{
	const RGBColor whiteRGB = { 0xFFFF, 0xFFFF, 0xFFFF };
	const RGBColor blackRGB = { 0x0000, 0x0000, 0x0000 };
	Rect destRect;
	Rect srcRect;
	RGBColor saveBack, saveFore;

	Require(inDest && inSource);
		
	destRect.left = inDestRect.left;
	destRect.top = inDestRect.top;
	destRect.right = inDestRect.right;
	destRect.bottom = inDestRect.bottom;
	
	srcRect.left = inSourceRect.left;
	srcRect.top = inSourceRect.top;
	srcRect.right = inSourceRect.right;
	srcRect.bottom = inSourceRect.bottom;

#if TARGET_API_MAC_CARBON
	Rect srcBounds;
	::GetPortBounds(((SImage *)inSource)->gw, &srcBounds);
#else
	Rect srcBounds = ((SImage *)inSource)->gw->portRect;
#endif

	if (srcRect.left < srcBounds.left || srcRect.top < srcBounds.top || srcRect.right > srcBounds.right || srcRect.bottom > srcBounds.bottom)
	{
		DebugBreak("UGraphics::StretchPixels - inSourceRect is not completely within the source image");
		return;
	}

	SET_PORT(inDest);
	
	::GetForeColor(&saveFore);
	::GetBackColor(&saveBack);
	::RGBForeColor(&blackRGB);
	::RGBBackColor(&whiteRGB);
	
#if TARGET_API_MAC_CARBON
	::CopyBits(::GetPortBitMapForCopyBits(((SImage *)inSource)->gw), ::GetPortBitMapForCopyBits(((SImage *)inDest)->gw), &srcRect, &destRect, srcCopy, nil);
#else
	::CopyBits(&((GrafPtr)(((SImage *)inSource)->gw))->portBits, &((GrafPtr)(((SImage *)inDest)->gw))->portBits, &srcRect, &destRect, srcCopy, nil);
#endif

	::RGBForeColor(&saveFore);
	::RGBBackColor(&saveBack);
}

void UGraphics::GetImageSize(TImage inImage, Uint32& outWidth, Uint32& outHeight)
{
	Require(inImage);
	
#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(((SImage *)inImage)->gw, &portRect);
#else
	Rect portRect = ((SImage *)inImage)->gw->portRect;
#endif
	
	outWidth = portRect.right - portRect.left;
	outHeight = portRect.bottom - portRect.top;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// outData is an array of Uint32s, each Uint32 is formatted like RGBA
// size in bytes of data written to outData is ((inRect.GetWidth() * sizeof(Uint32)) * inRect.GetHeight())
// if the TImage was created NewCompatibleImage() or similar, don't forget to lock it
void UGraphics::GetRawPixels(TImage inImage, const SRect& inRect, void *outData, Uint32 /* inOptions */)
{
	/*********************************************************
	
	Get rid of this function - incorporate into CopyPixels().
	(the one that copies from a TImage to SPixmap)
	
	**********************************************************/
	
	Uint32 rowBytes, rowCount, rLeft, rRight, rWid, i, n, pixelSize, prePix, postPix, midPix;
	Uint8 *rowPtr;
	Uint32 *op;
	ColorSpec *macColorTab;
	Uint32 colorTab[256];
	
	// get ptr to pixmap, note mustn't move memory after this b/c we didn't lock the handle
	Require(inImage && outData);

	// note that we're not locking so don't move memory!
#if TARGET_API_MAC_CARBON
	PixMap *pm = *::GetPortPixMap(((SImage *)inImage)->gw);
#else
	PixMap *pm = *((SImage *)inImage)->gw->portPixMap;
#endif
	
	pixelSize = pm->pixelSize;
	rowBytes = pm->rowBytes & 0x7FFF;
	rowCount = inRect.GetHeight();
	rLeft = inRect.left;
	rRight = inRect.right;
	rWid = inRect.GetWidth();
	rowPtr = BPTR(pm->baseAddr) + (rowBytes * inRect.top);
	op = (Uint32 *)outData;
	
	if (pixelSize == 32)
	{
		Uint32 *lp = ((Uint32 *)rowPtr) + rLeft;
		
		while (rowCount--)
		{
			for (i=0; i!=rWid; i++)
				op[i] = lp[i] << 8;
			
			op += rWid;
			
			(Uint8 *)lp += rowBytes;	// go down a row
		}
	}
	else if (pixelSize == 16)
	{
		Uint16 *wp = ((Uint16 *)rowPtr) + rLeft;
		
		while (rowCount--)
		{
			for (i=0; i!=rWid; i++)
				op[i] = _gConv5to8TabR[(wp[i] >> 10) & 0x1F] | _gConv5to8TabG[(wp[i] >> 5) & 0x1F] | _gConv5to8TabB[wp[i] & 0x1F];
			
			op += rWid;
			
			(Uint8 *)wp += rowBytes;	// go down a row
		}
	}
	else if (pixelSize == 8)
	{
		macColorTab = (**pm->pmTable).ctTable;
		n = (**pm->pmTable).ctSize + 1;
		
		if (n > 256) n = 256;
		
		for (i=0; i!=n; i++)
			colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
		
		for (i=n; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
			colorTab[i] = 0;
		
		Uint8 *bp = ((Uint8 *)rowPtr) + rLeft;
		
		while (rowCount--)
		{
			for (i=0; i!=rWid; i++)
				op[i] = colorTab[bp[i]];
			
			op += rWid;
			
			bp += rowBytes;		// go down a row
		}
	}
	else if (pixelSize == 4)
	{
		macColorTab = (**pm->pmTable).ctTable;
		n = (**pm->pmTable).ctSize + 1;
		
		if (n > 16) n = 16;
		
		for (i=0; i!=n; i++)
			colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
		
		for (i=n; i!=16; i++)		// clear any remaining entries in color table just in case color table is too short
			colorTab[i] = 0;
				
		prePix = rLeft - (rLeft & ~1);			// rLeft - ((rLeft / 2) * 2)
		postPix = rRight - (rRight & ~1);		// rRight - ((rRight / 2) * 2)
		midPix = rWid - prePix - postPix;
		
		n = midPix >> 1;						// midPix / 2
		rowPtr += rLeft >> 1;					// rLeft / 2
		
		while (rowCount--)
		{
			Uint8 *bp = rowPtr;
			
			if (prePix) *op++ = colorTab[*bp++ & 0x0F];
			
			for (i=0; i!=n; i++)
			{
				*op++ = colorTab[bp[i] >> 4];
				*op++ = colorTab[bp[i] & 0x0F];
			}
			
			if (postPix) *op++ = colorTab[bp[n] >> 4];
			
			rowPtr += rowBytes;		// go down a row
		}
	}
	else if (pixelSize == 2)
	{
		macColorTab = (**pm->pmTable).ctTable;
		n = (**pm->pmTable).ctSize + 1;
		
		if (n > 4) n = 4;
		
		for (i=0; i!=n; i++)
			colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
		
		for (i=n; i!=4; i++)		// clear any remaining entries in color table just in case color table is too short
			colorTab[i] = 0;
		
		prePix = rLeft - (rLeft & ~3);			// rLeft - ((rLeft / 4) * 4)
		if (prePix) prePix = 4 - prePix;
		postPix = rRight - (rRight & ~3);		// rRight - ((rRight / 4) * 4)
		midPix = rWid - prePix - postPix;
		
		n = midPix >> 2;						// midPix / 4
		rowPtr += rLeft >> 2;					// rLeft / 4
		
		while (rowCount--)
		{
			Uint8 *bp = rowPtr;
			
			switch (prePix)
			{
				case 1:
					 *op++ = colorTab[*bp++ & 3];
					break;
				case 2:
					*op++ = colorTab[(*bp >> 2) & 3];
					*op++ = colorTab[*bp & 3];
					bp++;
					break;
				case 3:
					*op++ = colorTab[(*bp >> 4) & 3];
					*op++ = colorTab[(*bp >> 2) & 3];
					*op++ = colorTab[*bp & 3];
					bp++;
					break;
			}
			
			for (i=0; i!=n; i++)
			{
				*op++ = colorTab[bp[i] >> 6];
				*op++ = colorTab[(bp[i] >> 4) & 3];
				*op++ = colorTab[(bp[i] >> 2) & 3];
				*op++ = colorTab[bp[i] & 3];
			}
			
			switch (postPix)
			{
				case 1:
					*op++ = colorTab[bp[n] >> 6];
					break;
				case 2:
					*op++ = colorTab[bp[n] >> 6];
					*op++ = colorTab[(bp[n] >> 4) & 3];
					break;
				case 3:
					*op++ = colorTab[bp[n] >> 6];
					*op++ = colorTab[(bp[n] >> 4) & 3];
					*op++ = colorTab[(bp[n] >> 2) & 3];
					break;
			}
			
			rowPtr += rowBytes;		// go down a row
		}
	}
	else if (pixelSize == 1)
	{
		Uint8 bv;
		
		macColorTab = (**pm->pmTable).ctTable;
		
		if ((**pm->pmTable).ctSize == 0)		// yes 0, not 1
		{
			colorTab[0] = (((Uint32)(macColorTab[0].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[0].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[0].rgb.blue) & 0xFF00);
			colorTab[1] = 0;
		}
		else
		{
			colorTab[0] = (((Uint32)(macColorTab[0].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[0].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[0].rgb.blue) & 0xFF00);
			colorTab[1] = (((Uint32)(macColorTab[1].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[1].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[1].rgb.blue) & 0xFF00);
		}
				
		prePix = rLeft - (rLeft & ~7);			// rLeft - ((rLeft / 8) * 8)
		if (prePix) prePix = 8 - prePix;
		postPix = rRight - (rRight & ~7);		// rRight - ((rRight / 8) * 8)
		midPix = rWid - prePix - postPix;
		
		n = midPix >> 3;						// midPix / 8
		rowPtr += rLeft >> 3;					// rLeft / 8
		
		while (rowCount--)
		{
			Uint8 *bp = rowPtr;
			
			if (prePix)
			{
				bv = *bp++;
				i = prePix;
				while (i--) *op++ = colorTab[(bv >> i) & 1];
			}
			
			for (i=0; i!=n; i++)
			{
				*op++ = colorTab[bp[i] >> 7];
				*op++ = colorTab[(bp[i] >> 6) & 1];
				*op++ = colorTab[(bp[i] >> 5) & 1];
				*op++ = colorTab[(bp[i] >> 4) & 1];
				*op++ = colorTab[(bp[i] >> 3) & 1];
				*op++ = colorTab[(bp[i] >> 2) & 1];
				*op++ = colorTab[(bp[i] >> 1) & 1];
				*op++ = colorTab[bp[i] & 1];
			}
			
			i = postPix;
			bv = bp[n];
			while (i)
			{
				*op++ = colorTab[(bv >> (8 - i)) & 1];
				i--;
			}
			
			rowPtr += rowBytes;		// go down a row
		}
	}
	else
		Fail(errorType_Misc, error_Unknown);
}

// inData is an array of Uint32s, each Uint32 is formatted like RGBA
// number of bytes inData should point to is ((inRect.GetWidth() * sizeof(Uint32)) * inRect.GetHeight())
// if the TImage was created NewCompatibleImage() or similar, don't forget to lock it
void UGraphics::SetRawPixels(TImage inImage, const SRect& inRect, const void *inData, Uint32 /* inOptions */)
{
	/*********************************************************
	
	Get rid of this function - incorporate into CopyPixels().
	(the one that copies from a SPixmap to a TImage)
	
	**********************************************************/

	Uint32 i, pixelSize, rowBytes, rowCount, rWid;
	Uint8 *rowPtr;
	Uint32 *pp = (Uint32 *)inData;
	
	Require(inImage && inData);
	
	// get ptr to pixmap
#if TARGET_API_MAC_CARBON
	PixMapHandle pmh = ::GetPortPixMap(((SImage *)inImage)->gw);
#else
	PixMapHandle pmh = ((SImage *)inImage)->gw->portPixMap;
#endif

	char saveState = ::HGetState((Handle)pmh);
	::HLock((Handle)pmh);
	PixMap *pm = *pmh;

	rWid = inRect.GetWidth();
	rowCount = inRect.GetHeight();
	pixelSize = pm->pixelSize;
	
	if (pixelSize == 32)
	{
		rowBytes = pm->rowBytes & 0x7FFF;
		rowPtr = BPTR(pm->baseAddr) + (rowBytes * inRect.top);
		
		Uint32 *lp = ((Uint32 *)rowPtr) + inRect.left;
		
		while (rowCount--)
		{
			for (i=0; i!=rWid; i++)
				 lp[i] = pp[i] >> 8;
			
			pp += rWid;
			
			(Uint8 *)lp += rowBytes;	// go down a row
		}
	}
	else if (pixelSize == 16)
	{
// incredibly cool tables for converting 8bit to 5bit without floating point calculations
static const Uint16 conv8to5TabR[] = {
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x0C00, 0x1000, 0x1000, 0x1000,
0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1000, 0x1400, 0x1400, 0x1400, 0x1400, 0x1400, 0x1400, 0x1400, 0x1400, 0x1800, 0x1800, 0x1800, 0x1800, 0x1800, 0x1800, 0x1800, 0x1800, 0x1C00, 0x1C00, 0x1C00, 0x1C00, 0x1C00, 0x1C00, 0x1C00, 0x1C00, 0x2000, 0x2000,
0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2400, 0x2400, 0x2400, 0x2400, 0x2400, 0x2400, 0x2400, 0x2400, 0x2400, 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x2C00, 0x3000,
0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3000, 0x3400, 0x3400, 0x3400, 0x3400, 0x3400, 0x3400, 0x3400, 0x3400, 0x3400, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00,
0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4400, 0x4400, 0x4400, 0x4400, 0x4400, 0x4400, 0x4400, 0x4400, 0x4800, 0x4800, 0x4800, 0x4800, 0x4800, 0x4800, 0x4800, 0x4800, 0x4800, 0x4C00, 0x4C00, 0x4C00, 0x4C00, 0x4C00, 0x4C00, 0x4C00,
0x4C00, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5000, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5400, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5800, 0x5C00, 0x5C00, 0x5C00, 0x5C00, 0x5C00, 0x5C00,
0x5C00, 0x5C00, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6400, 0x6800, 0x6800, 0x6800, 0x6800, 0x6800, 0x6800, 0x6800, 0x6800, 0x6C00, 0x6C00, 0x6C00, 0x6C00, 0x6C00, 0x6C00,
0x6C00, 0x6C00, 0x6C00, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7400, 0x7400, 0x7400, 0x7400, 0x7400, 0x7400, 0x7400, 0x7400, 0x7800, 0x7800, 0x7800, 0x7800, 0x7800, 0x7800, 0x7800, 0x7800, 0x7C00, 0x7C00, 0x7C00, 0x7C00, 0x7C00
};
static const Uint16 conv8to5TabG[] = {
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0040, 0x0040, 0x0040, 0x0040, 0x0040, 0x0040, 0x0040, 0x0040, 0x0060, 0x0060, 0x0060, 0x0060, 0x0060, 0x0060, 0x0060, 0x0060, 0x0080, 0x0080, 0x0080,
0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x00A0, 0x00A0, 0x00A0, 0x00A0, 0x00A0, 0x00A0, 0x00A0, 0x00A0, 0x00C0, 0x00C0, 0x00C0, 0x00C0, 0x00C0, 0x00C0, 0x00C0, 0x00C0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x0100, 0x0100,
0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0120, 0x0120, 0x0120, 0x0120, 0x0120, 0x0120, 0x0120, 0x0120, 0x0120, 0x0140, 0x0140, 0x0140, 0x0140, 0x0140, 0x0140, 0x0140, 0x0140, 0x0160, 0x0160, 0x0160, 0x0160, 0x0160, 0x0160, 0x0160, 0x0160, 0x0180,
0x0180, 0x0180, 0x0180, 0x0180, 0x0180, 0x0180, 0x0180, 0x01A0, 0x01A0, 0x01A0, 0x01A0, 0x01A0, 0x01A0, 0x01A0, 0x01A0, 0x01A0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01E0, 0x01E0, 0x01E0, 0x01E0, 0x01E0, 0x01E0, 0x01E0, 0x01E0,
0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0220, 0x0220, 0x0220, 0x0220, 0x0220, 0x0220, 0x0220, 0x0220, 0x0240, 0x0240, 0x0240, 0x0240, 0x0240, 0x0240, 0x0240, 0x0240, 0x0240, 0x0260, 0x0260, 0x0260, 0x0260, 0x0260, 0x0260, 0x0260,
0x0260, 0x0280, 0x0280, 0x0280, 0x0280, 0x0280, 0x0280, 0x0280, 0x0280, 0x02A0, 0x02A0, 0x02A0, 0x02A0, 0x02A0, 0x02A0, 0x02A0, 0x02A0, 0x02C0, 0x02C0, 0x02C0, 0x02C0, 0x02C0, 0x02C0, 0x02C0, 0x02C0, 0x02C0, 0x02E0, 0x02E0, 0x02E0, 0x02E0, 0x02E0, 0x02E0,
0x02E0, 0x02E0, 0x0300, 0x0300, 0x0300, 0x0300, 0x0300, 0x0300, 0x0300, 0x0300, 0x0320, 0x0320, 0x0320, 0x0320, 0x0320, 0x0320, 0x0320, 0x0320, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0340, 0x0360, 0x0360, 0x0360, 0x0360, 0x0360, 0x0360,
0x0360, 0x0360, 0x0360, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x03A0, 0x03A0, 0x03A0, 0x03A0, 0x03A0, 0x03A0, 0x03A0, 0x03A0, 0x03C0, 0x03C0, 0x03C0, 0x03C0, 0x03C0, 0x03C0, 0x03C0, 0x03C0, 0x03E0, 0x03E0, 0x03E0, 0x03E0, 0x03E0
};
static const Uint16 conv8to5TabB[] = {
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0004, 0x0004, 0x0004,
0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0005, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0007, 0x0008, 0x0008,
0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x000A, 0x000A, 0x000A, 0x000A, 0x000A, 0x000A, 0x000A, 0x000A, 0x000B, 0x000B, 0x000B, 0x000B, 0x000B, 0x000B, 0x000B, 0x000B, 0x000C,
0x000C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000C, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000E, 0x000E, 0x000E, 0x000E, 0x000E, 0x000E, 0x000E, 0x000E, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F,
0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013,
0x0013, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017,
0x0017, 0x0017, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x001A, 0x001A, 0x001A, 0x001A, 0x001A, 0x001A, 0x001A, 0x001A, 0x001B, 0x001B, 0x001B, 0x001B, 0x001B, 0x001B,
0x001B, 0x001B, 0x001B, 0x001C, 0x001C, 0x001C, 0x001C, 0x001C, 0x001C, 0x001C, 0x001C, 0x001D, 0x001D, 0x001D, 0x001D, 0x001D, 0x001D, 0x001D, 0x001D, 0x001E, 0x001E, 0x001E, 0x001E, 0x001E, 0x001E, 0x001E, 0x001E, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F
};
				
		rowBytes = pm->rowBytes & 0x7FFF;
		rowPtr = BPTR(pm->baseAddr) + (rowBytes * inRect.top);
	
		Uint16 *wp = ((Uint16 *)rowPtr) + inRect.left;
		
		while (rowCount--)
		{
			for (i=0; i!=rWid; i++)
				wp[i] = conv8to5TabR[pp[i] >> 24] | conv8to5TabG[(pp[i] >> 16) & 0xFF] | conv8to5TabB[(pp[i] >> 8) & 0xFF];

			pp += rWid;
			
			(Uint8 *)wp += rowBytes;	// go down a row
		}
	}
	else
	{
		PixMap srcMap;
		Rect destRect;
		Handle h;
		OSErr err;
		
		srcMap.rowBytes = (rWid << 2) | 0x8000;
		srcMap.bounds.top = srcMap.bounds.left = 0;
		srcMap.bounds.bottom = rowCount;
		srcMap.bounds.right = rWid;
		srcMap.pmVersion = 0;
		srcMap.packType = srcMap.packSize = 0;
		srcMap.hRes = srcMap.vRes = 72;
		srcMap.pixelType = 16;
		srcMap.pixelSize = 32;
		srcMap.cmpCount = 3;
		srcMap.cmpSize = 8;
	#if TARGET_API_MAC_CARBON
		srcMap.pixelFormat = 0;
	#else
		srcMap.planeBytes = 0;
	#endif
		srcMap.pmTable = 0;
	#if TARGET_API_MAC_CARBON
		srcMap.pmExt = nil;
	#else
		srcMap.pmReserved = 0;
	#endif
		
		destRect.top = inRect.top;
		destRect.left = inRect.left;
		destRect.bottom = inRect.bottom;
		destRect.right = inRect.right;
		
		i = (rWid << 2) * rowCount;
		h = ::TempNewHandle(i, &err);
		if (h == nil) h = ::NewHandle(i);
		
		if (h)
		{
			::HLock(h);
			
			Uint32 *lp = (Uint32 *)*h;
			srcMap.baseAddr = (Ptr)lp;

			rWid *= rowCount;
			for (i=0; i!=rWid; i++)
				 lp[i] = pp[i] >> 8;
			
			::CopyBits((BitMap *)&srcMap, (BitMap *)pm, &srcMap.bounds, &destRect, srcCopy, nil);
			::DisposeHandle(h);
		}
	}
	
	HSetState((Handle)pmh, saveState);
}

// examine a rect of an image and build the best color table to match those pixels
// don't forget to lock the image if necessary
Uint32 UGraphics::BuildColorTable(TImage inImage, const SRect& inRect, Uint32 inMethod, Uint32 *outColors, Uint32 inMaxColors)
{
	Uint32 i, n, colorCount, rLeft, rWid, rowBytes, rowCount;
	Uint8 *rowPtr;
	ColorSpec *macColorTab;
	Uint32 colorTab[256];

	Require(inImage && inMaxColors);
	ASSERT(outColors);

	// note that we're not locking so don't move memory!
#if TARGET_API_MAC_CARBON
	PixMap *pm = *::GetPortPixMap(((SImage *)inImage)->gw);
#else	
	PixMap *pm = *((SImage *)inImage)->gw->portPixMap;
#endif

	rowBytes = pm->rowBytes & 0x7FFF;
	rowPtr = BPTR(pm->baseAddr) + (rowBytes * inRect.top);
	rowCount = inRect.GetHeight();
	rLeft = inRect.left;
	rWid = inRect.GetWidth();
	colorCount = 0;
	
	/*
	 * Method 1 is a simple "minimize" algorithm that puts each unique color
	 * into the color table.  This is fast, but the quality can be poor if
	 * there are more unique colors than will fit in the color table.  A higher
	 * quality method could look for the most commonly used colors, eg like
	 * a histogram.
	 */
	Require(inMethod == 1);
	
	switch (pm->pixelSize)
	{
		case 32:
		{
			Uint32 *lp = ((Uint32 *)rowPtr) + rLeft;
			
			while (rowCount--)
			{
				for (i=0; i!=rWid; i++)
				{
					Uint32 v = lp[i] << 8;
					Uint32 d = colorCount;
					Uint32 *p = outColors;
					
					while (d--)
					{
						if (*p++ == v)
							goto nextCol32;
					}
					
					if (colorCount == inMaxColors) break;
					outColors[colorCount++] = v;
					
					nextCol32:;
				}
								
				(Uint8 *)lp += rowBytes;	// go down a row
			}
		}
		break;
		
		case 16:
		{
			Uint16 *wp = ((Uint16 *)rowPtr) + rLeft;
			
			while (rowCount--)
			{
				for (i=0; i!=rWid; i++)
				{
					Uint32 v = _gConv5to8TabR[(wp[i] >> 10) & 0x1F] | _gConv5to8TabG[(wp[i] >> 5) & 0x1F] | _gConv5to8TabB[wp[i] & 0x1F];
					Uint32 d = colorCount;
					Uint32 *p = outColors;
					
					while (d--)
					{
						if (*p++ == v)
							goto nextCol16;
					}
					
					if (colorCount == inMaxColors) break;
					outColors[colorCount++] = v;
					
					nextCol16:;
				}
				
				(Uint8 *)wp += rowBytes;	// go down a row
			}
		}
		break;
		
		case 8:
		{
			macColorTab = (**pm->pmTable).ctTable;
			n = (**pm->pmTable).ctSize + 1;
			
			if (n > 256) n = 256;
			
			for (i=0; i!=n; i++)
				colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
			
			for (i=n; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
				colorTab[i] = 0;
			
			Uint8 *bp = ((Uint8 *)rowPtr) + rLeft;

			while (rowCount--)
			{
				for (i=0; i!=rWid; i++)
				{
					Uint32 v = colorTab[bp[i]];
					Uint32 d = colorCount;
					Uint32 *p = outColors;
					
					while (d--)
					{
						if (*p++ == v)
							goto nextCol8;
					}
					
					if (colorCount == inMaxColors) break;
					outColors[colorCount++] = v;
					
					nextCol8:;
				}
				
				(Uint8 *)bp += rowBytes;	// go down a row
			}
		}
		break;
		
		case 4:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 2:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 1:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		default:
		Fail(errorType_Misc, error_Unknown);
		break;
	}
	
	return colorCount;
}

bool UGraphics::UserSelectColor(SColor& ioColor, const Uint8 *inPrompt, Uint32 /* inOptions */)
{
	Point where = { 0, 0 };		// center
	Int16 ref = ::CurResFile();	// get around bug (?) in toolbox

	bool b = ::GetColor(where, inPrompt ? inPrompt : "\pChoose a color:", (RGBColor *)&ioColor, (RGBColor *)&ioColor);
	
	::UseResFile(ref);
	return b;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void _SetVirtualOrigin(TImage inImage, const SPoint& inVirtualOrigin)
{
	ASSERT(inImage);

	((SImage *)inImage)->virtualOrigin = inVirtualOrigin;
}

void _GetVirtualOrigin(TImage inImage, SPoint& outVirtualOrigin)
{
	ASSERT(inImage);

	outVirtualOrigin = ((SImage *)inImage)->virtualOrigin;
}

void _AddVirtualOrigin(TImage inImage, const SPoint& inVirtualOrigin)
{
	ASSERT(inImage);

	((SImage *)inImage)->virtualOrigin.x += inVirtualOrigin.x;
	((SImage *)inImage)->virtualOrigin.y += inVirtualOrigin.y;
}

void _ResetVirtualOrigin(TImage inImage)
{
	ASSERT(inImage);

	((SImage *)inImage)->virtualOrigin.x = 0;
	((SImage *)inImage)->virtualOrigin.y = 0;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// copy MAC 32 (ARGB) to AW 32 (RGBA)
static void _GRCopyM32To32(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
			((Uint32 *)inDst)[i] = ((Uint32 *)inSrc)[i] << 8;
		
		(Uint8 *)inSrc += inSrcRowBytes;
		(Uint8 *)inDst += inDstRowBytes;
	}
}

// copy AW 32 (RGBA) to MAC 32 (ARGB)
static void _GRCopy32ToM32(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
			((Uint32 *)inDst)[i] = ((Uint32 *)inSrc)[i] >> 8;
		
		(Uint8 *)inSrc += inSrcRowBytes;
		(Uint8 *)inDst += inDstRowBytes;
	}
}

// copy AW 24 (RGB) to MAC 32 (ARGB)
static void _GRCopy24ToM32(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint32 endPad = inSrcRowBytes - inWid * 3;
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			((Uint32 *)inDst)[i] = *((Uint32 *)inSrc) >> 8;
			(Uint8 *)inSrc += 3;
		}
		
		(Uint8 *)inSrc += endPad;
		(Uint8 *)inDst += inDstRowBytes;
	}
}

// copy AW (also MAC) 16 (ARRRRRGGGGGBBBBB) to MAC 32 (ARGB)
static void _GRCopy16ToM32(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint16 *sp = (Uint16 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
			dp[i] = _gConv5to8TabMR[(sp[i] >> 10) & 0x1F] | _gConv5to8TabMG[(sp[i] >> 5) & 0x1F] | _gConv5to8TabMB[sp[i] & 0x1F];
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy MAC 32 (ARGB) to AW (also MAC) 16 (ARRRRRGGGGGBBBBB)
static void _GRCopyM32To16(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint16 *dp = (Uint16 *)inDst;
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
			dp[i] = (((sp[i] >> 9) & 0x7C00) | ((sp[i] >> 6) & 0x03E0) | ((sp[i] >> 3) & 0x001F));
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRCopy16ToM8(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors)
{
	Uint16 *sp = (Uint16 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 colorCount = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (colorCount > 256) colorCount = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=colorCount; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);

	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
		{
			Uint32 v = _gConv5to8TabR[(sp[i] >> 10) & 0x1F] | _gConv5to8TabG[(sp[i] >> 5) & 0x1F] | _gConv5to8TabB[sp[i] & 0x1F];
			Uint32 d = colorCount;
			const Uint32 *p = colorTab;
			
			while (d--)
			{
				if (*p++ == v)
				{
					dp[i] = p - colorTab - 1;
					goto nextPix;
				}
			}

			dp[i] = UGraphics::GetNearestColor(v, colorTab, colorCount);
			
			nextPix:;
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy MAC 32 (ARGB) to any 8
static void _GRCopyM32To8(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inDstColors, Uint32 inDstColCount)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			Uint32 v = sp[i] << 8;
			Uint32 d = inDstColCount;
			const Uint32 *p = inDstColors;
			
			while (d--)
			{
				if (*p++ == v)
				{
					dp[i] = p - inDstColors - 1;
					goto nextPix;
				}
			}

			dp[i] = UGraphics::GetNearestColor(v, inDstColors, inDstColCount);
			
			nextPix:;
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRCopy32ToM8(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 colorCount = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (colorCount > 256) colorCount = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=colorCount; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);

	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
		{
			Uint32 v = sp[i];
			Uint32 d = colorCount;
			const Uint32 *p = colorTab;
			
			while (d--)
			{
				if (*p++ == v)
				{
					dp[i] = p - colorTab - 1;
					goto nextPix;
				}
			}

			dp[i] = UGraphics::GetNearestColor(v, colorTab, colorCount);
			
			nextPix:;
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRCopy24ToM8(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 colorCount = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (colorCount > 256) colorCount = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=colorCount; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);

	while (inRowCount--)
	{
		Uint8 *spp = BPTR(sp);
		for (i=0; i!=inWid; i++)
		{
			Uint32 v = *(Uint32 *)spp << 8;
			spp += 3;
			Uint32 d = colorCount;
			const Uint32 *p = colorTab;
			
			while (d--)
			{
				if (*p++ == v)
				{
					dp[i] = p - colorTab - 1;
					goto nextPix;
				}
			}

			dp[i] = UGraphics::GetNearestColor(v, colorTab, colorCount);
			
			nextPix:;
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}



static void _GRCopyM8To32(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inSrcColors)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
	
	ColorSpec *macColorTab = (**inSrcColors).ctTable;
	Uint32 n = (**inSrcColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	for (i=n; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
		colorTab[i] = 0;

	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
			dp[i] = colorTab[sp[i]];
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRCopy8ToM32(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inSrcColors, Uint32 inSrcColorCount)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
	Uint32 i;
	
	Uint32 colorTab[256];

	if (inSrcColorCount > 256) inSrcColorCount = 256;

	for (i=0; i!=inSrcColorCount; i++)
		colorTab[i] = inSrcColors[i] >> 8;
	
	for (i=inSrcColorCount; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
		colorTab[i] = 0;

	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
			dp[i] = colorTab[sp[i]];
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRCopyM8To16(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inSrcColors)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint16 *dp = (Uint16 *)inDst;
	
	ColorSpec *macColorTab = (**inSrcColors).ctTable;
	Uint32 n = (**inSrcColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	
	Uint16 colorTab[256];

	for (i=0; i!=n; i++)
		colorTab[i] = ((macColorTab[i].rgb.red >> 11) << 10) | ((macColorTab[i].rgb.green >> 11) << 5) | (macColorTab[i].rgb.blue >> 11);
	
	for (i=n; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
		colorTab[i] = 0;

	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
			dp[i] = colorTab[sp[i]];
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRCopyM8To8(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inDstColors, Uint32 inDstColCount, CTabHandle inSrcColors)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inSrcColors).ctTable;
	Uint32 n = (**inSrcColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	if (inDstColCount > 256) inDstColCount = 256;
	
	Uint32 colorTab[256];
	Uint8 xlat[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	if (n == inDstColCount && UMemory::Equal(colorTab, inDstColors, n * sizeof(Uint32)))	// if exactly the same color tab
	{
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
				dp[i] = sp[i];
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
	else
	{
		for (i=0; i!=n; i++)
			xlat[i] = UGraphics::GetNearestColor(colorTab[i], inDstColors, inDstColCount);
		
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
				dp[i] = xlat[sp[i]];
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
}

static void _GRCopy8ToM8(void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors, const Uint32 *inSrcColors, Uint32 inSrcColCount)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 n = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	if (inSrcColCount > 256) inSrcColCount = 256;
	
	Uint32 colorTab[256];
	Uint8 xlat[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	if (n == inSrcColCount && UMemory::Equal(colorTab, inSrcColors, n * sizeof(Uint32)))	// if exactly the same color tab
	{
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
				dp[i] = sp[i];
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
	else
	{
		for (i=0; i!=inSrcColCount; i++)
			xlat[i] = UGraphics::GetNearestColor(inSrcColors[i], colorTab, n);
		
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
				dp[i] = xlat[sp[i]];
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
}

void UGraphics_CopyPixels(const SPixmap& inDest, const SPoint& inDestPt, TImage inSource, const SPoint& inSourcePt, Uint32 inWidth, Uint32 inHeight, Uint32 /* inOptions */)
{
	// sanity checks on inSource
	Require(inSource);
	
	// sanity checks on inDest
	if (!UPixmap::IsValid(inDest))
	{
		DebugBreak("UGraphics::CopyPixels - invalid dest pixmap");
		Fail(errorType_Misc, error_Param);
	}
	
	// note that we're not locking so don't move memory!
#if TARGET_API_MAC_CARBON
	PixMap *srcPM = *::GetPortPixMap(((SImage *)inSource)->gw);
#else
	PixMap *srcPM = *((SImage *)inSource)->gw->portPixMap;
#endif

	// validate the copy rects
	SPoint theDestPt = inDestPt;
	SPoint theSourcePt = inSourcePt;
	SRect dstBnd(0, 0, inDest.width, inDest.height);
	SRect srcBnd(srcPM->bounds.left, srcPM->bounds.top, srcPM->bounds.right, srcPM->bounds.bottom);
	if (!UGraphics::ValidateCopyRects(dstBnd, srcBnd, theDestPt, theSourcePt, inWidth, inHeight))
		return;
	
	// get info about the source pixmap
	Uint32 srcRowBytes = srcPM->rowBytes & 0x7FFF;
	Uint8 *srcRowPtr = BPTR(srcPM->baseAddr) + (srcRowBytes * theSourcePt.y);

	// switch on depth and perform appropriate copy
	switch (srcPM->pixelSize)
	{
		case 32:
		{
			switch (inDest.depth)
			{
				case 32:
					_GRCopyM32To32(((Uint32 *)inDest.data) + theDestPt.x, inDest.rowBytes, ((Uint32 *)srcRowPtr) + theSourcePt.x, srcRowBytes, inWidth, inHeight);
					break;
				case 16:
					_GRCopyM32To16(((Uint16 *)inDest.data) + theDestPt.x, inDest.rowBytes, ((Uint32 *)srcRowPtr) + theSourcePt.x, srcRowBytes, inWidth, inHeight);
					break;
				case 8:
					_GRCopyM32To8(((Uint8 *)inDest.data) + theDestPt.x, inDest.rowBytes, ((Uint32 *)srcRowPtr) + theSourcePt.x, srcRowBytes, inWidth, inHeight, inDest.colorTab, inDest.colorCount);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Param);
					break;
			}
		}
		break;
		
		case 16:
		{
			switch (inDest.depth)
			{
				case 32:
					_GRCopy16To32(((Uint32 *)inDest.data) + theDestPt.x, inDest.rowBytes, ((Uint16 *)srcRowPtr) + theSourcePt.x, srcRowBytes, inWidth, inHeight);
					break;
				case 16:
					_GRCopy16To16(((Uint16 *)inDest.data) + theDestPt.x, inDest.rowBytes, ((Uint16 *)srcRowPtr) + theSourcePt.x, srcRowBytes, inWidth, inHeight);
					break;
				case 8:
					_GRCopy16To8(((Uint8 *)inDest.data) + theDestPt.x, inDest.rowBytes, ((Uint16 *)srcRowPtr) + theSourcePt.x, srcRowBytes, inWidth, inHeight, inDest.colorTab, inDest.colorCount);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Param);
					break;
			}
		}
		break;
		
		case 8:
		{
			switch (inDest.depth)
			{
				case 32:
					_GRCopyM8To32(((Uint32 *)inDest.data) + theDestPt.x, inDest.rowBytes, ((Uint8 *)srcRowPtr) + theSourcePt.x, srcRowBytes, inWidth, inHeight, srcPM->pmTable);
					break;
				case 16:
					_GRCopyM8To16(((Uint16 *)inDest.data) + theDestPt.x, inDest.rowBytes, ((Uint8 *)srcRowPtr) + theSourcePt.x, srcRowBytes, inWidth, inHeight, srcPM->pmTable);
					break;
				case 8:
					_GRCopyM8To8(((Uint8 *)inDest.data) + theDestPt.x, inDest.rowBytes, ((Uint8 *)srcRowPtr) + theSourcePt.x, srcRowBytes, inWidth, inHeight, inDest.colorTab, inDest.colorCount, srcPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Param);
					break;
			}
		}
		break;
		
		case 4:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 2:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 1:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		default:
		Fail(errorType_Misc, error_Unknown);
		break;
	}
}

void UGraphics_CopyPixels(TImage inDest, const SPoint& inDestPt, const SPixmap& inSource, const SPoint& inSourcePt, Uint32 inWidth, Uint32 inHeight, Uint32 /* inOptions */)
{
	// sanity checks on inDest
	Require(inDest);

	// sanity checks on inSource
	if (!UPixmap::IsValid(inSource))
	{
		DebugBreak("UGraphics::CopyPixels - invalid source pixmap");
		Fail(errorType_Misc, error_Param);
	}

	// note that we're not locking so don't move memory!	
#if TARGET_API_MAC_CARBON
	PixMap *dstPM = *::GetPortPixMap(((SImage *)inDest)->gw);
#else
	PixMap *dstPM = *((SImage *)inDest)->gw->portPixMap;
#endif

	// validate the copy rects
	SPoint theDestPt = inDestPt;
	SPoint theSourcePt = inSourcePt;
	SRect srcBnd(0, 0, inSource.width, inSource.height);
	SRect dstBnd(dstPM->bounds.left, dstPM->bounds.top, dstPM->bounds.right, dstPM->bounds.bottom);
	if (!UGraphics::ValidateCopyRects(dstBnd, srcBnd, theDestPt, theSourcePt, inWidth, inHeight))
		return;

	// get info about the dest pixmap
	Uint32 dstRowBytes = dstPM->rowBytes & 0x7FFF;
	Uint8 *dstRowPtr = BPTR(dstPM->baseAddr) + (dstRowBytes * theDestPt.y);
	Uint8 *srcRowPtr = (Uint8 *)inSource.data + theSourcePt.y * inSource.rowBytes;

	// switch on depth and perform appropriate copy
	switch (inSource.depth)
	{
		case 32:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRCopy32ToM32(((Uint32 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint32 *)srcRowPtr) + theSourcePt.x, inSource.rowBytes, inWidth, inHeight);
					break;
				case 16:
					_GRCopy32To16(((Uint16 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint32 *)srcRowPtr) + theSourcePt.x, inSource.rowBytes, inWidth, inHeight);
					break;
				case 8:
					_GRCopy32ToM8(((Uint8 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint32 *)srcRowPtr) + theSourcePt.x, inSource.rowBytes, inWidth, inHeight, dstPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Unknown);
					break;
			}
		}
		break;
		
		case 24:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRCopy24ToM32(((Uint32 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint8 *)srcRowPtr) + 3 * theSourcePt.x, inSource.rowBytes, inWidth, inHeight);
					break;
				case 16:
					_GRCopy24To16(((Uint16 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint8 *)srcRowPtr) + 3 * theSourcePt.x, inSource.rowBytes, inWidth, inHeight);
					break;
				case 8:
					_GRCopy24ToM8(((Uint8 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint8 *)srcRowPtr) + 3 * theSourcePt.x, inSource.rowBytes, inWidth, inHeight, dstPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
					
				default:
					Fail(errorType_Misc, error_Unknown);
					break;
			}
		}
		break;
		
		case 16:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRCopy16ToM32(((Uint32 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint16 *)srcRowPtr) + theSourcePt.x, inSource.rowBytes, inWidth, inHeight);
					break;
				case 16:
					_GRCopy16To16(((Uint16 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint16 *)srcRowPtr) + theSourcePt.x, inSource.rowBytes, inWidth, inHeight);
					break;
				case 8:
					_GRCopy16ToM8(((Uint8 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint16 *)srcRowPtr) + theSourcePt.x, inSource.rowBytes, inWidth, inHeight, dstPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Unknown);
					break;
			}
		}
		break;
		
		case 8:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRCopy8ToM32(((Uint32 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint8 *)srcRowPtr) + theSourcePt.x, inSource.rowBytes, inWidth, inHeight, inSource.colorTab, inSource.colorCount);
					break;
				case 16:
					_GRCopy8To16(((Uint16 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint8 *)srcRowPtr) + theSourcePt.x, inSource.rowBytes, inWidth, inHeight, inSource.colorTab, inSource.colorCount);
					break;
				case 8:
					_GRCopy8ToM8(((Uint8 *)dstRowPtr) + theDestPt.x, dstRowBytes, ((Uint8 *)srcRowPtr) + theSourcePt.x, inSource.rowBytes, inWidth, inHeight, dstPM->pmTable, inSource.colorTab, inSource.colorCount);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Unknown);
					break;
			}
		}
		break;
		
		case 4:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 2:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 1:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;

		default:
		Fail(errorType_Misc, error_Param);
		break;
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// copy MAC 32 (ARGB) to MAC 32 with a mask
static void _GRTransCopyM32To32(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint32 *mask = (Uint32 *)inMask;
	Uint32 *sp = (Uint32 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			if (mask[i] == 0)
				dp[i] = sp[i];
		}
		
		(Uint8 *)mask += inSrcRowBytes;
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy MAC 32 (ARGB) to MAC 32
static void _GRTransCopyM32To32(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			if (sp[i] != inTransCol)
				dp[i] = sp[i];
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy AW 32 (RGBA) to MAC 32 (ARGB)
static void _GRTransCopy32ToM32(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;

	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			if (sp[i] != inTransCol)
				dp[i] = sp[i] >> 8;
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy AW (also MAC) 16 (ARRRRRGGGGGBBBBB) to MAC 32 (ARGB) with a mask
static void _GRTransCopy16ToM32(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint16 *mask = (Uint16 *)inMask;
	Uint16 *sp = (Uint16 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
		
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			if (mask[i] == 0)
				dp[i] = _gConv5to8TabMR[(sp[i] >> 10) & 0x1F] | _gConv5to8TabMG[(sp[i] >> 5) & 0x1F] | _gConv5to8TabMB[sp[i] & 0x1F];
		}
		
		(Uint8 *)mask += inSrcRowBytes;
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy AW (also MAC) 16 (ARRRRRGGGGGBBBBB) to MAC 32 (ARGB)
static void _GRTransCopy16ToM32(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint16 *sp = (Uint16 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
	
	Uint16 transCol = (((inTransCol >> 17) & 0x7C00) | ((inTransCol >> 14) & 0x03E0) | ((inTransCol >> 11) & 0x001F));
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			if (sp[i] != transCol)
				dp[i] = _gConv5to8TabMR[(sp[i] >> 10) & 0x1F] | _gConv5to8TabMG[(sp[i] >> 5) & 0x1F] | _gConv5to8TabMB[sp[i] & 0x1F];
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy MAC 32 (ARGB) to AW (also MAC) 16 (ARRRRRGGGGGBBBBB) with a mask
static void _GRTransCopyM32To16(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint32 *mask = (Uint32 *)inMask;
	Uint32 *sp = (Uint32 *)inSrc;
	Uint16 *dp = (Uint16 *)inDst;
	
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			if (mask[i] == 0)
				dp[i] = (((sp[i] >> 9) & 0x7C00) | ((sp[i] >> 6) & 0x03E0) | ((sp[i] >> 3) & 0x001F));
		}
		
		(Uint8 *)mask += inSrcRowBytes;
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy MAC 32 (ARGB) to AW (also MAC) 16 (ARRRRRGGGGGBBBBB)
static void _GRTransCopyM32To16(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint16 *dp = (Uint16 *)inDst;
	
	inTransCol >>= 8;
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			if (sp[i] != inTransCol)
				dp[i] = (((sp[i] >> 9) & 0x7C00) | ((sp[i] >> 6) & 0x03E0) | ((sp[i] >> 3) & 0x001F));
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy AW (also MAC) 16 (ARRRRRGGGGGBBBBB) to MAC 8 with a mask
static void _GRTransCopy16ToM8(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors)
{
	Uint16 *mask = (Uint16 *)inMask;
	Uint16 *sp = (Uint16 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 colorCount = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (colorCount > 256) colorCount = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=colorCount; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			if (mask[i] == 0)
			{
				// ******* this could be optimized somewhat by having a version of GetNearestColor() that works on a 16-bit color table
				Uint32 v = _gConv5to8TabR[(sp[i] >> 10) & 0x1F] | _gConv5to8TabG[(sp[i] >> 5) & 0x1F] | _gConv5to8TabB[sp[i] & 0x1F];
				Uint32 d = colorCount;
				const Uint32 *p = colorTab;
				
				while (d--)
				{
					if (*p++ == v)
					{
						dp[i] = p - colorTab - 1;
						goto nextPix;
					}
				}

				dp[i] = UGraphics::GetNearestColor(v, colorTab, colorCount);
			}
			
			nextPix:;
		}
		
		(Uint8 *)mask += inSrcRowBytes;
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy AW (also MAC) 16 (ARRRRRGGGGGBBBBB) to MAC 8
static void _GRTransCopy16ToM8(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors)
{
	Uint16 *sp = (Uint16 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 colorCount = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (colorCount > 256) colorCount = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=colorCount; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);

	Uint16 transCol = (((inTransCol >> 17) & 0x7C00) | ((inTransCol >> 14) & 0x03E0) | ((inTransCol >> 11) & 0x001F));
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			if (sp[i] != transCol)
			{
				// ******* this could be optimized somewhat by having a version of GetNearestColor() that works on a 16-bit color table
				Uint32 v = _gConv5to8TabR[(sp[i] >> 10) & 0x1F] | _gConv5to8TabG[(sp[i] >> 5) & 0x1F] | _gConv5to8TabB[sp[i] & 0x1F];
				Uint32 d = colorCount;
				const Uint32 *p = colorTab;
				
				while (d--)
				{
					if (*p++ == v)
					{
						dp[i] = p - colorTab - 1;
						goto nextPix;
					}
				}

				dp[i] = UGraphics::GetNearestColor(v, colorTab, colorCount);
			}
			
			nextPix:;
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy MAC 32 (ARGB) to any 8
static void _GRTransCopyM32To8(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inDstColors, Uint32 inDstColCount)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	while (inRowCount--)
	{
		for (Uint32 i=0; i!=inWid; i++)
		{
			Uint32 v = sp[i] << 8;
			if (v != inTransCol)
			{
				Uint32 d = inDstColCount;
				const Uint32 *p = inDstColors;
				
				while (d--)
				{
					if (*p++ == v)
					{
						dp[i] = p - inDstColors - 1;
						goto nextPix;
					}
				}

				dp[i] = UGraphics::GetNearestColor(v, inDstColors, inDstColCount);
			}
			
			nextPix:;
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy MAC 32 (ARGB) to MAC 8 with a mask
static void _GRTransCopyM32ToM8(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors)
{
	Uint32 *mask = (Uint32 *)inMask;
	Uint32 *sp = (Uint32 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 colorCount = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (colorCount > 256) colorCount = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=colorCount; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
		{
			if (mask[i] == 0)
			{
				Uint32 v = sp[i] << 8;
				Uint32 d = colorCount;
				const Uint32 *p = colorTab;
				
				while (d--)
				{
					if (*p++ == v)
					{
						dp[i] = p - colorTab - 1;
						goto nextPix;
					}
				}

				dp[i] = UGraphics::GetNearestColor(v, colorTab, colorCount);
			}
			
			nextPix:;
		}
		
		(Uint8 *)mask += inSrcRowBytes;
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

// copy MAC 32 (ARGB) to MAC 8
static void _GRTransCopyM32ToM8(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 colorCount = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (colorCount > 256) colorCount = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=colorCount; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
		{
			Uint32 v = sp[i] << 8;
			if (v != inTransCol)
			{
				Uint32 d = colorCount;
				const Uint32 *p = colorTab;
				
				while (d--)
				{
					if (*p++ == v)
					{
						dp[i] = p - colorTab - 1;
						goto nextPix;
					}
				}

				dp[i] = UGraphics::GetNearestColor(v, colorTab, colorCount);
			}
			
			nextPix:;
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRTransCopy32ToM8(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors)
{
	Uint32 *sp = (Uint32 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 colorCount = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (colorCount > 256) colorCount = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=colorCount; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);

	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
		{
			Uint32 v = sp[i];
			if (v != inTransCol)
			{
				Uint32 d = colorCount;
				const Uint32 *p = colorTab;
				
				while (d--)
				{
					if (*p++ == v)
					{
						dp[i] = p - colorTab - 1;
						goto nextPix;
					}
				}

				dp[i] = UGraphics::GetNearestColor(v, colorTab, colorCount);
			}
			
			nextPix:;
		}
		
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRTransCopyM8To32(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inSrcColors)
{
	Uint8 *mask = (Uint8 *)inMask;
	Uint8 *sp = (Uint8 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
	
	ColorSpec *macColorTab = (**inSrcColors).ctTable;
	Uint32 n = (**inSrcColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	for (i=n; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
		colorTab[i] = 0;

	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
		{
			if (mask[i] == 0)
				dp[i] = colorTab[sp[i]];
		}
		
		(Uint8 *)mask += inSrcRowBytes;
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRTransCopyM8To32(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inSrcColors)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
	
	ColorSpec *macColorTab = (**inSrcColors).ctTable;
	Uint32 n = (**inSrcColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	for (i=n; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
		colorTab[i] = 0;

	if (UGraphics::GetExactColor(inTransCol, colorTab, n, i))
	{
		Uint8 transCol = i;
		
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
			{
				if (sp[i] != transCol)
					dp[i] = colorTab[sp[i]];
			}
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
	else
	{
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
				dp[i] = colorTab[sp[i]];
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
}

static void _GRTransCopy8ToM32(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inSrcColors, Uint32 inSrcColorCount)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint32 *dp = (Uint32 *)inDst;
	Uint32 i;
	
	Uint32 colorTab[256];

	if (inSrcColorCount > 256) inSrcColorCount = 256;

	for (i=0; i!=inSrcColorCount; i++)
		colorTab[i] = inSrcColors[i] >> 8;
	
	for (i=inSrcColorCount; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
		colorTab[i] = 0;

	if (UGraphics::GetExactColor(inTransCol, inSrcColors, inSrcColorCount, i))
	{
		Uint8 transCol = i;

		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
			{
				if (sp[i] != transCol)
					dp[i] = colorTab[sp[i]];
			}
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
	else
	{
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
				dp[i] = colorTab[sp[i]];
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
}

static void _GRTransCopyM8To16(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inSrcColors)
{
	Uint8 *mask = (Uint8 *)inMask;
	Uint8 *sp = (Uint8 *)inSrc;
	Uint16 *dp = (Uint16 *)inDst;
	
	ColorSpec *macColorTab = (**inSrcColors).ctTable;
	Uint32 n = (**inSrcColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	
	Uint16 colorTab[256];

	for (i=0; i!=n; i++)
		colorTab[i] = ((macColorTab[i].rgb.red >> 11) << 10) | ((macColorTab[i].rgb.green >> 11) << 5) | (macColorTab[i].rgb.blue >> 11);
	
	for (i=n; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
		colorTab[i] = 0;

	while (inRowCount--)
	{
		for (i=0; i!=inWid; i++)
		{
			if (mask[i] == 0)
				dp[i] = colorTab[sp[i]];
		}
			
		(Uint8 *)mask += inSrcRowBytes;
		(Uint8 *)sp += inSrcRowBytes;
		(Uint8 *)dp += inDstRowBytes;
	}
}

static void _GRTransCopyM8To16(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inSrcColors)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint16 *dp = (Uint16 *)inDst;
	
	ColorSpec *macColorTab = (**inSrcColors).ctTable;
	Uint32 n = (**inSrcColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	
	Uint16 colorTab[256];

	for (i=0; i!=n; i++)
		colorTab[i] = ((macColorTab[i].rgb.red >> 11) << 10) | ((macColorTab[i].rgb.green >> 11) << 5) | (macColorTab[i].rgb.blue >> 11);
	
	for (i=n; i!=256; i++)		// clear any remaining entries in color table just in case color table is too short
		colorTab[i] = 0;

	if (UGraphics::GetExactColor((((inTransCol >> 17) & 0x7C00) | ((inTransCol >> 14) & 0x03E0) | ((inTransCol >> 11) & 0x001F)), colorTab, n, i))
	{
		Uint8 transCol = i;

		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
			{
				if (sp[i] != transCol)
					dp[i] = colorTab[sp[i]];
			}
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
	else
	{
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
				dp[i] = colorTab[sp[i]];
			
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
}

static void _GRTransCopyM8To8(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inDstColors, Uint32 inDstColCount, CTabHandle inSrcColors)
{
	Uint8 *mask = (Uint8 *)inMask;
	Uint8 *sp = (Uint8 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inSrcColors).ctTable;
	Uint32 n = (**inSrcColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	if (inDstColCount > 256) inDstColCount = 256;
	
	Uint32 colorTab[256];
	Uint8 xlat[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	if (n == inDstColCount && UMemory::Equal(colorTab, inDstColors, n * sizeof(Uint32)))	// if exactly the same color tab
	{
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
			{
				if (mask[i] == 0)
					dp[i] = sp[i];
			}
				
			(Uint8 *)mask += inSrcRowBytes;
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
	else
	{
		for (i=0; i!=n; i++)
			xlat[i] = UGraphics::GetNearestColor(colorTab[i], inDstColors, inDstColCount);
		
		while (inRowCount--)
		{
			for (i=0; i!=inWid; i++)
			{
				if (mask[i] == 0)
					dp[i] = xlat[sp[i]];
			}
				
			(Uint8 *)mask += inSrcRowBytes;
			(Uint8 *)sp += inSrcRowBytes;
			(Uint8 *)dp += inDstRowBytes;
		}
	}
}

static void _GRTransCopyM8To8(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, const Uint32 *inDstColors, Uint32 inDstColCount, CTabHandle inSrcColors)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inSrcColors).ctTable;
	Uint32 n = (**inSrcColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	if (inDstColCount > 256) inDstColCount = 256;
	
	Uint32 colorTab[256];
	Uint8 xlat[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	if (n == inDstColCount && UMemory::Equal(colorTab, inDstColors, n * sizeof(Uint32)))	// if exactly the same color tab
	{
		if (UGraphics::GetExactColor(inTransCol, colorTab, n, i))
		{
			Uint8 transCol = i;
			
			while (inRowCount--)
			{
				for (i=0; i!=inWid; i++)
				{
					if (sp[i] != transCol)
						dp[i] = sp[i];
				}
				
				(Uint8 *)sp += inSrcRowBytes;
				(Uint8 *)dp += inDstRowBytes;
			}
		}
		else
		{
			while (inRowCount--)
			{
				for (i=0; i!=inWid; i++)
					dp[i] = sp[i];
				
				(Uint8 *)sp += inSrcRowBytes;
				(Uint8 *)dp += inDstRowBytes;
			}
		}
	}
	else
	{
		for (i=0; i!=n; i++)
			xlat[i] = UGraphics::GetNearestColor(colorTab[i], inDstColors, inDstColCount);
		
		if (UGraphics::GetExactColor(inTransCol, colorTab, n, i))
		{
			Uint8 transCol = i;

			while (inRowCount--)
			{
				for (i=0; i!=inWid; i++)
				{
					if (sp[i] != transCol)
						dp[i] = xlat[sp[i]];
				}
				
				(Uint8 *)sp += inSrcRowBytes;
				(Uint8 *)dp += inDstRowBytes;
			}
		}
		else
		{
			while (inRowCount--)
			{
				for (i=0; i!=inWid; i++)
					dp[i] = xlat[sp[i]];
				
				(Uint8 *)sp += inSrcRowBytes;
				(Uint8 *)dp += inDstRowBytes;
			}
		}
	}
}

static void _GRTransCopyM8ToM8(const void *inMask, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors, CTabHandle inSrcColors)
{
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 n = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);

	_GRTransCopyM8To8(inMask, inDst, inDstRowBytes, inSrc, inSrcRowBytes, inWid, inRowCount, colorTab, n, inSrcColors);
}

static void _GRTransCopyM8ToM8(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors, CTabHandle inSrcColors)
{
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 n = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	
	Uint32 colorTab[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);

	_GRTransCopyM8To8(inTransCol, inDst, inDstRowBytes, inSrc, inSrcRowBytes, inWid, inRowCount, colorTab, n, inSrcColors);
}

static void _GRTransCopy8ToM8(Uint32 inTransCol, void *inDst, Uint32 inDstRowBytes, const void *inSrc, Uint32 inSrcRowBytes, Uint32 inWid, Uint32 inRowCount, CTabHandle inDstColors, const Uint32 *inSrcColors, Uint32 inSrcColCount)
{
	Uint8 *sp = (Uint8 *)inSrc;
	Uint8 *dp = (Uint8 *)inDst;
	
	ColorSpec *macColorTab = (**inDstColors).ctTable;
	Uint32 n = (**inDstColors).ctSize + 1;
	Uint32 i;
	
	if (n > 256) n = 256;
	if (inSrcColCount > 256) inSrcColCount = 256;
	
	Uint32 colorTab[256];
	Uint8 xlat[256];

	for (i=0; i!=n; i++)
		colorTab[i] = (((Uint32)(macColorTab[i].rgb.red >> 8)) << 24) | (((Uint32)(macColorTab[i].rgb.green >> 8)) << 16) | (((Uint32)macColorTab[i].rgb.blue) & 0xFF00);
	
	if (n == inSrcColCount && UMemory::Equal(colorTab, inSrcColors, n * sizeof(Uint32)))	// if exactly the same color tab
	{
		if (UGraphics::GetExactColor(inTransCol, inSrcColors, inSrcColCount, i))
		{
			Uint8 transCol = i;
			
			while (inRowCount--)
			{
				for (i=0; i!=inWid; i++)
				{
					if (sp[i] != transCol)
						dp[i] = sp[i];
				}
				
				(Uint8 *)sp += inSrcRowBytes;
				(Uint8 *)dp += inDstRowBytes;
			}
		}
		else
		{
			while (inRowCount--)
			{
				for (i=0; i!=inWid; i++)
					dp[i] = sp[i];
				
				(Uint8 *)sp += inSrcRowBytes;
				(Uint8 *)dp += inDstRowBytes;
			}
		}
	}
	else
	{
		for (i=0; i!=inSrcColCount; i++)
			xlat[i] = UGraphics::GetNearestColor(inSrcColors[i], colorTab, n);
		
		if (UGraphics::GetExactColor(inTransCol, inSrcColors, inSrcColCount, i))
		{
			Uint8 transCol = i;
			
			while (inRowCount--)
			{
				for (i=0; i!=inWid; i++)
				{
					if (sp[i] != transCol)
						dp[i] = xlat[sp[i]];
				}
				
				(Uint8 *)sp += inSrcRowBytes;
				(Uint8 *)dp += inDstRowBytes;
			}
		}
		else
		{
			while (inRowCount--)
			{
				for (i=0; i!=inWid; i++)
					dp[i] = xlat[sp[i]];
				
				(Uint8 *)sp += inSrcRowBytes;
				(Uint8 *)dp += inDstRowBytes;
			}
		}
	}
}

void UGraphics_CopyPixelsMasked(TImage inDest, const SPoint& inDestPt, TImage inSource, const SPoint& inSourcePt, Uint32 inWidth, Uint32 inHeight, TImage inTransMask, Uint32 /*inOptions*/)
{
	Require(inDest && inSource && inTransMask);

#if TARGET_API_MAC_CARBON
	PixMap *maskPM = *::GetPortPixMap(((SImage *)inTransMask)->gw);
	PixMap *srcPM = *::GetPortPixMap(((SImage *)inSource)->gw);
	PixMap *dstPM = *::GetPortPixMap(((SImage *)inDest)->gw);
#else
	PixMap *maskPM = *((SImage *)inTransMask)->gw->portPixMap;
	PixMap *srcPM = *((SImage *)inSource)->gw->portPixMap;
	PixMap *dstPM = *((SImage *)inDest)->gw->portPixMap;
#endif
	
	SRect maskBounds(maskPM->bounds.left, maskPM->bounds.top, maskPM->bounds.right, maskPM->bounds.bottom);
	SRect srcBounds(srcPM->bounds.left, srcPM->bounds.top, srcPM->bounds.right, srcPM->bounds.bottom);
	SRect dstBounds(dstPM->bounds.left, dstPM->bounds.top, dstPM->bounds.right, dstPM->bounds.bottom);
	
	if (srcBounds != maskBounds)
		return;
	
	SPoint theSourcePt = inSourcePt;
	SPoint theDestPt = inDestPt;
	
	if (!UGraphics::ValidateCopyRects(dstBounds, srcBounds, theDestPt, theSourcePt, inWidth, inHeight))
		return;
		
	Uint32 maskRowBytes = maskPM->rowBytes & 0x7FFF;
	Uint8 *maskRowPtr = BPTR(maskPM->baseAddr) + (maskRowBytes * (theSourcePt.y - maskBounds.top));

	Uint32 srcRowBytes = srcPM->rowBytes & 0x7FFF;
	Uint8 *srcRowPtr = BPTR(srcPM->baseAddr) + (srcRowBytes * (theSourcePt.y - srcBounds.top));

	Uint32 dstRowBytes = dstPM->rowBytes & 0x7FFF;
	Uint8 *dstRowPtr = BPTR(dstPM->baseAddr) + (dstRowBytes * (theDestPt.y - dstBounds.top));

	Int32 maskPixLeft = theSourcePt.x - maskBounds.left;
	Int32 srcPixLeft = theSourcePt.x - srcBounds.left;
	Int32 dstPixLeft = theDestPt.x - dstBounds.left;
	
	switch (srcPM->pixelSize)
	{
		case 32:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRTransCopyM32To32(((Uint32 *)maskRowPtr) + maskPixLeft, ((Uint32 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint32 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight);
					break;
				case 16:
					_GRTransCopyM32To16(((Uint32 *)maskRowPtr) + maskPixLeft, ((Uint16 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint32 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight);
					break;
				case 8:
					_GRTransCopyM32ToM8(((Uint32 *)maskRowPtr) + maskPixLeft, ((Uint8 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint32 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, dstPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Param);
					break;
			}
		}
		break;
		
		case 16:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRTransCopy16ToM32(((Uint16 *)maskRowPtr) + maskPixLeft, ((Uint32 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint16 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight);
					break;
				case 16:
					_GRTransCopy16To16(((Uint16 *)maskRowPtr) + maskPixLeft, ((Uint16 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint16 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight);
					break;
				case 8:
					_GRTransCopy16ToM8(((Uint16 *)maskRowPtr) + maskPixLeft, ((Uint8 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint16 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, dstPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Param);
					break;
			}
		}
		break;
		
		case 8:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRTransCopyM8To32(((Uint8 *)maskRowPtr) + maskPixLeft, ((Uint32 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint8 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, srcPM->pmTable);
					break;
				case 16:
					_GRTransCopyM8To16(((Uint8 *)maskRowPtr) + maskPixLeft, ((Uint16 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint8 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, srcPM->pmTable);
					break;
				case 8:
					_GRTransCopyM8ToM8(((Uint8 *)maskRowPtr) + maskPixLeft, ((Uint8 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint8 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, dstPM->pmTable, srcPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Param);
					break;
			}
		}
		break;
		
		case 4:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 2:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 1:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		default:
		Fail(errorType_Misc, error_Unknown);
		break;
	}
}

void UGraphics_CopyPixelsTrans(TImage inDest, const SPoint& inDestPt, TImage inSource, const SPoint& inSourcePt, Uint32 inWidth, Uint32 inHeight, Uint32 inTransCol, Uint32 /* inOptions */)
{
	Require(inDest && inSource);

	// note that we're not locking so don't move memory!
#if TARGET_API_MAC_CARBON
	PixMap *dstPM = *::GetPortPixMap(((SImage *)inDest)->gw);
	PixMap *srcPM = *::GetPortPixMap(((SImage *)inSource)->gw);
#else
	PixMap *dstPM = *((SImage *)inDest)->gw->portPixMap;
	PixMap *srcPM = *((SImage *)inSource)->gw->portPixMap;
#endif
	
	SRect srcBounds(srcPM->bounds.left, srcPM->bounds.top, srcPM->bounds.right, srcPM->bounds.bottom);
	SRect dstBounds(dstPM->bounds.left, dstPM->bounds.top, dstPM->bounds.right, dstPM->bounds.bottom);
	
	SPoint theSourcePt = inSourcePt;
	SPoint theDestPt = inDestPt;
	
	if (!UGraphics::ValidateCopyRects(dstBounds, srcBounds, theDestPt, theSourcePt, inWidth, inHeight))
		return;
		
	Uint32 dstRowBytes = dstPM->rowBytes & 0x7FFF;
	Uint8 *dstRowPtr = BPTR(dstPM->baseAddr) + (dstRowBytes * (theDestPt.y - dstBounds.top));

	Uint32 srcRowBytes = srcPM->rowBytes & 0x7FFF;
	Uint8 *srcRowPtr = BPTR(srcPM->baseAddr) + (srcRowBytes * (theSourcePt.y - srcBounds.top));
	
	Int32 srcPixLeft = theSourcePt.x - srcBounds.left;
	Int32 dstPixLeft = theDestPt.x - dstBounds.left;
	
	switch (srcPM->pixelSize)
	{
		case 32:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRTransCopyM32To32(inTransCol, ((Uint32 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint32 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight);
					break;
				case 16:
					_GRTransCopyM32To16(inTransCol, ((Uint16 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint32 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight);
					break;
				case 8:
					_GRTransCopyM32ToM8(inTransCol, ((Uint8 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint32 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, dstPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Param);
					break;
			}
		}
		break;
		
		case 16:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRTransCopy16ToM32(inTransCol, ((Uint32 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint16 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight);
					break;
				case 16:
					_GRTransCopy16To16(inTransCol, ((Uint16 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint16 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight);
					break;
				case 8:
					_GRTransCopy16ToM8(inTransCol, ((Uint8 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint16 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, dstPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Param);
					break;
			}
		}
		break;
		
		case 8:
		{
			switch (dstPM->pixelSize)
			{
				case 32:
					_GRTransCopyM8To32(inTransCol, ((Uint32 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint8 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, srcPM->pmTable);
					break;
				case 16:
					_GRTransCopyM8To16(inTransCol, ((Uint16 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint8 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, srcPM->pmTable);
					break;
				case 8:
					_GRTransCopyM8ToM8(inTransCol, ((Uint8 *)dstRowPtr) + dstPixLeft, dstRowBytes, ((Uint8 *)srcRowPtr) + srcPixLeft, srcRowBytes, inWidth, inHeight, dstPM->pmTable, srcPM->pmTable);
					break;
				case 4:
				case 2:
				case 1:
					Fail(errorType_Misc, error_Unimplemented);
					break;
				default:
					Fail(errorType_Misc, error_Param);
					break;
			}
		}
		break;
		
		case 4:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 2:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		case 1:
		{
			Fail(errorType_Misc, error_Unimplemented);
		}
		break;
		
		default:
		Fail(errorType_Misc, error_Unknown);
		break;
	}
}

void UGraphics_CopyPixelsTrans(const SPixmap& inDest, const SPoint& inDestPt, TImage inSource, const SPoint& inSourcePt, Uint32 inWidth, Uint32 inHeight, Uint32 inTransCol, Uint32 /* inOptions */)
{
	#pragma unused(inDest, inDestPt, inSource, inSourcePt, inWidth, inHeight, inTransCol)
	
	// don't forget to use ValidateCopyRects
	
	Fail(errorType_Misc, error_Unimplemented);
}

void UGraphics_CopyPixelsTrans(TImage inDest, const SPoint& inDestPt, const SPixmap& inSource, const SPoint& inSourcePt, Uint32 inWidth, Uint32 inHeight, Uint32 inTransCol, Uint32 /* inOptions */)
{
	#pragma unused(inDest, inDestPt, inSource, inSourcePt, inWidth, inHeight, inTransCol)
	
	// don't forget to use ValidateCopyRects

	Fail(errorType_Misc, error_Unimplemented);
}

/* -------------------------------------------------------------------------- */
#pragma mark -
#pragma mark �� Old Stuff ��

void UGraphics::SetPattern(TImage inImage, TPattern inPat)
{
	SET_PORT(inImage);
	FAIL_IF_INVALID_PAT(inPat);

	::PenPixPat((PixPatHandle)inPat);
	CheckQDError();
}

void UGraphics::SetPattern(TImage inImage, const SSimplePattern& inPat)
{
	SET_PORT(inImage);
	
	::PenPat((PatPtr)&inPat);
}

void UGraphics::GetPattern(TImage inImage, TPattern outPat)
{
	FAIL_IF_INVALID_PAT(outPat);

#if TARGET_API_MAC_CARBON	
	::CopyPixPat(::GetPortPenPixPat(((SImage *)inImage)->gw, nil), (PixPatHandle)outPat);
#else
	::CopyPixPat(((SImage *)inImage)->gw->pnPixPat, (PixPatHandle)outPat);
#endif

	CheckQDError();
}

void UGraphics::SetBackPattern(TImage inImage, TPattern inPat)
{
	SET_PORT(inImage);
	FAIL_IF_INVALID_PAT(inPat);

	::BackPixPat((PixPatHandle)inPat);
	CheckQDError();
}

void UGraphics::SetBackPattern(TImage inImage, const SSimplePattern& inPat)
{
	SET_PORT(inImage);
	
	::BackPat((PatPtr)&inPat);
}

void UGraphics::GetBackPattern(TImage inImage, TPattern outPat)
{
	FAIL_IF_INVALID_PAT(outPat);

#if TARGET_API_MAC_CARBON	
	::CopyPixPat(::GetPortBackPixPat(((SImage *)inImage)->gw, nil), (PixPatHandle)outPat);
#else
	::CopyPixPat(((SImage *)inImage)->gw->bkPixPat, (PixPatHandle)outPat);
#endif

	CheckQDError();
}

// inBounds is in global coords
/*
void UGraphics::SetBounds(TImage inImage, const SRect& inBounds)
{
	SET_PORT(inImage);

	::MovePortTo(inBounds.left, inBounds.top);
	::PortSize(inBounds.GetWidth(), inBounds.GetHeight());
}
*/

// outBounds is in global coords
void UGraphics::GetBounds(TImage inImage, SRect& outBounds)
{
	SET_PORT(inImage);

#if TARGET_API_MAC_CARBON
	Rect portRect;
	::GetPortBounds(((SImage *)inImage)->gw, &portRect);
#else
	Rect portRect = qd.thePort->portRect;
#endif
	
	Point pt = { 0, 0 };
	::LocalToGlobal(&pt);
	
	::OffsetRect(&portRect, pt.h, pt.v);
	
	outBounds.top = portRect.top;
	outBounds.left = portRect.left;
	outBounds.bottom = portRect.bottom;
	outBounds.right = portRect.right;
}

void UGraphics::SetBackColor(TImage inImage, const SColor& inColor)
{
	SET_PORT(inImage);

	::RGBBackColor((RGBColor *)&inColor);
}

void UGraphics::GetBackColor(TImage inImage, SColor& outColor)
{
	SET_PORT(inImage);

	::GetBackColor((RGBColor *)&outColor);
}

void UGraphics::SetWhiteBack(TImage inImage)
{
	SetBackColor(inImage, color_White);
	SetBackPattern(inImage, pattern_White);
}

void UGraphics::SetColorToBack(TImage inImage)
{
	SET_PORT(inImage);

	RGBColor rgb;
	::GetBackColor(&rgb);
	::RGBForeColor(&rgb);
}

/*
 * New() creates a new offscreen image that can be drawn in.  The image will
 * be the same height and width of <inBounds>, and its pixel depth will be
 * the same as the deepest graphics device intersecting <inBounds> (which
 * should be in global/screen coordinates).  You MUST call LockImage() before
 * drawing to the image.
 */
TImage UGraphics::NewImage(const SRect& inBounds)
{
	GWorldPtr gw = nil;
	Rect r = {inBounds.top, inBounds.left, inBounds.bottom, inBounds.right};
	
	CheckMacError(::NewGWorld(&gw, 0, &r, nil, nil, noNewDevice | useTempMem));
	
	SImage *img = (SImage *)UMemory::NewClear(sizeof(SImage));
	img->gw = gw;
	img->lockCount=0;
	
	return (TImage)img;
}

void UGraphics::CopyPixels(TImage inDestPort, const SRect& inDestRect, TImage inSourcePort, const SRect& inSourceRect, Int16 inMode, TRegion inClip)
{
	FAIL_IF_INVALID_PORT(inDestPort);
	FAIL_IF_INVALID_PORT(inSourcePort);
	if (inClip) FAIL_IF_INVALID_RGN(inClip);

#if TARGET_API_MAC_CARBON
	RgnHandle visRgn = ::NewRgn();
	if (visRgn == nil)
		return;

	::GetPortVisibleRegion(((SImage *)inDestPort)->gw, visRgn);
#endif

	RgnHandle rgn = nil;
	Rect destRect = { inDestRect.top, inDestRect.left, inDestRect.bottom, inDestRect.right };
	Rect srcRect = { inSourceRect.top, inSourceRect.left, inSourceRect.bottom, inSourceRect.right };
	
	if (inClip)
	{
		rgn = ::NewRgn();
		if (rgn == nil)
		{
		#if TARGET_API_MAC_CARBON
			::DisposeRgn(visRgn);
		#endif
			return;
		}
	
	#if TARGET_API_MAC_CARBON
		::UnionRgn((RgnHandle)inClip, visRgn, rgn);
	#else
		::UnionRgn((RgnHandle)inClip, ((SImage *)inDestPort)->gw->visRgn, rgn);
	#endif
	}
	
#if TARGET_API_MAC_CARBON	
	::CopyBits(::GetPortBitMapForCopyBits(((SImage *)inSourcePort)->gw), ::GetPortBitMapForCopyBits(((SImage *)inDestPort)->gw), &srcRect, &destRect, inMode, rgn == nil ? visRgn : rgn);
#else	
	::CopyBits(&((GrafPtr)(((SImage *)inSourcePort)->gw))->portBits, &((GrafPtr)(((SImage *)inDestPort)->gw))->portBits, &srcRect, &destRect, inMode, rgn == nil ? ((SImage *)inDestPort)->gw->visRgn : rgn);
#endif

	if (rgn) ::DisposeRgn(rgn);
#if TARGET_API_MAC_CARBON
	::DisposeRgn(visRgn);
#endif
}

void UGraphics::ScrollPixels(TImage inImage, const SRect& inSourceRect, Int16 inDistHoriz, Int16 inDistVert, TRegion outUpdateRgn)
{
	SET_PORT(inImage);
	FAIL_IF_INVALID_RGN(outUpdateRgn);
	
	Rect r = { inSourceRect.top, inSourceRect.left, inSourceRect.bottom, inSourceRect.right };

	::ScrollRect(&r, inDistHoriz, inDistVert, (RgnHandle)outUpdateRgn);
}

// correctly handles purged pictures
void UGraphics::DrawPicture(TImage inImage, TPicture inPict, const SRect& inDestRect)
{
	if (inPict && *inPict)
	{
		SET_PORT(inImage);
		FAIL_IF_INVALID_PICT(inPict);
	
		Rect r;
		Int8 saveState;
		Int16 n;
		
		saveState = HGetState((Handle)inPict);
		HNoPurge((Handle)inPict);					// don't want pict purged while drawing
				
		// center the picture within its bounds
		r = (**(PicHandle)inPict).picFrame;
		n = r.right - r.left;
		r.left = (inDestRect.left + inDestRect.right - n) / 2;
		r.right = r.left + n;
		n = r.bottom - r.top;
		r.top = (inDestRect.top + inDestRect.bottom - n) / 2;
		r.bottom = r.top + n;
		
		::DrawPicture((PicHandle)inPict, &r);
		
		HSetState((Handle)inPict, saveState);
	}
}

void UGraphics::DrawPicture(TImage inImage, Int16 inPictID, const SRect& inDestRect)
{
	PicHandle pic;
	Rect r;
	Int16 n;
	Int8 saveState;
		
	pic = (PicHandle)::GetResource('PICT', inPictID);
	if (pic == nil || *pic == nil) return;
	
	saveState = HGetState((Handle)pic);
	HNoPurge((Handle)pic);
	
	// center the picture within its bounds
	r = (**pic).picFrame;
	n = r.right - r.left;
	r.left = (inDestRect.left + inDestRect.right - n) / 2;
	r.right = r.left + n;
	n = r.bottom - r.top;
	r.top = (inDestRect.top + inDestRect.bottom - n) / 2;
	r.bottom = r.top + n;
	
	SET_PORT(inImage);
	::DrawPicture((PicHandle)pic, &r);
	
	HSetState((Handle)pic, saveState);
}

/*
TPicture UGraphics::GetPicture(const SFileSpec& inFile)
{
	Uint32 s;
	TPicture h;
	
	// open file
	StFile ref(inFile);
	UFile::Open(ref, perm_Read);

	// determine size of picture
	s = UFile::GetSize(ref);
	if (s <= 512) FailMacError(eofErr);
	s -= 512;							// don't want header
	
	// alloc memory for picture
	h = (TPicture)UMemory::NewHandle(s);
	UMemory::Lock((THdl)h);
	
	try
	{
		// read the picture in
		UFile::SetPos(ref, 512);		// move past header
		UFile::Read(ref, *h, s);		// read everything else
	}
	catch(...)
	{
		// clean up
		UMemory::Dispose((THdl)h);
		throw;
	}
	
	// all done
	UMemory::Unlock((THdl)h);
	return h;
}
*/

Int32 UGraphics::GetPictureHeight(TPicture inPict)
{
	if (inPict)
		return (**inPict).macPict.picFrame.bottom - (**inPict).macPict.picFrame.top;
		
	return 0;
}

Int32 UGraphics::GetPictureWidth(TPicture inPict)
{
	if (inPict)
		return (**inPict).macPict.picFrame.right - (**inPict).macPict.picFrame.left;
		
	return 0;
}

TImage UGraphics::GetDummyImage()
{
	Init();
	
	return (TImage)&_gDummyImage;
}

TImage UGraphics::GetDesktopImage()
{
	Init();
		
	return (TImage)&_gDesktopImage;
}

TPattern UGraphics::GetResPattern(Int16 inID)
{
	PixPatHandle h = ::GetPixPat(inID);
	if (h == nil)
	{
		CheckMacError(ResError());
		Fail(errorType_Memory, memError_NotEnough);
	}
	
	return (TPattern)h;
}

void UGraphics::DisposePattern(TPattern inPat)
{
	if (inPat) 
		DisposePixPat((PixPatHandle)inPat);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void _FillRectWithPattern(TImage inImage, const SRect& inBounds, TPattern inPat)
{
	Rect r = { inBounds.top, inBounds.left, inBounds.bottom, inBounds.right };
	
	SET_PORT(inImage);
	::FillCRect(&r, (PixPatHandle)inPat);
}

// returns a PicHandle
void *_ImageToMacPict(TImage inImage, const SRect *inRect)
{
	const RGBColor whiteRGB = { 0xFFFF, 0xFFFF, 0xFFFF };
	const RGBColor blackRGB = { 0x0000, 0x0000, 0x0000 };
	GrafPtr port = (GrafPtr)((SImage *)inImage)->gw;
	PicHandle h;
	Rect r;
	RGBColor saveBack, saveFore;

	Require(inImage);
	SET_PORT(inImage);

	if (inRect)
	{
		r.top = inRect->top;
		r.left = inRect->left;
		r.bottom = inRect->bottom;
		r.right = inRect->right;
	}
	else
	{
	#if TARGET_API_MAC_CARBON
		::GetPortBounds(port, &r);
	#else
		r = port->portRect;
	#endif
	}	
	
	h = ::OpenPicture(&r);
	if (h == nil) Fail(errorType_Memory, memError_NotEnough);
	
	::GetForeColor(&saveFore);
	::GetBackColor(&saveBack);
	::RGBForeColor(&blackRGB);
	::RGBBackColor(&whiteRGB);

#if TARGET_API_MAC_CARBON	
	::CopyBits(::GetPortBitMapForCopyBits(port), ::GetPortBitMapForCopyBits(port), &r, &r, srcCopy, nil);
#else
	::CopyBits(&port->portBits, &port->portBits, &r, &r, srcCopy, nil);
#endif
	
	::RGBForeColor(&saveFore);
	::RGBBackColor(&saveBack);

	::ClosePicture();
	
	return h;
}

// inHdl is a PicHandle
TImage _MacPictToImage(void *inHdl, SRect *outRect)
{
	Require(inHdl);
	
	Rect r = (**(PicHandle)inHdl).picFrame;
	r.right -= r.left;
	r.left = 0;
	r.bottom -= r.top;
	r.top = 0;
	
	if (outRect) 
		outRect->Set(0, 0, r.right, r.bottom);
	
	TImage image = UGraphics::NewCompatibleImage(r.right, r.bottom);
	SET_PORT(image);
	
	::EraseRect(&r);
	::DrawPicture((PicHandle)inHdl, &r);
	
	return image;
}

GrafPtr _ImageToGrafPtr(TImage inImage)
{
	return (GrafPtr)((SImage *)inImage)->gw;
}

TImage _NewGWImage(GWorldPtr inGW)
{
	SImage *img = (SImage *)UMemory::NewClear(sizeof(SImage));
	img->gw = inGW;
	img->lockCount=0;
	
	return (TImage)img;
}

// does not dispose the actual GWorld
void _DisposeGWImage(TImage inImage)
{
	if (inImage)
		UMemory::Dispose((TPtr)inImage);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static Int16 _FontNameToMacNum(const Uint8 *inName)
{
	Int16 macNum;
	
	if (inName == kDefaultFont || inName == nil)
		macNum = 1;		// Geneva
	else if (inName == kSystemFont)
		macNum = 0;		// Chicago
	else if (inName == kFixedFont)
		macNum = 4;		// Monaco
	else if (inName == kSansFont)
	{
	#if TARGET_API_MAC_CARBON
		macNum = ::FMGetFontFamilyFromName("\pHelvetica");
		if (macNum == kInvalidFontFamily) macNum = 1;	// if not available, use geneva
	#else
		::GetFNum("\pHelvetica", &macNum);
		if (macNum == 0) macNum = 1;					// if not available, use geneva
	#endif
	}
	else if (inName == kSerifFont)
	{
	#if TARGET_API_MAC_CARBON
		macNum = ::FMGetFontFamilyFromName("\pTimes");
		if (macNum == kInvalidFontFamily) macNum = 1;	// if not available, use geneva
	#else
		::GetFNum("\pTimes", &macNum);
		if (macNum == 0) macNum = 1;					// if not available, use geneva
	#endif
	}
	else
	{
	#if TARGET_API_MAC_CARBON
		macNum = ::FMGetFontFamilyFromName(inName);
		if (macNum == kInvalidFontFamily) macNum = 0;
	#else
		::GetFNum(inName, &macNum);
	#endif
	}
	
	return macNum;
}

static Uint16 _FontEffectToMacStyle(Uint32 inEffect)
{
	Uint16 macStyle = 0;
	
	if (inEffect)
	{
		if (inEffect & fontEffect_Bold) macStyle |= bold;
		if (inEffect & fontEffect_Italic) macStyle |= italic;
		if (inEffect & fontEffect_Underline) macStyle |= underline;
		if (inEffect & fontEffect_Outline) macStyle |= outline;
		if (inEffect & fontEffect_Condense) macStyle |= condense;
		if (inEffect & fontEffect_Extend) macStyle |= extend;
	}
	
	return macStyle;
}

TFontDesc UFontDesc::New(const SFontDesc& inInfo)
{
	Int16 macNum;
	Uint16 macStyle;
		
	macNum = _FontNameToMacNum(inInfo.name);
	macStyle = _FontEffectToMacStyle(inInfo.effect);
	
	SFontDescObj *fd = (SFontDescObj *)UMemory::New(sizeof(SFontDescObj));
	
	fd->macNum = macNum;
	fd->macStyle = macStyle;
	fd->size = inInfo.size;
	fd->effect = inInfo.effect;
	fd->align = inInfo.textAlign;
	fd->customVal = inInfo.customVal;
	fd->locked = false;
	
	if (inInfo.color)
		fd->color = *inInfo.color;
	else
		fd->color.Set(0);
	
	if (inInfo.customColor)
		fd->customColor = *inInfo.customColor;
	else
		fd->customColor.Set(0);
	
	return (TFontDesc)fd;
}

TFontDesc UFontDesc::New(const Uint8 *inName, const Uint8 */* inStyle */, Uint32 inSize, Uint32 inEffect)
{
	Int16 macNum;
	Uint16 macStyle;
		
	macNum = _FontNameToMacNum(inName);
	macStyle = _FontEffectToMacStyle(inEffect);
	
	SFontDescObj *fd = (SFontDescObj *)UMemory::NewClear(sizeof(SFontDescObj));
	
	fd->macNum = macNum;
	fd->macStyle = macStyle;
	fd->size = inSize;
	fd->effect = inEffect;
	fd->align = textAlign_Left;
	
	return (TFontDesc)fd;
}

void UFontDesc::Dispose(TFontDesc inFD)
{
	if (inFD && !FD->locked)
		UMemory::Dispose((TPtr)inFD);
}

TFontDesc UFontDesc::Clone(TFontDesc inFD)
{
	Require(inFD);
	
	return (TFontDesc)UMemory::New(inFD, sizeof(SFontDescObj));
}

// can't dispose or modify if locked
void UFontDesc::SetLock(TFontDesc inFD, bool inLock)
{
	Require(inFD);
	
	FD->locked = (inLock != 0);
}

bool UFontDesc::IsLocked(TFontDesc inFD)
{
	return inFD && FD->locked;
}

void UFontDesc::SetDefault(const SFontDesc& inInfo)
{
	SFontDescObj *fd = (SFontDescObj *)UFontDesc::New(inInfo);
	
	_gDefaultFontDesc = *fd;
	UMemory::Dispose((TPtr)fd);
}

void UFontDesc::SetFontName(TFontDesc inFD, const Uint8 *inName, const Uint8 */* inStyle */)
{
	Require(inFD);
	
	if (FD->locked) Fail(errorType_Misc, error_Locked);
	FD->macNum = _FontNameToMacNum(inName);
}

// returns true if changed
bool UFontDesc::SetFontSize(TFontDesc inFD, Uint32 inSize)
{
	Require(inFD);
	if (FD->locked) Fail(errorType_Misc, error_Locked);
	
	if (inSize != FD->size)
	{
		FD->size = inSize;
		return true;
	}
	
	return false;
}

Uint32 UFontDesc::GetFontSize(TFontDesc inFD)
{
	return inFD ? FD->size : _gDefaultFontDesc.size;
}

void UFontDesc::SetFontEffect(TFontDesc inFD, Uint32 inFlags)
{
	Require(inFD);
	if (FD->locked) Fail(errorType_Misc, error_Locked);

	FD->effect = inFlags;
	
	Uint16 macStyle = 0;
	if (inFlags)
	{
		if (inFlags & fontEffect_Bold) macStyle |= bold;
		if (inFlags & fontEffect_Italic) macStyle |= italic;
		if (inFlags & fontEffect_Underline) macStyle |= underline;
		if (inFlags & fontEffect_Outline) macStyle |= outline;
		if (inFlags & fontEffect_Condense) macStyle |= condense;
		if (inFlags & fontEffect_Extend) macStyle |= extend;
	}
	
	FD->macStyle = macStyle;
}

Uint32 UFontDesc::GetFontEffect(TFontDesc inFD)
{
	return inFD ? FD->effect : _gDefaultFontDesc.effect;
}

void UFontDesc::SetAlign(TFontDesc inFD, Uint32 inAlign)
{
	Require(inFD);
	if (FD->locked) Fail(errorType_Misc, error_Locked);
	FD->align = inAlign;
}

Uint32 UFontDesc::GetAlign(TFontDesc inFD)
{
	return inFD ? FD->align : _gDefaultFontDesc.align;
}

void UFontDesc::SetColor(TFontDesc inFD, const SColor& inColor)
{
	Require(inFD);
	if (FD->locked) Fail(errorType_Misc, error_Locked);
	FD->color = inColor;
}

void UFontDesc::GetColor(TFontDesc inFD, SColor& outColor)
{
	outColor = inFD ? FD->color : _gDefaultFontDesc.color;
}

void UFontDesc::SetCustomColor(TFontDesc inFD, const SColor& inColor)
{
	Require(inFD);
	if (FD->locked) Fail(errorType_Misc, error_Locked);
	FD->customColor = inColor;
}

void UFontDesc::GetCustomColor(TFontDesc inFD, SColor& outColor)
{
	outColor = inFD ? FD->customColor : _gDefaultFontDesc.customColor;
}

void UFontDesc::SetCustomValue(TFontDesc inFD, Uint32 inVal)
{
	Require(inFD);
	if (FD->locked) Fail(errorType_Misc, error_Locked);
	FD->customVal = inVal;
}

Uint32 UFontDesc::GetCustomValue(TFontDesc inFD)
{
	return inFD ? FD->customVal : _gDefaultFontDesc.customVal;
}

TFontDesc UFontDesc::Unflatten(const void *inData, Uint32 inDataSize)
{
	Uint32 *lp;
	Uint8 *name;
	Uint16 stdFont;

	if (inDataSize < 67) Fail(errorType_Misc, error_Corrupt);
	
	if (*(Uint16 *)inData != 'FD') Fail(errorType_Misc, error_FormatUnknown);
	if (FB(*(Uint16 *)(BPTR(inData)+2)) != 1) Fail(errorType_Misc, error_VersionUnknown);

	SFontDescObj *fd = (SFontDescObj *)UMemory::New(sizeof(SFontDescObj));
	
	try
	{
		lp = (Uint32 *)inData;
		
		fd->size = FB(lp[1]);
		fd->effect = FB(lp[2]);
		fd->align = FB(lp[6]);
		fd->customVal = FB(lp[7]);
		fd->locked = false;

		fd->color = *(SColor *)(lp+12);
		fd->customColor = *(SColor *)(lp+14);
		
		stdFont = *((Uint8 *)inData + 64);
		name = BPTR(inData) + 65;
		
		if (stdFont)
		{
			switch (stdFont)
			{
				case 1:
					name = kDefaultFont;
					break;
				case 2:
					name = kSystemFont;
					break;
				case 3:
					name = kFixedFont;
					break;
				case 4:
					name = kSansFont;
					break;
				case 5:
					name = kSerifFont;
					break;
				default:
					name = kDefaultFont;
					break;
			}
		}
		
		fd->macNum = _FontNameToMacNum(name);
		fd->macStyle = _FontEffectToMacStyle(fd->effect);
	}
	catch(...)
	{
		UMemory::Dispose((TPtr)fd);
		throw;
	}
	
	return (TFontDesc)fd;
}

void UFontDesc::EnumFontNames(TEnumFontNamesProc inProc, void *inRef)
{
	MenuHandle menu;
	Str255 str;
	short i, n;
	
	menu = ::NewMenu(22684, "\p");
	if (menu == nil) Fail(errorType_Memory, memError_NotEnough);
	
	::AppendResMenu(menu, 'FONT');
	

#if TARGET_API_MAC_CARBON
	n = ::CountMenuItems(menu);
#else
	n = ::CountMItems(menu);
#endif
	
	try
	{
		for (i=1; i<=n; i++)
		{
			::GetMenuItemText(menu, i, str);
			
			inProc(str, 0, 0, inRef);
		}
	}
	catch(...)
	{
		::DisposeMenu(menu);
		throw;
	}
	
	::DisposeMenu(menu);
}

#endif
