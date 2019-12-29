/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
 
#include "UHttpTransact.h"

#include <Wininet.h>


static bool _IsDefaultBrowser_InternetExplorer();
static bool _IsDefaultBrowser_NetscapeNavigator();
static bool _IsDefaultBrowser(const Uint8 *inBrowserName);

static void *_GetExternalCookie_InternetExplorer(const Uint8 *inHost, const Uint8 *inDomain, Uint32& outDataSize);
static void *_GetExternalCookie_NetscapeNavigator(const Uint8 *inHost, const Uint8 *inDomain, Uint32& outDataSize);
bool _AddExternalCookie_InternetExplorer(const Uint8 *inHost, const Uint8 *inDomain, const void *inData, Uint32 inDataSize, const SCalendarDate& inExpiredDate);
bool _AddExternalCookie_NetscapeNavigator(const Uint8 *inHost, const Uint8 *inDomain, const void *inData, Uint32 inDataSize, const SCalendarDate& inExpiredDate);

static Int8 *_ComposeUrl_InternetExplorer(const Uint8 *inHost, const Uint8 *inDomain);

static TFSRefObj* _GetCookiesRef_NetscapeNavigator();
static Uint32 _ReadNextCookie_NetscapeNavigator(const Uint8 *inData, Uint32 inDataSize, Uint8 *outHost, Uint8 *outDomain, Uint8 *outCookieData, Uint32& outCookieSize, Uint32 inMaxSize);


void *UHttpTransact::GetExternalHttpCookie(const Uint8 *inHost, const Uint8 *inDomain, Uint32& outDataSize)
{
	outDataSize = 0;
	if (!inHost || !inDomain)
		return nil;

	if (_IsDefaultBrowser_InternetExplorer())
		return _GetExternalCookie_InternetExplorer(inHost, inDomain, outDataSize);
	else if (_IsDefaultBrowser_NetscapeNavigator())
		return _GetExternalCookie_NetscapeNavigator(inHost, inDomain, outDataSize);

	return nil;
}

// inExpiredDate must be in local time
bool UHttpTransact::AddExternalHttpCookie(const Uint8 *inHost, const Uint8 *inDomain, const void *inData, Uint32 inDataSize, const SCalendarDate& inExpiredDate)
{
	if (_IsDefaultBrowser_InternetExplorer())
		return _AddExternalCookie_InternetExplorer(inHost, inDomain, inData, inDataSize, inExpiredDate);
	else if (_IsDefaultBrowser_NetscapeNavigator())
		return _AddExternalCookie_NetscapeNavigator(inHost, inDomain, inData, inDataSize, inExpiredDate);

	return false;
}

#pragma mark -

static bool _IsDefaultBrowser_InternetExplorer()
{
	return _IsDefaultBrowser("\pIExplore");
}

static bool _IsDefaultBrowser_NetscapeNavigator()
{
	return _IsDefaultBrowser("\pNSShell");
}

static bool _IsDefaultBrowser(const Uint8 *inBrowserName)
{
	if (!inBrowserName || !inBrowserName[0])
		return false;
	
	HKEY hOpenKey;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, ".html", NULL, KEY_READ, &hOpenKey) != ERROR_SUCCESS)
		return false;
    
    DWORD dwType;
    Int8 bufVal[256];
    Uint32 nSize = sizeof(bufVal) - 1;
	
	if (RegQueryValueEx(hOpenKey, NULL, NULL, &dwType, (LPBYTE)(bufVal + 1), &nSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hOpenKey);
		return false;
	}

	RegCloseKey(hOpenKey);

	if (dwType != REG_SZ || !nSize)
		return false;

	bufVal[0] = nSize - 1;
    bufVal[UText::Format(bufVal, sizeof(bufVal) - 1, "%#s\\shell\\open\\ddeexec\\Application", bufVal)] = 0;

	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, bufVal, NULL, KEY_READ, &hOpenKey) != ERROR_SUCCESS)
		return false;

    nSize = sizeof(bufVal) - 1;
	if (RegQueryValueEx(hOpenKey, NULL, NULL, &dwType, (LPBYTE)(bufVal + 1), &nSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hOpenKey);
		return false;
	}

	RegCloseKey(hOpenKey);

	if (dwType != REG_SZ || !nSize)
		return false;

	bufVal[0] = nSize - 1;
	if (UText::CompareInsensitive(bufVal + 1, bufVal[0], inBrowserName + 1, inBrowserName[0]))
		return false;
		
	return true;
}

