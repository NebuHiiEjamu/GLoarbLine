/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"

// alignment types
enum {
	align_Left			= 0x001,
	align_Right			= 0x002,
	align_Top			= 0x008,
	align_Bottom		= 0x010,
	align_CenterHoriz	= 0x020,
	align_CenterVert	= 0x040,
	align_InsideHoriz	= 0x080,
	align_InsideVert	= 0x100,
	align_NotIfInside	= 0x200,	// don't perform the align if already inside the rect

	align_TopLeft		= align_Top + align_Left,
	align_BottomRight	= align_Bottom + align_Right,
	align_TopRight		= align_Top + align_Right,
	align_BottomLeft	= align_Bottom + align_Left,
	align_Center		= align_CenterHoriz + align_CenterVert,
	align_Inside		= align_InsideHoriz + align_InsideVert
};

// forward declare
class SPoint;
class SRect;
class SColor;
class SRoundRect;

// selectors
enum HVSelector { kHoriz, kVert };

// point class
struct SPoint {
	// data members
	union {
		Int32 x, h;
	};
	union {
		Int32 y, v;
	};

	// construction
	SPoint()										{}
	SPoint(Int32 inHoriz, Int32 inVert)				: h(inHoriz), v(inVert) {}
	SPoint(const SPoint& inPt)						: h(inPt.h), v(inPt.v) {}
	
	// selector operators
	Int32& operator[](HVSelector i)					{	return i == kVert ? v : h;	}
	const Int32& operator[](HVSelector i) const		{	return i == kVert ? v : h;	}
	
	// arithmetic operators
	SPoint& operator+=(const SPoint& pt)			{	x += pt.x; y += pt.y; return *this;	}
	SPoint& operator-=(const SPoint& pt)			{	x -= pt.x; y -= pt.y; return *this;	}
	SPoint operator+(const SPoint& pt) const		{	return SPoint(h + pt.h, v + pt.v);	}
	SPoint operator-(const SPoint& pt) const		{	return SPoint(h - pt.h, v - pt.v);	}
	SPoint operator-() const						{	return SPoint(-h, -v);				}
	
	// comparison operators
	bool operator==(const SPoint& pt) const			{	return x == pt.x && y == pt.y;		}
	bool operator!=(const SPoint& pt) const			{	return x != pt.x || y != pt.y;		}
	bool operator>(const SPoint& pt) const			{	return x > pt.x && y > pt.y;		}
	bool operator<(const SPoint& pt) const			{	return x < pt.x && y < pt.y;		}
	bool operator>=(const SPoint& pt) const			{	return x >= pt.x && y >= pt.y;		}
	bool operator<=(const SPoint& pt) const			{	return x <= pt.x && y <= pt.y;		}
	
	// move
	void Move(Int32 inHorizDelta, Int32 inVertDelta)		{	x += inHorizDelta; y += inVertDelta;	}
	void MoveBack(Int32 inHorizDelta, Int32 inVertDelta)	{	x -= inHorizDelta; y -= inVertDelta;	}
	void Move(Int32 inDelta)								{	x += inDelta; y += inDelta;				}
	void MoveBack(Int32 inDelta)							{	x -= inDelta; y -= inDelta;				}

	// misc
	void Constrain(const SRect& r);
	void Set(Int32 inX, Int32 inY)							{	x = inX; y = inY;						}
	bool IsNull() const										{	return (x == 0 && y == 0);				}
};

struct SShortPoint {
	union {
		Int16 x, h;
	};
	union {
		Int16 y, v;
	};
};

// selectors
enum PointSelector { kTopLeft, kBotRight };

// rectangle struct
struct SRect {
	// data members
	Int32 left, top, right, bottom;
	
	// construction
	SRect() {}
	SRect(Int32 inLeft, Int32 inTop, Int32 inRight, Int32 inBottom)		: left(inLeft), top(inTop), right(inRight), bottom(inBottom) {}
	SRect(const SRect& inRect)											: left(inRect.left), top(inRect.top), right(inRect.right), bottom(inRect.bottom) {}
	SRect(const SPoint& inTopLeft, const SPoint& inBotRight)			: top(inTopLeft.v), left(inTopLeft.h), bottom(inBotRight.v), right(inBotRight.h) {}
	
