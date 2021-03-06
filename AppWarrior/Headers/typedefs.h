/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once

#ifndef nil
#define nil 	0
#endif

typedef char				Int8;
typedef short				Int16;
typedef long				Int32;
typedef long long			Int64;

typedef unsigned char		Uint8;
typedef unsigned short		Uint16;
typedef unsigned long		Uint32;
typedef unsigned long long	Uint64;

typedef unsigned char		Char8;
typedef unsigned short		Char16;

//typedef Uint16				bool;

typedef float				Float32;
//typedef double				Float64;

#if __POWERPC__	// on PPC, double (64-bit) is fastest
	typedef double fast_float;
#elif __MC68K__	// on 68K, long double (a.k.a. extended) is always the fastest (80 or 96-bits)
	typedef long double fast_float;
#elif __INTEL__
	typedef double fast_float;
#else
	typedef float fast_float;
#endif

#define RANGE(num, min, max)	(((num) >= (min)) && ((num) <= (max)))

//#ifndef OFFSET_OF
//#define OFFSET_OF(T, member)		((Uint32)&(((T *)0)->member))
//#endif

#ifndef HiWord
#define HiWord(n)	((Uint16)(((Uint32)(n)) >> 16))
#define LoWord(n)	((Uint16)(n))
#endif

#ifndef true
#define false	0
#define true	(!0)
#endif

#define FORCE_CAST(t, d)		(*(t *)&(d))
#define BPTR(p)					((Uint8 *)(p))
#define CPTR(p)					((Int8 *)(p))
#define WPTR(p)					((Uint16 *)(p))
#undef LPTR												// work around win32 conflict
//#define LPTR(p)					((Uint32 *)(p))

#if WIN32
typedef unsigned int operator_new_size_t;
#else
typedef Uint32 operator_new_size_t;
#endif

extern void *operator new(operator_new_size_t s);
extern void *operator new(operator_new_size_t, operator_new_size_t s);
extern void operator delete(void *p);

const Int8		max_Int8	= 0x7F;
const Int8		min_Int8	= 0x80;
const Uint8		max_Uint8	= 0xFF;
const Uint8		min_Uint8	= 0x00;

const Int16		max_Int16	= 0x7FFF;
const Int16		min_Int16	= 0x8000;
const Uint16	max_Uint16	= 0xFFFF;
const Uint16	min_Uint16	= 0x0000;

const Int32		max_Int32	= 0x7FFFFFFF;
const Int32		min_Int32	= 0x80000000;
const Uint32	max_Uint32	= 0xFFFFFFFF;
const Uint32	min_Uint32	= 0x00000000;

#undef min
#undef max
#undef swap
#undef abs

template <class T>
inline T min (T a, T b)
{
    return (b < a) ? b : a;
}

template <class T>
inline T min (T a, T b, T c)
{
    return min(min(a, b), c);
}

template <class T>
inline T min (T a, T b, T c, T d)
{
    return min(min(min(a, b), c), d);
}

template <class T>
inline T max (T a, T b)
{
    return (a < b) ? b : a;
}

template <class T>
inline T max (T a, T b, T c)
{
    return max(max(a, b), c);
}

template <class T>
inline T max (T a, T b, T c, T d)
{
    return max(max(max(a, b), c), d);
}

