/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// Utility class that converts between Unicode characters and ASCII

#ifndef _H_UStringConverter_
#define _H_UStringConverter_ 

#if PRAGMA_ONCE
	#pragma once
#endif

HL_Begin_Namespace_BigRedH


class UStringConverter
{
	public:
		enum EStringEncoding
		{
			eGenericAscii,
			eMacAscii,
			ePCAscii,

			#if TARGET_OS_MAC
				eDefaultEncoding = eMacAscii
			#elif TARGET_OS_WIN32
				eDefaultEncoding = ePCAscii
			#elif TARGET_OS_UNIX
				eDefaultEncoding = eGenericAscii
			#endif
			
		};

	private:
								// creates a CString from a c style string
								// in a given encoding
		static CString	CStrToUnistring(
									const char* inCStr,
									const EStringEncoding inStringEncoding = eDefaultEncoding );

								// creates a CString from a pascal style
								// string in a given encoding
		static CString	PStrToUnistring(
									const UInt8* inPStr,
									const EStringEncoding inStringEncoding = eDefaultEncoding );

								// creates a CString from data and size
								// in a given encoding
		static CString	StringDataToUnistring(
									const void* inCStr,
									const UInt32 inSize,
									const EStringEncoding inStringEncoding = eDefaultEncoding );

								// creates a cstring from a CString
								// (allocates memory for it)
		static char*		UnistringToCStr(
									const CString& inStr,
									const EStringEncoding inEncoding = eDefaultEncoding,
									const bool inAllowUndefined = true,
									const char inReplaceCharacter = 0 );

								// creates a p string from a c string
								// (allocates memory for it)
		static UInt8*		UnistringToPStr(
									const CString& inStr,
									const EStringEncoding inEncoding = eDefaultEncoding,
									const bool inAllowUndefined = true,
									const char inReplaceCharacter = 0 );

								// creates string data given a unicode
								// string (allocates memory for it)
		static void			UnistringToStringData(
									const CString& inStr,
									void* &outData,
									UInt32& outSize,
									const EStringEncoding inEncoding = eDefaultEncoding,
									const bool inAllowUndefined = true,
									const char inReplaceCharacter = 0 );

								// converts an ascii character in a given
								// encoding into a unicode character
		static wchar_t		CharToUnichar(
									const char inChar,
									const EStringEncoding inEncoding = eDefaultEncoding );

								// converts a unicode character in a given
								// encoding into a unicode character
		static char			UnicharToChar(
									const wchar_t inChar,
									const EStringEncoding inEncoding = eDefaultEncoding,
									const bool inAllowUndefined = true,
									const char inReplaceCharaceter = 0 );
	
								// don't construct
								UStringConverter();
		static bool			ConvertUnicodeToAscii(
									wchar_t inChar,
									const EStringEncoding inEncoding,
									char& outChar );

	friend class CString;
	friend class StCStyleString;
	friend class StCopybackCStyleString;
	friend class StPStyleString;
	friend class StCopybackPStyleString;
};


HL_End_Namespace_BigRedH
#endif	//_H_UStringConverter_