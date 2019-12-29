/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"

/*
 * Structures
 */

#pragma options align=packed

struct SInternetAddress {
	Uint16 type;		// always kInternetAddressType
	Uint16 port;		// port number
	Uint8 host[8];		// host address
};

struct SInternetNameAddress {
	Uint16 type;		// always kInternetNameAddressType
	Uint16 port;		// port number (0 = use port in name)
	Uint8 name[256];	// DNS name (p-string)
};

struct SIPAddress {
	union {
		struct {
			Uint8 nB_IP1;
			Uint8 nB_IP2;
			Uint8 nB_IP3;
			Uint8 nB_IP4;	
		} stB_IP;
	 
	 	struct {
	 		Uint16 nW_IP1;
	 		Uint16 nW_IP2;
	 	} stW_IP;
	 	
	 	struct {
		 	Uint32 nDW_IP;
		} stDW_IP;
	} un_IP;
	
	void SetNull()		{	un_IP.stDW_IP.nDW_IP = 0;									}
	bool IsNull()		{	return (un_IP.stDW_IP.nDW_IP == 0);							}
};

struct SEthernetAddress {
	Uint8 eaddr[6];
	
	void SetNull()		{	eaddr[0] = 0; eaddr[1] = 0; eaddr[2] = 0; 
							eaddr[3] = 0; eaddr[4] = 0; eaddr[5] = 0;					}
						
	bool IsNull()		{	return (eaddr[0] == 0 && eaddr[1] == 0 && eaddr[2] == 0 && 
									eaddr[3] == 0 && eaddr[4] == 0 && eaddr[5] == 0);	}
};

#pragma options align=reset

/*
 * Types
 */

typedef class TRegularTransportObj *TRegularTransport;
typedef void (*TTransportMonitorProc)(TRegularTransport inTpt, const void *inData, Uint32 inDataSize, bool inIsSend);


/*
 * URegularTransport
 */

class URegularTransport
{
	public:	
		// initialize
		static bool IsAvailable();
		static bool HasTCP();
		static void Init();

		// new, dispose, properties
		static TRegularTransport New(Uint32 inProtocol, Uint32 inOptions = 0);
		static void Dispose(TRegularTransport inTpt);
		static void SetDataMonitor(TRegularTransport inTpt, TTransportMonitorProc inProc);
		
		// messaging
		static void SetMessageHandler(TRegularTransport inTpt, TMessageProc inProc, void *inContext = nil, void *inObject = nil);
		static void PostMessage(TRegularTransport inTpt, Uint32 inMsg, const void *inData = nil, Uint32 inDataSize = 0, Int16 inPriority = priority_Normal);
		static void ReplaceMessage(TRegularTransport inTpt, Uint32 inMsg, const void *inData = nil, Uint32 inDataSize = 0, Int16 inPriority = priority_Normal);

		// addresses
		static SIPAddress GetDefaultIPAddress();
		static void SetIPAddress(TRegularTransport inTpt, SIPAddress inIPAddress);
		static SIPAddress GetIPAddress(TRegularTransport inTpt);
		static Uint32 GetRemoteAddressText(TRegularTransport inTpt, void *outAddr, Uint32 inMaxSize);
		static Uint32 GetLocalAddressText(TRegularTransport inTpt, void *outAddr, Uint32 inMaxSize);
		static Uint32 GetLocalAddress(TRegularTransport inTpt, void *outAddr, Uint32 inMaxSize);
		static Uint32 GetProxyServerAddress(Uint16 inType, void *outProxyAddr, Uint32 inMaxSize, const void *inBypassAddr = nil, Uint32 inBypassAddrSize = 0);
		static bool GetEthernetAddress(SEthernetAddress& outEthernetAddr);