	// selector operators
	SPoint& operator[](PointSelector i)					{	return i == kTopLeft ? *((SPoint *)&left) : *((SPoint *)&right);					}
	const SPoint& operator[](PointSelector i) const		{	return i == kTopLeft ? *((SPoint *)&left) : *((SPoint *)&right);					}
	
	// arithmetic operators
	SRect operator+(const SRect& r) const				{	return SRect(left + r.left, top + r.top, right + r.right, bottom + r.bottom);		}
	SRect operator-(const SRect& r) const				{	return SRect(left - r.left, top - r.top, right - r.right, bottom - r.bottom);		}
	SRect& operator+=(const SRect& r)					{	left += r.left; top += r.top; right += r.right; bottom += r.bottom; return *this;	}
	SRect& operator-=(const SRect& r)					{	left -= r.left; top -= r.top; right -= r.right; bottom -= r.bottom; return *this;	}
	SRect operator-() const								{	return SRect(-left, -top, -right, -bottom);											}
	SRect operator+(const SPoint& pt) const				{	return SRect(left + pt.h, top + pt.v, right + pt.h, bottom + pt.v);					}
	SRect operator-(const SPoint& pt) const				{	return SRect(left - pt.h, top - pt.v, right - pt.h, bottom - pt.v);					}
	SRect& operator+=(const SPoint& pt)					{	left += pt.h; top += pt.v; right += pt.h; bottom += pt.v; return *this;				}
	SRect& operator-=(const SPoint& pt)					{	left -= pt.h; top -= pt.v; right -= pt.h; bottom -= pt.v; return *this;				}
	
	// comparison operators
	bool operator==(const SRect& r) const				{	return left == r.left && top == r.top && right == r.right && bottom == r.bottom;	}
	bool operator!=(const SRect& r) const				{	return left != r.left || top != r.top || right != r.right || bottom != r.bottom;	}
	bool operator>(const SRect& r) const;				// area a > b
	bool operator<(const SRect& r) const;				// area a < b
	bool operator>=(const SRect& r) const;				// area a >= b
	bool operator<=(const SRect& r) const;				// area a <= b

	// area operators
	SRect operator&(const SRect& r) const;				// intersect a & b
	SRect operator|(const SRect& r) const;				// union a | b
	SRect& operator|=(const SRect& r);					// union a |= b
	
	// misc
	bool IsEmpty() const												{	return right <= left || bottom <= top;	}
	bool IsNotEmpty() const												{	return right > left && bottom > top;	}
	void SetEmpty()														{	top = left = bottom = right = 0;		}
	bool IsValid() const												{	return left <= right && top <= bottom;	}
	bool IsInvalid() const												{	return left > right || top > bottom;	}
	bool Contains(const SPoint& pt) const								{	return pt.h >= left && pt.v >= top && pt.h < right && pt.v < bottom;					}
	bool Contains(const SRect& r) const									{	return r.left >= left && r.top >= top && r.right <= right && r.bottom <= bottom;		}
	bool Intersects(const SRect& inRect) const;
	bool GetIntersection(const SRect& inRect, SRect& outSect) const;
	void GetUnion(const SRect& inRect, SRect& outUnion) const;
	Int32 GetHeight() const												{	return bottom - top;					}
	Int32 GetWidth() const												{	return right - left;					}
	Int32 GetArea() const												{	return (right - left) * (bottom - top);	}
	void Set(const SRect& inRect)										{	left = inRect.left; top = inRect.top; right = inRect.right; bottom = inRect.bottom;		}
	void Set(Int32 inLeft, Int32 inTop, Int32 inRight, Int32 inBottom)	{	left = inLeft; top = inTop; right = inRight; bottom = inBottom;							}
	void Enlarge(Int32 inHorizDelta, Int32 inVertDelta)					{	left -= inHorizDelta; top -= inVertDelta; right += inHorizDelta; bottom += inVertDelta;	}
	void Enlarge(Int32 inDelta)											{	left -= inDelta; top -= inDelta; right += inDelta; bottom += inDelta;					}
	void Constrain(const SRect& inMax);
	void Validate();
	void Center(const SRect& inBase);
	void CenterHoriz(const SRect& inBase);
	void CenterVert(const SRect& inBase);
	void Align(Uint16 inAlign, const SRect& inBase);
	void Rotate();
	