template <class T>
inline void swap (T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

template <class T>
inline void clamp (T& n, T l, T h)
{
	if (n < l) n = l;
	if (n > h) n = h;
}

template <class T>
inline T abs (T i)
{
	return (i < 0) ? -i : i;
}

template <class T>
inline T diff (T a, T b)
{
	return (a > b) ? a - b : b - a;
}

inline Uint32 swap_int(Uint32 n)
{
#if __MWERKS__ && __POWERPC__
	return __lwbrx((void *)&n, 0);
#else
	return (n >> 24) | ((n << 8) & 0x00FF0000) | ((n >> 8) & 0x0000FF00) | (n << 24);
#endif
}

inline Uint16 swap_int(Uint16 n)
{
#if __MWERKS__ && __POWERPC__
	Uint16 array[2];
	array[0] = n;
	return (Uint16)__lwbrx(array, 0);
#else
	return (n >> 8) | (n << 8);
#endif
}

inline Uint8 TB(Uint8 n)	{	return n;	}
inline Int8 TB(Int8 n)		{	return n;	}
inline Uint8 FB(Uint8 n)	{	return n;	}
inline Int8 FB(Int8 n)		{	return n;	}

#if __INTEL__

	#define CONVERT_INTS	1
	inline Uint16 TB(Uint16 n)	{	return swap_int(n);			}
	inline Uint32 TB(Uint32 n)	{	return swap_int(n);			}
	inline Uint16 FB(Uint16 n)	{	return swap_int(n);			}
	inline Uint32 FB(Uint32 n)	{	return swap_int(n);			}
	
	inline Int16 TB(Int16 n)	{	return swap_int((Uint16)n);	}
	inline Int32 TB(Int32 n)	{	return swap_int((Uint32)n);	}
	inline Int16 FB(Int16 n)	{	return swap_int((Uint16)n);	}
	inline Int32 FB(Int32 n)	{	return swap_int((Uint32)n);	}

#elif __POWERPC__ || __MC68K__

	#define CONVERT_INTS	0
	inline Uint16 TB(Uint16 n)	{	return n;					}
	inline Uint32 TB(Uint32 n)	{	return n;					}
	inline Uint16 FB(Uint16 n)	{	return n;					}
	inline Uint32 FB(Uint32 n)	{	return n;					}

	inline Int16 TB(Int16 n)	{	return n;					}
	inline Int32 TB(Int32 n)	{	return n;					}
	inline Int16 FB(Int16 n)	{	return n;					}
	inline Int32 FB(Int32 n)	{	return n;					}

#else

	#error "unknown processor"

#endif

template <class T>
class scopekiller
{
	public:
		scopekiller(T *inObj) : obj(inObj) {}
		~scopekiller() { if (obj) delete obj; }
		void cancel() { obj = nil; }
	
	private:
		T *obj;
};

#define scopekill(type, obj)	scopekiller<type> kill_##obj(obj)

template <class T>
class StValueChanger
{
	public:
		StValueChanger(T& inVariable, T inNewValue) : mVariable(inVariable), mOrigValue(inVariable)	{	inVariable = inNewValue;	}
		~StValueChanger()																			{	mVariable = mOrigValue;		}
	
	private:
		T& mVariable;
		T mOrigValue;
};


// we like the inherited keyword and :P to all who disagree
#pragma def_inherited on

// round up or down
#define RoundUp2(n)			((((Uint32)(n)) + 1) & ~1)
#define RoundUp4(n)			((((Uint32)(n)) + 3) & ~3)
#define RoundUp8(n)			((((Uint32)(n)) + 7) & ~7)
#define RoundUp16(n)		((((Uint32)(n)) + 15) & ~15)
#define RoundUp32(n)		((((Uint32)(n)) + 31) & ~31)
#define RoundUp64(n)		((((Uint32)(n)) + 63) & ~63)
#define RoundDown2(n)		(((Uint32)(n)) & ~1)
#define RoundDown4(n)		(((Uint32)(n)) & ~3)
#define RoundDown8(n)		(((Uint32)(n)) & ~7)
#define RoundDown16(n)		(((Uint32)(n)) & ~15)
#define RoundDown32(n)		(((Uint32)(n)) & ~31)
#define RoundDown64(n)		(((Uint32)(n)) & ~63)

// on some processors, *++p is faster than *p++
#if __POWERPC__
	#define USE_PRE_INCREMENT		1
#else
	#define USE_PRE_INCREMENT		0
#endif

// whether or not this OS uses file name extensions (eg "test.txt")
#if WIN32
	#define USES_FILE_EXTENSIONS	1
#else
	#define USES_FILE_EXTENSIONS	0
#endif

// undefine troublesome mac #defines
#undef SetItem
#undef GetItem
#undef SetScript
#undef GetScript

// undefine troublesome win #defines
#undef PostMessage
#undef GetMessage
#undef SendMessage
#undef DrawText
#undef CreateFile
#undef DeleteFile
#undef CreateDirectory
#undef CopyFile
#undef MoveFile
#undef GetDiskFreeSpace


