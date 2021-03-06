#if WIN32

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "URegularTransport.h"
#include "UOleAutomation.h"
#include <WININET.H>

/*
 * Structures and Types
 */

struct SSendBuf {
	SSendBuf *next;
	Uint32 offset;		// for when part of the buffer was sent straight away
	Uint32 dataSize;
	Uint8 data[];
};

struct SFirewallInfo
{
	Uint8 fwAddr[4];			// address of firewall			
	Uint8 destAddr[4];			// address of our dest
	HANDLE lookupTask;			// for DNS lookups
	TPtr lookupBuf;
	Uint32 timeout;				// in milliseconds
	Uint16 port;				// destination port
	Uint16 status;				// status - mainly used for dns lookups
};

struct SReceivedData
{
	void *pData;
	Uint32 nDataSize;
	Uint32 nReceivedSize;
	Uint32 nReceivedOffset;
};

struct SRegularTransport {
	SRegularTransport *next;
	SOCKET sock;
	SSendBuf *sendQueue;
	TMessageProc msgProc;
	void *msgProcContext;
	void *msgObject;
	TTransportMonitorProc dataMonitorProc;
	HANDLE lookupTask;		// for DNS lookups in StartConnect()
	HANDLE unitLookupTask;	// for DNS lookups in SendUnit()
	TPtr lookupBuf, sendUnitBuf;
	SIPAddress localIPAddress;
	SFirewallInfo *firewallInfo;
	Uint32 sendUnitBufSize;
	SReceivedData receivedData;
	Uint32 notifyReceivedSize;
	Uint16 lookupBufPort;
	Uint16 connectResult;
	Uint8 isBound;
	Uint8 isConnected;
	Uint8 checkReadyToSend;
	Uint8 disconWhenNoOutData;
	Uint8 isStream;
};

#define TPT		((SRegularTransport *)inTpt)

enum {
	kNotComplete	= 0xFFFF
};

enum
{
	kFWStat_Connecting		= 0,		// connected to firewall and awaiting authentication OK
	kFWStat_FWDNS			= 1,		// looking up firewall ip
	kFWStat_DestDNS			= 2,		// looking up destination ip

	kFWStat_Authenticating	= 16,
	kFWStat_Ready			= 32		// connected to firewall and authenticated - connection is open - not really necessary

};

/*
 * Macros
 */
 
void _FailLastWinError(const Int8 *inFile, Uint32 inLine);
void _FailWinError(Int32 inWinError, const Int8 *inFile, Uint32 inLine);
#if DEBUG
	#define FailLastWinError()		_FailLastWinError(__FILE__, __LINE__)
	#define FailWinError(id)		_FailWinError(id, __FILE__, __LINE__)
#else
	#define FailLastWinError()		_FailLastWinError(nil, 0)
	#define FailWinError(id)		_FailWinError(id, nil, 0)
#endif


#define WM_SOCKET_NOTIFY				0x0373
#define WM_SOCKET_LOOKUPANDCONNECT		0x0373
#define WM_SOCKET_LOOKUPANDSENDTO		0x0374

// can I define my own constants?  these don't look to arbitrary...
#define WM_SOCKET_FWLOOKUPDEST			0x0375
#define WM_SOCKET_FWLOOKUPANDCONNECT	0x0376

/*
 * Function Prototypes
 */

static Uint32 _TRWinToStdAddr(const void *inAddr, Uint32 inSize, void *outData, Uint32 inMaxSize);
static Uint32 _TRStdToWinAddr(const void *inAddr, Uint32 inSize, void *outData, Uint32 inMaxSize);
static bool _TRNotifyReadyToReceive(TRegularTransport inTpt, Uint32 inReceivedSize, bool inPostMessage = true);
static LRESULT CALLBACK _TRSockProc(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam);
static LRESULT CALLBACK _TRAsyncProc(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam);
static void _TRExtractNameAddr(const SInternetNameAddress& inAddr, Int8 *outName, Uint16 *outPort);
static void _TRSendQueued(TRegularTransport inTpt);
static void _TRBindIPAddress(TRegularTransport inTpt, Uint16 inPort = 0);
static void _TRTryConnectViaFirewall(SRegularTransport *inTpt);

static bool _IsRegisteredProtocol_InternetExplorer(const Uint8 *inProtocol);
static bool _IsRegisteredForProtocol_InternetExplorer(const Uint8 *inProtocol);
static bool _RegisterProtocol_InternetExplorer(const Uint8 *inProtocol);
static bool _IsRegisteredForProtocol_NetscapeNavigator(const Uint8 *inProtocol, const Int8 *inProgID);
static bool _RegisterProtocol_NetscapeNavigator(const Uint8 *inProtocol, const Int8 *inProgID);

/*
 * Global Variables
 */

extern HINSTANCE _gProgramInstance;
static ATOM _TRSockClassAtom = 0;	// for use with WSAAsyncSelect
	static HWND _TRSockWin = 0;			// for use with WSAAsyncSelect
static ATOM _TRAsyncClassAtom = 0;	// for use with WSAAsyncGetHostByName
static HWND _TRAsyncWin = 0;		// for use with WSAAsyncGetHostByName
static SRegularTransport *_TRFirsTRegularTransport = nil;
static HINSTANCE _hWininetLib = nil;// because WININET.DLL doesn't exist in some early Win95 versions we will use dynamic link

Uint32 _TRSendBufSize = 65536;	// 64K


/* -------------------------------------------------------------------------- */

bool URegularTransport::IsAvailable()
{
	return true;
}

bool URegularTransport::HasTCP()
{
	return true;
}

static void _Deinit()
{
	// free WININET.DLL library
	if (_hWininetLib)
	{
		::FreeLibrary(_hWininetLib);
		_hWininetLib = nil;
	}

	::WSACleanup();
}

void URegularTransport::Init()
{
	DWORD err;
	WSADATA wsa;
	WNDCLASSEX wndclass;
	
	if (_TRSockClassAtom == 0)
	{
		// fill in window class info
		wndclass.cbSize = sizeof(WNDCLASSEX);
		wndclass.style = 0;
		wndclass.lpfnWndProc = _TRSockProc;
		wndclass.cbClsExtra = wndclass.cbWndExtra = 0;
		wndclass.hInstance = _gProgramInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "URegularTransport-sock";
		wndclass.hIconSm = NULL;
	
		// register window class
		_TRSockClassAtom = RegisterClassEx(&wndclass);
		if (_TRSockClassAtom == 0) FailLastWinError();
	}
	
	if (_TRSockWin == 0)
	{
		_TRSockWin = CreateWindow((char *)_TRSockClassAtom, "", WS_OVERLAPPEDWINDOW, 0, 0, 32, 32, NULL, NULL, _gProgramInstance, NULL);
		if (_TRSockWin == 0) FailLastWinError();
	}
	
	if (_TRAsyncClassAtom == 0)
	{
		// fill in window class info
		wndclass.cbSize = sizeof(WNDCLASSEX);
		wndclass.style = 0;
		wndclass.lpfnWndProc = _TRAsyncProc;
		wndclass.cbClsExtra = wndclass.cbWndExtra = 0;
		wndclass.hInstance = _gProgramInstance;
		wndclass.hIcon = NULL;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "URegularTransport-async";
		wndclass.hIconSm = NULL;
	
		// register window class
		_TRAsyncClassAtom = RegisterClassEx(&wndclass);
		if (_TRAsyncClassAtom == 0) FailLastWinError();
	}
	
	if (_TRAsyncWin == 0)
	{
		_TRAsyncWin = CreateWindow((char *)_TRAsyncClassAtom, "", WS_OVERLAPPEDWINDOW, 0, 0, 32, 32, NULL, NULL, _gProgramInstance, NULL);
		if (_TRAsyncWin == 0) FailLastWinError();
	}

	err = ::WSAStartup(MAKEWORD(1, 1), &wsa);
	if (err) FailWinError(err);	

	// load WININET.DLL library
	_hWininetLib = ::LoadLibrary("WININET.DLL");

	UProgramCleanup::InstallSystem(_Deinit);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

TRegularTransport URegularTransport::New(Uint32 inProtocol, Uint32 /* inOptions */)
{
	SRegularTransport *tpt;
	int type;
	SOCKET sock;
	DWORD err;
		
	switch (inProtocol)
	{
		case transport_TCPIP:
			type = SOCK_STREAM;
			break;
		case transport_UDPIP:
			type = SOCK_DGRAM;
			break;
		default:
			Fail(errorType_Transport, transError_InvalidConfig);
			break;
	}

	tpt = (SRegularTransport *)UMemory::NewClear(sizeof(SRegularTransport));
	if (type == SOCK_STREAM) tpt->isStream = true;
	
	// open socket
	sock = ::socket(PF_INET, type, 0);
	if (sock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		UMemory::Dispose((TPtr)tpt);
		FailWinError(err);
	}
	
	// increase send buffer
	int sndbufval = _TRSendBufSize;
	if (::setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&sndbufval, sizeof(sndbufval)) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		::closesocket(sock);
		UMemory::Dispose((TPtr)tpt);
		FailWinError(err);
	}

	// install asyncronous handling
	if (::WSAAsyncSelect(sock, _TRSockWin, WM_SOCKET_NOTIFY, FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE|FD_ACCEPT) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		::closesocket(sock);
		UMemory::Dispose((TPtr)tpt);
		FailWinError(err);
	}
	
	// successfully setup socket
	tpt->sock = sock;
	
	// add to transporter list
	tpt->next = _TRFirsTRegularTransport;
	_TRFirsTRegularTransport = tpt;

	return (TRegularTransport)tpt;
}

// also closes connection.  Any unsent data is not sent
void URegularTransport::Dispose(TRegularTransport inTpt)
{
	if (inTpt)
	{
		// remove from transporter list
		SRegularTransport *tm = _TRFirsTRegularTransport;
		SRegularTransport *ptm = nil;
		while (tm)
		{
			if (tm == TPT)
			{
				if (ptm)
					ptm->next = tm->next;
				else
					_TRFirsTRegularTransport = tm->next;
				break;
			}
			ptm = tm;
			tm = tm->next;
		}
		
		::closesocket(TPT->sock);
		
		if (TPT->lookupTask) ::WSACancelAsyncRequest(TPT->lookupTask);
		if (TPT->unitLookupTask) ::WSACancelAsyncRequest(TPT->unitLookupTask);
		UMemory::Dispose((TPtr)TPT->lookupBuf);
		UMemory::Dispose((TPtr)TPT->sendUnitBuf);
		
		// dispose of all firewall stuff
		if (TPT->firewallInfo)
		{
			if (TPT->firewallInfo->lookupTask) ::WSACancelAsyncRequest(TPT->firewallInfo->lookupTask);
			
			UMemory::Dispose((TPtr)TPT->firewallInfo->lookupBuf);
			UMemory::Dispose((TPtr)TPT->firewallInfo);
		}
				
		// dispose received data
		if (TPT->receivedData.pData)
			UMemory::Dispose((TPtr)TPT->receivedData.pData);
		
		// dispose unsent data
		SSendBuf *nextbuf;
		SSendBuf *buf = TPT->sendQueue;
		while (buf)
		{
		#if DEBUG
			if (::IsBadWritePtr(buf, sizeof(SSendBuf))
				|| ::IsBadWritePtr(buf->data, buf->dataSize))
				DebugBreak("URegularTransport::Dispose - bad send buffers!");
		#endif

			nextbuf = buf->next;
			UMemory::Dispose((TPtr)buf);
			buf = nextbuf;
		}
		
		if (TPT->msgProc)
			UApplication::FlushMessages(TPT->msgProc, TPT->msgProcContext, TPT->msgObject);
		
		UMemory::Dispose((TPtr)inTpt);
	}
}

