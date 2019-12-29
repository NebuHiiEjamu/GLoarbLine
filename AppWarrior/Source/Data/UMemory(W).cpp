#if WIN32

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UMemory.h"
#include "UError.h"
#include "UDebug.h"
#include "UMemory(priv).h"

/*
 * UMemory supports fast pool allocation for pointers.  When compiled for debugging,
 * UMemory does NOT use this allocation scheme because it tends to be more forgiving
 * and makes stress testing tools such as QC less useful.
 */
#if DEBUG
	#define USE_POOL_ALLOC		0
#else
	#define USE_POOL_ALLOC		1
#endif

// these should be odd so that UMemory::IsValid() will detect an invalid pointer
const Uint8 kUninitialized	= 0x2B;		// '+'
const Uint8 kReleased		= 0xF3;

#if DEBUG
static const Int8 kNoResizeLockedHdlMsg[] = "UMemory - cannot resize locked handle";
static const Int8 kInvalidPointerMsg[] = "UMemory - TPtr is not valid";
static const Int8 kInvalidHdlMsg[] = "UMemory - THdl is not valid";
#endif

// win32 MoveMemory/CopyMemory func is just a #define for memmove/memcpy
void __copy_mem(void * dst, const void * src, unsigned long n);
void __move_mem(void * dst, const void * src, unsigned long n);
#undef MoveMemory
#define MoveMemory	__move_mem
#undef CopyMemory
#define CopyMemory	__copy_mem

#if USE_POOL_ALLOC
static CRITICAL_SECTION __malloc_pool_access = { 0 };
static void _InitPoolAllocCriticalSection();

static mem_pool_obj __malloc_pool;
static char __malloc_pool_initted = false;
#endif

/* -------------------------------------------------------------------------- */

void UMemory::Init()
{
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// cannot allocate a block of 0 bytes in size
TPtr UMemory::New(Uint32 inSize)
{
	Require(inSize);
	
#if USE_POOL_ALLOC

	void *block;
	
	// protect from other threads
	_InitPoolAllocCriticalSection();
	::EnterCriticalSection(&__malloc_pool_access);
	
	if (!__malloc_pool_initted)
	{
		_MEInitPool(&__malloc_pool);
		__malloc_pool_initted = true;
	}

	block = _MEPoolAlloc(&__malloc_pool, inSize);
	
	::LeaveCriticalSection(&__malloc_pool_access);
	
	if (block == nil)
		Fail(errorType_Memory, memError_NotEnough);

	return (TPtr)block;

#else

	TPtr p = (TPtr)::GlobalAlloc(GMEM_FIXED, inSize);
	if (p == nil) Fail(errorType_Memory, memError_NotEnough);

	#if DEBUG
	Fill(p, inSize, kUninitialized);
	#endif
	
	return p;
	
#endif
}


TPtr UMemory::New(const void *inData, Uint32 inSize)
{
	TPtr p = New(inSize);
	::CopyMemory(p, inData, inSize);
	return p;
}


TPtr UMemory::NewClear(Uint32 inSize)
{
	Require(inSize);

#if USE_POOL_ALLOC

	void *block;

	_InitPoolAllocCriticalSection();
	::EnterCriticalSection(&__malloc_pool_access);

	if (!__malloc_pool_initted)
	{
		_MEInitPool(&__malloc_pool);
		__malloc_pool_initted = true;
	}
	
	block = _MEPoolAllocClear(&__malloc_pool, inSize);
	
	::LeaveCriticalSection(&__malloc_pool_access);
	
	if (block == nil)
		Fail(errorType_Memory, memError_NotEnough);

	return (TPtr)block;

#else

	TPtr p = (TPtr)::GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, inSize);
	if (p == nil) Fail(errorType_Memory, memError_NotEnough);
	return p;
	
#endif
}

void UMemory::Dispose(TPtr inPtr)
{
#if USE_POOL_ALLOC

	if (inPtr && __malloc_pool_initted)
	{
		_InitPoolAllocCriticalSection();
		::EnterCriticalSection(&__malloc_pool_access);

		_MEPoolFree(&__malloc_pool, inPtr);
		
		::LeaveCriticalSection(&__malloc_pool_access);
	}

#else

	if (inPtr)
	{
	#if DEBUG
		//if (((Uint32)inPtr) & 7)	// doesn't work on 95
		if (((Uint32)inPtr) & 3)
		{
			DebugBreak(kInvalidPointerMsg);
			return;
		}
		
		Fill(inPtr, ::GlobalSize((HGLOBAL)inPtr), kReleased);
	#endif
		
		::GlobalFree((HGLOBAL)inPtr);
	}

#endif
}