	// move
	void Move(Int32 inHorizDelta, Int32 inVertDelta)					{	left += inHorizDelta; top += inVertDelta; right += inHorizDelta; bottom += inVertDelta;	}
	void MoveBack(Int32 inHorizDelta, Int32 inVertDelta)				{	left -= inHorizDelta; top -= inVertDelta; right -= inHorizDelta; bottom -= inVertDelta;	}
	void Move(Int32 inDelta)											{	left += inDelta; top += inDelta; right += inDelta; bottom += inDelta;					}
	void MoveBack(Int32 inDelta)										{	left -= inDelta; top -= inDelta; right -= inDelta; bottom -= inDelta;					}
	void MoveTo(Int32 inHorizLoc, Int32 inVertLoc);
	void MoveTo(const SPoint& inPt)										{	MoveTo(inPt.h, inPt.v);			}

	// points
	SPoint& TopLeft()													{	return *((SPoint *)&left);		}
	const SPoint& TopLeft() const										{	return *((SPoint *)&left);		}
	SPoint& BottomRight()												{	return *((SPoint *)&right);		}
	const SPoint& BottomRight() const									{	return *((SPoint *)&right);		}
	SPoint& LeftTop()													{	return *((SPoint *)&left);		}
	const SPoint& LeftTop() const										{	return *((SPoint *)&left);		}
	SPoint& RightBottom()												{	return *((SPoint *)&right);		}
	const SPoint& RightBottom() const									{	return *((SPoint *)&right);		}
};

struct SShortRect {
	Int16 left, top, right, bottom;
};

// color struct
struct SColor {
	// data members
	Uint16 red, green, blue;
	
	// construction
	SColor() {}
	SColor(const SColor& inColor)							: red(inColor.red), green(inColor.green), blue(inColor.blue) {}
	SColor(Uint16 inRed, Uint16 inGreen, Uint16 inBlue)		: red(inRed), green(inGreen), blue(inBlue) {}
	SColor(Uint16 inGray)									: red(inGray), green(inGray), blue(inGray) {}
	
	// comparison operators
	bool operator==(const SColor& c) const					{	return red == c.red && green == c.green && blue == c.blue;		}
	bool operator!=(const SColor& c) const					{	return red != c.red || green != c.green || blue != c.blue;		}
	bool operator>(const SColor& r) const;
	bool operator<(const SColor& r) const;
	bool operator>=(const SColor& r) const;
	bool operator<=(const SColor& r) const;

	// arithmetic operators (by another color)
	SColor operator+(const SColor& c) const					{	return SColor(red + c.red, green + c.green, blue + c.blue);		}
	SColor operator-(const SColor& c) const					{	return SColor(red - c.red, green - c.green, blue - c.blue);		}
	SColor operator*(const SColor& c) const					{	return SColor(red * c.red, green * c.green, blue * c.blue);		}
	SColor operator/(const SColor& c) const					{	return SColor(red / c.red, green / c.green, blue / c.blue);		}
	SColor& operator+=(const SColor& c)						{	red += c.red; green += c.green; blue += c.blue; return *this;	}
	SColor& operator-=(const SColor& c)						{	red -= c.red; green -= c.green; blue -= c.blue; return *this;	}
	SColor& operator*=(const SColor& c)						{	red *= c.red; green *= c.green; blue *= c.blue; return *this;	}
	SColor& operator/=(const SColor& c)						{	red /= c.red; green /= c.green; blue /= c.blue; return *this;	}
	
	// arithmetic operators (by a scalar)
	SColor operator+(const Uint16 n) const					{	return SColor(red + n, green + n, blue + n);					}
	SColor operator-(const Uint16 n) const					{	return SColor(red - n, green - n, blue - n);					}
	SColor operator*(const Uint16 n) const					{	return SColor(red * n, green * n, blue * n);					}
	SColor operator/(const Uint16 n) const					{	return SColor(red / n, green / n, blue / n);					}
	SColor& operator+=(const Uint16 n)						{	red += n; green += n; blue += n; return *this;					}
	SColor& operator-=(const Uint16 n)						{	red -= n; green -= n; blue -= n; return *this;					}
	SColor& operator*=(const Uint16 n)						{	red *= n; green *= n; blue *= n; return *this;					}
	SColor& operator/=(const Uint16 n)						{	red /= n; green /= n; blue /= n; return *this;					}
	