void URegularTransport::SetDataMonitor(TRegularTransport inTpt, TTransportMonitorProc inProc)
{
	Require(inTpt);
	TPT->dataMonitorProc = inProc;
}

void URegularTransport::SetMessageHandler(TRegularTransport inTpt, TMessageProc inProc, void *inContext, void *inObject)
{
	Require(inTpt);
	
	TPT->msgProc = inProc;
	TPT->msgProcContext = inContext;
	TPT->msgObject = inObject;
}

void URegularTransport::PostMessage(TRegularTransport inTpt, Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority)
{
	Require(inTpt);
	if (TPT->msgProc)
		UApplication::PostMessage(inMsg, inData, inDataSize, inPriority, TPT->msgProc, TPT->msgProcContext, TPT->msgObject);
}

void URegularTransport::ReplaceMessage(TRegularTransport inTpt, Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority)
{
	Require(inTpt);
	if (TPT->msgProc)
		UApplication::ReplaceMessage(inMsg, inData, inDataSize, inPriority, TPT->msgProc, TPT->msgProcContext, TPT->msgObject);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

SIPAddress URegularTransport::GetDefaultIPAddress()
{
	SIPAddress defaultIPAddress;
	defaultIPAddress.un_IP.stDW_IP.nDW_IP = 0;
		
	char str[256];
	if (::gethostname(str, sizeof(str)) == 0)
	{
		struct hostent *he = ::gethostbyname(str);
		if (he) 
		{
			Uint8 buf[32];
			((sockaddr_in *)buf)->sin_addr.s_addr = *(Uint32 *)(he->h_addr_list[0]);
	
			in_addr *ia = &((sockaddr_in *)buf)->sin_addr;
			
			defaultIPAddress.un_IP.stB_IP.nB_IP1 = ia->S_un.S_un_b.s_b1;
			defaultIPAddress.un_IP.stB_IP.nB_IP2 = ia->S_un.S_un_b.s_b2;
			defaultIPAddress.un_IP.stB_IP.nB_IP3 = ia->S_un.S_un_b.s_b3;
			defaultIPAddress.un_IP.stB_IP.nB_IP4 = ia->S_un.S_un_b.s_b4;
		}
	}

	return defaultIPAddress;
}

void URegularTransport::SetIPAddress(TRegularTransport inTpt, SIPAddress inIPAddress)
{
	Require(inTpt);
	
	if (TPT->localIPAddress.un_IP.stDW_IP.nDW_IP != inIPAddress.un_IP.stDW_IP.nDW_IP)
	{
		TPT->localIPAddress = inIPAddress;
		TPT->isBound = false;
	}
}

SIPAddress URegularTransport::GetIPAddress(TRegularTransport inTpt)
{
	Require(inTpt);
	
	// check the saved address
	if (!TPT->localIPAddress.IsNull())
		return TPT->localIPAddress;

	// get the address
	Uint8 buf[256];
	int size = sizeof(buf);
	in_addr *ia;
		
	if (::getsockname(TPT->sock, (sockaddr *)buf, &size) != 0)
		FailWinError(::WSAGetLastError());
	
	ia = &((sockaddr_in *)buf)->sin_addr;

	// check the address
	if (ia->s_addr == 0)
	{
		// grab the default address
		char str[256];
		if (::gethostname(str, sizeof(str)) == 0)
		{
			struct hostent *he = ::gethostbyname(str);
			if (he) 
				((sockaddr_in *)buf)->sin_addr.s_addr = *(Uint32 *)(he->h_addr_list[0]);
		}
	}

	SIPAddress localIPAddress;
	localIPAddress.un_IP.stB_IP.nB_IP1 = ia->S_un.S_un_b.s_b1;
	localIPAddress.un_IP.stB_IP.nB_IP2 = ia->S_un.S_un_b.s_b2;
	localIPAddress.un_IP.stB_IP.nB_IP3 = ia->S_un.S_un_b.s_b3;
	localIPAddress.un_IP.stB_IP.nB_IP4 = ia->S_un.S_un_b.s_b4;

	// save the address
	if (TPT->isBound)
		TPT->localIPAddress.un_IP.stDW_IP.nDW_IP = localIPAddress.un_IP.stDW_IP.nDW_IP;

	return localIPAddress;
}

Uint32 URegularTransport::GetRemoteAddressText(TRegularTransport inTpt, void *outAddr, Uint32 inMaxSize)
{
	Uint8 buf[256];
	int size = sizeof(buf);
	in_addr *ia;
	
	Require(inTpt);
	
	if (::getpeername(TPT->sock, (sockaddr *)buf, &size) != 0)
		FailWinError(::WSAGetLastError());
	
	ia = &((sockaddr_in *)buf)->sin_addr;
	
	return UText::Format(outAddr, inMaxSize, "%d.%d.%d.%d", (int)ia->S_un.S_un_b.s_b1, (int)ia->S_un.S_un_b.s_b2, (int)ia->S_un.S_un_b.s_b3, (int)ia->S_un.S_un_b.s_b4);
}

Uint32 URegularTransport::GetLocalAddressText(TRegularTransport inTpt, void *outAddr, Uint32 inMaxSize)
{
	Require(inTpt && inMaxSize > 15);
	
	// check the saved address
	if (TPT->isBound && !TPT->localIPAddress.IsNull())
		return UText::Format(outAddr, inMaxSize, "%d.%d.%d.%d", TPT->localIPAddress.un_IP.stB_IP.nB_IP1, TPT->localIPAddress.un_IP.stB_IP.nB_IP2, TPT->localIPAddress.un_IP.stB_IP.nB_IP3, TPT->localIPAddress.un_IP.stB_IP.nB_IP4);

	// get the address
	Uint8 buf[256];
	int size = sizeof(buf);
	in_addr *ia;
		
	if (::getsockname(TPT->sock, (sockaddr *)buf, &size) != 0)
		FailWinError(::WSAGetLastError());
	
	ia = &((sockaddr_in *)buf)->sin_addr;

	// check the address
	if (ia->s_addr == 0)
	{
		// grab the default address
		char str[256];
		if (::gethostname(str, sizeof(str)) == 0)
		{
			struct hostent *he = ::gethostbyname(str);
			if (he) 
				((sockaddr_in *)buf)->sin_addr.s_addr = *(Uint32 *)(he->h_addr_list[0]);
		}
	}

	// save the address
	if (TPT->isBound)
	{
		TPT->localIPAddress.un_IP.stB_IP.nB_IP1 = ia->S_un.S_un_b.s_b1;
		TPT->localIPAddress.un_IP.stB_IP.nB_IP2 = ia->S_un.S_un_b.s_b2;
		TPT->localIPAddress.un_IP.stB_IP.nB_IP3 = ia->S_un.S_un_b.s_b3;
		TPT->localIPAddress.un_IP.stB_IP.nB_IP4 = ia->S_un.S_un_b.s_b4;
	}
	
	return UText::Format(outAddr, inMaxSize, "%d.%d.%d.%d", (int)ia->S_un.S_un_b.s_b1, (int)ia->S_un.S_un_b.s_b2, (int)ia->S_un.S_un_b.s_b3, (int)ia->S_un.S_un_b.s_b4);
}

Uint32 URegularTransport::GetLocalAddress(TRegularTransport inTpt, void *outAddr, Uint32 inMaxSize)
{
	Require(inTpt && inMaxSize > 15);

	Uint8 buf[256];
	int size = sizeof(buf);
	
	// get the address
	if (::getsockname(TPT->sock, (sockaddr *)buf, &size) != 0)
		FailWinError(::WSAGetLastError());
	
	// check the address
	if (((sockaddr_in *)buf)->sin_addr.s_addr == 0)
	{
		// grab the default address
		char str[256];
		if (::gethostname(str, sizeof(str)) == 0)
		{
			struct hostent *he = ::gethostbyname(str);
			if (he) 
				((sockaddr_in *)buf)->sin_addr.s_addr = *(Uint32 *)(he->h_addr_list[0]);
		}
	}

	SInternetAddress *oa = (SInternetAddress *)outAddr;
	
	oa->type = kInternetAddressType;
	oa->port = ((sockaddr_in *)buf)->sin_port;
	*(Uint32 *)oa->host = ((sockaddr_in *)buf)->sin_addr.s_addr;
	((Uint32 *)oa->host)[1] = 0;
	
	// save the address
	if (TPT->isBound && TPT->localIPAddress.IsNull())
		TPT->localIPAddress.un_IP.stDW_IP.nDW_IP = *(Uint32 *)oa->host;

	return sizeof(SInternetAddress);
}

Uint32 URegularTransport::GetProxyServerAddress(Uint16 inType, void *outProxyAddr, Uint32 inMaxSize, const void *inBypassAddr, Uint32 inBypassAddrSize)
{
	// because WININET.DLL doesn't exist in some early Win95 versions we will use dynamic link
	if (!_hWininetLib)
		return 0;

	typedef BOOL (CALLBACK* LPPROCDLL_InternetQueryOption)(HINTERNET, DWORD, LPVOID, LPDWORD);
	LPPROCDLL_InternetQueryOption lpProc_InternetQueryOption = (LPPROCDLL_InternetQueryOption)::GetProcAddress(_hWininetLib, "InternetQueryOptionA");
	if (!lpProc_InternetQueryOption)
		return 0;

	void *pProxyInfo = nil;
	Uint32 nProxySize = 512;

	try
	{
		pProxyInfo = UMemory::NewClear(nProxySize);
	}
	catch(...)
	{
		return 0;
	}
	 
//	if (!::InternetQueryOption(NULL, INTERNET_OPTION_PROXY, pProxyInfo, &nProxySize))
	if (!lpProc_InternetQueryOption(NULL, INTERNET_OPTION_PROXY, pProxyInfo, &nProxySize))
	{
		try
		{
			pProxyInfo = UMemory::Reallocate((TPtr)pProxyInfo, nProxySize);
		}
		catch(...)
		{
			return 0;
		}
			
//		if (!::InternetQueryOption(NULL, INTERNET_OPTION_PROXY, pProxyInfo, &nProxySize))
		if (!lpProc_InternetQueryOption(NULL, INTERNET_OPTION_PROXY, pProxyInfo, &nProxySize))
		{
			UMemory::Dispose((TPtr)pProxyInfo);						
			return 0;
		}
	}
		
	INTERNET_PROXY_INFO *pProxyStruct = (INTERNET_PROXY_INFO *)pProxyInfo;
	if (pProxyStruct->dwAccessType != INTERNET_OPEN_TYPE_PROXY)
	{
		UMemory::Dispose((TPtr)pProxyInfo);		
		return 0;
	}
	
	// search for bypass address
	if (inBypassAddr && inBypassAddrSize)
	{
		Uint32 nLpszBypassSize = strlen(pProxyStruct->lpszProxyBypass);
		Uint8 *pBypassOffset = (Uint8 *)pProxyStruct->lpszProxyBypass;
	
		while (true)
		{
			Uint8 *pBypassEnd = UMemory::SearchByte(' ', pBypassOffset, (Uint8 *)pProxyStruct->lpszProxyBypass + nLpszBypassSize - pBypassOffset);
	
			if (!pBypassEnd)
				pBypassEnd = (Uint8 *)pProxyStruct->lpszProxyBypass + nLpszBypassSize;
		
			if (pBypassOffset >= pBypassEnd)
				break;
				
			Uint32 nAddrSize = pBypassEnd - pBypassOffset;
			if (nAddrSize <= inBypassAddrSize && !UMemory::Compare(inBypassAddr, pBypassOffset, nAddrSize))
			{
				UMemory::Dispose((TPtr)pProxyInfo);				
				return 0;
			}
			
			pBypassOffset = pBypassEnd + 1;
			if (pBypassOffset >= (Uint8 *)pProxyStruct->lpszProxyBypass + nLpszBypassSize)
				break;
		}
	}
	
	Uint32 nLpszProxySize = strlen(pProxyStruct->lpszProxy);
	Uint8 *pProxyHttp = UMemory::Search("http=", 5, pProxyStruct->lpszProxy, nLpszProxySize);
	Uint8 *pProxyHttpSecure = UMemory::Search("https=", 6, pProxyStruct->lpszProxy, nLpszProxySize);
	Uint8 *pProxyFtp = UMemory::Search("ftp=", 4, pProxyStruct->lpszProxy, nLpszProxySize);
	Uint8 *pProxyGopher = UMemory::Search("gopher=", 7, pProxyStruct->lpszProxy, nLpszProxySize);
	Uint8 *pProxySocks = UMemory::Search("socks=", 6, pProxyStruct->lpszProxy, nLpszProxySize);

	if (!pProxyHttp && !pProxyHttpSecure && !pProxyFtp && !pProxyGopher && !pProxySocks)
	{
		Uint32 nAddrSize = nLpszProxySize > inMaxSize ? inMaxSize : nLpszProxySize;
		UMemory::Copy(outProxyAddr, pProxyStruct->lpszProxy, nAddrSize);
		UMemory::Dispose((TPtr)pProxyInfo);
		
		return nAddrSize;
	}
		
	Uint8 *pProxyAddrBegin = nil;
	switch (inType)
	{
		case proxy_HttpAddress: 		if (pProxyHttp) pProxyAddrBegin = pProxyHttp + 5;break;
		case proxy_HttpSecureAddress: 	if (pProxyHttpSecure) pProxyAddrBegin = pProxyHttpSecure + 6;break;
		case proxy_FtpAddress: 			if (pProxyFtp) pProxyAddrBegin = pProxyFtp + 4;break;
		case proxy_GopherAddress: 		if (pProxyGopher) pProxyAddrBegin = pProxyGopher + 7;break;
		case proxy_SocksAddress: 		if (pProxySocks) pProxyAddrBegin = pProxySocks + 6;break;
	};
		
	if (!pProxyAddrBegin)
	{
		UMemory::Dispose((TPtr)pProxyInfo);		
		return 0;
	}
	
	Uint8 *pProxyAddrEnd = UMemory::SearchByte(' ', pProxyAddrBegin, (Uint8 *)pProxyStruct->lpszProxy + nLpszProxySize - pProxyAddrBegin);
	
	if (!pProxyAddrEnd)
		pProxyAddrEnd = (Uint8 *)pProxyStruct->lpszProxy + nLpszProxySize;
	
	if (pProxyAddrBegin >= pProxyAddrEnd)
	{
		UMemory::Dispose((TPtr)pProxyInfo);		
		return 0;
	}
	
	Uint32 nAddrSize = pProxyAddrEnd - pProxyAddrBegin > inMaxSize ? inMaxSize : pProxyAddrEnd - pProxyAddrBegin;
	UMemory::Copy(outProxyAddr, pProxyAddrBegin, nAddrSize);

	UMemory::Dispose((TPtr)pProxyInfo);
		
	return nAddrSize;
}
	   
bool URegularTransport::GetEthernetAddress(SEthernetAddress& outEthernetAddr)
{
	struct ASTAT {
		ADAPTER_STATUS stAdapt;
		NAME_BUFFER stNameBuff[30];
	} stAdapter;

	// enumerate the LAN adapter (LANA) number
	NCB stNcb;
	LANA_ENUM stLanaEnum;
	memset(&stNcb, 0, sizeof(stNcb));
	memset(&stLanaEnum, 0, sizeof(stLanaEnum));
	stNcb.ncb_command = NCBENUM;
	stNcb.ncb_buffer = (UCHAR *)&stLanaEnum;
	stNcb.ncb_length = sizeof(stLanaEnum);
	UCHAR uRetCode = ::Netbios(&stNcb);
	if (uRetCode != 0)
		stLanaEnum.length = 1;	// if error, assume there is 1 LAN adapter

	for (int i = 0; i < stLanaEnum.length; i++)
	{	
		// reset the LAN adapter
		::memset(&stNcb, 0, sizeof(stNcb));
		stNcb.ncb_command = NCBRESET;
		stNcb.ncb_lana_num = stLanaEnum.lana[i];
		uRetCode = Netbios(&stNcb);
		if (uRetCode != 0)
			continue;

		// retrive the status of the LAN adapter
		::memset(&stNcb, 0, sizeof(stNcb));
		stNcb.ncb_command = NCBASTAT;
		stNcb.ncb_lana_num = stLanaEnum.lana[i];
		::strcpy((char *)stNcb.ncb_callname, "*               ");
		stNcb.ncb_buffer = (unsigned char *)&stAdapter;
		stNcb.ncb_length = sizeof(stAdapter);
		uRetCode = Netbios(&stNcb);
		if (uRetCode != 0 || stAdapter.stAdapt.adapter_type != 0xFE)	// check if is a Ethernet adapter
			continue;
		
		// get the MAC (Media Access Control) address associated with the LAN adapter
		::memcpy(outEthernetAddr.eaddr, stAdapter.stAdapt.adapter_address, sizeof(outEthernetAddr.eaddr));
		return true;
	}
	
	outEthernetAddr.SetNull();
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool URegularTransport::IsConnected(TRegularTransport inTpt)
{
	return inTpt && TPT->isConnected;
}

void URegularTransport::StartConnectThruFirewall(TRegularTransport inTpt, const void *inAddr, Uint32 inAddrSize, const void *inFirewallAddr, Uint32 inFirewallAddrSize, Uint32 inMaxSecs, Uint32 /* inOptions */)
{
	Uint8 winAddr[256];
	Uint32 winAddrSize;
	int err;
	
	Require(inTpt);
	
	if (TPT->isConnected)
	{
		DebugBreak("URegularTransport - already connected");
		Fail(errorType_Misc, error_Protocol);
	}
	if (TPT->connectResult == kNotComplete || TPT->lookupTask != 0)
	{
		DebugBreak("URegularTransport - already connecting");
		Fail(errorType_Misc, error_Protocol);
	}

	SFirewallInfo *info = (SFirewallInfo *)UMemory::NewClear(sizeof(SFirewallInfo));
	
	
	// first let's try to find the destination ip
	try
	{
		TPT->firewallInfo = info;
		
		info->timeout = inMaxSecs ? inMaxSecs * 1000 : 30000;

		
		winAddrSize = _TRStdToWinAddr(inAddr, inAddrSize, winAddr, sizeof(winAddr));
		if (winAddrSize == 0) Fail(errorType_Transport, transError_BadAddress);
		
		if (winAddrSize == max_Uint32)	// if DNS name
		{
			_TRExtractNameAddr(*(SInternetNameAddress *)inAddr, (Int8 *)winAddr, &info->port);
			
			info->lookupBuf = UMemory::New(MAXGETHOSTSTRUCT);
	
			info->status |= kFWStat_DestDNS;

			info->lookupTask = ::WSAAsyncGetHostByName(_TRAsyncWin, WM_SOCKET_FWLOOKUPDEST, (char *)winAddr, (char *)info->lookupBuf, MAXGETHOSTSTRUCT);
			
			if (info->lookupTask == 0)
			{
				err = WSAGetLastError();
				UMemory::Dispose((TPtr)info->lookupBuf);
				info->lookupBuf = nil;
				FailWinError(err);
			}
		}
		else	// got an IP address
		{
			*(Uint32 *)info->destAddr = ((SOCKADDR_IN *)winAddr)->sin_addr.S_un.S_addr;
			info->port = htons(((SOCKADDR_IN *)winAddr)->sin_port);

		}
		

		// now for the firewall IP
		
		winAddrSize = _TRStdToWinAddr(inFirewallAddr, inFirewallAddrSize, winAddr, sizeof(winAddr));
		if (winAddrSize == 0) Fail(errorType_Transport, transError_BadAddress);
		
		if (winAddrSize == max_Uint32)	// if DNS name
		{
			_TRExtractNameAddr(*(SInternetNameAddress *)inFirewallAddr, (Int8 *)winAddr, &TPT->lookupBufPort);
			
			TPT->lookupBuf = UMemory::New(MAXGETHOSTSTRUCT);

			info->status |= kFWStat_FWDNS;
			
			TPT->lookupTask = ::WSAAsyncGetHostByName(_TRAsyncWin, WM_SOCKET_FWLOOKUPANDCONNECT, (char *)winAddr, (char *)TPT->lookupBuf, MAXGETHOSTSTRUCT);
			
			if (TPT->lookupTask == 0)
			{
				err = WSAGetLastError();
				UMemory::Dispose((TPtr)TPT->lookupBuf);
				TPT->lookupBuf = nil;
				FailWinError(err);
			}
		}
		else if (info->status)
		{
			// need to store the port and IP for later when we get the ip of the destination
			// port is in network-byte-order so reverse it with htons
			TPT->lookupBufPort = htons(((SOCKADDR_IN *)winAddr)->sin_port);
			*((Uint32 *)info->fwAddr) = ((SOCKADDR_IN *)winAddr)->sin_addr.S_un.S_addr;
		}
		else //(!info->status)	// got an IP address, so can start the connect straight away if I don't need to look up the above addr
		{
			// bind if necessary
			if (!TPT->isBound)
				_TRBindIPAddress(inTpt);
			
			info->status = kFWStat_Connecting;
						
			TPT->connectResult = kNotComplete;
			err = ::connect(TPT->sock, (sockaddr *)winAddr, winAddrSize);
			if (err == SOCKET_ERROR)
			{
				err = ::WSAGetLastError();
				if (err != WSAEWOULDBLOCK)	// WSAEWOULDBLOCK is normal
				{
					TPT->connectResult = 0;
					FailWinError(err);
				}
			}
		}
	}
	catch(...)
	{
		TPT->firewallInfo = nil;
		UMemory::Dispose((TPtr)info);
		throw;
	}
}

void URegularTransport::StartConnect(TRegularTransport inTpt, const void *inAddr, Uint32 inAddrSize, Uint32 /* inMaxSecs */)
{
	Uint8 winAddr[256];
	Uint32 winAddrSize;
	int err;
	
	Require(inTpt);
	
	if (TPT->isConnected)
	{
		DebugBreak("URegularTransport - already connected");
		Fail(errorType_Misc, error_Protocol);
	}
	if (TPT->connectResult == kNotComplete || TPT->lookupTask != 0)
	{
		DebugBreak("URegularTransport - already connecting");
		Fail(errorType_Misc, error_Protocol);
	}
	
	winAddrSize = _TRStdToWinAddr(inAddr, inAddrSize, winAddr, sizeof(winAddr));
	if (winAddrSize == 0) Fail(errorType_Transport, transError_BadAddress);
	
	if (winAddrSize == max_Uint32)	// if DNS name
	{
		_TRExtractNameAddr(*(SInternetNameAddress *)inAddr, (Int8 *)winAddr, &TPT->lookupBufPort);
		
		TPT->lookupBuf = UMemory::New(MAXGETHOSTSTRUCT);
		
		TPT->lookupTask = ::WSAAsyncGetHostByName(_TRAsyncWin, WM_SOCKET_LOOKUPANDCONNECT, (char *)winAddr, (char *)TPT->lookupBuf, MAXGETHOSTSTRUCT);
		
		if (TPT->lookupTask == 0)
		{
			err = WSAGetLastError();
			UMemory::Dispose((TPtr)TPT->lookupBuf);
			TPT->lookupBuf = nil;
			FailWinError(err);
		}
	}
	else	// got an IP address, so can start the connect straight away
	{
		// bind if necessary
		if (!TPT->isBound)
			_TRBindIPAddress(inTpt);

		TPT->connectResult = kNotComplete;
		err = ::connect(TPT->sock, (sockaddr *)winAddr, winAddrSize);
		if (err == SOCKET_ERROR)
		{
			err = ::WSAGetLastError();
			if (err != WSAEWOULDBLOCK)	// WSAEWOULDBLOCK is normal
			{
				TPT->connectResult = 0;
				FailWinError(err);
			}
		}
	}
}

Uint16 URegularTransport::GetConnectStatus(TRegularTransport inTpt)
{
	Require(inTpt);
	
	if (TPT->isConnected)
		return kConnectionEstablished;
	
	if (TPT->connectResult == kNotComplete || TPT->lookupTask != 0)
		return kWaitingForConnection;

	if (TPT->firewallInfo)
	{
		if (TPT->firewallInfo->status == kFWStat_Ready)
			return kConnectionEstablished;
		else
			return kWaitingForConnection;

	}

	FailWinError(TPT->connectResult);
	return kWaitingForConnection;
}

/*
 * Disconnect() sends an abortive disconnect, which is effective
 * immediately.  Any unsent data will NOT get sent.  Note that
 * once the TRegularTransport is disconnected, you CAN'T use StartConnect()
 * on it.  A TRegularTransport can only be connected ONCE.
 */
void URegularTransport::Disconnect(TRegularTransport inTpt)
{
	if (inTpt)
	{
		::shutdown(TPT->sock, 2);
		
		if (TPT->lookupTask)
		{
			::WSACancelAsyncRequest(TPT->lookupTask);
			TPT->lookupTask = 0;
		}
		UMemory::Dispose((TPtr)TPT->lookupBuf);
		TPT->lookupBuf = nil;
		
		// dispose unsent data
		SSendBuf *nextbuf;
		SSendBuf *buf = TPT->sendQueue;
		TPT->sendQueue = nil;
		while (buf)
		{
			nextbuf = buf->next;
			UMemory::Dispose((TPtr)buf);
			buf = nextbuf;
		}
	}
}

/*
 * StartDisconnect() starts an orderly disconnect.  Any unsent
 * data will be sent before the connection closes.  You cannot
 * send any more data after this call, but you CAN receive data.
 * A msg_ConnectionClosed will be sent once the disconnect
 * is complete.  IsConnected() returns TRUE up until this time.
 * You can receive data even after the connection has closed.
 */
void URegularTransport::StartDisconnect(TRegularTransport inTpt)
{
	if (inTpt)
	{
		if (TPT->sendQueue == nil)		// if no outgoing data
			Disconnect(inTpt);			// then we can disconnect now
		else
			TPT->disconWhenNoOutData = true;
	}
}

/*
 * IsDisconnecting() returns TRUE if the connection is in the
 * process of being closed (either because StartDisconnect() has
 * been called, or the remote host has requested a disconnect).
 * When TRUE, you should NOT send any more data.  When all unsent
 * data has been actually sent, the connection will close with a
 * msg_ConnectionClosed.  You can continue to receive data even
 * after the connection has closed.
 */
bool URegularTransport::IsDisconnecting(TRegularTransport inTpt)
{
	return inTpt && TPT->disconWhenNoOutData;
}

// returns true if the specified TRegularTransport is in the process of connection (returns false if connected or disconnected)
bool URegularTransport::IsConnecting(TRegularTransport inTpt)
{
	return inTpt && (TPT->connectResult == kNotComplete || TPT->lookupTask != 0);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// establishes a TRegularTransport to listen for incoming connections
void URegularTransport::Listen(TRegularTransport inTpt, const void *inAddr, Uint32 inAddrSize)
{
	Require(inTpt);
	
	if ((inAddrSize < sizeof(SInternetAddress)) || (*(Uint16 *)inAddr != kInternetAddressType))
		Fail(errorType_Transport, transError_BadAddress);
	
	_TRBindIPAddress(inTpt, ((Uint16 *)inAddr)[1]);
	
	if (TPT->isStream && ::listen(TPT->sock, 10) != 0)
		FailWinError(::WSAGetLastError());
}

// returns NIL if no connection to accept
TRegularTransport URegularTransport::Accept(TRegularTransport inTpt, void *outAddr, Uint32 *ioAddrSize)
{
	SOCKET sock;
	SRegularTransport *tpt;
	Uint8 addr[256];
	int addrSize;
		
	if (inTpt)
	{
		addrSize = sizeof(addr);
		sock = ::accept(TPT->sock, (sockaddr *)addr, &addrSize);
		
		if (sock != INVALID_SOCKET)
		{
			if (outAddr && ioAddrSize)
				*ioAddrSize = _TRWinToStdAddr(addr, addrSize, outAddr, *ioAddrSize);
			
			try
			{
				tpt = (SRegularTransport *)UMemory::NewClear(sizeof(SRegularTransport));
				tpt->sock = sock;
				tpt->isConnected = true;
				tpt->isStream = TPT->isStream;
				
				// note don't need to call WSAAsyncSelect, it's set the same as the listening socket
			}
			catch(...)
			{
				::closesocket(sock);
				throw;
			}
			
			// add to transporter list
			tpt->next = _TRFirsTRegularTransport;
			_TRFirsTRegularTransport = tpt;
			
			return (TRegularTransport)tpt;
		}
	}
	
	return nil;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void *URegularTransport::NewBuffer(Uint32 inSize)
{
	SSendBuf *buf = (SSendBuf *)UMemory::New(sizeof(SSendBuf) + inSize);
	
	buf->next = nil;
	buf->offset = 0;
	buf->dataSize = inSize;
	
	return buf->data;
}

void URegularTransport::DisposeBuffer(void *inBuf)
{
	if (inBuf) UMemory::Dispose((TPtr)(BPTR(inBuf) - sizeof(SSendBuf)));
}

Uint32 URegularTransport::GetBufferSize(void *inBuf)
{
	return inBuf ? ((Uint32 *)inBuf)[-1] : 0;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// takes ownership of the buffer
void URegularTransport::SendBuffer(TRegularTransport inTpt, void *inBuf)
{
	SSendBuf *buf, *link;
	int result;

	Require(inTpt && inBuf);
	
	buf = (SSendBuf *)(BPTR(inBuf) - sizeof(SSendBuf));
	
	// call the data monitor proc
	if (TPT->dataMonitorProc)
	{
		try {
			TPT->dataMonitorProc(inTpt, buf->data, buf->dataSize, true);
		} catch(...) {}
	}
	
	if (TPT->sendQueue)		// if there's a queue, must enqueue this new data
	{
		// add to end of queue
		link = TPT->sendQueue;
		while (link->next)
			link = link->next;
	
		link->next = buf;
	}
	else	// no queue, try sending now, then enqueue what doesn't get sent
	{
		result = ::send(TPT->sock, (char *)buf->data, buf->dataSize, 0);
		if (result < 0) result = 0;		// if error, no data got sent
		
		if (result < buf->dataSize)		// if only part of the data got sent, enqueue the rest
		{
			buf->offset = result;
			TPT->sendQueue = buf;
		}
		else	// all of it got sent, can kill the buffer
			UMemory::Dispose((TPtr)buf);
	}
}

void URegularTransport::Send(TRegularTransport inTpt, const void *inData, Uint32 inDataSize)
{
	SSendBuf *buf, *link;
	int result;
	
	Require(inTpt);
	
	if (TPT->sendQueue)		// if there's a queue, must enqueue this new data
	{
		buf = (SSendBuf *)UMemory::New(sizeof(SSendBuf) + inDataSize);
		
		buf->next = nil;
		buf->offset = 0;
		buf->dataSize = inDataSize;
		::CopyMemory(buf->data, inData, inDataSize);
		
		// add to end of queue
		link = TPT->sendQueue;
		while (link->next)
			link = link->next;
		link->next = buf;
	}
	else	// no queue, try sending now, then enqueue what doesn't get sent
	{
		result = ::send(TPT->sock, (char *)inData, inDataSize, 0);
		if (result < 0) result = 0;		// if error, no data got sent
		
		if (result < inDataSize)		// if only part of the data got sent, enqueue the rest
		{
			Uint32 bytesLeft = inDataSize - result;
			
			buf = (SSendBuf *)UMemory::New(sizeof(SSendBuf) + bytesLeft);
			
			buf->next = nil;
			buf->offset = 0;
			buf->dataSize = bytesLeft;
			::CopyMemory(buf->data, BPTR(inData) + result, bytesLeft);
			
			TPT->sendQueue = buf;
		}
	}
	
	// call the data monitor proc
	if (TPT->dataMonitorProc)
	{
		try {
			TPT->dataMonitorProc(inTpt, inData, inDataSize, true);
		} catch(...) {}
	}
}

bool URegularTransport::ReceiveBuffer(TRegularTransport inTpt, void*& outBuf, Uint32& outSize)
{
	outBuf = nil;
	outSize = 0;
	
	if (inTpt == nil) 
		return false;				
		
	DWORD nSize = 0;
	Uint32 nReceivedSize = TPT->receivedData.nReceivedSize - TPT->receivedData.nReceivedOffset;
	
	if (::ioctlsocket(TPT->sock, FIONREAD, &nSize) != 0)
	{
		if (nReceivedSize)
		{
			outBuf = URegularTransport::NewBuffer(nReceivedSize);
			if (!outBuf)
				return false;
			
			outSize = nReceivedSize;
			UMemory::Copy(outBuf, (Uint8 *)TPT->receivedData.pData + TPT->receivedData.nReceivedOffset, nReceivedSize);
								
			UMemory::Dispose((TPtr)TPT->receivedData.pData);
			ClearStruct(TPT->receivedData);

			if (TPT->notifyReceivedSize)
				TPT->notifyReceivedSize = 0;

			return true;
		}
		
		return false;
	}
			
	if (nReceivedSize + nSize)
	{
		void *pBuf = URegularTransport::NewBuffer(nReceivedSize + nSize);
		if (!pBuf)
			return false;
			
		if (nReceivedSize)
		{
			UMemory::Copy(pBuf, (Uint8 *)TPT->receivedData.pData + TPT->receivedData.nReceivedOffset, nReceivedSize);
								
			UMemory::Dispose((TPtr)TPT->receivedData.pData);
			ClearStruct(TPT->receivedData);			
		}

		// even if nSize is 0 ::recv must be called because FD_READ message must be re-enabled
		int nResult = ::recv(TPT->sock, (char *)((Uint8 *)pBuf + nReceivedSize), nSize, 0); 
			
		if (nReceivedSize || nResult > 0)
		{
			outBuf = pBuf;
			outSize = nReceivedSize;
			if (nResult > 0)
				outSize += nResult;
						
			if (TPT->notifyReceivedSize)
				TPT->notifyReceivedSize = 0;
	
			// call the data monitor proc
			if (TPT->dataMonitorProc)
			{
				try {
					TPT->dataMonitorProc(inTpt, outBuf, outSize, false);
				} catch(...) {}
			}
				
			return true;
		}
		
		URegularTransport::DisposeBuffer(pBuf);
	}
			
	return false;
}

Uint32 URegularTransport::Receive(TRegularTransport inTpt, void *outData, Uint32 inMaxSize)
{
	if (inTpt == nil) 
		return 0;
	
	Uint32 nReceivedSize = TPT->receivedData.nReceivedSize - TPT->receivedData.nReceivedOffset;
	if (nReceivedSize > inMaxSize)
		nReceivedSize = inMaxSize;
	
	if (nReceivedSize)
	{
		UMemory::Copy(outData, (Uint8 *)TPT->receivedData.pData + TPT->receivedData.nReceivedOffset, nReceivedSize);
		TPT->receivedData.nReceivedOffset += nReceivedSize;
		inMaxSize -= nReceivedSize;
		
		if (TPT->receivedData.nReceivedSize == TPT->receivedData.nReceivedOffset)
		{
			UMemory::Dispose((TPtr)TPT->receivedData.pData);
			ClearStruct(TPT->receivedData);			
		}
	}

	// even if inMaxSize is 0 ::recv must be called because FD_READ message must be re-enabled
	int nResult = ::recv(TPT->sock, (char *)((Uint8 *)outData + nReceivedSize), inMaxSize, 0);
		
	if (nReceivedSize || nResult > 0)
	{
		Uint32 nDataSize = nReceivedSize;
		if (nResult > 0)
			nDataSize += nResult;

		if (TPT->notifyReceivedSize)
			TPT->notifyReceivedSize = 0;

		// call the data monitor proc
		if (TPT->dataMonitorProc)
		{
			try {
				TPT->dataMonitorProc(inTpt, outData, nDataSize, false);
			} catch(...) {}
		}
		
		return nDataSize;
	}
	
	return 0;
}

Uint32 URegularTransport::GetReceiveSize(TRegularTransport inTpt)
{
	if (inTpt == nil) 
		return 0;
	
	Uint32 nReceivedSize = TPT->receivedData.nReceivedSize - TPT->receivedData.nReceivedOffset;

	DWORD nSize;
	if (::ioctlsocket(TPT->sock, FIONREAD, &nSize) != 0)
		return nReceivedSize;
		
	return (nReceivedSize + nSize);
}

Uint32 URegularTransport::GetUnsentSize(TRegularTransport inTpt)
{
	if (inTpt == nil) return 0;
	
	SSendBuf *buf = TPT->sendQueue;
	Uint32 s = 0;
	
	while (buf)
	{
		s += buf->dataSize - buf->offset;
		buf = buf->next;
	}
	
	return s;
}

void URegularTransport::NotifyReadyToReceive(TRegularTransport inTpt, Uint32 inReceivedSize)
{
	Require(inTpt);
	
	TPT->notifyReceivedSize = inReceivedSize;
	if (!TPT->notifyReceivedSize)
		return;

	DWORD nReceivedSize;
	if (::ioctlsocket(TPT->sock, FIONREAD, &nReceivedSize) != 0 || !nReceivedSize)
		return;

	_TRNotifyReadyToReceive(inTpt, nReceivedSize);	
}

void URegularTransport::NotifyReadyToSend(TRegularTransport inTpt)
{
	Require(inTpt);
	
	TPT->checkReadyToSend = true;
	
	// handle situation where we became ready-to-send before NotifyReadyToSend() was called
	if (TPT->sendQueue == nil || TPT->sendQueue->next == nil)
	{
		TPT->checkReadyToSend = false;
		URegularTransport::PostMessage(inTpt, msg_ReadyToSend, nil, 0, priority_Normal);		
	}
}

bool URegularTransport::IsReadyToReceive(TRegularTransport inTpt, Uint32 inReceivedSize)
{
	Require(inTpt);
	
	TPT->notifyReceivedSize = inReceivedSize;
	if (!TPT->notifyReceivedSize)
		return true;

	Uint32 nReceivedSize = TPT->receivedData.nReceivedSize - TPT->receivedData.nReceivedOffset;
	if (nReceivedSize >= inReceivedSize)
		return true;

	if (::ioctlsocket(TPT->sock, FIONREAD, &nReceivedSize) != 0 || !nReceivedSize)
		return false;

	return _TRNotifyReadyToReceive(inTpt, nReceivedSize, false);
}

bool URegularTransport::IsReadyToSend(TRegularTransport inTpt)
{
	// we're ready-to-send if there are less than 2 outstanding buffers
	return inTpt && (TPT->sendQueue == nil || TPT->sendQueue->next == nil);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void URegularTransport::SendUnit(TRegularTransport inTpt, const void *inAddr, Uint32 inAddrSize, const void *inData, Uint32 inDataSize)
{
	Uint8 winAddr[256];
	Uint32 winAddrSize;
	Uint16 port;
	
	Require(inTpt);
	
	winAddrSize = _TRStdToWinAddr(inAddr, inAddrSize, winAddr, sizeof(winAddr));
	if (winAddrSize == 0) Fail(errorType_Transport, transError_BadAddress);

	// bind if necessary
	if (!TPT->isBound)
		_TRBindIPAddress(inTpt);

	if (winAddrSize == max_Uint32)	// if DNS name
	{
		_TRExtractNameAddr(*(SInternetNameAddress *)inAddr, (Int8 *)winAddr, &port);

		// if a lookup is already in progress, we'll do a syncronous lookup
		if (TPT->lookupBuf)
		{
			hostent *he = ::gethostbyname((char *)winAddr);
			if (he)
			{
				SOCKADDR_IN sockAddr;
				
				sockAddr.sin_family = AF_INET;
				sockAddr.sin_port = htons(port);
				sockAddr.sin_addr.s_addr = *(Uint32 *)(he->h_addr_list[0]);
				((Uint32 *)sockAddr.sin_zero)[0] = ((Uint32 *)sockAddr.sin_zero)[1] = 0;

				::sendto(TPT->sock, (char *)inData, inDataSize, 0, (sockaddr *)&sockAddr, sizeof(sockAddr));
			}
		}
		else	// can do an asyncronous lookup
		{
			try
			{
				TPT->sendUnitBuf = UMemory::New(inData, inDataSize);
				TPT->lookupBuf = UMemory::New(MAXGETHOSTSTRUCT);
				
				TPT->unitLookupTask = ::WSAAsyncGetHostByName(_TRAsyncWin, WM_SOCKET_LOOKUPANDSENDTO, (char *)winAddr, (char *)TPT->lookupBuf, MAXGETHOSTSTRUCT);
				if (TPT->unitLookupTask == 0) FailWinError(::WSAGetLastError());
			}
			catch(...)
			{
				UMemory::Dispose(TPT->lookupBuf);
				TPT->lookupBuf = nil;
				UMemory::Dispose(TPT->sendUnitBuf);
				TPT->sendUnitBuf = nil;
				throw;
			}
			
			TPT->lookupBufPort = port;
			TPT->sendUnitBufSize = inDataSize;
		}
	}
	else
	{
		::sendto(TPT->sock, (char *)inData, inDataSize, 0, (sockaddr *)winAddr, winAddrSize);
	}
}

/*
 * ReceiveUnit() receives a unit of data (datagram).  Up to <ioDataSize> bytes of data
 * are copied to <outData>, and on exit, <ioDataSize> is set to the actual number of bytes
 * received.  Up to <ioAddrSize> bytes of the senders address are copied to <outAddr>, and
 * on exit, <ioAddrSize> is set to the actual number of bytes copied to <outAddr>.
 * Returns true if a unit was received.  If there is more data than will fit in the supplied
 * buffer, the extra data is lost (another call to ReceiveUnit() will receive the next unit,
 * not the rest of the data).
 */
bool URegularTransport::ReceiveUnit(TRegularTransport inTpt, void *outAddr, Uint32& ioAddrSize, void *outData, Uint32& ioDataSize)
{
	Uint8 addr[256];
	int addrSize, result;
	
	if (inTpt)
	{
		addrSize = sizeof(addr);
		result = ::recvfrom(TPT->sock, (char *)outData, ioDataSize, 0, (sockaddr *)addr, &addrSize);
		
		if (result == WSAEMSGSIZE)	// if there was more data than would fit (but ioDataSize bytes was copied anyway)
		{
			ioAddrSize = _TRWinToStdAddr(addr, addrSize, outAddr, ioAddrSize);
			return true;
		}
		else if (result > 0)
		{
			ioDataSize = result;
			ioAddrSize = _TRWinToStdAddr(addr, addrSize, outAddr, ioAddrSize);
			return true;
		}
	}
	
	ioAddrSize = ioDataSize = 0;
	return false;
}


/* -------------------------------------------------------------------------- */
#pragma mark -

bool _IsValidURLChar(Uint16 inChar);
extern Int8 *gProgID_Protocol_Handler;

// returns true if the specified protocol is registered
bool URegularTransport::IsRegisteredProtocol(const Uint8 inProtocol[])
{
	return _IsRegisteredProtocol_InternetExplorer(inProtocol);
}

// returns true if the current application is registered to handle the specified protocol
bool URegularTransport::IsRegisteredForProtocol(const Uint8 inProtocol[])
{
	bool bIsRegProtocol = true;
	
	if (!_IsRegisteredForProtocol_InternetExplorer(inProtocol))
		bIsRegProtocol = false;
	
	if (gProgID_Protocol_Handler && !_IsRegisteredForProtocol_NetscapeNavigator(inProtocol, gProgID_Protocol_Handler))
		bIsRegProtocol = false;

	return bIsRegProtocol;
}

// returns true if the current application was successfully registered to handle the specified protocol
bool URegularTransport::RegisterProtocol(const Uint8 inProtocol[])
{
	bool bRegProtocol = true;
	
	if (!_IsRegisteredForProtocol_InternetExplorer(inProtocol) && 
	    !_RegisterProtocol_InternetExplorer(inProtocol))
		bRegProtocol = false;
	
	if (gProgID_Protocol_Handler && !_IsRegisteredForProtocol_NetscapeNavigator(inProtocol, gProgID_Protocol_Handler) && 
	    !_RegisterProtocol_NetscapeNavigator(inProtocol, gProgID_Protocol_Handler))
		bRegProtocol = false;

	return bRegProtocol;	
}

// returns false if unable to launch
bool URegularTransport::LaunchURL(const void *inText, Uint32 inTextSize, 
								  Uint32 *ioSelStart, Uint32 *ioSelEnd)
{	
	Uint8 *start = (Uint8 *)inText;
	Uint8 *end = start + inTextSize;
	if (ioSelEnd != 0)
		end = start + *ioSelEnd;
	if (ioSelStart != 0)
		start += *ioSelStart;
	if (start > end)
		start = end;

//	Uint32 st = ioSelStart ? *ioSelStart : 0;
//	if (st > inTextSize)
//		st = inTextSize;
//	Uint8 *start = (Uint8 *)inText + st;
//	Uint8 *end = start;
//	while (start >= inText)
//	{
//		if (!_IsValidURLChar(*start))
//			break;
//		start--;
//	}
//	start++;
//	Uint8 *absEnd = (Uint8 *)inText + inTextSize;
//	while (end != absEnd)
//	{
//		if (!_IsValidURLChar(*end))
//			break;
//		end++;
//	}
    // make local copy
	Uint8 url[512];
	UInt32 z = UMemory::Copy(url, start, min((Uint32)(end - start), (Uint32)(sizeof(url) - 1)));
	url[z] = 0; // ASCIIZ at end of string
//UDebug::LogToDebugFile("LaunchURL='%s'",url);
	
	// file:///x|/ should be converted back to windows format to keep consistent behaviour across
	// windows versions
	bool localURL = false;
	if (UMemory::Equal(url, "file:///", 8) && (url[9] == '|'))
	{
		url[9] = ':';
		for(UInt32 k=10; k<z; ++k)
			if (url[k] == '/')
				url[k] = '\\';
		localURL = true;
	}
	if ((Uint8 *)inText <= start && start < end && end <= inTextSize+(Uint8 *)inText)
	{
		Uint32 returnVal = (Uint32)ShellExecute(nil, 
										NULL/*"open"*/, 
										(const char*) (localURL ? url+8 : url+0), 
										nil, nil, SW_SHOWNORMAL);
//UDebug::LogToDebugFile("ShellExecute='%s'",(localURL ? url+8 : url+0));
		if (returnVal >= 0 && returnVal <= 32)	// an error
			return false;
		// these are updated here so we don't modify if false
		if (ioSelStart)
			*ioSelStart = start - (Uint8 *)inText;
		if (ioSelEnd)
			*ioSelEnd = end - (Uint8 *)inText;
		return true;
	}
	return false;
}
 

/* -------------------------------------------------------------------------- */
#pragma mark -

// returns max_Uint32 if is kInternetNameAddressType (which win can't handle by default)
static Uint32 _TRStdToWinAddr(const void *inAddr, Uint32 inSize, void *outData, Uint32 inMaxSize)
{
	if (inSize >= 6 && inMaxSize >= sizeof(SOCKADDR_IN))
	{
		Uint16 type = *(Uint16 *)inAddr;
		SOCKADDR_IN *oa = (SOCKADDR_IN *)outData;
		Uint32 n;
		Uint8 *p;
		Int8 str[256];
		
		if (type == kInternetAddressType)
		{
			if (inSize >= sizeof(SInternetAddress))
			{
				SInternetAddress *ipaddr = (SInternetAddress *)inAddr;
				
				oa->sin_family = AF_INET;
				oa->sin_port = htons(ipaddr->port);
				oa->sin_addr.S_un.S_addr = *(Uint32 *)ipaddr->host;
				((Uint32 *)oa->sin_zero)[0] = ((Uint32 *)oa->sin_zero)[1] = 0;
				
				return sizeof(SOCKADDR_IN);
			}
		}
		else if (type == kInternetNameAddressType)
		{
			SInternetNameAddress *naddr = (SInternetNameAddress *)inAddr;
			n = naddr->name[0];
			
			if (n && (inSize >= n + 5))
			{
				::CopyMemory(str, naddr->name+1, n);
				str[n] = 0;
				
				if (naddr->port == 0)	// if use port in name
				{
					p = UMemory::SearchByte(':', str, n);
					if (p == nil) return 0;	// no port specified
					oa->sin_port = htons(strtoul((char *)p+1, NULL, 10));
				}
				else
				{
					// remove any port from the name and use the specified port instead
					p = UMemory::SearchByte(':', str, n);
					if (p) *p = 0;
					oa->sin_port = htons(naddr->port);
				}
				
				oa->sin_addr.s_addr = inet_addr(str);

				if (oa->sin_addr.s_addr == INADDR_NONE)
					return max_Uint32;	// DNS
				
				oa->sin_family = AF_INET;
				((Uint32 *)oa->sin_zero)[0] = ((Uint32 *)oa->sin_zero)[1] = 0;

				return sizeof(SOCKADDR_IN);
			}
		}
	}
	
	return 0;
}

static Uint32 _TRWinToStdAddr(const void *inAddr, Uint32 inSize, void *outData, Uint32 inMaxSize)
{
	if (inSize >= sizeof(SOCKADDR_IN) && inMaxSize >= sizeof(SInternetAddress))
	{
		SOCKADDR_IN *ia = (SOCKADDR_IN *)inAddr;
		
		if (ia->sin_family == AF_INET)
		{
			SInternetAddress *oa = (SInternetAddress *)outData;
			
			oa->type = kInternetAddressType;
			oa->port = ntohs(ia->sin_port);
			((Uint32 *)oa->host)[0] = ia->sin_addr.S_un.S_addr;
			((Uint32 *)oa->host)[1] = 0;

			return sizeof(SInternetAddress);
		}
	}
	
	return 0;
}

static SRegularTransport *_TRLookupSock(SOCKET inSock)
{
	SRegularTransport *tm = _TRFirsTRegularTransport;
	
	while (tm)
	{
		if (tm->sock == inSock)
			return tm;

		tm = tm->next;
	}
	
	return nil;
}

static SRegularTransport *_TRLookupSockByLTHdl(HANDLE inHdl)
{
	SRegularTransport *tm = _TRFirsTRegularTransport;
	
	while (tm)
	{
		if (tm->lookupTask == inHdl)
			return tm;

		tm = tm->next;
	}
	
	return nil;
}

static SRegularTransport *_TRLookupSockByULTHdl(HANDLE inHdl)
{
	SRegularTransport *tm = _TRFirsTRegularTransport;
	
	while (tm)
	{
		if (tm->unitLookupTask == inHdl)
			return tm;

		tm = tm->next;
	}
	
	return nil;
}

static SRegularTransport *_TRLookupSockByFWLTHdl(HANDLE inHdl)
{
	SRegularTransport *tm = _TRFirsTRegularTransport;
	
	while (tm)
	{
		if (tm->firewallInfo)
			if (tm->firewallInfo->lookupTask == inHdl)
				return tm;

		tm = tm->next;
	}
	
	return nil;
}

static bool _TRNotifyReadyToReceive(TRegularTransport inTpt, Uint32 inReceivedSize, bool inPostMessage)
{
	if (TPT->receivedData.nReceivedSize - TPT->receivedData.nReceivedOffset >= TPT->notifyReceivedSize)
	{
		TPT->notifyReceivedSize = 0;
		if (inPostMessage)
			URegularTransport::PostMessage(inTpt, msg_DataArrived, nil, 0, priority_Normal);		
		
		return true;
	}
		
	Uint32 nReceivedSize = TPT->notifyReceivedSize - (TPT->receivedData.nReceivedSize - TPT->receivedData.nReceivedOffset);
	if (nReceivedSize > inReceivedSize)
		nReceivedSize = inReceivedSize;
		
	if (TPT->receivedData.nDataSize < TPT->receivedData.nReceivedSize + nReceivedSize)
	{
		TPT->receivedData.nDataSize = TPT->receivedData.nReceivedSize + nReceivedSize;
		
		if (TPT->receivedData.pData)
			TPT->receivedData.pData = UMemory::Reallocate((TPtr)TPT->receivedData.pData, TPT->receivedData.nDataSize);
		else
			TPT->receivedData.pData = UMemory::New(TPT->receivedData.nDataSize);
	}
					
	TPT->receivedData.nReceivedSize += ::recv(TPT->sock, (char *)((Uint8 *)TPT->receivedData.pData + TPT->receivedData.nReceivedSize), nReceivedSize, 0);

	if (TPT->receivedData.nReceivedSize - TPT->receivedData.nReceivedOffset >= TPT->notifyReceivedSize)
	{
		TPT->notifyReceivedSize = 0;
		if (inPostMessage)
			URegularTransport::PostMessage(inTpt, msg_DataArrived, nil, 0, priority_Normal);
			
		return true;
	}
	
	return false;
}

// for use with WSAAsyncSelect
// fortunately, this func runs in the main app thread, NOT in it's own thread (even tho windoze creates a thread to manage the socket)
// would be a major pain in the butt if it ran in diff thread (would have to put critical sections all over the place!!)
static LRESULT CALLBACK _TRSockProc(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam)
{
	Uint8 buf[16];

	if (inMsg != WM_SOCKET_NOTIFY)
		return DefWindowProc(inWnd, inMsg, inWParam, inLParam);
		
	try
	{
		// note that can still receive messages after the socket is closed (if in queue before closed).  This is okay because _TRLookupSock() won't find the TRegularTransport if it has been killed
		SRegularTransport *tpt = _TRLookupSock((SOCKET)inWParam);

		if (tpt)
		{
			switch (WSAGETSELECTEVENT(inLParam))
			{
				case FD_READ:
					if (tpt->msgProc)
					{
						if (!tpt->isStream)
						{
							URegularTransport::PostMessage((TRegularTransport)tpt, msg_DataArrived, nil, 0, priority_Normal);
						}
						else if (tpt->isConnected) // TG do I need to check if connected as well?  eg - does this fix the d/l bug?
						{
							DWORD nReceivedSize;
							if (::ioctlsocket(tpt->sock, FIONREAD, &nReceivedSize) != 0)
								return 0;
							
							if (!nReceivedSize) 
							{
								::recv(tpt->sock, nil, 0, 0);	// this fixes the stall bug (re-enable FD_READ message)
								return 0;
							}
												
							if (tpt->notifyReceivedSize)
								_TRNotifyReadyToReceive((TRegularTransport)tpt, nReceivedSize);
							else							
								URegularTransport::PostMessage((TRegularTransport)tpt, msg_DataArrived, nil, 0, priority_Normal);
						}
						else if (tpt->firewallInfo)
						{
							// gets handled in the above case - tpt->isConnected
							/*
							if (tpt->firewallInfo->status == kFWStat_Ready && tpt->isConnected) // TG do I need to check if connected as well?  eg - does this fix the d/l bug?
							{
								URegularTransport::PostMessage(tpt, msg_DataArrived, nil, 0, priority_Normal);
							}
							else */
							if (tpt->firewallInfo->status == kFWStat_Authenticating)
							{
								if (::recv(tpt->sock, (char *)buf, 8, 0) == 8)
								{
									if ((buf[0] == 4 || buf[0] == 0) && buf[1] == 90)	// if connect successful
									{
										tpt->connectResult = 0;			// no error
										tpt->isConnected = true;
										URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionEstablished, nil, 0, priority_Normal);
										tpt->firewallInfo->status = kFWStat_Ready;
									}
									else
									{
										tpt->connectResult = WSAECONNREFUSED;
										URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionRefused, nil, 0, priority_Normal);
									}
								}
							}
						}
					}
					break;
					 
				case FD_WRITE:
					_TRSendQueued((TRegularTransport)tpt);
					
					if (tpt->sendQueue == nil && tpt->disconWhenNoOutData)
					{
						::shutdown(tpt->sock, 2);
					}
					else if (tpt->msgProc && tpt->checkReadyToSend && (tpt->sendQueue == nil || tpt->sendQueue->next == nil))
					{
						tpt->checkReadyToSend = false;	// NotifyReadyToSend() is a one-off notification (to avoid a flood of messages)
						URegularTransport::PostMessage((TRegularTransport)tpt, msg_ReadyToSend, nil, 0, priority_Normal);
					}
					break;
					
				case FD_CONNECT:
					int err = WSAGETSELECTERROR(inLParam);

					if (err == 0)	// if no error
					{

						// to do..check if firewall and if so, send packet of firewall info.
						if (tpt->firewallInfo)
						{
							
							if (tpt->firewallInfo->status == kFWStat_Connecting)
							{
								tpt->firewallInfo->status = kFWStat_Authenticating;
								
								// send the packet
								// make packet of info to tell firewall where to connect to on our behalf
								buf[0] = 4;
								buf[1] = 1;
								*(Uint16 *)(buf+2) = htons(tpt->firewallInfo->port);
								*(Uint32 *)(buf+4) = *((Uint32 *)tpt->firewallInfo->destAddr);
								buf[8] = 'x';
								buf[9] = 0;
								
								Uint8 *p = buf;
								Uint8 *q = p + 10;
								
								// I doubt we'll have a prob sending 10 bytes at once ever, but let's try anyways...
								while (p != q)
								{
									int sent = ::send(tpt->sock, (char *)(p + 10 - (q - p)), q - p, 0);
									
									if (sent <= 0)
									{
										tpt->connectResult = WSAECONNREFUSED;
										sent = 0;		// if error, no data got sent
										URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionRefused, nil, 0, priority_Normal);
									}
									
									p += sent;
								
								}
							}
							
						}
						else
						{
							tpt->connectResult = err;
							// note msg_ConnectionEstablished MUST be posted before msg_DataArrived (mustn't have DataArrived being processed before ConnectionEstablished)
							tpt->isConnected = true;
							URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionEstablished, nil, 0, priority_Normal);
						}
					}
					else
					{
						tpt->connectResult = err;
						URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionRefused, nil, 0, priority_Normal);
					}
					break;
					
				case FD_ACCEPT:
					URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionRequest, nil, 0, priority_Normal);
					break;
					
				case FD_CLOSE:
					tpt->isConnected = false;
					URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionClosed, nil, 0, priority_Normal);
					break;
			}
		}
	}
	catch(...)
	{
		// DON'T throw exceptions!
	}
		
	return 0;
}