		// connecting and disconnecting
		static bool IsConnected(TRegularTransport inTpt);
		static void StartConnect(TRegularTransport inTpt, const void *inAddr, Uint32 inAddrSize, Uint32 inMaxSecs = 0);
		static void StartConnectThruFirewall(TRegularTransport inTpt, const void *inAddr, Uint32 inAddrSize, const void *inFirewallAddr, Uint32 inFirewallAddrSize, Uint32 inMaxSecs = 0, Uint32 inOptions = 0);
		static Uint16 GetConnectStatus(TRegularTransport inTpt);
		static void Disconnect(TRegularTransport inTpt);
		static void StartDisconnect(TRegularTransport inTpt);
		static bool IsDisconnecting(TRegularTransport inTpt);
		static bool IsConnecting(TRegularTransport inTpt);

		// incoming connection requests
		static void Listen(TRegularTransport inTpt, const void *inAddr, Uint32 inAddrSize);
		static TRegularTransport Accept(TRegularTransport inTpt, void *outAddr = nil, Uint32 *ioAddrSize = nil);
		
		// buffers
		static void *NewBuffer(Uint32 inSize);
		static void DisposeBuffer(void *inBuf);
		static Uint32 GetBufferSize(void *inBuf);

		// send and receive data
		static void SendBuffer(TRegularTransport inTpt, void *inBuf);
		static void Send(TRegularTransport inTpt, const void *inData, Uint32 inDataSize);
		static bool ReceiveBuffer(TRegularTransport inTpt, void*& outBuf, Uint32& outSize);
		static Uint32 Receive(TRegularTransport inTpt, void *outData, Uint32 inMaxSize);
		static Uint32 GetReceiveSize(TRegularTransport inTpt);
		static Uint32 GetUnsentSize(TRegularTransport inTpt);
		static void NotifyReadyToReceive(TRegularTransport inTpt, Uint32 inReceivedSize);
		static void NotifyReadyToSend(TRegularTransport inTpt);
		static bool IsReadyToReceive(TRegularTransport inTpt, Uint32 inReceivedSize);
		static bool IsReadyToSend(TRegularTransport inTpt);
		
		// connectionless send/receive data
		static void SendUnit(TRegularTransport inTpt, const void *inAddr, Uint32 inAddrSize, const void *inData, Uint32 inDataSize);
		static bool ReceiveUnit(TRegularTransport inTpt, void *outAddr, Uint32& ioAddrSize, void *outData, Uint32& ioDataSize);

		// global protocol stuff
		static bool IsRegisteredProtocol(const Uint8 inProtocol[]);
		static bool IsRegisteredForProtocol(const Uint8 inProtocol[]);
		static bool RegisterProtocol(const Uint8 inProtocol[]);
		static bool LaunchURL(const void *inText, Uint32 inTextSize, Uint32 *ioSelStart = nil, Uint32 *ioSelEnd = nil);
};


/*
 * URegularTransport Object Interface
 */

class TRegularTransportObj
{
	public:
		void SetDataMonitor(TTransportMonitorProc inProc)													{	URegularTransport::SetDataMonitor(this, inProc);											}
		
		void SetMessageHandler(TMessageProc inProc, void *inContext = nil, void *inObject = nil)			{	URegularTransport::SetMessageHandler(this, inProc, inContext, inObject);					}
		void PostMessage(Uint32 inMsg, const void *inData = nil, Uint32 inDataSize = 0, Int16 inPriority = priority_Normal)			{	URegularTransport::PostMessage(this, inMsg, inData, inDataSize, inPriority);	}
		void ReplaceMessage(Uint32 inMsg, const void *inData = nil, Uint32 inDataSize = 0, Int16 inPriority = priority_Normal)		{	URegularTransport::ReplaceMessage(this, inMsg, inData, inDataSize, inPriority);	}