	// misc
	void Set(Uint16 inRed, Uint16 inGreen, Uint16 inBlue)	{	red = inRed; green = inGreen; blue = inBlue;					}
	void Set(Uint16 inGray)									{	red = green = blue = inGray;									}
	void Lighten(Uint16 inAmount);
	void Darken(Uint16 inAmount);
	
	// conversions with 24-bit color (SColor is 48-bit)
	void SetShort(Uint8 inRed, Uint8 inGreen, Uint8 inBlue)				{	red = (Uint16)inRed * 257; green = (Uint16)inGreen * 257; blue = (Uint16)inBlue * 257;		}
	void SetShort(Uint8 inGray)											{	red = green = blue = (Uint16)inGray * 257;													}
	void GetShort(Uint8& outRed, Uint8& outGreen, Uint8& outBlue) const	{	outRed = red >> 8; outGreen = green >> 8; outBlue = blue >> 8;								}
	
	// conversions with packed 32-bit RGBA value (always RGBA regardless of endianess)
#if CONVERT_INTS
	void SetPacked(Uint32 inRGBA)										{	red = (inRGBA & 0xFF) * 257; green = ((inRGBA >> 8) & 0xFF) * 257; blue = ((inRGBA >> 16) & 0xFF) * 257;			}
	Uint32 GetPacked() const											{	return ((Uint32)(red >> 8)) | ((Uint32)(green & 0xFF00)) | (((Uint32)(blue >> 8)) << 16);							}
#else
	void SetPacked(Uint32 inRGBA)										{	red = ((inRGBA >> 24) & 0xFF) * 257; green = ((inRGBA >> 16) & 0xFF) * 257; blue = ((inRGBA >> 8) & 0xFF) * 257;	}
	Uint32 GetPacked() const											{	return (((Uint32)(red >> 8)) << 24) | (((Uint32)(green >> 8)) << 16) | (((Uint32)blue) & 0xFF00);					}
#endif
};

struct SPackedColor {
	Uint8 red, green, blue, alpha;
};

// round rectangle struct
struct SRoundRect : public SRect
{
	// data members
	Int16 ovalWidth, ovalHeight;
	
	// construction
	SRoundRect();
	SRoundRect(Int32 inLeft, Int32 inTop, Int32 inRight, Int32 inBottom, Int16 inOvalWidth = 0, Int16 inOvalHeight = 0);
	SRoundRect(const SRect& inBounds, Int16 inOvalWidth = 0, Int16 inOvalHeight = 0);
};

// inline constructors
inline SRoundRect::SRoundRect() {}
inline SRoundRect::SRoundRect(Int32 inLeft, Int32 inTop, Int32 inRight, Int32 inBottom, Int16 inOvalWidth, Int16 inOvalHeight) : SRect(inLeft, inTop, inRight, inBottom), ovalWidth(inOvalWidth), ovalHeight(inOvalHeight) {}
inline SRoundRect::SRoundRect(const SRect& inBounds, Int16 inOvalWidth, Int16 inOvalHeight) : SRect(inBounds), ovalWidth(inOvalWidth), ovalHeight(inOvalHeight) {}

// line struct
struct SLine {
	// data members
	SPoint start, end;
	
	// construction
	SLine();
	SLine(const SPoint& inStartPt, const SPoint& inEndPt);
	SLine(Int32 inLeft, Int32 inTop, Int32 inRight, Int32 inBottom);
};

// inline constructors
inline SLine::SLine() {}
inline SLine::SLine(const SPoint& inStartPt, const SPoint& inEndPt) : start(inStartPt), end(inEndPt) {}
inline SLine::SLine(Int32 inLeft, Int32 inTop, Int32 inRight, Int32 inBottom)
{	start.v = inTop; start.h = inLeft; end.v = inBottom; end.h = inRight;	}

// floating-point point struct
struct SFloatPoint {
	fast_float x, y;
};

// 3x3 matrix class
struct SMatrix3 {
	// data members
	fast_float map[3][3];
	