// for use with WSAAsyncGetHostByName and similar
static LRESULT CALLBACK _TRAsyncProc(HWND inWnd, UINT inMsg, WPARAM inWParam, LPARAM inLParam)
{
	SRegularTransport *tpt;
	SOCKADDR_IN sockAddr;
	
	if (inMsg == WM_SOCKET_LOOKUPANDCONNECT)	// from StartConnect()
	{
		tpt = _TRLookupSockByLTHdl((HANDLE)inWParam);
		
		if (tpt)	// note that the message can be sent even after the async task is cancelled.  This is fine because _TRLookupSockByLTHdl() won't find the TRegularTransport and will return nil
		{
			DWORD err = WSAGETASYNCERROR(inLParam);
			
			if (err == 0)	// if no error
			{
				if (tpt->connectResult != kNotComplete)	// if not already connecting
				{
					// bind if necessary
					if (!tpt->isBound)
						_TRBindIPAddress((TRegularTransport)tpt);

					sockAddr.sin_family = AF_INET;
					sockAddr.sin_port = htons(tpt->lookupBufPort);
					sockAddr.sin_addr.s_addr = ((LPIN_ADDR)((LPHOSTENT)tpt->lookupBuf)->h_addr)->s_addr;
					((Uint32 *)sockAddr.sin_zero)[0] = ((Uint32 *)sockAddr.sin_zero)[1] = 0;

					tpt->connectResult = kNotComplete;
					err = ::connect(tpt->sock, (sockaddr *)&sockAddr, sizeof(sockAddr));
					if (err == SOCKET_ERROR)
					{
						err = ::WSAGetLastError();
						if (err != WSAEWOULDBLOCK)	// WSAEWOULDBLOCK is normal
						{
							tpt->connectResult = err;
							
							try {
								URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionRefused, nil, 0, priority_Normal);
							} catch(...) {}		// DON'T throw exceptions!
						}
					}
				}
			}
			else
			{
				tpt->connectResult = err;
				
				try {
					URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionRefused, nil, 0, priority_Normal);
				} catch(...) {}		// DON'T throw exceptions!
			}
			
			UMemory::Dispose((TPtr)tpt->lookupBuf);
			tpt->lookupBuf = nil;
			tpt->lookupTask = 0;
		}
		
		return 0;
	}
	else if (inMsg == WM_SOCKET_FWLOOKUPANDCONNECT)
	{

		tpt = _TRLookupSockByLTHdl((HANDLE)inWParam);
		if (tpt)
		{
			DWORD err = WSAGETASYNCERROR(inLParam);
			
			if (err == 0)	// if no error
			{

				if (tpt->connectResult != kNotComplete)	// if not already connecting
				{
					if (tpt->lookupBuf && tpt->firewallInfo && tpt->firewallInfo->status & kFWStat_FWDNS)
					{
						if (tpt->connectResult != kNotComplete)	// if not already connecting
						{
							
							*((Uint32 *)tpt->firewallInfo->fwAddr) = ((LPIN_ADDR)((LPHOSTENT)tpt->lookupBuf)->h_addr)->s_addr;
							
							tpt->firewallInfo->status = tpt->firewallInfo->status & (~kFWStat_FWDNS);
							
							_TRTryConnectViaFirewall(tpt);
						}

					}
				}
			}
			else
			{
				tpt->connectResult = err;
				
				try {
					URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionRefused, nil, 0, priority_Normal);
				} catch(...) {}		// DON'T throw exceptions!
			}
			
			UMemory::Dispose((TPtr)tpt->lookupBuf);
			tpt->lookupBuf = nil;
			tpt->lookupTask = 0;
		}
		
		return 0;
	}
	else if (inMsg == WM_SOCKET_FWLOOKUPDEST)
	{
		tpt = _TRLookupSockByFWLTHdl((HANDLE)inWParam);
		if (tpt)
		{
			DWORD err = WSAGETASYNCERROR(inLParam);
			
			if (err == 0)	// if no error
			{
				if (tpt->firewallInfo && tpt->firewallInfo->lookupBuf && tpt->firewallInfo->status & kFWStat_DestDNS)
				{
					
					if (tpt->connectResult != kNotComplete)	// if not already connecting
					{
						*((Uint32 *)tpt->firewallInfo->destAddr) = ((LPIN_ADDR)((LPHOSTENT)tpt->firewallInfo->lookupBuf)->h_addr)->s_addr;
						
						tpt->firewallInfo->status = tpt->firewallInfo->status & (~kFWStat_DestDNS);
						
						_TRTryConnectViaFirewall(tpt);
					}

				}
			}
			else
			{
				tpt->connectResult = err;
				
				try {
					URegularTransport::PostMessage((TRegularTransport)tpt, msg_ConnectionRefused, nil, 0, priority_Normal);
				} catch(...) {}		// DON'T throw exceptions!
			}
			
			UMemory::Dispose((TPtr)tpt->firewallInfo->lookupBuf);
			tpt->firewallInfo->lookupBuf = nil;
			tpt->firewallInfo->lookupTask = 0;
		}
	
		return 0;
	}
	else if (inMsg == WM_SOCKET_LOOKUPANDSENDTO)	// from SendUnit()
	{
		tpt = _TRLookupSockByULTHdl((HANDLE)inWParam);
		if (tpt)
		{
			if (WSAGETASYNCERROR(inLParam) == 0)		// if no error
			{
				sockAddr.sin_family = AF_INET;
				sockAddr.sin_port = htons(tpt->lookupBufPort);
				sockAddr.sin_addr.s_addr = ((LPIN_ADDR)((LPHOSTENT)tpt->lookupBuf)->h_addr)->s_addr;
				((Uint32 *)sockAddr.sin_zero)[0] = ((Uint32 *)sockAddr.sin_zero)[1] = 0;
				
				::sendto(tpt->sock, (char *)tpt->sendUnitBuf, tpt->sendUnitBufSize, 0, (sockaddr *)&sockAddr, sizeof(sockAddr));
			}
			
			UMemory::Dispose((TPtr)tpt->sendUnitBuf);
			tpt->sendUnitBuf = nil;
			UMemory::Dispose((TPtr)tpt->lookupBuf);
			tpt->lookupBuf = nil;
			tpt->unitLookupTask = 0;
		}
		return 0;
	}
	
	return DefWindowProc(inWnd, inMsg, inWParam, inLParam);
}