/* -------------------------------------------------------------------------- */
#pragma mark -

THdl UMemory::NewHandle(Uint32 inSize, Uint32 /* inOptions */)
{
	// immediately fail on ridiculous allocations (fixes bug where inSize overflows and wraps around to a small size)
	if (inSize & 0x80000000) Fail(errorType_Memory, memError_NotEnough);

	HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, inSize + 4);
	if (h == nil) Fail(errorType_Memory, memError_NotEnough);
	
#if DEBUG

	Uint8 *p = (Uint8 *)::GlobalLock(h);
	*(Uint32 *)p = inSize;
	Fill(p+4, inSize, kUninitialized);
	::GlobalUnlock(h);
	
#else

	*(Uint32 *)::GlobalLock(h) = inSize;	// we store the correct size at the start b/c win95 rounds sizes up to the nearest 8 byte boundary.  winNT reports the correct size.
	::GlobalUnlock(h);
	
#endif

	return (THdl)h;
}

THdl UMemory::NewHandle(const void *inData, Uint32 inSize, Uint32 /* inOptions */)
{
	// immediately fail on ridiculous allocations (fixes bug where inSize overflows and wraps around to a small size)
	if (inSize & 0x80000000) Fail(errorType_Memory, memError_NotEnough);

	HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, inSize + 4);
	if (h == nil) Fail(errorType_Memory, memError_NotEnough);
	
	Uint8 *p = (Uint8 *)::GlobalLock(h);
	*(Uint32 *)p = inSize;
	::CopyMemory(p+4, inData, inSize);
	::GlobalUnlock(h);

	return (THdl)h;
}

THdl UMemory::NewHandleClear(Uint32 inSize, Uint32 /* inOptions */)
{
	// immediately fail on ridiculous allocations (fixes bug where inSize overflows and wraps around to a small size)
	if (inSize & 0x80000000) Fail(errorType_Memory, memError_NotEnough);

	HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, inSize + 4);
	if (h == nil) Fail(errorType_Memory, memError_NotEnough);
	
	*(Uint32 *)::GlobalLock(h) = inSize;
	::GlobalUnlock(h);

	return (THdl)h;
}

/*
 * Clone() creates a new handle with the same data as in the specified
 * handle.  The handle is created unlocked and non-discardable.
 */
THdl UMemory::Clone(THdl inHdl)
{
	Require(inHdl);
	
	Uint32 s = ::GlobalSize((HGLOBAL)inHdl);
	
	HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, s);
	if (h == nil) Fail(errorType_Memory, memError_NotEnough);
	
	void *sp = ::GlobalLock((HGLOBAL)inHdl);
	void *dp = ::GlobalLock(h);
	
	::CopyMemory(dp, sp, s);
	
	::GlobalUnlock(h);
	::GlobalUnlock((HGLOBAL)inHdl);

	return (THdl)h;
}

/*
 * Dispose() deallocates the specified handle, releasing all
 * memory belonging to it.  After this call, the handle is not
 * valid and must not be used.  You should not dispose a 
 * locked handle.
 */