#pragma mark -

static void *_GetExternalCookie_InternetExplorer(const Uint8 *inHost, const Uint8 *inDomain, Uint32& outDataSize)
{
	Int8 *pUrl = _ComposeUrl_InternetExplorer(inHost, inDomain);
	if (!pUrl)
		return nil;
	
	Uint32 nDataSize = 0;
	::InternetGetCookie(pUrl, nil, nil, &nDataSize);
   	if (!nDataSize)
    {
    	UMemory::Dispose((TPtr)pUrl);
    	return nil;
    }

	void *pData;
	try
	{
		pData = UMemory::NewClear(nDataSize);
	}
	catch(...)
	{
    	UMemory::Dispose((TPtr)pUrl);
		throw;
	}

	bool bRet = ::InternetGetCookie(pUrl, nil, (Int8 *)pData, &nDataSize);	// get cookie
    
    UMemory::Dispose((TPtr)pUrl);
	if (!bRet)
	{
		UMemory::Dispose((TPtr)pData);
		return nil;
	}
	
	outDataSize = nDataSize - 1;	// eliminate null char
	return pData;
}

static void *_GetExternalCookie_NetscapeNavigator(const Uint8 *inHost, const Uint8 *inDomain, Uint32& outDataSize)
{
	TFSRefObj* pCookiesRef = _GetCookiesRef_NetscapeNavigator();
	if (!pCookiesRef)
		return nil;

	scopekill(TFSRefObj, pCookiesRef);
	StFileOpener pCookiesOpener(pCookiesRef, perm_Read);

	void *pData = nil;
	Uint32 nDataSize = 0;

	Uint8 bufReadData[2048];
	Uint32 nReadOffset;
	Uint32 nReadSize;
	
	Uint32 nFileOffset = 0;
	Uint8 psHost[256], psDomain[256], bufCookieData[1024];
	
	while (true)
	{
		nReadOffset = 0;
		nReadSize = pCookiesRef->Read(nFileOffset, bufReadData, sizeof(bufReadData));
		if (!nReadSize)
			break;
			
		while (true)
		{
			Uint32 nCookieSize;
			Uint32 nLineLength = _ReadNextCookie_NetscapeNavigator(bufReadData + nReadOffset, nReadSize - nReadOffset, psHost, psDomain, bufCookieData, nCookieSize, sizeof(bufCookieData));
			if (!nLineLength)
				break;
			
			nReadOffset += nLineLength;

			if (nCookieSize)
			{
				if (inHost[0] >= psHost[0] && !UText::CompareInsensitive(inHost + 1 + inHost[0] - psHost[0], psHost + 1, psHost[0]) && 	// tail matching
		    		inDomain[0] >= psDomain[0] && !UText::CompareInsensitive(inDomain + 1, psDomain + 1, psDomain[0]))					// head matching
		    	{
		    		if (pData)
						pData = UMemory::Reallocate((TPtr)pData, nDataSize + nCookieSize + 2);
					else
						pData = UMemory::NewClear(nCookieSize);
						
					if (!pData)
						return nil;
		    		
					if (nDataSize)
						nDataSize += UMemory::Copy((Uint8 *)pData + nDataSize, "; ", 2);

		    		nDataSize += UMemory::Copy((Uint8 *)pData + nDataSize, bufCookieData, nCookieSize);
		    	}
		    }
		}
		
		if (!nReadOffset)
			break;
			
		nFileOffset += nReadOffset;
	}
	
	outDataSize = nDataSize;
	return pData;
}