// outName is a C-string and must point to 256 bytes
static void _TRExtractNameAddr(const SInternetNameAddress& inAddr, Int8 *outName, Uint16 *outPort)
{
	Uint32 n;
	Uint8 *p;

	n = inAddr.name[0];
	::CopyMemory(outName, inAddr.name+1, n);
	outName[n] = 0;
	
	if (inAddr.port == 0)	// if use port in name
	{
		p = UMemory::SearchByte(':', outName, n);
		if (p == nil) Fail(errorType_Transport, transError_BadAddress);	// no port specified
		*outPort = strtoul((char *)p+1, NULL, 10);
	}
	else
	{
		// remove any port from the name and use the specified port instead
		p = UMemory::SearchByte(':', outName, n);
		if (p) *p = 0;
		*outPort = inAddr.port;
	}
}

// returns bytes sent (or 0)
static Uint32 _TRCappedSend(SOCKET inSock, const void *inBuf, Uint32 inSize)
{
	// 32K seems to work well for getting around the WSAEFAULT bug in windoze
	enum {
		cap = 32 * 1024
	};
	
	Uint32 sent = 0;
	int result;
	
	while (inSize)
	{
		Uint32 s = (inSize > cap) ? cap : inSize;
		
		result = ::send(inSock, ((char *)inBuf) + sent, s, 0);
		
		if (result <= 0)
		{
#if DEBUG
			if (result < 0)
			{
				result = ::WSAGetLastError();
				if (result != WSAEWOULDBLOCK && result != WSAECONNRESET)	// these are normal so don't display alert
				{
					// TG - don't show this since it happens on cancel dl (for the server)
					//	DebugBreak("Winsock error attempting to send data:  %d\rSize of data:  %lu (capped)", result, s);
				}
			}
#endif
			break;		
		}
		
		if (result < s)	// if only part of the data was sent
		{
			sent += result;
			break;
		}
		else
		{
			sent += s;
			inSize -= s;
		}
	}
	
	return sent;
}