void UMemory::Dispose(THdl inHdl)
{
	if (inHdl)
	{
	#if DEBUG
		UINT flags = ::GlobalFlags((HGLOBAL)inHdl);
		if (flags == GMEM_INVALID_HANDLE)
		{
			DebugBreak(kInvalidHdlMsg);
			return;
		}
		if (flags & GMEM_LOCKCOUNT)
		{
			DebugBreak("UMemory - cannot dispose locked handle!");
			return;
		}
		void *p = ::GlobalLock((HGLOBAL)inHdl);
		if (p)		// don't zap discarded handle!
		{
			Fill(p, ::GlobalSize((HGLOBAL)inHdl), kReleased);
			::GlobalUnlock((HGLOBAL)inHdl);
		}
	#endif
	
		::GlobalFree((HGLOBAL)inHdl);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
 * SetSize() changes the amount of memory allocated to the specified
 * handle.  As much of the existing data as will fit is preserved.
 * If expanding, all of the data will be preserved, and the contents
 * of the extra bytes at the end is undefined.  If shrinking, the
 * data will be truncated.  Note that you cannot use SetSize() on a
 * locked handle.  Do not use SetSize() on a discarded handle - use
 * Reallocate() instead.
 */
void UMemory::SetSize(THdl inHdl, Uint32 inSize)
{
	Require(inHdl);

	// immediately fail on ridiculous allocations (fixes bug where inSize overflows and wraps around to a small size)
	if (inSize & 0x80000000) Fail(errorType_Memory, memError_NotEnough);

	inSize += 4;	// add space to store correct size
	
#if DEBUG
	if (::GlobalFlags((HGLOBAL)inHdl) & GMEM_LOCKCOUNT)
	{
		DebugBreak(kNoResizeLockedHdlMsg);
		Fail(errorType_Misc, error_Protocol);
	}
	Uint32 origSize = ::GlobalSize((HGLOBAL)inHdl);
#endif

	if (::GlobalReAlloc((HGLOBAL)inHdl, inSize, GMEM_MOVEABLE) == NULL)
		Fail(errorType_Memory, memError_NotEnough);
	
	// store new size
	*(Uint32 *)::GlobalLock((HGLOBAL)inHdl) = inSize - 4;
	::GlobalUnlock((HGLOBAL)inHdl);

#if DEBUG
	if (inSize > origSize)
	{
		void *p = ::GlobalLock((HGLOBAL)inHdl);
		Fill(BPTR(p) + origSize, inSize - origSize, kUninitialized);
		::GlobalUnlock((HGLOBAL)inHdl);
	}
#endif
}

/*
 * GetSize() returns the size in bytes of the specified handle.
 * If the handle is discarded, GetSize() returns 0.
 */
Uint32 UMemory::GetSize(THdl inHdl)
{
	Require(inHdl);
	//return ::GlobalSize((HGLOBAL)inHdl);
	
	// GlobalSize() can be rounded up, so we'll use the exact size stored at the start of the handle

	void *p = ::GlobalLock((HGLOBAL)inHdl);
	
	Uint32 s = (p == NULL) ? 0 : *(Uint32 *)p;
	
	::GlobalUnlock((HGLOBAL)inHdl);

	return s;
}

Uint32 UMemory::Grow(THdl inHdl, Int32 inDelta)
{
	Int32 s = (Int32)GetSize(inHdl) + inDelta;
	if (s < 0) s = 0;
	SetSize(inHdl, s);
	return s;
}

/*
 * Reallocate() changes the size of a block of memory while preserving its
 * contents.  If necessary, Reallocate() copies the contents to another
 * block of memory.  If the pointer is nil, a new block of the specified
 * size is allocated.  If the size is 0, then the pointer is disposed of
 * and set to nil.
 */
TPtr UMemory::Reallocate(TPtr inPtr, Uint32 inSize)
{
	if (inPtr == nil)
	{
		if (inSize != 0)
			inPtr = New(inSize);
	}
	else if (inSize == 0)
	{
		Dispose(inPtr);
		inPtr = nil;
	}
	else
	{
#if USE_POOL_ALLOC

		void *block;

		_InitPoolAllocCriticalSection();
		::EnterCriticalSection(&__malloc_pool_access);

		if (!__malloc_pool_initted)
		{
			_MEInitPool(&__malloc_pool);
			__malloc_pool_initted = true;
		}
		
		block = _MEPoolRealloc(&__malloc_pool, inPtr, inSize);
		
		::LeaveCriticalSection(&__malloc_pool_access);

		if (block == nil)
			Fail(errorType_Memory, memError_NotEnough);

		inPtr = (TPtr)block;

#else
		
	#if DEBUG
		//if (((Uint32)inPtr) & 7)	// doesn't work on 95
		if (((Uint32)inPtr) & 3)
		{
			DebugBreak(kInvalidPointerMsg);
			Fail(errorType_Misc, error_Protocol);
		}
	#endif
	
		inPtr = (TPtr)::GlobalReAlloc((HGLOBAL)inPtr, inSize, GMEM_MOVEABLE);
		if (inPtr == nil) Fail(errorType_Memory, memError_NotEnough);
		
#endif
	}
	
	return inPtr;
}

/*
 * Reallocate() changes the amount of memory allocated to the
 * specified handle.  Unlike SetSize(), the contents of the
 * handle is lost.  Reallocate() is normally used to reallocate
 * discarded handles.  Note that the handle is reallocated
 * unlocked and non-discardable (even if it was before).
 * Also, you should not attempt to reallocate a locked handle.
 */
void UMemory::Reallocate(THdl inHdl, Uint32 inSize)
{
	Require(inHdl);
	
#if DEBUG
	if (::GlobalFlags((HGLOBAL)inHdl) & GMEM_LOCKCOUNT)
	{
		DebugBreak(kNoResizeLockedHdlMsg);
		Fail(errorType_Misc, error_Protocol);
	}
#endif
	
	// immediately fail on ridiculous allocations (fixes bug where inSize overflows and wraps around to a small size)
	if (inSize & 0x80000000) Fail(errorType_Memory, memError_NotEnough);

	// turn discard off.  Must be done BEFORE ReAlloc (otherwise might get discarded immediately after ReAlloc)
	::GlobalReAlloc((HGLOBAL)inHdl, 0, GMEM_MODIFY);
	
	if (::GlobalReAlloc((HGLOBAL)inHdl, inSize + 4, GMEM_MOVEABLE) == NULL)
		Fail(errorType_Memory, memError_NotEnough);
	
#if DEBUG

	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	*(Uint32 *)p = inSize;
	Fill(p+4, inSize, kUninitialized);
	::GlobalUnlock((HGLOBAL)inHdl);

#else

	*(Uint32 *)::GlobalLock((HGLOBAL)inHdl) = inSize;
	::GlobalUnlock((HGLOBAL)inHdl);

#endif
}

void UMemory::ReallocateClear(THdl inHdl, Uint32 inSize)
{
	Require(inHdl);
	
#if DEBUG
	if (::GlobalFlags((HGLOBAL)inHdl) & GMEM_LOCKCOUNT)
	{
		DebugBreak(kNoResizeLockedHdlMsg);
		Fail(errorType_Misc, error_Protocol);
	}
#endif
	
	// immediately fail on ridiculous allocations (fixes bug where inSize overflows and wraps around to a small size)
	if (inSize & 0x80000000) Fail(errorType_Memory, memError_NotEnough);

	::GlobalReAlloc((HGLOBAL)inHdl, 0, GMEM_MODIFY);
	
	if (::GlobalReAlloc((HGLOBAL)inHdl, inSize + 4, GMEM_MOVEABLE) == NULL)
		Fail(errorType_Memory, memError_NotEnough);
	
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	*(Uint32 *)p = inSize;
	Fill(p+4, inSize, 0);
	::GlobalUnlock((HGLOBAL)inHdl);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
 * Lock() locks the specified handle and returns a pointer to the
 * data in that handle.  If the handle is discarded, Lock() returns
 * nil.  You must ALWAYS check for a nil result when locking discardable
 * handles because until the handle is locked, it can be discarded
 * at ANY time.  A lock count is maintained for each handle so you
 * can nest calls to Lock() and Unlock() (every call to Lock() should
 * be matched with a call to Unlock(), except when Lock() returns nil).
 */
Uint8 *UMemory::Lock(THdl inHdl)
{
	Require(inHdl);
	return ((Uint8 *)::GlobalLock((HGLOBAL)inHdl)) + 4;		// +4 to skip over stored size
}

/*
 * Unlock() decrements the lock count for the specified handle.
 * If the count reaches zero, the handle is unlocked and any
 * pointers obtained from locking the handle become invalid.
 * You should not attempt to unlock a discarded handle.
 */
void UMemory::Unlock(THdl inHdl)
{
	/*
	 * This is a clean-up func, so DON'T throw exceptions!
	 */
	if (inHdl)
	{
	#if DEBUG
		UINT flags = ::GlobalFlags((HGLOBAL)inHdl);
		if (flags == GMEM_INVALID_HANDLE)
		{
			DebugBreak(kInvalidHdlMsg);
			return;
		}
		if (flags & GMEM_DISCARDED) return;	// in windoze, a 0-byte handle is the same as discarded, and the lock count is always 0
		if ((flags & GMEM_LOCKCOUNT) == 0)
		{
			DebugBreak("UMemory - Unlock() called too many times!");
			return;
		}
	#endif

		::GlobalUnlock((HGLOBAL)inHdl);
	}
}

/*
 * SetDiscardable() makes the specified handle discardable.  Discardable
 * unlocked handles can be deallocated by the system at ANY time. However,
 * unlike Dispose(), the THdl value remains valid.  Lock() will return nil
 * for discarded handles.
 */
void UMemory::SetDiscardable(THdl inHdl)
{
	Require(inHdl);
	::GlobalReAlloc((HGLOBAL)inHdl, 0, GMEM_MODIFY | GMEM_DISCARDABLE);
}

/*
 * ClearDiscardable() makes the specified handle non-discardable, and returns
 * whether or not the handle is discarded (remember that an unlocked discardable
 * handle can be discarded at ANY time).  An IsDiscarded() function is NOT provided
 * because it would be unsafe.  For example, the handle could be discarded immediately
 * after IsDiscarded() returned true.  Instead, ClearDiscardable() returns whether
 * or not the handle is discarded, and this is safe because it makes the handle
 * non-discardable before checking if it is discarded.
 */
bool UMemory::ClearDiscardable(THdl inHdl)
{
	// note must turn discardable OFF before checking if discarded (to ensure that it's accurate)
	Require(inHdl);
	::GlobalReAlloc((HGLOBAL)inHdl, 0, GMEM_MODIFY);
	return (::GlobalFlags((HGLOBAL)inHdl) & GMEM_DISCARDED) != 0;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UMemory::Set(THdl inHdl, const void *inData, Uint32 inSize)
{
	Require(inHdl);
	
#if DEBUG
	if (::GlobalFlags((HGLOBAL)inHdl) & GMEM_LOCKCOUNT)
	{
		DebugBreak(kNoResizeLockedHdlMsg);
		Fail(errorType_Misc, error_Protocol);
	}
#endif
	
	if (::GlobalReAlloc((HGLOBAL)inHdl, inSize + 4, GMEM_MOVEABLE) == NULL)
		Fail(errorType_Memory, memError_NotEnough);
	
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	*(Uint32 *)p = inSize;
	::CopyMemory(p+4, inData, inSize);
	::GlobalUnlock((HGLOBAL)inHdl);
}

void UMemory::Set(THdl inHdl, THdl inSource)
{
	Require(inHdl && inSource);
	
#if DEBUG
	if (::GlobalFlags((HGLOBAL)inHdl) & GMEM_LOCKCOUNT)
	{
		DebugBreak(kNoResizeLockedHdlMsg);
		Fail(errorType_Misc, error_Protocol);
	}
#endif

	Uint32 s = ::GlobalSize((HGLOBAL)inSource);
	
	if (::GlobalReAlloc((HGLOBAL)inHdl, s, GMEM_MOVEABLE) == NULL)
		Fail(errorType_Memory, memError_NotEnough);
	
	void *sp = ::GlobalLock((HGLOBAL)inSource);
	void *dp = ::GlobalLock((HGLOBAL)inHdl);
	
	::CopyMemory(dp, sp, s);
	
	::GlobalUnlock((HGLOBAL)inHdl);
	::GlobalUnlock((HGLOBAL)inSource);
}

void UMemory::Clear(THdl inHdl)
{
	Require(inHdl);
	
	Uint32 s = ::GlobalSize((HGLOBAL)inHdl);
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	
	Fill(p+4, s-4, 0);
	
	::GlobalUnlock((HGLOBAL)inHdl);
}

void UMemory::Append(THdl inHdl, const void *inData, Uint32 inSize)
{
	Require(inHdl);
	if (inSize == 0) return;

#if DEBUG
	if (::GlobalFlags((HGLOBAL)inHdl) & GMEM_LOCKCOUNT)
	{
		DebugBreak(kNoResizeLockedHdlMsg);
		Fail(errorType_Misc, error_Protocol);
	}
#endif

	Uint32 existingSize = *(Uint32 *)::GlobalLock((HGLOBAL)inHdl);
	::GlobalUnlock((HGLOBAL)inHdl);
	
	Uint32 newSize = existingSize + inSize;
	
	if (::GlobalReAlloc((HGLOBAL)inHdl, newSize + 4, GMEM_MOVEABLE) == NULL)
		Fail(errorType_Memory, memError_NotEnough);
	
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	
	*(Uint32 *)p = newSize;
	::CopyMemory(p + 4 + existingSize, inData, inSize);
	
	::GlobalUnlock((HGLOBAL)inHdl);
}

// returns offset at which insert took place
// if inData is nil, inserts space for inDataSize bytes, but does not copy anything to it
Uint32 UMemory::Insert(THdl inHdl, Uint32 inOffset, const void *inData, Uint32 inDataSize)
{
	Require(inHdl);

#if DEBUG
	if (::GlobalFlags((HGLOBAL)inHdl) & GMEM_LOCKCOUNT)
	{
		DebugBreak(kNoResizeLockedHdlMsg);
		Fail(errorType_Misc, error_Protocol);
	}
#endif
	
	if (inDataSize)
	{
		// grab size from start of handle
		Uint32 s = *(Uint32 *)::GlobalLock((HGLOBAL)inHdl);
		::GlobalUnlock((HGLOBAL)inHdl);

		if (inOffset > s) inOffset = s;
		Uint32 newSize = s + inDataSize;
		
		if (::GlobalReAlloc((HGLOBAL)inHdl, newSize + 4, GMEM_MOVEABLE) == NULL)
			Fail(errorType_Memory, memError_NotEnough);
		
		Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
		*(Uint32 *)p = newSize;
		
		p += 4;
		p += inOffset;

		::MoveMemory(p + inDataSize, p, s - inOffset);
		if (inData) ::CopyMemory(p, inData, inDataSize);
		
		::GlobalUnlock((HGLOBAL)inHdl);
	}
	
	return inOffset;
}

// returns offset at which remove took place
Uint32 UMemory::Remove(THdl inHdl, Uint32 inOffset, Uint32 inSize)
{
	Require(inHdl);
	
#if DEBUG
	if (::GlobalFlags((HGLOBAL)inHdl) & GMEM_LOCKCOUNT)
	{
		DebugBreak(kNoResizeLockedHdlMsg);
		Fail(errorType_Misc, error_Protocol);
	}
#endif
	
	if (inSize)
	{
		// grab size from start of handle
		Uint32 s = *(Uint32 *)::GlobalLock((HGLOBAL)inHdl);
		::GlobalUnlock((HGLOBAL)inHdl);

		if (inOffset >= s) return s;
		if (inOffset + inSize > s) inSize = s - inOffset;	// remove to end
		Uint32 newSize = s - inSize;
		
		Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
		*(Uint32 *)p = newSize;
		
		p += 4;
		p += inOffset;
		::MoveMemory(p, p + inSize, s - (inOffset + inSize));
		::GlobalUnlock((HGLOBAL)inHdl);
		
		if (::GlobalReAlloc((HGLOBAL)inHdl, newSize + 4, GMEM_MOVEABLE) == NULL)
			Fail(errorType_Memory, memError_NotEnough);
	}
	
	return inOffset;
}

Uint32 UMemory::Replace(THdl inHdl, Uint32 inOffset, Uint32 inExistingSize, const void *inData, Uint32 inDataSize)
{
	Require(inHdl);

#if DEBUG
	if (::GlobalFlags((HGLOBAL)inHdl) & GMEM_LOCKCOUNT)
	{
		DebugBreak(kNoResizeLockedHdlMsg);
		Fail(errorType_Misc, error_Protocol);
	}
#endif

	// grab size from start of handle
	Uint32 oldSize = *(Uint32 *)::GlobalLock((HGLOBAL)inHdl);
	::GlobalUnlock((HGLOBAL)inHdl);

	Uint32 newSize;
	Uint8 *p;

	// bring offset into range
	if (inOffset > oldSize)
		inOffset = oldSize;
	
	// bring existing size into range
	if (inOffset + inExistingSize > oldSize)
		inExistingSize = oldSize - inOffset;
	
	// move existing data if necessary
	if (inExistingSize != inDataSize)
	{
		if (inDataSize > inExistingSize)
		{
			// move following data forward
			newSize = oldSize + (inDataSize - inExistingSize);
			if (::GlobalReAlloc((HGLOBAL)inHdl, newSize + 4, GMEM_MOVEABLE) == NULL)
				Fail(errorType_Memory, memError_NotEnough);
			
			p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
			*(Uint32 *)p = newSize;
			
			p += 4;
			p += inOffset;
			::MoveMemory(p + inDataSize, p + inExistingSize, oldSize - (inOffset + inExistingSize));
			::GlobalUnlock((HGLOBAL)inHdl);
		}
		else
		{
			// move following data backward
			newSize = oldSize - (inExistingSize - inDataSize);
			
			p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
			*(Uint32 *)p = newSize;
			
			p += 4;
			p += inOffset;
			::MoveMemory(p + inDataSize, p + inExistingSize, oldSize - (inOffset + inExistingSize));
			
			::GlobalUnlock((HGLOBAL)inHdl);
			
			if (::GlobalReAlloc((HGLOBAL)inHdl, newSize + 4, GMEM_MOVEABLE) == NULL)
				Fail(errorType_Memory, memError_NotEnough);
		}
	}
	
	// write the new data
	if (inData && inDataSize)
	{
		p = ((Uint8 *)::GlobalLock((HGLOBAL)inHdl)) + inOffset + 4;
		::CopyMemory(p, inData, inDataSize);
		::GlobalUnlock((HGLOBAL)inHdl);
	}
	
	// return offset at which replace occured
	return inOffset;
}

/*
 * Read() copies up to <inMaxSize> bytes from offset <inOffset> within <inHdl>
 * to <outData>, and returns the number of bytes copied.  Returns 0 if <inHdl>
 * is nil.  Returns 0 if the offset is invalid.
 */
Uint32 UMemory::Read(THdl inHdl, Uint32 inOffset, void *outData, Uint32 inMaxSize)
{
	if (inHdl == nil) return 0;
		
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	
	Uint32 s = *(Uint32 *)p;
	if (inOffset >= s) { ::GlobalUnlock((HGLOBAL)inHdl); return 0; }
	if (inOffset + inMaxSize > s) inMaxSize = s - inOffset;

	::CopyMemory(outData, p + 4 + inOffset, inMaxSize);
	
	::GlobalUnlock((HGLOBAL)inHdl);
	
	return inMaxSize;
}

/*
 * Write() copies up to <inDataSize> bytes from <inData> to offset <inOffset>
 * within <inHdl>, and returns the number of bytes copied (which might be less
 * than <inDataSize> because Write() does not attempt to resize the handle).
 * Returns 0 if the offset is invalid.
 */
Uint32 UMemory::Write(THdl inHdl, Uint32 inOffset, const void *inData, Uint32 inDataSize)
{
	Require(inHdl);
	
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	
	Uint32 s = *(Uint32 *)p;
	if (inOffset >= s) { ::GlobalUnlock((HGLOBAL)inHdl); return 0; }
	if (inOffset + inDataSize > s) inDataSize = s - inOffset;

	::CopyMemory(p + 4 + inOffset, inData, inDataSize);
	
	::GlobalUnlock((HGLOBAL)inHdl);

	return inDataSize;
}

/*
 * ReadLong() returns the 4 bytes at byte offset <inOffset> within <inHdl>.
 * Returns 0 if <inHdl> is nil.  Returns 0 if the offset is invalid.
 */
Uint32 UMemory::ReadLong(THdl inHdl, Uint32 inOffset)
{
	if (inHdl == nil) return 0;
	
	Uint32 n;
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	
	if (inOffset + sizeof(Uint32) > *(Uint32 *)p)
		n = 0;
	else
		n = *(Uint32 *)(p + 4 + inOffset);
	
	::GlobalUnlock((HGLOBAL)inHdl);
	
	return n;
}

/*
 * WriteLong() sets the 4 bytes at byte offset <inOffset> within <inHdl>
 * to <inVal>.  Does nothing if the offset is invalid.
 */
void UMemory::WriteLong(THdl inHdl, Uint32 inOffset, Uint32 inVal)
{
	Require(inHdl);
	
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	
	if (inOffset + sizeof(Uint32) <= *(Uint32 *)p)
		*(Uint32 *)(p + 4 + inOffset) = inVal;
	
	::GlobalUnlock((HGLOBAL)inHdl);
}

/*
 * ReadWord() returns the 2 bytes at byte offset <inOffset> within <inHdl>.
 * Returns 0 if <inHdl> is nil.  Returns 0 if the offset is invalid.
 */
Uint16 UMemory::ReadWord(THdl inHdl, Uint32 inOffset)
{
	if (inHdl == nil) return 0;
	
	Uint16 n;
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	
	if (inOffset + sizeof(Uint16) > *(Uint32 *)p)
		n = 0;
	else
		n = *(Uint16 *)(p + 4 + inOffset);
	
	::GlobalUnlock((HGLOBAL)inHdl);
	
	return n;
}

/*
 * WriteWord() sets the 2 bytes at byte offset <inOffset> within <inHdl>
 * to <inVal>.  Does nothing if the offset is invalid.
 */
void UMemory::WriteWord(THdl inHdl, Uint32 inOffset, Uint16 inVal)
{
	Require(inHdl);
	
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);
	
	if (inOffset + sizeof(Uint16) <= *(Uint32 *)p)
		*(Uint16 *)(p + 4 + inOffset) = inVal;
	
	::GlobalUnlock((HGLOBAL)inHdl);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
 * IsValid(THdl) returns whether or not the specified handle appears valid.
 * Note that invalid handles may appear valid.  Discarded handles are valid.
 */
bool UMemory::IsValid(THdl inHdl)
{
	return inHdl && ::GlobalFlags((HGLOBAL)inHdl) != GMEM_INVALID_HANDLE;
}

/*
 * IsValid(TPtr) returns whether or not the specified pointer appears valid.
 * Note that invalid pointers may appear valid. 
 */
bool UMemory::IsValid(TPtr inPtr)
{
#if USE_POOL_ALLOC

	_InitPoolAllocCriticalSection();
	::EnterCriticalSection(&__malloc_pool_access);

	bool valid = inPtr && __malloc_pool_initted && _MEPoolValid(&__malloc_pool, inPtr);
	
	::LeaveCriticalSection(&__malloc_pool_access);

	return valid;

#else

	// must be non-zero and allocated on 8-byte boundary
	return inPtr && (((Uint32)inPtr) & 7) == 0;
	
#endif
}

Uint32 UMemory::GetUsedSize()
{
	// don't know how to get this information
	return 0;
}

bool UMemory::IsAvailable(Uint32 /* inSize */)
{
	// virtual memory, so...
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

Uint32 UMemory::Copy(void *outDest, const void *inSource, Uint32 inSize)
{
	::MoveMemory(outDest, inSource, inSize);
	return inSize;
}

void UMemory::Insert(void *ioDest, Uint32 inDestSize, Uint32 inDestOffset, const void *inNewData, Uint32 inNewDataSize)
{
	Require(inDestOffset <= inDestSize);
	::MoveMemory(BPTR(ioDest) + inDestOffset + inNewDataSize, BPTR(ioDest) + inDestOffset, inDestSize - inDestOffset);
	if (inNewData) ::CopyMemory(BPTR(ioDest) + inDestOffset, inNewData, inNewDataSize);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void *_MESysAlloc(unsigned long size)
{
	return GlobalAlloc(GMEM_FIXED, size);
}

void _MESysFree(void *ptr)
{
	GlobalFree((HGLOBAL)ptr);
}

HGLOBAL _HdlToWinHdl(THdl inHdl, Uint32 *outSize)
{
	Require(inHdl);
	
	Uint8 *p = (Uint8 *)::GlobalLock((HGLOBAL)inHdl);

	Uint32 s = *(Uint32 *)p;
	if (outSize) *outSize = s;

	::MoveMemory(p, p + sizeof(Uint32), s);
	
	::GlobalUnlock((HGLOBAL)inHdl);
	
	return (HGLOBAL)inHdl;
}

#if USE_POOL_ALLOC
void _InitPoolAllocCriticalSection()
{
	// init only once
	static Uint8 isInitted = false;
	if (isInitted) return;
	isInitted = true;

	// this needs to be done before ::EnterCriticalSection()
	::InitializeCriticalSection(&__malloc_pool_access);
}
#endif

#endif