		void SetIPAddress(SIPAddress inIPAddress)															{	URegularTransport::SetIPAddress(this, inIPAddress);											}
		SIPAddress GetIPAddress()																			{	return URegularTransport::GetIPAddress(this);												}
		Uint32 GetRemoteAddressText(void *outAddr, Uint32 inMaxSize)										{	return URegularTransport::GetRemoteAddressText(this, outAddr, inMaxSize);					}
		Uint32 GetLocalAddressText(void *outAddr, Uint32 inMaxSize)											{	return URegularTransport::GetLocalAddressText(this, outAddr, inMaxSize);					}
		Uint32 GetLocalAddress(void *outAddr, Uint32 inMaxSize)												{	return URegularTransport::GetLocalAddress(this, outAddr, inMaxSize);						}
		
		bool IsConnected()																					{	return URegularTransport::IsConnected(this);												}
		void StartConnect(const void *inAddr, Uint32 inAddrSize, Uint32 inMaxSecs = 0)						{	URegularTransport::StartConnect(this, inAddr, inAddrSize, inMaxSecs);						}
		void StartConnectThruFirewall(const void *inAddr, Uint32 inAddrSize, const void *inFirewallAddr, Uint32 inFirewallAddrSize, Uint32 inMaxSecs = 0, Uint32 inOptions = 0)	{	URegularTransport::StartConnectThruFirewall(this, inAddr, inAddrSize, inFirewallAddr, inFirewallAddrSize, inMaxSecs, inOptions);	}
		Uint16 GetConnectStatus()																			{	return URegularTransport::GetConnectStatus(this);											}
		void Disconnect()																					{	URegularTransport::Disconnect(this);														}
		void StartDisconnect()																				{	URegularTransport::StartDisconnect(this);													}
		bool IsDisconnecting()																				{	return URegularTransport::IsDisconnecting(this);											}
		bool IsConnecting()																					{	return URegularTransport::IsConnecting(this);												}

		void Listen(const void *inAddr, Uint32 inAddrSize)													{	URegularTransport::Listen(this, inAddr, inAddrSize);										}
		TRegularTransport Accept(void *outAddr = nil, Uint32 *ioAddrSize = nil)									{	return URegularTransport::Accept(this, outAddr, ioAddrSize);							}
		
		void SendBuffer(void *inBuf)																		{	URegularTransport::SendBuffer(this, inBuf);													}
		void Send(const void *inData, Uint32 inDataSize)													{	URegularTransport::Send(this, inData, inDataSize);											}
		bool ReceiveBuffer(void*& outBuf, Uint32& outSize)													{	return URegularTransport::ReceiveBuffer(this, outBuf, outSize);								}
		Uint32 Receive(void *outData, Uint32 inMaxSize)														{	return URegularTransport::Receive(this, outData, inMaxSize);								}
		Uint32 GetReceiveSize()																				{	return URegularTransport::GetReceiveSize(this);												}
		Uint32 GetUnsentSize()																				{	return URegularTransport::GetUnsentSize(this);												}
		void NotifyReadyToReceive(Uint32 inReceivedSize)													{	URegularTransport::NotifyReadyToReceive(this, inReceivedSize);								}
		void NotifyReadyToSend()																			{	URegularTransport::NotifyReadyToSend(this);													}
		bool IsReadyToReceive(Uint32 inReceivedSize)														{	return URegularTransport::IsReadyToReceive(this, inReceivedSize);							}
		bool IsReadyToSend()																				{	return URegularTransport::IsReadyToSend(this);												}

		void SendUnit(const void *inAddr, Uint32 inAddrSize, const void *inData, Uint32 inDataSize)			{	URegularTransport::SendUnit(this, inAddr, inAddrSize, inData, inDataSize);					}
		bool ReceiveUnit(void *outAddr, Uint32& ioAddrSize, void *outData, Uint32& ioDataSize)				{	return URegularTransport::ReceiveUnit(this, outAddr, ioAddrSize, outData, ioDataSize);		}

		void operator delete(void *p)																		{	URegularTransport::Dispose((TRegularTransport)p);											}
	protected:
		TRegularTransportObj() {}				// force creation via URegularTransport
};