// there's no need to turn this on, because we're recovering using the capped send method if the WSAEFAULT error occurs
#define USE_CAPPED_SEND		0

static void _TRSendQueued(TRegularTransport inTpt)
{
	SSendBuf *buf = TPT->sendQueue;
	SOCKET sock = TPT->sock;
	SSendBuf *nextbuf;
	Uint32 s;
	
	while (buf)
	{
		ASSERT(buf->offset < buf->dataSize);
		ASSERT((sizeof(SSendBuf) + buf->dataSize) <= ::GlobalSize((HGLOBAL)buf));
		ASSERT(!::IsBadReadPtr(buf->data, buf->dataSize));
		ASSERT(!::IsBadWritePtr(buf->data, buf->dataSize));
		
#if USE_CAPPED_SEND

		s = buf->dataSize - buf->offset;
		Uint32 sent = _TRCappedSend(sock, ((char *)buf->data) + buf->offset, s);
		
		if (sent < s)	// if only part of the data was sent
		{
			buf->offset += sent;
			break;
		}

#else

		s = buf->dataSize - buf->offset;
		int result = ::send(sock, ((char *)buf->data) + buf->offset, s, 0);
		
		// if error or no data sent, just fall out of the loop
		if (result <= 0)
		{
			if (result == 0) break;		// if just simply flow control, that's ok
			result = ::WSAGetLastError();
			
			if (result == WSAEFAULT)
			{
			//#if DEBUG
			//	::MessageBeep(MB_OK);
			//#endif
				// work around a really stupid bug in windoze 95 that happens sometimes
				result = _TRCappedSend(sock, ((char *)buf->data) + buf->offset, s);
			}
			else
			{
			#if DEBUG
				if (result != WSAEWOULDBLOCK && result != WSAECONNRESET)	// these are normal so don't display alert
					DebugBreak("Winsock error attempting to send data:  %d\rSize of data:  %lu", result, s);
			#endif
				break;
			}
		}
		
		if (result < s)	// if only part of the data was sent
		{
			buf->offset += result;
			break;
		}

#endif

		// all of this buffer was sent, so we can dispose and unlink it
		nextbuf = buf->next;
		UMemory::Dispose((TPtr)buf);
		TPT->sendQueue = nextbuf;
		
		// move to next buffer
		buf = nextbuf;
	}
}

