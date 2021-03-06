#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UClipboard.h"

static bool _CLIsOpen = false;

void _MIMEToMacTypeCode(const void *inText, Uint32 inSize, Uint32& outTypeCode, Uint32& outCreatorCode);
void *_ImageToMacPict(TImage inImage, const SRect *inRect);
TImage _MacPictToImage(void *inHdl, SRect *outRect);
THdl _MacSoundToPortaSound(void *inHdl);

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

/* -------------------------------------------------------------------------- */

// returns nil if no data
THdl UClipboard::GetData(const Int8 *inType)
{
	Uint32 macType, n;
	_MIMEToMacTypeCode(inType, strlen(inType), macType, n);
	if (macType == 0) 
		return nil;
	
#if TARGET_API_MAC_CARBON
	ScrapRef currentScarp;
	CheckMacError(::GetCurrentScrap(&currentScarp));
	
	Size byteCount;
	long result = ::GetScrapFlavorSize(currentScarp, macType, &byteCount);
	if (result < 0)
		return nil;
		
	Handle h = ::NewHandle(byteCount);
	if (h == nil) 
		return nil;
		
	HLock(h);
	result = ::GetScrapFlavorData(currentScarp, macType, &byteCount, *h);
	HUnlock(h);
#else
	Handle h = ::NewHandle(0);
	if (h == nil) 
		return nil;
	
	long scrapOffset;
	long result = ::GetScrap(h, macType, &scrapOffset);
#endif
	
	if (result < 0)
	{
		::DisposeHandle(h);
		return nil;
	}
	
	// insert a lock count (convert to THdl)
	n = 0;
	Munger(h, 0, nil, 0, &n, sizeof(n));
	result = ::MemError();
	if (result)
	{
		::DisposeHandle(h);
		FailMacError(result);
	}
	
	return (THdl)h;
}

// if you just want one item on the clipboard, you don't have to call BeginSet/EndSet
void UClipboard::SetData(const Int8 *inType, const void *inData, Uint32 inDataSize)
{
	Uint32 macType, macCreator;

	_MIMEToMacTypeCode(inType, strlen(inType), macType, macCreator);
	if (macType == 0) return;
	
#if TARGET_API_MAC_CARBON
	if (!_CLIsOpen) 
		CheckMacError(::ClearCurrentScrap());
		
	ScrapRef currentScarp;
	CheckMacError(::GetCurrentScrap(&currentScarp));
	
	CheckMacError(::PutScrapFlavor(currentScarp, macType, kScrapFlavorMaskNone, inDataSize, inData));
#else
	if (!_CLIsOpen) 
		CheckMacError(::ZeroScrap());

	CheckMacError(::PutScrap(inDataSize, macType, (Ptr)inData));
#endif
}

void UClipboard::BeginSet()
{
	if (_CLIsOpen)
	{
		DebugBreak("UClipboard - BeginSet() already called");
		Fail(errorType_Misc, error_Protocol);
	}
	
#if TARGET_API_MAC_CARBON
	CheckMacError(::ClearCurrentScrap());
#else	
	CheckMacError(::ZeroScrap());
#endif

	_CLIsOpen = true;
}

void UClipboard::EndSet()
{
	_CLIsOpen = false;
}

// does not modify nor take ownership of <inImage>
void UClipboard::SetImageData(TImage inImage, const SRect *inRect, Uint32 /* inOptions */)
{
	Handle h = (Handle)_ImageToMacPict(inImage, inRect);
	
	try
	{
		HLock(h);
		
	#if TARGET_API_MAC_CARBON
		if (!_CLIsOpen) 
			CheckMacError(::ClearCurrentScrap());

		ScrapRef currentScarp;
		CheckMacError(::GetCurrentScrap(&currentScarp));
	
		CheckMacError(::PutScrapFlavor(currentScarp, kScrapFlavorTypePicture, kScrapFlavorMaskNone, ::GetHandleSize(h), *h));
	#else
		if (!_CLIsOpen) 
			CheckMacError(::ZeroScrap());
			
		CheckMacError(::PutScrap(::GetHandleSize(h), 'PICT', *h));
	#endif
	}
	catch(...)
	{
		::DisposeHandle(h);
		throw;
	}
	
	::DisposeHandle(h);
}

// returns nil if no image data on the clipboard
TImage UClipboard::GetImageData(SRect *outRect, Uint32 /* inOptions */)
{
#if TARGET_API_MAC_CARBON
	ScrapRef currentScarp;
	CheckMacError(::GetCurrentScrap(&currentScarp));
	
	Size byteCount;
	long result = ::GetScrapFlavorSize(currentScarp, kScrapFlavorTypePicture, &byteCount);
	if (result < 0)
		return nil;
		
	Handle h = ::NewHandle(byteCount);
	if (h == nil) 
		return nil;
		
	HLock(h);
	result = ::GetScrapFlavorData(currentScarp, kScrapFlavorTypePicture, &byteCount, *h);
	HUnlock(h);
#else	
	Handle h = ::NewHandle(0);
	if (h == nil) 
		return nil;

	long scrapOffset;
	long result = ::GetScrap(h, 'PICT', &scrapOffset);
#endif
	
	if (result < 0)
	{
		::DisposeHandle(h);
		return nil;
	}

	TImage image;
	try
	{
		image = _MacPictToImage((void *)h, outRect);
	}
	catch(...)
	{
		::DisposeHandle(h);
		throw;
	}
	
	::DisposeHandle(h);
	
	return image;
}

// does not modify nor take ownership of <inHdl>
void UClipboard::SetSoundData(THdl inHdl, Uint32 /* inOptions */)
{
	Require(inHdl);
	Fail(errorType_Misc, error_Unimplemented);
}

THdl UClipboard::GetSoundData(Uint32 /* inOptions */)
{
#if TARGET_API_MAC_CARBON
	ScrapRef currentScarp;
	CheckMacError(::GetCurrentScrap(&currentScarp));
	
	Size byteCount;
	long result = ::GetScrapFlavorSize(currentScarp, kScrapFlavorTypeSound, &byteCount);
	if (result < 0)
		return nil;
		
	Handle h = ::NewHandle(byteCount);
	if (h == nil) 
		return nil;
		
	HLock(h);
	result = ::GetScrapFlavorData(currentScarp, kScrapFlavorTypeSound, &byteCount, *h);
	HUnlock(h);
#else
	Handle h = ::NewHandle(0);
	if (h == nil) 
		return nil;

	long scrapOffset;
	long result = ::GetScrap(h, 'snd ', &scrapOffset);
#endif
	
	if (result < 0)
	{
		::DisposeHandle(h);
		return nil;
	}

	THdl portaSound;
	try
	{
		portaSound = _MacSoundToPortaSound((void *)h);
	}
	catch(...)
	{
		::DisposeHandle(h);
		throw;
	}
	
	::DisposeHandle(h);
	
	return portaSound;
}

#endif /* MACINTOSH */