// inExpiredDate must be in local time
bool _AddExternalCookie_InternetExplorer(const Uint8 *inHost, const Uint8 *inDomain, const void *inData, Uint32 inDataSize, const SCalendarDate& inExpiredDate)
{
	Int8 *pUrl = _ComposeUrl_InternetExplorer(inHost, inDomain);
	if (!pUrl)
		return false;

	// convert expired date to "Wdy, DD-Mon-YYYY HH:MM:SS GMT"
	Uint8 psExpiredDate[64];psExpiredDate[0] = 0;
	if (inExpiredDate.IsValid())
	{
		// convert from local time to GMT
		SCalendarDate stExpiredDate = inExpiredDate - UDateTime::GetGMTDelta();
	
		const Int8 *pWeekDayList[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
		const Int8 *pMonthList[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

		psExpiredDate[0] = UText::Format(psExpiredDate + 1, sizeof(psExpiredDate) - 1, "; expires = %s, %2.2hu-%s-%hu %2.2hu:%2.2hu:%2.2hu GMT", pWeekDayList[stExpiredDate.weekDay - 1], stExpiredDate.day, pMonthList[stExpiredDate.month - 1], stExpiredDate.year, stExpiredDate.hour, stExpiredDate.minute, stExpiredDate.second);
	}
		
	Int8 *pData;
	try
	{
		pData = (Int8 *)UMemory::New(inDataSize + psExpiredDate[0] + 1);
	}
	catch(...)
	{
    	UMemory::Dispose((TPtr)pUrl);
		throw;
	}

	UMemory::Copy(pData, inData, inDataSize);
	UMemory::Copy(pData + inDataSize, psExpiredDate + 1, psExpiredDate[0]);
	*(pData + inDataSize + psExpiredDate[0]) = 0;		// append null char

	bool bRet = ::InternetSetCookie(pUrl, nil, pData);	// set cookie

   	UMemory::Dispose((TPtr)pUrl);
	UMemory::Dispose((TPtr)pData);

	return bRet;
}

// inExpiredDate must be in local time
bool _AddExternalCookie_NetscapeNavigator(const Uint8 *inHost, const Uint8 *inDomain, const void *inData, Uint32 inDataSize, const SCalendarDate& inExpiredDate)
{
	// if the cookie expires when the user's session ends don't write in the file
	if (!inExpiredDate.IsValid())
		return true;

	Uint8 *pValue = UMemory::SearchByte('=', inData, inDataSize);
	if (!pValue)
		return false;

	TFSRefObj* pCookiesRef = _GetCookiesRef_NetscapeNavigator();
	if (!pCookiesRef)
		return false;

	scopekill(TFSRefObj, pCookiesRef);
	
	// convert from local time to GMT
	SDateTimeStamp stExpiredDate = inExpiredDate - UDateTime::GetGMTDelta();	
	stExpiredDate.ConvertYear(1970);	// calculate seconds elapsed from midnight, January 1, 1970

	Uint8 psExpiredDate[16];
	psExpiredDate[0] = UText::Format(psExpiredDate + 1, sizeof(psExpiredDate) - 1, "%lu", stExpiredDate.seconds);
	
	Uint32 nOffset = pCookiesRef->GetSize();
	StFileOpener pCookiesOpener(pCookiesRef, perm_ReadWrite);

	// format shall be: "host \t isDomain \t domain \t isSecure \t expiresDate \t name \t value \r\r\n"
	nOffset += pCookiesRef->Write(nOffset, inHost + 1, inHost[0]);					// write host
	nOffset += pCookiesRef->Write(nOffset, "\tTRUE\t", 6);							// write isDomain flag
	nOffset += pCookiesRef->Write(nOffset, inDomain + 1, inDomain[0]);				// write domain
	nOffset += pCookiesRef->Write(nOffset, "\tFALSE\t", 7);							// write isSecure flag
	nOffset += pCookiesRef->Write(nOffset, psExpiredDate + 1, psExpiredDate[0]);	// write expiresDate
	nOffset += pCookiesRef->Write(nOffset, "\t", 1);								// write tab
	nOffset += pCookiesRef->Write(nOffset, inData, pValue - inData);				// write cookie name
	nOffset += pCookiesRef->Write(nOffset, "\t", 1);								// write tab
	pValue++;																		// skip '='
	nOffset += pCookiesRef->Write(nOffset, pValue, inDataSize - (pValue - inData));	// write cookie value
	pCookiesRef->Write(nOffset, "\r\r\n", 3);										// write end of line

	return true;	
}

#pragma mark -

static Int8 *_ComposeUrl_InternetExplorer(const Uint8 *inHost, const Uint8 *inDomain)
{
	Uint32 nUrlSize = inHost[0] + inDomain[0] + 1;
	bool bAddHttp = false, bAddWWW = false;
	
	if (inHost[0] < 7 || UText::CompareInsensitive(inHost + 1, "http://", 7))
	{
		nUrlSize += 7;
		bAddHttp = true;
		
		if (*(inHost + 1) == '.')
		{
			nUrlSize += 3;
			bAddWWW = true;
		}
	}
	
	Int8 *pUrl = (Int8 *)UMemory::New(nUrlSize);
	if (!pUrl)
		return nil;
	
	Uint32 nOffset = 0;
	if (bAddHttp)
	{
		nOffset = UMemory::Copy(pUrl, "http://", 7);			// add "http://"
	
		if (bAddWWW)
			nOffset += UMemory::Copy(pUrl + nOffset, "www", 3);	// add "www"
	}

	nOffset += UMemory::Copy(pUrl + nOffset, inHost + 1, inHost[0]);
	nOffset += UMemory::Copy(pUrl + nOffset, inDomain + 1, inDomain[0]);
	*(pUrl + nOffset) = 0;
	
	return pUrl;
}

extern TFSRefObj* _FSNewRefFromWinPath(const void *inPath, Uint32 inPathSize);

static TFSRefObj* _GetCookiesRef_NetscapeNavigator()
{
	HKEY hOpenKey;

	// search Nescape path
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Netscape\\Netscape Navigator\\Main", NULL, KEY_READ, &hOpenKey) != ERROR_SUCCESS)
		return nil;

    DWORD dwType;
    Int8 bufPath[1024];
    Uint32 nPathSize = sizeof(bufPath);
	
	if (RegQueryValueEx(hOpenKey, "Install Directory", NULL, &dwType, (LPBYTE)bufPath, &nPathSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hOpenKey);
		return nil;
	}

	RegCloseKey(hOpenKey);

	nPathSize--;
	if (dwType != REG_SZ || nPathSize <= 12 || UText::CompareInsensitive(bufPath + nPathSize - 12, "Communicator", 12))
		return nil;

	nPathSize -= 12;

	// search Netscape default user
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Netscape\\Netscape Navigator\\biff", NULL, KEY_READ, &hOpenKey) != ERROR_SUCCESS)
		return nil;

    Int8 bufUser[256];
    Uint32 nUserSize = sizeof(bufUser);
    
	if (RegQueryValueEx(hOpenKey, "CurrentUser", NULL, &dwType, (LPBYTE)bufUser, &nUserSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hOpenKey);
		return nil;
	}

	RegCloseKey(hOpenKey);

	nUserSize--;
	if (dwType != REG_SZ)
		return nil;
	
	// compose cookies file path
	nPathSize += UMemory::Copy(bufPath + nPathSize, "Users\\", 6);
	nPathSize += UMemory::Copy(bufPath + nPathSize, bufUser, nUserSize);
	nPathSize += UMemory::Copy(bufPath + nPathSize, "\\cookies.txt", 12);

	// make file ref
	TFSRefObj* pCookiesRef = _FSNewRefFromWinPath(bufPath, nPathSize);
		
	bool bIsFolder;
	if (!pCookiesRef->Exists(&bIsFolder) || bIsFolder)
	{
		UFileSys::Dispose(pCookiesRef);
		return nil;
	}
	
	return pCookiesRef;
}

// format shall be: "host \t isDomain \t domain \t isSecure \t expiresDate \t name \t value \r\r\n"
static Uint32 _ReadNextCookie_NetscapeNavigator(const Uint8 *inData, Uint32 inDataSize, Uint8 *outHost, Uint8 *outDomain, Uint8 *outCookieData, Uint32& outCookieSize, Uint32 inMaxSize)
{
	outCookieSize = 0;
	if (!inData || !inDataSize)
		return 0;
		
	// the end of line in "cookies.txt" is "\r\r\n"
	Uint8 *pLineEnd = UMemory::Search("\r\r\n", 3, inData, inDataSize);
	if (!pLineEnd)
		return 0;

	Uint32 nLineLength = pLineEnd + 3 - inData;
	if (*inData == '#' || nLineLength <= 3)			// comment line or empty line
		return nLineLength;

	// search host
	Uint8 *pHostEnd = UMemory::SearchByte('\t', inData, pLineEnd - inData);
	if (!pHostEnd)
		return 0;
	
	outHost[0] = UMemory::Copy(outHost + 1, inData, pHostEnd - inData > 255 ? 255 : pHostEnd - inData);
	
	// skip isDomain
	Uint8 *pFieldBegin = pHostEnd + 1;
	Uint8 *pFieldEnd = UMemory::SearchByte('\t', pFieldBegin, pLineEnd - pFieldBegin);
	if (!pFieldEnd)
		return 0;
	
	// search domain
	Uint8 *pDomainBegin = pFieldEnd + 1;
	Uint8 *pDomainEnd = UMemory::SearchByte('\t', pDomainBegin, pLineEnd - pDomainBegin);
	if (!pDomainEnd)
		return 0;

	outDomain[0] = UMemory::Copy(outDomain + 1, pDomainBegin, pDomainEnd - pDomainBegin > 255 ? 255 : pDomainEnd - pDomainBegin);
	
	// skip isSecure
	pFieldBegin = pDomainEnd + 1;
	pFieldEnd = UMemory::SearchByte('\t', pFieldBegin, pLineEnd - pFieldBegin);
	if (!pFieldEnd)
		return 0;
		
	// check expiresDate
	Uint8 *pDateBegin = pFieldEnd + 1;
	Uint8 *pDateEnd = UMemory::SearchByte('\t', pDateBegin, pLineEnd - pDateBegin);
	if (!pDateEnd)
		return 0;

	SDateTimeStamp stExpiredDate;
	stExpiredDate.year = 1970;	//  seconds elapsed from midnight, January 1, 1970
	stExpiredDate.seconds = UText::TextToUInteger(pDateBegin, pDateEnd - pDateBegin);
	stExpiredDate.msecs = 0;
	
	// convert from GMT to local time
	stExpiredDate += UDateTime::GetGMTDelta();

	SDateTimeStamp stCurrentDate;
	UDateTime::GetDateTimeStamp(stCurrentDate);
	
	// check if this cookie has expired
	if (stCurrentDate >= stExpiredDate)
		return nLineLength;

	// search cookie name
	Uint8 *pNameBegin = pDateEnd + 1;
	Uint8 *pNameEnd = UMemory::SearchByte('\t', pNameBegin, pLineEnd - pNameBegin);
	if (!pNameEnd)
		return 0;

	outCookieSize = UMemory::Copy(outCookieData, pNameBegin, pNameEnd - pNameBegin > inMaxSize - 1 ? inMaxSize - 1 : pNameEnd - pNameBegin) + 1;
	*(outCookieData + outCookieSize - 1) = '=';
	
	// search cookie value
	Uint8 *pValueBegin = pNameEnd + 1;
	outCookieSize += UMemory::Copy(outCookieData + outCookieSize, pValueBegin, pLineEnd - pValueBegin > inMaxSize - outCookieSize ? inMaxSize - outCookieSize : pLineEnd - pValueBegin);

	return nLineLength;
}