struct SSendData
{
	void *pSendData;
	Uint32 nDataSize;
	bool bDataBuffer;
};

void _TRGetUnsentData(TRegularTransport inTpt, CPtrList<SSendData> *inSendDataList)
{
	SSendBuf *pNextBuf;
	SSendBuf *pSendBuf = TPT->sendQueue;
	
	while (pSendBuf)
	{
		if (pSendBuf->offset)
		{
			pNextBuf = pSendBuf->next;
			UMemory::Dispose((TPtr)pSendBuf);
			pSendBuf = pNextBuf;
			
			continue;
		}
		
		SSendData *pSendData = (SSendData *)UMemory::New(sizeof(SSendData));
		
		pSendData->pSendData = pSendBuf->data;
		pSendData->nDataSize = pSendBuf->dataSize;
		pSendData->bDataBuffer = false;

		inSendDataList->AddItem(pSendData);
		
		pNextBuf = pSendBuf->next;
		pSendBuf->next = nil;
		pSendBuf = pNextBuf;
	}

	TPT->sendQueue = nil;
}

static void _TRBindIPAddress(TRegularTransport inTpt, Uint16 inPort)
{
	Require(inTpt);

	SOCKADDR_IN sin;
	ClearStruct(sin);
	
	sin.sin_family = AF_INET;
	sin.sin_port = htons(inPort);
	sin.sin_addr.s_addr = TPT->localIPAddress.un_IP.stDW_IP.nDW_IP;
	
	if (::bind(TPT->sock, (sockaddr *)&sin, sizeof(sin)) != 0)
		FailWinError(::WSAGetLastError());
		
	TPT->isBound = true;
}