	// construction
	SMatrix3() {}
	SMatrix3(fast_float m00, fast_float m01, fast_float m02, fast_float m10, fast_float m11, fast_float m12, fast_float m20, fast_float m21, fast_float m22)	{	map[0][0] = m00; map[0][1] = m01; map[0][2] = m02; map[1][0] = m10; map[1][1] = m11; map[1][2] = m12; map[2][0] = m20; map[2][1] = m21; map[2][2] = m22;	}
	SMatrix3(const SMatrix3& inMat);

	// array operators
	fast_float *operator[](int i)									{	return &map[i][0];	}
	const fast_float *operator[](int i) const						{	return &map[i][0];	}

	// arithmetic operators
	SMatrix3& operator +=(const SMatrix3& inMat);
	SMatrix3& operator -=(const SMatrix3& inMat);
	SMatrix3& operator *=(const SMatrix3& inMat);
	SMatrix3& operator *=(fast_float inNum);
	SMatrix3& operator /=(fast_float inNum);

	// set
	void Set(fast_float m00, fast_float m01, fast_float m02, fast_float m10, fast_float m11, fast_float m12, fast_float m20, fast_float m21, fast_float m22)	{	map[0][0] = m00; map[0][1] = m01; map[0][2] = m02; map[1][0] = m10; map[1][1] = m11; map[1][2] = m12; map[2][0] = m20; map[2][1] = m21; map[2][2] = m22;	}
	void SetIdentity();
	void Reset()													{	SetIdentity();		}
	
	// transformations
	void Move(fast_float inHorizDelta, fast_float inVertDelta)		{	map[2][0] += inHorizDelta; map[2][1] += inVertDelta;	}
	void Scale(fast_float inHorizFactor, fast_float inVertFactor);
	void Rotate(fast_float inAngle);

	// apply matrix
	void TransformPoint(SFloatPoint& ioPt) const;
	void TransformPoints(SFloatPoint *ioPts, Uint32 inCount) const;

	// generate transformation matrices
	static void MakeRotateMatrix(fast_float inAngle, SMatrix3& outMat);
	static void MakeScaleMatrix(fast_float inHorizFactor, fast_float inVertFactor, SMatrix3& outMat);
	static void MakeMoveMatrix(fast_float inHorizDelta, fast_float inVertDelta, SMatrix3& outMat);
};



// geometric constants
extern const SRect kZeroRect;
extern const SPoint kZeroPoint;

// boring color constants
extern const SColor color_Gray0, color_Gray1, color_Gray2, color_Gray3, color_Gray4, color_Gray5, color_Gray6, color_Gray7;
extern const SColor color_Gray8, color_Gray9, color_GrayA, color_GrayB, color_GrayC, color_GrayD, color_GrayE, color_GrayF;
extern const SColor color_White, color_Black, color_Gray, color_Red, color_Green, color_Blue;
extern const SColor color_Cyan, color_Magenta, color_Yellow, color_Orange, color_Chartreuse, color_Aqua, color_Slate, color_Purple, color_Maroon, color_Brown, color_Pink, color_Turquoise;

