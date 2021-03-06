/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
#include "AW.h"
#include "CString.h"
#include "StString.h"
#include "UStringConverter.h"
#include <cctype>

/*
#include <UnicodeUtilities.h>
#include <UnicodeConverter.h>
*/
HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  operator==                                                  [public]
// ---------------------------------------------------------------------
// 
bool CString::operator== (const CString& rhs) const {	
/*	if (CEnvironmentMac::HasUnicode()) {
		::Boolean equivalent;
		::SInt32  order;
		OSStatus osErr = ::UCCompareTextDefault ( kUCCollateWidthInsensitiveMask | kUCCollateDiacritInsensitiveMask
								, reinterpret_cast<const ::UniChar *>(this->mString.data()), this->length()
								, reinterpret_cast<const ::UniChar *>(rhs.mString.data()), rhs.length()
								, &equivalent
								, &order );
		if (equivalent) return true;
		else return false;
	}*/
	return mString == rhs.mString;
}

// ---------------------------------------------------------------------
//  operator<                                                   [public]
// ---------------------------------------------------------------------
// 
bool
CString::operator< (const CString& rhs) const
{
/*	if (CEnvironmentMac::HasUnicode()) {
		::Boolean equivalent;
		::SInt32  order;
		OSStatus osErr = ::UCCompareTextDefault ( kUCCollateWidthInsensitiveMask | kUCCollateDiacritInsensitiveMask
								, reinterpret_cast<const ::UniChar *>(mString.data()), mString.length()
								, reinterpret_cast<const ::UniChar *>(rhs.mString.data()), rhs.length()
								, &equivalent
								, &order );
		if (order < 0) return true;
		else return false;
	}*/
	return mString < rhs.mString;
}

/*::UnicodeMapping ASCIIConversionMapping = {
	 kTextEncodingMacUnicode
	,kTextEncodingMacRoman
	,kUnicodeUseLatestMapping
};*/

/*std::size_t
CString::ToASCII(void* outBuffer, std::size_t inBufferLen, bool inAllowFallbacks) const
{
	if (CEnvironmentMac::HasUnicode()) {
		::UnicodeToTextInfo ASCIIConversionInfo;
		::CreateUnicodeToTextInfo(&ASCIIConversionMapping, &ASCIIConversionInfo);
		
		::ByteCount inputRead;
		::ByteCount outputLength;
		OSErr err = ::ConvertFromUnicodeToText(ASCIIConversionInfo
							,mString.length() * 2
							,reinterpret_cast<const UniChar*>(mString.data())
							, kUnicodeDefaultDirectionMask | (inAllowFallbacks)?kUnicodeUseFallbacksMask:0
							, 0, 0, 0, 0
							, inBufferLen
							, &inputRead, &outputLength, outBuffer);
		
		::DisposeUnicodeToTextInfo(&ASCIIConversionInfo);
		
		if (err != noErr) 
		{}	// throw up here...  Note that there are a great many error codes, which are intelligent and we should handle appropriately
		return outputLength;
	}
	// other conversion
	return 0;
}*/

// ---------------------------------------------------------------------
//  CString                                                     [public]
// ---------------------------------------------------------------------
// 
CString::CString (const char* inPtr)
{
	Init(inPtr, std::strlen(inPtr));
}

// ---------------------------------------------------------------------
//  CString                                                     [public]
// ---------------------------------------------------------------------
// 
CString::CString (const unsigned char* inPtr)
{
	Init(&inPtr[1], inPtr[0]);
}

// ---------------------------------------------------------------------
//  CString                                                     [public]
// ---------------------------------------------------------------------
// 
CString::CString (const void* inPtr, std::size_t inLength)
{
	Init(inPtr, inLength);
}

// ---------------------------------------------------------------------
//  Init                                                       [private]
// ---------------------------------------------------------------------
// 	
void
CString::Init (const void* inPtr, std::size_t inLength)
{
	*this = UStringConverter::StringDataToUnistring(inPtr, inLength);
}

// ---------------------------------------------------------------------
//  compareNoCase                                               [public]
// ---------------------------------------------------------------------
// 
int
CString::compareNoCase (const CString& inStr) const
{
	StCStyleString myString (*this);
	StCStyleString otherString (*this);
	for (int i=0;i<std::min(inStr.length(), length());++i) {
		int a = std::tolower(static_cast<char*>(myString)[i]);
		int b = std::tolower(static_cast<char*>(otherString)[i]);
		if (a<b) return -1;
		else if (a>b) return 1;
	}
	if (inStr.length() == length()) return 0;
	if (inStr.length() > length()) return 1;
	else return -1;
}

HL_End_Namespace_BigRedH