void _TRTryConnectViaFirewall(SRegularTransport *inTpt)
{
	SOCKADDR_IN sockAddr;

	if (inTpt->firewallInfo && inTpt->firewallInfo->status == kFWStat_Connecting)
	{
		// bind if necessary
		if (!inTpt->isBound)
			_TRBindIPAddress((TRegularTransport)inTpt);
		
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = htons(inTpt->lookupBufPort);
		sockAddr.sin_addr.s_addr = *((Uint32 *)inTpt->firewallInfo->fwAddr);
		((Uint32 *)sockAddr.sin_zero)[0] = ((Uint32 *)sockAddr.sin_zero)[1] = 0;

		inTpt->connectResult = kNotComplete;
		inTpt->firewallInfo->status = kFWStat_Connecting;

		int err = ::connect(inTpt->sock, (sockaddr *)&sockAddr, sizeof(sockAddr));
		if (err == SOCKET_ERROR)
		{
			err = ::WSAGetLastError();
			if (err != WSAEWOULDBLOCK)	// WSAEWOULDBLOCK is normal
			{
				inTpt->connectResult = err;

				try {
					URegularTransport::PostMessage((TRegularTransport)inTpt, msg_ConnectionRefused, nil, 0, priority_Normal);
				} catch(...) {}		// DON'T throw exceptions!
			}
		}
	}
}