// artistic color constants
extern const SColor color_cadmium_lemon;
extern const SColor color_cadmium_light_yellow;
extern const SColor color_aureoline_yellow;
extern const SColor color_naples_deep_yellow;
extern const SColor color_cadmium_yellow;
extern const SColor color_cadmium_deep_yellow;
extern const SColor color_cadmium_orange;
extern const SColor color_cadmium_light_red;
extern const SColor color_cadmium_deep_red;
extern const SColor color_geranium_lake;
extern const SColor color_alizarin_crimson;
extern const SColor color_rose_madder;
extern const SColor color_madder_deep_lake;
extern const SColor color_brown_madder;
extern const SColor color_permanent_red_violet;
extern const SColor color_cobalt_deep_violet;
extern const SColor color_ultramarine_violet;
extern const SColor color_ultramarine_blue;
extern const SColor color_cobalt_blue;
extern const SColor color_royal_blue;
extern const SColor color_cerulean_blue;
extern const SColor color_manganese_blue;
extern const SColor color_indigo;
extern const SColor color_turquoise_blue;
extern const SColor color_emerald_green;
extern const SColor color_permanent_green;
extern const SColor color_viridian_light;
extern const SColor color_cobalt_green;
extern const SColor color_cinnabar_green;
extern const SColor color_sap_green;
extern const SColor color_chromium_oxide_green;
extern const SColor color_terre_verte;
extern const SColor color_yellow_ochre;
extern const SColor color_mars_yellow;
extern const SColor color_raw_sienna;
extern const SColor color_mars_orange;
extern const SColor color_gold_ochre;
extern const SColor color_brown_ochre;
extern const SColor color_deep_ochre;
extern const SColor color_burnt_umber;
extern const SColor color_burnt_sienna;
extern const SColor color_flesh;
extern const SColor color_flesh_ochre;
extern const SColor color_english_red;
extern const SColor color_venetian_red;
extern const SColor color_indian_red;
extern const SColor color_raw_umber;
extern const SColor color_greenish_umber;
extern const SColor color_van_dyck_brown;
extern const SColor color_sepia;
extern const SColor color_warm_grey;
extern const SColor color_cold_grey;
extern const SColor color_ivory_black;
extern const SColor color_lamp_black;
extern const SColor color_titanium_white;
extern const SColor color_zinc_white;
extern const SColor color_pale_gold;
extern const SColor color_gold;
extern const SColor color_old_gold;
extern const SColor color_pink_gold;
extern const SColor color_white_gold;
extern const SColor color_yellow_gold;
extern const SColor color_green_gold;
extern const SColor color_platinum;
extern const SColor color_silver;
extern const SColor color_antique_silver;
extern const SColor color_chrome;
extern const SColor color_steel;
extern const SColor color_copper;
extern const SColor color_antique_copper;
extern const SColor color_oxidized_copper;
extern const SColor color_bronze;
extern const SColor color_brass;
extern const SColor color_iron;
extern const SColor color_rusted_iron;
extern const SColor color_lead;
extern const SColor color_fluorescent_pink;
extern const SColor color_fluorescent_green;
extern const SColor color_fluorescent_blue;
extern const SColor color_incadescent_high;
extern const SColor color_incadescent_low;
extern const SColor color_moonlight;
extern const SColor color_sodium;
extern const SColor color_daylight;
extern const SColor color_dawn;
extern const SColor color_afternoon;
extern const SColor color_dusk;
extern const SColor color_purple_gray;

// graphics math constants
const fast_float gm_2Pi =       6.28318530717958623200;	// 2pi
const fast_float gm_DegToRad =  0.01745329251994329547;	// pi/180
const fast_float gm_E =         2.71828182845904553488;	// e
const fast_float gm_EExpPi =   23.14069263277927390732;	// e exp pi
const fast_float gm_Golden =    1.61803398874989490253;	// golden ratio
const fast_float gm_InvPi =     0.31830988618379069122;	// pi exp -1
const fast_float gm_Ln10 =      2.30258509299404590109;	// ln10
const fast_float gm_Ln2 =       0.69314718055994528623;	// ln2
const fast_float gm_Log10e =    0.43429448190325187218;	// log e
const fast_float gm_Log2e =     1.44269504088896338700;	// lg e
const fast_float gm_Pi =        3.14159265358979323846;	// pi
const fast_float gm_PiDiv2 =    1.57079632679489655800;	// pi/2
const fast_float gm_PiDiv4 =    0.78539816339744827900;	// pi/4
const fast_float gm_RadToDeg = 57.29577951308232286465;	// 180/pi
const fast_float gm_Sqrt2 =     1.41421356237309514547;	// sqrt(2)
const fast_float gm_Sqrt2PI =   2.50662827463100024161;	// sqrt(2pi)
const fast_float gm_Sqrt3 =     1.73205080756887719318;	// sqrt(3)
const fast_float gm_Sqrt10 =    3.16227766016837952279;	// sqrt(10)
const fast_float gm_SqrtE =     1.64872127070012819416;	// sqrt(e)
const fast_float gm_SqrtHalf =  0.70710678118654757274;	// sqrt(0.5)
const fast_float gm_SqrtLn2 =   0.83255461115769768821;	// sqrt(ln2)
const fast_float gm_SqrtPi =    1.77245385090551588192;	// sqrt(pi)
const fast_float gm_Epsilon =   1.0e-10;				// next double > 0
const fast_float gm_Googol =    1.0e50;					// large double


inline fast_float RadToDeg(fast_float f)
{
	return f * gm_RadToDeg;
}

inline fast_float DegToRad(fast_float f)
{
	return f * gm_DegToRad;
}

