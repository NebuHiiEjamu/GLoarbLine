// =====================================================================
//	VCSTD.h                              (C) Hotline Communications 2000
// =====================================================================
//

#ifndef _H_VCSTD_
#define _H_VCSTD_

#if PRAGMA_ONCE
	#pragma once
#endif

namespace std
{
//
typedef unsigned int size_t;

#undef min
#undef max

template <typename _T> const _T& min(const _T& _t1, const _T& _t2)
{
	return _t1 < _t2 ? _t1 : _t2;
}

template <typename _T> const _T& max(const _T& _t1, const _T& _t2)
{
	return _t1 > _t2 ? _t1 : _t2;
}

inline size_t strlen(const char* _s)
{
	return ::strlen(_s);
}

inline int _stricmp(const char* _s1, const char* _s2)
{
	return ::_stricmp(_s1, _s2);
}

inline int _strnicmp(const char* _s1, const char* _s2, size_t _count)
{
	return ::_strnicmp(_s1, _s2, _count);
}

inline char* _strlwr(char* _s)
{
	return ::_strlwr(_s);
}

inline char tolower(char _c)
{
	return ::tolower(_c);
}

inline void* memcpy(void* _dst, const void* _src, size_t _count)
{
	return ::memcpy(_dst, _src, _count);
}
//
}
			   
#endif // _H_VCSTD_