bool _IsValidURLChar(Uint16 inChar)
{
	if (inChar < '!' || inChar > '~')
		return false;
	
	if (UText::IsAlphaNumeric(inChar))
		return true;
		
	switch (inChar)
	{
		case '#':
		case '$':
		case '-':
		case '_':
		case '.':
		case '+':
		case '!':
		case '*':
		case '\'':
		case '(':
		case ')':
		case ',':
		case ';':
		case '/':
		case '?':
		case ':':
		case '@':
		case '=':
		case '&':
		case '%':
		case '~':

		// these might not be valid, but for now, return true
		case '{':
		case '}':
		case '|':
		case '\\':
		case '^':
		case '[':
		case ']':
		case '`':
			return true;
	}
	
	return false;
}

#pragma mark -

// open the HKEY_CLASSES_ROOT\\"protocol"\\shell\\open\\command key
// read the defaul value of this key
// returns true if the specified protocol is registered
static bool _IsRegisteredProtocol_InternetExplorer(const Uint8 *inProtocol)
{	    
    Int8 buf[2048];
    buf[UText::Format(buf, sizeof(buf), "%#s\\shell\\open\\command", inProtocol)] = 0;
    
	// open the HKEY_CLASSES_ROOT\\"protocol"\\shell\\open\\command key
	HKEY hOpenKey;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, buf, NULL, KEY_ALL_ACCESS, &hOpenKey) != ERROR_SUCCESS)
		return false;

    // copy the default key value in the buffer
    DWORD dwType;
    Uint32 nSize = sizeof(buf);
	if (RegQueryValueEx(hOpenKey, NULL, NULL, &dwType, (LPBYTE)buf, &nSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hOpenKey);
		return false;
	}

	// close the key
	RegCloseKey(hOpenKey);

	if (dwType != REG_SZ)
		return false;

	return true;
}

// open the HKEY_CLASSES_ROOT\\"protocol"\\shell\\open\\command key
// compare the default value of this key with the current application path
// returns true if the current application is registered to handle the specified protocol
static bool _IsRegisteredForProtocol_InternetExplorer(const Uint8 *inProtocol)
{
	// copy the application path in the buffer and append " %1"
	Int8 bufPath[2048];
	Uint32 nPathSize = ::GetModuleFileName(NULL, bufPath, sizeof(bufPath));
	nPathSize += UMemory::Copy(bufPath + nPathSize, " \%1", 4);
	    
    Int8 buf[2048];
    buf[UText::Format(buf, sizeof(buf), "%#s\\shell\\open\\command", inProtocol)] = 0;

	// open the HKEY_CLASSES_ROOT\\"protocol"\\shell\\open\\command key
	HKEY hOpenKey;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, buf, NULL, KEY_ALL_ACCESS, &hOpenKey) != ERROR_SUCCESS)
		return false;

    // copy the default key value in the buffer
    DWORD dwType;
    Uint32 nSize = sizeof(buf);
	if (RegQueryValueEx(hOpenKey, NULL, NULL, &dwType, (LPBYTE)buf, &nSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hOpenKey);
		return false;
	}

	// close the key
	RegCloseKey(hOpenKey);

	// compare the key value with the application path
	if (dwType != REG_SZ || nPathSize != nSize || !UMemory::Equal(bufPath, buf, nSize))
		return false;

	return true;
}

// create the HKEY_CLASSES_ROOT\\"protocol"\\shell\\open\\command key in Registry
// set the default value of this key = the current application path
// returns true if the current application was successfully registered to handle the specified protocol
static bool _RegisterProtocol_InternetExplorer(const Uint8 *inProtocol)
{
	HKEY hNewKey;
	DWORD dwResult; 
	Int8 buf[2048];
        
	// create or open the protocol key in HKEY_CLASSES_ROOT
    buf[UMemory::Copy(buf, inProtocol + 1, inProtocol[0])] = 0;
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, buf, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hNewKey, &dwResult) != ERROR_SUCCESS)
		return false;

//	dwResult == REG_CREATED_NEW_KEY		- the key was created
//	dwResult == REG_OPENED_EXISTING_KEY	- the key exists and was simply opened

	// set the default value of the protocol key
    buf[UText::Format(buf, sizeof(buf), "URL:%#s Protocol", inProtocol)] = NULL;
	if (RegSetValueEx(hNewKey, NULL, 0, REG_SZ, (BYTE*)buf, strlen(buf)) != ERROR_SUCCESS)
	{
		RegCloseKey(hNewKey);
		return false;
	}

    // set a new value for the protocol key
    buf[0] = NULL;
	if (RegSetValueEx(hNewKey, "URL Protocol", 0, REG_SZ, (BYTE*)buf, strlen(buf)) != ERROR_SUCCESS)
	{
		RegCloseKey(hNewKey);
		return false;
	}

	// create new keys inside the protocol key
	strcpy(buf, "shell\\open\\command");
	if (RegCreateKeyEx(hNewKey, buf, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hNewKey, &dwResult) != ERROR_SUCCESS)
	{
		RegCloseKey(hNewKey);
		return false;
	}

	// copy the application path in the buffer and append " %1"
	Uint32 nPathSize = ::GetModuleFileName(NULL, buf, sizeof(buf));
	UMemory::Copy(buf + nPathSize, " \%1", 4);
	
	// set the default value of the HKEY_CLASSES_ROOT\\"protocol"\\shell\\open\\command key
	if (RegSetValueEx(hNewKey, NULL, 0, REG_SZ, (BYTE*)buf, strlen(buf)) != ERROR_SUCCESS)
	{
		RegCloseKey(hNewKey);
		return false;
	}

	// close the key
	RegCloseKey(hNewKey);

	return true;
}

// open the "HKEY_CURRENT_USER\\Software\\Netscape\\Netscape Navigator\\Automation Protocols" key
// compare the protocol key value with the protocol handler name
// returns true if the current application is registered to handle the specified protocol
static bool _IsRegisteredForProtocol_NetscapeNavigator(const Uint8 *inProtocol, const Int8 *inProgID)
{
	// if this key doesn't exist in the registry means that Netscape Navigator is not installed
	HKEY hOpenKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Netscape\\Netscape Navigator\\Automation Protocols", NULL, KEY_ALL_ACCESS, &hOpenKey) != ERROR_SUCCESS)
		return true;	// there is no need to register the protocol with Netscape

    Uint8 bufProtocol[256];
    bufProtocol[UMemory::Copy(bufProtocol, inProtocol + 1, inProtocol[0])] = 0;
    
    DWORD dwType;
    Int8 bufProgID[256];
    
    // read the protocol key value
    Uint32 nSize = sizeof(bufProgID);
	if (RegQueryValueEx(hOpenKey, (LPTSTR)bufProtocol, NULL, &dwType, (LPBYTE)bufProgID, &nSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hOpenKey);
		return false;
	}

	// close the key
	RegCloseKey(hOpenKey);

	// compare the key value with the protocol handler name
	nSize--;
	if (dwType != REG_SZ || nSize != strlen(inProgID) || !UMemory::Equal(inProgID, bufProgID, nSize))
		return false;

	return true;
}

// read the Windows version and return false if Win95
// create an instance of the "Netscape.Registry.1" object (which is an OLE Automation object)
// invoke "RegisterProtocol" member and pass the protocol name and the protocol handler
// the protocol handler must be another OLE Automation object
// returns true if the current application was successfully registered to handle the specified protocol
static bool _RegisterProtocol_NetscapeNavigator(const Uint8 *inProtocol, const Int8 *inProgID)
{
	// in some Win95 versions the application will crash if we use OLE Automation
	OSVERSIONINFO stVersion = {sizeof(OSVERSIONINFO),0,0,0,0,""};
	if (::GetVersionEx(&stVersion) && stVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && stVersion.dwMajorVersion == 4 && stVersion.dwMinorVersion == 0)
		return false;

  	// create an instance of the "Netscape.Registry.1" object
  	LPDISPATCH pDispatch;
 	if (!UOleAutomation::CreateObject(OLESTR("Netscape.Registry.1"), &pDispatch))
  		return false;
 	 	
	// convert the protocol name and the handler name to Unicode
	Int8 bufProtocol[128];
    bufProtocol[UMemory::Copy(bufProtocol, inProtocol + 1, inProtocol[0])] = 0;
	
	WCHAR wideProtocol[128];
	::MultiByteToWideChar(CP_ACP, 0, bufProtocol, -1, wideProtocol, sizeof(wideProtocol)/sizeof(WCHAR));

	WCHAR wideProgID[128];
	::MultiByteToWideChar(CP_ACP, 0, inProgID, -1, wideProgID, sizeof(wideProgID)/sizeof(WCHAR));

	// invoke "RegisterProtocol" member
	VARIANT vRet;
  	if (!UOleAutomation::Invoke(pDispatch, DISPATCH_METHOD, &vRet, OLESTR("RegisterProtocol"), "ss", wideProtocol, wideProgID))
    {
        pDispatch->Release();
        return false;
    }

	pDispatch->Release();
 	if (!vRet.boolVal)
 		return false;
 	
 	return true;
}

#endif /* WIN32 */
